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
  uint64_t segment_content_size = TCPConfig::MAX_PAYLOAD_SIZE;
  uint64_t window_size = receiver_window_size_;
  if (window_size == 0) {
    window_size = 1; // 根据规则，如果窗口为0，那么必须当成是1来处理
  }
  if (segment_content_size + input_.reader().bytes_popped() >
      window_size + last_ackno_ - 1) {
    // 根据对方剩余空间来决定发送的数据量
    segment_content_size =
        window_size + last_ackno_ - 1 - input_.reader().bytes_popped();
  }
  if (segment_content_size >
      writer().bytes_pushed() - input_.reader().bytes_popped()) {
    // 根据待发送的数据量来决定发送的数据量
    segment_content_size =
        writer().bytes_pushed() - input_.reader().bytes_popped();
  }
  TCPSenderMessage msg;
  uint64_t segment_size =
      segment_content_size; // 整个segment的大小，包括SYN和FIN
  if (!SYN_sent_) {
    msg.SYN = true;
    msg.seqno = isn_;
    segment_size += 1;
  } else {
    msg.SYN = false;
    msg.seqno = last_send_ + static_cast<uint32_t>(1);
  }
  if (writer().is_closed() &&
      input_.reader().bytes_popped() + segment_content_size ==
          writer().bytes_pushed() &&
      segment_size + input_.reader().bytes_popped() <
          window_size + last_ackno_ - 1) {
    msg.FIN = true;
    end_ = true;
    segment_size += 1;
  } else {
    msg.FIN = false;
  }
  last_send_ = msg.seqno + static_cast<uint32_t>(segment_size - 1);
  if (writer().has_error()) {
    msg.RST = true;
  }
  if (segment_size == 0) {
    return;
  }
  SYN_sent_ = true;
  msg.payload = reader().peek().substr(0, segment_size);
  input_.reader().pop(segment_size);
  transmit(msg);
  message_queue_.push_back(msg);
  if (!timer_running_) {
    timer_running_ = true;
    timer_ = 0;
  }
  push(transmit); // 递归调用，直到不能发送为止
  // end_ == true 或者 segment_size == 0 都会导致递归结束
}

TCPSenderMessage TCPSender::make_empty_message() const {
  TCPSenderMessage msg;
  if (!SYN_sent_) {
    msg.SYN = true;
    msg.seqno = isn_;
  } else {
    msg.SYN = false;
    msg.seqno = last_send_ + static_cast<uint32_t>(1);
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
  receiver_window_size_ = msg.window_size;
  if (msg.ackno != std::nullopt) {
    if (last_ackno_ == (*msg.ackno).unwrap(isn_, last_ackno_)) {
      // do nothing
      // 讲义并没有要求重复ack的处理，所以这里什么都不做
    } else {
      uint64_t tmp = (*msg.ackno).unwrap(isn_, last_ackno_);
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
}

void TCPSender::tick(uint64_t ms_since_last_tick,
                     const TransmitFunction &transmit) {
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
