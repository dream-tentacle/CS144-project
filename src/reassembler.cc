#include "reassembler.hh"

using namespace std;
void Reassembler::check_end() {
  if (output_.writer().bytes_pushed() ==
          static_cast<uint64_t>(ending_index + 1) &&
      ending_index != -2) {
    // -2是初始化的值，表示还没有数据
    // 不设置成-1是因为如果数据为空，ending_index成为-1
    output_.writer().close();
  }
}
void Reassembler::process_data_store() {
  // 进行数据的push
  while (!data_store.empty()) {
    if (data_store.begin()->first.first_index != waiting_index)
      return;
    output_.writer().push(data_store.begin()->second);
    waiting_index = output_.writer().bytes_pushed();
    data_store.erase(data_store.begin());
    check_end();
  }
}
void Reassembler::delete_inner_info(reassembler_info info) {
  // 删除被info完全覆盖的数据
  for (auto it = data_store.lower_bound(info); it != data_store.end();) {
    if (it->first.last_index > info.last_index) {
      break;
    }
    it = data_store.erase(it);
  }
}
void Reassembler::insert(uint64_t first_index, std::string data,
                         bool is_last_substring) {
  if (is_last_substring) {
    // 记录末尾在哪里
    ending_index = first_index + data.length() - 1;
    check_end();
  }
  if (capacity() == 0 ||
      first_index >= waiting_index + capacity() || // 数据在能存储的范围之外
      first_index + data.length() <= waiting_index // 数据已经被push过了
  ) {
    return;
  }
  if (first_index < waiting_index) {
    // 前面已经push过的内容要去掉
    data = data.substr(waiting_index - first_index);
    first_index = waiting_index;
  }
  if (first_index + data.length() > waiting_index + capacity()) {
    // 后面存不下的数据要去掉
    data = data.substr(0, waiting_index + capacity() - first_index);
  }
  reassembler_info info = {first_index, first_index + data.length() - 1};
  delete_inner_info(info);
  // 这里先把内部覆盖的数据删掉，保证后续正确性。后续的it一定不被新进来的数据覆盖
  auto it = data_store.lower_bound(info);
  if (it != data_store.begin()) {
    auto pre = prev(it);
    // 处理前面的数据和新进来的数据有重叠的情况
    if (pre->first.last_index >= info.first_index) {
      // 如果成立，二者一定有重叠
      if (pre->first.last_index >= info.last_index) {
        // 直接被以前获取的覆盖了
        return;
      }
      data = data.substr(pre->first.last_index - info.first_index + 1);
      info.first_index = pre->first.last_index + 1;
    }
  }
  it = data_store.lower_bound(info);
  if (it != data_store.end()) {
    // 处理后面的数据和新进来的数据有重叠的情况
    if (it->first.first_index <= info.last_index) {
      // 如果成立，二者一定有重叠
      if (it->first.last_index >= info.last_index &&
          it->first.first_index <= info.first_index) {
        // 直接被以前获取的覆盖了
        return;
      }
      data = data.substr(0, it->first.first_index - info.first_index);
      info.last_index = it->first.first_index - 1;
    }
  }
  if (info.first_index <= info.last_index) {
    data_store[info] = data;
    process_data_store();
  }
  // 如果info.first_index > info.last_index,
  // 说明新进来的数据已经被完全覆盖了，这种情况出现在前后两个数据连在一起且覆盖了新进来的数据
  // 就不用存储了
}

uint64_t Reassembler::bytes_pending() const {
  uint64_t bytes = 0;
  for (auto &it : data_store) {
    bytes += it.second.length();
  }
  return bytes;
}
