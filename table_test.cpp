
#include <assert.h>
#include <thread> 
#include "table.h" 

using namespace table ; 
using namespace std ;

static inline void my_assert(bool status , table::Status s) {
    if(status == true) return ; 
    cout<<s.string()<<endl ; 
    assert(true == false) ;
}

static const string DEFAULT_NAME = "table_TEST.txt" ; 
static const string RANDOM_NAME = "table_RANDOM_NAME.txt" ; 
static string random_string(size_t length) {
    const char* charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz" ; 
    string str(length , 0) ; 
    std::mt19937 mt_rand{std::random_device{}()};
    for(int i = 0 ; i < length ; ++i){
        str[i] = charset[mt_rand() % length] ; 
    }
    return str ; 
}

void OPEN_AND_CLOSE() {

    // file is not exist 
    {    
        Options options ; 
        options.create_if_missing = false ;
        Table table(options , random_string(16)) ; 
        Status s = table.open() ; 
        // cout<<s.string()<<endl ; 
        my_assert(s.good() == false, s) ; 
    }
    
    {
        Options options ; 
        options.create_if_missing = true ; 
        options.dump_when_close = false ; 

        Table table(options , DEFAULT_NAME) ; 

        Status s = table.open() ; 
        my_assert(s.good() == true, s) ; 

        s = table.close() ; 
        my_assert(s.good() == true, s) ; 
    }
}

void LOAD_AND_DUMP(){
    Options options ; 
    options.create_if_missing = true ; 
    options.dump_when_close = true ; 
    options.max_file_size = 4096 ; 

    vector<string> keys(1000) , values(1000);
    for(int i = 0 ; i < 1000 ; ++i){
        keys[i] = random_string(16) ; 
        values[i] = random_string(16) ; 
    }

    string table_name = DEFAULT_NAME ; 
    // put keys 
    {
        Table table(options , table_name) ; 
        Status s = table.open() ; 
        my_assert(s.good() == true, s) ; 
        for(size_t i = 0 ; i < keys.size() ; ++i) {
             
            s = table.put(keys[i] , values[i]) ; 
            my_assert(s.good(), s) ; 
        }
        s = table.close() ; 
        my_assert(s.good() == true, s) ; 
    }
    cout<<"put over"<<endl ;
    // get exist keys and judge whether equal
    {
        Table table(options , table_name) ; 
        Status s = table.open() ; 
        my_assert(s.good() == true, s) ; 
        for(size_t i = 0 ; i < keys.size() ; ++i){
            string value ; 
            s = table.get(keys[i] , &value) ; 
            my_assert(s.good(), s) ; 
            my_assert(values[i] == value, s) ; 
        }
        s = table.close() ; 
        my_assert(s.good() == true, s) ; 
    }
    cout<<"get over"<<endl ;
    // delete 
    {
        Table table(options , table_name) ; 
        Status s = table.open() ; 
        my_assert(s.good() == true, s) ; 
        for(size_t i = 0 ; i < keys.size() ; ++i) {
            s = table.del(keys[i]) ; 
            my_assert(s.good() == true, s) ; 
        }
        s = table.close() ; 
        my_assert(s.good() == true, s) ; 
    }
    cout<<"delete over"<<endl ;

    // find not exist keys ; 
    {
        Table table(options , table_name) ; 
        Status s = table.open() ; 
        my_assert(s.good() == true, s) ; 
        for(size_t i = 0 ; i < keys.size() ; ++i){
            string value ;
            s = table.get(keys[i] , &value) ; 
            my_assert(s.good() == false, s) ; 
        }
    }
    cout<<"test successful"<<endl ;
}

