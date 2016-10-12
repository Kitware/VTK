/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfSubset.cpp                                                      */
/*                                                                           */
/*  Author:                                                                  */
/*     Andrew Burns                                                          */
/*     andrew.j.burns2@us.army.mil                                           */
/*     US Army Research Laboratory                                           */
/*     Aberdeen Proving Ground, MD                                           */
/*                                                                           */
/*     Copyright @ 2013 US Army Research Laboratory                          */
/*     All Rights Reserved                                                   */
/*     See Copyright.txt for details                                         */
/*                                                                           */
/*     This software is distributed WITHOUT ANY WARRANTY; without            */
/*     even the implied warranty of MERCHANTABILITY or FITNESS               */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice             */
/*     for more information.                                                 */
/*                                                                           */
/*****************************************************************************/

#include <numeric>
#include <functional>
#include <boost/tokenizer.hpp>
#include "string.h"
#include "XdmfArray.hpp"
#include "XdmfError.hpp"
#include "XdmfSubset.hpp"
#include "XdmfWriter.hpp"

XdmfSubset::XdmfSubset(shared_ptr<XdmfArray> referenceArray,
                       std::vector<unsigned int> & start,
                       std::vector<unsigned int> & stride,
                       std::vector<unsigned int> & dimensions) :
  mParent(referenceArray),
  mDimensions(dimensions),
  mStart(start),
  mStride(stride)
{
  if(!(mStart.size() == mStride.size() &&
       mStride.size() == mDimensions.size())) {
    XdmfError::message(XdmfError::FATAL,
                       "mStart, mStride, mDimensions must all be of equal "
                       "length in XdmfSubset constructor");
  }
}

XdmfSubset::XdmfSubset(XdmfSubset & refSubset) :
  XdmfArrayReference(refSubset),
  mParent(refSubset.getReferenceArray()),
  mDimensions(refSubset.getDimensions()),
  mStart(refSubset.getStart()),
  mStride(refSubset.getStride())
{
}

XdmfSubset::~XdmfSubset()
{
}

const std::string XdmfSubset::ItemTag = "Subset";

shared_ptr<XdmfSubset>
XdmfSubset::New(shared_ptr<XdmfArray> referenceArray,
                std::vector<unsigned int> & start,
                std::vector<unsigned int> & stride,
                std::vector<unsigned int> & dimensions)
{
  shared_ptr<XdmfSubset> p(new XdmfSubset(referenceArray, start, stride, dimensions));
  return p;
}

std::vector<unsigned int> XdmfSubset::getDimensions() const
{
  return mDimensions;
}

std::map<std::string, std::string>
XdmfSubset::getItemProperties() const
{
  // Check to make sure the subset is valid
  // before generating the properties.
  if(!(mStart.size() == mStride.size() &&
       mStride.size() == mDimensions.size())) {
    XdmfError::message(XdmfError::FATAL,
                       "mStart, mStride, mDimensions must all be of equal "
                       "length in XdmfSubset getItemProperties");
  }

  if (mStart.size() < 1 ||
      mStride.size() < 1 ||
      mDimensions.size() < 1) {
    XdmfError::message(XdmfError::WARNING,
                       "mStart, mStride, mDimensions must have at least "
                       "one value contained within");
  }

  std::map<std::string, std::string> subsetMap = XdmfArrayReference::getItemProperties();

  std::stringstream vectorStream;

  vectorStream << mStart[0];

  for (unsigned int i = 1; i < mStart.size(); ++i) {
    vectorStream << " " << mStart[i];
  }

  subsetMap["SubsetStarts"] = vectorStream.str();

  vectorStream.str(std::string());

  vectorStream << mStride[0];

  for (unsigned int i = 1; i < mStride.size(); ++i) {
    vectorStream << " " << mStride[i];
  }

  subsetMap["SubsetStrides"] = vectorStream.str();

  vectorStream.str(std::string());

  vectorStream << mDimensions[0];

  for (unsigned int i = 1; i < mDimensions.size(); ++i) {
    vectorStream << " " << mDimensions[i];
  }

  subsetMap["SubsetDimensions"] = vectorStream.str();

  return subsetMap;
}

