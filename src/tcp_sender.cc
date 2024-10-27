#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const { // Your code here.
  uint64_t re = 0;
  if (message_queue_.empty()) {
    return 0;
  }
  auto cur = message_queue_.begin();
  while (cur != message_queue_.end()) {
    re += cur->sequence_length();
    cur++;
  }
  return re;
}

uint64_t TCPSender::consecutive_retransmissions() const {
  return consecutive_retransmissions_;
}

void TCPSender::push(const TransmitFunction &transmit) {
  if (end_) {
    return;
  }
  uint64_t segment_size = TCPConfig::MAX_PAYLOAD_SIZE;
  uint64_t window_size = receiver_window_size_;
  if (window_size == 0) {
    window_size = 1;
  }
  if (segment_size + input_.reader().bytes_popped() >
      window_size + last_ackno_ - 1) {
    segment_size =
        window_size + last_ackno_ - 1 - input_.reader().bytes_popped();
  }
  if (segment_size > writer().bytes_pushed() - input_.reader().bytes_popped()) {
    segment_size = writer().bytes_pushed() - input_.reader().bytes_popped();
  }
  if (writer().is_closed() && segment_size == 0 &&
      window_size + last_ackno_ - 1 - input_.reader().bytes_popped() > 0 &&
      writer().bytes_pushed() == input_.reader().bytes_popped() &&
      window_size > 0) {
    // 如果writer关闭了，所有数据都发送了，但是还没有发送FIN，那么就发送一个空消息，不能直接返回
    // do nothing here
  } else if (segment_size == 0 && SYN_sent_) {
    return;
  }
  TCPSenderMessage msg;
  if (!SYN_sent_) {
    msg.SYN = true;
    msg.seqno = isn_;
    SYN_sent_ = true;
    last_send_ = msg.seqno + static_cast<uint32_t>(segment_size);
  } else {
    msg.SYN = false;
    msg.seqno = last_send_ + static_cast<uint32_t>(1);
    last_send_ = msg.seqno + static_cast<uint32_t>(segment_size - 1);
  }
  if (writer().has_error()) {
    msg.RST = true;
  }
  if (writer().is_closed() &&
      input_.reader().bytes_popped() + segment_size ==
          writer().bytes_pushed() &&
      window_size > segment_size) {
    msg.FIN = true;
    end_ = true;
  } else {
    msg.FIN = false;
  }
  msg.payload = reader().peek().substr(0, segment_size);
  input_.reader().pop(segment_size);
  transmit(msg);
  message_queue_.push_back(msg);
  if (!timer_running_) {
    timer_running_ = true;
    timer_ = 0;
  }
  if (msg.payload.size() != 0 && receiver_window_size_ != 0) {
    push(transmit); // 递归调用，如果还有数据就继续发送
  }
}

TCPSenderMessage TCPSender::make_empty_message() const {
  TCPSenderMessage msg;
  if (!SYN_sent_) {
    msg.SYN = true;
    msg.seqno = isn_;
  } else {
    msg.SYN = false;
    msg.seqno = last_send_ + static_cast<uint32_t>(1 + (end_ ? 1 : 0));
  }
  if (writer().has_error()) {
    msg.RST = true;
  }
  msg.payload = "";
  return msg;
}

void TCPSender::receive(const TCPReceiverMessage &msg) {
  if (msg.RST) {
    writer().set_error();
    return;
  }
  if (reader().has_error()) {
    return;
  }
  if (msg.ackno != std::nullopt) {
    if (last_ackno_ == (*msg.ackno).unwrap(isn_, last_ackno_)) {
      // do nothing
    } else {
      auto tmp = (*msg.ackno).unwrap(isn_, last_ackno_);
      if (tmp > input_.reader().bytes_popped() + (SYN_sent_ ? 1 : 0) +
                    (end_ ? 1 : 0)) {
        return;
      }
      last_ackno_ = tmp;
      consecutive_retransmissions_ = 0;
      current_RTO_ms_ = initial_RTO_ms_;
      while (!message_queue_.empty()) {
        if (message_queue_.front().seqno.unwrap(isn_, last_ackno_) +
                message_queue_.front().sequence_length() <=
            last_ackno_) {
          message_queue_.pop_front();
        } else {
          break;
        }
      }
      if (message_queue_.empty()) {
        timer_running_ = false;
      } else {
        timer_running_ = true;
        timer_ = 0;
      }
    }
  }
  receiver_window_size_ = msg.window_size;
}

void TCPSender::tick(uint64_t ms_since_last_tick,
                     const TransmitFunction &transmit) {
  // Your code here.
  (void)ms_since_last_tick;
  (void)transmit;
  (void)initial_RTO_ms_;
  if (timer_running_) {
    timer_ += ms_since_last_tick;
    if (timer_ >= current_RTO_ms_) {
      if (message_queue_.empty()) {
        return;
      }
      transmit(message_queue_.front());
      if (receiver_window_size_ != 0) {
        consecutive_retransmissions_++;
        current_RTO_ms_ *= 2;
      }
      timer_ = 0;
    }
  }
}
