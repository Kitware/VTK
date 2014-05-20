/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfAttributeCenter.cpp                                             */
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
#include "XdmfAttributeCenter.hpp"
#include "XdmfError.hpp"

// Supported XdmfAttributeCenters
shared_ptr<const XdmfAttributeCenter>
XdmfAttributeCenter::Grid()
{
  static shared_ptr<const XdmfAttributeCenter>
    p(new XdmfAttributeCenter("Grid"));
  return p;
}

shared_ptr<const XdmfAttributeCenter>
XdmfAttributeCenter::Cell()
{
  static shared_ptr<const XdmfAttributeCenter> 
    p(new XdmfAttributeCenter("Cell"));
  return p;
}

shared_ptr<const XdmfAttributeCenter>
XdmfAttributeCenter::Face()
{
  static shared_ptr<const XdmfAttributeCenter>
    p(new XdmfAttributeCenter("Face"));
  return p;
}

shared_ptr<const XdmfAttributeCenter>
XdmfAttributeCenter::Edge()
{
  static shared_ptr<const XdmfAttributeCenter>
    p(new XdmfAttributeCenter("Edge"));
  return p;
}

shared_ptr<const XdmfAttributeCenter>
XdmfAttributeCenter::Node()
{
  static shared_ptr<const XdmfAttributeCenter>
    p(new XdmfAttributeCenter("Node"));
  return p;
}

XdmfAttributeCenter::XdmfAttributeCenter(const std::string & name) :
  mName(name)
{
}

XdmfAttributeCenter::~XdmfAttributeCenter()
{
}

shared_ptr<const XdmfAttributeCenter>
XdmfAttributeCenter::New(const std::map<std::string, std::string> & itemProperties)
{
  std::map<std::string, std::string>::const_iterator center =
    itemProperties.find("Center");
  if(center == itemProperties.end()) {
    XdmfError::message(XdmfError::FATAL, 
                       "'Center' not found in itemProperties in "
                       "XdmfAttributeCenter::New");
  }
  const std::string & centerVal = center->second;
  
  if(centerVal.compare("Node") == 0) {
    return Node();
  }
  else if(centerVal.compare("Cell") == 0) {
    return Cell();
  }
  else if(centerVal.compare("Grid") == 0) {
    return Grid();
  }
  else if(centerVal.compare("Face") == 0) {
    return Face();
  }
  else if(centerVal.compare("Edge") == 0) {
    return Edge();
  }

  XdmfError::message(XdmfError::FATAL, 
                     "Center not of 'Grid','Cell','Face','Edge','Node' "
                     "in XdmfAttributeCenter::New");

  // unreachable
  return shared_ptr<const XdmfAttributeCenter>();
}

void
XdmfAttributeCenter::getProperties(std::map<std::string, std::string> & collectedProperties) const
{
  collectedProperties.insert(std::make_pair("Center", mName));
}
