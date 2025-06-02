# README

## Team Members:
1. **Monika Kumari (210629)**
2. **Priya Gangwar (210772)**
3. **Ritam Acharya (210859)**

---

## 1. Assignment Features

### Implemented

1. **Iterative DNS Resolution**  
   - Starts from well-known root servers.  
   - Queries root → TLD → authoritative servers step by step.  
   - Extracts next-level nameservers from the Authority/Additional sections.  
   - Stops once it finds an A record (the final IP address) or fails if none can be found.

2. **Recursive DNS Resolution**  
   - Uses the system's (or configured) DNS resolver to handle recursion automatically.  
   - Simply queries NS records and A records for the domain, delegating the entire DNS resolution process to the local or system-defined resolver.

3. **Timeout and Error Handling**  
   - If a query to a particular nameserver times out or fails, the code reports an error.  
   - Iterative lookups move on to another server if available. Recursive lookups raise an exception.

4. **Logging & Debugging**  
   - Iterative mode prints debug messages at each step, such as which server it is querying, which NS records it extracts, etc.  
   - Recursive mode prints each record found, including NS and A records.

5. **Execution Timing**  
   - After each lookup (iterative or recursive), the script displays how many seconds the process took.

6. **Command-Line Interface**  
   - `python3 dnsresolver.py iterative <domain>`  
   - `python3 dnsresolver.py recursive <domain>`  

---

## 2. Overall Structure & Classes

Our code is largely contained in a single Python file, **`dnsresolver.py`**, but it conceptually has several logical sections. While not implemented as separate classes (due to Python’s module-based style), each section fulfills its own role:

1. **Global Variables & Constants**  
   - `ROOT_SERVERS`: A dictionary of known root server IPs.  
   - `TIMEOUT`: Timeout in seconds for DNS query attempts.

2. **Functions**  
   - **`send_dns_query(server, domain)`**: Sends a DNS UDP query (type A) to the specified `server` for `domain`.  
   - **`extract_next_nameservers(response)`**: Processes the Authority (and Additional) sections of a DNS response to gather nameservers’ hostnames and resolve them to IPs.  
   - **`iterative_dns_lookup(domain)`**: Implements the iterative approach, starting at the root servers and proceeding until an answer is found.  
   - **`recursive_dns_lookup(domain)`**: Relies on `dns.resolver` to fetch NS and A records in a fully recursive manner.

3. **Main Guard**  
   - The `if __name__ == "__main__": ...` block parses CLI arguments, chooses iterative or recursive mode, logs time taken, and prints final results or errors.

There is no additional “class-based” structure because Python’s dnspython library itself encapsulates much of the DNS logic. However, the above functions effectively serve separate sub-components of the assignment.

---

## 3. Design Decisions

1. **Iterative vs. Recursive Separation**  
   - We chose to keep these two resolution methods in **separate functions**, making the flow clearer and simpler to maintain.

2. **Nameserver Resolution Strategy**  
   - When performing iterative lookups, if the Additional section doesn’t provide IPs, we rely on Python’s built-in resolver (a minor recursion step) to convert the NS hostname into an IP. This was done for simplicity, acknowledging that a “pure” iterative approach would do it manually at each step.

3. **Timeout & Error Handling**  
   - We set a `TIMEOUT` of 3 seconds for UDP queries. If the server doesn’t respond, we return `None`, log an error, and attempt to pick the next server.

4. **Logging**  
   - Iterative queries include debug prints indicating the stage (ROOT/TLD/AUTH) and any IPs or hostnames found.

5. **Performance & Complexity**  
   - For iterative mode, each step queries one nameserver at a time. This is not heavily optimized, but for typical usage it’s sufficient and meets the assignment requirements.

---

## 4. Implementation Flow

1. **Iterative DNS Lookup**  
   1. Begin with the `ROOT_SERVERS` IP list.  
   2. Query each server in sequence until you receive a valid response.  
   3. If the response has an **Answer** section with an A record for the domain, you’re done.  
   4. Otherwise, parse the **Authority** and (if present) **Additional** sections to find new nameservers.  
   5. Resolve any NS hostnames to IPs if needed, update the server list, and repeat the query at the next “stage” (Root → TLD → Authoritative).  
   6. If no servers respond or provide an answer, error out.

