/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfArray.cpp                                                       */
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

#include <boost/tokenizer.hpp>
#include <limits>
#include <sstream>
#include <utility>
#include <stack>
#include <math.h>
#include <string.h>
#include <cstdint>
#include "XdmfArray.hpp"
#include "XdmfArrayType.hpp"
#include "XdmfArrayReference.hpp"
#include "XdmfBinaryController.hpp"
#include "XdmfCoreReader.hpp"
#include "XdmfFunction.hpp"
#include "XdmfSubset.hpp"
#include "XdmfHeavyDataController.hpp"
#include "XdmfVisitor.hpp"
#include "XdmfError.hpp"

XDMF_CHILDREN_IMPLEMENTATION(XdmfArray,
                             XdmfHeavyDataController,
                             HeavyDataController,
                             Name)

class XdmfArray::Clear : public boost::static_visitor<void> {
public:

  Clear(XdmfArray * const array) :
    mArray(array)
  {
  }

  void
  operator()(const boost::blank & array) const
  {
    return;
  }

  template<typename T>
  void
  operator()(const shared_ptr<std::vector<T> > & array) const
  {
    array->clear();
  }

  template<typename T>
  void
  operator()(const boost::shared_array<const T> & array) const
  {
    mArray->internalizeArrayPointer();
    boost::apply_visitor(*this,
                         mArray->mArray);
  }

private:
  XdmfArray * const mArray;
};

class XdmfArray::Erase : public boost::static_visitor<void> {
public:

  Erase(XdmfArray * const array,
        const unsigned int index) :
    mArray(array),
    mIndex(index)
  {
  }

  void
  operator()(const boost::blank & array) const
  {
    return;
  }

  template<typename T>
  void
  operator()(const shared_ptr<std::vector<T> > & array) const
  {
    array->erase(array->begin() + mIndex);
  }

  template<typename T>
  void
  operator()(const boost::shared_array<const T> & array) const
  {
    mArray->internalizeArrayPointer();
    boost::apply_visitor(*this,
                         mArray->mArray);
  }

private:

  XdmfArray * const mArray;
  const unsigned int mIndex;
};

class XdmfArray::GetArrayType :
  public boost::static_visitor<shared_ptr<const XdmfArrayType> > {
public:

  GetArrayType(const shared_ptr<XdmfHeavyDataController> & heavyDataController) :
    mHeavyDataController(heavyDataController)
  {
  }

  shared_ptr<const XdmfArrayType>
  getArrayType(const char * const) const
  {
    return XdmfArrayType::Int8();
  }

  shared_ptr<const XdmfArrayType>
  getArrayType(const short * const) const
  {
    return XdmfArrayType::Int16();
  }

  shared_ptr<const XdmfArrayType>
  getArrayType(const int * const) const
  {
    return XdmfArrayType::Int32();
  }

  shared_ptr<const XdmfArrayType>
  getArrayType(const long * const) const
  {
    return XdmfArrayType::Int64();
  }

  shared_ptr<const XdmfArrayType>
  getArrayType(const float * const) const
  {
    return XdmfArrayType::Float32();
  }

  shared_ptr<const XdmfArrayType>
  getArrayType(const double * const) const
  {
    return XdmfArrayType::Float64();
  }

  shared_ptr<const XdmfArrayType>
  getArrayType(const unsigned char * const) const
  {
    return XdmfArrayType::UInt8();
  }

  shared_ptr<const XdmfArrayType>
  getArrayType(const unsigned short * const) const
  {
    return XdmfArrayType::UInt16();
  }

  shared_ptr<const XdmfArrayType>
  getArrayType(const unsigned int * const) const
  {
    return XdmfArrayType::UInt32();
  }

  shared_ptr<const XdmfArrayType>
  getArrayType(const uint64_t * const) const
  {
    return XdmfArrayType::UInt64();
  }

  shared_ptr<const XdmfArrayType>
  getArrayType(const std::string * const) const
  {
    return XdmfArrayType::String();
  }

  shared_ptr<const XdmfArrayType>
  operator()(const boost::blank & array) const
  {
    if(mHeavyDataController) {
      return mHeavyDataController->getType();
    }
    return XdmfArrayType::Uninitialized();
  }

  template<typename T>
  shared_ptr<const XdmfArrayType>
  operator()(const shared_ptr<std::vector<T> > & array) const
  {
    return this->getArrayType(&(array.get()->operator[](0)));
  }

  template<typename T>
  shared_ptr<const XdmfArrayType>
  operator()(const boost::shared_array<const T> & array) const
  {
    return this->getArrayType(array.get());
  }

private:

  const shared_ptr<XdmfHeavyDataController> mHeavyDataController;
};

class XdmfArray::GetCapacity : public boost::static_visitor<unsigned int> {
public:

  GetCapacity()
  {
  }

  unsigned int
  operator()(const boost::blank & array) const
  {
    return 0;
  }

  template<typename T>
  unsigned int
  operator()(const shared_ptr<std::vector<T> > & array) const
  {
    return array->capacity();
  }

  template<typename T>
  unsigned int
  operator()(const boost::shared_array<const T> & array) const
  {
    return 0;
  }
};

class XdmfArray::GetValuesPointer :
  public boost::static_visitor<const void *> {
public:

  GetValuesPointer()
  {
  }

  const void *
  operator()(const boost::blank & array) const
  {
    return NULL;
  }

  template<typename T>
  const void *
  operator()(const shared_ptr<std::vector<T> > & array) const
  {
    return &array->operator[](0);
  }

  template<typename T>
  const void *
  operator()(const boost::shared_array<const T> & array) const
  {
    return array.get();
  }
};

class XdmfArray::GetValuesString : public boost::static_visitor<std::string> {
public:

  GetValuesString(const int arrayPointerNumValues) :
    mArrayPointerNumValues(arrayPointerNumValues)
  {
  }

  template<typename T, typename U>
  std::string
  getValuesString(const T * const array,
                  const int numValues) const
  {
    const int lastIndex = numValues - 1;

    if(lastIndex < 0) {
      return "";
    }

    std::stringstream toReturn;
    toReturn.precision(std::numeric_limits<U>::digits10 + 2);
    for(int i=0; i<lastIndex; ++i) {
      toReturn << (U)array[i] << " ";
    }
    toReturn << (U)array[lastIndex];
    return toReturn.str();
  }

  std::string
  getValuesString(const char * const array,
                  const int numValues) const
  {
    return getValuesString<char, int>(array, numValues);
  }

  std::string
  getValuesString(const unsigned char * const array,
                  const int numValues) const
  {
    return getValuesString<unsigned char, int>(array, numValues);
  }

  template<typename T>
  std::string
  getValuesString(const T * const array,
                  const int numValues) const
  {
    return getValuesString<T, T>(array, numValues);
  }

  std::string
  operator()(const boost::blank & array) const
  {
    return "";
  }

  template<typename T>
  std::string
  operator()(const shared_ptr<std::vector<T> > & array) const
  {
    return getValuesString(&(array->operator[](0)), array->size());
  }

  template<typename T>
  std::string
  operator()(const boost::shared_array<const T> & array) const
  {
    return getValuesString(array.get(), mArrayPointerNumValues);
  }

private:

  const unsigned int mArrayPointerNumValues;
};

class XdmfArray::InsertArray : public boost::static_visitor<void> {
public:

  InsertArray(XdmfArray * const array,
              const unsigned int startIndex,
              const unsigned int valuesStartIndex,
              const unsigned int numValues,
              const unsigned int arrayStride,
              const unsigned int valuesStride,
              std::vector<unsigned int> & dimensions,
              const shared_ptr<const XdmfArray> & arrayToCopy) :
    mArray(array),
    mStartIndex(startIndex),
    mValuesStartIndex(valuesStartIndex),
    mNumValues(numValues),
    mArrayStride(arrayStride),
    mValuesStride(valuesStride),
    mDimensions(dimensions),
    mArrayToCopy(arrayToCopy)
  {
  }

  void
  operator()(const boost::blank & array) const
  {
    const shared_ptr<const XdmfArrayType> copyType =
      mArrayToCopy->getArrayType();
    if(copyType == XdmfArrayType::Uninitialized()) {
        return;
    }
    mArray->initialize(copyType);
    boost::apply_visitor(*this,
                         mArray->mArray);
  }

  template<typename T>
  void
  operator()(const shared_ptr<std::vector<T> > & array) const
  {
    unsigned int size = mStartIndex + (mNumValues - 1) * mArrayStride + 1;
    if(array->size() < size) {
      array->resize(size);
      mDimensions.clear();
    }
    mArrayToCopy->getValues(mValuesStartIndex,
                            &(array->operator[](mStartIndex)),
                            mNumValues,
                            mValuesStride,
                            mArrayStride);
  }

  template<typename T>
  void
  operator()(const boost::shared_array<const T> & array) const
  {
    mArray->internalizeArrayPointer();
    boost::apply_visitor(*this,
                         mArray->mArray);
  }

private:

  XdmfArray * const mArray;
  const unsigned int mStartIndex;
  const unsigned int mValuesStartIndex;
  const unsigned int mNumValues;
  const unsigned int mArrayStride;
  const unsigned int mValuesStride;
  std::vector<unsigned int> & mDimensions;
  const shared_ptr<const XdmfArray> mArrayToCopy;
};

class XdmfArray::InternalizeArrayPointer : public boost::static_visitor<void> {
public:

  InternalizeArrayPointer(XdmfArray * const array) :
    mArray(array)
  {
  }

  void
  operator()(const boost::blank & array) const
  {
    return;
  }

  template<typename T>
  void
  operator()(const shared_ptr<std::vector<T> > & array) const
  {
    return;
  }

  template<typename T>
  void
  operator()(const boost::shared_array<const T> & array) const
  {
    const T * const pointer = array.get();
    shared_ptr<std::vector<T> > newArray(new std::vector<T>(pointer,
                                                            pointer + mArray->mArrayPointerNumValues));
    mArray->mArray = newArray;
    mArray->mArrayPointerNumValues = 0;
  }

private:

  XdmfArray * const mArray;
};

class XdmfArray::IsInitialized : public boost::static_visitor<bool> {
public:

  IsInitialized()
  {
  }

  bool
  operator()(const boost::blank &) const
  {
    return false;
  }

  template<typename T>
  bool
  operator()(const shared_ptr<std::vector<T> > &) const
  {
    return true;
  }

  template<typename T>
  bool
  operator()(const T &) const
  {
    return true;
  }
};

class XdmfArray::Reserve : public boost::static_visitor<void> {
public:

  Reserve(XdmfArray * const array,
          const unsigned int size):
    mArray(array),
    mSize(size)
  {
  }

