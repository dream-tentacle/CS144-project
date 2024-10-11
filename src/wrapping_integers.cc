#include "wrapping_integers.hh"
#include <iostream>
#include <string>
using namespace std;

Wrap32 Wrap32::wrap(uint64_t n, Wrap32 zero_point) {
  // Your code here.
  return Wrap32(
      static_cast<uint32_t>((n + zero_point.raw_value_) % (1LL << 32)));
}

uint64_t Wrap32::unwrap(Wrap32 zero_point, uint64_t checkpoint) const {
  // Your code here.
  uint64_t approx = checkpoint & ~0xFFFFFFFFUL;
  uint32_t diff = raw_value_ - zero_point.raw_value_;
  uint64_t re = approx + static_cast<uint64_t>(diff);
  // if (checkpoint == 3 * (1UL << 32) && diff == UINT32_MAX - 1) {
  //   string tmp = "checkpoint: " + to_string(checkpoint) +
  //                ", diff: " + to_string(diff) + ", re: " + to_string(re) +
  //                ", approx: " + to_string(approx);
  //   throw runtime_error(tmp);
  // }
  if (re > checkpoint) {
    if (re < (1LL << 32)) {
      return re;
    }
  } else {
    if (re + (1LL << 32) < re) {
      return re;
    }
    re += 1LL << 32;
  }
  if (re - checkpoint < checkpoint - (re - (1LL << 32))) {
    return re;
  } else {
    return re - (1LL << 32);
  }
}
