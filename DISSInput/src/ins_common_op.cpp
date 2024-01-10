#include "ins_helper.h"
#include "ins_common_op.h"



tag_t get_m2r_tag(ADDRINT src, uint32_t callstack){
  tag_t src_tag_id = MTAG(src);
  if (!src_tag_id){
    return 0;
  }
  tag_entity* src_tag = tag_get(src_tag_id);
  ADDRINT firstAddr = getFirstAddr(src, src_tag_id);

  LOGD("[m2r taint!]  src=%p, src_tag=%s, src_tag_address=[%p,%p)\n", (void*)src, tag_sprint(src_tag).c_str(), (void*)firstAddr, (void*)(getFinalAddr(src, src_tag_id)));
  tag_off newOffset = src - firstAddr + src_tag->begin;  
  if (newOffset == src_tag->begin && src_tag->getLen() == 1){
    return src_tag_id;
  }

  tag_entity* newTag = tag_alloc(newOffset, (newOffset + 1), src_tag_id, true, callstack);
  return newTag->id;
}

void taint2m_op(ADDRINT dst, tag_entity *src_tag, bool hasNext){
  tag_entity* orig_tag = tag_get(MTAG(dst));
  if (orig_tag != NULL){
    ADDRINT origFirstAddr = getFirstAddr(dst, orig_tag->id);
    if (dst - origFirstAddr == src_tag->begin - orig_tag->begin){
        return;
    }
    tagmap_clrb(dst);
  }
  tag_entity* pre_tag = tag_get(MTAG(dst-1));
  tag_entity* next_tag = tag_get(MTAG(dst + src_tag->getLen()));
  ADDRINT firstAddr = dst;
  ADDRINT finalAddr = dst + src_tag->getLen();

  if (pre_tag && src_tag->begin == pre_tag->end){
    firstAddr = getFirstAddr(dst, pre_tag->id);
    LOGD("[taint2m taint!][combined with pre-tag]  lhs=%s, rhs=%s\n", tag_sprint(pre_tag).c_str(), tag_sprint(src_tag).c_str());
    src_tag = tag_combine(pre_tag, src_tag, true);
  }
  if (next_tag && src_tag->end == next_tag->begin){
    finalAddr = getFinalAddr(dst+1, next_tag->id);
    LOGD("[taint2m taint!][combined with next-tag]  lhs=%s, rhs=%s\n", tag_sprint(src_tag).c_str(), tag_sprint(next_tag).c_str());
    src_tag = tag_combine(src_tag, next_tag, false);
  }

  src_tag->temp = false;
  for (ADDRINT i = firstAddr; i < finalAddr; i++){
    tagmap_setb(i, src_tag->id);
  }

  LOGD("[taint2m taint!]  dst=%p, src_tag=%s, pre_tag=%s, next_tag=%s, orig_tag=%s, updateAddr=[%p,%p)\n", 
      (void*)dst, tag_sprint(src_tag).c_str(), tag_sprint(pre_tag).c_str(), tag_sprint(next_tag).c_str(), tag_sprint(orig_tag).c_str(), (void*)firstAddr, (void*)finalAddr);
}
 
void r2m_op(ADDRINT dst, tag_t src_tag_id, bool hasNext){
  if (!src_tag_id)
  {
    tagmap_clrb(dst);
    return;
  }
  tag_entity* src_tag = tag_get(src_tag_id);
  taint2m_op(dst, src_tag, hasNext);
  
}