2. **Recursive DNS Lookup**  
   1. Use `dns.resolver.resolve(domain, "NS")` to get the domain’s nameservers. Print them.  
   2. Use `dns.resolver.resolve(domain, "A")` to get the final IP address(es). Print them.  
   3. The system’s resolver handles all recursion behind the scenes.

3. **CLI & Timing**  
   - We parse the command-line arguments (`iterative` or `recursive`) and measure the execution time with `time.time()` calls.

---

## 5. Testing

### 5.1 Correctness Testing

- **Multiple Domains**  
  - Tried `google.com`, `iitk.ac.in`, `facebook.com`, `example.com`.  
- **Non-Existent Domains**  
  - Queried nonsense like `no-such-domain-1234.com`, expecting an error or NXDOMAIN.

### 5.2 Variation in DNS Behavior

- **Order of NS Records**  
  - Confirmed that records can appear in different orders on different runs, which is normal DNS behavior.  
- **Performance Checks**  
  - Observed minimal overhead for small domains. For iterative, a multi-step root → TLD → Auth chain can take a fraction of a second to a few seconds if network or servers are slow.

---

## 6. Restrictions

1. **Limited Root Servers**  
   - We only listed a handful of root servers in `ROOT_SERVERS`. If those fail or if your local network blocks them, the iterative approach may fail.
2. **Timeout**  
   - We set 3 seconds as a UDP query timeout. On poor connections or large latency, queries might fail prematurely.
3. **DNSSEC / IPv6**  
   - We have not implemented DNSSEC or IPv6 lookups; only standard IPv4 A records are resolved.

---

## 7. Challenges

1. **Handling Partial Answers**  
   - Sometimes the Additional section is empty, forcing us to do a separate step to resolve the NS hostnames.  
2. **Ensuring Correct Iterative Steps**  
   - The assignment's requirement to manually hop from root → TLD → authoritative means carefully extracting the right records each time.  
3. **Exception Handling**  
   - When a query times out or a server is unreachable, we gracefully move on to the next server.

---

## 8. Contribution Breakdown

1. **Monika Kumari (210629) – ~33%**  
   - Set up iterative DNS logic: `send_dns_query`, partial logic in `iterative_dns_lookup`.  
   - Wrote error and debug message outputs.  
   - Handled initial testing on known domains.

2. **Priya Gangwar (210772) – ~33%**  
   - Implemented `extract_next_nameservers`, ensuring we parse Authority & Additional.  
   - Integrated time measurement and final output formatting.  
   - Conducted tests on corner cases like non-existent domains.

3. **Ritam Acharya (210859) – ~34%**  
   - Implemented `recursive_dns_lookup` using `dns.resolver`.  
   - Cleaned up code structure, wrote final comments.  
   - Verified performance and wrote up detailed debugging instructions.

---

## 9. What Extra We Did Beyond Minimum Requirements

1. **Additional Logging**  
   - Provided thorough `[DEBUG]` statements (e.g., printing every newly discovered nameserver).  
2. **Error Messages**  
   - On iterative lookup failure, we display which stage or server IP timed out for easier troubleshooting.  
3. **Resolving NS Names**  
   - Even if the Additional section is missing IPs, the code attempts system-level resolution for each NS hostname to proceed.  


---

## 10. Sources

- **dnspython Official Documentation**: [https://www.dnspython.org/](https://www.dnspython.org/)  
- **Lecture Notes** from CS425, specifically the sections on DNS hierarchical architecture.  
- **“Computer Networking: A Top-Down Approach”** by James Kurose and Keith Ross for DNS fundamentals.

---

## 11. Declaration

We declare that all the work presented here is our own, and we have not indulged in any plagiarism or unauthorized collaboration beyond our team.

---

## 12. Acknowledgment

We would like to thank **Prof. Adithya Vadapalli** for providing the guidance and skeleton code for this DNS assignment.
