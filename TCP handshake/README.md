# README

## Team Members:

1. **Monika Kumari (210629)**
2. **Priya Gangwar (210772)**
3. **Ritam Acharya (210859)**

---

## 1. Assignment Features

### Implemented

1. **Client-Side TCP Handshake with Raw Sockets**

   - Constructs TCP packets with SYN, SYN-ACK, and ACK flags using raw sockets.
   - Sends SYN packet with SEQ=200.
   - Validates SYN-ACK from server (SEQ=400, ACK=201).
   - Sends final ACK with SEQ=600.

2. **Checksum and Header Construction**

   - Manual construction of IP and TCP headers.
   - Custom implementation of the Internet checksum algorithm.

3. **Timeout and Retry Handling**

   - SYN-ACK wait with 5-second timeout.
   - Retries handshake up to 3 times if response is invalid or missing.

4. **Logging & Debugging**

   - Console outputs for TCP flags, SEQ/ACK values.
   - Explicit error reporting and retry indicators.

5. **Makefile for Easy Compilation**

   - Provided by instructor. Use `make` to compile both server and client.

---

## 2. Overall Structure & Files

- `client.cpp`: Client-side TCP three-way handshake using raw sockets.  
- `server.cpp`: Provided server code that replies with SYN-ACK and receives final ACK.  
- `Makefile`: Provided build script for compilation.

---

## 3. Quick Run Instructions

```bash
make              # Build server and client
sudo ./server     # Run server in terminal 1
sudo ./client     # Run client in terminal 2
```

---

## 4. Design Decisions

1. **Why Raw Sockets?**

   - Raw sockets bypass the OS TCP/IP stack, offering control over packet-level behavior.
   - Ideal for educational experiments that demonstrate how TCP handshakes function internally.

2. **Static Sequence Numbers**

   - Used for predictable, reproducible validation against a stateless server (200, 400, 600).

3. **Minimal Stateful Logic**

   - Stateless server avoids maintaining connections. Server exits after one successful handshake.

4. **Validation of SYN-ACK**

   - Ensures received SYN-ACK has correct flags and sequence (SEQ=400, ACK=201).

---

## 5. Implementation Flow

### TCP Handshake Diagram

```text
Client                            Server
  |                                 |
  |--- SYN (SEQ=200) -------------> |
  |                                 |
  |<-- SYN-ACK (SEQ=400, ACK=201) --|
  |                                 |
  |--- ACK (SEQ=600, ACK=401) ----> |
  |                                 |
```

### Client Execution

- Create raw socket with IP_HDRINCL.
- Send SYN packet.
- Wait for SYN-ACK; validate fields.
- Retry up to 3 times if timeout/error.
- Send final ACK to complete handshake.

### Server Execution

- Listen for SYN packets on port 12345.
- Reply with SYN-ACK (SEQ=400).
- Wait for ACK (SEQ=600); log success.

---

## 6. Testing

### 6.1 Correctness Testing

```bash
sudo ./server
sudo ./client
```

Expected output:
```
[+] Packet Sent - SYN: 1 ACK: 0 SEQ: 200 ACK_SEQ: 0
[+] Received SYN-ACK with SEQ: 400 ACK_SEQ: 201
[+] Packet Sent - SYN: 0 ACK: 1 SEQ: 600 ACK_SEQ: 401
```

### 6.2 Edge Case Testing

| Test Case                        | Expected Result                     |
|----------------------------------|-------------------------------------|
| Invalid server SEQ               | Client rejects SYN-ACK              |
| Server crash after SYN-ACK       | Client retries 3x then exits        |
| Malformed packet (too small)     | Packet ignored                      |
| Wireshark capture on lo          | TCP flags and SEQs match expected   |

> **Note:** Wireshark confirms correct TCP flag and header usage on the loopback interface.

---

## 7. Restrictions

- Requires root privileges (due to raw socket usage).
- Linux-only (tested on Ubuntu 22.04).
- Only tested on `127.0.0.1`; real-network testing may need additional firewall tweaks.

---

## 8. Challenges

- Calculating checksums without OS stack.
- Debugging raw packets with no kernel-level protection.
- Understanding TCP/IP structures and bit-fields.
- Timing retries without introducing congestion.

---

## 9. Contribution Breakdown

The team focused exclusively on developing `client.cpp` and `README.md`. Server code and Makefile were provided by course staff.

1. **Monika Kumari (33%)**  
   - Wrote this `README.md`, test table, and diagrams.
   - Developed sequence/ACK handling and retry loop.

2. **Priya Gangwar (33%)**  
   - Implemented `compute_checksum()` and pseudogram logic.
   - Enhanced robustness via error-checking and retry wait.

3. **Ritam Acharya (34%)**  
   - Wrote TCP/IP packet construction logic in `client.cpp`.
   - Integrated console logging, debugging outputs, and comments.

---

## 10. What Extra We Did Beyond Minimum Requirements

- Added retry with timeout for SYN-ACK recovery.
- Created a detailed testing matrix and visual handshake diagram.
- Used modular function design for clarity and grading ease.

---

## 11. Future Enhancements

- Support for dynamic sequence numbers (like real TCP).
- Add IPv6 compatibility and checksum.
- Extend to 4-way FIN termination and concurrent sessions.
- Export logs as JSON for automated grading.

---

## 12. Sources

- [Assignment repo](https://github.com/privacy-iitk/cs425-2025)
- Beej's Guide to Network Programming
- Linux Raw Socket Examples (StackOverflow)

---

## 13. Declaration

We declare that all the work presented here is our own, and we have not indulged in any plagiarism or unauthorized collaboration beyond our team.

---

## 14. Acknowledgment

We would like to thank **Prof. Adithya Vadapalli** for his guidance on TCP internals and providing the server code.