  void
  operator()(const boost::blank & array) const
  {
    mArray->mTmpReserveSize = mSize;
  }

  template<typename T>
  void
  operator()(shared_ptr<std::vector<T> > & array) const
  {
    array->reserve(mSize);
  }

  template<typename T>
  void
  operator()(const boost::shared_array<const T> & array) const
  {
    mArray->internalizeArrayPointer();
    boost::apply_visitor(*this,
                         mArray->mArray);
  }

private:

  XdmfArray * const mArray;
  const unsigned int mSize;
};

class XdmfArray::Size : public boost::static_visitor<unsigned int> {
public:

  Size(const XdmfArray * const array) :
    mArray(array)
  {
  }

  unsigned int
  operator()(const boost::blank & array) const
  {
    unsigned int total = 0;
    for (unsigned int i = 0; i < mArray->mHeavyDataControllers.size(); ++i) {
      total += mArray->mHeavyDataControllers[i]->getSize();
    }
    return total;
  }

  template<typename T>
  unsigned int
  operator()(const shared_ptr<std::vector<T> > & array) const
  {
    return array->size();
  }

  template<typename T>
  unsigned int
  operator()(const boost::shared_array<const T> & array) const
  {
    return mArray->mArrayPointerNumValues;
  }

private:

  const XdmfArray * const mArray;
};

shared_ptr<XdmfArray>
XdmfArray::New()
{
  shared_ptr<XdmfArray> p(new XdmfArray());
  return p;
}

XdmfArray::XdmfArray() :
  mArrayPointerNumValues(0),
  mName(""),
  mTmpReserveSize(0),
  mReadMode(XdmfArray::Controller)
{
}

XdmfArray::XdmfArray(XdmfArray & refArray):
  XdmfItem(refArray),
  mDimensions(refArray.getDimensions()),
  mName(refArray.getName()),
  mReadMode(refArray.getReadMode())
{
  if (refArray.getArrayType() != XdmfArrayType::Uninitialized()) {
    this->initialize(refArray.getArrayType(), 0);
    if (refArray.getSize() > 0) {
      shared_ptr<const XdmfArray> tempPointer = shared_ptr<const XdmfArray>(&refArray, XdmfNullDeleter());
      this->insert(0, tempPointer, 0, tempPointer->getSize());
    }
  }
  if (refArray.getNumberHeavyDataControllers() > 0) {
    for (unsigned int i = 0; i < refArray.getNumberHeavyDataControllers(); ++i) {
      this->insert(refArray.getHeavyDataController(i));
    }
  }
  if (refArray.mReference) {
    this->setReference(refArray.getReference());
  }
}

XdmfArray::~XdmfArray()
{
}

const std::string XdmfArray::ItemTag = "DataItem";

void
XdmfArray::clear()
{
  boost::apply_visitor(Clear(this),
                       mArray);
  mDimensions.clear();
  this->setIsChanged(true);
}

void
XdmfArray::erase(const unsigned int index)
{
  boost::apply_visitor(Erase(this,
                             index),
                       mArray);
  mDimensions.clear();
  this->setIsChanged(true);
}

shared_ptr<const XdmfArrayType>
XdmfArray::getArrayType() const
{
  if (mHeavyDataControllers.size()>0) {
    return boost::apply_visitor(GetArrayType(mHeavyDataControllers[0]),
                                mArray);
  }
  else {
    return boost::apply_visitor(GetArrayType(shared_ptr<XdmfHeavyDataController>()),
                                mArray);
  }
}

unsigned int
XdmfArray::getCapacity() const
{
  return boost::apply_visitor(GetCapacity(),
                              mArray);
}

std::vector<unsigned int>
XdmfArray::getDimensions() const
{
  if(mDimensions.size() == 0) {
    if(!this->isInitialized() && mHeavyDataControllers.size() > 0) {
      std::vector<unsigned int> returnDimensions;
      std::vector<unsigned int> tempDimensions;
      // Find the controller with the most dimensions
      int dimControllerIndex = 0;
      unsigned int dimSizeMax = 0;
      unsigned int dimTotal = 0;
      for (unsigned int i = 0; i < mHeavyDataControllers.size(); ++i) {
        dimTotal += mHeavyDataControllers[i]->getSize();
        if (mHeavyDataControllers[i]->getSize() > dimSizeMax) {
          dimSizeMax = mHeavyDataControllers[i]->getSize();
          dimControllerIndex = i;
        }
      }
      // Total up the size of the lower dimensions
      int controllerDimensionSubtotal = 1;
      for (unsigned int i = 0;
           i < mHeavyDataControllers[dimControllerIndex]->getDimensions().size() - 1;
           ++i) {
        returnDimensions.push_back(mHeavyDataControllers[dimControllerIndex]->getDimensions()[i]);
        controllerDimensionSubtotal *= mHeavyDataControllers[dimControllerIndex]->getDimensions()[i];
      }
      // Divide the total contained by the dimensions by the size of the lower dimensions
      returnDimensions.push_back(dimTotal/controllerDimensionSubtotal);
      return returnDimensions;
    }
    const unsigned int size = this->getSize();
    return std::vector<unsigned int>(1, size);
  }
  return mDimensions;
}

std::string
XdmfArray::getDimensionsString() const
{
  const std::vector<unsigned int> dimensions = this->getDimensions();
  return GetValuesString(dimensions.size()).getValuesString(&dimensions[0],
                                                            dimensions.size());
}

std::map<std::string, std::string>
XdmfArray::getItemProperties() const
{
  std::map<std::string, std::string> arrayProperties;
  if(mHeavyDataControllers.size() > 0) {
    mHeavyDataControllers[0]->getProperties(arrayProperties);
  }
  else {
    arrayProperties.insert(std::make_pair("Format", "XML"));
  }
  arrayProperties.insert(std::make_pair("Dimensions",
                                        this->getDimensionsString()));
  if(mName.compare("") != 0) {
    arrayProperties.insert(std::make_pair("Name", mName));
  }
  shared_ptr<const XdmfArrayType> type = this->getArrayType();
  type->getProperties(arrayProperties);
  return arrayProperties;
}

std::string
XdmfArray::getItemTag() const
{
  return ItemTag;
}

std::string
XdmfArray::getName() const
{
  return mName;
}

XdmfArray::ReadMode
XdmfArray::getReadMode() const
{
  return mReadMode;
}

unsigned int
XdmfArray::getSize() const
{
  return boost::apply_visitor(Size(this),
                              mArray);
}

shared_ptr<XdmfArrayReference>
XdmfArray::getReference()
{
  if (mReference) {
    return mReference;
  }
  else {
    // Returning arbitrary Reference since one isn't defined
    return shared_ptr<XdmfArrayReference>();
  }
}

void *
XdmfArray::getValuesInternal()
{
  return const_cast<void *>
    (static_cast<const XdmfArray &>(*this).getValuesInternal());
}

const void *
XdmfArray::getValuesInternal() const
{
  return boost::apply_visitor(GetValuesPointer(),
                              mArray);
}

std::string
XdmfArray::getValuesString() const
{
  return boost::apply_visitor(GetValuesString(mArrayPointerNumValues),
                              mArray);
}

shared_ptr<XdmfHeavyDataController>
XdmfArray::getHeavyDataController()
{
  return boost::const_pointer_cast<XdmfHeavyDataController>
    (static_cast<const XdmfArray &>(*this).getHeavyDataController(0));
}

shared_ptr<const XdmfHeavyDataController>
XdmfArray::getHeavyDataController() const
{
  if (mHeavyDataControllers.size() > 0) {
    return mHeavyDataControllers[0];
  }
  else {
    return shared_ptr<XdmfHeavyDataController>();
  }
}

void
XdmfArray::initialize(const shared_ptr<const XdmfArrayType> & arrayType,
                      const unsigned int size)
{
  if(arrayType == XdmfArrayType::Int8()) {
    this->initialize<char>(size);
  }
  else if(arrayType == XdmfArrayType::Int16()) {
    this->initialize<short>(size);
  }
  else if(arrayType == XdmfArrayType::Int32()) {
    this->initialize<int>(size);
  }
  else if(arrayType == XdmfArrayType::Int64()) {
    this->initialize<long>(size);
  }
  else if(arrayType == XdmfArrayType::Float32()) {
    this->initialize<float>(size);
  }
  else if(arrayType == XdmfArrayType::Float64()) {
    this->initialize<double>(size);
  }
  else if(arrayType == XdmfArrayType::UInt8()) {
    this->initialize<unsigned char>(size);
  }
  else if(arrayType == XdmfArrayType::UInt16()) {
    this->initialize<unsigned short>(size);
  }
  else if(arrayType == XdmfArrayType::UInt32()) {
    this->initialize<unsigned int>(size);
  }
  else if(arrayType == XdmfArrayType::UInt64()) {
    this->initialize<uint64_t>(size);
  }
  else if(arrayType == XdmfArrayType::String()) {
    this->initialize<std::string>(size);
  }
  else if(arrayType == XdmfArrayType::Uninitialized()) {
    this->release();
  }
  else {
    XdmfError::message(XdmfError::FATAL,
                       "Array of unsupported type in XdmfArray::initialize");
  }
  this->setIsChanged(true);
}

void
XdmfArray::initialize(const shared_ptr<const XdmfArrayType> & arrayType,
                      const std::vector<unsigned int> & dimensions)
{
  mDimensions = dimensions;
  const unsigned int size = std::accumulate(dimensions.begin(),
                                            dimensions.end(),
                                            1,
                                            std::multiplies<unsigned int>());
  return this->initialize(arrayType, size);
}

void
XdmfArray::insert(const unsigned int startIndex,
                  const shared_ptr<const XdmfArray> values,
                  const unsigned int valuesStartIndex,
                  const unsigned int numValues,
                  const unsigned int arrayStride,
                  const unsigned int valuesStride)
{
  boost::apply_visitor(InsertArray(this,
                                   startIndex,
                                   valuesStartIndex,
                                   numValues,
                                   arrayStride,
                                   valuesStride,
                                   mDimensions,
                                   values),
                       mArray);
  this->setIsChanged(true);
}


