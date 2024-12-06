#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;
// 为某个路由前缀添加一个映射
void Router::add_map(uint32_t route_prefix, uint8_t prefix_length,
                     std::optional<Address> next_hop, size_t interface_num) {
  router_key key = {route_prefix, prefix_length};
  router_value value = {next_hop, interface_num};
  _routes[key] = value;
}
optional<router_value> Router::longest_prefix_match(uint32_t route_prefix) {
  for (int i = 32; i >= 1; i--) {
    uint32_t mask = 0xffffffff << (32 - i);
    uint32_t prefix = route_prefix & mask;
    router_key key = {prefix, static_cast<uint32_t>(i)};
    if (_routes.find(key) != _routes.end()) {
      return _routes[key];
    }
  }
  router_key key = {0, 0};
  if (_routes.find(key) != _routes.end()) {
    return _routes[key];
  }
  return nullopt;
}
// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's
// destination address against prefix_length: For this route to be applicable,
// how many high-order (most-significant) bits of the route_prefix will need to
// match the corresponding bits of the datagram's destination address?
// next_hop:
// The IP address of the next hop. Will be empty if the network is directly
// attached to the router (in which case, the next hop address should be the
// datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix, const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
  cerr << "DEBUG: adding route "
       << Address::from_ipv4_numeric(route_prefix).ip() << "/"
       << static_cast<int>(prefix_length) << " => "
       << (next_hop.has_value() ? next_hop->ip() : "(direct)")
       << " on interface " << interface_num << "\n";
  add_map(route_prefix, prefix_length, next_hop, interface_num);
  // Your code here.
}

// Go through all the interfaces, and route every incoming datagram to its
// proper outgoing interface.
void Router::route() {
  // Your code here.
  for (auto interface_i : _interfaces) {
    while (interface_i->datagrams_received().size() > 0) {
      InternetDatagram dgram = interface_i->datagrams_received().front();
      if (dgram.header.ttl <= 1) {
        cerr << "DEBUG: TTL expired\n";
        interface_i->datagrams_received().pop();
        continue;
      }
      dgram.header.ttl--;
      dgram.header.compute_checksum();
      optional<router_value> match = longest_prefix_match(dgram.header.dst);
      if (match.has_value()) {
        if (match->next_hop.has_value()) {
          interface(match->interface_num)
              ->send_datagram(dgram, match->next_hop.value());
        } else {
          interface(match->interface_num)
              ->send_datagram(dgram,
                              Address::from_ipv4_numeric(dgram.header.dst));
        }
        interface_i->datagrams_received().pop();
      } else {
        cerr << "DEBUG: No match\n";
        interface_i->datagrams_received().pop();
        continue;
      }
    }
  }
}
