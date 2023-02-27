#ifndef TABLE_SKIPLIST_H
#define TABLE_SKIPLIST_H

#include <iostream>
#include <string> 
#include <vector>
#include <fstream> 
#include <random>
#include <sstream>
#include <mutex> 
#include <assert.h>
#include "memory_pool.h"
#include "byte_array.h"


#define TABLE_DEBUG

namespace table {

class SkipList{
private : 
    const int MAX_LEVEL = 16 ; // 该跳表的最大层级数
    int cur_skiplist_level ;   // 当前跳表所在的层级
    std::mutex _mutex; 

    struct Node {
        ByteArray key ; 
        ByteArray value ; 
        int level ; 
        std::vector<Node*> next ; // 是一个指针数组，有很多层，每一场都有指向下一个层级的索引 
        
        explicit Node(const ByteArray& key , const ByteArray& value , const int & level) {
            this->key = key ; 
            this->value = value ; 
            this->level =  level ; 
            this->next.resize(this->level , nullptr) ; 
        }
        ~Node() {
            delete [] this->key.data() ; 
            delete [] this->value.data() ; 
            this->next.clear() ; 
        }
    }; 

    Node *head ;

    Node* new_node(const ByteArray& key, const ByteArray& value, int height);
    
    void  delete_node(Node* node);

    void remove_node(Node* node, Node** prev);

    int get_random_level() const ; 

    //找到每一层 i 小于目标值 targetKey 的最大节点 pre[i]，最后 pre 中存的就是每一层小于 target 的最大节点
    void find_prekey(const ByteArray& targetKey , Node ** prev) const ;


public : 

    class Iterator {
    private  :
        Node *_node ; 
    public : 
        Iterator() : _node(nullptr) { } ;
        Iterator(Node *node) : _node(node) { }; 
        ~Iterator() { }; 

        bool good()                 { return this->_node != nullptr ; }

        void next()                 { this->_node = this->_node->next[0] ; }

        const ByteArray& key()      { return this->_node->key ; } 

        const ByteArray& value()    { return this->_node->value ; }  
    };

    SkipList() ; 

    ~SkipList() ; 

    Iterator begin();

    Iterator insert(const ByteArray& key, const ByteArray& value);

    bool erase(const ByteArray& key);
    
    Iterator update(const ByteArray& key, const ByteArray& new_value);
    
    Iterator lookup(const ByteArray& key);

