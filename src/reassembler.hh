#pragma once

#include "byte_stream.hh"
#include <map>

class Reassembler {
public:
  // Construct Reassembler to write into given ByteStream.
  explicit Reassembler(ByteStream &&output) : output_(std::move(output)) {}

  /*
   * Insert a new substring to be reassembled into a ByteStream.
   *   `first_index`: the index of the first byte of the substring
   *   `data`: the substring itself
   *   `is_last_substring`: this substring represents the end of the stream
   *   `output`: a mutable reference to the Writer
   *
   * The Reassembler's job is to reassemble the indexed substrings (possibly
   * out-of-order and possibly overlapping) back into the original ByteStream.
   * As soon as the Reassembler learns the next byte in the stream, it should
   * write it to the output.
   *
   * If the Reassembler learns about bytes that fit within the stream's
   * available capacity but can't yet be written (because earlier bytes remain
   * unknown), it should store them internally until the gaps are filled in.
   *
   * The Reassembler should discard any bytes that lie beyond the stream's
   * available capacity (i.e., bytes that couldn't be written even if earlier
   * gaps get filled in).
   *
   * The Reassembler should close the stream after writing the last byte.
   */
  void insert(uint64_t first_index, std::string data, bool is_last_substring);

  // How many bytes are stored in the Reassembler itself?
  uint64_t bytes_pending() const;

  // Access output stream reader
  Reader &reader() { return output_.reader(); }
  const Reader &reader() const { return output_.reader(); }

  // Access output stream writer, but const-only (can't write from outside)
  const Writer &writer() const { return output_.writer(); }

  // 用来记录数据的起始和结束位置，设成struct是为了方便map内部的比较
  struct reassembler_info {
    uint64_t first_index;
    uint64_t last_index;
    friend bool operator<(const reassembler_info &a,
                          const reassembler_info &b) {
      return a.first_index < b.first_index;
    }
  };

  // 存储数据的map，用map是因为内部以红黑树实现，查找效率高
  std::map<reassembler_info, std::string> data_store{};
  uint64_t waiting() const { return waiting_index; }
  uint64_t capacity() const { return output_.writer().available_capacity(); }

private:
  ByteStream output_; // the Reassembler writes to this ByteStream
  uint64_t waiting_index = 0;
  int64_t ending_index = -2; // 结尾的位置
  void delete_inner_info(reassembler_info info);
  void process_data_store();
  void check_end();
};
