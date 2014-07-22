/*****************************************************************************/
/*                                    Xdmf                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfBinaryController.cpp                                              */
/*                                                                           */
/*  Author:                                                                  */
/*     Kenneth Leiter                                                        */
/*     kenneth.leiter@arl.army.mil                                           */
/*     US Army Research Laboratory                                           */
/*     Aberdeen Proving Ground, MD                                           */
/*                                                                           */
/*     Copyright @ 2011 US Army Research Laboratory                          */
/*     All Rights Reserved                                                   */
/*     See Copyright.txt for details                                         */
/*                                                                           */
/*     This software is distributed WITHOUT ANY WARRANTY; without            */
/*     even the implied warranty of MERCHANTABILITY or FITNESS               */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice             */
/*     for more information.                                                 */
/*                                                                           */
/*****************************************************************************/

#include <fstream>
#include <sstream>
#include "XdmfArray.hpp"
#include "XdmfArrayType.hpp"
#include "XdmfBinaryController.hpp"
#include "XdmfError.hpp"

namespace {

  template<size_t T>
  struct ByteSwaper {
    static inline void swap(void * p){}
    static inline void swap(void * p,
                            unsigned int length)
    {
      char * data = static_cast<char *>(p);
      for(unsigned int i=0; i<length; ++i, data+=T){
        ByteSwaper<T>::swap(data);
      }
    }
  };

  template<>
  void ByteSwaper<2>::swap(void * p){
    char one_byte;
    char* data = static_cast<char*>(p);
    one_byte = data[0]; data[0] = data[1]; data[1] = one_byte;
  };

  template<>
  void ByteSwaper<4>::swap(void * p){
    char one_byte;
    char* data = static_cast<char*>(p);
    one_byte = data[0]; data[0] = data[3]; data[3] = one_byte;
    one_byte = data[1]; data[1] = data[2]; data[2] = one_byte;
  
  };
  template<>
  void ByteSwaper<8>::swap(void * p){
    char one_byte;
    char* data = static_cast<char*>(p);
    one_byte = data[0]; data[0] = data[7]; data[7] = one_byte;
    one_byte = data[1]; data[1] = data[6]; data[6] = one_byte;
    one_byte = data[2]; data[2] = data[5]; data[5] = one_byte;
    one_byte = data[3]; data[3] = data[4]; data[4] = one_byte;
  };

}

shared_ptr<XdmfBinaryController>
XdmfBinaryController::New(const std::string & filePath,
                          const shared_ptr<const XdmfArrayType> & type,
                          const Endian & endian,
                          const unsigned int seek,
                          const std::vector<unsigned int> & dimensions)
{
  shared_ptr<XdmfBinaryController> p(new XdmfBinaryController(filePath,
                                                              type,
                                                              endian,
                                                              seek,
                                                              dimensions));
  return p;
}

XdmfBinaryController::XdmfBinaryController(const std::string & filePath,
                                           const shared_ptr<const XdmfArrayType> & type,
                                           const Endian & endian,
                                           const unsigned int seek,
                                           const std::vector<unsigned int> & dimensions) :
  XdmfHeavyDataController(filePath,
                          "",
                          type,
                          dimensions),
  mEndian(endian),
  mSeek(seek)
{
}

XdmfBinaryController::~XdmfBinaryController()
{
}

XdmfBinaryController::Endian
XdmfBinaryController::getEndian() const
{
  return mEndian;
}

std::string
XdmfBinaryController::getName() const
{
  return "Binary";
}

void
XdmfBinaryController::getProperties(std::map<std::string, std::string> & collectedProperties) const
{
  collectedProperties["Format"] = this->getName();
  std::stringstream seekStream;
  seekStream << mSeek;
  collectedProperties["Seek"] = seekStream.str();
  if(mEndian == BIG) {
    collectedProperties["Endian"] = "Big";
  }
  else if(mEndian == LITTLE) {
    collectedProperties["Endian"] = "Little";
  }
}

unsigned int
XdmfBinaryController::getSeek() const
{
  return mSeek;
}

void
XdmfBinaryController::read(XdmfArray * const array)
{
  array->initialize(mType, mDimensions);

  std::ifstream fileStream(mFilePath.c_str(),
                           std::ifstream::binary);

  if(!fileStream.good()) {
    XdmfError::message(XdmfError::FATAL,
                       "Error reading " + mFilePath + 
                       " in XdmfBinaryController::read");
  }

  fileStream.seekg(mSeek);
  
  if(!fileStream.good()) {
    XdmfError::message(XdmfError::FATAL,
                       "Error seeking " + mFilePath + 
                       " in XdmfBinaryController::read");
  }
  
  fileStream.read(static_cast<char *>(array->getValuesInternal()),
                  array->getSize() * mType->getElementSize());
  
#if defined(XDMF_BIG_ENDIAN)
  const bool needByteSwap = mEndian == LITTLE;
#else
  const bool needByteSwap = mEndian == BIG;
#endif // XDMF_BIG_ENDIAN
  
  if(needByteSwap) {
    switch(mType->getElementSize()){
    case 1:
      break;
    case 2:
      ByteSwaper<2>::swap(array->getValuesInternal(),
                          array->getSize());
        break;
    case 4:
      ByteSwaper<4>::swap(array->getValuesInternal(),
                          array->getSize());
      break;
    case 8:
      ByteSwaper<8>::swap(array->getValuesInternal(),
                          array->getSize());
      break;
    default:
      XdmfError::message(XdmfError::FATAL,
                         "Cannot perform endianness swap for datatype");
      break;
    }
  }

}