void
XdmfArray::insert(const std::vector<unsigned int> startIndex,
                  const shared_ptr<const XdmfArray> values,
                  const std::vector<unsigned int> valuesStartIndex,
                  const std::vector<unsigned int> numValues,
                  const std::vector<unsigned int> numInserted,
                  const std::vector<unsigned int> arrayStride,
                  const std::vector<unsigned int> valuesStride)
{
  // Ensuring dimensions match up when pulling data
  if ((values->getDimensions().size() == valuesStartIndex.size()
      && valuesStartIndex.size() == numValues.size()
      && numValues.size() == valuesStride.size())
      && (numInserted.size() == startIndex.size()
      && startIndex.size() == this->getDimensions().size()
      && this->getDimensions().size() == arrayStride.size())) {
    // Pull data from values
    std::vector<unsigned int > dimTotalVector;
    unsigned int dimTotal = 1;
    for (unsigned int i = 0; i < values->getDimensions().size(); ++i) {
      dimTotalVector.push_back(dimTotal);
      dimTotal *= values->getDimensions()[i];
    }
    std::vector<unsigned int> indexVector;
    for (unsigned int i = 0; i < values->getDimensions().size(); ++i) {
      indexVector.push_back(0);
    }
    shared_ptr<XdmfArray> holderArray = XdmfArray::New();
    unsigned int holderoffset = 0;
    // End when the last index is incremented
    while (indexVector[indexVector.size()-1] < 1) {
      // Initialize the section of the array you're pulling from
      unsigned int startTotal = 0;
      dimTotal = 1;
      for (unsigned int i = 0; i < values->getDimensions().size(); ++i) {
        // Stride doesn't factor in to the first dimension
        // Since it's being used with the insert call
        if (i == 0) {
          startTotal += valuesStartIndex[i] * dimTotal;
        }
        else {
          startTotal += valuesStartIndex[i] * dimTotal
                        + valuesStride[i] * dimTotal * indexVector[i-1];
        }
        dimTotal *= values->getDimensions()[i];
      }
      // Insert the subsection
      holderArray->insert(holderoffset,
                          values,
                          startTotal,
                          numValues[0],
                          1,
                          valuesStride[0]);
      holderoffset+=numValues[0];
      // Increment up the vector
      bool increment = true;
      for (unsigned int i = 0; i < indexVector.size() && increment; ++i) {
        indexVector[i]++;
        // To keep the loop from breaking at the end
        if (i+1 < numValues.size()) {
          if (indexVector[i] >= numValues[i+1]) {
            indexVector[i] = indexVector[i] % numValues[i+1];
          }
          else {
            increment = false;
          }
        }
      }
    }
    // Values being inserted retrieved
    // Use an variation of the last loop to insert into this array

    indexVector.clear();
    for (unsigned int i = 0; i < this->getDimensions().size(); ++i) {
      indexVector.push_back(0);
    }
    holderoffset = 0;
    // End when the last index is incremented
    while (indexVector[indexVector.size()-1] < 1) {
      // Initialize the section of the array you're pulling from
      unsigned int startTotal = 0;
      dimTotal = 1;
      for (unsigned int i = 0; i < this->getDimensions().size(); ++i) {
        if (i == 0) {
          // Stride doesn't factor in to the first dimension
          // Since it's being used with the insert call
          startTotal += startIndex[i] * dimTotal;
        }
        else {
          startTotal += startIndex[i] * dimTotal + arrayStride[i] * dimTotal * indexVector[i-1];
        }
        dimTotal *= this->getDimensions()[i];
      }
      // Insert the subsection
      this->insert(startTotal, holderArray, holderoffset, numInserted[0], arrayStride[0], 1);
      holderoffset+=numInserted[0];
      // Increment up the vector
      bool increment = true;
      for (unsigned int i = 0; i < indexVector.size() && increment; ++i) {
        indexVector[i]++;
        if (i+1 < numInserted.size()) {
          // To keep the loop from breaking at the end
          if (indexVector[i] >= numInserted[i+1]) {
            indexVector[i] = indexVector[i] % numInserted[i+1];
          }
          else {
            increment = false;
          }
        }
      }
    }
    this->setIsChanged(true);
  }
  else {
    // Throw an error
    if (!(values->getDimensions().size() == valuesStartIndex.size()
          && valuesStartIndex.size() == numValues.size()
          && numValues.size() == valuesStride.size())) {
      XdmfError::message(XdmfError::FATAL,
                         "Error: Number of starts, strides, and/or values "
                         "retrieved does not match up with the dimensions "
                         "of the array being retrieved from");
    }
    else if (!(numInserted.size() == startIndex.size()
               && startIndex.size() == this->getDimensions().size()
               && this->getDimensions().size() == arrayStride.size())) {
      XdmfError::message(XdmfError::FATAL,
                         "Error: Number of starts, strides, and/or values "
                         "written does not match up with the dimensions "
                         "of the array being inserted into");
    }
  }
}

bool
XdmfArray::isInitialized() const
{
  return boost::apply_visitor(IsInitialized(),
                                mArray);
}

void
XdmfArray::internalizeArrayPointer()
{
  boost::apply_visitor(InternalizeArrayPointer(this),
                       mArray);
}

void
XdmfArray::populateItem(const std::map<std::string, std::string> & itemProperties,
                        const std::vector<shared_ptr<XdmfItem> > & childItems,
                        const XdmfCoreReader * const reader)
{
  // This inserts any XdmfInformation in childItems into the object.
  XdmfItem::populateItem(itemProperties, childItems, reader);

  bool filled = false;

  // Check for Function
  std::map<std::string, std::string>::const_iterator itemType =
    itemProperties.find("ItemType");

  if (itemType !=  itemProperties.end()) {
    if (itemType->second.compare("Function") == 0) {
      std::map<std::string, std::string>::const_iterator expressionLocation =
        itemProperties.find("Function");
      if (expressionLocation ==  itemProperties.end()) {
        XdmfError::message(XdmfError::FATAL,
                           "'Function' not found in itemProperties for Function"
                           " ItemType in XdmfArray::populateItem");
      }
      std::string expression = expressionLocation->second;

      // Convert from old format to new Variable format
      // $X -> ValX
      size_t loc = expression.find("$");

      while (loc != std::string::npos) {
        expression.replace(loc, 1, "Val");
        loc = expression.find("$", loc);
     }

      // Create Variable list

      std::map<std::string, shared_ptr<XdmfArray> > variableMap;

      unsigned int variableIndex = 0;
      for(std::vector<shared_ptr<XdmfItem> >::const_iterator iter =
            childItems.begin();
          iter != childItems.end();
          ++iter) {
        if(shared_ptr<XdmfArray> array = shared_dynamic_cast<XdmfArray>(*iter)) {
          std::stringstream variableKey;
          variableKey << "Val" << variableIndex;
          variableMap[variableKey.str()] = array;
          variableIndex++;
        }
      }

      shared_ptr<XdmfFunction> function = XdmfFunction::New(expression, variableMap);

      this->setReference(function);
      this->setReadMode(XdmfArray::Reference);
      filled = true;
    }
    else if (itemType->second.compare("HyperSlab") == 0) {

      shared_ptr<XdmfArray> dimArray;
      shared_ptr<XdmfArray> valArray;

      unsigned int foundArrayIndex = 0;

      for(std::vector<shared_ptr<XdmfItem> >::const_iterator iter =
            childItems.begin();
          iter != childItems.end();
          ++iter) {
        if(shared_ptr<XdmfArray> array = shared_dynamic_cast<XdmfArray>(*iter)) {
          if (foundArrayIndex == 0)
          {
            dimArray = array;
            foundArrayIndex++;
          }
          else if (foundArrayIndex == 1)
          {
            valArray = array;
            foundArrayIndex++;
          }
        }
      }

      if (!(dimArray))
      {
        XdmfError::message(XdmfError::FATAL,
                           "Error: Hyperslab description missing");
      }
      if (!(valArray))
      {
        XdmfError::message(XdmfError::FATAL,
                           "Error: Hyperslab values missing");
      }

      if (dimArray->getSize() % 3 != 0)
      {
        XdmfError::message(XdmfError::FATAL,
                           "Error: Hyperslab description structured improperly");
      }

      // A start, stride, and dimension need to be
      // specified for each dimension
      unsigned int numDims = dimArray->getSize() / 3;

      // Start, stride, and dims are set via the first array provided
      std::vector<unsigned int> start;
      std::vector<unsigned int> stride;
      std::vector<unsigned int> dimensions;

      unsigned int i = 0;

      while (i < dimArray->getSize() / 3)
      {
        start.push_back(dimArray->getValue<unsigned int>(i));
        ++i;
      }

      while (i < 2 * (dimArray->getSize() / 3))
      {
        stride.push_back(dimArray->getValue<unsigned int>(i));
        ++i;
      }

      while (i < dimArray->getSize())
      {
        dimensions.push_back(dimArray->getValue<unsigned int>(i));
        ++i;
      }

      shared_ptr<XdmfSubset> subset =
        XdmfSubset::New(valArray,
                        start,
                        stride,
                        dimensions);
      this->setReference(subset);
      this->setReadMode(XdmfArray::Reference);
      filled = true;
    }
  }

  if (!filled) {
    std::vector<shared_ptr<XdmfHeavyDataController> > readControllers = reader->generateHeavyDataControllers(itemProperties);

    mHeavyDataControllers.clear();

    for (unsigned int i = 0; i < readControllers.size(); ++i) {
      mHeavyDataControllers.push_back(readControllers[i]);
    }

    const shared_ptr<const XdmfArrayType> arrayType =
      XdmfArrayType::New(itemProperties);

    std::map<std::string, std::string>::const_iterator content =
      itemProperties.find("Content");
    if(content == itemProperties.end()) {
      XdmfError::message(XdmfError::FATAL,
                         "'Content' not found in itemProperties in "
                         "XdmfArray::populateItem");
    }

    unsigned int contentIndex;

    const std::string & contentVal = content->second;

    std::vector<std::string> contentVals;

    // Split the content based on "|" characters
    size_t barSplit = 0;
    std::string splitString(contentVal);
    std::string subcontent;
    while (barSplit != std::string::npos) {
      barSplit = 0;
      barSplit = splitString.find_first_of("|", barSplit);
      if (barSplit == std::string::npos) {
        subcontent = splitString;
      }
      else {
        subcontent = splitString.substr(0, barSplit);
        splitString = splitString.substr(barSplit+1);
        barSplit++;
      }
      contentVals.push_back(subcontent);
    }

    std::map<std::string, std::string>::const_iterator dimensions =
      itemProperties.find("Dimensions");
    if(dimensions == itemProperties.end()) {
      XdmfError::message(XdmfError::FATAL,
                         "'Dimensions' not found in itemProperties in "
                         "XdmfArray::populateItem");
    }

    boost::tokenizer<> tokens(dimensions->second);
    for(boost::tokenizer<>::const_iterator iter = tokens.begin();
        iter != tokens.end();
        ++iter) {
      mDimensions.push_back(atoi((*iter).c_str()));
    }

    std::map<std::string, std::string>::const_iterator format =
      itemProperties.find("Format");
    if(format == itemProperties.end()) {
      XdmfError::message(XdmfError::FATAL,
                         "'Format' not found in itemProperties in "
                         "XdmfArray::populateItem");
    }
    const std::string & formatVal = format->second;

    if (readControllers.size() == 0) {
      if(formatVal.compare("XML") == 0) {
        this->initialize(arrayType,
                         mDimensions);
        unsigned int index = 0;
        boost::char_separator<char> sep(" \t\n");
        for (contentIndex = 0; contentIndex < contentVals.size(); ++contentIndex)
        {
          boost::tokenizer<boost::char_separator<char> > tokens(contentVals[contentIndex], sep);
          if(arrayType == XdmfArrayType::String()) {
            for(boost::tokenizer<boost::char_separator<char> >::const_iterator
                  iter = tokens.begin();
                iter != tokens.end();
                ++iter, ++index) {
              this->insert(index, *iter);
            }
          }
          else {
            for(boost::tokenizer<boost::char_separator<char> >::const_iterator
                  iter = tokens.begin();
                iter != tokens.end();
                ++iter, ++index) {
              this->insert(index, atof((*iter).c_str()));
            }
          }
        }
      }
      else
      {
        XdmfError::message(XdmfError::FATAL,
                           "Error: Invalid Data Format "
                           "in XdmfArray::populateItem");
      }
    }
  }

  std::map<std::string, std::string>::const_iterator name =
    itemProperties.find("Name");
  if(name != itemProperties.end()) {
    mName = name->second;
  }
  else {
    mName = "";
  }
  this->setIsChanged(true);
}

