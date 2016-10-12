/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfFunction.cpp                                                    */
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


#include "XdmfArray.hpp"
#include "XdmfArrayType.hpp"
#include "XdmfArrayReference.hpp"
#include <stack>
#include <math.h>
#include <boost/assign.hpp>
#include "XdmfError.hpp"

XdmfArrayReference::XdmfArrayReference():
  mConstructedType("")
{
}

XdmfArrayReference::XdmfArrayReference(XdmfArrayReference & refReference) :
  XdmfItem(refReference),
  mConstructedType(refReference.mConstructedType),
  mConstructedProperties(refReference.mConstructedProperties)
{
}

XdmfArrayReference::~XdmfArrayReference()
{
}

std::map<std::string, std::string>
XdmfArrayReference::getConstructedProperties()
{
  return mConstructedProperties;
}

std::string
XdmfArrayReference::getConstructedType() const
{
  if (mConstructedType.c_str() != NULL) {
    return mConstructedType;
  }
  else {
    return "";
  }
}

std::map<std::string, std::string>
XdmfArrayReference::getItemProperties() const
{
  std::map<std::string, std::string> referenceProperties;

  referenceProperties["ConstructedType"] = mConstructedType;

  for (std::map<std::string, std::string>::const_iterator constructedIt = mConstructedProperties.begin();
       constructedIt != mConstructedProperties.end();
       ++constructedIt) {
    referenceProperties[constructedIt->first] = constructedIt->second;
  }

  // An array is missing a lot of data if not read first
  if (mConstructedType.compare(XdmfArray::ItemTag) == 0) {
    shared_ptr<XdmfArray> resultArray = this->read();
    shared_ptr<const XdmfArrayType> resultArrayType = resultArray->getArrayType();
    std::map<std::string, std::string> arrayTypeProperties;
    resultArrayType->getProperties(arrayTypeProperties);
    for (std::map<std::string, std::string>::const_iterator arrayTypeIt = arrayTypeProperties.begin();
         arrayTypeIt != arrayTypeProperties.end();
         ++arrayTypeIt) {
      referenceProperties[arrayTypeIt->first] = arrayTypeIt->second;
    }
    referenceProperties["Format"] = "XML";
    referenceProperties["Dimensions"] = resultArray->getDimensionsString();
  }

  return referenceProperties;
}

void
XdmfArrayReference::setConstructedProperties(std::map<std::string, std::string> newProperties)
{
  mConstructedProperties = newProperties;
  this->setIsChanged(true);
}

void
XdmfArrayReference::setConstructedType(std::string newType)
{
  mConstructedType = newType;
  this->setIsChanged(true);
}

// C Wrappers

char * XdmfArrayReferenceGetConstructedType(XDMFARRAYREFERENCE * arrayReference)
{
  try
  {
    char * returnPointer = strdup((*((XdmfArrayReference *)arrayReference)).getConstructedType().c_str());
    return returnPointer;
  }
  catch (...)
  {
    char * returnPointer = strdup((*((XdmfArrayReference *)arrayReference)).getConstructedType().c_str());
    return returnPointer;
  }
}

void * XdmfArrayReferenceRead(XDMFARRAYREFERENCE * arrayReference, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  try
  {
    shared_ptr<XdmfArray> returnItem = ((XdmfArrayReference *)arrayReference)->read();
    return new XdmfArray(*returnItem.get());
  }
  catch (...)
  {
    shared_ptr<XdmfArray> returnItem = ((XdmfArrayReference *)arrayReference)->read();
    return new XdmfArray(*returnItem.get());
  }
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

void XdmfArrayReferenceSetConstructedProperties(XDMFARRAYREFERENCE * arrayReference, void * referenceobject)
{
  ((XdmfArrayReference *)arrayReference)->setConstructedProperties(((XdmfItem *)referenceobject)->getItemProperties());
}

void XdmfArrayReferenceSetConstructedType(XDMFARRAYREFERENCE * arrayReference, char * newType)
{
  ((XdmfArrayReference *)arrayReference)->setConstructedType(newType);
}

// C Wrappers for parent classes are generated by macros

XDMF_ITEM_C_CHILD_WRAPPER(XdmfArrayReference, XDMFARRAYREFERENCE)