void TABLE_CRUD(){
    Options options ; 
    options.create_if_missing = true ; 
    options.dump_when_close = false ; 
    Table table(options , DEFAULT_NAME) ; 
    Status s = table.open() ; 
    my_assert(s.good() == true, s) ; 

    // Create 
    s = table.put("key" , "value") ; 
    my_assert(s.good() == true, s) ; 

    // Read 
    string value ; 
    table.get("key" , &value) ; 
    my_assert(s.good() == true, s) ; 
    // cout<<s.string()<<" "<<value<<endl ;
    my_assert(value == "value", s) ; 

    s = table.get("key" , nullptr) ; 
    my_assert(s.good() == true, s) ; 

    // update 
    s = table.put("key" , "new-value") ; 
    my_assert(s.good() == true, s) ; 

    s = table.get("key" , &value) ; 
    my_assert(s.good() == true, s) ; 
    my_assert(value == "new-value", s) ; 

    // detele 

    s = table.del("key") ; 
    my_assert(s.good() == true, s) ; 

    s = table.get("key" , nullptr) ; 
    my_assert(s.good() == false, s) ; 
}

void INVALID_OPERATION(){
    // double open / close
    {
        Options options ; 
        options.create_if_missing = true ; 
        options.dump_when_close = false ; 
        Table table(options , DEFAULT_NAME) ; 
        Status s = table.open() ; 
        my_assert(s.good() == true , s) ; 
        s = table.open() ; 
        my_assert(s.good() == false, s) ; 
        s = table.close() ; 
        my_assert(s.good() == true, s) ; 
        s = table.close() ; 
        my_assert(s.good() == false, s) ; 
    }

    // get/put/del/dump before open 
    {
        Options options ; 
        options.create_if_missing = true ; 
        Table table(options , DEFAULT_NAME) ; 
        Status s = table.get("key" , nullptr) ; 
        my_assert(s.good() == false, s) ; 
        s = table.put("key" , "value") ; 
        my_assert(s.good() == false, s) ; 
        s = table.del("key") ; 
        my_assert(s.good() == false, s) ; 
        s = table.dump() ; 
        my_assert(s.good() == false, s) ; 
    }

    // size of entry is too large 
    {
        Options options ; 
        options.max_file_size = 1 ; 
        options.dump_when_close = false ; 
        options.create_if_missing = true ; 
        Table table(options , RANDOM_NAME) ; 

        Status s = table.open() ;
        my_assert(s.good() == true, s) ; 
        s = table.put("key" , "value") ; 
        my_assert(s.good() == false, s) ; 
    }

    // file is not exists and 
    {
        Options options ; 
        options.error_if_exists = true ;
        Table table(options , DEFAULT_NAME) ; 
        Status s = table.open() ; 
        my_assert(s.good() == false, s) ; 
    }
}

int main() {
    // // check whether can open file or open not exist file 
    // OPEN_AND_CLOSE() ; 
    
    // // check table put-insert read update delete 
    // TABLE_CRUD() ; 

    // // check some invalid operations 
    // INVALID_OPERATION() ;

    // check dump table and load table 
    LOAD_AND_DUMP() ; 

    // Options options ; 
    // options.create_if_missing = true ; 
    // options.dump_when_close = true ; 
    // options.max_file_size = 4096 ; 

    // vector<string> keys(10) , values(10);
    // for(int i = 0 ; i < 10 ; ++i){
    //     keys[i] = random_string(16) ; 
    //     values[i] = random_string(16) ; 
    // }
    
    // string table_name = DEFAULT_NAME ; 
    // // put keys 
    // {
    //     Table table(options , table_name) ; 
    //     Status s = table.open() ; 
    //     my_assert(s.good() == true, s) ; 
    //     for(size_t i = 0 ; i < keys.size() ; ++i) {
             
    //         s = table.put(keys[i] , values[i]) ; 
    //         my_assert(s.good(), s) ; 
    //     }

    //     for(size_t i = 0 ; i < keys.size() ; ++i) {
             
    //         s = table.del(keys[i]) ; 
    //         my_assert(s.good() == true, s) ; 
    //     } 
    //     s = table.close() ; 
    //     my_assert(s.good() == true, s) ;         
    // }
    // {
    //     Table table(options , table_name) ; 
    //     Status s = table.open() ; 
    //     my_assert(s.good() == true, s) ; 
    //     for(size_t i = 0 ; i < keys.size() ; ++i){
    //         string value ;
    //         s = table.get(keys[i] , &value) ; 
    //         cout<<keys[i]<<" "<<value<<endl ;
    //         my_assert(s.good() == false, s) ; 
    //     }
    // }
    return 0 ; 
}