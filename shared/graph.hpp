#pragma once

#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <queue>
#include <functional>
#include <stack>

/**
 * @brief A directed, unweighted graph of nodes of type `_Node`, indexed by keys of type `_Key`.
 *
 * Stores nodes keyed by `_Key`, plus a forward and an inverse adjacency set per key so that both
 * outgoing and incoming edges can be queried in constant time.
 * @tparam _Key Type used to identify nodes and look them up.
 * @tparam _Node Type of value stored at each node.
 * @tparam _Hash Hash functor for `_Key`, used by the underlying unordered containers.
 */
template<typename _Key, typename _Node, typename _Hash = std::hash<_Key>>
class Graph {
  std::unordered_map<_Key, _Node, _Hash> nodes_; ///< Node values, keyed by `_Key`.
  std::unordered_map<_Key, std::unordered_set<_Key, _Hash>, _Hash> edges_; ///< Key connected to values, outgoing connections.
  std::unordered_map<_Key, std::unordered_set<_Key, _Hash>, _Hash> inv_edges_; ///< Inverse edges, incoming connections.

public:
  /** @brief Check whether the graph has no nodes. @return True if empty. */
  bool empty() const { return nodes_.empty(); }

  /** @brief Get the number of nodes in the graph. @return Node count. */
  size_t size() const { return nodes_.size(); }

  /** @brief Remove all nodes and edges. */
  void clear() {
    nodes_.clear();
    edges_.clear();
    inv_edges_.clear();
  }

  /**
   * @brief Check whether a node exists.
   * @param key Key to look up.
   * @return True if a node with this key exists.
   */
  bool exists(const _Key& key) const {
    return nodes_.contains(key);
  }

  /**
   * @brief Check whether a directed edge exists.
   * @param a Source node key.
   * @param b Destination node key.
   * @return True if an edge from `a` to `b` exists.
   */
  bool exists(const _Key& a, const _Key& b) const {
    return nodes_.contains(a) && edges_[a].contains(b);
  }

  /**
   * @brief Get the node associated with a key.
   * @param key Key to look up.
   * @return Reference to the node, or empty if no node has this key.
   */
  std::optional<std::reference_wrapper<const _Node>> get(const _Key& key) const {
    if (auto it = nodes_.find(key); it != nodes_.end()) return it->second;
    return {};
  }

  /**
   * @brief Get the node associated with a key.
   * @param key Key to look up.
   * @return Reference to the node, or empty if no node has this key.
   */
  std::optional<std::reference_wrapper<_Node>> get(const _Key& key) {
    if (auto it = nodes_.find(key); it != nodes_.end()) return it->second;
    return {};
  }

  /**
   * @brief Insert a node.
   * @param key Key to insert the node under.
   * @param node Node value to store.
   */
  void insert(_Key key, _Node node) {
    nodes_.insert({std::move(key), std::move(node)});
  }

  /**
   * @brief Insert a directed edge between two existing keys.
   * @param from Source node key.
   * @param to Destination node key.
   */
  void insert(const _Key& from, const _Key& to) {
    edges_[from].insert(to);
    inv_edges_[to].insert(from);
  }

  /**
   * @brief Insert an edge in both directions between two keys.
   * @param from First node key.
   * @param to Second node key.
   */
  void insert_symmetric(const _Key& from, const _Key& to) {
    insert(from, to);
    insert(to, from);
  }

  /**
   * @brief Insert a chain of directed edges connecting consecutive keys in `path`.
   * @param path Sequence of keys `a, b, c, ...` to connect as `a -> b -> c -> ...`.
   */
  void insert_path(const std::vector<_Key>& path) {
    for (int i = 0; i < path.size() - 1; i++) {
      insert(path[i], path[i + 1]);
    }
  }

  /**
   * @brief Remove a node and every edge touching it.
   * @param key Key of the node to remove.
   */
  void remove(const _Key& key) {
    if (!nodes_.contains(key)) return;

    nodes_.erase(key);
    edges_.erase(key);
    for (auto& [_, edges] : edges_)
      edges.erase(key);
    inv_edges_.erase(key);
    for (auto& [_, edges] : inv_edges_)
      edges.erase(key);
  }

