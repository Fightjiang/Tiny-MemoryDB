#ifndef HUFMAN_CODE_H
#define HUFMAN_CODE_H

// 只对 key 和 value 进行 huffman 压缩
// 前面的 len_key 和 len_value 改成压缩完所占用的字节数
// 所以这个文件只需要做的是：
// 1. 统计词频，建立 哈弗曼编码
// 2. 根据 bits 查找对应的字符 
// 3. 根据字符写入对应的编码 
// 4. 保存/读取编码文件
#include <string> 
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <queue>
#include <memory>
 
#include <unistd.h> // close_file_fd
#include <fcntl.h> // open file_fd
#include "byte_array.h"
namespace table {
 
#define		TARGETCODE_FILE_EXT		".huffman_code"

class HuffmanTree{
public : 
    HuffmanTree() ; 
    ~HuffmanTree() ; 
    bool insert_word(const ByteArray &word) ; 
    bool build_huffmanTree() ; 
    bool save_encryptedFile(const char *fileName) ; 
    bool decrypt_File(const char *fileName) ;  
    bool write_string(std::shared_ptr<int> &fd , const ByteArray &str) const ;
    std::string read_string(std::shared_ptr<char> &data , const off_t offset , const uint16_t len) const ; 
private : 

    struct HuffmanNode {
        const char _ch ; 
        int _weight ; 
        struct HuffmanNode * _left , *_right ; 
        HuffmanNode() : _ch(0) , _weight(0) , _left(nullptr) , _right(nullptr) {}
        HuffmanNode(const char ch , int weight) : _ch(ch) , _weight(weight) , _left(nullptr) , _right(nullptr) {}
    } ; 

    std::unordered_map<char , int> frequencyTable ; 
    struct cmp {
        bool operator() (const HuffmanNode* a, const HuffmanNode* b) const {
            return a->_weight > b->_weight;
        }
    };
    std::priority_queue<HuffmanNode* , std::vector<HuffmanNode*> , cmp> smallHeap ;
    std::unordered_map<char , uint16_t> huffmanCodeTable ; 
    std::unordered_map<uint16_t , char> r_huffmanCodeTable ; 
    struct HuffmanNode * head ; 
    
