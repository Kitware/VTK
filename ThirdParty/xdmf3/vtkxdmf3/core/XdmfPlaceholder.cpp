/*****************************************************************************/
/*                                    Xdmf                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfPlaceholder.cpp                                                 */
/*                                                                           */
/*  Author:                                                                  */
/*     Andrew Burns                                                          */
/*     andrew.j.burns2@us.army.mil                                           */
/*     US Army Research Laboratory                                           */
/*     Aberdeen Proving Ground, MD                                           */
/*                                                                           */
/*     Copyright @ 2014 US Army Research Laboratory                          */
/*     All Rights Reserved                                                   */
/*     See Copyright.txt for details                                         */
/*                                                                           */
/*     This software is distributed WITHOUT ANY WARRANTY; without            */
/*     even the implied warranty of MERCHANTABILITY or FITNESS               */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice             */
/*     for more information.                                                 */
/*                                                                           */
/*****************************************************************************/

#include <hdf5.h>
#include <sstream>
#include "XdmfArray.hpp"
#include "XdmfArrayType.hpp"
#include "XdmfError.hpp"
#include "XdmfHeavyDataDescription.hpp"
#include "XdmfPlaceholder.hpp"
#include "XdmfSystemUtils.hpp"
#include "string.h"

shared_ptr<XdmfPlaceholder>
XdmfPlaceholder::New(const std::string & filePath,
                     const shared_ptr<const XdmfArrayType> type,
                     const std::vector<unsigned int> & start,
                     const std::vector<unsigned int> & stride,
                     const std::vector<unsigned int> & dimensions,
                     const std::vector<unsigned int> & dataspaceDimensions)
{
  shared_ptr<XdmfPlaceholder> 
    p(new XdmfPlaceholder(filePath,
                          type,
                          start,
                          stride,
                          dimensions,
                          dataspaceDimensions));
  return p;
}

XdmfPlaceholder::XdmfPlaceholder(const std::string & filePath,
                                 const shared_ptr<const XdmfArrayType> type,
                                 const std::vector<unsigned int> & start,
                                 const std::vector<unsigned int> & stride,
                                 const std::vector<unsigned int> & dimensions,
                                 const std::vector<unsigned int> & dataspaceDimensions) :
  XdmfHeavyDataController(filePath,
                          type,
                          start,
                          stride,
                          dimensions,
                          dataspaceDimensions)
{
}

XdmfPlaceholder::XdmfPlaceholder(const XdmfPlaceholder & refController):
  XdmfHeavyDataController(refController)
{
}

XdmfPlaceholder::~XdmfPlaceholder()
{
}

shared_ptr<XdmfHeavyDataController>
XdmfPlaceholder::createSubController(const std::vector<unsigned int> & starts,
                                     const std::vector<unsigned int> & strides,
                                     const std::vector<unsigned int> & dimensions)
{
  return XdmfPlaceholder::New(mFilePath,
                              mType,
                              starts,
                              strides,
                              dimensions,
                              mDataspaceDimensions);
}

std::string
XdmfPlaceholder::getDescriptor() const
{
  return "";
}

shared_ptr<XdmfHeavyDataDescription>
XdmfPlaceholder::getHeavyDataDescription()
{
  static shared_ptr<XdmfHeavyDataDescription> p = shared_ptr<XdmfHeavyDataDescription>();
  return p;
}

std::string
XdmfPlaceholder::getName() const
{
  return "Placeholder";
}

void
XdmfPlaceholder::getProperties(std::map<std::string, std::string> & collectedProperties) const
{
  collectedProperties["Format"] = this->getName();
}

void
XdmfPlaceholder::read(XdmfArray * const array)
{
  array->initialize(this->getType(), this->getDimensions());
}

// C Wrappers

XDMFPLACEHOLDER * XdmfPlaceholderNew(char * hdf5FilePath,
                                     int type,
                                     unsigned int * start,
                                     unsigned int * stride,
                                     unsigned int * dimensions,
                                     unsigned int * dataspaceDimensions,
                                     unsigned int numDims,
                                     int * status)
{
  XDMF_ERROR_WRAP_START(status)
  try
  {
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
    shared_ptr<XdmfPlaceholder> generatedController = XdmfPlaceholder::New(std::string(hdf5FilePath), buildType, startVector, strideVector, dimVector, dataspaceVector);
    return (XDMFPLACEHOLDER *)((void *)(new XdmfPlaceholder(*generatedController.get())));
  }
  catch (...)
  {
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
    shared_ptr<XdmfPlaceholder> generatedController = XdmfPlaceholder::New(std::string(hdf5FilePath), buildType, startVector, strideVector, dimVector, dataspaceVector);
    return (XDMFPLACEHOLDER *)((void *)(new XdmfPlaceholder(*generatedController.get())));
  }
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

// C Wrappers for parent classes are generated by macros

XDMF_HEAVYCONTROLLER_C_CHILD_WRAPPER(XdmfPlaceholder, XDMFPLACEHOLDER)
