/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfSetType.cpp                                                     */
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
#include "XdmfSetType.hpp"
#include "XdmfError.hpp"

// Supported XdmfSetTypes
shared_ptr<const XdmfSetType>
XdmfSetType::NoSetType()
{
  static shared_ptr<const XdmfSetType> p(new XdmfSetType("None"));
  return p;
}

shared_ptr<const XdmfSetType>
XdmfSetType::Node()
{
  static shared_ptr<const XdmfSetType> p(new XdmfSetType("Node"));
  return p;
}

shared_ptr<const XdmfSetType>
XdmfSetType::Cell()
{
  static shared_ptr<const XdmfSetType> p(new XdmfSetType("Cell"));
  return p;
}

shared_ptr<const XdmfSetType>
XdmfSetType::Face()
{
  static shared_ptr<const XdmfSetType> p(new XdmfSetType("Face"));
  return p;
}

shared_ptr<const XdmfSetType>
XdmfSetType::Edge()
{
  static shared_ptr<const XdmfSetType> p(new XdmfSetType("Edge"));
  return p;
}

XdmfSetType::XdmfSetType(const std::string & name) :
  mName(name)
{
}

XdmfSetType::~XdmfSetType()
{
}

shared_ptr<const XdmfSetType>
XdmfSetType::New(const std::map<std::string, std::string> & itemProperties)
{
  std::map<std::string, std::string>::const_iterator type =
    itemProperties.find("Type");
  if(type == itemProperties.end()) {
    type = itemProperties.find("SetType");
  }
  if(type == itemProperties.end()) {
    XdmfError::message(XdmfError::FATAL, 
                       "Neither 'Type' nor 'SetType' found in itemProperties "
                       "in XdmfSetType::New");
  }
  const std::string & typeVal = type->second;

  if(typeVal.compare("Node") == 0) {
    return Node();
  }
  else if(typeVal.compare("Cell") == 0) {
    return Cell();
  }
  else if(typeVal.compare("Face") == 0) {
    return Face();
  }
  else if(typeVal.compare("Edge") == 0) {
    return Edge();
  }
  else if(typeVal.compare("None") == 0) {
    return NoSetType();
  }

  XdmfError::message(XdmfError::FATAL, 
                     "Type not of 'None', 'Node', 'Cell', 'Face', or "
                     "'Edge' in XdmfSetType::New");

  // unreachable
  return shared_ptr<const XdmfSetType>();
}

void
XdmfSetType::getProperties(std::map<std::string, std::string> & collectedProperties) const
{
  collectedProperties.insert(std::make_pair("Type", mName));
}
