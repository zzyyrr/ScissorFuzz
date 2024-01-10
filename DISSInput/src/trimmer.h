#pragma once
#include "seg_tag.h"
#include "cstring"
#include "unordered_set"
#include "iostream"

using namespace std;

typedef uint32_t node_id;

typedef struct hdd_tree_node {
    node_id id;
    tag_off begin;
    tag_off end;
    uint32_t callstack;
    vector<hdd_tree_node*> children;
} hdd_tree_node;

/*
typedef struct input_trim_seg {
    tag_off begin;
    tag_off end;
} input_trim_seg;
*/
int trimmer_init(const char* output_name);
void startTrim();


typedef struct input_char {
    char val;
    bool disable;
} input_char;

class InputModifier{
    private:
        vector<input_char*> input;
        unordered_set<tag_off> tryCutOffset;
    public:
        InputModifier(const char* originInput){
            size_t len = strlen(originInput);
            for(size_t i = 0; i < len; i++){
                input.push_back(new input_char{originInput[i], false});
            }
            cout << "origin input length=" << len << "," << input.size() << endl;

        }
        
        vector<char> tryCut(tag_off begin, tag_off end){
            tryCutOffset.clear();
            for (tag_off i = begin; i < end; i++){
                tryCutOffset.insert(i);
            }
            vector<char> cutResult; 
            tag_off j = 0;
            for(vector<input_char*>::iterator it = input.begin(); it != input.end(); it++){
                input_char* curChar = *it;
                if (!curChar->disable && tryCutOffset.find(j) == tryCutOffset.end()){
                    cutResult.push_back(curChar->val);
                }
                j++;
            }
            return cutResult;
        }

        void confirmCut(){
            int i = 0;
            for(unordered_set<tag_off>::iterator it = tryCutOffset.begin(); it != tryCutOffset.end(); it++){
                input.at(*it)->disable = true;
                i++;
            }
        }
};