std::string
XdmfSubset::getItemTag() const
{
  return ItemTag;
}

shared_ptr<XdmfArray>
XdmfSubset::getReferenceArray()
{
  if (mParent) {
    return mParent;
  }
  else {
    return shared_ptr<XdmfArray>();
  }
}

unsigned int
XdmfSubset::getSize() const
{
  return std::accumulate(mDimensions.begin(),
                         mDimensions.end(),
                         1,
                         std::multiplies<unsigned int>());
}

std::vector<unsigned int>
XdmfSubset::getStart() const
{
  return mStart;
}

std::vector<unsigned int>
XdmfSubset::getStride() const
{
  return mStride;
}

void
XdmfSubset::populateItem(const std::map<std::string, std::string> & itemProperties,
             const std::vector<shared_ptr<XdmfItem> > & childItems,
             const XdmfCoreReader * const reader)
{
  std::map<std::string, std::string>::const_iterator starts =
    itemProperties.find("SubsetStarts");

  boost::tokenizer<> tokens(starts->second);
  for(boost::tokenizer<>::const_iterator iter = tokens.begin();
      iter != tokens.end();
      ++iter) {
    mStart.push_back(atoi((*iter).c_str()));
  }

  std::map<std::string, std::string>::const_iterator strides =
    itemProperties.find("SubsetStrides");

  boost::tokenizer<> stridetokens(strides->second);
  for(boost::tokenizer<>::const_iterator iter = stridetokens.begin();
      iter != stridetokens.end();
      ++iter) {
    mStride.push_back(atoi((*iter).c_str()));
  }

  std::map<std::string, std::string>::const_iterator dimensions =
    itemProperties.find("SubsetDimensions");

  boost::tokenizer<> dimtokens(dimensions->second);
  for(boost::tokenizer<>::const_iterator iter = dimtokens.begin();
      iter != dimtokens.end();
      ++iter) {
    mDimensions.push_back(atoi((*iter).c_str()));
  }

  mParent = shared_dynamic_cast<XdmfArray>(childItems[0]);
}

shared_ptr<XdmfArray>
XdmfSubset::read() const
{
  if (mStart.size() < 1 ||
      mStride.size() < 1 ||
      mDimensions.size() < 1) {
    XdmfError::message(XdmfError::WARNING,
                       "mStart, mStride, mDimensions must have at least "
                       "one value contained within");
  }

  if (!mParent->isInitialized()) {
    mParent->read();
  }

  shared_ptr<XdmfArray> tempArray = XdmfArray::New();
  tempArray->initialize(mParent->getArrayType());
  tempArray->resize(this->getSize(), 0);
  std::vector<unsigned int> writeStarts;
  writeStarts.push_back(0);
  std::vector<unsigned int> writeStrides;
  writeStrides.push_back(1);
  std::vector<unsigned int> writeDimensions;
  writeDimensions.push_back(this->getSize());

  tempArray->insert(writeStarts,
                    mParent,
                    mStart,
                    mDimensions,
                    writeDimensions,
                    writeStrides,
                    mStride);
  return tempArray;
}

void
XdmfSubset::setDimensions(std::vector<unsigned int> newDimensions)
{
  mDimensions = newDimensions;
  // Give the user a warning so they know they might have messed something up.
  // If they don't want the warning they can suppress it.
  if(!(mStart.size() == mStride.size() &&
       mStride.size() == mDimensions.size())) {
    XdmfError::message(XdmfError::WARNING,
                       "mStart, mStride, mDimensions now have different sizes."
                       "The sizes should be equal before use.");
  }
  this->setIsChanged(true);
}

void
XdmfSubset::setReferenceArray(shared_ptr<XdmfArray> newReference)
{
  mParent = newReference;
  this->setIsChanged(true);
}

void
XdmfSubset::setStart(std::vector<unsigned int> newStarts)
{
  mStart = newStarts;
  // Give the user a warning so they know they might have messed something up.
  // If they don't want the warning they can suppress it.
  if(!(mStart.size() == mStride.size() &&
       mStride.size() == mDimensions.size())) {
    XdmfError::message(XdmfError::WARNING,
                       "mStart, mStride, mDimensions now have different sizes."
                       "The sizes should be equal before use.");
  }
  this->setIsChanged(true);
}

