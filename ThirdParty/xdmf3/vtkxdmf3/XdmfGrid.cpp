/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfGrid.cpp                                                        */
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
#include "XdmfGeometry.hpp"
#include "XdmfGrid.hpp"
#include "XdmfMap.hpp"
#include "XdmfSet.hpp"
#include "XdmfTime.hpp"
#include "XdmfTopology.hpp"

XDMF_CHILDREN_IMPLEMENTATION(XdmfGrid, XdmfAttribute, Attribute, Name)
XDMF_CHILDREN_IMPLEMENTATION(XdmfGrid, XdmfMap, Map, Name)
XDMF_CHILDREN_IMPLEMENTATION(XdmfGrid, XdmfSet, Set, Name)

XdmfGrid::XdmfGrid(const shared_ptr<XdmfGeometry> geometry,
                   const shared_ptr<XdmfTopology> topology,
                   const std::string & name) :
  mGeometry(geometry),
  mTopology(topology),
  mName(name),
  mTime(shared_ptr<XdmfTime>())
{
}

XdmfGrid::~XdmfGrid()
{
}

const std::string XdmfGrid::ItemTag = "Grid";

shared_ptr<const XdmfGeometry>
XdmfGrid::getGeometry() const
{
  return mGeometry;
}

std::map<std::string, std::string>
XdmfGrid::getItemProperties() const
{
  std::map<std::string, std::string> gridProperties;
  gridProperties.insert(std::make_pair("Name", mName));
  return gridProperties;
}

std::string
XdmfGrid::getItemTag() const
{
  return ItemTag;
}

std::string
XdmfGrid::getName() const
{
    return mName;
}

shared_ptr<XdmfTime>
XdmfGrid::getTime()
{
  return boost::const_pointer_cast<XdmfTime>
    (static_cast<const XdmfGrid &>(*this).getTime());
}

shared_ptr<const XdmfTime>
XdmfGrid::getTime() const
{
  return mTime;
}

shared_ptr<const XdmfTopology>
XdmfGrid::getTopology() const
{
  return mTopology;
}

void
XdmfGrid::populateItem(const std::map<std::string, std::string> & itemProperties,
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
    mName = "";
  }
  for(std::vector<shared_ptr<XdmfItem> >::const_iterator iter =
        childItems.begin();
      iter != childItems.end();
      ++iter) {
    if(shared_ptr<XdmfAttribute> attribute =
       shared_dynamic_cast<XdmfAttribute>(*iter)) {
      this->insert(attribute);
    }
    else if(shared_ptr<XdmfGeometry> geometry =
            shared_dynamic_cast<XdmfGeometry>(*iter)) {
      mGeometry = geometry;
    }
    else if(shared_ptr<XdmfMap> map =
            shared_dynamic_cast<XdmfMap>(*iter)) {
      this->insert(map);
    }
    else if(shared_ptr<XdmfSet> set =
            shared_dynamic_cast<XdmfSet>(*iter)) {
      this->insert(set);
    }
    else if(shared_ptr<XdmfTime> time =
            shared_dynamic_cast<XdmfTime>(*iter)) {
      mTime = time;
    }
    else if(shared_ptr<XdmfTopology> topology =
            shared_dynamic_cast<XdmfTopology>(*iter)) {
      mTopology = topology;
    }
  }
}

void
XdmfGrid::setName(const std::string & name)
{
  mName = name;
}

void
XdmfGrid::setTime(const shared_ptr<XdmfTime> time)
{
  mTime = time;
}

void
XdmfGrid::traverse(const shared_ptr<XdmfBaseVisitor> visitor)
{
  XdmfItem::traverse(visitor);
  if(mTime) {
    mTime->accept(visitor);
  }
  if(mGeometry) {
    mGeometry->accept(visitor);
  }
  if(mTopology) {
    mTopology->accept(visitor);
  }
  for(std::vector<shared_ptr<XdmfAttribute> >::const_iterator iter = 
        mAttributes.begin();
      iter != mAttributes.end();
      ++iter) {
    (*iter)->accept(visitor);
  }
  for(std::vector<shared_ptr<XdmfMap> >::const_iterator iter = 
        mMaps.begin();
      iter != mMaps.end();
      ++iter) {
    (*iter)->accept(visitor);
  }
  for(std::vector<shared_ptr<XdmfSet> >::const_iterator iter = mSets.begin();
      iter != mSets.end();
      ++iter) {
    (*iter)->accept(visitor);
  }
}
