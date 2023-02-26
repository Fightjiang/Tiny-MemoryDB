#ifndef TABLE_TABLE_H
#define TABLE_TABLE_H


#include <sys/stat.h>
#include <dirent.h>
#include <memory>
#include <unistd.h> // close_file_fd
#include <fcntl.h> // open file_fd
#include <sys/mman.h> // mmap 
#include <atomic> 

#include "status.h"
#include "options.h"
#include "byte_array.h"
#include "skiplist.h"
#include "memory_pool.h"
#include "hufman_code.h"

namespace table { 

class Table {
public : 
    // Open the table with the specified "name" that in constructor.
    // You should call Close() when it is no longer needed.
    Table(const Options& option , const std::string &filename) ; 
    
    ~Table() ; 

    // Returns OK on success.
    Status open();

    // Close the table.
    // You should not do any operation after the table is closed.
    // Returns OK on success.
    Status close();

    // Persist the entries to disk.
    // Returns OK on success.
    Status dump();

    // Store the corresponding value in *value if the table contains an entry for "key".
    // If value == nullptr, the corresponding value is not set.
    // Returns OK on success.
    Status get(const ByteArray& key, std::string* value);

    // Set the table entry for "key" to "value".
    // Returns OK on success.
    Status put(const ByteArray& key, const ByteArray& value);

    // Remove the table entry (if any) for "key".
    // It is an error if "key" did not exist in the table.
    // Returns OK on success.
    Status del(const ByteArray& key);