    void makeHuffCode(const HuffmanNode *root , uint16_t s) ;
    void destroyTree(const HuffmanNode *root) ; 
} ; 

HuffmanTree::HuffmanTree() : head(nullptr) {}
HuffmanTree::~HuffmanTree() {
    this->destroyTree(this->head) ; 
}

void HuffmanTree::destroyTree(const HuffmanNode* root) {
    if(root == nullptr) return ; 
    destroyTree(root->_left) ; 
    destroyTree(root->_right) ; 
    delete root ; 
}

bool HuffmanTree::insert_word(const ByteArray &word){
    for(uint16_t i = 0 ; i < word.size() ; ++i) {
        this->frequencyTable[word[i]]++ ; 
        //std::cout<<word[i]<<" "<<this->frequencyTable[word[i]]<<std::endl ;
    }
    return true ; 
}
// 权重越高，出现次数越频繁，越靠近树的根节点
bool HuffmanTree::build_huffmanTree() {

    if(this->frequencyTable.empty()) return true ; 
    
    for(const auto &iter : this->frequencyTable){
        smallHeap.push(new HuffmanNode(iter.first , iter.second)) ; 
    } 
    //std::cout<<smallHeap.top()->_ch<<" "<<smallHeap.top()->_weight<<std::endl ;
    while(smallHeap.size() > 1) {
        HuffmanNode* tmp = new HuffmanNode() ; 
        tmp->_left = smallHeap.top() ; smallHeap.pop() ; 
        tmp->_right = smallHeap.top() ; smallHeap.pop() ; 
        tmp->_weight = tmp->_left->_weight + tmp->_right->_weight ; 
        smallHeap.push(tmp) ; 
    }
    this->head = smallHeap.top() ; smallHeap.pop() ; 
    this->huffmanCodeTable.clear() ; this->r_huffmanCodeTable.clear() ; 
    makeHuffCode(this->head , 1) ; 
    return true ;  
}

void HuffmanTree::makeHuffCode(const HuffmanNode *root , uint16_t code) {
    
    if(root->_left == nullptr && root->_right == nullptr) {
        this->huffmanCodeTable[root->_ch] = code ; 
        this->r_huffmanCodeTable[code] = root->_ch ; 
        //std::cout<<root->_ch<<" "<<this->huffmanCodeTable[root->_ch]<<std::endl ; 
        return ;
    }
    makeHuffCode(root->_left  , (code << 1) | 0) ; 
    makeHuffCode(root->_right , (code << 1) | 1) ; 
}

bool HuffmanTree::save_encryptedFile(const char *fileName){
     
    std::ofstream outfile; 
    outfile.open(fileName) ; 
    if(outfile.is_open() == false) {
        return false ; 
    }
	for(const auto &it : huffmanCodeTable) {
        //std::cout<<it.first<<" "<<it.second<<std::endl ;
        outfile<<it.first<<" "<< it.second<<std::endl ;
    }
    outfile.close() ; 
    return true ; 
}

bool HuffmanTree::decrypt_File(const char *fileName) {
    std::ifstream infile ; 
    infile.open(fileName) ; 
    if(infile.is_open() == false) {
        return false ; 
    }
    while (infile.peek() != EOF) {
        char ch = infile.get() ; 
        uint16_t code ; infile >> code ; 
        infile.get(); // get \n
        //std::cout<<ch<<" "<<code<<std::endl ;
        this->r_huffmanCodeTable[code] = ch ; 
    }
    infile.close() ; 
    return true ; 
}


bool HuffmanTree::write_string(std::shared_ptr<int> &fd , const ByteArray &str) const {
    uint16_t len = 0 ;
    // computer string len ; 
    for(uint8_t i = 0 ; i < str.size() ; ++i){
        auto iter = this->huffmanCodeTable.find(str[i]) ; 
        if(iter == this->huffmanCodeTable.end()){
            return false ; 
        }       
        uint16_t code = iter->second ; 
        for(uint16_t index = 7 ; i >= 0 ; --index){
            if(code & (1 << index)) {
                len = len + index + 1; 
                //std::cout<<len<<" "<<index<<std::endl ;
                break ; 
            }
        }
    } 
    len = (len + 7) / 8 ; // 每个字符串按照 1 个字节进行对齐。
    //std::cout<<"len = "<<len<<std::endl ;
    if (write(*fd, &len , 1) == -1) { // 写入一个字节就够了，因为不会有超过 2^8 个字符
        return false ; 
    }
    uint8_t bitIndex = 0 , byte = 0  ; 
    for(uint8_t i = 0 ; i < str.size() ; ++i){
        auto iter = this->huffmanCodeTable.find(str[i]) ;     
        uint16_t code = iter->second , flag = 0 ; 
        for(int8_t index = 7 ; index >= 0 ; --index){
            if(code & (1 << index)) {
                flag = 1 ; 
            }
            if(flag == 1) {
                if(code & (1 << index)) { // 1 
                    byte = byte | (1 << (7 ^ (bitIndex)) ) ; 
                }// else 0 , 那么就不用动 , 但是 bitIndex 需要 ++ 
                ++bitIndex ; 
                if(bitIndex == 8){
                    if (write(*fd, &byte , 1) == -1) {
                        return false ; 
                    }
                    bitIndex = 0 ; byte = 0 ;
                }
            }
        }
    }

    if(bitIndex){
        if (write(*fd, &byte , 1) == -1) {
            return false ; 
        }
    }
    return true ; 
}

std::string HuffmanTree::read_string(std::shared_ptr<char> &data , const off_t offset , const uint16_t len) const{
    std::string str ; 
    uint16_t ZeroOne = 0 ; 
    for(uint8_t i = 0 ; i < len ; ++i ) {
        uint8_t Bit = *reinterpret_cast<uint8_t*>(data.get() + offset + i) ;
        for(int index = 7 ; index >= 0; --index){
            ZeroOne =  ZeroOne | ((Bit >> index) & 1) ; 
            auto iter = this->r_huffmanCodeTable.find(ZeroOne) ; 
            if(iter != this->r_huffmanCodeTable.end()){
                str.push_back(iter->second) ; 
                ZeroOne = 0 ; 
            } else {
                ZeroOne = ZeroOne << 1 ; 
            }
        } 
    }
    return str ; 
}

} // namespace table
#endif