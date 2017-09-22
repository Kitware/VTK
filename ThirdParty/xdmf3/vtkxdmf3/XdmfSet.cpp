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
  bool filled = false;
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
      if (!filled) {
        this->swap(array);
        filled = true;
      }
      if (array->getReference()) {
        this->setReference(array->getReference());
        this->setReadMode(XdmfArray::Reference);
      }
      // TODO: If multiple dataitems.
    }
  }
}

void
XdmfSet::setName(const std::string & name)
{
  mName = name;
  this->setIsChanged(true);
}

void
XdmfSet::setType(const shared_ptr<const XdmfSetType> type)
{
  mType = type;
  this->setIsChanged(true);
}

void
XdmfSet::traverse(const shared_ptr<XdmfBaseVisitor> visitor)
{
  XdmfItem::traverse(visitor);
  for (unsigned int i = 0; i < mAttributes.size(); ++i)
  {
    mAttributes[i]->accept(visitor);
  }
}

// C Wrappers

XDMFSET * XdmfSetNew()
{
  shared_ptr<XdmfSet> * p = 
    new shared_ptr<XdmfSet>(XdmfSet::New());
  return (XDMFSET *) p;
}

XDMFATTRIBUTE * XdmfSetGetAttribute(XDMFSET * set, unsigned int index)
{
  shared_ptr<XdmfSet> & refSet = *(shared_ptr<XdmfSet> *)(set);
  shared_ptr<XdmfAttribute> * p = new shared_ptr<XdmfAttribute>(refSet->getAttribute(index));
  return (XDMFATTRIBUTE *) p;
}

XDMFATTRIBUTE * XdmfSetGetAttributeByName(XDMFSET * set, char * Name)
{
  shared_ptr<XdmfSet> & refSet = *(shared_ptr<XdmfSet> *)(set);
  shared_ptr<XdmfAttribute> * p = new shared_ptr<XdmfAttribute>(refSet->getAttribute(Name));
  return (XDMFATTRIBUTE *) p;
}

unsigned int XdmfSetGetNumberAttributes(XDMFSET * set)
{
  shared_ptr<XdmfSet> & refSet = *(shared_ptr<XdmfSet> *)(set);
  return refSet->getNumberAttributes();
}

int XdmfSetGetType(XDMFSET * set)
{
  shared_ptr<XdmfSet> & refSet = *(shared_ptr<XdmfSet> *)(set);
  shared_ptr<const XdmfSetType> checkType = refSet->getType();

  if (checkType == XdmfSetType::NoSetType()) {
    return XDMF_SET_TYPE_NO_SET_TYPE;
  }
  else if (checkType == XdmfSetType::Node()) {
    return XDMF_SET_TYPE_NODE;
  }
  else if (checkType == XdmfSetType::Cell()) {
    return XDMF_SET_TYPE_CELL;
  }
  else if (checkType == XdmfSetType::Face()) {
    return XDMF_SET_TYPE_FACE;
  }
  else if (checkType == XdmfSetType::Edge()) {
    return XDMF_SET_TYPE_EDGE;
  }
  else {
    return -1;
  }
}

void XdmfSetInsertAttribute(XDMFSET * set, XDMFATTRIBUTE * Attribute, int passControl)
{
  shared_ptr<XdmfSet> & refSet = *(shared_ptr<XdmfSet> *)(set);
  shared_ptr<XdmfAttribute> & refAttribute = *(shared_ptr<XdmfAttribute> *)(Attribute);
  refSet->insert(refAttribute);
}

void XdmfSetRemoveAttribute(XDMFSET * set, unsigned int index)
{
  shared_ptr<XdmfSet> & refSet = *(shared_ptr<XdmfSet> *)(set);
  refSet->removeAttribute(index);
}

void XdmfSetRemoveAttributeByName(XDMFSET * set, char * Name)
{
  shared_ptr<XdmfSet> & refSet = *(shared_ptr<XdmfSet> *)(set);
  refSet->removeAttribute(Name);
}

void XdmfSetSetType(XDMFSET * set, int type, int * status)
{
  XDMF_ERROR_WRAP_START(status)
    shared_ptr<XdmfSet> & refSet = *(shared_ptr<XdmfSet> *)(set);
  shared_ptr<const XdmfSetType> newType = shared_ptr<const XdmfSetType>();
  switch (type) {
    case XDMF_SET_TYPE_NO_SET_TYPE:
      newType = XdmfSetType::NoSetType();
      break;
    case XDMF_SET_TYPE_NODE:
      newType = XdmfSetType::Node();
      break;
    case XDMF_SET_TYPE_CELL:
      newType = XdmfSetType::Cell();
      break;
    case XDMF_SET_TYPE_FACE:
      newType = XdmfSetType::Face();
      break;
    case XDMF_SET_TYPE_EDGE:
      newType = XdmfSetType::Edge();
      break;
    default:
      XdmfError::message(XdmfError::FATAL,
                         "Error: Invalid Set Type: Code " + type);
      break;
  }
  refSet->setType(newType);
  XDMF_ERROR_WRAP_END(status)
}

XDMF_ITEM_C_CHILD_WRAPPER(XdmfSet, XDMFSET)
XDMF_ARRAY_C_CHILD_WRAPPER(XdmfSet, XDMFSET)
