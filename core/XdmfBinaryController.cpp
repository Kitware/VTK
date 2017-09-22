/*****************************************************************************/
/*                                    Xdmf                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfBinaryController.cpp                                            */
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
  }

  template<>
  void ByteSwaper<4>::swap(void * p){
    char one_byte;
    char* data = static_cast<char*>(p);
    one_byte = data[0]; data[0] = data[3]; data[3] = one_byte;
    one_byte = data[1]; data[1] = data[2]; data[2] = one_byte;
  
  }

  template<>
  void ByteSwaper<8>::swap(void * p){
    char one_byte;
    char* data = static_cast<char*>(p);
    one_byte = data[0]; data[0] = data[7]; data[7] = one_byte;
    one_byte = data[1]; data[1] = data[6]; data[6] = one_byte;
    one_byte = data[2]; data[2] = data[5]; data[5] = one_byte;
    one_byte = data[3]; data[3] = data[4]; data[4] = one_byte;
  }

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
                                                              std::vector<unsigned int>(dimensions.size(), 0),
                                                              std::vector<unsigned int>(dimensions.size(), 1),
                                                              dimensions,
                                                              dimensions));
  return p;
}

shared_ptr<XdmfBinaryController>
XdmfBinaryController::New(const std::string & filePath,
                          const shared_ptr<const XdmfArrayType> & type,
                          const Endian & endian,
                          const unsigned int seek,
                          const std::vector<unsigned int> & starts,
                          const std::vector<unsigned int> & strides,
                          const std::vector<unsigned int> & dimensions,
                          const std::vector<unsigned int> & dataspaces)
{
  shared_ptr<XdmfBinaryController> p(new XdmfBinaryController(filePath,
                                                              type,
                                                              endian,
                                                              seek,
                                                              starts,
                                                              strides,
                                                              dimensions,
                                                              dataspaces));
  return p;
}

XdmfBinaryController::XdmfBinaryController(const std::string & filePath,
                                           const shared_ptr<const XdmfArrayType> & type,
                                           const Endian & endian,
                                           const unsigned int seek,
                                           const std::vector<unsigned int> & starts,
                                           const std::vector<unsigned int> & strides,
                                           const std::vector<unsigned int> & dimensions,
                                           const std::vector<unsigned int> & dataspaces) :
  XdmfHeavyDataController(filePath,
                          type,
                          starts,
                          strides,
                          dimensions,
                          dataspaces),
  mEndian(endian),
  mSeek(seek)
{
}

XdmfBinaryController::~XdmfBinaryController()
{
}

std::string
XdmfBinaryController::getDataspaceDescription() const
{
  std::stringstream descstream;
  descstream << mSeek << ":" << XdmfHeavyDataController::getDataspaceDescription();
  return descstream.str();
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

  shared_ptr<XdmfArray> dataspaceArray = XdmfArray::New();

  dataspaceArray->initialize(mType, mDataspaceDimensions);

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
  
  fileStream.read(static_cast<char *>(dataspaceArray->getValuesInternal()),
                  dataspaceArray->getSize() * mType->getElementSize());
  
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
      ByteSwaper<2>::swap(dataspaceArray->getValuesInternal(),
                          dataspaceArray->getSize());
        break;
    case 4:
      ByteSwaper<4>::swap(dataspaceArray->getValuesInternal(),
                          dataspaceArray->getSize());
      break;
    case 8:
      ByteSwaper<8>::swap(dataspaceArray->getValuesInternal(),
                          dataspaceArray->getSize());
      break;
    default:
      XdmfError::message(XdmfError::FATAL,
                         "Cannot perform endianness swap for datatype");
      break;
    }
  }
  array->insert(std::vector<unsigned int>(mDimensions.size(), 0),
                dataspaceArray,
                mStart,
                mDataspaceDimensions,
                mDimensions,
                std::vector<unsigned int>(mDimensions.size(), 1),
                mStride);
}

// C Wrappers

