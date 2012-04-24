#if !defined EXCEPTION_H
#define      EXCEPTION_H

#include <exception>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include <string>

class Exception : public std::exception {
public:
    Exception() {}
    Exception(const std::string & msg) : message(msg) {}

    ~Exception() throw() {}

    const char * what() const throw() { return message.c_str(); }
    void setMessage(const std::string & msg) { message = msg; }

private:
    std::string message;
};

class SystemException : public Exception {
public:
    SystemException(int error) {
        setMessage(strerror(error));
    }

    SystemException(int error, const std::string & msg) {
        setMessage(msg + ": " + strerror(error));
    }


    static void check(int rc, const char * msg) {
        if (rc < 0) {
            throw SystemException(errno, msg);
        }
    }

    static void check(int rc, const std::string & msg) {
        check(rc, msg.c_str());
    }
};

class AssertionException : public Exception {
public:
    AssertionException(const char * test, const char * file, int lineno) {
        std::string msg("Assertion (");
        if (test != 0) msg += test;
        msg += ") failed";
        if (file != 0) {
            msg += " in " + std::string(file);
            msg += " at line ";
            char buffer[20];
            snprintf(buffer, sizeof(buffer), "%d", lineno);
            msg += buffer;
        }
        setMessage(msg);
    }
};

#define ASSERTION(p)  do { if (!(p)) throw AssertionException( #p, __FILE__, __LINE__ ); } while (false)
#define PRECONDITION(p) ASSERTION(p)
#define POSTCONDITION(p) ASSERTION(p)

#endif
