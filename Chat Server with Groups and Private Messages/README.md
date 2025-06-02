# README

## Team Members:
1. Monika Kumari (210629)
2. Priya Gangwar (210772)
3. Ritam Acharya (210859)

---

## 1. Assignment Features

### Implemented

1. **User Authentication**  
   - Validates against entries in `users.txt` using the format `username:password`.
   - Disconnects clients who fail authentication.

2. **Broadcast Messages**  
   - Command: `/broadcast <message>`  
   - Sends a message to all **connected** users, except the sender.

3. **Private Messages**  
   - Command: `/msg <username> <message>`  
   - Sends a **direct message** to a specific user.

4. **Group Functionality**  
   - **Create**: `/create_group <group_name>`  
   - **Join**: `/join_group <group_name>`  
   - **Leave**: `/leave_group <group_name>`  
   - **Group Message**: `/group_msg <group_name> <message>`  
   - Uses a `GroupManager` class to handle group membership and messaging.

5. **Multithreading**  
   - The server listens on a designated port (default: 12345).
   - Each client connection is handled by a **separate thread** for concurrency.

6. **Server-Side Cleanup**  
   - When a client disconnects, the server removes them from the active clients map and from all groups.

---

## 2. Overall Structure & Classes

The codebase is primarily divided into the following classes. Each class addresses a specific part of the server's functionality.

1. **`ServerManager`**
   - **Purpose**:  
     - Owns the main server socket.
     - Accepts incoming client connections and spawns threads.
     - Maintains a global `clients` map: `socket -> username`.
     - Loads user credentials from `users.txt`.
   - **Key Methods**:  
     - `start()`: sets up the listening socket and enters the accept loop.  
     - `handle_client(int client_socket)`: per-client thread function. Handles authentication, message parsing, and dispatching to other managers.  

2. **`GroupManager`**
   - **Purpose**:  
     - Manages all group-related actions: create, join, leave, and group messaging.
     - Tracks group membership in `std::unordered_map<std::string, std::unordered_set<int>>`.
   - **Key Methods**:  
     - `create_group(int socket, const std::string& username, const std::string& group_name)`: Creates a new group (if not already present).  
     - `join_group(int socket, const std::string& username, const std::string& group_name)`: Adds a client socket to an existing group.  
     - `leave_group(int socket, const std::string& username, const std::string& group_name)`: Removes a client from a group.  
     - `send_group_message(int socket, const std::string& username, const std::string& group_name, const std::string& message)`: Sends a message to all members in a group.  
     - `remove_socket_from_all_groups(int socket)`: Cleans up group memberships when a user disconnects.

3. **`BroadcastMessage`**
   - **Purpose**:  
     - Broadcast a message to **all connected users**, excluding the sender.
   - **Key Methods**:  
     - `send_broadcast(int sender_socket, const std::string& message)`:  
       Sends a message with the format `[Broadcast from <sender_username>]: <message>` to every active user.

4. **`PrivateMessage`**
   - **Purpose**:  
     - Handles direct (one-to-one) communication between two users.
   - **Key Method**:  
     - `send_private_message(int client_socket, const std::string& recipient, const std::string& message)`: Finds the recipient by username and sends a private message.

5. **`ErrorHandler`**
   - **Purpose**:  
     - Centralizes error-related messages and behaviors.  
     - Sends error strings to the client or logs if needed (e.g., authentication failures).

---

## 3. Design Decisions

### 3.1 Concurrency Model

- **Thread per Client**:  
  - Each client connection is handled by a separate thread, created upon `accept()`.  
  - Pros: Simpler to implement and reason about. Good for moderate-scale concurrency.  
  - Cons: For very high concurrency (thousands of clients), an event-driven or thread-pool model might be more scalable.

### 3.2 Data Structures & Synchronization

- **Shared Maps**:
  1. **`clients`**: `std::unordered_map<int, std::string>`  
     - Key: client’s socket descriptor  
     - Value: username
  2. **`groups`** (in `GroupManager`): `std::unordered_map<std::string, std::unordered_set<int>>`  
     - Key: group name  
     - Value: set of member sockets

- **Mutex Usage**:
  - We protect each shared structure with a `std::mutex` (e.g., `clients_mutex` in the server).  
  - Whenever a thread modifies or reads these structures, it acquires a lock guard to prevent data races.

### 3.3 Message Parsing

- **Command Parsing**:
  - Each message received is compared to known prefixes (`/broadcast`, `/msg`, `/create_group`, etc.).
  - Arguments (like `username`, `group_name`, or the actual message) are extracted by `find(' ')` operations.
  - This approach is simple string-based parsing.

### 3.4 Complexity Considerations

- **Time Complexity**:
  - **Broadcast**: O(N) in the worst case, where N = number of connected clients.  
  - **Private Message**: O(N) to find the recipient in the `clients` map if you search by username; or O(1) if you invert that mapping. Currently, we do a linear search.  
  - **Group Operations**:  
    - Creating a group: O(1) to insert into `std::unordered_map`.  
    - Joining/Leaving: O(1) average to insert/erase in a `std::unordered_set`.  
    - Group Messaging: O(k) where k = number of members in the group.  
- **Space Complexity**:
  - In memory, each client uses an entry in `clients`, each group uses an entry in `groups`, etc.  
  - Overall memory depends on the maximum number of concurrent connections and groups.  

