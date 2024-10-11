#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive(TCPSenderMessage message) {
  // Your code here.
  if (message.RST) {
    reader().set_error();
    return;
  }
  if (writer().has_error()) {
    return;
  }
  if (message.SYN) {
    seq_ = message.seqno;
    initial_seqno_received_ = true;
  }
  if (!initial_seqno_received_) {
    return;
  }
  uint64_t abs_seqno = message.seqno.unwrap(seq_, reassembler_.waiting());
  if (!message.SYN) {
    abs_seqno--;
  }
  reassembler_.insert(abs_seqno, message.payload, message.FIN);
}

TCPReceiverMessage TCPReceiver::send() const {
  // Your code here.
  if (reader().has_error()) {
    TCPReceiverMessage message;
    message.RST = true;
    return message;
  }
  TCPReceiverMessage message;
  uint64_t abs_ackno = reassembler_.waiting() + 1;
  if (writer().is_closed()) {
    abs_ackno++;
  }
  if (initial_seqno_received_) {
    message.ackno = Wrap32::wrap(abs_ackno, seq_);
  }
  message.window_size =
      static_cast<uint16_t>(std::min(65535UL, reassembler_.capacity()));
  message.RST = false;
  return message;
}
