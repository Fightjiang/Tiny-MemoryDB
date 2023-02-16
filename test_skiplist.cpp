#include "skiplist.h"
#include <assert.h>
#include <thread>
using namespace table ; 
using namespace std ; 

void insert_test(SkipList *skList) {
    auto it = skList->insert("f", "f");
    assert(it.good() == true);
    assert(it.key()== "f");
    assert(it.value()== "f");
    
    // insert at the head
    it = skList->insert("a" , "a");
    assert(it.good());
    assert(it.key()== "a");
    assert(it.value()== "a");

    // insert at the tail
    it = skList->insert("z" , "z");
    assert(it.good());
    assert(it.key()== "z");
    assert(it.value()== "z");

    // insert into the middle
    it = skList->insert("b", "b");
    assert(it.good());
    assert(it.key()== "b");
    assert(it.value()== "b");

    // double-insert
    it = skList->insert("b", "b");
    assert(it.good() == false);
}

void update_test(SkipList *skList) {
    auto it = skList->update("a", "a");
    assert(it.good() == false);

    it = skList->insert("a", "a");
    assert(it.good() == false);

    it = skList->update("a", "b");

    assert(it.good() == true);
    assert(it.key()  ==  "a");
    assert(it.value() == "b");
}

void erase_test(SkipList *skList) {
     
    assert(skList->erase("a") == true) ;
    auto iter = skList->insert("a", "a");
    assert(iter.good() == true);
    assert(skList->erase("a")== true);
}


int main(){
    SkipList *skList = new SkipList() ;  
    // insert f a z b 
    insert_test(skList) ;
    cout<<skList->serialize()<<endl ;
    // update "a-a" -> "a-b" 
    update_test(skList) ; 
    cout<<skList->serialize()<<endl ;
    // erase "a"
    erase_test(skList) ;
    // look result 
    cout<<skList->serialize()<<endl ;
    delete skList ; 
    
    return 0 ; 
}
