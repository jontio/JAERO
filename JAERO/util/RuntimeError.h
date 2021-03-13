#ifndef RUNTIMEERROR_H
#define RUNTIMEERROR_H

#include <stdexcept>
#include <string>

class RuntimeError : public std::runtime_error
{
public:
    RuntimeError(const std::string &arg, const char *file, int line,int parameter);
    ~RuntimeError() throw();
    const char *what() const throw();
private:
    std::string msg;
};
#define RUNTIME_ERROR(description, parameter) throw RuntimeError(description, __FILE__, __LINE__,parameter);

#endif
