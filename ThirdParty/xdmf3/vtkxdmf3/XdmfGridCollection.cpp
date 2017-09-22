/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfGridCollection.cpp                                              */
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
#include "XdmfGeometry.hpp"
#include "XdmfTopology.hpp"
#include "XdmfGridCollection.hpp"
#include "XdmfGridCollectionType.hpp"

shared_ptr<XdmfGridCollection>
XdmfGridCollection::New()
{
  shared_ptr<XdmfGridCollection> p(new XdmfGridCollection());
  return p;
}

XdmfGridCollection::XdmfGridCollection() :
  XdmfDomain(),
  XdmfGrid(shared_ptr<XdmfGeometry>(), shared_ptr<XdmfTopology>(), "Collection"),
  mType(XdmfGridCollectionType::NoCollectionType())
{
}

XdmfGridCollection::~XdmfGridCollection()
{
}

const std::string XdmfGridCollection::ItemTag = "Grid";

void
XdmfGridCollection::copyGrid(shared_ptr<XdmfGrid> sourceGrid)
{
  XdmfGrid::copyGrid(sourceGrid);
  if (shared_ptr<XdmfGridCollection> classedGrid = shared_dynamic_cast<XdmfGridCollection>(sourceGrid))
  {
    // Copy stucture from read grid to this grid
    while (this->getNumberGridCollections() > 0)
    {
      this->removeGridCollection(0);
    }
    for (unsigned int i = 0; i < classedGrid->getNumberGridCollections(); ++i)
    {
      this->insert(classedGrid->getGridCollection(i));
    }
    while (this->getNumberCurvilinearGrids() > 0)
    {
      this->removeCurvilinearGrid(0);
    }
    for (unsigned int i = 0; i < classedGrid->getNumberCurvilinearGrids(); ++i)
    {
      this->insert(classedGrid->getCurvilinearGrid(i));
    }
    while (this->getNumberGraphs() > 0)
    {
      this->removeGraph(0);
    }
    for (unsigned int i = 0; i < classedGrid->getNumberGraphs(); ++i)
    {
      this->insert(classedGrid->getGraph(i));
    }
    while (this->getNumberRectilinearGrids() > 0)
    {
      this->removeRectilinearGrid(0);
    }
    for (unsigned int i = 0; i < classedGrid->getNumberRectilinearGrids(); ++i)
    {
      this->insert(classedGrid->getRectilinearGrid(i));
    }
    while (this->getNumberRegularGrids() > 0)
    {
      this->removeRegularGrid(0);
    }
    for (unsigned int i = 0; i < classedGrid->getNumberRegularGrids(); ++i)
    {
      this->insert(classedGrid->getRegularGrid(i));
    }
    while (this->getNumberUnstructuredGrids() > 0)
    {
      this->removeUnstructuredGrid(0);
    }
    for (unsigned int i = 0; i < classedGrid->getNumberUnstructuredGrids(); ++i)
    {
      this->insert(classedGrid->getUnstructuredGrid(i));
    }
  }
}

std::map<std::string, std::string>
XdmfGridCollection::getItemProperties() const
{
  std::map<std::string, std::string> collectionProperties =
    XdmfGrid::getItemProperties();
  collectionProperties.insert(std::make_pair("GridType", "Collection"));
  mType->getProperties(collectionProperties);
  return collectionProperties;
}

std::string
XdmfGridCollection::getItemTag() const
{
  return ItemTag;
}

shared_ptr<const XdmfGridCollectionType>
XdmfGridCollection::getType() const
{
  return mType;
}

void
XdmfGridCollection::insert(const shared_ptr<XdmfInformation> information)
{
  XdmfItem::insert(information);
}

void
XdmfGridCollection::populateItem(const std::map<std::string, std::string> & itemProperties,
                                 const std::vector<shared_ptr<XdmfItem> > & childItems,
                                 const XdmfCoreReader * const reader)
{
  mType = XdmfGridCollectionType::New(itemProperties);
  XdmfDomain::populateItem(itemProperties, childItems, reader);
  mInformations.clear();
  XdmfGrid::populateItem(itemProperties, childItems, reader);
}

