#include <iostream>
#include <vector>
#include <limits>
#include <queue>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cassert>

using namespace std;

// A large value to represent 'infinite' distance (i.e., no direct link)
static const int INF = 9999;

/**
 * Print the routing table for a single node under DVR.
 * Shows destination, cost, and next hop for each entry.
 */
void printDVRTable(
    const int node,
    const vector<vector<int>>& dist,
    const vector<vector<int>>& nextHop)
{
    // Header for alignment
    cout << left << setw(8) << "Dest"
         << setw(8) << "Cost" << "Next Hop" << "\n";

    // Each row: destination index, cost, and next hop router
    for (auto dest = 0u; dest < dist.size(); ++dest) {
        cout << left << setw(8) << dest
             << setw(8) << dist[node][dest];

        // If nextHop == -1, it's unreachable
        if (nextHop[node][dest] == -1) {
            cout << "-";
        } else {
            cout << nextHop[node][dest];
        }
        cout << "\n";
    }
    cout << "\n";
}

/**
 * Simulate Distance Vector Routing (Bellman-Ford style).
 * Each node updates its table by exchanging info with neighbors.
 * Converges in at most (n-1) iterations for n nodes.
 */
void simulateDVR(const vector<vector<int>>& graph)
{
    int n = graph.size();

    // dist[u][v] = current best cost from u to v
    vector<vector<int>> dist = graph;

    // nextHop[u][v] = the immediate neighbor on the best path from u to v
    vector<vector<int>> nextHop(n, vector<int>(n, -1));
    for (int u = 0; u < n; ++u) {
        for (int v = 0; v < n; ++v) {
            if (graph[u][v] != INF && u != v) {
                nextHop[u][v] = v;
            }
        }
    }

    bool updated = true;
    int maxIterations = n - 1;
    int iteration = 0;

    // Repeat until no updates or until safety cap reached
    while (updated && maxIterations-- > 0) {
        updated = false;
        ++iteration;

        // For every source-destination pair, try all neighbors as intermediates
        for (int u = 0; u < n; ++u) {
            for (int neighbor = 0; neighbor < n; ++neighbor) {
                if (graph[u][neighbor] == INF || u == neighbor)
                    continue;  // skip non-neighbors & self

                for (int dest = 0; dest < n; ++dest) {
                    if (dist[neighbor][dest] == INF)
                        continue;  // neighbor can't reach dest

                    int newCost = dist[u][neighbor] + dist[neighbor][dest];
                    if (newCost < dist[u][dest]) {
                        dist[u][dest] = newCost;
                        nextHop[u][dest] = neighbor;
                        updated = true;
                    }
                }
            }
        }

        // Optional: show intermediate state after each iteration
        cout << "--- DVR Iteration " << iteration << " ---\n";
        for (int node = 0; node < n; ++node) {
            cout << "Node " << node << " Routing Table:\n";
            printDVRTable(node, dist, nextHop);
        }
    }

    // Final tables after convergence
    cout << "--- DVR Final Tables ---\n";
    for (int node = 0; node < n; ++node) {
        cout << "Node " << node << " Routing Table:\n";
        printDVRTable(node, dist, nextHop);
    }
}

/**
 * Print the routing table for a single node under LSR.
 * Uses precomputed nextHop array for direct lookup.
 */
void printLSRTable(
    const int src,
    const vector<int>& dist,
    const vector<int>& nextHop)
{
    cout << left << setw(8) << "Dest"
         << setw(8) << "Cost" << "Next Hop" << "\n";

    // Skip the source itself
    for (auto dest = 0u; dest < dist.size(); ++dest){
        if (dest == static_cast<size_t>(src)) continue;


        cout << left << setw(8) << dest
             << setw(8) << dist[dest];

        if (nextHop[dest] == -1) {
            cout << "-";
        } else {
            cout << nextHop[dest];
        }
        cout << "\n";
    }
    cout << "\n";
}

/**
 * Simulate Link State Routing (Dijkstra's algorithm).
 * Each router floods the entire topology and independently computes shortest paths.
 */
void simulateLSR(const vector<vector<int>>& graph)
{
    int n = graph.size();

    // Run Dijkstra for each source router
    for (int src = 0; src < n; ++src) {
        vector<int> dist(n, INF), prev(n, -1), nextHop(n, -1);
        vector<bool> visited(n, false);

        // Distance to self is zero
        dist[src] = 0;

        // Prepare a min-heap (distance, node)
        vector<pair<int,int>> buffer;
        buffer.reserve(n * n);  // avoid realloc overhead on large graphs
        priority_queue<
            pair<int,int>,
            vector<pair<int,int>>,
            greater<pair<int,int>>
        > pq(greater<pair<int,int>>(), move(buffer));
        pq.push(make_pair(0, src));

        while (!pq.empty()) {
            // Extract top without structured bindings for C++11 compatibility
            pair<int,int> top = pq.top();
            pq.pop();
            int currentDist = top.first;
            int u = top.second;

            if (visited[u]) continue;
            visited[u] = true;

            // Relax edges from u to all neighbors
            for (int v = 0; v < n; ++v) {
                if (graph[u][v] == INF || u == v)
                    continue;

                int candidate = currentDist + graph[u][v];
                if (candidate < dist[v]) {
                    dist[v] = candidate;
                    prev[v] = u;

                    // Track first hop: if coming straight from src, v is nextHop;
                    // otherwise inherit nextHop of u
                    nextHop[v] = (u == src) ? v : nextHop[u];
                    pq.push(make_pair(candidate, v));
                }
            }
        }

        // Output routing table for this source
        cout << "Node " << src << " Routing Table:\n";
        printLSRTable(src, dist, nextHop);
    }
}

/**
 * Read adjacency matrix from a file. Enforces non-negative and zero-diagonal.
 * Throws error and exits on bad input.
 */
vector<vector<int>> readGraphFromFile(const string& filename)
{
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Could not open file '" << filename << "'\n";
        exit(EXIT_FAILURE);
    }

    int n;
    file >> n;
    vector<vector<int>> g(n, vector<int>(n));

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            file >> g[i][j];

            // Disallow negative weights
            if (g[i][j] < 0) {
                cerr << "Error: Negative weights not supported\n";
                exit(EXIT_FAILURE);
            }

            // Enforce zero on diagonal (no self-loop cost)
            if (i == j && g[i][j] != 0) {
                cerr << "Error: Self-loop detected at (" << i << "," << j << ")\n";
                exit(EXIT_FAILURE);
            }
        }
    }
    return g;
}

int main(int argc, char* argv[])
{
    // Expect exactly one argument: the input file path
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <input_file>\n";
        return EXIT_FAILURE;
    }

    // Parse the network graph from file
    auto graph = readGraphFromFile(argv[1]);

    cout << "\n--- Distance Vector Routing Simulation ---\n";
    simulateDVR(graph);

    cout << "\n--- Link State Routing Simulation ---\n";
    simulateLSR(graph);

    return EXIT_SUCCESS;
}
