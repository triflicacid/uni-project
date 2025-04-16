#pragma once

#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <queue>
#include <functional>
#include <stack>

/// represents a directed unweighted graph with nodes of type `_Node_` which are indexed by keys of type `_Key`
template<typename _Key, typename _Node, typename _Hash = std::hash<_Key>>
class Graph {
  std::unordered_map<_Key, _Node, _Hash> nodes_;
  std::unordered_map<_Key, std::unordered_set<_Key, _Hash>, _Hash> edges_; // key connected to values, outgoing connections
  std::unordered_map<_Key, std::unordered_set<_Key, _Hash>, _Hash> inv_edges_; // inverse edges, incoming connections

public:
  bool empty() const { return nodes_.empty(); }

  size_t size() const { return nodes_.size(); }

  void clear() {
    nodes_.clear();
    edges_.clear();
    inv_edges_.clear();
  }

  // check if the given node exists
  bool exists(const _Key& key) const {
    return nodes_.contains(key);
  }

  // check if the given edge exists
  bool exists(const _Key& a, const _Key& b) const {
    return nodes_.contains(a) && edges_[a].contains(b);
  }

  // get the node associated with the given key
  std::optional<std::reference_wrapper<const _Node>> get(const _Key& key) const {
    if (auto it = nodes_.find(key); it != nodes_.end()) return it->second;
    return {};
  }

  // get the node associated with the given key
  std::optional<std::reference_wrapper<_Node>> get(const _Key& key) {
    if (auto it = nodes_.find(key); it != nodes_.end()) return it->second;
    return {};
  }

  // insert the given node
  void insert(_Key key, _Node node) {
    nodes_.insert({std::move(key), std::move(node)});
  }

  // insert an edge
  void insert(const _Key& from, const _Key& to) {
    edges_[from].insert(to);
    inv_edges_[to].insert(from);
  }

  // insert an edge, with the connection both ways
  void insert_symmetric(const _Key& from, const _Key& to) {
    insert(from, to);
    insert(to, from);
  }

  // insert a path a -> b -> c -> ...
  void insert_path(const std::vector<_Key>& path) {
    for (int i = 0; i < path.size() - 1; i++) {
      insert(path[i], path[i + 1]);
    }
  }

  // delete the given node
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

  // remove the given edge
  void remove(const _Key& from, const _Key& to) {
    edges_[from].erase(to);
    inv_edges_[to].erase(from);
  }

  // get outward connections for a node
  std::unordered_set<_Key> get_outward_connections(const _Key& from) {
    if (!nodes_.contains(from)) return {};
    return edges_[from];
  }

  // get ingoing connections to a node
  std::unordered_set<_Key> get_inward_connections(const _Key& to) {
    if (!nodes_.contains(to)) return {};
    return inv_edges_[to];
  }

  // execute a BFS search
  // call visit() on node, return whether to halt (true = halt, false = continue)
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

  // execute a bfs search
  void bfs(const _Key& start, const std::function<void(const _Key&)>& visit) const {
    conditional_bfs(start, [&](const _Key& key) -> bool {
      visit(key);
      return false;
    });
  }

  // execute a DFS search
  // call visit() on node, return whether to halt (true = halt, false = continue)
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

  // execute a dfs search
  void dfs(const _Key& start, const std::function<void(const _Key&)>& visit) const {
    conditional_dfs(start, [&](const _Key& key) {
      visit(key);
      return false;
    });
  }

  // check if two nodes are connected
  // (uses BFS search)
  bool are_connected(const _Key& from, const _Key& to) const {
    if (!exists(from) || !exists(to)) return false;
    bool found = false;
    conditional_bfs(from, [&](const _Key& node) { return found = node == to; });
    return found;
  }

  auto begin() { return nodes_.begin(); }
  auto begin() const { return nodes_.begin(); }
  auto end() { return nodes_.end(); }
  auto end() const { return nodes_.end(); }
};
