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

#include <sstream>

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

XdmfSet::XdmfSet(XdmfSet & refSet) :
  XdmfArray(refSet),
  mName(refSet.mName),
  mType(refSet.mType)
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
  try
  {
    shared_ptr<XdmfSet> generatedSet = XdmfSet::New();
    return (XDMFSET*)((void *)(new XdmfSet(*generatedSet.get())));
  }
  catch (...)
  {
    shared_ptr<XdmfSet> generatedSet = XdmfSet::New();
    return (XDMFSET*)((void *)(new XdmfSet(*generatedSet.get())));
  }
}

XDMFATTRIBUTE * XdmfSetGetAttribute(XDMFSET * set, unsigned int index)
{
  return (XDMFATTRIBUTE *)((void *)(((XdmfSet *)(set))->getAttribute(index).get()));
}

XDMFATTRIBUTE * XdmfSetGetAttributeByName(XDMFSET * set, char * Name)
{
  return (XDMFATTRIBUTE *)((void *)(((XdmfSet *)(set))->getAttribute(Name).get()));
}

unsigned int XdmfSetGetNumberAttributes(XDMFSET * set)
{
  return ((XdmfSet *)(set))->getNumberAttributes();
}

int XdmfSetGetType(XDMFSET * set)
{
  shared_ptr<const XdmfSetType> checkType = ((XdmfSet *)set)->getType();

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
  if (passControl) {
    ((XdmfSet *)(set))->insert(shared_ptr<XdmfAttribute>((XdmfAttribute *)Attribute));
  }
  else {
    ((XdmfSet *)(set))->insert(shared_ptr<XdmfAttribute>((XdmfAttribute *)Attribute, XdmfNullDeleter()));
  }
}

void XdmfSetRemoveAttribute(XDMFSET * set, unsigned int index)
{
  ((XdmfSet *)(set))->removeAttribute(index);
}

void XdmfSetRemoveAttributeByName(XDMFSET * set, char * Name)
{
  ((XdmfSet *)(set))->removeAttribute(Name);
}

void XdmfSetSetType(XDMFSET * set, int type, int * status)
{
  XDMF_ERROR_WRAP_START(status)
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
      {
        std::stringstream sstr;
        sstr << "Error: Invalid Set Type: Code " << type;
        XdmfError::message(XdmfError::FATAL, sstr.str());
      }
      break;
  }
  ((XdmfSet *)set)->setType(newType);
  XDMF_ERROR_WRAP_END(status)
}

XDMF_ITEM_C_CHILD_WRAPPER(XdmfSet, XDMFSET)
XDMF_ARRAY_C_CHILD_WRAPPER(XdmfSet, XDMFSET)