void
XdmfArray::read()
{
  switch (mReadMode)
  {
    case XdmfArray::Controller:
      this->readController();
      break;
    case XdmfArray::Reference:
      this->readReference();
      break;
    default:
      XdmfError::message(XdmfError::FATAL,
                         "Error: Invalid Read Mode");
  }
}

void
XdmfArray::readController()
{
  if(mHeavyDataControllers.size() > 1) {
    this->release();
    for (unsigned int i = 0; i < mHeavyDataControllers.size(); ++i) {
      shared_ptr<XdmfArray> tempArray = XdmfArray::New();
      mHeavyDataControllers[i]->read(tempArray.get());
      unsigned int dimTotal = 1;
      for (unsigned int j = 0; j < mHeavyDataControllers[i]->getDimensions().size(); ++j) {
        dimTotal *= mHeavyDataControllers[i]->getDimensions()[j];
      }
      this->insert(mHeavyDataControllers[i]->getArrayOffset(), tempArray, 0, dimTotal, 1, 1);
    }
    std::vector<unsigned int> returnDimensions;
    std::vector<unsigned int> tempDimensions;
    // Find the controller with the most dimensions
    int dimControllerIndex = 0;
    unsigned int dimSizeMax = 0;
    unsigned int dimTotal = 0;
    for (unsigned int i = 0; i < mHeavyDataControllers.size(); ++i) {
        dimTotal += mHeavyDataControllers[i]->getSize();
        if (mHeavyDataControllers[i]->getSize() > dimSizeMax) {
          dimSizeMax = mHeavyDataControllers[i]->getSize();
          dimControllerIndex = i;
        }
    }
    // Total up the size of the lower dimensions
    int controllerDimensionSubtotal = 1;
    for (unsigned int i = 0;
         i < mHeavyDataControllers[dimControllerIndex]->getDimensions().size() - 1;
         ++i) {
      returnDimensions.push_back(mHeavyDataControllers[dimControllerIndex]->getDimensions()[i]);
      controllerDimensionSubtotal *= mHeavyDataControllers[dimControllerIndex]->getDimensions()[i];
    }
    // Divide the total contained by the dimensions by the size of the lower dimensions
    returnDimensions.push_back(dimTotal/controllerDimensionSubtotal);
    mDimensions = returnDimensions;
  }
  else if (mHeavyDataControllers.size() == 1 && mHeavyDataControllers[0]->getArrayOffset() == 0) {
    this->release();
    mHeavyDataControllers[0]->read(this);
    mDimensions = mHeavyDataControllers[0]->getDimensions();
  }
  else if (mHeavyDataControllers.size() == 1 && mHeavyDataControllers[0]->getArrayOffset() > 0) {
    this->release();
    shared_ptr<XdmfArray> tempArray = XdmfArray::New();
    mHeavyDataControllers[0]->read(tempArray.get());
    this->insert(mHeavyDataControllers[0]->getArrayOffset(), tempArray, 0, mHeavyDataControllers[0]->getSize(), 1, 1);
    mDimensions = mHeavyDataControllers[0]->getDimensions();
  }
  this->setIsChanged(true);
}

void
XdmfArray::readReference()
{
  shared_ptr<XdmfArray> tempArray = mReference->read();
  this->swap(tempArray);
  this->setIsChanged(true);
}

void
XdmfArray::release()
{
  mArray = boost::blank();
  mArrayPointerNumValues = 0;
  mDimensions.clear();
}

void
XdmfArray::reserve(const unsigned int size)
{
  boost::apply_visitor(Reserve(this,
                               size),
                       mArray);
  this->setIsChanged(true);
}

void
XdmfArray::setHeavyDataController(shared_ptr<XdmfHeavyDataController> newController)
{
  // Since this is replacing the previous version which was designed to
  // completely replace the controller of the array
  // It will clear the current controllers before adding the new one in
  mHeavyDataControllers.clear();
  mHeavyDataControllers.push_back(newController);
  this->setIsChanged(true);
}

void
XdmfArray::setHeavyDataController(std::vector<shared_ptr<XdmfHeavyDataController> > & newControllers)
{
  if (mHeavyDataControllers.size() != newControllers.size()) {
    mHeavyDataControllers.resize(newControllers.size());
  }
  for (unsigned int i = 0; i < newControllers.size(); ++i) {
    mHeavyDataControllers[i] = newControllers[i];
  }
  this->setIsChanged(true);
}


void
XdmfArray::setName(const std::string & name)
{
  mName = name;
  this->setIsChanged(true);
}

void
XdmfArray::setReadMode(XdmfArray::ReadMode newStatus)
{
  mReadMode = newStatus;
  this->setIsChanged(true);
}

void
XdmfArray::setReference(shared_ptr<XdmfArrayReference> newReference)
{
  mReference = newReference;
  this->setIsChanged(true);
}

void
XdmfArray::swap(const shared_ptr<XdmfArray> array)
{
  std::swap(mArray, array->mArray);
  std::swap(mArrayPointerNumValues, array->mArrayPointerNumValues);
  std::swap(mDimensions, array->mDimensions);
  std::swap(mHeavyDataControllers, array->mHeavyDataControllers);
  this->setIsChanged(true);
}

void
XdmfArray::traverse(const shared_ptr<XdmfBaseVisitor> visitor)
{
  XdmfItem::traverse(visitor);
  if (mReference) {
    mReference->accept(visitor);
  }
}

// C wrappers

XDMFARRAY *
XdmfArrayNew()
{
  try
  {
    XDMFARRAY * returnPointer;
    shared_ptr<XdmfArray> generatedArray = XdmfArray::New();
    returnPointer = (XDMFARRAY *)((void *)(new XdmfArray(*generatedArray.get())));
    generatedArray.reset();
    return returnPointer;
  }
  catch (...)
  {
    XDMFARRAY * returnPointer;
    shared_ptr<XdmfArray> generatedArray = XdmfArray::New();
    returnPointer = (XDMFARRAY *)((void *)(new XdmfArray(*generatedArray.get())));
    generatedArray.reset();
    return returnPointer;
  }
}

void XdmfArrayClear(XDMFARRAY * array)
{
  ((XdmfArray *)(array))->clear();
}

void XdmfArrayErase(XDMFARRAY * array, unsigned int index)
{
  ((XdmfArray *)(array))->erase(index);
}

int XdmfArrayGetArrayType(XDMFARRAY * array, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  shared_ptr<const XdmfArrayType> compareType = ((XdmfArray *)(array))->getArrayType();
  std::string typeName = compareType->getName();
  unsigned int typePrecision = compareType->getElementSize();
  if (typeName == XdmfArrayType::UInt8()->getName())
  {
      return XDMF_ARRAY_TYPE_UINT8;
  }
  else if (typeName == XdmfArrayType::UInt16()->getName())
  {
      return XDMF_ARRAY_TYPE_UINT16;
  }
  else if (typeName == XdmfArrayType::UInt32()->getName())
  {
      return XDMF_ARRAY_TYPE_UINT32;
  }
  else if (typeName == XdmfArrayType::UInt64()->getName())
  {
      return XDMF_ARRAY_TYPE_UINT64;
  }
  else if (typeName == XdmfArrayType::Int8()->getName())
  {
      return XDMF_ARRAY_TYPE_INT8;
  }
  else if (typeName == XdmfArrayType::Int16()->getName())
  {
      return XDMF_ARRAY_TYPE_INT16;
  }
  else if (typeName == XdmfArrayType::Int32()->getName() || typeName == XdmfArrayType::Int64()->getName())
  {
    if (typePrecision == 4)
    {
      return XDMF_ARRAY_TYPE_INT32;
    }
    else if (typePrecision == 8)
    {
      return XDMF_ARRAY_TYPE_INT64;
    }
    else
    {
    }
  }
  else if (typeName == XdmfArrayType::Float32()->getName() || typeName == XdmfArrayType::Float64()->getName())
  {
    if (typePrecision == 4)
    {
      return XDMF_ARRAY_TYPE_FLOAT32;
    }
    else if (typePrecision == 8)
    {
      return XDMF_ARRAY_TYPE_FLOAT64;
    }
    else
    {
    }
  }
  else if (typeName == XdmfArrayType::String()->getName())
  {
    //This shouldn't be used from C bindings
    XdmfError::message(XdmfError::FATAL,
                       "Error: String type not usable from C.");
  }
  else
  {
    XdmfError::message(XdmfError::FATAL,
                       "Error: Invalid ArrayType.");
  }
  XDMF_ERROR_WRAP_END(status)
  return -1;
}

unsigned int XdmfArrayGetCapacity(XDMFARRAY * array)
{
  return ((XdmfArray *)(array))->getCapacity();
}

unsigned int *
XdmfArrayGetDimensions(XDMFARRAY * array)
{
  try
  {
    std::vector<unsigned int> tempVector = ((XdmfArray *)(array))->getDimensions();
    unsigned int returnSize = tempVector.size();
    unsigned int * returnArray = new unsigned int[returnSize]();
    for (unsigned int i = 0; i < returnSize; ++i) {
      returnArray[i] = tempVector[i];
    }
    return returnArray;
  }
  catch (...)
  {
    std::vector<unsigned int> tempVector = ((XdmfArray *)(array))->getDimensions();
    unsigned int returnSize = tempVector.size();
    unsigned int * returnArray = new unsigned int[returnSize]();
    for (unsigned int i = 0; i < returnSize; ++i) {
      returnArray[i] = tempVector[i];
    }
    return returnArray;
  }
}

