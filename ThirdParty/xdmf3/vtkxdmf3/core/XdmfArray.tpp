/*****************************************************************************/
/*                                    Xdmf                                   */
/*                       Extensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfArray.tpp                                                       */
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

#include <functional>
#include <numeric>
#include <sstream>
#include "XdmfArray.hpp"

template <typename T>
class XdmfArray::GetValue : public boost::static_visitor<T> {
public:

  GetValue(const unsigned int index) :
    mIndex(index)
  {
  }

  T
  operator()(const boost::blank &) const
  {
    return 0;
  }

  T
  operator()(const shared_ptr<std::vector<std::string> > & array) const
  {
    return (T)atof(array->operator[](mIndex).c_str());
  }

  template<typename U>
  T
  operator()(const shared_ptr<std::vector<U> > & array) const
  {
    return (T)array->operator[](mIndex);
  }

  template<typename U>
  T
  operator()(const boost::shared_array<const U> & array) const
  {
    return (T)array[mIndex];
  }

private:

  const unsigned int mIndex;
};

template <>
class XdmfArray::GetValue<std::string> :
  public boost::static_visitor<std::string> {
public:

  GetValue(const unsigned int index) :
    mIndex(index)
  {
  }

  std::string
  operator()(const boost::blank &) const
  {
    return "";
  }

  std::string
  operator()(const shared_ptr<std::vector<std::string> > & array) const
  {
    return array->operator[](mIndex);
  }

  template<typename U>
  std::string
  operator()(const shared_ptr<std::vector<U> > & array) const
  {
    std::stringstream value;
    value << array->operator[](mIndex);
    return value.str();
  }

  std::string
  operator()(const boost::shared_array<const std::string> & array) const
  {
    return array[mIndex];
  }

  template<typename U>
  std::string
  operator()(const boost::shared_array<const U> & array) const
  {
    std::stringstream value;
    value << array[mIndex];
    return value.str();
  }

private:

  const unsigned int mIndex;
};

template <typename T>
class XdmfArray::GetValues : public boost::static_visitor<void> {
public:

  GetValues(const unsigned int startIndex,
            T * valuesPointer,
            const unsigned int numValues,
            const unsigned int arrayStride,
            const unsigned int valuesStride) :
    mStartIndex(startIndex),
    mValuesPointer(valuesPointer),
    mNumValues(numValues),
    mArrayStride(arrayStride),
    mValuesStride(valuesStride)
  {
  }

  void
  operator()(const boost::blank &) const
  {
    return;
  }

  void
  operator()(const shared_ptr<std::vector<std::string> > & array) const
  {
    for(unsigned int i=0; i<mNumValues; ++i) {
      mValuesPointer[i*mValuesStride] =
        (T)atof(array->operator[](mStartIndex + i*mArrayStride).c_str());
    }
  }

  template<typename U>
  void
  operator()(const shared_ptr<std::vector<U> > & array) const
  {
    for(unsigned int i=0; i<mNumValues; ++i) {
      mValuesPointer[i*mValuesStride] =
        (T)array->operator[](mStartIndex + i*mArrayStride);
    }
  }

  template<typename U>
  void
  operator()(const boost::shared_array<const U> & array) const
  {
    for(unsigned int i=0; i<mNumValues; ++i) {
      mValuesPointer[i*mValuesStride] = (T)array[mStartIndex + i*mArrayStride];
    }
  }

private:

  const unsigned int mStartIndex;
  T * mValuesPointer;
  const unsigned int mNumValues;
  const unsigned int mArrayStride;
  const unsigned int mValuesStride;
};

template <>
class XdmfArray::GetValues<std::string> : public boost::static_visitor<void> {
public:

  GetValues(const unsigned int startIndex,
            std::string * valuesPointer,
            const unsigned int numValues,
            const unsigned int arrayStride,
            const unsigned int valuesStride) :
    mStartIndex(startIndex),
    mValuesPointer(valuesPointer),
    mNumValues(numValues),
    mArrayStride(arrayStride),
    mValuesStride(valuesStride)
  {
  }

  void
  operator()(const boost::blank &) const
  {
    return;
  }

  template<typename U>
  void
  operator()(const shared_ptr<std::vector<U> > & array) const
  {
    for(unsigned int i=0; i<mNumValues; ++i) {
      std::stringstream value;
      value << array->operator[](mStartIndex + i*mArrayStride);
      mValuesPointer[i*mValuesStride] = value.str();
    }
  }

  template<typename U>
  void
  operator()(const boost::shared_array<const U> & array) const
  {
    for(unsigned int i=0; i<mNumValues; ++i) {
      std::stringstream value;
      value << array[mStartIndex + i*mArrayStride];
      mValuesPointer[i*mValuesStride] = value.str();
    }
  }

private:

