#ifndef __INS_COMMON_OP_H__
#define __INS_COMMON_OP_H__
#include "pin.H"
#include "tag_traits.h"


tag_t get_m2r_tag(ADDRINT src, uint32_t callstack);

void taint2m_op(ADDRINT dst, tag_entity *src_tag, bool hasNext);

void r2m_op(ADDRINT dst, tag_t src_tag_id, bool hasNext);


#endif