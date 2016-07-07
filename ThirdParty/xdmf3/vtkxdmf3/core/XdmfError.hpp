#ifndef XDMFERROR_HPP_
#define XDMFERROR_HPP_

// C Compatible Includes
#include "XdmfCore.hpp"

#ifdef __cplusplus

// Includes
#include <iosfwd>
#include <sstream>
#include <exception>

class XDMFCORE_EXPORT XdmfError  : public std::exception
{
public:
    enum Level {FATAL, WARNING, DEBUG};

    /**
     * One of the constructors for XdmfError, this one doesn't print to the buffer.
     *
     * Example of use:
     *
     * C++
     *
     * @dontinclude ExampleXdmfError.cpp
     * @skipline //#initialization
     * @until //#initialization
     *
     * Python:
     *
     * @dontinclude XdmfExampleError.py
     * @skipline #//initialization
     * @until #//initialization
     *
     * @param   level           The error level of the exception being constructed
     * @param   message         The message to be attached to the exception
     */
    XdmfError(Level level, std::string message);

    ~XdmfError() throw();

    /**
     * Sets the error level of the exception.
     *
     * Example of use:
     *
     * C++
     *
     * @dontinclude ExampleXdmfError.cpp
     * @skipline //#initialization
     * @until //#initialization
     * @skipline //#setLevel
     * @until //#setLevel
     *
     * Python:
     *
     * @dontinclude XdmfExampleError.py
     * @skipline #//setLevel
     * @until #//setLevel
     *
     * @param   l       The new level of the exception
     */
    void setLevel(Level l);

    /**
     * Gets the error level of the exception.
     *
     * Example of use:
     *
     * C++
     *
     * @dontinclude ExampleXdmfError.cpp
     * @skipline //#initialization
     * @until //#initialization
     * @skipline //#getLevel
     * @until //#getLevel
     *
     * Python:
     *
     * @dontinclude XdmfExampleError.py
     * @skipline #//getLevel
     * @until #//getLevel
     *
     * @return  The error level of the exception
     */
    Level getLevel() const;

    /**
     * Gets whether C errors are Fatal. If set to false a status will be returned
     * to C when an error is thrown. Otherwise there will be no handling and the
     * program will exit since C cannot handle exceptions.
     *
     * @param   status  Whether the C wrappers are to pass an integer value to
     *                  C on failure instead of exiting.
     */
    static void setCErrorsAreFatal(bool status);

    /**
     * Sets the level limit for Errors. This determines what level of errors will be thrown with message.
     *
     * Example of use:
     *
     * C++
     *
     * @dontinclude ExampleXdmfError.cpp
     * @skipline //#initialization
     * @until //#initialization
     * @skipline //#setLevelLimit
     * @until //#setLevelLimit
     *
     * Python
     *
     * @dontinclude XdmfExampleError.py
     * @skipline #//setLevelLimit
     * @until #//setLevelLimit
     *
     * @param   l       The cutoff level for sending exceptions via message
     */
    static void setLevelLimit(Level l);

    /**
     * Sets the minimum Error level that displays messages with the message function.
     *
     * Example of use:
     *
     * C++
     *
     * @dontinclude ExampleXdmfError.cpp
     * @skipline //#setSuppressionLevel
     * @until //#setSuppressionLevel
     *
     * Python
     *
     * @dontinclude XdmfExampleError.py
     * @skipline #//setSuppressionLevel
     * @until #//setSuppressionLevel
     *
     * @param   l       The new minimum error level to display a message
     */
    static void setSuppressionLevel(Level l);

    /**
     * Gets whether C errors are Fatal. If set to false a status will be returned
     * to C when an error is thrown. Otherwise there will be no handling and the
     * program will exit since C cannot handle exceptions.
     *
     * @return  Whether the C wrappers with pass an integer value to C
     *          instead of stopping the program
     */
    static bool getCErrorsAreFatal();

