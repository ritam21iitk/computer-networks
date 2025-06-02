// client.cpp
/**
 * TCP Three-Way Handshake Client using Raw Sockets
 * 
 * This program performs a simplified TCP three-way handshake by:
 * 1. Sending a SYN packet with sequence number 200
 * 2. Receiving a SYN-ACK with sequence number 400 (expected)
 * 3. Sending the final ACK with sequence number 600
 *
 * Notes:
 * - Designed for educational use, primarily for localhost (127.0.0.1)
 * - Sequence numbers (200, 400, 600) are predefined for controlled testing
 * - Raw socket manipulation bypasses kernel TCP/IP stack
 */

 #include <iostream>
 #include <cstring>
 #include <cstdlib>
 #include <unistd.h>
 #include <netinet/ip.h>
 #include <netinet/tcp.h>
 #include <arpa/inet.h>
 #include <sys/socket.h>
 #include <sys/time.h>
 #include <vector>
 #include <thread>
 #include <chrono>
 
 // ---------------- Constants & Configuration ----------------
 #define DEFAULT_DEST_PORT 12345           ///< Port server listens on
 #define DEFAULT_SRC_PORT 54321            ///< Client source port
 #define DEFAULT_SERVER_IP "127.0.0.1"     ///< Default server IP for localhost tests
 #define DEFAULT_CLIENT_IP "127.0.0.1"     ///< Use the loopback interface as source IP
 #define CLIENT_SEQ_NUM 200                ///< Initial client sequence number (SYN)
 #define CLIENT_FINAL_SEQ 600              ///< Final ACK packet sequence number
 #define SERVER_EXPECTED_SEQ 400           ///< Expected SYN-ACK server sequence
 #define TIMEOUT_SECONDS 5                 ///< Timeout for SYN-ACK reception
 #define SEND_BUFFER_SIZE 4096             ///< Max size for outgoing datagram
 #define RECV_BUFFER_SIZE 65536            ///< Max size for incoming datagram
 #define MAX_RETRY 3                       ///< Max retries on failure
 
 // ---------------- TCP Pseudo Header ----------------
 struct pseudo_header {
     u_int32_t source_address;
     u_int32_t dest_address;
     u_int8_t placeholder;
     u_int8_t protocol;
     u_int16_t tcp_length;
 };
 
 /**
  * Computes the Internet checksum (used for IP/TCP).
  * @param ptr Pointer to data buffer
  * @param nbytes Number of bytes to checksum
  * @return Computed checksum value
  */
 unsigned short compute_checksum(unsigned short *ptr, int nbytes) {
     long sum = 0;
     while (nbytes > 1) {
         sum += *ptr++;
         nbytes -= 2;
     }
     if (nbytes) sum += *(unsigned char *)ptr;
     sum = (sum >> 16) + (sum & 0xffff);
     sum += (sum >> 16);
     return (unsigned short)(~sum);
 }
 
 /**
  * Constructs and sends a TCP packet with given parameters.
  * @param sockfd Raw socket descriptor
  * @param saddr Source IP address (uint32)
  * @param daddr Destination IP address (uint32)
  * @param seq Sequence number to use
  * @param ack_seq Acknowledgement number to use
  * @param syn SYN flag value (bool)
  * @param ack ACK flag value (bool)
  * @param raw_seq For display/logging
  * @param raw_ack For display/logging
  * @param src_port Source port number
  * @param dest_port Destination port number
  */
 void send_tcp_packet(int sockfd, uint32_t saddr, uint32_t daddr, uint32_t seq, uint32_t ack_seq,
                      bool syn, bool ack, int raw_seq, int raw_ack, uint16_t src_port, uint16_t dest_port) {
     char datagram[SEND_BUFFER_SIZE];
     memset(datagram, 0, sizeof(datagram));
 
     struct iphdr *iph = (struct iphdr *)datagram;
     struct tcphdr *tcph = (struct tcphdr *)(datagram + sizeof(struct iphdr));
     struct sockaddr_in dest;
     pseudo_header psh;
 
     iph->ihl = 5;
     iph->version = 4;
     iph->tos = 0;
     iph->tot_len = htons(sizeof(struct iphdr) + sizeof(struct tcphdr));
     iph->id = htons(54321);
     iph->frag_off = 0;
     iph->ttl = 64;
     iph->protocol = IPPROTO_TCP;
     iph->saddr = saddr;
     iph->daddr = daddr;
     iph->check = compute_checksum((unsigned short *)datagram, iph->ihl * 4);
 
     tcph->source = htons(src_port);
     tcph->dest = htons(dest_port);
     tcph->seq = htonl(seq);
     tcph->ack_seq = htonl(ack_seq);
     tcph->doff = 5;
     tcph->syn = syn ? 1 : 0;
     tcph->ack = ack ? 1 : 0;
     tcph->window = htons(5840);
     tcph->check = 0;
 
     psh.source_address = saddr;
     psh.dest_address = daddr;
     psh.placeholder = 0;
     psh.protocol = IPPROTO_TCP;
     psh.tcp_length = htons(sizeof(struct tcphdr));
 
     std::vector<unsigned char> pseudogram(sizeof(pseudo_header) + sizeof(struct tcphdr));
     memcpy(&pseudogram[0], &psh, sizeof(pseudo_header));
     memcpy(&pseudogram[sizeof(pseudo_header)], tcph, sizeof(struct tcphdr));
     tcph->check = compute_checksum((unsigned short *)&pseudogram[0], pseudogram.size());
 
     dest.sin_family = AF_INET;
     dest.sin_port = htons(dest_port);
     dest.sin_addr.s_addr = daddr;
 
     if (sendto(sockfd, datagram, ntohs(iph->tot_len), 0, (struct sockaddr *)&dest, sizeof(dest)) < 0) {
         perror("sendto failed");
     } else {
         std::cout << "[+] Packet Sent - SYN: " << syn
                   << " ACK: " << ack
                   << " SEQ: " << raw_seq
                   << " ACK_SEQ: " << raw_ack << std::endl;
     }
 }
 
 /**
  * Waits for a valid SYN-ACK response from the server.
  * @param sockfd Raw socket descriptor
  * @param server_seq Sequence number received from server
  * @param expected_src_port Port server should be responding from
  * @return true if valid SYN-ACK received, false otherwise
  */
 bool wait_for_syn_ack(int sockfd, uint32_t &server_seq, uint16_t expected_src_port) {
     char buffer[RECV_BUFFER_SIZE];
     struct sockaddr saddr;
     socklen_t saddr_len = sizeof(saddr);
 
     struct timeval timeout = {TIMEOUT_SECONDS, 0};
     setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
 
     while (true) {
         int data_size = recvfrom(sockfd, buffer, sizeof(buffer), 0, &saddr, &saddr_len);
         if (data_size < 0) {
             perror("recvfrom() failed or timed out");
             return false;
         }
 
         if (data_size < (int)(sizeof(struct iphdr) + sizeof(struct tcphdr))) {
             std::cerr << "[-] Packet too small, skipping.\n";
             continue;
         }
 
         struct iphdr *iph = (struct iphdr *)buffer;
         if (iph->protocol == IPPROTO_TCP) {
             struct tcphdr *tcph = (struct tcphdr *)(buffer + iph->ihl * 4);
 
             if (ntohs(tcph->dest) == DEFAULT_SRC_PORT && ntohs(tcph->source) == expected_src_port &&
                 tcph->syn == 1 && tcph->ack == 1 && ntohl(tcph->ack_seq) == CLIENT_SEQ_NUM + 1) {
                 server_seq = ntohl(tcph->seq);
                 std::cout << "[+] Received SYN-ACK with SEQ: " << server_seq
                           << " ACK_SEQ: " << ntohl(tcph->ack_seq) << std::endl;
                 if (server_seq != SERVER_EXPECTED_SEQ) {
                     std::cerr << "[-] Unexpected server SEQ. Expected: " << SERVER_EXPECTED_SEQ << std::endl;
                     return false;
                 }
                 return true;
             }
         }
     }
 }
 
 // ---------------- Main Function ----------------
 int main(int argc, char *argv[]) {
     const char *dest_ip = (argc > 1) ? argv[1] : DEFAULT_SERVER_IP;
 
     int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
     if (sockfd < 0) {
         perror("Socket creation failed");
         return 1;
     }
 
     int one = 1;
     if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
         perror("Error setting IP_HDRINCL");
         return 1;
     }
 
     uint32_t saddr = inet_addr(DEFAULT_CLIENT_IP);
     uint32_t daddr = inet_addr(dest_ip);
 
     // Step 1: Send SYN with sequence 200
     send_tcp_packet(sockfd, saddr, daddr, CLIENT_SEQ_NUM, 0, true, false,
                     CLIENT_SEQ_NUM, 0, DEFAULT_SRC_PORT, DEFAULT_DEST_PORT);
 
     // Step 2: Retry SYN-ACK reception if needed
     uint32_t server_seq;
     bool success = false;
     for (int i = 0; i < MAX_RETRY; ++i) {
         if (wait_for_syn_ack(sockfd, server_seq, DEFAULT_DEST_PORT)) {
             success = true;
             break;
         } else {
             std::cerr << "[!] Retry " << (i + 1) << " of " << MAX_RETRY << std::endl;
             std::this_thread::sleep_for(std::chrono::seconds(1));
         }
     }
 
     if (!success) {
         std::cerr << "[-] Failed to complete handshake. Please check server status and network configuration." << std::endl;
         close(sockfd);
         return 1;
     }
 
     // Step 3: Final ACK with sequence 600
     send_tcp_packet(sockfd, saddr, daddr, CLIENT_FINAL_SEQ, server_seq + 1, false, true,
                     CLIENT_FINAL_SEQ, server_seq + 1, DEFAULT_SRC_PORT, DEFAULT_DEST_PORT);
 
     close(sockfd);
     return 0;
 }
 