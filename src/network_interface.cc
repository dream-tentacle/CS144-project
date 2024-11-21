#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

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
  frame.payload = dgram.payload;
  frame.header.src = ethernet_address_;
  if (ethernet_adr_table.find(next_hop.ipv4_numeric()) ==
      ethernet_adr_table.end()) {
    ARPMessage arp;
    arp.opcode = ARPMessage::OPCODE_REQUEST;
    arp.sender_ethernet_address = ethernet_address_;
    arp.sender_ip_address = ip_address_.ipv4_numeric();
    arp.target_ip_address = next_hop.ipv4_numeric();
    EthernetFrame arp_frame;
    arp_frame.header.type = EthernetHeader::TYPE_ARP;
    arp_frame.header.src = ethernet_address_;
    arp_frame.header.dst = ETHERNET_BROADCAST;
    // 以某种方式把ARPMessage的内容放到arp_frame的payload里
    // ** SOMETHING **
    frames_to_send.push_back({frame, next_hop});
    transmit(arp_frame);
  } else {
    frame.header.dst = ethernet_adr_table[next_hop.ipv4_numeric()].first;
    frames_to_send.push_back({frame, next_hop});
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame(const EthernetFrame &frame) {
  if (frame.header.type == EthernetHeader::TYPE_ARP) {
    ARPMessage arp;
    Parser p(frame.payload);
    arp.parse(p);
    if (arp.opcode == ARPMessage::OPCODE_REQUEST) {
      if (arp.target_ip_address == ip_address_.ipv4_numeric()) {
        // 正在请求我们的地址
        ARPMessage arp_reply;
        arp_reply.opcode = ARPMessage::OPCODE_REPLY;
        arp_reply.sender_ethernet_address = ethernet_address_;
        arp_reply.sender_ip_address = ip_address_.ipv4_numeric();
        arp_reply.target_ip_address = arp.sender_ip_address;
        EthernetFrame arp_frame;
        arp_frame.header.type = EthernetHeader::TYPE_ARP;
        arp_frame.header.src = ethernet_address_;
        arp_frame.header.dst = arp.sender_ethernet_address;
        // 以某种方式把ARPMessage的内容放到arp_frame的payload里
        // ** SOMETHING **
        transmit(arp_frame);
      }
      // else: NOTHING
    } else if (arp.opcode == ARPMessage::OPCODE_REPLY) {
      // 接到回复了
      ethernet_adr_table[arp.sender_ip_address] = {arp.sender_ethernet_address,
                                                   30 * 1000};
      for (auto it = frames_to_send.begin(); it != frames_to_send.end();) {
        if (it->second.ipv4_numeric() == arp.sender_ip_address) {
          it->first.header.dst = arp.sender_ethernet_address;
          transmit(it->first);
          it = frames_to_send.erase(it);
        } else {
          it++;
        }
      }
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call
//! to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
  for (auto it : ethernet_adr_table) {
    it.second.second -= ms_since_last_tick;
    if (it.second.second <= 0) {
      ethernet_adr_table.erase(it.first);
    }
  }
}