char *
XdmfArrayGetDimensionsString(XDMFARRAY * array)
{
  try
  {
    char * returnPointer = strdup(((XdmfArray *)(array))->getDimensionsString().c_str());
    return returnPointer;
  }
  catch (...)
  {
    char * returnPointer = strdup(((XdmfArray *)(array))->getDimensionsString().c_str());
    return returnPointer;
  }
}

XDMFHEAVYDATACONTROLLER *
XdmfArrayGetHeavyDataController(XDMFARRAY * array, unsigned int index)
{
  return (XDMFHEAVYDATACONTROLLER *)((void *)(((XdmfArray *)(array))->getHeavyDataController(index).get()));
}

char *
XdmfArrayGetName(XDMFARRAY * array)
{
  try
  {
    char * returnPointer = strdup(((XdmfArray *)(array))->getName().c_str());
    return returnPointer;
  }
  catch (...)
  {
    char * returnPointer = strdup(((XdmfArray *)(array))->getName().c_str());
    return returnPointer;
  }
}

unsigned int
XdmfArrayGetNumberDimensions(XDMFARRAY * array)
{
  return ((XdmfArray *)(array))->getDimensions().size();
}

unsigned int
XdmfArrayGetNumberHeavyDataControllers(XDMFARRAY * array)
{
  return ((XdmfArray *)(array))->getNumberHeavyDataControllers();
}

unsigned int
XdmfArrayGetSize(XDMFARRAY * array)
{
  return ((XdmfArray *)(array))->getSize();
}

int
XdmfArrayGetReadMode(XDMFARRAY * array, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  int readMode = ((XdmfArray *)(array))->getReadMode();
  switch (readMode) {
    case XdmfArray::Controller:
      return XDMF_ARRAY_READ_MODE_CONTROLLER;
      break;
    case XdmfArray::Reference:
      return XDMF_ARRAY_READ_MODE_REFERENCE;
      break;
    default:
      XdmfError::message(XdmfError::FATAL,
                         "Error: Invalid ReadMode.");
      break;
  }
  XDMF_ERROR_WRAP_END(status)
  return -1;
}

XDMFARRAYREFERENCE *
XdmfArrayGetReference(XDMFARRAY * array)
{
  return (XDMFARRAYREFERENCE *)((void *)(((XdmfArray *)(array))->getReference().get()));
}

