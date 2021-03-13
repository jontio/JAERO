#include "RuntimeError.h"
#include <iostream>
#include <sstream>

RuntimeError::RuntimeError(const std::string &arg, const char *file, int line,int parameter) :
    std::runtime_error(arg)
{
        std::ostringstream o;
        o << file << ":" << line << ": " << arg<<" ("<<parameter<<")";
        msg = o.str();
}

RuntimeError::~RuntimeError() throw() 
{
    
}

const char* RuntimeError::what() const throw()
{
        return msg.c_str();
}
