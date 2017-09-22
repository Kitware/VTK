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
#include <string.h>
#include "XdmfAttribute.hpp"
#include "XdmfError.hpp"
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

shared_ptr<XdmfGridController>
XdmfGrid::getGridController()
{
  return mGridController;
}

void
XdmfGrid::copyGrid(shared_ptr<XdmfGrid> sourceGrid)
{
  this->setName(sourceGrid->getName());
  this->setTime(sourceGrid->getTime());
  while(this->getNumberAttributes() > 0)
  {
    this->removeAttribute(0);
  }
  for (unsigned int i = 0; i < sourceGrid->getNumberAttributes(); ++i)
  {
    this->insert(sourceGrid->getAttribute(i));
  }
  while(this->getNumberInformations() > 0)
  {
    this->removeInformation(0);
  }
  for (unsigned int i = 0; i < sourceGrid->getNumberInformations(); ++i)
  {
    this->insert(sourceGrid->getInformation(i));
  }
  while(this->getNumberSets() > 0)
  {
    this->removeSet(0);
  }
  for (unsigned int i = 0; i < sourceGrid->getNumberSets(); ++i)
  {
    this->insert(sourceGrid->getSet(i));
  }
  while(this->getNumberMaps() > 0)
  {
    this->removeMap(0);
  }
  for (unsigned int i = 0; i < sourceGrid->getNumberMaps(); ++i)
  {
    this->insert(sourceGrid->getMap(i));
  }
}

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
  return const_pointer_cast<XdmfTime>
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
    else if(shared_ptr<XdmfGridController> gridController =
            shared_dynamic_cast<XdmfGridController>(*iter)) {
      this->setGridController(gridController);
    }
  }
}

void
XdmfGrid::read()
{

}

void
XdmfGrid::release()
{
  this->setName("");
  this->setTime(shared_ptr<XdmfTime>());
  while(this->getNumberAttributes() > 0)
  {
    this->removeAttribute(0);
  }
  while(this->getNumberInformations() > 0)
  {
    this->removeInformation(0);
  }
  while(this->getNumberSets() > 0)
  {
    this->removeSet(0);
  }
  while(this->getNumberMaps() > 0)
  {
    this->removeMap(0);
  }
}

void
XdmfGrid::setGridController(shared_ptr<XdmfGridController> newController)
{
  mGridController = newController;
}


void
XdmfGrid::setName(const std::string & name)
{
  mName = name;
  this->setIsChanged(true);
}

void
XdmfGrid::setTime(const shared_ptr<XdmfTime> time)
{
  mTime = time;
  this->setIsChanged(true);
}

void
XdmfGrid::traverse(const shared_ptr<XdmfBaseVisitor> visitor)
{
  XdmfItem::traverse(visitor);
  if (mGridController) {
    mGridController->accept(visitor);
  }
  if(mTime) {
    mTime->accept(visitor);
  }
  if(mGeometry) {
    mGeometry->accept(visitor);
  }
  if(mTopology) {
    mTopology->accept(visitor);
  }
  for (unsigned int i = 0; i < mAttributes.size(); ++i)
  {
    mAttributes[i]->accept(visitor);
  }
  for (unsigned int i = 0; i < mMaps.size(); ++i)
  {
    mMaps[i]->accept(visitor);
  }
  for (unsigned int i = 0; i < mSets.size(); ++i)
  {
    mSets[i]->accept(visitor);
  }
}

// C Wrappers

XDMFATTRIBUTE * XdmfGridGetAttribute(XDMFGRID * grid, unsigned int index)
{
  shared_ptr<XdmfGrid> & refGrid = *(shared_ptr<XdmfGrid> *)(grid);
  shared_ptr<XdmfAttribute> * attribute = 
    new shared_ptr<XdmfAttribute>(refGrid->getAttribute(index));
  return (XDMFATTRIBUTE *) attribute;
}

