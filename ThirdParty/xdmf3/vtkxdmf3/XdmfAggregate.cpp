/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfAggregate.cpp                                                   */
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
#include "XdmfArray.hpp"
#include "XdmfError.hpp"
#include "XdmfAggregate.hpp"
#include "XdmfVisitor.hpp"
#include "XdmfWriter.hpp"
#include "string.h"

XDMF_CHILDREN_IMPLEMENTATION(XdmfAggregate, XdmfArray, Array, Name)

XdmfAggregate::XdmfAggregate()
{
}

XdmfAggregate::~XdmfAggregate()
{
}

const std::string XdmfAggregate::ItemTag = "Aggregate";

shared_ptr<XdmfAggregate>
XdmfAggregate::New()
{
  shared_ptr<XdmfAggregate> p(new XdmfAggregate());
  return p;
}

std::vector<unsigned int> XdmfAggregate::getDimensions() const
{
  std::vector<unsigned int> testDims = mArrays[0]->getDimensions();

  bool isSame = true;

  for(std::vector<shared_ptr<XdmfArray> >::const_iterator iter =
        mArrays.begin();
      iter != mArrays.end() && isSame;
      ++iter) {
    std::vector<unsigned int> compareDims = (*iter)->getDimensions();
    if (compareDims.size() == testDims.size())
    {
      for (unsigned int i = 0; i < testDims.size(); ++i)
      {
        if (compareDims[i] != testDims[i])
        {
          isSame = false;
          break;
        }
      }
    }
    else
    {
      isSame = false;
      break;
    }
  }

  if (isSame)
  {
    testDims.push_back(mArrays.size());
    return testDims;
  }
  else
  {
    std::vector<unsigned int> returnDims;
    returnDims.push_back(this->getSize());
    return returnDims;
  }
}

std::map<std::string, std::string>
XdmfAggregate::getItemProperties() const
{
  std::map<std::string, std::string> aggregateMap = XdmfArrayReference::getItemProperties();

  return aggregateMap;
}

std::string
XdmfAggregate::getItemTag() const
{
  return ItemTag;
}

unsigned int
XdmfAggregate::getSize() const
{
  unsigned int total = 0;
  for(std::vector<shared_ptr<XdmfArray> >::const_iterator iter =
        mArrays.begin();
      iter != mArrays.end();
      ++iter) {
    total += (*iter)->getSize();
  }
  return total;
}

void
XdmfAggregate::populateItem(const std::map<std::string, std::string> & itemProperties,
                            const std::vector<shared_ptr<XdmfItem> > & childItems,
                            const XdmfCoreReader * const reader)
{
  bool placeholderFound = false;
  for(std::vector<shared_ptr<XdmfItem> >::const_iterator iter =
        childItems.begin();
      iter != childItems.end();
      ++iter) {
    if(shared_ptr<XdmfArray> array = shared_dynamic_cast<XdmfArray>(*iter)) {
      if (!placeholderFound) {
        placeholderFound = true;
      }
      else {
        this->insert(array);
      }
/*
      this->swap(array);
      if (array->getReference()) {
        this->setReference(array->getReference());
        this->setReadMode(XdmfArray::Reference);
      }
      break;
*/
    }
  }
}

shared_ptr<XdmfArray>
XdmfAggregate::read() const
{
  shared_ptr<XdmfArray> returnArray = XdmfArray::New();

  if (mArrays.size() > 0)
  {
    if (!mArrays[0]->isInitialized()) {
      mArrays[0]->read();
    }
    returnArray->insert(0, mArrays[0], 0, mArrays[0]->getSize(),  1, 1);
    if (mArrays.size() > 1)
    {
      unsigned int offset = mArrays[0]->getSize();
      for (unsigned int i = 1; i < mArrays.size(); ++i)
      {
        if (!mArrays[i]->isInitialized()) {
          mArrays[i]->read();
        }
        returnArray->insert(offset, mArrays[i], 0, mArrays[i]->getSize(), 1, 1);
        offset += mArrays[i]->getSize();
      }
    }
  }

  return returnArray;
}

void
XdmfAggregate::traverse(const shared_ptr<XdmfBaseVisitor> visitor)
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

  for (unsigned int i = 0; i < mArrays.size(); ++i)
  {
    mArrays[i]->accept(visitor);
  }
}

// C Wrappers

XDMFAGGREGATE * XdmfAggregateNew()
{
  shared_ptr<XdmfAggregate> * p = new shared_ptr<XdmfAggregate>(XdmfAggregate::New());
  return (XDMFAGGREGATE *) p;
}

XDMFARRAY *
XdmfAggregateGetArray(XDMFAGGREGATE * aggregate, unsigned int index)
{
  shared_ptr<XdmfAggregate> & refAggregate = *(shared_ptr<XdmfAggregate> *)(aggregate);
  shared_ptr<XdmfArray> * p = new shared_ptr<XdmfArray>(refAggregate->getArray(index));
  return (XDMFARRAY *) p;
}

XDMFARRAY *
XdmfAggregateGetArrayByName(XDMFAGGREGATE * aggregate, char * name)
{
  shared_ptr<XdmfAggregate> & refAggregate = *(shared_ptr<XdmfAggregate> *)(aggregate);
  shared_ptr<XdmfArray> * p = new shared_ptr<XdmfArray>(refAggregate->getArray(name));
  return (XDMFARRAY *) p;
}

unsigned int
XdmfAggregateGetNumberArrays(XDMFAGGREGATE * aggregate)
{
  shared_ptr<XdmfAggregate> & refAggregate = *(shared_ptr<XdmfAggregate> *)(aggregate);
  return refAggregate->getNumberArrays();
}

void
XdmfAggregateInsertArray(XDMFAGGREGATE * aggregate, XDMFARRAY * array, int transferOwnership)
{
  shared_ptr<XdmfAggregate> & refAggregate = *(shared_ptr<XdmfAggregate> *)(aggregate);
  shared_ptr<XdmfArray> & refArray = *(shared_ptr<XdmfArray> *)(array);
  refAggregate->insert(refArray);
}

void
XdmfAggregateRemoveArray(XDMFAGGREGATE * aggregate, unsigned int index)
{
  shared_ptr<XdmfAggregate> & refAggregate = *(shared_ptr<XdmfAggregate> *)(aggregate);
  refAggregate->removeArray(index);
}

void
XdmfAggregateRemoveArrayByName(XDMFAGGREGATE * aggregate, char * name)
{
  shared_ptr<XdmfAggregate> & refAggregate = *(shared_ptr<XdmfAggregate> *)(aggregate);
  refAggregate->removeArray(name);
}

// C Wrappers for parent classes are generated by macros

XDMF_ITEM_C_CHILD_WRAPPER(XdmfAggregate, XDMFAGGREGATE)
XDMF_ARRAYREFERENCE_C_CHILD_WRAPPER(XdmfAggregate, XDMFAGGREGATE)