  const unsigned int mStartIndex;
  std::string * mValuesPointer;
  const unsigned int mNumValues;
  const unsigned int mArrayStride;
  const unsigned int mValuesStride;
};

template <typename T>
class XdmfArray::Insert : public boost::static_visitor<void> {
public:

  Insert(XdmfArray * const array,
         const unsigned int startIndex,
         const T * const valuesPointer,
         const unsigned int numValues,
         const unsigned int arrayStride,
         const unsigned int valuesStride,
         std::vector<unsigned int> & dimensions) :
    mArray(array),
    mStartIndex(startIndex),
    mValuesPointer(valuesPointer),
    mNumValues(numValues),
    mArrayStride(arrayStride),
    mValuesStride(valuesStride),
    mDimensions(dimensions)
  {
  }

  void
  operator()(const boost::blank &) const
  {
    mArray->initialize<T>();
    boost::apply_visitor(*this,
                         mArray->mArray);
  }

  void
  operator()(shared_ptr<std::vector<std::string> > & array) const
  {
    unsigned int size = mStartIndex + (mNumValues - 1) * mArrayStride + 1;
    if(array->size() < size) {
      array->resize(size);
      mDimensions.clear();
    }
    for(unsigned int i=0; i<mNumValues; ++i) {
      std::stringstream value;
      value << mValuesPointer[i*mValuesStride];
      array->operator[](mStartIndex + i*mArrayStride) = value.str();
    }
  }

  template<typename U>
  void
  operator()(shared_ptr<std::vector<U> > & array) const
  {
    unsigned int size = mStartIndex + (mNumValues - 1) * mArrayStride + 1;
    if(array->size() < size) {
      array->resize(size);
      mDimensions.clear();
    }
    for(unsigned int i=0; i<mNumValues; ++i) {
      array->operator[](mStartIndex + i*mArrayStride) =
        (U)mValuesPointer[i*mValuesStride];
    }
  }

  template<typename U>
  void
  operator()(boost::shared_array<const U> &) const
  {
    mArray->internalizeArrayPointer();
    boost::apply_visitor(*this,
                         mArray->mArray);
  }

private:

  XdmfArray * const mArray;
  const unsigned int mStartIndex;
  const T * const mValuesPointer;
  const unsigned int mNumValues;
  const unsigned int mArrayStride;
  const unsigned int mValuesStride;
  std::vector<unsigned int> & mDimensions;
};

template <>
class XdmfArray::Insert<std::string> : public boost::static_visitor<void> {
public:

  Insert(XdmfArray * const array,
         const unsigned int startIndex,
         const std::string * const valuesPointer,
         const unsigned int numValues,
         const unsigned int arrayStride,
         const unsigned int valuesStride,
         std::vector<unsigned int> & dimensions) :
    mArray(array),
    mStartIndex(startIndex),
    mValuesPointer(valuesPointer),
    mNumValues(numValues),
    mArrayStride(arrayStride),
    mValuesStride(valuesStride),
    mDimensions(dimensions)
  {
  }

  void
  operator()(const boost::blank &) const
  {
    mArray->initialize<std::string>();
    boost::apply_visitor(*this,
                         mArray->mArray);
  }

  void
  operator()(shared_ptr<std::vector<std::string> > & array) const
  {
    unsigned int size = mStartIndex + (mNumValues - 1) * mArrayStride + 1;
    if(array->size() < size) {
      array->resize(size);
      mDimensions.clear();
    }
    for(unsigned int i=0; i<mNumValues; ++i) {
      array->operator[](mStartIndex + i*mArrayStride) =
        mValuesPointer[i*mValuesStride].c_str();
    }
  }

  template<typename U>
  void
  operator()(shared_ptr<std::vector<U> > & array) const
  {
    unsigned int size = mStartIndex + (mNumValues - 1) * mArrayStride + 1;
    if(array->size() < size) {
      array->resize(size);
      mDimensions.clear();
    }
    for(unsigned int i=0; i<mNumValues; ++i) {
      array->operator[](mStartIndex + i*mArrayStride) =
        (U)atof(mValuesPointer[i*mValuesStride].c_str());
    }
  }

  template<typename U>
  void
  operator()(boost::shared_array<const U> &) const
  {
    mArray->internalizeArrayPointer();
    boost::apply_visitor(*this,
                         mArray->mArray);
  }

private:

  XdmfArray * const mArray;
  const unsigned int mStartIndex;
  const std::string * const mValuesPointer;
  const unsigned int mNumValues;
  const unsigned int mArrayStride;
  const unsigned int mValuesStride;
  std::vector<unsigned int> & mDimensions;
};

template <typename T>
class XdmfArray::PushBack : public boost::static_visitor<void> {
public:

  PushBack(const T & val,
           XdmfArray * const array) :
    mVal(val),
    mArray(array)
  {
  }

  void
  operator()(const boost::blank &) const
  {
    mArray->initialize<T>();
    boost::apply_visitor(*this,
                         mArray->mArray);
  }

  void
  operator()(shared_ptr<std::vector<std::string> > & array) const
  {
    std::stringstream value;
    value << mVal;
    array->push_back(value.str());
    mArray->mDimensions.clear();
  }

  template<typename U>
  void
  operator()(shared_ptr<std::vector<U> > & array) const
  {
    array->push_back((U)mVal);
    mArray->mDimensions.clear();
  }

  template<typename U>
  void
  operator()(const boost::shared_array<const U> &) const
  {
    mArray->internalizeArrayPointer();
    boost::apply_visitor(*this,
                         mArray->mArray);
  }

private:

  const T & mVal;
  XdmfArray * const mArray;
};

template <>
class XdmfArray::PushBack<std::string> : public boost::static_visitor<void> {
public:

  PushBack(const std::string & val,
           XdmfArray * const array) :
    mVal(val),
    mArray(array)
  {
  }

  void
  operator()(const boost::blank &) const
  {
    mArray->initialize<std::string>();
    boost::apply_visitor(*this,
                         mArray->mArray);
  }

  void
  operator()(shared_ptr<std::vector<std::string> > & array) const
  {
    array->push_back(mVal);
    mArray->mDimensions.clear();
  }

  template<typename U>
  void
  operator()(shared_ptr<std::vector<U> > & array) const
  {
    array->push_back((U)atof(mVal.c_str()));
    mArray->mDimensions.clear();
  }

  template<typename U>
  void
  operator()(const boost::shared_array<const U> &) const
  {
    mArray->internalizeArrayPointer();
    boost::apply_visitor(*this,
                         mArray->mArray);
  }

private:

  const std::string & mVal;
  XdmfArray * const mArray;
};

template <typename T>
class XdmfArray::Resize : public boost::static_visitor<void> {
public:

  Resize(XdmfArray * const array,
         const unsigned int numValues,
         const T & val) :
    mArray(array),
    mNumValues(numValues),
    mVal(val)
  {
  }

  void
  operator()(const boost::blank &) const
  {
    mArray->initialize<T>();
    boost::apply_visitor(*this,
                         mArray->mArray);
  }

  void
  operator()(shared_ptr<std::vector<std::string> > & array) const
  {
    std::stringstream value;
    value << mVal;
    array->resize(mNumValues, value.str());
    mArray->mDimensions.clear();
  }

  template<typename U>
  void
  operator()(shared_ptr<std::vector<U> > & array) const
  {
    array->resize(mNumValues, (U)mVal);
    mArray->mDimensions.clear();
  }

  template<typename U>
  void
  operator()(const boost::shared_array<const U> &) const
  {
    mArray->internalizeArrayPointer();
    boost::apply_visitor(*this,
                         mArray->mArray);
  }

private:

  XdmfArray * mArray;
  const unsigned int mNumValues;
  const T & mVal;
};

template <>
class XdmfArray::Resize<std::string> : public boost::static_visitor<void> {
public:

  Resize(XdmfArray * const array,
         const unsigned int numValues,
         const std::string & val) :
    mArray(array),
    mNumValues(numValues),
    mVal(val)
  {
  }

  void
  operator()(const boost::blank &) const
  {
    mArray->initialize<std::string>();
    boost::apply_visitor(*this,
                         mArray->mArray);
  }

  void
  operator()(shared_ptr<std::vector<std::string> > & array) const
  {
    array->resize(mNumValues, mVal);
    mArray->mDimensions.clear();
  }

  template<typename U>
  void
  operator()(shared_ptr<std::vector<U> > & array) const
  {
    array->resize(mNumValues, (U)atof(mVal.c_str()));
    mArray->mDimensions.clear();
  }

  template<typename U>
  void
  operator()(const boost::shared_array<const U> &) const
  {
    mArray->internalizeArrayPointer();
    boost::apply_visitor(*this,
                         mArray->mArray);
  }

private:

  XdmfArray * mArray;
  const unsigned int mNumValues;
  const std::string & mVal;
};

struct XdmfArray::NullDeleter
{
  void
  operator()(void const *) const
  {
  }
};

template <typename T>
T
XdmfArray::getValue(const unsigned int index) const
{
  return boost::apply_visitor(GetValue<T>(index),
                              mArray);
}

template <typename T>
void
XdmfArray::getValues(const unsigned int startIndex,
                     T * const valuesPointer,
                     const unsigned int numValues,
                     const unsigned int arrayStride,
                     const unsigned int valuesStride) const
{
  boost::apply_visitor(GetValues<T>(startIndex,
                                    valuesPointer,
                                    numValues,
                                    arrayStride,
                                    valuesStride),
                       mArray);
}

