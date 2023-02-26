#include "hufman_code.h"
#include "status.h"
#include "skiplist.h"
#include <assert.h>
#include <unistd.h> // close_file_fd
#include <fcntl.h> // open file_fd
#include <sys/mman.h> // mmap 
#include <sys/stat.h>
#include <memory>
#include <vector>
using namespace table ; 
using namespace std ; 
static inline void my_assert(bool status , const string &s) {
    if(status == true) return ; 
    cout<<s<<endl ; 
    assert(true == false) ;
}

void test_encode(){
    HuffmanTree *tree = new HuffmanTree() ; 
    vector<string> strVec = {"test" , "abcd" , "aacc" , "vvcc"} ; 
    for(auto &str : strVec){
        tree->insert_word(str) ; 
    }

    my_assert(tree->build_huffmanTree() == true , "fail build tree" ) ; 
    my_assert(tree->save_encryptedFile("huffmanTest.code") == true , "fail save code") ; 

    auto close_func = [](int *fd) {
        if(fd) {
            ::close(*fd) ; 
            delete fd ; 
        }
    } ; 
    {
        // write 
        std::shared_ptr<int> fd1(new int(::open("test_huffman.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666)), close_func);
        
        my_assert((*fd1 != -1) , "read 1 file error") ; 
        for(auto &str : strVec) {
            cout<<str<<endl;
            bool status = tree->write_string(fd1 , str) ; 
            my_assert(status , "write file error") ; 
        }
    }


    // read 
    std::shared_ptr<int> fd2(new int(::open("test_huffman.txt" , O_RDONLY | O_CREAT , 0644)), close_func);
    my_assert((*fd2 != -1) , "read 2 file error") ; 
    struct stat info ; 
    bool table_exist = stat("test_huffman.txt" , &info) == 0 ;
    auto munmap_func = [&info](char *data){
        if(data != MAP_FAILED){
            munmap(data , info.st_size) ; 
        }
    } ; 
    std::shared_ptr<char> data(
        reinterpret_cast<char*>(mmap(nullptr, info.st_size, PROT_READ, MAP_PRIVATE , *fd2 , 0)), 
        munmap_func
    ) ;
    //cout<<"open file "<<info.st_size<<endl ;
    off_t offset = 0 ; 
    while(true) {
        string key_str , value_str ; 
        if(info.st_size - offset >= static_cast<off_t>(sizeof(uint8_t))) { // 判断是否还有 key-value 
            uint16_t key_size = *reinterpret_cast<uint8_t*>(data.get() + offset++) ; // 读取 length of key bits 
            key_str = tree->read_string(data , offset , key_size) ; 
            offset = offset + key_size ; 
            uint8_t value_size = *reinterpret_cast<uint8_t*>(data.get() + offset++) ;
            value_str = tree->read_string(data , offset , value_size) ; 
            offset = offset + value_size ;
        }
        if(key_str.size() == 0) {
            break ; 
        }
         
        cout<<key_str<<" "<<value_str<<endl ; 
    }
    
    delete tree ; 
}

void test_decode(){
    HuffmanTree *tree = new HuffmanTree() ; 
    my_assert(tree->decrypt_File("huffmanTest.code") == true , "fail decrypt code") ; 
    auto close_func = [](int *fd) {
        if(fd) {
            ::close(*fd) ; 
            delete fd ; 
        }
    } ; 
    // read 
    std::shared_ptr<int> fd2(new int(::open("test_huffman.txt" , O_RDONLY | O_CREAT , 0644)), close_func);
    my_assert((*fd2 != -1) , "read 2 file error") ; 
    struct stat info ; 
    bool table_exist = stat("test_huffman.txt" , &info) == 0 ;
    auto munmap_func = [&info](char *data){
        if(data != MAP_FAILED){
            munmap(data , info.st_size) ; 
        }
    } ; 
    std::shared_ptr<char> data(
        reinterpret_cast<char*>(mmap(nullptr, info.st_size, PROT_READ, MAP_PRIVATE , *fd2 , 0)), 
        munmap_func
    ) ;
    //cout<<"open file "<<info.st_size<<endl ;
    // | length of key | key | length of value | value |
    off_t offset = 0 ; 
    while(true) {
        string key_str , value_str ; 
        if(info.st_size - offset >= static_cast<off_t>(sizeof(uint8_t))) { // 判断是否还有 key-value 
            uint16_t key_size = *reinterpret_cast<uint8_t*>(data.get() + offset++) ; // 读取 length of key bits 
            key_str = tree->read_string(data , offset , key_size) ; 
            offset = offset + key_size ; 
            uint8_t value_size = *reinterpret_cast<uint8_t*>(data.get() + offset++) ;
            value_str = tree->read_string(data , offset , value_size) ; 
            offset = offset + value_size ;
        }
        if(key_str.size() == 0) {
            break ; 
        }
         
        cout<<key_str<<" "<<value_str<<endl ; 
    }
    
    delete tree ; 
}
int main(){
    test_encode() ;
    test_decode() ; 
    return 0 ; 
        
}