void
XdmfGridCollection::read()
{
  if (mGridController)
  {
    if (shared_ptr<XdmfGridCollection> grid = shared_dynamic_cast<XdmfGridCollection>(mGridController->read()))
    {
      // Copy stucture from read grid to this grid
      while(this->getNumberGridCollections() > 0)
      {
        this->removeGridCollection(0);
      }
      for (unsigned int i = 0; i < grid->getNumberGridCollections(); ++i)
      {
        this->insert(grid->getGridCollection(i));
      }
      while(this->getNumberUnstructuredGrids() > 0)
      {
        this->removeUnstructuredGrid(0);
      }
      for (unsigned int i = 0; i < grid->getNumberUnstructuredGrids(); ++i)
      {
        this->insert(grid->getUnstructuredGrid(i));
      }
      while(this->getNumberCurvilinearGrids() > 0)
      {
        this->removeCurvilinearGrid(0);
      }
      for (unsigned int i = 0; i < grid->getNumberCurvilinearGrids(); ++i)
      {
        this->insert(grid->getCurvilinearGrid(i));
      }
      while(this->getNumberRectilinearGrids() > 0)
      {
        this->removeRectilinearGrid(0);
      }
      for (unsigned int i = 0; i < grid->getNumberRectilinearGrids(); ++i)
      {
        this->insert(grid->getRectilinearGrid(i));
      }
      while(this->getNumberRegularGrids() > 0)
      {
        this->removeRegularGrid(0);
      }
      for (unsigned int i = 0; i < grid->getNumberRegularGrids(); ++i)
      {
        this->insert(grid->getRegularGrid(i));
      }
      while(this->getNumberAttributes() > 0)
      {
        this->removeAttribute(0);
      }
      for (unsigned int i = 0; i < grid->getNumberAttributes(); ++i)
      {
        this->insert(grid->getAttribute(i));
      }
      while(this->getNumberInformations() > 0)
      {
        this->removeInformation(0);
      }
      for (unsigned int i = 0; i < grid->getNumberInformations(); ++i)
      {
        this->insert(grid->getInformation(i));
      }
      while(this->getNumberSets() > 0)
      {
        this->removeSet(0);
      }
      for (unsigned int i = 0; i < grid->getNumberSets(); ++i)
      {
        this->insert(grid->getSet(i));
      }
      while(this->getNumberMaps() > 0)
      {
        this->removeMap(0);
      }
      for (unsigned int i = 0; i < grid->getNumberMaps(); ++i)
      {
        this->insert(grid->getMap(i));
      }
    }
    else if (shared_ptr<XdmfGrid> grid = shared_dynamic_cast<XdmfGrid>(mGridController->read()))
    {
      XdmfError::message(XdmfError::FATAL, "Error: Grid Type Mismatch");
    }
    else
    {
      XdmfError::message(XdmfError::FATAL, "Error: Invalid Grid Reference");
    }
  }
}

void
XdmfGridCollection::release()
{
  while(this->getNumberGridCollections() > 0)
  {
    this->removeGridCollection(0);
  }
  while(this->getNumberUnstructuredGrids() > 0)
  {
    this->removeUnstructuredGrid(0);
  }
  while(this->getNumberCurvilinearGrids() > 0)
  {
    this->removeCurvilinearGrid(0);
  }
  while(this->getNumberRectilinearGrids() > 0)
  {
    this->removeRectilinearGrid(0);
  }
  while(this->getNumberRegularGrids() > 0)
  {
    this->removeRegularGrid(0);
  }
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
XdmfGridCollection::setType(const shared_ptr<const XdmfGridCollectionType> type)
{
  mType = type;
  this->setIsChanged(true);
}

void
XdmfGridCollection::traverse(const shared_ptr<XdmfBaseVisitor> visitor)
{
  XdmfGrid::traverse(visitor);

  // Only write XdmfInformations once (deal with diamond inheritance)
  std::vector<shared_ptr<XdmfInformation> > informations;
  informations.swap(mInformations);
  XdmfDomain::traverse(visitor);
  informations.swap(mInformations);
}

// C Wrappers

XDMFGRIDCOLLECTION * XdmfGridCollectionNew()
{
  shared_ptr<XdmfGridCollection> * p = 
    new shared_ptr<XdmfGridCollection>(XdmfGridCollection::New());
  return (XDMFGRIDCOLLECTION *) p;
}

int XdmfGridCollectionGetType(XDMFGRIDCOLLECTION * collection, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  shared_ptr<XdmfGridCollection> & refCollection = *(shared_ptr<XdmfGridCollection> *)(collection);
  shared_ptr<const XdmfGridCollectionType> checkType = refCollection->getType();
  if (checkType == XdmfGridCollectionType::NoCollectionType()) {
    return XDMF_GRID_COLLECTION_TYPE_NO_COLLECTION_TYPE;
  }
  else if (checkType == XdmfGridCollectionType::Spatial()) {
    return XDMF_GRID_COLLECTION_TYPE_SPATIAL;
  }
  else if (checkType == XdmfGridCollectionType::Temporal()) {
    return XDMF_GRID_COLLECTION_TYPE_TEMPORAL;
  }
  else {
    XdmfError::message(XdmfError::FATAL,
                       "Error: Invalid ArrayType.");
  }
  XDMF_ERROR_WRAP_END(status)
  return -1;
}

void XdmfGridCollectionSetType(XDMFGRIDCOLLECTION * collection, int type, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  shared_ptr<XdmfGridCollection> & refCollection = *(shared_ptr<XdmfGridCollection> *)(collection);
  switch (type) {
  case XDMF_GRID_COLLECTION_TYPE_NO_COLLECTION_TYPE:
    refCollection->setType(XdmfGridCollectionType::NoCollectionType());
    break;
  case XDMF_GRID_COLLECTION_TYPE_SPATIAL:
    refCollection->setType(XdmfGridCollectionType::Spatial());
    break;
  case XDMF_GRID_COLLECTION_TYPE_TEMPORAL:
    refCollection->setType(XdmfGridCollectionType::Temporal());
    break;
  default:
    XdmfError::message(XdmfError::FATAL,
		       "Error: Invalid ArrayType.");
    break;
  }
  XDMF_ERROR_WRAP_END(status)
}

XDMF_DOMAIN_C_CHILD_WRAPPER(XdmfGridCollection, XDMFGRIDCOLLECTION)
XDMF_GRID_C_CHILD_WRAPPER(XdmfGridCollection, XDMFGRIDCOLLECTION)
XDMF_ITEM_C_CHILD_WRAPPER(XdmfGridCollection, XDMFGRIDCOLLECTION)