XDMFBINARYCONTROLLER *
XdmfBinaryControllerNew(char * filePath,
                        int type,
                        int endian,
                        unsigned int seek,
                        unsigned int * dimensions,
                        unsigned int numDims,
                        int * status)
{
  XDMF_ERROR_WRAP_START(status)
  std::vector<unsigned int> dimVector(dimensions, dimensions + numDims);
  shared_ptr<const XdmfArrayType> buildType = shared_ptr<XdmfArrayType>();
  switch (type) {
    case XDMF_ARRAY_TYPE_UINT8:
      buildType = XdmfArrayType::UInt8();
      break;
    case XDMF_ARRAY_TYPE_UINT16:
      buildType = XdmfArrayType::UInt16();
      break;
    case XDMF_ARRAY_TYPE_UINT32:
      buildType = XdmfArrayType::UInt32();
      break;
    case XDMF_ARRAY_TYPE_INT8:
      buildType = XdmfArrayType::Int8();
      break;
    case XDMF_ARRAY_TYPE_INT16:
      buildType = XdmfArrayType::Int16();
      break;
    case XDMF_ARRAY_TYPE_INT32:
      buildType = XdmfArrayType::Int32();
      break;
    case XDMF_ARRAY_TYPE_INT64:
      buildType = XdmfArrayType::Int64();
      break;
    case XDMF_ARRAY_TYPE_FLOAT32:
      buildType = XdmfArrayType::Float32();
      break;
    case XDMF_ARRAY_TYPE_FLOAT64:
      buildType = XdmfArrayType::Float64();
      break;
    default:
      XdmfError::message(XdmfError::FATAL,
                         "Error: Invalid ArrayType.");
      break;
  }
  XdmfBinaryController::Endian buildEndian = XdmfBinaryController::NATIVE;
  printf("switch endian = %u\n", endian);
  switch (endian) {
    case XDMF_BINARY_CONTROLLER_ENDIAN_BIG:
      buildEndian = XdmfBinaryController::BIG;
      break;
    case XDMF_BINARY_CONTROLLER_ENDIAN_LITTLE:
      buildEndian = XdmfBinaryController::LITTLE;
      break;
    case XDMF_BINARY_CONTROLLER_ENDIAN_NATIVE:
      buildEndian = XdmfBinaryController::NATIVE;
      break;
    default:
      XdmfError::message(XdmfError::FATAL,
                         "Error: Invalid Endian.");
      break;
  }
  shared_ptr<XdmfBinaryController> * generatedController = 
    new shared_ptr<XdmfBinaryController>(XdmfBinaryController::New(filePath, 
								   buildType, 
								   buildEndian, 
								   seek, 
								   dimVector));
  return (XDMFBINARYCONTROLLER *) generatedController;
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

XDMFBINARYCONTROLLER *
XdmfBinaryControllerNewHyperslab(char * filePath,
                                 int type,
                                 int endian,
                                 unsigned int seek,
                                 unsigned int * start,
                                 unsigned int * stride,
                                 unsigned int * dimensions,
                                 unsigned int * dataspaceDimensions,
                                 unsigned int numDims,
                                 int * status)
{
  XDMF_ERROR_WRAP_START(status)
  std::vector<unsigned int> startVector(start, start + numDims);
  std::vector<unsigned int> strideVector(stride, stride + numDims);
  std::vector<unsigned int> dimVector(dimensions, dimensions + numDims);
  std::vector<unsigned int> dataspaceVector(dataspaceDimensions, dataspaceDimensions + numDims);
  shared_ptr<const XdmfArrayType> buildType = shared_ptr<XdmfArrayType>();
  switch (type) {
    case XDMF_ARRAY_TYPE_UINT8:
      buildType = XdmfArrayType::UInt8();
      break;
    case XDMF_ARRAY_TYPE_UINT16:
      buildType = XdmfArrayType::UInt16();
      break;
    case XDMF_ARRAY_TYPE_UINT32:
      buildType = XdmfArrayType::UInt32();
      break;
    case XDMF_ARRAY_TYPE_INT8:
      buildType = XdmfArrayType::Int8();
      break;
    case XDMF_ARRAY_TYPE_INT16:
      buildType = XdmfArrayType::Int16();
      break;
    case XDMF_ARRAY_TYPE_INT32:
      buildType = XdmfArrayType::Int32();
      break;
    case XDMF_ARRAY_TYPE_INT64:
      buildType = XdmfArrayType::Int64();
      break;
    case XDMF_ARRAY_TYPE_FLOAT32:
      buildType = XdmfArrayType::Float32();
      break;
    case XDMF_ARRAY_TYPE_FLOAT64:
      buildType = XdmfArrayType::Float64();
      break;
    default:
      XdmfError::message(XdmfError::FATAL,
                         "Error: Invalid ArrayType.");
      break;
  }
  XdmfBinaryController::Endian buildEndian = XdmfBinaryController::NATIVE;
  switch (endian) {
    case XDMF_BINARY_CONTROLLER_ENDIAN_BIG:
      buildEndian = XdmfBinaryController::BIG;
      break;
    case XDMF_BINARY_CONTROLLER_ENDIAN_LITTLE:
      buildEndian = XdmfBinaryController::LITTLE;
      break;
    case XDMF_BINARY_CONTROLLER_ENDIAN_NATIVE:
      buildEndian = XdmfBinaryController::NATIVE;
      break;
    default:
      XdmfError::message(XdmfError::FATAL,
                         "Error: Invalid Endian.");
      break;
  }
  shared_ptr<XdmfBinaryController> * generatedController = 
    new shared_ptr<XdmfBinaryController>(XdmfBinaryController::New(filePath, 
								   buildType, 
								   buildEndian, 
								   seek, 
								   startVector, 
								   strideVector, 
								   dimVector, 
								   dataspaceVector));
  return (XDMFBINARYCONTROLLER *) generatedController;
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

int
XdmfBinaryControllerGetEndian(XDMFBINARYCONTROLLER * controller)
{
  shared_ptr<XdmfBinaryController> & refController = *(shared_ptr<XdmfBinaryController> *)(controller);
  return refController->getEndian();
}

unsigned int
XdmfBinaryControllerGetSeek(XDMFBINARYCONTROLLER * controller)
{
  shared_ptr<XdmfBinaryController> & refController = *(shared_ptr<XdmfBinaryController> *)(controller);
  return refController->getSeek();
}

// C Wrappers for parent classes are generated by macros
XDMF_HEAVYCONTROLLER_C_CHILD_WRAPPER(XdmfBinaryController, XDMFBINARYCONTROLLER)
