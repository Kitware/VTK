/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfSet.cpp                                                         */
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
#include "XdmfHDF5Controller.hpp"
#include "XdmfSet.hpp"
#include "XdmfSetType.hpp"
#include "XdmfError.hpp"

XDMF_CHILDREN_IMPLEMENTATION(XdmfSet, XdmfAttribute, Attribute, Name)

  shared_ptr<XdmfSet>
XdmfSet::New()
{
  shared_ptr<XdmfSet> p(new XdmfSet());
  return p;
}

XdmfSet::XdmfSet() :
  mName(""),
  mType(XdmfSetType::NoSetType())
{
}

XdmfSet::~XdmfSet()
{
}

const std::string XdmfSet::ItemTag = "Set";

std::map<std::string, std::string>
XdmfSet::getItemProperties() const
{
  std::map<std::string, std::string> setProperties;
  setProperties.insert(std::make_pair("Name", mName));
  mType->getProperties(setProperties);
  return setProperties;
}

std::string
XdmfSet::getItemTag() const
{
  return ItemTag;
}

std::string
XdmfSet::getName() const
{
  return mName;
}

shared_ptr<const XdmfSetType>
XdmfSet::getType() const
{
  return mType;
}

void
XdmfSet::populateItem(const std::map<std::string, std::string> & itemProperties,
                      const std::vector<shared_ptr<XdmfItem> > & childItems,
                      const XdmfCoreReader * const reader)
{
  XdmfItem::populateItem(itemProperties, childItems, reader);
  std::map<std::string, std::string>::const_iterator name =
    itemProperties.find("Name");
  if(name != itemProperties.end()) {
    mName = name->second;
  }
  mType = XdmfSetType::New(itemProperties);
  for(std::vector<shared_ptr<XdmfItem> >::const_iterator iter =
        childItems.begin();
      iter != childItems.end();
      ++iter) {
    if(shared_ptr<XdmfAttribute> attribute =
       shared_dynamic_cast<XdmfAttribute>(*iter)) {
      this->insert(attribute);
    }
    else if(shared_ptr<XdmfArray> array = 
            shared_dynamic_cast<XdmfArray>(*iter)) {
      this->swap(array);
      if (array->getReference()) {
        this->setReference(array->getReference());
        this->setReadMode(XdmfArray::Reference);
      }
      // TODO: If multiple dataitems.
      break;
    }
  }
}

void
XdmfSet::setName(const std::string & name)
{
  mName = name;
}

void
XdmfSet::setType(const shared_ptr<const XdmfSetType> type)
{
  mType = type;
}

void
XdmfSet::traverse(const shared_ptr<XdmfBaseVisitor> visitor)
{
  XdmfItem::traverse(visitor);
  for(std::vector<shared_ptr<XdmfAttribute> >::const_iterator iter =
        mAttributes.begin();
      iter != mAttributes.end();
      ++iter) {
    (*iter)->accept(visitor);
  }
}
