#pragma once

#include <cstring>
#include <string>
#include <vector>

#include "cloak.h"

namespace cloak {
namespace internal {
class StringUart {
 public:
  StringUart(const uint8_t *data, size_t size) {
    this->data_ = new uint8_t[size];
    this->size_ = size;
    std::memcpy(this->data_, data, size);
  }
  StringUart(const char *data) {
    auto size = std::strlen(data);
    this->data_ = new uint8_t[size / 2];
    auto ptr = data;
    auto end = data + size;
    while (ptr < end) {
      if ((*ptr == ' ' || *ptr == '.' || *ptr == '-' || *ptr == ':')) {
        ptr++;
        continue;
      }
      auto byte = char2int(*ptr) << 4;
      ptr++;
      if (ptr < end) {
        byte += char2int(*ptr);
        this->data_[this->size_++] = byte;
        ptr++;
      }
    }
  }

  virtual ~StringUart() { delete this->data_; }

  bool read_array_(void *data, size_t size) {
    if (this->pos_ + size > this->size_) {
      return false;
    }
    std::memcpy(data, &this->data_[this->pos_], size);
    this->pos_ += size;
    return true;
  }

  int available_() const {
    int av = this->size_ - this->pos_;
    return av;
  }

  bool read_array(void *data, size_t size) {
    if (size > this->available()) {
      return false;
    }
    return this->read_array_(data, size);
  }

  int available() {
    auto av = this->available_();
    auto res = this->av_ < av ? this->av_ : av;
    this->inc_av();
    return res;
  }

  void inc_av() {
    if (this->av_ < this->available_()) {
      this->av_++;
    }
  }

  int read() {
    uint8_t ch;
    return this->read_byte(&ch) ? ch : -1;
  }
  bool read_byte(uint8_t *ch) { return this->available() > 0 ? this->read_array(ch, 1) : false; }
  bool peek_byte(uint8_t *data) {
    if (this->available()) {
      *data = this->data_[this->pos_];
      return true;
    }
    return false;
  }

 private:
  size_t size_{};
  size_t pos_{};
  uint8_t *data_;
  size_t av_ = 1;
};

}  // namespace internal
}  // namespace cloak
