/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfInformation.cpp                                                 */
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

#include <utility>
#include "string.h"
#include "XdmfArray.hpp"
#include "XdmfError.hpp"
#include "XdmfInformation.hpp"

XDMF_CHILDREN_IMPLEMENTATION(XdmfInformation, XdmfArray, Array, Name)

shared_ptr<XdmfInformation>
XdmfInformation::New()
{
  shared_ptr<XdmfInformation> p(new XdmfInformation());
  return p;
}

shared_ptr<XdmfInformation>
XdmfInformation::New(const std::string & key,
                     const std::string & value)
{
  shared_ptr<XdmfInformation> p(new XdmfInformation(key, value));
  return p;
}

XdmfInformation::XdmfInformation(const std::string & key,
                                 const std::string & value) :
  mKey(key),
  mValue(value)
{
}

XdmfInformation::XdmfInformation(XdmfInformation & refInfo) :
  XdmfItem(refInfo),
  mArrays(refInfo.mArrays)
{
  mKey = refInfo.getKey();
  mValue = refInfo.getValue();
}

XdmfInformation::~XdmfInformation()
{
}

const std::string XdmfInformation::ItemTag = "Information";

std::map<std::string, std::string>
XdmfInformation::getItemProperties() const
{
  std::map<std::string, std::string> informationProperties;
  informationProperties.insert(std::make_pair("Name", mKey));
  informationProperties.insert(std::make_pair("Value", mValue));
  return informationProperties;
}

std::string
XdmfInformation::getItemTag() const
{
  return ItemTag;
}

std::string
XdmfInformation::getKey() const
{
  return mKey;
}

std::string
XdmfInformation::getValue() const
{
  return mValue;
}

void
XdmfInformation::populateItem(const std::map<std::string, std::string> & itemProperties,
                              const std::vector<shared_ptr<XdmfItem> > & childItems,
                              const XdmfCoreReader * const reader)
{
  XdmfItem::populateItem(itemProperties, childItems, reader);

  std::map<std::string, std::string>::const_iterator key =
    itemProperties.find("Name");
  if(key != itemProperties.end()) {
    mKey = key->second;
  }
  else {
    XdmfError::message(XdmfError::FATAL,
                       "'Name' not found in itemProperties in "
                       "XdmfInformation::populateItem");
  }

  std::map<std::string, std::string>::const_iterator value =
    itemProperties.find("Value");
  if(value != itemProperties.end()) {
    mValue = value->second;
  }
  else {
    value = itemProperties.find("Content");
    if(value != itemProperties.end()) {
      mValue = value->second;
    }
    else {
      XdmfError::message(XdmfError::FATAL,
                         "'Value' not found in itemProperties in "
                         "XdmfInformation::populateItem");
    }
  }
  for(std::vector<shared_ptr<XdmfItem> >::const_iterator iter =
        childItems.begin();
      iter != childItems.end();
      ++iter) {
    if(shared_ptr<XdmfArray> array = shared_dynamic_cast<XdmfArray>(*iter)) {
      this->insert(array);
    }
  }
}

void
XdmfInformation::setKey(const std::string & key)
{
  mKey = key;
  this->setIsChanged(true);
}

void
XdmfInformation::setValue(const std::string & value)
{
  mValue = value;
  this->setIsChanged(true);
}

void
XdmfInformation::traverse(const shared_ptr<XdmfBaseVisitor> visitor)
{
  XdmfItem::traverse(visitor);
  for (unsigned int i = 0; i < mArrays.size(); ++i)
  {
    mArrays[i]->accept(visitor);
  }
}

// C Wrappers

XDMFINFORMATION *
XdmfInformationNew(char * key, char * value)
{
  try
  {
    std::string createKey(key);
    std::string createValue(value);
    shared_ptr<XdmfInformation> generatedInfo = XdmfInformation::New(createKey, createValue);
    return (XDMFINFORMATION *)((void *)(new XdmfInformation(*generatedInfo.get())));
  }
  catch (...)
  {
    std::string createKey(key);
    std::string createValue(value);
    shared_ptr<XdmfInformation> generatedInfo = XdmfInformation::New(createKey, createValue);
    return (XDMFINFORMATION *)((void *)(new XdmfInformation(*generatedInfo.get())));
  }
}

XDMFARRAY *
XdmfInformationGetArray(XDMFINFORMATION * information, unsigned int index)
{
  return (XDMFARRAY *)((void *)(((XdmfInformation *)(information))->getArray(index).get()));
}

XDMFARRAY *
XdmfInformationGetArrayByName(XDMFINFORMATION * information, char * name)
{
  return (XDMFARRAY *)((void *)(((XdmfInformation *)(information))->getArray(name).get()));
}

char *
XdmfInformationGetKey(XDMFINFORMATION * information)
{
  try
  {
    XdmfInformation referenceInfo = *(XdmfInformation *)(information);
    char * returnPointer = strdup(referenceInfo.getKey().c_str());
    return returnPointer;
  }
  catch (...)
  {
    XdmfInformation referenceInfo = *(XdmfInformation *)(information);
    char * returnPointer = strdup(referenceInfo.getKey().c_str());
    return returnPointer;
  }
}

unsigned int
XdmfInformationGetNumberArrays(XDMFINFORMATION * information)
{
  return ((XdmfInformation *)(information))->getNumberArrays();
}

char *
XdmfInformationGetValue(XDMFINFORMATION * information)
{
  try
  {
    XdmfInformation referenceInfo = *(XdmfInformation *)(information);
    char * returnPointer = strdup(referenceInfo.getValue().c_str());
    return returnPointer;
  }
  catch (...)
  { 
    XdmfInformation referenceInfo = *(XdmfInformation *)(information);
    char * returnPointer = strdup(referenceInfo.getValue().c_str());
    return returnPointer;
  }
}

void
XdmfInformationInsertArray(XDMFINFORMATION * information, XDMFARRAY * array, int transferOwnership)
{
  if (transferOwnership) {
    ((XdmfInformation *)(information))->insert(shared_ptr<XdmfArray>((XdmfArray *)array));
  }
  else {
    ((XdmfInformation *)(information))->insert(shared_ptr<XdmfArray>((XdmfArray *)array, XdmfNullDeleter()));
  }
}

void
XdmfInformationRemoveArray(XDMFINFORMATION * information, unsigned int index)
{
  ((XdmfInformation *)(information))->removeArray(index);
}

void
XdmfInformationRemoveArrayByName(XDMFINFORMATION * information, char * name)
{
  ((XdmfInformation *)(information))->removeArray(name);
}

void
XdmfInformationSetKey(XDMFINFORMATION * information, char * key, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfInformation *)(information))->setKey(key);
  XDMF_ERROR_WRAP_END(status)
}

void
XdmfInformationSetValue(XDMFINFORMATION * information, char * value, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfInformation *)(information))->setValue(value);
  XDMF_ERROR_WRAP_END(status)
}

// C Wrappers for parent classes are generated by macros

XDMF_ITEM_C_CHILD_WRAPPER(XdmfInformation, XDMFINFORMATION)
