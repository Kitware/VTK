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

XdmfAttribute::XdmfAttribute(XdmfAttribute & refAttribute) :
  XdmfArray(refAttribute),
  mCenter(refAttribute.mCenter),
  mName(refAttribute.mName),
  mType(refAttribute.mType)
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
      if (array->getReference()) {
        this->setReference(array->getReference());
        this->setReadMode(XdmfArray::Reference);
      }
      break;
    }
  }
}

void
XdmfAttribute::setCenter(const shared_ptr<const XdmfAttributeCenter> center)
{
  mCenter = center;
  this->setIsChanged(true);
}

void
XdmfAttribute::setName(const std::string & name)
{
  mName = name;
  this->setIsChanged(true);
}

void
XdmfAttribute::setType(const shared_ptr<const XdmfAttributeType> type)
{
  mType = type;
  this->setIsChanged(true);
}

// C Wrappers

XDMFATTRIBUTE * XdmfAttributeNew()
{
  try
  {
    shared_ptr<XdmfAttribute> generatedAttribute = XdmfAttribute::New();
    return (XDMFATTRIBUTE *)((void *)(new XdmfAttribute(*generatedAttribute.get())));
  }
  catch (...)
  {
    shared_ptr<XdmfAttribute> generatedAttribute = XdmfAttribute::New();
    return (XDMFATTRIBUTE *)((void *)(new XdmfAttribute(*generatedAttribute.get())));
  }
}

int XdmfAttributeGetCenter(XDMFATTRIBUTE * attribute)
{
  if (((XdmfAttribute *)attribute)->getCenter() == XdmfAttributeCenter::Grid()) {
    return XDMF_ATTRIBUTE_CENTER_GRID;
  }
  else if (((XdmfAttribute *)attribute)->getCenter() == XdmfAttributeCenter::Cell()) {
    return XDMF_ATTRIBUTE_CENTER_CELL;
  }
  else if (((XdmfAttribute *)attribute)->getCenter() == XdmfAttributeCenter::Face()) {
    return XDMF_ATTRIBUTE_CENTER_FACE;
  }
  else if (((XdmfAttribute *)attribute)->getCenter() == XdmfAttributeCenter::Edge()) {
    return XDMF_ATTRIBUTE_CENTER_EDGE;
  }
  else if (((XdmfAttribute *)attribute)->getCenter() == XdmfAttributeCenter::Node()) {
    return XDMF_ATTRIBUTE_CENTER_NODE;
  }
  else {
    return -1;
  }
}

int XdmfAttributeGetType(XDMFATTRIBUTE * attribute)
{
  if (((XdmfAttribute *)attribute)->getType() == XdmfAttributeType::Scalar()) {
    return XDMF_ATTRIBUTE_TYPE_SCALAR;
  }
  else if (((XdmfAttribute *)attribute)->getType() == XdmfAttributeType::Vector()) {
    return XDMF_ATTRIBUTE_TYPE_VECTOR;
  }
  else if (((XdmfAttribute *)attribute)->getType() == XdmfAttributeType::Tensor()) {
    return XDMF_ATTRIBUTE_TYPE_TENSOR;
  }
  else if (((XdmfAttribute *)attribute)->getType() == XdmfAttributeType::Matrix()) {
    return XDMF_ATTRIBUTE_TYPE_MATRIX;
  }
  else if (((XdmfAttribute *)attribute)->getType() == XdmfAttributeType::Tensor6()) {
    return XDMF_ATTRIBUTE_TYPE_TENSOR6;
  }
  else if (((XdmfAttribute *)attribute)->getType() == XdmfAttributeType::GlobalId()) {
    return XDMF_ATTRIBUTE_TYPE_GLOBALID;
  }
  else if (((XdmfAttribute *)attribute)->getType() == XdmfAttributeType::NoAttributeType()) {
    return XDMF_ATTRIBUTE_TYPE_NOTYPE;
  }
  else {
    return -1;
  }
}

void XdmfAttributeSetCenter(XDMFATTRIBUTE * attribute, int center, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  switch(center) {
    case XDMF_ATTRIBUTE_CENTER_GRID:
      ((XdmfAttribute *)attribute)->setCenter(XdmfAttributeCenter::Grid());
      break;
    case XDMF_ATTRIBUTE_CENTER_CELL:
      ((XdmfAttribute *)attribute)->setCenter(XdmfAttributeCenter::Cell());
      break;
    case XDMF_ATTRIBUTE_CENTER_FACE:
      ((XdmfAttribute *)attribute)->setCenter(XdmfAttributeCenter::Face());
      break;
    case XDMF_ATTRIBUTE_CENTER_EDGE:
      ((XdmfAttribute *)attribute)->setCenter(XdmfAttributeCenter::Edge());
      break;
    case XDMF_ATTRIBUTE_CENTER_NODE:
      ((XdmfAttribute *)attribute)->setCenter(XdmfAttributeCenter::Node());
      break;
    default:
      XdmfError::message(XdmfError::FATAL,
                         "Error: Invalid Attribute Center: Code " + center);
      break;
  }
  XDMF_ERROR_WRAP_END(status)
}

void XdmfAttributeSetType(XDMFATTRIBUTE * attribute, int type, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  switch(type) {
    case XDMF_ATTRIBUTE_TYPE_SCALAR:
      ((XdmfAttribute *)attribute)->setType(XdmfAttributeType::Scalar());
      break;
    case XDMF_ATTRIBUTE_TYPE_VECTOR:
      ((XdmfAttribute *)attribute)->setType(XdmfAttributeType::Vector());
      break;
    case XDMF_ATTRIBUTE_TYPE_TENSOR:
      ((XdmfAttribute *)attribute)->setType(XdmfAttributeType::Tensor());
      break;
    case XDMF_ATTRIBUTE_TYPE_MATRIX:
      ((XdmfAttribute *)attribute)->setType(XdmfAttributeType::Matrix());
      break;
    case XDMF_ATTRIBUTE_TYPE_TENSOR6:
      ((XdmfAttribute *)attribute)->setType(XdmfAttributeType::Tensor6());
      break;
    case XDMF_ATTRIBUTE_TYPE_GLOBALID:
      ((XdmfAttribute *)attribute)->setType(XdmfAttributeType::GlobalId());
      break;
    case XDMF_ATTRIBUTE_TYPE_NOTYPE:
      ((XdmfAttribute *)attribute)->setType(XdmfAttributeType::NoAttributeType());
      break;
    default:
      XdmfError::message(XdmfError::FATAL,
                         "Error: Invalid Attribute Type: Code " + type);
      break;
  }
  XDMF_ERROR_WRAP_END(status)
}

XDMF_ITEM_C_CHILD_WRAPPER(XdmfAttribute, XDMFATTRIBUTE)
XDMF_ARRAY_C_CHILD_WRAPPER(XdmfAttribute, XDMFATTRIBUTE)