XDMFATTRIBUTE * XdmfGridGetAttributeByName(XDMFGRID * grid, char * Name)
{
  shared_ptr<XdmfGrid> & refGrid = *(shared_ptr<XdmfGrid> *)(grid);
  shared_ptr<XdmfAttribute> * attribute = 
    new shared_ptr<XdmfAttribute>(refGrid->getAttribute(Name));
  return (XDMFATTRIBUTE *) attribute;
}

unsigned int XdmfGridGetNumberAttributes(XDMFGRID * grid)
{
  shared_ptr<XdmfGrid> & refGrid = *(shared_ptr<XdmfGrid> *)(grid);
  return refGrid->getNumberAttributes();
}

void XdmfGridInsertAttribute(XDMFGRID * grid, XDMFATTRIBUTE * Attribute, int passControl)
{
  shared_ptr<XdmfGrid> & refGrid = *(shared_ptr<XdmfGrid> *)(grid);
  shared_ptr<XdmfAttribute> & refAttribute = *(shared_ptr<XdmfAttribute> *)(Attribute);
  refGrid->insert(refAttribute);
}

void XdmfGridRemoveAttribute(XDMFGRID * grid, unsigned int index)
{
  shared_ptr<XdmfGrid> & refGrid = *(shared_ptr<XdmfGrid> *)(grid);
  refGrid->removeAttribute(index);
}

void XdmfGridRemoveAttributeByName(XDMFGRID * grid, char * Name)
{
  shared_ptr<XdmfGrid> & refGrid = *(shared_ptr<XdmfGrid> *)(grid);
  refGrid->removeAttribute(Name);
}

XDMFSET * XdmfGridGetSet(XDMFGRID * grid, unsigned int index)
{
  shared_ptr<XdmfGrid> & refGrid = *(shared_ptr<XdmfGrid> *)(grid);
  shared_ptr<XdmfSet> * set = 
    new shared_ptr<XdmfSet>(refGrid->getSet(index));
  return (XDMFSET *) set;
}

XDMFSET * XdmfGridGetSetByName(XDMFGRID * grid, char * Name)
{
  shared_ptr<XdmfGrid> & refGrid = *(shared_ptr<XdmfGrid> *)(grid);
  shared_ptr<XdmfSet> * set = 
    new shared_ptr<XdmfSet>(refGrid->getSet(Name));
  return (XDMFSET *) set;
}

unsigned int XdmfGridGetNumberSets(XDMFGRID * grid)
{
  shared_ptr<XdmfGrid> & refGrid = *(shared_ptr<XdmfGrid> *)(grid);
  return refGrid->getNumberSets();
}

void XdmfGridInsertSet(XDMFGRID * grid, XDMFSET * Set, int passControl)
{
  shared_ptr<XdmfGrid> & refGrid = *(shared_ptr<XdmfGrid> *)(grid);
  shared_ptr<XdmfSet> & refSet = *(shared_ptr<XdmfSet> *)(Set);
  refGrid->insert(refSet);
}

void XdmfGridRemoveSet(XDMFGRID * grid, unsigned int index)
{
  shared_ptr<XdmfGrid> & refGrid = *(shared_ptr<XdmfGrid> *)(grid);
  refGrid->removeSet(index);
}

void XdmfGridRemoveSetByName(XDMFGRID * grid, char * Name)
{
  shared_ptr<XdmfGrid> & refGrid = *(shared_ptr<XdmfGrid> *)(grid);
  refGrid->removeSet(Name);
}

XDMFMAP * XdmfGridGetMap(XDMFGRID * grid, unsigned int index)
{
  shared_ptr<XdmfGrid> & refGrid = *(shared_ptr<XdmfGrid> *)(grid);
  shared_ptr<XdmfMap> * map = 
    new shared_ptr<XdmfMap>(refGrid->getMap(index));
  return (XDMFMAP *) map;
}

XDMFMAP * XdmfGridGetMapByName(XDMFGRID * grid, char * Name)
{
  shared_ptr<XdmfGrid> & refGrid = *(shared_ptr<XdmfGrid> *)(grid);
  shared_ptr<XdmfMap> * map = 
    new shared_ptr<XdmfMap>(refGrid->getMap(Name));
  return (XDMFMAP *) map;
}

