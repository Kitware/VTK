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
