#ifndef TABLE_STATUS_H
#define TABLE_STATUS_H

#include <string> 

namespace table { 
class Status {
public : 
    enum Code { 
        OK = 0 , 
        IO_ERROR = 1 , 
        NOT_FOUND = 2 , 
        INVALID_OPERATION = 3 ,
    } ; 
    Status () ; 
    explicit Status(Code code) ; 
    Status(Code code , const std::string &msg)  ; 
    ~Status() = default ; 

    bool good() const ; 
    Code code() const  ; 
    std::string string() const ; 

    static Status ok(const std::string &msg) ; 
    static Status io_error(const std::string &msg) ; 
    static Status not_found(const std::string &msg) ;
    static Status invalid_operation(const std::string &msg) ; 
    static Status ok() ; 
    static Status io_error() ; 
    static Status not_found() ; 
    static Status invalid_operation() ; 

private : 

    Code _code ; 
    std::string _msg ; 
} ;

Status::Status() : _code(OK) , _msg("") { }
Status::Status(Code code) : _code(code) { }
Status::Status(Code code , const std::string& msg) : _code(code) , _msg(msg)  { }
bool Status::good() const { return this->_code == OK  ;} 
Status::Code Status::code() const { return this->_code ; } 

std::string Status::string() const {
    std::string str ; 
    switch (_code)
    {
    case OK:
        str = "ok" ; 
        break;
    case IO_ERROR : 
        str = "IO_ERROR" ; 
        break ;
    case NOT_FOUND : 
        str = "NOT_FOUND" ; 
        break ;
    case INVALID_OPERATION : 
        str = "INVALID_OPERATION" ; 
        break ; 
    default:
        break;
    }
    if(!this->_msg.empty()){
        str = str + "-" + _msg ; 
    }
    return str ; 
}

Status Status::ok(const std::string &msg) {return Status(OK , msg) ; } ; 
Status Status::io_error(const std::string &msg) {return Status(IO_ERROR , msg) ; } 
Status Status::not_found(const std::string& msg) { return Status(NOT_FOUND, msg); }
Status Status::invalid_operation(const std::string& msg) { return Status(INVALID_OPERATION, msg); }
Status Status::ok() { return Status(OK); }
Status Status::io_error() { return Status(IO_ERROR); }
Status Status::not_found() { return Status(NOT_FOUND); }
Status Status::invalid_operation() { return Status(INVALID_OPERATION); }

} // namespace table 

#endif