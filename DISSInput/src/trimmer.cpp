#include "trimmer.h"
#include "unistd.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "time.h"
#include "iostream"
#include "stack"
#include "queue"
#include "list"
#include "unordered_set"
#include "unordered_map"
#include <cstdlib>
#include <fstream>
#include "cJSON.h"



using namespace std;

static double skipNodesRateThreashold = 0.6;
static double skipNodesNumThreashold = 100;

extern std::vector<SegTag*> tags;
// list<input_trim_seg*> trimSegList;
hdd_tree_node* tree = nullptr;
ofstream outFile; 

int trimmer_init(const char *output_name){
    outFile.open(output_name);
    if(!outFile || !outFile.is_open()){
        return 1;
    }
    return 0;
}

hdd_tree_node* createChildNode(SegTag* childTag){
    hdd_tree_node* newNode = new hdd_tree_node{ 0 };
    newNode->id = childTag->id;
    newNode->begin = childTag->begin;
    newNode->end = childTag->end;
    newNode->callstack = childTag->callstack;
    return newNode;
}

void buildTree(vector<SegTag*> tags){
    LOGD("Start build tree\n");
    tree = new hdd_tree_node{ 0 };
    unordered_set<std::string> exists_node;
    unordered_map<node_id, hdd_tree_node*> nodeMap;
    nodeMap.insert(make_pair(0, tree));
    
    queue<SegTag*> tagQueue;
    for (auto i = tags.rbegin(); i != tags.rend(); ++i){
        if ((*i)->temp){
            continue;
        }
        
        string tag_key = to_string((*i)->begin) + "-" + to_string((*i)->end);
        if (exists_node.count(tag_key) > 0){
            continue;
        } else {
            exists_node.insert(tag_key);
        }
        nodeMap.insert(make_pair((*i)->id, createChildNode((*i))));
        tagQueue.push((*i));
    }

    while (!tagQueue.empty()){
        SegTag* tag = tagQueue.front();
        unordered_map<node_id, hdd_tree_node*>::iterator parentIter = nodeMap.find(tag->parent);
        if (parentIter != nodeMap.end()) {
            parentIter->second->children.push_back(nodeMap.find(tag->id)->second);
        }
        tagQueue.pop();
    }
}

cJSON* node2Json(hdd_tree_node *treeNode){
    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "begin", treeNode->begin);
    cJSON_AddNumberToObject(json, "end", treeNode->end);
    cJSON *childrenArray = cJSON_CreateArray();
    cJSON_AddItemToObject(json, "children", childrenArray);
    for (auto i : treeNode->children){
        cJSON_AddItemToArray(childrenArray, node2Json(i));
    }
    return json;
}

bool reduceSeperateNode(hdd_tree_node *treeNode, int deep){
    if (treeNode->children.size() == 0){
        LOGD("%*s", deep * 2, "");
        LOGD("reduce tag finish id=%u :no child\n", treeNode->id);
        return true;
    }
    if (treeNode->children.size() >= skipNodesNumThreashold && treeNode->children.size() >= (treeNode->end-treeNode->begin) * skipNodesRateThreashold){
        LOGD("%*s", deep * 2, "");
        LOGD("reduce tag finish id=%u :too many child\n", treeNode->id);
        treeNode->children.clear();
        return true;
    }
    
    unordered_set<uint32_t> callstack_set;
    unordered_set<uint32_t> dup_callstack_set;
    LOGD("%*s", deep * 2, "");
    LOGD("start reduce tag:{%u, %u, %u, %ld}\n", treeNode->id, treeNode->begin, treeNode->end, treeNode->children.size());
    for (auto i : treeNode->children){
        if (callstack_set.count(i->callstack) > 0){
            dup_callstack_set.insert(i->callstack);
        } else {
            callstack_set.insert(i->callstack);
        }
    }
    bool r = true;
    for (vector<hdd_tree_node*>::iterator i = treeNode->children.begin(); i != treeNode->children.end(); i++){
        LOGD("%*s", deep * 2, "");
        LOGD("  child tag loop:{%u, %u, %u}\n", (*i)->id, (*i)->begin, (*i)->end);
        bool child_r = reduceSeperateNode(*i, deep + 1);
        if (child_r && dup_callstack_set.count((*i)->callstack) <= 0){
            treeNode->children.erase(i--);
        } else {
            r = false;
        }
    }
    LOGD("%*s", deep * 2, "");
    LOGD("reduce tag finish id=%u r=%d\n", treeNode->id, r);
    return r;
}

void startReduceSeperateNode(){
    for(auto i : tree->children){
        reduceSeperateNode(i, 0);
    }
}

void reduceSingleChildNode(hdd_tree_node *treeNode){
    if (treeNode->children.size() == 0){
        return;
    }
    if (treeNode->children.size() == 1){
        //level up
        *treeNode = *treeNode->children.at(0);
    }
    for (auto i : treeNode->children){
        reduceSingleChildNode(i);
    }
    
}

void startReduceSingleChildNode(){
    for (auto i : tree->children){
        reduceSingleChildNode(i);
    }
}

void startTrim() {
    buildTree(tags);

    startReduceSeperateNode();
    
    startReduceSingleChildNode();

    cJSON *json = node2Json(tree);

    char *buf = cJSON_PrintUnformatted(json);

    outFile.write(buf, strlen(buf));

    outFile.close();

    cJSON_Delete(json);

}
