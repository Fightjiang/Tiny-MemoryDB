## 基于跳表实现的轻量级键值型内存存储引擎

本项目是用 C++ 实现的基于跳表实现的轻量级键值型内存存储引擎。

#### 功能：
* Key 和 value 是任意字节的数组，自己实现的 byte_array 类，改进 C 语言中使用 char* 实现字符串的不足，如：'\0' 需要占一个字节且无法完整表示包含'\0'的数据，获得长度需要遍历字符串等。
* 数据是按 Key 有序的存储在跳表中的。
* 支持 CRUD 基本操作如：put(key , value) , get(key) , del(key) ; 
* 支持数据持久化到磁盘上，但是不支持 `crash-safe 崩溃恢复` 
* valgrind 内存泄漏检测安全


### 示例： 
Opening or Closing a Table 
```C++
table::Options options;
options.create_if_missing = true;
table::Table table(options, "test_table");

table::Status s = table.open();
if(!s.good()) {
    std::cerr << s.string() << std::endl;
}

table.close();
```

Read、Write、Detele
```C++
std::string key = "key";
std::string value = "value";

s = table.put(key, value);
if (!s.good()) {
    std::cerr << s.string() << std::endl;
}

s = table.get(key, &value);
if (!s.good()) {
    std::cerr << s.string() << std::endl;
} else {
    std::cout << value << std::endl;
}

s = table.del(key) ; 
if (!s.good()) {
    std::cerr << s.string() << std::endl;
}
```
Persisting data
```C++
std::string key = "key";
std::string value = "value";

s = table.put(key, value);
if (!s.good()) {
    std::cerr << s.string() << std::endl;
}

s = table.dump();
if (!s.good()) {
    std::cerr << s.string() << std::endl;
}
```







### TODO 优化

- [ ] byte_array 结构体优化，设计不同的结构头，如内部表示字符长度可以是：uint16_t 、uint32_t、uint64_t 等，现在只是 uint8_t 一个，这样的话字符串的长度必须小于 2^8 。
- [ ] 可以加布隆过滤器，加快判断 key 是否在内存里
- [ ] 可以添加 Huffman 编码进行文件压缩，减少磁盘占用。