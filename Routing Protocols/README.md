
# README

## Team Members:
1. **Priya Gangwar (210772)**
2. **Monika Kumari (210629)**
3. **Ritam Acharya (210859)**

---

## 1. Assignment Features

### Implemented
- Two core routing algorithms were implemented:
  - **Distance Vector Routing (DVR)** using the Bellman-Ford update logic.
  - **Link State Routing (LSR)** using Dijkstra’s algorithm.
- The simulation:
  - Reads an adjacency matrix input from a text file.
  - Simulates both routing algorithms independently.
  - Outputs the computed routing tables for each node.
- Efficient handling of infinite distances (`9999`) and unreachable nodes.

---

## 2. Overall Structure & Files
- **routing_sim.cpp**: Contains the full logic for both DVR and LSR algorithms.
- **inputfile.txt**: Input adjacency matrix for the graph.
- **Makefile**: Compiles `routing_sim.cpp` to `routing_sim`.

---

## 3. Design Decisions
- Used standard `C++ STL` containers (`vector`, `queue`) for clean and safe memory management.
- Defined `RoutingEntry` struct to abstract individual routing table entries.
- Used separate functions for each algorithm to allow easy testing and debugging.
- Assumed symmetric (undirected) graphs for simplicity.

---

## 4. Implementation Flow
1. The adjacency matrix is read from `inputfile.txt`.
2. **DVR**:
   - Each node initializes its table based on direct neighbors.
   - Updates are propagated iteratively using Bellman-Ford logic until convergence.
3. **LSR**:
   - Each node runs Dijkstra’s algorithm to compute the shortest path to all other nodes.
   - Next hops are resolved using backtracking via the `prev[]` array.
4. The routing tables for each node are printed separately for both algorithms.

---

## 5. Testing

### 5.1 Correctness Testing
- The input matrix was tested on multiple network topologies (4-node, 5-node).
- Verified:
  - Consistent routing tables between algorithms for symmetric graphs.
  - Correct next hops and distances.
  - Proper handling of unreachable nodes (`INF`).

### 5.2 Edge Cases
- Nodes with no direct neighbors.
- Fully connected graphs.
- Graphs with long indirect paths.

---

### Compilation

To build the simulator, run:

```bash
make
```
This compiles `routing_sim.cpp` into the `routing_sim` executable.

### Usage

Run the simulator with an input file:

```bash
./routing_sim input1.txt
```

---

## Input Format

- The first line: an integer $$ n $$, the number of nodes.
- Next $$ n $$ lines: $$ n $$ space-separated integers per line, representing the adjacency matrix.
  - `0` for self-loops (cost from node to itself)
  - Positive integer for link cost
  - `9999` for unreachable links

**Example (`input1.txt`):**
```
4
0 10 100 30
10 0 20 40
100 20 0 10
30 40 10 0
```

---

## Output Format

- **DVR:** Routing tables for each node after every iteration, showing destination, cost, and next hop.
- **LSR:** Routing tables for each node after running Dijkstra's algorithm.

**Sample Output:**
```
--- Distance Vector Routing Simulation ---
--- DVR Iteration 1 ---
Node 0 Routing Table:
Dest    Cost    Next Hop
0       0       -
1       10      1
2       100     2
3       30      3

...

--- DVR Final Tables ---
Node 0 Routing Table:
Dest    Cost    Next Hop
0       0       -
1       10      1
2       30      3
3       30      3

--- Link State Routing Simulation ---
Node 0 Routing Table:
Dest    Cost    Next Hop
1       10      1
2       30      3
3       30      3
...
```

---
## 6. Restrictions
- Input must be provided in the exact adjacency matrix format.
- Graph should be symmetric and contain only non-negative weights.
- Code assumes node numbering starts from 0.

---

## 7. Challenges
- Designing the update loop for DVR to converge properly.
- Correctly backtracking the shortest path in LSR to determine the next hop.
- Making the code modular and clean for both algorithms.

---

## 8. Contribution Breakdown
- **Priya Gangwar**:
  - Implemented Distance Vector Routing logic and convergence detection.
  - Wrote helper functions and initial matrix parsing.
- **Monika Kumari**:
  - Developed Link State Routing using Dijkstra's algorithm.
  - Wrote logic for backtracking `prev[]` to determine next hop.
- **Ritam Acharya**:
  - Created Makefile, set up input formatting and testing.
  - Handled output formatting, routing table display, and validation.

---

## 9. What Extra We Did Beyond Minimum Requirements
- Modular code design for easy testing and extension.
- Used structured data types (`RoutingEntry`) instead of raw arrays.
- Added verbose and clean formatting for outputs.

---

## 10. Sources
- Lecture slides from CS425.
- C++ STL documentation.
- Algorithm references from GeeksForGeeks and Wikipedia.

---

## 11. Declaration

We declare that all the work presented here is our own, and we have not indulged in any plagiarism or unauthorized collaboration beyond our team.

---

## 12. Acknowledgment

We thank **Prof. Adithya Vadapalli** for his guidance and the problem statements.