  /**
   * @brief Remove a directed edge.
   * @param from Source node key.
   * @param to Destination node key.
   */
  void remove(const _Key& from, const _Key& to) {
    edges_[from].erase(to);
    inv_edges_[to].erase(from);
  }

  /**
   * @brief Get the keys a node has outgoing edges to.
   * @param from Key of the node to query.
   * @return Set of directly reachable keys, empty if `from` doesn't exist.
   */
  std::unordered_set<_Key> get_outward_connections(const _Key& from) {
    if (!nodes_.contains(from)) return {};
    return edges_[from];
  }

  /**
   * @brief Get the keys that have outgoing edges to a node.
   * @param to Key of the node to query.
   * @return Set of keys with a direct edge to `to`, empty if `to` doesn't exist.
   */
  std::unordered_set<_Key> get_inward_connections(const _Key& to) {
    if (!nodes_.contains(to)) return {};
    return inv_edges_[to];
  }

  /**
   * @brief Breadth-first traversal from a start node, with early termination.
   * @param start Key to start the traversal from.
   * @param visit Called with each visited key; return true to stop the traversal, false to continue.
   */
  void conditional_bfs(const _Key& start, const std::function<bool(const _Key&)>& visit) const {
    std::unordered_set<_Key, _Hash> visited{start};
    std::queue<_Key> q;
    q.push(start);

    while (!q.empty()) {
      _Key current = q.front();
      q.pop();
      if (visit(current)) break;

      if (auto it = edges_.find(current); it != edges_.end()) {
        for (const _Key& neighbour : it->second) {
          if (!visited.contains(neighbour)) {
            visited.insert(neighbour);
            q.push(neighbour);
          }
        }
      }
    }
  }

  /**
   * @brief Breadth-first traversal from a start node, visiting every reachable node.
   * @param start Key to start the traversal from.
   * @param visit Called with each visited key.
   */
  void bfs(const _Key& start, const std::function<void(const _Key&)>& visit) const {
    conditional_bfs(start, [&](const _Key& key) -> bool {
      visit(key);
      return false;
    });
  }

  /**
   * @brief Depth-first traversal from a start node, with early termination.
   * @param start Key to start the traversal from.
   * @param visit Called with each visited key; return true to stop the traversal, false to continue.
   */
  void conditional_dfs(const _Key& start, const std::function<bool(const _Key&)>& visit) const {
    std::unordered_set<_Key, _Hash> visited;
    std::stack<_Key> s;
    s.push(start);

    while (!s.empty()) {
      _Key current = s.top();
      s.pop();
      if (visited.contains(current)) continue;

      if (visit(current)) break;
      visited.insert(current);

      if (auto it = edges_.find(current); it != edges_.end()) {
        for (const _Key& neighbour : it->second) {
          if (!visited.contains(neighbour)) {
            s.push(neighbour);
          }
        }
      }
    }
  }

  /**
   * @brief Depth-first traversal from a start node, visiting every reachable node.
   * @param start Key to start the traversal from.
   * @param visit Called with each visited key.
   */
  void dfs(const _Key& start, const std::function<void(const _Key&)>& visit) const {
    conditional_dfs(start, [&](const _Key& key) {
      visit(key);
      return false;
    });
  }

  /**
   * @brief Check whether two nodes are connected by a directed path (via BFS).
   * @param from Key of the starting node.
   * @param to Key of the target node.
   * @return True if `to` is reachable from `from`; false if either key doesn't exist or no path exists.
   */
  bool are_connected(const _Key& from, const _Key& to) const {
    if (!exists(from) || !exists(to)) return false;
    bool found = false;
    conditional_bfs(from, [&](const _Key& node) { return found = node == to; });
    return found;
  }

  /** @brief Iterator to the first node. @return Iterator over (key, node) pairs. */
  auto begin() { return nodes_.begin(); }
  /** @brief Iterator to the first node. @return Const iterator over (key, node) pairs. */
  auto begin() const { return nodes_.begin(); }
  /** @brief Iterator past the last node. @return Iterator over (key, node) pairs. */
  auto end() { return nodes_.end(); }
  /** @brief Iterator past the last node. @return Const iterator over (key, node) pairs. */
  auto end() const { return nodes_.end(); }
};