    // Non-copying
    SkipList(const SkipList&) = delete;
    SkipList& operator=(const SkipList&) = delete;

#ifdef TABLE_DEBUG
    std::string serialize();
#endif
} ; 
 
 
SkipList::SkipList(){
    this->cur_skiplist_level = 1 ; 
    this->head = new_node("head" , "head" , MAX_LEVEL) ; 
}

SkipList::~SkipList(){
    Node *cur = this->head ; 
    while(cur->next[0] != nullptr) {
        Node *next = cur->next[0] ; 
        delete cur ; cur = next ;  
    }
    delete cur ; 
}

inline int SkipList::get_random_level() const{
    int level = 1 ; 
    std::mt19937 mt_rand{std::random_device{}()};
    while(level <= this->MAX_LEVEL && (mt_rand() % 2)) {
        
        ++level ; 
    }
    return level ; 
}

void my_memcpy(void *dest , const void *src , const size_t& size){
    
    assert(dest != nullptr) ; 
    assert(src != nullptr) ; 

    char *d = reinterpret_cast<char*>(dest)  ; 
    const char *s = reinterpret_cast<const char*>(src) ; 
    size_t tmpLen = size ; 
    if(d > s && d < s + size) {
        d = d + tmpLen - 1 ; 
        s = s + tmpLen - 1 ;
        while(tmpLen--) {
            *d-- = *s-- ; 
        }
    }else {
        while(tmpLen--){
           // std::cout<<*d<<" "<<*s<<" tmpL= "<<tmpLen<<std::endl;
            *d++ = *s++ ; 
        }
    }
}

SkipList::Node* SkipList::new_node(const ByteArray& key, const ByteArray& value, int height) {

    char *new_key = new char[key.size()] ; 
    my_memcpy(new_key , key.data() , key.size()) ; 
    char *new_value = new char[value.size()] ; 
    my_memcpy(new_value , value.data() , value.size()) ;  
     
    // 一定要将 key.size() 和 value.size() 赋给新开的节点，因为构造函数里面的 strlen() 根本就无法判断出函数
    ByteArray _key(new_key , key.size()) , _value(new_value , value.size()) ;
    Node *node = new Node(_key , _value , height) ; 
    // std::cout<<std::string(node->key.data() , node->key.size())<<" "<<std::string(node->value.data() , node->value.size())<<std::endl ; 
    return node ; 
}

void SkipList::delete_node(Node *node){
    delete node ; 
}

SkipList::Iterator SkipList::begin() {
    return Iterator(this->head->next[0]) ; 
}

void SkipList::find_prekey(const ByteArray& targetKey, Node ** prev) const{
    
    Node* cur = this->head;
    for(int i = MAX_LEVEL - 1 ; i >= 0 ; --i){
        while(cur->next.size() > 0 && cur->next[i] != nullptr && cur->next[i]->key < targetKey){
            cur = cur->next[i] ; 
        }
        prev[i] = cur ; 
    }
     
}


SkipList::Iterator SkipList::insert(const ByteArray& key, const ByteArray& value) {
    Node *prev[MAX_LEVEL] = {nullptr} ; 
    this->find_prekey(key , prev) ; 
    // prev[0]->next[0] is must null , because new head will resize next size ;
    if(prev[0]->next[0] != nullptr && prev[0]->next[0]->key == key){ 
         return Iterator(nullptr) ; 
    }
     
    int random_level = this->get_random_level() ; 
    this->cur_skiplist_level = std::max(this->cur_skiplist_level , random_level) ; 

    std::lock_guard<std::mutex> lock(_mutex);
    Node *insert_node = new_node(key , value , random_level) ; 
    for(int i = 0 ; i < random_level ; ++i) {
        insert_node->next[i] = prev[i]->next[i] ; 
        prev[i]->next[i] = insert_node ; 
    }
     
    return Iterator(insert_node) ; 
}

bool SkipList::erase(const ByteArray &key) {
    Node *prev[MAX_LEVEL] = {nullptr} ; 
    this->find_prekey(key , prev) ; 

    std::lock_guard<std::mutex> lock(_mutex);
    if(prev[0]->next[0] != nullptr && prev[0]->next[0]->key == key){
        Node* node = prev[0]->next[0] ; 
        for(int i = 0 ; i < this->MAX_LEVEL ; ++i){
            if(prev[i]->next[i] != nullptr && prev[i]->next[i]->key == key){
                prev[i]->next[i] = prev[i]->next[i]->next[i] ;         
            }else {
                break ;// already find over 
            }
        }
        delete_node(node) ; 
        return true ; 
    }
    
    return false ;  
}
 
SkipList::Iterator SkipList::update(const ByteArray& key, const ByteArray& new_value) {
    Node *prev[MAX_LEVEL] = {nullptr} ; 
    this->find_prekey(key , prev) ; 
    std::lock_guard<std::mutex> lock(_mutex);
    if(prev[0]->next[0] != nullptr && prev[0]->next[0]->key == key && prev[0]->next[0]->value != new_value){
        Node* node = prev[0]->next[0] ; 
        Node *insert_node = new_node(key , new_value , node->level) ;
        for(int i = 0 ; i < this->MAX_LEVEL ; ++i){
            if(prev[i]->next[i] != nullptr && prev[i]->next[i]->key == key){
                prev[i]->next[i] = insert_node ; 
                insert_node->next[i] = node->next[i] ;         
            }else {
                break ;// already find over 
            }
        }
        delete_node(node) ; 
        return Iterator(insert_node) ; 
    }
    return Iterator(nullptr) ; 
}

SkipList::Iterator SkipList::lookup(const ByteArray& key) {
    Node *prev[MAX_LEVEL] = {nullptr} ; 
    this->find_prekey(key , prev) ; 
    if(prev[0]->next[0] != nullptr && prev[0]->next[0]->key == key){
        return Iterator(prev[0]->next[0]) ; 
    }
    return Iterator(nullptr) ; 
}

#ifdef TABLE_DEBUG
std::string SkipList::serialize() {
    std::stringstream sstr;

    int height = this->MAX_LEVEL - 1;
    while (height >= 0) {
        
        Node *p = this->head->next[height];
        if(p != nullptr) {
            sstr << "height " << height << ": ";
        }
        while (p) {
            sstr << std::string(p->key.data() , p->key.size())<<" "<<p->key.size()<<":"<<std::string(p->value.data() , p->key.size()) << "    ";
            p = p->next[height];
            if(p == nullptr) {
                 sstr << std::endl;
            }
        }
        --height;
    }

    return sstr.str();
}
#endif
}

#endif