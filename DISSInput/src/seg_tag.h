#pragma once
#include <stdint.h>
#include <string>
#include <vector>


typedef uint32_t tag_id;
typedef uint32_t tag_off;
static tag_id g_index = 1;

class SegTag {
public:
  tag_id id;
  tag_id parent;
  tag_off begin;
  tag_off end;
  bool temp = false;
  uint32_t callstack;
  SegTag(){
  }

  SegTag(tag_id id_, tag_id parent_, tag_off begin_, tag_off end_, bool temp_, uint32_t callstack_) {
    begin = begin_;
    end = end_;
    parent = parent_;
    id = id_;
    temp = temp_;
    callstack = callstack_;
  };

  SegTag(tag_off begin_, tag_off end_, tag_id parent_, uint32_t callstack_) {
    begin = begin_;
    end = end_;
    parent = parent_;
    id = g_index++;
    callstack = callstack_;
  };
  std::string toString(){
      char buf[128];
      sprintf(buf, "{\"id\":%d, \"begin\":%d, \"end\":%d, \"parent\":%d, \"temp\":%d, \"callstack\":%u}", id, begin, end, parent, temp, callstack);
      std::string s(buf);
      return s;
  }
  size_t getLen() { return (end - begin); }
};