void
XdmfSubset::setStride(std::vector<unsigned int> newStrides)
{
  mStride = newStrides;
  // Give the user a warning so they know they might have messed something up.
  // If they don't want the warning they can suppress it.
  if(!(mStart.size() == mStride.size() &&
       mStride.size() == mDimensions.size())) {
    XdmfError::message(XdmfError::WARNING,
                       "mStart, mStride, mDimensions now have different sizes."
                       "The sizes should be equal before use.");
  }
  this->setIsChanged(true);
}

void
XdmfSubset::traverse(const shared_ptr<XdmfBaseVisitor> visitor)
{
  XdmfItem::traverse(visitor);

  bool originalXPath;

  if (shared_ptr<XdmfWriter> writer =
        shared_dynamic_cast<XdmfWriter>(visitor)) {
    originalXPath = writer->getWriteXPaths();
    writer->setWriteXPaths(false);
  }

  shared_ptr<XdmfArray> spacerarray = XdmfArray::New();
  spacerarray->pushBack((int)0);
  spacerarray->accept(visitor);

  if (shared_ptr<XdmfWriter> writer =
        shared_dynamic_cast<XdmfWriter>(visitor)) {
    writer->setWriteXPaths(originalXPath);
  }

  mParent->accept(visitor);
}

// C Wrappers