    /**
     * Gets the level limit for Errors.
     *
     * Example of use:
     *
     * C++
     *
     * @dontinclude ExampleXdmfError.cpp
     * @skipline //#initialization
     * @until //#initialization
     * @skipline //#getLevelLimit
     * @until //#getLevelLimit
     *
     * Python
     *
     * @dontinclude XdmfExampleError.py
     * @skipline #//getLevelLimit
     * @until #//getLevelLimit
     *
     * @return  The cuttof level for sending exceptions via message
     */
    static Level getLevelLimit();

    /**
     * Gets the minimum Error level that displays messages with the message function.
     *
     * @dontinclude ExampleXdmfError.cpp
     * @skipline //#getSuppressionLevel
     * @until //#getSuppressionLevel
     *
     * Python
     *
     * @dontinclude XdmfExampleError.py
     * @skipline #//getSuppressionLevel
     * @until #//getSuppressionLevel
     *
     * @return  The minimum error level to display a message
     */
    static Level getSuppressionLevel();

    /**
     * Alternate constructor for XdmfError exceptions.
     * This one automatically prints out the message provided if the error level
     * above the suppression level. If the error level is not above the
     * level limit an exception will not be thrown.
     *
     * Example of use:
     *
     * C++
     *
     * @dontinclude ExampleXdmfError.cpp
     * @skipline //#initialization
     * @until //#initialization
     * @skipline //#message
     * @until //#message
     *
     * Python: Generates a RuntimeError instead of an XdmfError in Python
     *
     * @dontinclude XdmfExampleError.py
     * @skipline #//message
     * @until #//message
     *
     * @param   l       The level of the error to be generated
     * @param   msg     The message to be associated with the error generated and printed out
     */
    static void message(Level l, std::string msg);

    /**
     * Sets which buffer the error messages are printed to with the message function.
     *
     * Example of use:
     *
     * C++
     *
     * @dontinclude ExampleXdmfError.cpp
     * @skipline //#initialization
     * @until //#initialization
     * @skipline //#setBuffer
     * @until //#setBuffer
     *
     * Python: Not supported in Python
     *
     * @param   buf     The buffer that the error messages will be printed to
     */
    static void setBuffer(std::streambuf* buf);

    /**
     * Gets the message associated with this exception.
     *
     * Example of use:
     *
     * C++
     *
     * @dontinclude ExampleXdmfError.cpp
     * @skipline //#initialization
     * @until //#initialization
     * @skipline //#what
     * @until //#what
     *
     * Python
     *
     * @dontinclude XdmfExampleError.py
     * @skipline #//what
     * @until #//what
     *
     * @return  The message associated with the exception
     */
    virtual const char * what() const throw();

private:
    Level mLevel;
    static Level mLevelLimit;
    static Level mSuppressLevel;
    static std::streambuf* mBuf;
    static bool mCErrorsAreFatal;
    std::string mMessage;

    static void WriteToStream(std::string msg);
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

// C wrappers go here

#define XDMF_SUCCESS  1
#define XDMF_FAIL    -1

#define XDMF_ERROR_FATAL   40
#define XDMF_ERROR_WARNING 41
#define XDMF_ERROR_DEBUG   42

XDMFCORE_EXPORT void XdmfErrorSetCErrorsAreFatal(int status);

XDMFCORE_EXPORT void XdmfErrorSetLevelLimit(int level, int * status);

XDMFCORE_EXPORT void XdmfErrorSetSuppressionLevel(int level, int * status);

XDMFCORE_EXPORT int XdmfErrorGetCErrorsAreFatal();

XDMFCORE_EXPORT int XdmfErrorGetLevelLimit();

XDMFCORE_EXPORT int XdmfErrorGetSuppressionLevel();

#ifdef __cplusplus

//Use these macros to catch Exceptions for C code

#define XDMF_ERROR_WRAP_START(status)    \
if (status) {                            \
  *status = XDMF_SUCCESS;                \
}                                        \
try {

#define XDMF_ERROR_WRAP_END(status)      \
}                                        \
catch (XdmfError & e) {                  \
  if (XdmfError::getCErrorsAreFatal()) { \
    throw e;                             \
  }                                      \
  else {                                 \
    if (status) {                        \
    *status = XDMF_FAIL;                 \
    }                                    \
  }                                      \
}

}
#endif

#endif
