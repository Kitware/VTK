/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfGridCollectionType.cpp                                          */
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
#include "XdmfError.hpp"
#include "XdmfGridCollectionType.hpp"
#include "XdmfStringUtils.hpp"

std::map<std::string, shared_ptr<const XdmfGridCollectionType>(*)()>
  XdmfGridCollectionType::mGridCollectionDefinitions;

// Supported XdmfGridCollectionTypes
shared_ptr<const XdmfGridCollectionType>
XdmfGridCollectionType::NoCollectionType()
{
  static shared_ptr<const XdmfGridCollectionType>
    p(new XdmfGridCollectionType("None"));
  return p;
}

shared_ptr<const XdmfGridCollectionType>
XdmfGridCollectionType::Spatial()
{
  static shared_ptr<const XdmfGridCollectionType>
    p(new XdmfGridCollectionType("Spatial"));
  return p;
}

shared_ptr<const XdmfGridCollectionType>
XdmfGridCollectionType::Temporal()
{
  static shared_ptr<const XdmfGridCollectionType>
    p(new XdmfGridCollectionType("Temporal"));
  return p;
}

void
XdmfGridCollectionType::InitTypes()
{
  mGridCollectionDefinitions["NONE"] = NoCollectionType;
  mGridCollectionDefinitions["SPATIAL"] = Spatial;
  mGridCollectionDefinitions["TEMPORAL"] = Temporal;
}

XdmfGridCollectionType::XdmfGridCollectionType(const std::string & name) :
  mName(name)
{
}

XdmfGridCollectionType::~XdmfGridCollectionType()
{
}

shared_ptr<const XdmfGridCollectionType>
XdmfGridCollectionType::New(const std::map<std::string, std::string> & itemProperties)
{
  InitTypes();

  std::map<std::string, std::string>::const_iterator type =
    itemProperties.find("CollectionType");
  if(type == itemProperties.end()) {
    XdmfError::message(XdmfError::FATAL, 
                       "'CollectionType' not in itemProperties in "
                       "XdmfGridCollectionType::New");
  }

  const std::string typeVal = XdmfStringUtils::toUpper(type->second);

  std::map<std::string, shared_ptr<const XdmfGridCollectionType>(*)()>::const_iterator returnType
    = mGridCollectionDefinitions.find(typeVal);

  if (returnType == mGridCollectionDefinitions.end()) {
    XdmfError::message(XdmfError::FATAL,
                       "'CollectionType' not of 'None', 'Spatial', or "
                       "'Temporal' in XdmfGridCollectionType::New");
  }
  else {
    return (*(returnType->second))();
  }

  XdmfError::message(XdmfError::FATAL, 
                     "'CollectionType' not of 'None', 'Spatial', or "
                     "'Temporal' in XdmfGridCollectionType::New");

  // unreachable
  return shared_ptr<const XdmfGridCollectionType>();
}

void
XdmfGridCollectionType::getProperties(std::map<std::string, std::string> & collectedProperties) const
{
  collectedProperties.insert(std::make_pair("CollectionType", mName));
}

// C Wrappers

int XdmfGridCollectionTypeNoCollectionType()
{
  return XDMF_GRID_COLLECTION_TYPE_NO_COLLECTION_TYPE;
}

int XdmfGridCollectionTypeSpatial()
{
  return XDMF_GRID_COLLECTION_TYPE_SPATIAL;
}

int XdmfGridCollectionTypeTemporal()
{
  return XDMF_GRID_COLLECTION_TYPE_TEMPORAL;
}
