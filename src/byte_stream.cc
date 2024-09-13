#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream(uint64_t capacity) : capacity_(capacity), buffer_("") {}

bool Writer::is_closed() const {
  // Your code here.
  return closed_;
}

void Writer::push(string data) {
  // Your code here.
  // push the data to the stream, but only as much as available capacity allows.
  long unsigned int bytes_can_be_pushed = capacity_ - buffer_.size();
  if (bytes_can_be_pushed < data.size()) {
    buffer_ += data.substr(0, bytes_can_be_pushed);
    bytes_pushed_ += bytes_can_be_pushed;
  } else {
    buffer_ += data;
    bytes_pushed_ += data.size();
  }
  return;
}

void Writer::close() {
  // Your code here.
  closed_ = true;
}

uint64_t Writer::available_capacity() const {
  // Your code here.
  return capacity_ - buffer_.size();
}

uint64_t Writer::bytes_pushed() const {
  // Your code here.
  return bytes_pushed_;
}

bool Reader::is_finished() const {
  // Your code here.
  return closed_ && buffer_.empty(); // closed and fully popped
}

uint64_t Reader::bytes_popped() const {
  // Your code here.
  return bytes_popped_;
}

string_view Reader::peek() const {
  // Your code here.
  return string_view(buffer_);
}

void Reader::pop(uint64_t len) {
  // Your code here.
  if (len > buffer_.size()) {
    bytes_popped_ += buffer_.size();
    buffer_.clear();
  } else {
    bytes_popped_ += len;
    buffer_ = buffer_.substr(len);
  }
}

uint64_t Reader::bytes_buffered() const {
  // Your code here.
  return buffer_.size();
}
