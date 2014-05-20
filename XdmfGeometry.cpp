/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfGeometry.cpp                                                    */
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

#include "XdmfGeometry.hpp"
#include "XdmfGeometryType.hpp"
#include "XdmfError.hpp"

shared_ptr<XdmfGeometry> XdmfGeometry::New()
{
  shared_ptr<XdmfGeometry> p(new XdmfGeometry());
  return p;
}

XdmfGeometry::XdmfGeometry() :
  mNumberPoints(0),
  mType(XdmfGeometryType::NoGeometryType())
{
}

XdmfGeometry::~XdmfGeometry()
{
}

const std::string XdmfGeometry::ItemTag = "Geometry";

std::map<std::string, std::string>
XdmfGeometry::getItemProperties() const
{
  std::map<std::string, std::string> geometryProperties;
  mType->getProperties(geometryProperties);
  return geometryProperties;
}

std::string
XdmfGeometry::getItemTag() const
{
  return ItemTag;
}

unsigned int
XdmfGeometry::getNumberPoints() const
{
  if(mType->getDimensions() == 0) {
    return 0;
  }
  return this->getSize() / mType->getDimensions();
}

shared_ptr<const XdmfGeometryType>
XdmfGeometry::getType() const
{
  return mType;
}

void
XdmfGeometry::populateItem(const std::map<std::string, std::string> & itemProperties,
                           const std::vector<shared_ptr<XdmfItem> > & childItems,
                           const XdmfCoreReader * const reader)
{
  XdmfItem::populateItem(itemProperties, childItems, reader);
  mType = XdmfGeometryType::New(itemProperties);
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
XdmfGeometry::setType(const shared_ptr<const XdmfGeometryType> type)
{
  mType = type;
}
