#ifndef TABLE_BYTE_ARRAY_H
#define TABLE_BYTE_ARRAY_H

#include <string>
#include <string.h>
#include <iostream>
namespace table {

class ByteArray {
public:
    ByteArray();
    ~ByteArray();
    ByteArray(const char* data, const uint8_t &size);
    ByteArray(const char* str);
    ByteArray(const std::string& str);

    bool empty() const;
    uint8_t size() const;
    char* data();
    const char* data() const;
    void assign(const char* data,  const uint8_t &size);
    bool operator == (const ByteArray &other) const ; 
    bool operator != (const ByteArray &other) const ; 
    bool operator < (const ByteArray &other) const ; 
    bool operator > (const ByteArray &other) const ; 
    char operator [] (const uint8_t &index) const; 
    friend std::ostream & operator << (std::ostream &cout , const ByteArray &A) ; 

private:
    uint8_t      _size;
    const char *_data;
};

// 都是浅拷贝 
ByteArray::ByteArray() : _size(0) , _data(nullptr) { }
ByteArray::ByteArray(const char *data , const uint8_t &size) : _size(size) , _data(data) { }
ByteArray::ByteArray(const char *str) : _size(strlen(str)) , _data(str) { }
ByteArray::ByteArray(const std::string &str) : _size(str.size()) , _data(str.data()) { }
// 而且不释放内存空间，跳表删除时统一释放
ByteArray::~ByteArray(){ }

bool ByteArray::empty() const {
    return this->_size == 0 ; 
}

uint8_t ByteArray::size() const {
    return this->_size ; 
}

// can modify data value
char *ByteArray::data() {
    return const_cast<char*>(this->_data) ; 
}

const char *ByteArray::data() const { 
    return this->_data ; 
}

void ByteArray::assign(const char *data , const uint8_t &size){
    this->_data = data ; 
    this->_size = size ; 
}

// static int my_memcmp(const char *A , const char *B , size_t len) {
//     while(len--) {
//         std::cout<<*A<<" "<<*B<<" "<<" my_memcmp "<<(*A < *B)<<std::endl ;
//         if(*A > *B) return 1 ; 
//         else if(*A < *B) return -1 ; 
//         ++A ; ++B ; 
//     }
//     return 0 ; 
// }

bool ByteArray::operator == (const ByteArray &other) const{
    if (this->data() == other.data()) { 
        return true ; 
    }

    if(this->size() != other.size()) {
        return false ; 
    }
    return memcmp(this->data() , other.data() , this->size()) == 0 ; 
}

bool ByteArray::operator != (const ByteArray &other) const{
    
    if(this->size() != other.size()) {
        return true ; 
    }
    return memcmp(this->data() , other.data() , this->size()) != 0 ; 
}

bool ByteArray::operator < (const ByteArray &other) const{
    size_t size = std::min(this->_size , other.size()) ; 
    int cmp = memcmp(this->_data , other.data() , size) ; 
    if(cmp == 0) {
        if(this->_size < other.size()) return true ; 
        else return false ; 
    }else {
        return (cmp < 0) ? true : false ; 
    }
}

bool ByteArray::operator > (const ByteArray &other) const { 
    size_t size = std::min(this->_size , other.size()) ; 
    int cmp = memcmp(this->_data , other.data() , size) ; 
    if(cmp == 0) {
        if(this->_size > other.size()) return true ; 
        else return false ; 
    }else {
        return (cmp > 0) ? true : false ; 
    }
}

char ByteArray::operator [] (const uint8_t &index) const{
    if(index < this->_size) return this->data()[index] ; 
}

std::ostream & operator << (std::ostream &out , const ByteArray &other){
    out<<std::string(other.data() , other.size()) ;
    return out ; 
}

} // namespace table

#endif