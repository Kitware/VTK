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
}

void
XdmfInformation::setValue(const std::string & value)
{
  mValue = value;
}

void
XdmfInformation::traverse(const shared_ptr<XdmfBaseVisitor> visitor)
{
  XdmfItem::traverse(visitor);
  for(std::vector<shared_ptr<XdmfArray> >::const_iterator iter = 
        mArrays.begin();
      iter != mArrays.end();
      ++iter) {
    (*iter)->accept(visitor);
  }
}