void *
XdmfArrayGetValue(XDMFARRAY * array, unsigned int index, int arrayType, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  try
  {
    void * returnVal;
    switch (arrayType) {
      case XDMF_ARRAY_TYPE_UINT8:
        returnVal = new unsigned char();
        *((unsigned char *)returnVal) = ((XdmfArray *)(array))->getValue<unsigned char>(index);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_UINT16:
        returnVal = new unsigned short();
        *((unsigned short *)returnVal) = ((XdmfArray *)(array))->getValue<unsigned short>(index);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_UINT32:
        returnVal = new unsigned int();
        *((unsigned int *)returnVal) = ((XdmfArray *)(array))->getValue<unsigned int>(index);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_UINT64:
        returnVal = new uint64_t();
        *((uint64_t *)returnVal) = ((XdmfArray *)(array))->getValue<uint64_t>(index);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_INT8:
        returnVal = new char();
        *((char *)returnVal) = ((XdmfArray *)(array))->getValue<char>(index);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_INT16:
        returnVal = new short();
        *((short *)returnVal) = ((XdmfArray *)(array))->getValue<short>(index);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_INT32:
        returnVal = new int();
        *((int *)returnVal) = ((XdmfArray *)(array))->getValue<int>(index);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_INT64:
        returnVal = new long();
        *((long *)returnVal) = ((XdmfArray *)(array))->getValue<long>(index);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_FLOAT32:
        returnVal = new float();
        *((float *)returnVal) = ((XdmfArray *)(array))->getValue<float>(index);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_FLOAT64:
        returnVal = new double();
        *((double *)returnVal) = ((XdmfArray *)(array))->getValue<double>(index);
        return returnVal;
        break;
      default:
        XdmfError::message(XdmfError::FATAL,
                           "Error: Invalid ArrayType.");
        break;
    }
  }
  catch (...)
  {
    void * returnVal;
    switch (arrayType) {
      case XDMF_ARRAY_TYPE_UINT8:
        returnVal = new unsigned char();
        *((unsigned char *)returnVal) = ((XdmfArray *)(array))->getValue<unsigned char>(index);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_UINT16:
        returnVal = new unsigned short();
        *((unsigned short *)returnVal) = ((XdmfArray *)(array))->getValue<unsigned short>(index);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_UINT32:
        returnVal = new unsigned int();
        *((unsigned int *)returnVal) = ((XdmfArray *)(array))->getValue<unsigned int>(index);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_UINT64:
        returnVal = new uint64_t();
        *((uint64_t *)returnVal) = ((XdmfArray *)(array))->getValue<uint64_t>(index);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_INT8:
        returnVal = new char();
        *((char *)returnVal) = ((XdmfArray *)(array))->getValue<char>(index);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_INT16:
        returnVal = new short();
        *((short *)returnVal) = ((XdmfArray *)(array))->getValue<short>(index);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_INT32:
        returnVal = new int();
        *((int *)returnVal) = ((XdmfArray *)(array))->getValue<int>(index);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_INT64:
        returnVal = new long();
        *((long *)returnVal) = ((XdmfArray *)(array))->getValue<long>(index);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_FLOAT32:
        returnVal = new float();
        *((float *)returnVal) = ((XdmfArray *)(array))->getValue<float>(index);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_FLOAT64:
        returnVal = new double();
        *((double *)returnVal) = ((XdmfArray *)(array))->getValue<double>(index);
        return returnVal;
        break;
      default:
        XdmfError::message(XdmfError::FATAL,
                           "Error: Invalid ArrayType.");
        break;
    }
  }
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

void *
XdmfArrayGetValues(XDMFARRAY * array, unsigned int startIndex, int arrayType, unsigned int numValues, unsigned int arrayStride, unsigned int valueStride, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  try
  {
    void * returnVal;
    switch (arrayType) {
      case XDMF_ARRAY_TYPE_UINT8:
        returnVal = new unsigned char[numValues]();
        ((XdmfArray *)(array))->getValues<unsigned char>(startIndex, (unsigned char *)returnVal, numValues, arrayStride, valueStride);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_UINT16:
        returnVal = new unsigned short[numValues]();
        ((XdmfArray *)(array))->getValues<unsigned short>(startIndex, (unsigned short *)returnVal, numValues, arrayStride, valueStride);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_UINT32:
        returnVal = new unsigned int[numValues]();
        ((XdmfArray *)(array))->getValues<unsigned int>(startIndex, (unsigned int *)returnVal, numValues, arrayStride, valueStride);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_UINT64:
        returnVal = new uint64_t[numValues]();
        ((XdmfArray *)(array))->getValues<uint64_t>(startIndex, (uint64_t *)returnVal, numValues, arrayStride, valueStride);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_INT8:
        returnVal = new char[numValues]();
        ((XdmfArray *)(array))->getValues<char>(startIndex, (char *)returnVal, numValues, arrayStride, valueStride);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_INT16:
        returnVal = new short[numValues]();
        ((XdmfArray *)(array))->getValues<short>(startIndex, (short *)returnVal, numValues, arrayStride, valueStride);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_INT32:
        returnVal = new int[numValues]();
        ((XdmfArray *)(array))->getValues<int>(startIndex, (int *)returnVal, numValues, arrayStride, valueStride);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_INT64:
        returnVal = new long[numValues]();
        ((XdmfArray *)(array))->getValues<long>(startIndex, (long *)returnVal, numValues, arrayStride, valueStride);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_FLOAT32:
        returnVal = new float[numValues]();
        ((XdmfArray *)(array))->getValues<float>(startIndex, (float *)returnVal, numValues, arrayStride, valueStride);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_FLOAT64:
        returnVal = new double[numValues]();
        ((XdmfArray *)(array))->getValues<double>(startIndex, (double *)returnVal, numValues, arrayStride, valueStride);
        return returnVal;
        break;
      default:
        XdmfError::message(XdmfError::FATAL,
                           "Error: Invalid ArrayType.");
        break;
    }
  }
  catch (...)
  {
    void * returnVal;
    switch (arrayType) {
      case XDMF_ARRAY_TYPE_UINT8:
        returnVal = new unsigned char[numValues]();
        ((XdmfArray *)(array))->getValues<unsigned char>(startIndex, (unsigned char *)returnVal, numValues, arrayStride, valueStride);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_UINT16:
        returnVal = new unsigned short[numValues]();
        ((XdmfArray *)(array))->getValues<unsigned short>(startIndex, (unsigned short *)returnVal, numValues, arrayStride, valueStride);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_UINT32:
        returnVal = new unsigned int[numValues]();
        ((XdmfArray *)(array))->getValues<unsigned int>(startIndex, (unsigned int *)returnVal, numValues, arrayStride, valueStride);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_UINT64:
        returnVal = new uint64_t[numValues]();
        ((XdmfArray *)(array))->getValues<uint64_t>(startIndex, (uint64_t *)returnVal, numValues, arrayStride, valueStride);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_INT8:
        returnVal = new char[numValues]();
        ((XdmfArray *)(array))->getValues<char>(startIndex, (char *)returnVal, numValues, arrayStride, valueStride);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_INT16:
        returnVal = new short[numValues]();
        ((XdmfArray *)(array))->getValues<short>(startIndex, (short *)returnVal, numValues, arrayStride, valueStride);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_INT32:
        returnVal = new int[numValues]();
        ((XdmfArray *)(array))->getValues<int>(startIndex, (int *)returnVal, numValues, arrayStride, valueStride);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_INT64:
        returnVal = new long[numValues]();
        ((XdmfArray *)(array))->getValues<long>(startIndex, (long *)returnVal, numValues, arrayStride, valueStride);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_FLOAT32:
        returnVal = new float[numValues]();
        ((XdmfArray *)(array))->getValues<float>(startIndex, (float *)returnVal, numValues, arrayStride, valueStride);
        return returnVal;
        break;
      case XDMF_ARRAY_TYPE_FLOAT64:
        returnVal = new double[numValues]();
        ((XdmfArray *)(array))->getValues<double>(startIndex, (double *)returnVal, numValues, arrayStride, valueStride);
        return returnVal;
        break;
      default:
        XdmfError::message(XdmfError::FATAL,
                           "Error: Invalid ArrayType.");
        break;
    }
  }
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

void *
XdmfArrayGetValuesInternal(XDMFARRAY * array)
{
  return ((XdmfArray *)(array))->getValuesInternal();
}

char *
XdmfArrayGetValuesString(XDMFARRAY * array)
{
  try
  {
    char * returnPointer = strdup(((XdmfArray *)(array))->getValuesString().c_str());
    return returnPointer;
  }
  catch (...)
  {
    char * returnPointer = strdup(((XdmfArray *)(array))->getValuesString().c_str());
    return returnPointer;
  }
}

void
XdmfArrayInitialize(XDMFARRAY * array, int * dims, int numDims, int arrayType, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  std::vector<unsigned int> dimVector((int *)dims, (int *)dims + numDims);
  shared_ptr<const XdmfArrayType> tempPointer = XdmfArrayType::Uninitialized();
  switch (arrayType) {
    case XDMF_ARRAY_TYPE_UINT8:
      tempPointer = XdmfArrayType::UInt8();
      break;
    case XDMF_ARRAY_TYPE_UINT16:
      tempPointer = XdmfArrayType::UInt16();
      break;
    case XDMF_ARRAY_TYPE_UINT32:
      tempPointer = XdmfArrayType::UInt32();
      break;
    case XDMF_ARRAY_TYPE_UINT64:
      tempPointer = XdmfArrayType::UInt64();
      break;
    case XDMF_ARRAY_TYPE_INT8:
      tempPointer = XdmfArrayType::Int8();
      break;
    case XDMF_ARRAY_TYPE_INT16:
      tempPointer = XdmfArrayType::Int16();
      break;
    case XDMF_ARRAY_TYPE_INT32:
      tempPointer = XdmfArrayType::Int32();
      break;
    case XDMF_ARRAY_TYPE_INT64:
      tempPointer = XdmfArrayType::Int64();
      break;
    case XDMF_ARRAY_TYPE_FLOAT32:
      tempPointer = XdmfArrayType::Float32();
      break;
    case XDMF_ARRAY_TYPE_FLOAT64:
      tempPointer = XdmfArrayType::Float64();
      break;
    default:
      XdmfError::message(XdmfError::FATAL,
                         "Error: Invalid ArrayType.");
      break;
  }
  ((XdmfArray *)(array))->initialize(tempPointer, dimVector);
  XDMF_ERROR_WRAP_END(status)
}

void
XdmfArrayInsertDataFromPointer(XDMFARRAY * array, void * values, int arrayType, unsigned int startIndex, unsigned int numVals, unsigned int arrayStride, unsigned int valueStride, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  try
  {
    switch (arrayType) {
      case XDMF_ARRAY_TYPE_UINT8:
        ((XdmfArray *)(array))->insert<unsigned char>(startIndex, (unsigned char *)values, numVals, arrayStride, valueStride);
        break;
      case XDMF_ARRAY_TYPE_UINT16:
        ((XdmfArray *)(array))->insert<unsigned short>(startIndex, (unsigned short *)values, numVals, arrayStride, valueStride);
        break;
      case XDMF_ARRAY_TYPE_UINT32:
        ((XdmfArray *)(array))->insert<unsigned int>(startIndex, (unsigned int *)values, numVals, arrayStride, valueStride);
        break;
      case XDMF_ARRAY_TYPE_UINT64:
        ((XdmfArray *)(array))->insert<uint64_t>(startIndex, (uint64_t *)values, numVals, arrayStride, valueStride);
        break;
      case XDMF_ARRAY_TYPE_INT8:
        ((XdmfArray *)(array))->insert<char>(startIndex, (char *)values, numVals, arrayStride, valueStride);
        break;
      case XDMF_ARRAY_TYPE_INT16:
        ((XdmfArray *)(array))->insert<short>(startIndex, (short *)values, numVals, arrayStride, valueStride);
        break;
      case XDMF_ARRAY_TYPE_INT32:
        ((XdmfArray *)(array))->insert<int>(startIndex, (int *)values, numVals, arrayStride, valueStride);
        break;
      case XDMF_ARRAY_TYPE_INT64:
        ((XdmfArray *)(array))->insert<long>(startIndex, (long *)values, numVals, arrayStride, valueStride);
        break;
      case XDMF_ARRAY_TYPE_FLOAT32:
        ((XdmfArray *)(array))->insert<float>(startIndex, (float *)values, numVals, arrayStride, valueStride);
        break;
      case XDMF_ARRAY_TYPE_FLOAT64:
        ((XdmfArray *)(array))->insert<double>(startIndex, (double *)values, numVals, arrayStride, valueStride);
        break;
      default:
        XdmfError::message(XdmfError::FATAL,
                           "Error: Invalid ArrayType.");
        break;
    }
  }
  catch (...)
  {
    switch (arrayType) {
      case XDMF_ARRAY_TYPE_UINT8:
        ((XdmfArray *)(array))->insert<unsigned char>(startIndex, (unsigned char *)values, numVals, arrayStride, valueStride);
        break;
      case XDMF_ARRAY_TYPE_UINT16:
        ((XdmfArray *)(array))->insert<unsigned short>(startIndex, (unsigned short *)values, numVals, arrayStride, valueStride);
        break;
      case XDMF_ARRAY_TYPE_UINT32:
        ((XdmfArray *)(array))->insert<unsigned int>(startIndex, (unsigned int *)values, numVals, arrayStride, valueStride);
        break;
      case XDMF_ARRAY_TYPE_UINT64:
        ((XdmfArray *)(array))->insert<uint64_t>(startIndex, (uint64_t *)values, numVals, arrayStride, valueStride);
        break;
      case XDMF_ARRAY_TYPE_INT8:
        ((XdmfArray *)(array))->insert<char>(startIndex, (char *)values, numVals, arrayStride, valueStride);
        break;
      case XDMF_ARRAY_TYPE_INT16:
        ((XdmfArray *)(array))->insert<short>(startIndex, (short *)values, numVals, arrayStride, valueStride);
        break;
      case XDMF_ARRAY_TYPE_INT32:
        ((XdmfArray *)(array))->insert<int>(startIndex, (int *)values, numVals, arrayStride, valueStride);
        break;
      case XDMF_ARRAY_TYPE_INT64:
        ((XdmfArray *)(array))->insert<long>(startIndex, (long *)values, numVals, arrayStride, valueStride);
        break;
      case XDMF_ARRAY_TYPE_FLOAT32:
        ((XdmfArray *)(array))->insert<float>(startIndex, (float *)values, numVals, arrayStride, valueStride);
        break;
      case XDMF_ARRAY_TYPE_FLOAT64:
        ((XdmfArray *)(array))->insert<double>(startIndex, (double *)values, numVals, arrayStride, valueStride);
        break;
      default:
        XdmfError::message(XdmfError::FATAL,
                           "Error: Invalid ArrayType.");
        break;
    }
  }
  XDMF_ERROR_WRAP_END(status)
}

void XdmfArrayInsertDataFromXdmfArray(XDMFARRAY * array, XDMFARRAY * valArray, int * arrayStarts, int * valueStarts, int * arrayCounts, int * valueCounts, int * arrayStrides, int * valueStrides, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  try
  {
    shared_ptr<XdmfArray> tempPointer((XdmfArray *)valArray, XdmfNullDeleter());
    std::vector<unsigned int> arrayStartVector((int *)arrayStarts, (int *)arrayStarts + ((XdmfArray *)(array))->getDimensions().size());
    std::vector<unsigned int> valueStartVector((int *)valueStarts, (int *)valueStarts + tempPointer->getDimensions().size());
    std::vector<unsigned int> arrayCountVector((int *)arrayCounts, (int *)arrayCounts + ((XdmfArray *)(array))->getDimensions().size());
    std::vector<unsigned int> valueCountVector((int *)valueCounts, (int *)valueCounts + tempPointer->getDimensions().size());
    std::vector<unsigned int> arrayStrideVector((int *)arrayStrides, (int *)arrayStrides + ((XdmfArray *)(array))->getDimensions().size());
    std::vector<unsigned int> valueStrideVector((int *)valueStrides, (int *)valueStrides + tempPointer->getDimensions().size());
    ((XdmfArray *)(array))->insert(arrayStartVector, tempPointer, valueStartVector, arrayCountVector, valueCountVector, arrayStrideVector, valueStrideVector);
  }
  catch (...)
  {
    shared_ptr<XdmfArray> tempPointer((XdmfArray *)valArray, XdmfNullDeleter());
    std::vector<unsigned int> arrayStartVector((int *)arrayStarts, (int *)arrayStarts + ((XdmfArray *)(array))->getDimensions().size());
    std::vector<unsigned int> valueStartVector((int *)valueStarts, (int *)valueStarts + tempPointer->getDimensions().size());
    std::vector<unsigned int> arrayCountVector((int *)arrayCounts, (int *)arrayCounts + ((XdmfArray *)(array))->getDimensions().size());
    std::vector<unsigned int> valueCountVector((int *)valueCounts, (int *)valueCounts + tempPointer->getDimensions().size());
    std::vector<unsigned int> arrayStrideVector((int *)arrayStrides, (int *)arrayStrides + ((XdmfArray *)(array))->getDimensions().size());
    std::vector<unsigned int> valueStrideVector((int *)valueStrides, (int *)valueStrides + tempPointer->getDimensions().size());
    ((XdmfArray *)(array))->insert(arrayStartVector, tempPointer, valueStartVector, arrayCountVector, valueCountVector, arrayStrideVector, valueStrideVector);
  }
  XDMF_ERROR_WRAP_END(status)
}

void
XdmfArrayInsertHeavyDataController(XDMFARRAY * array, XDMFHEAVYDATACONTROLLER * controller, int passControl)
{
  if (passControl == 0) {
    ((XdmfArray *)(array))->insert(shared_ptr<XdmfHeavyDataController>((XdmfHeavyDataController *) controller, XdmfNullDeleter()));
  }
  else {
    ((XdmfArray *)(array))->insert(shared_ptr<XdmfHeavyDataController>((XdmfHeavyDataController *) controller));
  }
}

void
XdmfArrayInsertValue(XDMFARRAY * array, unsigned int index, void * value, int arrayType, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  try
  {
    switch (arrayType) {
      case XDMF_ARRAY_TYPE_UINT8:
        ((XdmfArray *)(array))->insert(index, *((unsigned char *)value));
        break;
      case XDMF_ARRAY_TYPE_UINT16:
        ((XdmfArray *)(array))->insert(index, *((unsigned short *)value));
        break;
      case XDMF_ARRAY_TYPE_UINT32:
        ((XdmfArray *)(array))->insert(index, *((unsigned int *)value));
        break;
      case XDMF_ARRAY_TYPE_UINT64:
        ((XdmfArray *)(array))->insert(index, *((uint64_t *)value));
        break;
      case XDMF_ARRAY_TYPE_INT8:
        ((XdmfArray *)(array))->insert(index, *((char *)value));
        break;
      case XDMF_ARRAY_TYPE_INT16:
        ((XdmfArray *)(array))->insert(index, *((short *)value));
        break;
      case XDMF_ARRAY_TYPE_INT32:
        ((XdmfArray *)(array))->insert(index, *((int *)value));
        break;
      case XDMF_ARRAY_TYPE_INT64:
        ((XdmfArray *)(array))->insert(index, *((long *)value));
        break;
      case XDMF_ARRAY_TYPE_FLOAT32:
        ((XdmfArray *)(array))->insert(index, *((float *)value));
        break;
      case XDMF_ARRAY_TYPE_FLOAT64:
        ((XdmfArray *)(array))->insert(index, *((double *)value));
        break;
      default:
        XdmfError::message(XdmfError::FATAL,
                           "Error: Invalid ArrayType.");
        break;
    }
  }
  catch (...)
  {
    switch (arrayType) {
      case XDMF_ARRAY_TYPE_UINT8:
        ((XdmfArray *)(array))->insert(index, *((unsigned char *)value));
        break;
      case XDMF_ARRAY_TYPE_UINT16:
        ((XdmfArray *)(array))->insert(index, *((unsigned short *)value));
        break;
      case XDMF_ARRAY_TYPE_UINT32:
        ((XdmfArray *)(array))->insert(index, *((unsigned int *)value));
        break;
      case XDMF_ARRAY_TYPE_UINT64:
        ((XdmfArray *)(array))->insert(index, *((uint64_t *)value));
        break;
      case XDMF_ARRAY_TYPE_INT8:
        ((XdmfArray *)(array))->insert(index, *((char *)value));
        break;
      case XDMF_ARRAY_TYPE_INT16:
        ((XdmfArray *)(array))->insert(index, *((short *)value));
        break;
      case XDMF_ARRAY_TYPE_INT32:
        ((XdmfArray *)(array))->insert(index, *((int *)value));
        break;
      case XDMF_ARRAY_TYPE_INT64:
        ((XdmfArray *)(array))->insert(index, *((long *)value));
        break;
      case XDMF_ARRAY_TYPE_FLOAT32:
        ((XdmfArray *)(array))->insert(index, *((float *)value));
        break;
      case XDMF_ARRAY_TYPE_FLOAT64:
        ((XdmfArray *)(array))->insert(index, *((double *)value));
        break;
      default:
        XdmfError::message(XdmfError::FATAL,
                           "Error: Invalid ArrayType.");
        break;
    }
  }
  XDMF_ERROR_WRAP_END(status)
}

int
XdmfArrayIsInitialized(XDMFARRAY * array)
{
  return ((XdmfArray *)(array))->isInitialized();
}

void
XdmfArrayPushBack(XDMFARRAY * array, void * value, int arrayType, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  try
  {
    switch (arrayType) {
      case XDMF_ARRAY_TYPE_UINT8:
        ((XdmfArray *)(array))->pushBack<unsigned char>(*((unsigned char *)value));
        break;
      case XDMF_ARRAY_TYPE_UINT16:
        ((XdmfArray *)(array))->pushBack<unsigned short>(*((unsigned short *)value));
        break;
      case XDMF_ARRAY_TYPE_UINT32:
        ((XdmfArray *)(array))->pushBack<unsigned int>(*((unsigned int *)value));
        break;
      case XDMF_ARRAY_TYPE_UINT64:
        ((XdmfArray *)(array))->pushBack<uint64_t>(*((uint64_t *)value));
        break;
      case XDMF_ARRAY_TYPE_INT8:
        ((XdmfArray *)(array))->pushBack<char>(*((char *)value));
        break;
      case XDMF_ARRAY_TYPE_INT16:
        ((XdmfArray *)(array))->pushBack<short>(*((short *)value));
        break;
      case XDMF_ARRAY_TYPE_INT32:
        ((XdmfArray *)(array))->pushBack<int>(*((int *)value));
        break;
      case XDMF_ARRAY_TYPE_INT64:
        ((XdmfArray *)(array))->pushBack<long>(*((long *)value));
        break;
      case XDMF_ARRAY_TYPE_FLOAT32:
        ((XdmfArray *)(array))->pushBack<float>(*((float *)value));
        break;
      case XDMF_ARRAY_TYPE_FLOAT64:
        ((XdmfArray *)(array))->pushBack<double>(*((double *)value));
        break;
      default:
        XdmfError::message(XdmfError::FATAL,
                           "Error: Invalid ArrayType.");
        break;
    }
  }
  catch (...)
  {
    switch (arrayType) {
      case XDMF_ARRAY_TYPE_UINT8:
        ((XdmfArray *)(array))->pushBack<unsigned char>(*((unsigned char *)value));
        break;
      case XDMF_ARRAY_TYPE_UINT16:
        ((XdmfArray *)(array))->pushBack<unsigned short>(*((unsigned short *)value));
        break;
      case XDMF_ARRAY_TYPE_UINT32:
        ((XdmfArray *)(array))->pushBack<unsigned int>(*((unsigned int *)value));
        break;
      case XDMF_ARRAY_TYPE_UINT64:
        ((XdmfArray *)(array))->pushBack<uint64_t>(*((uint64_t *)value));
        break;
      case XDMF_ARRAY_TYPE_INT8:
        ((XdmfArray *)(array))->pushBack<char>(*((char *)value));
        break;
      case XDMF_ARRAY_TYPE_INT16:
        ((XdmfArray *)(array))->pushBack<short>(*((short *)value));
        break;
      case XDMF_ARRAY_TYPE_INT32:
        ((XdmfArray *)(array))->pushBack<int>(*((int *)value));
        break;
      case XDMF_ARRAY_TYPE_INT64:
        ((XdmfArray *)(array))->pushBack<long>(*((long *)value));
        break;
      case XDMF_ARRAY_TYPE_FLOAT32:
        ((XdmfArray *)(array))->pushBack<float>(*((float *)value));
        break;
      case XDMF_ARRAY_TYPE_FLOAT64:
        ((XdmfArray *)(array))->pushBack<double>(*((double *)value));
        break;
      default:
        XdmfError::message(XdmfError::FATAL,
                           "Error: Invalid ArrayType.");
        break;
    }
  }
  XDMF_ERROR_WRAP_END(status)
}

void
XdmfArrayRead(XDMFARRAY * array, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  try
  {
    ((XdmfArray *)(array))->read();
  }
  catch (...)
  {
    ((XdmfArray *)(array))->read();
  }
  XDMF_ERROR_WRAP_END(status)
}

void
XdmfArrayReadController(XDMFARRAY * array, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  try
  {
    ((XdmfArray *)(array))->readController();
  }
  catch (...)
  {
    ((XdmfArray *)(array))->readController();
  }
  XDMF_ERROR_WRAP_END(status)
}

void
XdmfArrayReadReference(XDMFARRAY * array, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  try
  {
    ((XdmfArray *)(array))->readReference();
  }
  catch (...)
  {
    ((XdmfArray *)(array))->readReference();
  }
  XDMF_ERROR_WRAP_END(status)
}

void
XdmfArrayRelease(XDMFARRAY * array)
{
  ((XdmfArray *)(array))->release();
}

void
XdmfArrayRemoveHeavyDataController(XDMFARRAY * array, unsigned int index)
{
  ((XdmfArray *)(array))->removeHeavyDataController(index);
}

void
XdmfArrayReserve(XDMFARRAY * array, int size)
{
  ((XdmfArray *)(array))->reserve(size);
}

void
XdmfArrayResize(XDMFARRAY * array, int * dims, int numDims, int arrayType, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  try
  {
    std::vector<unsigned int> dimVector((int *)dims, (int *)dims + numDims);
    switch (arrayType) {
      case XDMF_ARRAY_TYPE_UINT8:
      {
        XdmfArray * classedArray = (XdmfArray *)((void *) array);
        classedArray->resize(dimVector, (unsigned char) 0);
        break;
      }
      case XDMF_ARRAY_TYPE_UINT16:
      {
        XdmfArray * classedArray = (XdmfArray *)((void *) array);
        classedArray->resize(dimVector, (unsigned short) 0);
        break;
      }
      case XDMF_ARRAY_TYPE_UINT32:
      {
        XdmfArray * classedArray = (XdmfArray *)((void *) array);
        classedArray->resize(dimVector, (unsigned int) 0);
        break;
      }
      case XDMF_ARRAY_TYPE_UINT64:
      {
        XdmfArray * classedArray = (XdmfArray *)((void *) array);
        classedArray->resize(dimVector, (uint64_t) 0);
        break;
      }
      case XDMF_ARRAY_TYPE_INT8:
      {
        XdmfArray * classedArray = (XdmfArray *)((void *) array);
        classedArray->resize(dimVector, (char) 0);
        break;
      }
      case XDMF_ARRAY_TYPE_INT16:
      {
        XdmfArray * classedArray = (XdmfArray *)((void *) array);
        classedArray->resize(dimVector, (short) 0);
        break;
      }
      case XDMF_ARRAY_TYPE_INT32:
      {
        XdmfArray * classedArray = (XdmfArray *)((void *) array);
        classedArray->resize(dimVector, (int) 0);
        break;
      }
      case XDMF_ARRAY_TYPE_INT64:
      {
        XdmfArray * classedArray = (XdmfArray *)((void *) array);
        classedArray->resize(dimVector, (long) 0);
        break;
      }
      case XDMF_ARRAY_TYPE_FLOAT32:
      {
        XdmfArray * classedArray = (XdmfArray *)((void *) array);
        classedArray->resize(dimVector, (float) 0);
        break;
      }
      case XDMF_ARRAY_TYPE_FLOAT64:
      {
        XdmfArray * classedArray = (XdmfArray *)((void *) array);
        classedArray->resize(dimVector, (double) 0);
        break;
      }
      default:
      {
        XdmfError::message(XdmfError::FATAL,
                           "Error: Invalid ArrayType.");
        break;
      }
    }
    dimVector.clear();
  }
  catch (...)
  {
    std::vector<unsigned int> dimVector((int *)dims, (int *)dims + numDims);
    switch (arrayType) {
      case XDMF_ARRAY_TYPE_UINT8:
      {
        XdmfArray * classedArray = (XdmfArray *)((void *) array);
        classedArray->resize(dimVector, (unsigned char) 0);
        break;
      }
      case XDMF_ARRAY_TYPE_UINT16:
      {
        XdmfArray * classedArray = (XdmfArray *)((void *) array);
        classedArray->resize(dimVector, (unsigned short) 0);
        break;
      }
      case XDMF_ARRAY_TYPE_UINT32:
      {
        XdmfArray * classedArray = (XdmfArray *)((void *) array);
        classedArray->resize(dimVector, (unsigned int) 0);
        break;
      }
      case XDMF_ARRAY_TYPE_UINT64:
      {
        XdmfArray * classedArray = (XdmfArray *)((void *) array);
        classedArray->resize(dimVector, (uint64_t) 0);
        break;
      }
      case XDMF_ARRAY_TYPE_INT8:
      {
        XdmfArray * classedArray = (XdmfArray *)((void *) array);
        classedArray->resize(dimVector, (char) 0);
        break;
      }
      case XDMF_ARRAY_TYPE_INT16:
      {
        XdmfArray * classedArray = (XdmfArray *)((void *) array);
        classedArray->resize(dimVector, (short) 0);
        break;
      }
      case XDMF_ARRAY_TYPE_INT32:
      {
        XdmfArray * classedArray = (XdmfArray *)((void *) array);
        classedArray->resize(dimVector, (int) 0);
        break;
      }
      case XDMF_ARRAY_TYPE_INT64:
      {
        XdmfArray * classedArray = (XdmfArray *)((void *) array);
        classedArray->resize(dimVector, (long) 0);
        break;
      }
      case XDMF_ARRAY_TYPE_FLOAT32:
      {
        XdmfArray * classedArray = (XdmfArray *)((void *) array);
        classedArray->resize(dimVector, (float) 0);
        break;
      }

      case XDMF_ARRAY_TYPE_FLOAT64:
      {
        XdmfArray * classedArray = (XdmfArray *)((void *) array);
        classedArray->resize(dimVector, (double) 0);
        break;
      }
      default:
      {
        XdmfError::message(XdmfError::FATAL,
                           "Error: Invalid ArrayType.");
        break;
      }
    }
    dimVector.clear();
  }
  XDMF_ERROR_WRAP_END(status)
}

void
XdmfArraySetReadMode(XDMFARRAY * array, int readMode, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  switch (readMode) {
    case XDMF_ARRAY_READ_MODE_CONTROLLER:
      ((XdmfArray *)(array))->setReadMode(XdmfArray::Controller);
      break;
    case XDMF_ARRAY_READ_MODE_REFERENCE:
      ((XdmfArray *)(array))->setReadMode(XdmfArray::Reference);
      break;
    default:
      XdmfError::message(XdmfError::FATAL,
                         "Error: Invalid ReadMode.");
      break;
  }
  XDMF_ERROR_WRAP_END(status)
}

void
XdmfArraySetReference(XDMFARRAY * array, XDMFARRAYREFERENCE * reference, int passControl)
{
  if (passControl) {
    ((XdmfArray *)(array))->setReference(shared_ptr<XdmfArrayReference>((XdmfArrayReference *)reference));
  }
  else {
    ((XdmfArray *)(array))->setReference(shared_ptr<XdmfArrayReference>((XdmfArrayReference *)reference, XdmfNullDeleter()));
  }
}

void
XdmfArraySetName(XDMFARRAY * array, char * name, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfArray *)(array))->setName(name);
  XDMF_ERROR_WRAP_END(status)
}