    // Non-copying
    Table(const Table&) = delete ;
    Table& operator=(const Table&) = delete ;

private : 
    std::atomic<bool> _is_closed ; 
    const std::string &_file_name ; 
    const Options& _options ;  
    SkipList *_skiplist ; 
    HuffmanTree *_HufTree ; 
}; 
 
Table::Table(const Options& option , const std::string &filename) : 
    _is_closed(true) , _file_name(filename) , _options(option) , 
    _skiplist(nullptr) , _HufTree(nullptr) { } // skiplist create when first put or open a existed file

Table::~Table(){
    this->close() ; 
}

Status Table::open() {
    
    if(!this->_is_closed) { 
        return Status::invalid_operation("Table was already open") ; 
    }

    struct stat info ; 
    bool table_exist = stat(this->_file_name.data() , &info) == 0 ;

    if(!table_exist && !this->_options.create_if_missing){
        return Status::io_error(this->_file_name + " does not exist") ; 
    }
    
    auto close_func = [](int* fd) {
        if (fd) {
            ::close(*fd);
            delete fd;
        }
    };
 
    std::shared_ptr<int> fd(new int(::open(this->_file_name.data(), O_RDONLY | O_CREAT , 0644)), close_func);
    if (*fd == -1) {
        return Status::io_error("open " + std::string(this->_file_name.data()) + " error, " + strerror(errno));
    }
    table_exist = stat(this->_file_name.data() , &info) == 0 ;
    if(table_exist && this->_options.error_if_exists) { 
        return Status::io_error(this->_file_name + " open file error");
    }
     
    // if (info.st_size > _options.max_file_size) {
    //     return Status::io_error("file " + std::string(this->_file_name.data()) + " is too large, "
    //                                 "max file size " + std::to_string(_options.max_file_size));
    // }

    // new SkipList
    if(this->_skiplist == nullptr) {
        this->_skiplist = new SkipList() ; 
    }
    // new HuffmanTree 
    if(this->_HufTree == nullptr) {
        this->_HufTree = new HuffmanTree() ; 
    }

    if (info.st_size > 0) {// read data
        // std::cout<<info.st_size<<std::endl ;
        if(this->_HufTree->decrypt_File(std::string(this->_file_name + TARGETCODE_FILE_EXT).data()) == false) {
            return Status::io_error(this->_file_name + TARGETCODE_FILE_EXT + " open huffman code file error");
        } 

        auto munmap_func = [&info](char *data){
            if(data != MAP_FAILED){
                munmap(data , info.st_size) ; 
            }
        } ; 
        std::shared_ptr<char> data(
            reinterpret_cast<char*>(mmap(nullptr, info.st_size, PROT_READ, MAP_PRIVATE , *fd , 0)), 
            munmap_func
        ) ; 
        if(data.get() == MAP_FAILED) {
            return Status::io_error("mmap " + std::string(this->_file_name.data()) + " error, " + strerror(errno));
        }

        // +--------------------Entry----------------------+
        // | length of key | key | length of value | value |
        // +-----------------------------------------------+
        off_t offset = 0 ; 
        while(true) {
            std::string key_str , value_str ; 

            if(info.st_size - offset >= static_cast<off_t>(sizeof(uint8_t))) { // 判断是否还有 key-value 
                uint16_t key_size = *reinterpret_cast<uint8_t*>(data.get() + offset++) ; // 读取 length of key bits 
                key_str = this->_HufTree->read_string(data , offset , key_size) ; 
                offset = offset + key_size ; 
                uint8_t value_size = *reinterpret_cast<uint8_t*>(data.get() + offset++) ;
                value_str = this->_HufTree->read_string(data , offset , value_size) ; 
                offset = offset + value_size ;
            }
            if(key_str.size() == 0) {
                break ; 
            }
            //std::cout<<key_str<<" "<<value_str<<std::endl ; 
            auto result = this->_skiplist->insert(key_str , value_str) ; 
            if(result.good() == false){
                return Status::invalid_operation(
                    "insert fail , maybe duplicate key = " + key_str + "value = " + value_str
                ) ;
            }
        }
    }
    _is_closed = false;
    return Status::ok();
}

Status Table::close() {
    if(this->_is_closed) {
        return Status::invalid_operation("Table is closed") ; 
    }

    if(this->_options.dump_when_close){
        Status s = this->dump() ; 
        if(!s.good()) {
            return s; 
        }
    }
    delete this->_skiplist ; this->_skiplist = nullptr ; 
    delete this->_HufTree ; this->_HufTree = nullptr ; 
    this->_is_closed = true ; 
    return Status::ok() ; 
}

Status Table::dump() {
     
    if(this->_is_closed){
        return Status::invalid_operation("Table is closed");
    }
     
    for(auto iter = this->_skiplist->begin() ;  iter.good() ; iter.next() ) {

        if(this->_HufTree->insert_word(iter.key()) == false ){
            return Status::invalid_operation("Huffman Tree insert key word fail " + *iter.key().data()) ;
        }  
        if(this->_HufTree->insert_word(iter.value()) == false) {
            return Status::invalid_operation("Huffman Tree insert value word fail" + *iter.value().data()) ;
        }
    }
     
    if(this->_HufTree->build_huffmanTree() == false) {
        return Status::invalid_operation("build Huffman Tree") ;
    }
    if(this->_HufTree->save_encryptedFile(std::string(this->_file_name + TARGETCODE_FILE_EXT).data()) == false) {
        return Status::invalid_operation("save Huffman Tree Code fail") ;
    }

    auto close_func = [](int *fd) {
        if(fd) {
            ::close(*fd) ; 
            delete fd ; 
        }
    } ; 
    std::shared_ptr<int> fd(new int(::open(this->_file_name.data(), O_WRONLY | O_CREAT | O_TRUNC, 0666)), close_func);
    if (*fd == -1) {
        return Status::io_error("open " + std::string(this->_file_name.data()) + " error, " + strerror(errno));
    }
    for(auto iter = this->_skiplist->begin() ; iter.good() ; iter.next() ) {
        // +--------------------Entry----------------------+
        // | length of key | key | length of value | value |
        // +-----------------------------------------------+
        if(this->_HufTree->write_string(fd , iter.key()) == false)
            return Status::io_error("write " + std::string(this->_file_name.data()) + " error, " + strerror(errno));
        
        if(this->_HufTree->write_string(fd , iter.value()) == false)
            return Status::io_error("write " + std::string(this->_file_name.data()) + " error, " + strerror(errno));
        
    }
    return Status::ok();
}

Status Table::get(const ByteArray &key , std::string *value){
    if (_is_closed) {
        return Status::invalid_operation("Table is closed");
    }

    auto it = this->_skiplist->lookup(key);
    if (!it.good()) {
        return Status::not_found();
    }

    if (value != nullptr) {
        value->assign(it.value().data(), it.value().size());
    }
    return Status::ok();
}

Status Table::put(const ByteArray& key, const ByteArray& value) {
    if (_is_closed) {
        return Status::invalid_operation("Table is closed");
    }

    uint8_t entry_size = key.size() + value.size() + sizeof(uint8_t) * 2;
    if (static_cast<off_t>(entry_size) > _options.max_file_size) {
        return Status::invalid_operation("size of entry is too large");
    }

    auto it = this->_skiplist->insert(key, value) ;
    
    if (it.good() == false) { // already exist , then update 
        this->_skiplist->update(key, value);
    }

    return Status::ok();
}

Status Table::del(const ByteArray& key) {
    if (_is_closed) {
        return Status::invalid_operation("Table is closed");
    }

    if (this->_skiplist->erase(key)) {
        return Status::ok();
    } else {
        return Status::not_found();
    }
}

}// namespace table

#endif