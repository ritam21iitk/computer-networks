import dns.message
import dns.query
import dns.rdatatype
import dns.resolver
import time

# Root DNS servers used to start the iterative resolution process
ROOT_SERVERS = {
    "198.41.0.4": "Root (a.root-servers.net)",
    "199.9.14.201": "Root (b.root-servers.net)",
    "192.33.4.12": "Root (c.root-servers.net)",
    "199.7.91.13": "Root (d.root-servers.net)",
    "192.203.230.10": "Root (e.root-servers.net)"
}

TIMEOUT = 3  # Timeout in seconds for each DNS query attempt

def send_dns_query(server, domain):
    """ 
    Sends a DNS query (for an A record) to the given server using UDP.
    Returns the response if successful, otherwise returns None.
    """
    try:
        query = dns.message.make_query(domain, dns.rdatatype.A)  # Construct the DNS query
        # TODO: Send the query using UDP 
        response = dns.query.udp(query, server, timeout=TIMEOUT)
        return response
    except Exception:
        return None  # If an error occurs (timeout, unreachable server, etc.), return None

def extract_next_nameservers(response):
    """ 
    Extracts NS (nameserver) records from the Authority section
    and resolves each NS name to its IP address (by a quick system resolver call).
    Returns a list of IPs of those next authoritative nameservers.
    """
    ns_ips = []   
    ns_names = [] #
    
    # Loop through the AUTHORITY section to extract NS records
    # (i.e., the domain names of the next nameservers)
    if response.authority:
        for rrset in response.authority:
            if rrset.rdtype == dns.rdatatype.NS:
                for rr in rrset:
                    ns_name = rr.to_text()
                    ns_names.append(ns_name)
                    print(f"Extracted NS hostname: {ns_name}")

    # Alternatively, sometimes the next nameserver IPs may appear
    # in the ADDITIONAL section as A records. If they do, we can
    # collect them directly. However, we’ll also attempt direct
    # resolution if needed.

    # Check the ADDITIONAL section for direct A records matching the NS name
    # (This can save time if the DNS server already provided them.)
    if response.additional:
        for rrset in response.additional:
            if rrset.rdtype == dns.rdatatype.A:
                # The name of this rrset is the NS hostname
                hostname = rrset.name.to_text()
                for rdata in rrset:
                    # If the hostname is indeed in our ns_names list, we can store it
                    if hostname in ns_names:
                        ip_addr = rdata.to_text()
                        ns_ips.append(ip_addr)
                        print(f"Resolved {hostname} to {ip_addr} (from ADDITIONAL)")

    # For any NS names that were not found in the ADDITIONAL section,
    # do a quick resolution with the system’s default resolver.
    # (In a fully “pure” iterative approach, we’d do lookups carefully
    #  at each step, but for brevity we’ll rely on dns.resolver here.)
    for ns_name in ns_names:
        # If not already in ns_ips, try system resolver
        # so we at least have one IP for it
        already_found = False
        for ip in ns_ips:
            # We do a naive check if the name is part of the line
            # or if we already resolved it
            # (In practice, you would match them more robustly.)
            if ns_name in ip:
                already_found = True

        if not already_found:
            try:
                answers = dns.resolver.resolve(ns_name, 'A')
                for rdata in answers:
                    ns_ip = rdata.to_text()
                    ns_ips.append(ns_ip)
                    print(f"Resolved {ns_name} to {ns_ip} (via system resolver)")
            except:
                pass  # If we fail to resolve, ignore

    return ns_ips

def iterative_dns_lookup(domain):
    """ 
    Performs an iterative DNS resolution starting from the root servers.
    It queries root servers, then TLD servers, then authoritative servers,
    following the hierarchy until an answer is found or resolution fails.
    """
    print(f"[Iterative DNS Lookup] Resolving {domain}")

    next_ns_list = list(ROOT_SERVERS.keys())  # Start with the root server IPs
    stage = "ROOT"  # Track resolution stage (ROOT -> TLD -> AUTH)

    while next_ns_list:
        ns_ip = next_ns_list[0]  # Pick the first available nameserver to query
        response = send_dns_query(ns_ip, domain)
        
        if response: 
            print(f"[DEBUG] Querying {stage} server ({ns_ip}) - SUCCESS")
            
            # If an answer is found, print and return
            if response.answer:
                # Usually the A record is in response.answer[0]. 
                print(f"[SUCCESS] {domain} -> {response.answer[0][0]}")
                return
            
            # If no answer, we attempt to find the next name servers from the Authority section.
            next_ns_list = extract_next_nameservers(response)

            # TODO: Move to the next resolution stage
            if stage == "ROOT":
                stage = "TLD"
            elif stage == "TLD":
                stage = "AUTH"
            else:
                stage = "AUTH"  # Stay on AUTH in case we need multiple queries

        else:
            print(f"[ERROR] Query failed for {stage} server {ns_ip}")
            return  # Stop resolution if a query fails
    
    print("[ERROR] Resolution failed.")  # If no nameservers remain or everything fails

def recursive_dns_lookup(domain):
    """ 
    Performs recursive DNS resolution using the system's default resolver.
    This approach delegates the entire resolution process to the local (or configured) resolver.
    """
    print(f"[Recursive DNS Lookup] Resolving {domain}")
    try:
        # TODO: Perform recursive resolution using the system's DNS resolver
        # First, get any NS records for the domain
        answer_ns = dns.resolver.resolve(domain, "NS")
        for rdata in answer_ns:
            print(f"[SUCCESS] {domain} -> {rdata.target}")

        # Then get the A records for the domain
        answer_a = dns.resolver.resolve(domain, "A")
        for rdata in answer_a:
            print(f"[SUCCESS] {domain} -> {rdata}")
    except Exception as e:
        print(f"[ERROR] Recursive lookup failed: {e}")  # Handle resolution failure

if __name__ == "__main__":
    import sys
    if len(sys.argv) != 3 or sys.argv[1] not in {"iterative", "recursive"}:
        print("Usage: python3 dnsresolver.py <iterative|recursive> <domain>")
        sys.exit(1)

    mode = sys.argv[1]  # Get mode (iterative or recursive)
    domain = sys.argv[2]  # Get domain to resolve
    start_time = time.time()  # Record start time
    
    # Execute the selected DNS resolution mode
    if mode == "iterative":
        iterative_dns_lookup(domain)
    else:
        recursive_dns_lookup(domain)
    
    print(f"Time taken: {time.time() - start_time:.3f} seconds")