---

## 4. Implementation Flow

1. **Server Boot-Up**  
   - `ServerManager::start()`:  
     1. Load `users.txt` into a map `users[username] = password`.  
     2. Create listening socket.  
     3. While true:  
        - `accept()` a new client.  
        - Create a new thread → `handle_client(socket)`.  

2. **Client Handling**  
   - **Authentication**:  
     1. Prompt for username/password.  
     2. Compare with loaded credentials. If match, continue; otherwise disconnect.  
   - **Main Command Loop**:  
     - Reads client message, checks for known commands:  
       - `/broadcast` → call `BroadcastMessage::send_broadcast`.  
       - `/msg` → call `PrivateMessage::send_private_message`.  
       - `/create_group`, `/join_group`, `/leave_group`, `/group_msg` → call corresponding methods in `GroupManager`.  
     - On client disconnect or `/exit`, remove from `clients`, remove from all groups.

3. **Group Operations**  
   - Single `GroupManager` for the entire server.  
   - **Create**: add empty set with group name, add creating socket.  
   - **Join**: add client socket to existing group set.  
   - **Leave**: remove client socket from group set.  
   - **Send Group Message**: iterate over group's set of sockets, send to each.

---

## 5. Testing

### 5.1 Correctness Testing

- **Multiple Terminal Sessions**:
  - Launched several `./client_grp` instances manually.
  - Verified user login, broadcast, private messaging, group creation, joining, and leaving.
- **Edge Cases**:
  - Invalid username/password → immediate disconnect.
  - Non-existent group join → error message.
  - Duplicate group creation → error message.
  - Large messages (over 1024 bytes) → truncated or partial read.

### 5.2 Stress Testing

- **Automated Script**:
  - We provided a `stress_test.cpp` that spawns multiple simulated clients, each randomly executing broadcast, private messages, group commands, etc.
  - Checked for concurrency issues and potential deadlocks or crashes.
- **Memory / CPU Observations**:
  - Verified the server remains stable under multiple parallel connections.

---

## 6. Restrictions

- **Max Clients**: Defined by `#define MAX_CLIENTS 10` in the listen queue. You can increase it if needed.  
- **Max Groups**: Not explicitly enforced; limited by memory.  
- **Max Group Members**: Also limited by memory; no fixed upper bound.  
- **Max Message Size**: Hard-coded buffer of 1024 bytes (`BUFFER_SIZE`).

---

## 7. Challenges

1. **Thread Synchronization**  
   - Ensuring correct lock usage to avoid race conditions.  
   - Shared data structures were prone to concurrency issues if not handled carefully.
2. **Command Parsing**  
   - Splitting strings correctly for `/msg <user> <message>`, or `/group_msg <group> <message>`.
3. **Group Consistency**  
   - Making sure a single `GroupManager` was used server-wide, so changes reflect for all clients.
4. **Debugging**  
   - Race conditions can be intermittent. Logging and stress tests were crucial to catch these.

---

## 8. Contribution Breakdown


- **210629 Monika Kumari (34%)**  
  - Implemented the threading logic in `ServerManager::start()` and `handle_client()`.  
  - Added authentication checks and broadcast functionality.  
  - Performed early debugging.
  
- **210772 Priya Gangwar (33%)**  
  - Implemented `GroupManager` (create, join, leave, group_msg).  
  - Refined error handling and user feedback messages.  
  - Wrote part of the manual test cases.

- **210859 Ritam Acharya (33%)**  
  - Developed `PrivateMessage` and integrated the final code.  
  - Conducted edge case testing (invalid commands, group not found, etc.).  
  - Prepared the `README.md`, set up the stress testing (`stress_test.cpp`), verified concurrency, and compiled final deliverables.

---
## 9. What Extra we Did beyond minimum Requirements

1. **Join/Leave Notifications**  
   - Whenever a user joins or leaves a group, all existing group members receive a notification like:  
     `"[Group <group_name>] <username> has joined."`  
     `"[Group <group_name>] <username> has left."`  

2. **/exit Command**  
   - Allows clients to gracefully close their session by typing `/exit`. The server cleans up resources accordingly.

3. **Comprehensive Error Handling**  
   - Centralized in an `ErrorHandler` class, sending user-friendly error messages for unknown commands, non-existent groups, inactive users, etc.

4. **Detailed Stress Testing**  
   - We added a dedicated **`stress_test.cpp`** that spawns multiple simulated clients randomly issuing commands, testing concurrency, stability, and performance.

5. **Well-Organized Classes**  
   - We separated functionalities into multiple classes (`ServerManager`, `GroupManager`, `BroadcastMessage`, `PrivateMessage`, `ErrorHandler`) rather than a single monolithic file. This improves maintainability and readability.

6. **Announcements on Chat Join/Leave**  
   - Broadcasts a global message like `"<username> has joined the chat."` or `"<username> has left the chat."` so every user sees the entrance/exit of others.
---

## 10. Sources

- "Computer Networking: A Top-Down Approach" by Jim Kurose and Keith Ross

---

## 11. Declaration

We declare that all the work presented here is our own and we have not indulged in plagiarism.

---

## 12. Acknowledgment

We are grateful to Prof. Adithya Vadapalli; without his guidance and help, we couldn't have completed this assignment.