template <typename T>
shared_ptr<std::vector<T> >
XdmfArray::getValuesInternal()
{
  this->internalizeArrayPointer();
  try {
    shared_ptr<std::vector<T> > currArray =
      boost::get<shared_ptr<std::vector<T> > >(mArray);
    return currArray;
  }
  catch(const boost::bad_get & exception) {
    return shared_ptr<std::vector<T> >();
  }
}

template <typename T>
shared_ptr<std::vector<T> >
XdmfArray::initialize(const unsigned int size)
{
  // Set type of variant to type of pointer
  shared_ptr<std::vector<T> > newArray(new std::vector<T>(size));
  if(mTmpReserveSize > 0) {
    newArray->reserve(mTmpReserveSize);
    mTmpReserveSize = 0;
  }
  mArray = newArray;
  this->setIsChanged(true);
  return newArray;
}

template <typename T>
shared_ptr<std::vector<T> >
XdmfArray::initialize(const std::vector<unsigned int> & dimensions)
{
  mDimensions = dimensions;
  const unsigned int size = static_cast<unsigned int>(
    std::accumulate(dimensions.begin(),
                                            dimensions.end(),
                                            1,
                    std::multiplies<unsigned int>()));
  return this->initialize<T>(size);
}

template<typename T>
void
XdmfArray::insert(const unsigned int index,
                  const T & value)
{
  boost::apply_visitor(Insert<T>(this,
                                 index,
                                 &value,
                                 1,
                                 0,
                                 0,
                                 mDimensions),
                       mArray);
}

template <typename T>
void
XdmfArray::insert(const unsigned int startIndex,
                  const T * const valuesPointer,
                  const unsigned int numValues,
                  const unsigned int arrayStride,
                  const unsigned int valuesStride)
{
  boost::apply_visitor(Insert<T>(this,
                                 startIndex,
                                 valuesPointer,
                                 numValues,
                                 arrayStride,
                                 valuesStride,
                                 mDimensions),
                       mArray);
  this->setIsChanged(true);
}

template <typename T>
void
XdmfArray::pushBack(const T & value)
{
  return boost::apply_visitor(PushBack<T>(value,
                                          this),
                              mArray);
  this->setIsChanged(true);
}

template<typename T>
void
XdmfArray::resize(const unsigned int numValues,
                  const T & value)
{
  return boost::apply_visitor(Resize<T>(this,
                                        numValues,
                                        value),
                              mArray);
  this->setIsChanged(true);
}

template<typename T>
void
XdmfArray::resize(const std::vector<unsigned int> & dimensions,
                  const T & value)
{
  const unsigned int size = static_cast<unsigned int>(
    std::accumulate(dimensions.begin(),
                                            dimensions.end(),
                                            1,
                    std::multiplies<unsigned int>()));
  this->resize(size, value);
  mDimensions = dimensions;
  this->setIsChanged(true);
}

template <typename T>
void
XdmfArray::setValuesInternal(const T * const arrayPointer,
                             const unsigned int numValues,
                             const bool transferOwnership)
{
  // Remove contents of internal array.
  if(transferOwnership) {
    const boost::shared_array<const T> newArrayPointer(arrayPointer);
    mArray = newArrayPointer;
  }
  else {
    const boost::shared_array<const T> newArrayPointer(arrayPointer,
                                                       NullDeleter());
    mArray = newArrayPointer;
  }
  mArrayPointerNumValues = numValues;
  this->setIsChanged(true);
}

template <typename T>
void
XdmfArray::setValuesInternal(std::vector<T> & array,
                             const bool transferOwnership)
{
  if(transferOwnership) {
    shared_ptr<std::vector<T> > newArray(&array);
    mArray = newArray;
  }
  else {
    shared_ptr<std::vector<T> > newArray(&array, NullDeleter());
    mArray = newArray;
  }
  this->setIsChanged(true);
}

template <typename T>
void
XdmfArray::setValuesInternal(const shared_ptr<std::vector<T> > array)
{
  mArray = array;
  this->setIsChanged(true);
}

template <typename T>
bool
XdmfArray::swap(std::vector<T> & array)
{
  this->internalizeArrayPointer();
  if(!this->isInitialized()) {
    this->initialize<T>();
  }
  try {
    shared_ptr<std::vector<T> > currArray =
      boost::get<shared_ptr<std::vector<T> > >(mArray);
    currArray->swap(array);
    return true;
  }
  catch(const boost::bad_get & exception) {
    return false;
  }
  this->setIsChanged(true);
}

template <typename T>
bool
XdmfArray::swap(const shared_ptr<std::vector<T> > array)
{
  return this->swap(*array.get());
}
