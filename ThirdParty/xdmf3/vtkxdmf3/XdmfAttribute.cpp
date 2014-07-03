/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfAttribute.cpp                                                   */
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
#include "XdmfAttribute.hpp"
#include "XdmfAttributeCenter.hpp"
#include "XdmfAttributeType.hpp"
#include "XdmfError.hpp"

shared_ptr<XdmfAttribute>
XdmfAttribute::New()
{
  shared_ptr<XdmfAttribute> p(new XdmfAttribute());
  return p;
}

XdmfAttribute::XdmfAttribute() :
  mCenter(XdmfAttributeCenter::Grid()),
  mName(""),
  mType(XdmfAttributeType::NoAttributeType())
{
}

XdmfAttribute::~XdmfAttribute()
{
}

const std::string XdmfAttribute::ItemTag = "Attribute";

shared_ptr<const XdmfAttributeCenter>
XdmfAttribute::getCenter() const
{
  return mCenter;
}

std::map<std::string, std::string>
XdmfAttribute::getItemProperties() const
{
  std::map<std::string, std::string> attributeProperties;
  attributeProperties.insert(std::make_pair("Name", mName));
  mType->getProperties(attributeProperties);
  mCenter->getProperties(attributeProperties);
  return attributeProperties;
}

std::string
XdmfAttribute::getItemTag() const
{
  return ItemTag;
}

std::string
XdmfAttribute::getName() const
{
  return mName;
}

shared_ptr<const XdmfAttributeType>
XdmfAttribute::getType() const
{
  return mType;
}

void
XdmfAttribute::populateItem(const std::map<std::string, std::string> & itemProperties,
                            const std::vector<shared_ptr<XdmfItem> > & childItems,
                            const XdmfCoreReader * const reader)
{
  XdmfItem::populateItem(itemProperties, childItems, reader);

  std::map<std::string, std::string>::const_iterator name =
    itemProperties.find("Name");
  if(name != itemProperties.end()) {
    mName = name->second;
  }
  else {
    XdmfError::message(XdmfError::FATAL,
                       "'Name' not found in itemProperties in "
                       "XdmfAttribute::populateItem");
  }

  mCenter = XdmfAttributeCenter::New(itemProperties);
  mType = XdmfAttributeType::New(itemProperties);
  for(std::vector<shared_ptr<XdmfItem> >::const_iterator iter = 
        childItems.begin();
      iter != childItems.end();
      ++iter) {
    if(shared_ptr<XdmfArray> array = shared_dynamic_cast<XdmfArray>(*iter)) {
      this->swap(array);
      break;
    }
  }
}

void
XdmfAttribute::setCenter(const shared_ptr<const XdmfAttributeCenter> center)
{
  mCenter = center;
}

void
XdmfAttribute::setName(const std::string & name)
{
  mName= name;
}

void
XdmfAttribute::setType(const shared_ptr<const XdmfAttributeType> type)
{
  mType = type;
}
