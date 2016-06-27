#include <XdmfError.hpp>
#include <cstdlib>
#include <iostream>

XdmfError::XdmfError(Level level, std::string message) :
  mLevel(level),
  mMessage(message)
{
}

XdmfError::~XdmfError() throw()
{
}

/************************
 *** Public Functions ***
 ************************/

XdmfError::Level
XdmfError::getLevel() const
{
  return XdmfError::mLevel;
}

void
XdmfError::setLevel(Level l)
{
  XdmfError::mLevel = l;
}

const char *
XdmfError::what() const throw()
{
  return XdmfError::mMessage.c_str();
}

/*******************************
 *** Public Static Functions ***
 *******************************/

bool
XdmfError::getCErrorsAreFatal()
{
  return XdmfError::mCErrorsAreFatal;
}

XdmfError::Level
XdmfError::getLevelLimit()
{
  return XdmfError::mLevelLimit;
}

XdmfError::Level
XdmfError::getSuppressionLevel()
{
  return XdmfError::mSuppressLevel;
}

void
XdmfError::setCErrorsAreFatal(bool status)
{
  XdmfError::mCErrorsAreFatal = status;
}

void
XdmfError::setLevelLimit(Level l)
{
  XdmfError::mLevelLimit = l;
}

void
XdmfError::setSuppressionLevel(Level l)
{
  XdmfError::mSuppressLevel = l;
}

void
XdmfError::message(Level level, std::string msg)
{
  if (level<=XdmfError::getSuppressionLevel())
  {
    XdmfError::WriteToStream(msg);
  }
  if(level<=XdmfError::getLevelLimit()) {
    throw XdmfError(level, msg);
  }
}

void
XdmfError::setBuffer(std::streambuf* buf)
{
  XdmfError::mBuf = buf;
}

/********************************
 *** Private Static Functions ***
 ********************************/

// automatically writes the message to the provided buffer
// by default this is basically a print statement
void
XdmfError::WriteToStream(std::string msg)
{
  if(msg[msg.length()-1] != '\n') {
    // using \r\n for windows compatiblity
    msg+="\r\n";
  }
  XdmfError::mBuf->sputn(msg.c_str(),msg.length());
}

/******************************************
 *** Initialize Static Member Variables ***
 ******************************************/

XdmfError::Level XdmfError::mLevelLimit = XdmfError::FATAL;
XdmfError::Level XdmfError::mSuppressLevel = XdmfError::WARNING;
std::streambuf* XdmfError::mBuf=std::cout.rdbuf();
bool XdmfError::mCErrorsAreFatal = false;

// C Wrappers

void XdmfErrorSetCErrorsAreFatal(int status)
{
  XdmfError::setCErrorsAreFatal(status);
}

void XdmfErrorSetLevelLimit(int level, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  switch (level) {
    case XDMF_ERROR_FATAL:
      XdmfError::setLevelLimit(XdmfError::FATAL);
      break;
    case XDMF_ERROR_WARNING:
      XdmfError::setLevelLimit(XdmfError::WARNING);
      break;
    case XDMF_ERROR_DEBUG:
      XdmfError::setLevelLimit(XdmfError::DEBUG);
      break;
    default:
      try {
        XdmfError::message(XdmfError::FATAL, "Error: Invalid Error Level");
      }
      catch (XdmfError & e) {
        throw e;
      }
  }
  XDMF_ERROR_WRAP_END(status)
}

void XdmfErrorSetSuppressionLevel(int level, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  switch (level) {
    case XDMF_ERROR_FATAL:
      XdmfError::setSuppressionLevel(XdmfError::FATAL);
      break;
    case XDMF_ERROR_WARNING:
      XdmfError::setSuppressionLevel(XdmfError::WARNING);
      break;
    case XDMF_ERROR_DEBUG:
      XdmfError::setSuppressionLevel(XdmfError::DEBUG);
      break;
    default:
      XdmfError::message(XdmfError::FATAL, "Error: Invalid Error Level");
  }
  XDMF_ERROR_WRAP_END(status)
}

int XdmfErrorGetCErrorsAreFatal()
{
  return XdmfError::getCErrorsAreFatal();
}

int XdmfErrorGetLevelLimit()
{
  if (XdmfError::getLevelLimit() == XdmfError::FATAL) {
    return XDMF_ERROR_FATAL;
  }
  else if (XdmfError::getLevelLimit() == XdmfError::WARNING) {
    return XDMF_ERROR_WARNING;
  }
  else if (XdmfError::getLevelLimit() == XdmfError::DEBUG) {
    return XDMF_ERROR_DEBUG;
  }
  else {
    XdmfError::message(XdmfError::FATAL, "Error: Invalid Error Level");
  }
  return -1;
}

int XdmfErrorGetSuppressionLevel()
{
  if (XdmfError::getSuppressionLevel() == XdmfError::FATAL) {
    return XDMF_ERROR_FATAL;
  }
  else if (XdmfError::getSuppressionLevel() == XdmfError::WARNING) {
    return XDMF_ERROR_WARNING;
  }
  else if (XdmfError::getSuppressionLevel() == XdmfError::DEBUG) {
    return XDMF_ERROR_DEBUG;
  }
  else {
    XdmfError::message(XdmfError::FATAL, "Error: Invalid Error Level");
  }
  return -1;
}