unsigned int XdmfGridGetNumberMaps(XDMFGRID * grid)
{
  shared_ptr<XdmfGrid> & refGrid = *(shared_ptr<XdmfGrid> *)(grid);
  return refGrid->getNumberMaps();
}

void XdmfGridInsertMap(XDMFGRID * grid, XDMFMAP * Map, int passControl)
{
  shared_ptr<XdmfGrid> & refGrid = *(shared_ptr<XdmfGrid> *)(grid);
  shared_ptr<XdmfMap> & refMap = *(shared_ptr<XdmfMap> *)(Map);
  refGrid->insert(refMap);
}

void XdmfGridRemoveMap(XDMFGRID * grid, unsigned int index)
{
  shared_ptr<XdmfGrid> & refGrid = *(shared_ptr<XdmfGrid> *)(grid);
  refGrid->removeMap(index);
}

void XdmfGridRemoveMapByName(XDMFGRID * grid, char * Name)
{
  shared_ptr<XdmfGrid> & refGrid = *(shared_ptr<XdmfGrid> *)(grid);
  refGrid->removeMap(Name);
}

XDMFGRIDCONTROLLER * XdmfGridGetGridController(XDMFGRID * grid)
{
  shared_ptr<XdmfGrid> & refGrid = *(shared_ptr<XdmfGrid> *)(grid);
  shared_ptr<XdmfGridController> * gridController =
    new shared_ptr<XdmfGridController>(refGrid->getGridController());
  return (XDMFGRIDCONTROLLER *) gridController;
}

char * XdmfGridGetName(XDMFGRID * grid)
{
  shared_ptr<XdmfGrid> & refGrid = *(shared_ptr<XdmfGrid> *)(grid);
  return strdup(refGrid->getName().c_str());
}

XDMFTIME * XdmfGridGetTime(XDMFGRID * grid)
{
  shared_ptr<XdmfGrid> & refGrid = *(shared_ptr<XdmfGrid> *)(grid);
  shared_ptr<XdmfTime> * time =
    new shared_ptr<XdmfTime>(refGrid->getTime());
  return (XDMFTIME *) time;

  XdmfItem * classedPointer = (XdmfItem *)grid;
  XdmfGrid * gridPointer = dynamic_cast<XdmfGrid *>(classedPointer);
  shared_ptr<XdmfTime> generatedTime = gridPointer->getTime();
  return (XDMFTIME *)((void *)(generatedTime.get()));
}

void
XdmfGridRead(XDMFGRID * grid, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  shared_ptr<XdmfGrid> & refGrid = *(shared_ptr<XdmfGrid> *)(grid);
  refGrid->read();
  XDMF_ERROR_WRAP_END(status)
}

void
XdmfGridRelease(XDMFGRID * grid)
{
  shared_ptr<XdmfGrid> & refGrid = *(shared_ptr<XdmfGrid> *)(grid);
  refGrid->release();
}

void XdmfGridSetGridController(XDMFGRID * grid, XDMFGRIDCONTROLLER * controller, int passControl)
{
  shared_ptr<XdmfGrid> & refGrid = *(shared_ptr<XdmfGrid> *)(grid);
  shared_ptr<XdmfGridController> & refGridController = *(shared_ptr<XdmfGridController> *)(controller);
  refGrid->setGridController(refGridController);
}

void XdmfGridSetName(XDMFGRID * grid, char * name, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  shared_ptr<XdmfGrid> & refGrid = *(shared_ptr<XdmfGrid> *)(grid);
  refGrid->setName(name);
  XDMF_ERROR_WRAP_END(status)
}

void XdmfGridSetTime(XDMFGRID * grid, XDMFTIME * time, int passControl)
{
  shared_ptr<XdmfGrid> & refGrid = *(shared_ptr<XdmfGrid> *)(grid);
  shared_ptr<XdmfTime> & refTime = *(shared_ptr<XdmfTime> *)(time);
  refGrid->setTime(refTime);
}

XDMF_ITEM_C_CHILD_WRAPPER(XdmfGrid, XDMFGRID)
