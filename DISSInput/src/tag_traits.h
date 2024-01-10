#ifndef LIBDFT_TAG_TRAITS_H
#define LIBDFT_TAG_TRAITS_H

#include <string>
#include "./seg_tag.h"

#define L false
#define R true

typedef tag_id tag_t;
typedef SegTag tag_entity;

struct tag_traits {
  static tag_id cleared_val;
};

SegTag* tag_combine(SegTag* lhs, SegTag* rhs, bool lr);

std::string tag_sprint(SegTag* tag);

SegTag* tag_alloc(tag_off begin, tag_off end, tag_id parent, bool temp, uint32_t callstack);

SegTag* tag_get(tag_id t);

void printAllTags();

inline bool tag_is_empty(tag_t const &tag) {
  return tag == tag_traits::cleared_val;
}

#endif /* LIBDFT_TAG_TRAITS_H */