void
XdmfArraySetValuesInternal(XDMFARRAY * array, void * pointer, unsigned int numValues, int arrayType, int transferOwnership, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  switch (arrayType) {
    case XDMF_ARRAY_TYPE_UINT8:
      ((XdmfArray *)array)->setValuesInternal((unsigned char *)pointer, numValues, transferOwnership);
      break;
    case XDMF_ARRAY_TYPE_UINT16:
      ((XdmfArray *)array)->setValuesInternal((unsigned short *)pointer, numValues, transferOwnership);
      break;
    case XDMF_ARRAY_TYPE_UINT32:
      ((XdmfArray *)array)->setValuesInternal((unsigned int *)pointer, numValues, transferOwnership);
      break;
    case XDMF_ARRAY_TYPE_UINT64:
      ((XdmfArray *)array)->setValuesInternal((uint64_t *)pointer, numValues, transferOwnership);
      break;
    case XDMF_ARRAY_TYPE_INT8:
      ((XdmfArray *)array)->setValuesInternal((char *)pointer, numValues, transferOwnership);
      break;
    case XDMF_ARRAY_TYPE_INT16:
      ((XdmfArray *)array)->setValuesInternal((short *)pointer, numValues, transferOwnership);
     break;
    case XDMF_ARRAY_TYPE_INT32:
      ((XdmfArray *)array)->setValuesInternal((int *)pointer, numValues, transferOwnership);
      break;
    case XDMF_ARRAY_TYPE_INT64:
      ((XdmfArray *)array)->setValuesInternal((long *)pointer, numValues, transferOwnership);
      break;
    case XDMF_ARRAY_TYPE_FLOAT32:
      ((XdmfArray *)array)->setValuesInternal((float *)pointer, numValues, transferOwnership);
      break;
    case XDMF_ARRAY_TYPE_FLOAT64:
      ((XdmfArray *)array)->setValuesInternal((double *)pointer, numValues, transferOwnership);
      break;
    default:
      XdmfError::message(XdmfError::FATAL,
                         "Error: Invalid ArrayType.");
      break;
  }
  XDMF_ERROR_WRAP_END(status)
}

