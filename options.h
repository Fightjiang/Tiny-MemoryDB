#ifndef TABLE_OPTIONS_H
#define TABLE_OPTIONS_H

#include "byte_array.h"

namespace table {

struct Options { 
   
    // 如果表文件没有存在，是否则创建
    bool create_if_missing = false;

    // 如果表文件已经存在，是否报错
    bool error_if_exists = false ;

    // 关闭文件的时候，是否可持久化到磁盘上
    bool dump_when_close = true ;

    // 文件的最大大小，也就是内存存储的键值最大字节大小，待实现中ing
    size_t max_file_size = 512 * 1024 * 1024 ;

} ;  

}// namespace leveldb

#endif