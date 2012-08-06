/*
  To create C++ files:
ICE_INCLUDES="-I${Xdmf_SOURCE_DIR} -I${Xdmf_SOURCE_DIR}/libsrc -I${Xdmf_BINARY_DIR}/libsrc -I${Xdmf_BINARY_DIR}/Ice/libsrc"

XdmfUtilsTcl.cxx:
swig -v -c++ -make_default -includeall -tcl ${ICE_INCLUDES} -o XdmfUtilsTcl.cxx XdmfUtils.i

XdmfUtilsPython.cxx:
swig -v -c++ -make_default -includeall -python ${ICE_INCLUDES} -o XdmfUtilsPython.cxx XdmfUtils.i

XdmfUtilsJava.cxx:
swig -v -c++ -make_default -includeall -java ${ICE_INCLUDES} -o XdmfUtilsJava.cxx XdmfUtils.i
*/

%module XdmfUtils
%{

#include <XdmfDiff.h>
#include <XdmfExodusReader.h>
#include <XdmfExodusWriter.h>
#include <XdmfPartitioner.h>
#include <Xdmf.h>

#ifndef HAVE_STRTOLL
# define strtoll XDMF_strtoll
inline XDMF_LONG64 XDMF_strtoll(char *str, void*, int)
{
  XDMF_LONG64 result = 0;
  int negative=0;

  while (*str == ' ' || *str == '\t')
    {
    str++;
    }
  if (*str == '+')
    {
    str++;
    }
  else if (*str == '-')
    {
    negative = 1;
    str++;
    }

  while (*str >= '0' && *str <= '9')
    {
    result = (result*10)-(*str++ - '0');
    }
  return negative ? result : -result;
}
#else
# define XDMF_strtoll strtoll
#endif

%}

%import Xdmf.i // import typedefs etc. from xdmf lib --- this should keep us from regenerating the same code
%include std_string.i
%include XdmfDiff.h
%include XdmfExodusReader.h
%include XdmfExodusWriter.h
%include XdmfPartitioner.h

#ifdef SWIGPYTHON
%{
void XdmfSwigException(int code, const char* msg)
{
/*   SWIG_exception(code, msg); */
}
%}
#endif