void
XdmfArraySwapWithXdmfArray(XDMFARRAY * array, XDMFARRAY * swapArray)
{
  shared_ptr<XdmfArray> pointerToSwap((XdmfArray *) swapArray, XdmfNullDeleter());
  ((XdmfArray *)array)->swap(pointerToSwap);
}

void
XdmfArraySwapWithArray(XDMFARRAY * array, void ** pointer, int numValues, int arrayType, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  switch (arrayType) {
    case XDMF_ARRAY_TYPE_UINT8: {
      std::vector<unsigned char> swapVector((unsigned char *)(*pointer), (unsigned char *)(*pointer) + numValues);
      ((XdmfArray *)array)->swap(swapVector);
      *pointer = new unsigned char[swapVector.size()];
      for (unsigned int i = 0; i < swapVector.size(); ++i)
      {
        ((unsigned char *) (*pointer))[i] = swapVector[i];
      }
      break;
    }
    case XDMF_ARRAY_TYPE_UINT16: {
      std::vector<unsigned short> swapVector((unsigned short *)(*pointer), (unsigned short *)(*pointer) + numValues);
      ((XdmfArray *)array)->swap(swapVector);
      *pointer = new unsigned short[swapVector.size()];
      for (unsigned int i = 0; i < swapVector.size(); ++i)
      {
        ((unsigned short *) (*pointer))[i] = swapVector[i];
      }
      break;
    }
    case XDMF_ARRAY_TYPE_UINT32: {
      std::vector<unsigned int> swapVector((unsigned int *)(*pointer), (unsigned int *)(*pointer) + numValues);
      ((XdmfArray *)array)->swap(swapVector);
      *pointer = new unsigned int[swapVector.size()];
      for (unsigned int i = 0; i < swapVector.size(); ++i)
      {
        ((unsigned int *) (*pointer))[i] = swapVector[i];
      }
      break;
    }
    case XDMF_ARRAY_TYPE_UINT64: {
      std::vector<uint64_t> swapVector((uint64_t *)(*pointer), (uint64_t *)(*pointer) + numValues);
      ((XdmfArray *)array)->swap(swapVector);
      *pointer = new uint64_t[swapVector.size()];
      for (unsigned int i = 0; i < swapVector.size(); ++i)
      {
        ((uint64_t *) (*pointer))[i] = swapVector[i];
      }
      break;
    }
    case XDMF_ARRAY_TYPE_INT8: {
      std::vector<char> swapVector((char *)(*pointer), (char *)(*pointer) + numValues);
      ((XdmfArray *)array)->swap(swapVector);
      *pointer = new char[swapVector.size()];
      for (unsigned int i = 0; i < swapVector.size(); ++i)
      {
        ((char *) (*pointer))[i] = swapVector[i];
      }
      break;
    }
    case XDMF_ARRAY_TYPE_INT16: {
      std::vector<short> swapVector((short *)(*pointer), (short *)(*pointer) + numValues);
      ((XdmfArray *)array)->swap(swapVector);
      *pointer = new short[swapVector.size()];
      for (unsigned int i = 0; i < swapVector.size(); ++i)
      {
        ((short *) (*pointer))[i] = swapVector[i];
      }
      break;
    }
    case XDMF_ARRAY_TYPE_INT32: {
      std::vector<int> swapVector((int *)(*pointer), (int *)(*pointer) + numValues);
      ((XdmfArray *)array)->swap(swapVector);
      *pointer = new int[swapVector.size()];
      for (unsigned int i = 0; i < swapVector.size(); ++i)
      {
        ((int *) (*pointer))[i] = swapVector[i];
      }
      break;
    }
    case XDMF_ARRAY_TYPE_INT64: {
      std::vector<long> swapVector((long *)(*pointer), (long *)(*pointer) + numValues);
      ((XdmfArray *)array)->swap(swapVector);
      *pointer = new long[swapVector.size()];
      for (unsigned int i = 0; i < swapVector.size(); ++i)
      {
        ((long *) (*pointer))[i] = swapVector[i];
      }
      break;
    }
    case XDMF_ARRAY_TYPE_FLOAT32: {
      std::vector<float> swapVector((float *)(*pointer), (float *)(*pointer) + numValues);
      ((XdmfArray *)array)->swap(swapVector);
      *pointer = new float[swapVector.size()];
      for (unsigned int i = 0; i < swapVector.size(); ++i)
      {
        ((float *) (*pointer))[i] = swapVector[i];
      }
      break;
    }
    case XDMF_ARRAY_TYPE_FLOAT64: {
      std::vector<double> swapVector((double *)(*pointer), (double *)(*pointer) + numValues);
      ((XdmfArray *)array)->swap(swapVector);
      *pointer = new double[swapVector.size()];
      for (unsigned int i = 0; i < swapVector.size(); ++i)
      {
        ((double *) (*pointer))[i] = swapVector[i];
      }
      break;
    }
    default:
      XdmfError::message(XdmfError::FATAL,
                         "Error: Invalid ArrayType.");
      break;
  }
  XDMF_ERROR_WRAP_END(status)
}

// C Wrappers for parent classes are generated by macros

XDMF_ITEM_C_CHILD_WRAPPER(XdmfArray, XDMFARRAY)
