#ifndef TABLE_OPTIONS_H
#define TABLE_OPTIONS_H

#include "byte_array.h"

namespace table {

struct Options { 
   
    // If true, the table will be created if it is missing.
    // Default: false
    bool create_if_missing = false;

    // If true, an error is raised if the table already exists.
    // Default: false
    bool error_if_exists = false ;

    // If true, the table will dump all entries when it is closed.
    // Default: true
    bool dump_when_close = true ;

    // Time-To-Live of a read operation.
    // Default: 2000
    int read_ttl_msec = 2000;

    // Maximum size of a single file.
    // Default: 1073741824(1GB)
    size_t max_file_size = 512 * 1024 * 1024 ;

    // Comparator used to define the order of keys in the table.
    // Default: a comparator that uses lexicographic byte-wise ordering
    int compare(const ByteArray &A , const ByteArray &B) const { 
        size_t size = std::min(A.size() , B.size()) ; 
        int cmp = memcmp(A.data() , B.data() , size) ; 
        if(cmp != 0) return cmp ; 
        return static_cast<int>(A.size()) - static_cast<int>(B.size());
    } 

} ;  

}// namespace leveldb

#endif