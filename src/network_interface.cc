#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;
string_view get_ethernet_address(const EthernetAddress &ethernet_address) {
  static char buffer[18];
  snprintf(buffer, sizeof(buffer), "%02x:%02x:%02x:%02x:%02x:%02x",
           ethernet_address[0], ethernet_address[1], ethernet_address[2],
           ethernet_address[3], ethernet_address[4], ethernet_address[5]);
  return buffer;
}
//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of
//! the interface \param[in] ip_address IP (what ARP calls "protocol") address
//! of the interface
NetworkInterface::NetworkInterface(string_view name,
                                   shared_ptr<OutputPort> port,
                                   const EthernetAddress &ethernet_address,
                                   const Address &ip_address)
    : name_(name), port_(notnull("OutputPort", move(port))),
      ethernet_address_(ethernet_address), ip_address_(ip_address) {
  cerr << "DEBUG: Network interface has Ethernet address "
       << to_string(ethernet_address) << " and IP address " << ip_address.ip()
       << "\n";
}
//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically
//! a router or default gateway, but may also be another host if directly
//! connected to the same network as the destination) Note: the Address type can
//! be converted to a uint32_t (raw 32-bit IP address) by using the
//! Address::ipv4_numeric() method.
void NetworkInterface::send_datagram(const InternetDatagram &dgram,
                                     const Address &next_hop) {
  EthernetFrame frame;
  frame.header.type = EthernetHeader::TYPE_IPv4;
  frame.payload = serialize(dgram);
  frame.header.src = ethernet_address_;
  if (ethernet_adr_table[next_hop.ipv4_numeric()].first == ETHERNET_BROADCAST ||
      ethernet_adr_table[next_hop.ipv4_numeric()].second <= 0) {
    frames_to_send.push_back({frame, next_hop});
    if (query_adr_table[next_hop.ipv4_numeric()].second <= 0) {
      EthernetFrame arp_frame;
      ARPMessage arp;
      arp.opcode = ARPMessage::OPCODE_REQUEST;
      arp.sender_ethernet_address = ethernet_address_;
      arp.sender_ip_address = ip_address_.ipv4_numeric();
      arp.target_ip_address = next_hop.ipv4_numeric();
      arp_frame.payload = serialize(arp);
      arp_frame.header.type = EthernetHeader::TYPE_ARP;
      arp_frame.header.src = ethernet_address_;
      arp_frame.header.dst = ETHERNET_BROADCAST;
      transmit(arp_frame);
      query_adr_table[next_hop.ipv4_numeric()].second = 5 * 1000;
    }
  } else {
    frame.header.dst = ethernet_adr_table[next_hop.ipv4_numeric()].first;
    transmit(frame);
  }
}

void NetworkInterface::process_on_learnt(const ARPMessage &recv_arp) {
  ethernet_adr_table[recv_arp.sender_ip_address] = {
      recv_arp.sender_ethernet_address, 30 * 1000};
  for (auto it = frames_to_send.begin(); it != frames_to_send.end(); it++) {
    Parser p(it->first.payload);
    InternetDatagram dgram;
    dgram.parse(p);
  }

  for (auto it = frames_to_send.begin(); it != frames_to_send.end();) {
    if (it->second.ipv4_numeric() == recv_arp.sender_ip_address) {
      it->first.header.dst = recv_arp.sender_ethernet_address;
      transmit(it->first);
      it = frames_to_send.erase(it);
    } else {
      it++;
    }
  }
  for (auto it = frames_to_send.begin(); it != frames_to_send.end(); it++) {
    Parser p(it->first.payload);
    InternetDatagram dgram;
    dgram.parse(p);
  }
}
//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame(const EthernetFrame &frame) {
  if (frame.header.type == EthernetHeader::TYPE_ARP) {
    ARPMessage recv_arp;
    Parser p(frame.payload);
    recv_arp.parse(p);
    if (recv_arp.opcode == ARPMessage::OPCODE_REQUEST) {
      if (recv_arp.target_ip_address == ip_address_.ipv4_numeric()) {
        // 正在请求我们的地址
        ARPMessage arp_reply;
        arp_reply.opcode = ARPMessage::OPCODE_REPLY;
        arp_reply.sender_ethernet_address = ethernet_address_;
        arp_reply.sender_ip_address = ip_address_.ipv4_numeric();
        arp_reply.target_ethernet_address = recv_arp.sender_ethernet_address;
        arp_reply.target_ip_address = recv_arp.sender_ip_address;
        EthernetFrame arp_frame;
        arp_frame.header.type = EthernetHeader::TYPE_ARP;
        arp_frame.header.src = ethernet_address_;
        arp_frame.header.dst = recv_arp.sender_ethernet_address;
        arp_frame.payload = serialize(arp_reply);
        transmit(arp_frame);
        // 从这个地址学习
      }
      process_on_learnt(recv_arp);
      // else: NOTHING
    } else if (recv_arp.opcode == ARPMessage::OPCODE_REPLY) {
      // 接到回复了
      process_on_learnt(recv_arp);
    }
  } else if (frame.header.type == EthernetHeader::TYPE_IPv4 &&
             frame.header.dst == ethernet_address_) {
    InternetDatagram dgram;
    Parser p(frame.payload);
    dgram.parse(p);
    datagrams_received_.push(dgram);
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call
//! to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
  for (auto it = ethernet_adr_table.begin(); it != ethernet_adr_table.end();
       it++) {
    it->second.second -= ms_since_last_tick;
  }
  for (auto it = query_adr_table.begin(); it != query_adr_table.end(); it++) {
    it->second.second -= ms_since_last_tick;
  }
}
