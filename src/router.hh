#pragma once

#include <memory>
#include <optional>
#include <map>

#include "exception.hh"
#include "network_interface.hh"
struct router_key {
  uint32_t route_prefix;
  uint32_t prefix_length;
  bool operator<(const router_key &rhs) const {
    if (route_prefix == rhs.route_prefix) {
      return prefix_length < rhs.prefix_length;
    }
    return route_prefix < rhs.route_prefix;
  }
};
struct router_value {
  std::optional<Address> next_hop{};
  size_t interface_num{};
};
// \brief A router that has multiple network interfaces and
// performs longest-prefix-match routing between them.
class Router {
public:
  // Add an interface to the router
  // \param[in] interface an already-constructed network interface
  // \returns The index of the interface after it has been added to the router
  size_t add_interface(std::shared_ptr<NetworkInterface> interface) {
    _interfaces.push_back(notnull("add_interface", std::move(interface)));
    return _interfaces.size() - 1;
  }

  // Access an interface by index
  std::shared_ptr<NetworkInterface> interface(const size_t N) {
    return _interfaces.at(N);
  }

  // Add a route (a forwarding rule)
  void add_route(uint32_t route_prefix, uint8_t prefix_length,
                 std::optional<Address> next_hop, size_t interface_num);

  // Route packets between the interfaces
  void route();

private:
  // The router's collection of network interfaces
  std::vector<std::shared_ptr<NetworkInterface>> _interfaces{};
  std::map<router_key, router_value> _routes{};
  void add_map(uint32_t route_prefix, uint8_t prefix_length,
               std::optional<Address> next_hop, size_t interface_num);
  std::optional<router_value> longest_prefix_match(uint32_t route_prefix);
};
