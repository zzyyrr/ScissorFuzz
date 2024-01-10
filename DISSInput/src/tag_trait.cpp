#include "pin.H"
#include "debug.h"
#include "tag_traits.h"
#include <string.h>


std::vector<SegTag*> tags;
tag_id tag_traits::cleared_val = 0;

SegTag* tag_combine(SegTag* lhs, SegTag* rhs, bool leftMain) {
  if (lhs == NULL || rhs == NULL){
    return NULL;
  }
  
  SegTag* parentTag = tag_get(lhs->parent);
  if (parentTag != NULL && lhs->begin == parentTag->begin && rhs->end == parentTag->end)
  {
    lhs->temp = true;
    rhs->temp = true;
    return parentTag;
  }
  
  SegTag* newTag = tag_alloc(lhs->begin, rhs->end, lhs->parent, false, leftMain ? lhs->callstack : rhs->callstack);
  lhs->parent = newTag->id;
  rhs->parent = newTag->id;
  lhs->temp = true;
  rhs->temp = true;
  return newTag;

}

std::string tag_sprint(SegTag* tag) {
  if (tag == NULL)
  {
    return "{}";
  }
  
  return tag->toString();
}

SegTag* tag_alloc(tag_off begin, tag_off end, tag_id parent, bool temp, uint32_t callstack) {
  SegTag* newTag = new SegTag(begin, end, parent, callstack);
  newTag->temp = temp;
  tags.push_back(newTag);
  LOGD("[gen new tag!] tag:%s\n", newTag->toString().c_str());
  return newTag;
}


void printAllTags(){
  printf("start printAllTags\n");
  FILE * fp;

  fp = fopen ("tagList.out", "w+");

  for(auto i : tags){
    if (i->temp == false){
      fprintf(fp, "%s\n", i->toString().c_str());
    }
  }
  fclose(fp);

}


SegTag* tag_get(tag_id t) {
   if (t == 0){
      return NULL;
   }
   return tags.at(t-1);
}