XDMFSUBSET * XdmfSubsetNew(void * referenceArray, unsigned int * start, unsigned int * stride, unsigned int * dimensions, unsigned int numDims, int passControl, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  try
  {
    std::vector<unsigned int> startVector(start, start + numDims);
    std::vector<unsigned int> strideVector(stride, stride + numDims);
    std::vector<unsigned int> dimVector(dimensions, dimensions + numDims);
    shared_ptr<XdmfArray> referencePointer;
    if (passControl) {
      referencePointer = shared_ptr<XdmfArray>((XdmfArray *)referenceArray);
    }
    else {
      referencePointer = shared_ptr<XdmfArray>((XdmfArray *)referenceArray, XdmfNullDeleter());
    }
    shared_ptr<XdmfSubset> generatedSubset = XdmfSubset::New(referencePointer, startVector, strideVector, dimVector);
    return (XDMFSUBSET *)((void *)(new XdmfSubset(*generatedSubset.get())));
  }
  catch (...)
  {
    std::vector<unsigned int> startVector(start, start + numDims);
    std::vector<unsigned int> strideVector(stride, stride + numDims);
    std::vector<unsigned int> dimVector(dimensions, dimensions + numDims);
    shared_ptr<XdmfArray> referencePointer;
    if (passControl) {
      referencePointer = shared_ptr<XdmfArray>((XdmfArray *)referenceArray);
    }
    else {
      referencePointer = shared_ptr<XdmfArray>((XdmfArray *)referenceArray, XdmfNullDeleter());
    }
    shared_ptr<XdmfSubset> generatedSubset = XdmfSubset::New(referencePointer, startVector, strideVector, dimVector);
    return (XDMFSUBSET *)((void *)(new XdmfSubset(*generatedSubset.get())));
  }
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

unsigned int * XdmfSubsetGetDimensions(XDMFSUBSET * subset)
{
  try
  {
    std::vector<unsigned int> tempVector = ((XdmfSubset *)(subset))->getDimensions();
    unsigned int returnSize = tempVector.size();
    unsigned int * returnArray = new unsigned int[returnSize]();
    for (unsigned int i = 0; i < returnSize; ++i) {
      returnArray[i] = tempVector[i];
    }
    return returnArray;
  }
  catch (...)
  {
    std::vector<unsigned int> tempVector = ((XdmfSubset *)(subset))->getDimensions();
    unsigned int returnSize = tempVector.size();
    unsigned int * returnArray = new unsigned int[returnSize]();
    for (unsigned int i = 0; i < returnSize; ++i) {
      returnArray[i] = tempVector[i];
    }
    return returnArray;
  }
}

unsigned int XdmfSubsetGetNumberDimensions(XDMFSUBSET * subset)
{
  return ((XdmfSubset *)(subset))->getDimensions().size();
}

void * XdmfSubsetGetReferenceArray(XDMFSUBSET * subset)
{
  shared_ptr<XdmfArray> returnItem = ((XdmfSubset *)subset)->getReferenceArray();
  return returnItem.get();
}

unsigned int XdmfSubsetGetSize(XDMFSUBSET * subset)
{
  return ((XdmfSubset *)(subset))->getSize();
}

unsigned int * XdmfSubsetGetStart(XDMFSUBSET * subset)
{
  try
  {
    std::vector<unsigned int> tempVector = ((XdmfSubset *)(subset))->getStart();
    unsigned int returnSize = tempVector.size();
    unsigned int * returnArray = new unsigned int[returnSize]();
    for (unsigned int i = 0; i < returnSize; ++i) {
      returnArray[i] = tempVector[i];
    }
    return returnArray;
  }
  catch (...)
  {
    std::vector<unsigned int> tempVector = ((XdmfSubset *)(subset))->getStart();
    unsigned int returnSize = tempVector.size();
    unsigned int * returnArray = new unsigned int[returnSize]();
    for (unsigned int i = 0; i < returnSize; ++i) {
      returnArray[i] = tempVector[i];
    }
    return returnArray;
  }
}

unsigned int * XdmfSubsetGetStride(XDMFSUBSET * subset)
{
  try
  {
    std::vector<unsigned int> tempVector = ((XdmfSubset *)(subset))->getStride();
    unsigned int returnSize = tempVector.size();
    unsigned int * returnArray = new unsigned int[returnSize]();
    for (unsigned int i = 0; i < returnSize; ++i) {
      returnArray[i] = tempVector[i];
    }
    return returnArray;
  }
  catch (...)
  {
    std::vector<unsigned int> tempVector = ((XdmfSubset *)(subset))->getStride();
    unsigned int returnSize = tempVector.size();
    unsigned int * returnArray = new unsigned int[returnSize]();
    for (unsigned int i = 0; i < returnSize; ++i) {
      returnArray[i] = tempVector[i];
    }
    return returnArray;
  }
}

void XdmfSubsetSetDimensions(XDMFSUBSET * subset, unsigned int * newDimensions, unsigned int numDims, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  std::vector<unsigned int> dimVector(newDimensions, newDimensions + numDims);
  ((XdmfSubset *)(subset))->setDimensions(dimVector);
  XDMF_ERROR_WRAP_END(status)
}

void XdmfSubsetSetReferenceArray(XDMFSUBSET * subset, XDMFARRAY * referenceArray, int passControl)
{
  shared_ptr<XdmfArray> referencePointer;
  if (passControl) {
    referencePointer = shared_ptr<XdmfArray>((XdmfArray *)referenceArray);
  }
  else {
    referencePointer = shared_ptr<XdmfArray>((XdmfArray *)referenceArray, XdmfNullDeleter());
  }
  ((XdmfSubset *)subset)->setReferenceArray(referencePointer);
}

void XdmfSubsetSetStart(XDMFSUBSET * subset, unsigned int * newStarts, unsigned int numDims, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  std::vector<unsigned int> startVector(newStarts, newStarts + numDims);
  ((XdmfSubset *)(subset))->setStart(startVector);
  XDMF_ERROR_WRAP_END(status)
}

void XdmfSubsetSetStride(XDMFSUBSET * subset, unsigned int * newStrides, unsigned int numDims, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  std::vector<unsigned int> strideVector(newStrides, newStrides + numDims);
  ((XdmfSubset *)(subset))->setStride(strideVector);
  XDMF_ERROR_WRAP_END(status)
}

// C Wrappers for parent classes are generated by macros

XDMF_ITEM_C_CHILD_WRAPPER(XdmfSubset, XDMFSUBSET)
XDMF_ARRAYREFERENCE_C_CHILD_WRAPPER(XdmfSubset, XDMFSUBSET)
