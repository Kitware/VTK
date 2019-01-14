/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfUnstructuredGrid.cpp                                            */
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

#include "XdmfError.hpp"
#include "XdmfGeometry.hpp"
#include "XdmfGeometryType.hpp"
#include "XdmfRegularGrid.hpp"
#include "XdmfTopology.hpp"
#include "XdmfTopologyType.hpp"
#include "XdmfUnstructuredGrid.hpp"

/**
 * local functions
 */
namespace {

  void
  convertRegularGeometry(unsigned int          index,
                         shared_ptr<XdmfArray> point,
                         shared_ptr<XdmfArray> dimensions,
                         shared_ptr<XdmfArray> brickSize,
                         shared_ptr<XdmfArray> mGeometry) {

    const unsigned int nDim = dimensions->getValue<unsigned int>(index);
    const double nBrickSize = brickSize->getValue<double>(index);
    const double originalPoint = point->getValue<double>(index);

    for(unsigned int i=0; i<nDim; ++i) {
      if(index == 0) {
        mGeometry->insert(mGeometry->getSize(),
                          point,
                          0,
                          point->getSize());
      }
      else {
        convertRegularGeometry(index - 1,
                               point,
                               dimensions,
                               brickSize,
                               mGeometry);
      }
      const double currPoint = point->getValue<double>(index);
      point->insert(index, currPoint + nBrickSize);
    }

    point->insert(index, originalPoint);
  }

  void
  convertRegularTopology(shared_ptr<XdmfArray> dimensions,
                         shared_ptr<XdmfArray> mTopology) 
  {

    if(dimensions->getSize() == 2) {
      const unsigned int nx = dimensions->getValue<unsigned int>(0);
      const unsigned int ny = dimensions->getValue<unsigned int>(1);
      unsigned int offset = 0;
      for(unsigned int i=1; i<ny; ++i) {
        for(unsigned int j=1; j<nx; ++j) {
          mTopology->pushBack<unsigned int>(offset);
          mTopology->pushBack<unsigned int>(offset + 1);
          mTopology->pushBack<unsigned int>(offset + nx + 1);
          mTopology->pushBack<unsigned int>(offset + nx);
          ++offset;
        }
        ++offset;
      }
    }
    else if(dimensions->getSize() == 3) {
      const unsigned int nx = dimensions->getValue<unsigned int>(0);
      const unsigned int ny = dimensions->getValue<unsigned int>(1);
      const unsigned int nz = dimensions->getValue<unsigned int>(2);
      const unsigned int zOffset = nx * ny;
      unsigned int offset = 0;
      for(unsigned int i=1; i<nz; ++i) {
        for(unsigned int j=1; j<ny; ++j) {
          for(unsigned int k=1; k<nx; ++k) {
            mTopology->pushBack<unsigned int>(offset);
            mTopology->pushBack<unsigned int>(offset + 1);
            mTopology->pushBack<unsigned int>(offset + nx + 1);
            mTopology->pushBack<unsigned int>(offset + nx);
            mTopology->pushBack<unsigned int>(offset + zOffset);
            mTopology->pushBack<unsigned int>(offset + zOffset + 1);
            mTopology->pushBack<unsigned int>(offset + zOffset + nx + 1);
            mTopology->pushBack<unsigned int>(offset + zOffset + nx);
            ++offset;
          }
          ++offset;
        }
        offset += nx;
      }
    }
  }      
}

class XdmfUnstructuredGrid::XdmfUnstructuredGridImpl : public XdmfGridImpl
{
  public:
  XdmfUnstructuredGridImpl()
  {
    mGridType = "Unstructured";
  }

  ~XdmfUnstructuredGridImpl()
  {
  }

  XdmfGridImpl * duplicate()
  {
    return new XdmfUnstructuredGridImpl();
  }

  std::string getGridType() const
  {
    return mGridType;
  }
};

shared_ptr<XdmfUnstructuredGrid>
XdmfUnstructuredGrid::New()
{
  shared_ptr<XdmfUnstructuredGrid> p(new XdmfUnstructuredGrid());
  return p;
}

shared_ptr<XdmfUnstructuredGrid> 
XdmfUnstructuredGrid::New(const shared_ptr<XdmfRegularGrid> regularGrid)
{
  shared_ptr<XdmfUnstructuredGrid> p(new XdmfUnstructuredGrid(regularGrid));
  return p;
}

XdmfUnstructuredGrid::XdmfUnstructuredGrid() :
  XdmfGrid(XdmfGeometry::New(), XdmfTopology::New())
{
  mImpl = new XdmfUnstructuredGridImpl();
}

XdmfUnstructuredGrid::XdmfUnstructuredGrid(const shared_ptr<XdmfRegularGrid> regularGrid) :
  XdmfGrid(XdmfGeometry::New(), XdmfTopology::New())
{
  mImpl = new XdmfUnstructuredGridImpl();

  const shared_ptr<XdmfArray> origin = regularGrid->getOrigin();

  shared_ptr<XdmfArray> brickSize = regularGrid->getBrickSize();
  shared_ptr<XdmfArray> dimensions = regularGrid->getDimensions();

  if(dimensions->getSize() != brickSize->getSize() ||
     dimensions->getSize() != origin->getSize()) {
    XdmfError::message(XdmfError::FATAL,
                       "Inconsistent brick, dimension, and origin sizes when"
                       "converting regular grid to unstructured grid in "
                       "XdmfUnstructuredGrid constructor");
  }

  bool releaseOrigin = false;
  bool releaseBrickSize = false;
  bool releaseDimensions = false;
  if(!origin->isInitialized()) {
    origin->read();
    releaseOrigin = true;
  }
  if(!brickSize->isInitialized()) {
    brickSize->read();
    releaseBrickSize = true;
  }  
  if(!dimensions->isInitialized()) {
    dimensions->read();
    releaseDimensions = true;
  }
  
  shared_ptr<const XdmfGeometryType> geometryType;
  shared_ptr<const XdmfTopologyType> topologyType;
  if(origin->getSize() == 2) {
    geometryType = XdmfGeometryType::XY();
    topologyType = XdmfTopologyType::Quadrilateral();
  }
  else if(origin->getSize() == 3) {
    geometryType = XdmfGeometryType::XYZ();
    topologyType = XdmfTopologyType::Hexahedron();
  }
  else {
    XdmfError::message(XdmfError::FATAL, 
                       "Cannot convert regular grid of dimensions not 2 or 3 "
                       "to XdmfUnstructuredGrid in XdmfUnstructuredGrid "
                       "constructor");
  }
  mGeometry->setType(geometryType);
  mTopology->setType(topologyType);

  shared_ptr<XdmfArray> point = XdmfArray::New();
  point->insert(0, origin, 0, origin->getSize());
  convertRegularGeometry(dimensions->getSize() - 1,
                         point,
                         dimensions,
                         brickSize,
                         mGeometry);
  convertRegularTopology(dimensions,
                         mTopology);
  
  if(releaseOrigin) {
    origin->release();
  }
  if(releaseBrickSize) {
    brickSize->release();
  }
  if(releaseDimensions) {
    dimensions->release();
  }  
}

XdmfUnstructuredGrid::XdmfUnstructuredGrid(XdmfUnstructuredGrid & refGrid) :
  XdmfGrid(refGrid)
{
}

XdmfUnstructuredGrid::~XdmfUnstructuredGrid()
{
  if (mImpl) {
    delete mImpl;
  }
  mImpl = NULL;
}

const std::string XdmfUnstructuredGrid::ItemTag = "Grid";

void
XdmfUnstructuredGrid::copyGrid(shared_ptr<XdmfGrid> sourceGrid)
{
  XdmfGrid::copyGrid(sourceGrid);
  if (shared_ptr<XdmfUnstructuredGrid> classedGrid = shared_dynamic_cast<XdmfUnstructuredGrid>(sourceGrid))
  {
    this->setGeometry(classedGrid->getGeometry());
    this->setTopology(classedGrid->getTopology());
  }
}

shared_ptr<XdmfGeometry>
XdmfUnstructuredGrid::getGeometry()
{
  return boost::const_pointer_cast<XdmfGeometry>
    (static_cast<const XdmfGrid &>(*this).getGeometry());
}

std::string
XdmfUnstructuredGrid::getItemTag() const
{
  return ItemTag;
}

shared_ptr<XdmfTopology>
XdmfUnstructuredGrid::getTopology()
{
  return boost::const_pointer_cast<XdmfTopology>
    (static_cast<const XdmfGrid &>(*this).getTopology());
}

void
XdmfUnstructuredGrid::read()
{
  if (mGridController)
  {
    if (shared_ptr<XdmfUnstructuredGrid> grid = shared_dynamic_cast<XdmfUnstructuredGrid>(mGridController->read()))
    {
      copyGrid(grid);
    }
    else if (shared_dynamic_cast<XdmfGrid>(mGridController->read()))
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
XdmfUnstructuredGrid::release()
{
  XdmfGrid::release();
  this->setGeometry(shared_ptr<XdmfGeometry>());
  this->setTopology(shared_ptr<XdmfTopology>());
}

void
XdmfUnstructuredGrid::setGeometry(const shared_ptr<XdmfGeometry> geometry)
{
  mGeometry = geometry;
}

void
XdmfUnstructuredGrid::setTopology(const shared_ptr<XdmfTopology> topology)
{
  mTopology = topology;
}

// C Wrappers

XDMFUNSTRUCTUREDGRID * XdmfUnstructuredGridNew()
{
  try
  {
    shared_ptr<XdmfUnstructuredGrid> generatedGrid = XdmfUnstructuredGrid::New();
    return (XDMFUNSTRUCTUREDGRID *)((void *)((XdmfItem *)(new XdmfUnstructuredGrid(*generatedGrid.get()))));
  }
  catch (...)
  {
    shared_ptr<XdmfUnstructuredGrid> generatedGrid = XdmfUnstructuredGrid::New();
    return (XDMFUNSTRUCTUREDGRID *)((void *)((XdmfItem *)(new XdmfUnstructuredGrid(*generatedGrid.get()))));
  }
}

XDMFUNSTRUCTUREDGRID * XdmfUnstructuredGridNewFromRegularGrid(XDMFREGULARGRID * regularGrid, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  try
  {
    // Here it works when classed directly to the grid type,
    // in other cases this may not work.
    XdmfItem * tempPointer = (XdmfItem *)regularGrid;
    XdmfRegularGrid * classedPointer = dynamic_cast<XdmfRegularGrid *>(tempPointer);
    shared_ptr<XdmfRegularGrid> originGrid = shared_ptr<XdmfRegularGrid>(classedPointer, XdmfNullDeleter());
    shared_ptr<XdmfUnstructuredGrid> generatedGrid = XdmfUnstructuredGrid::New(originGrid);
    return (XDMFUNSTRUCTUREDGRID *)((void *)((XdmfItem *)(new XdmfUnstructuredGrid(*generatedGrid.get()))));
  }
  catch (...)
  {
    // Here it works when classed directly to the grid type,
    // in other cases this may not work.
    XdmfItem * tempPointer = (XdmfItem *)regularGrid;
    XdmfRegularGrid * classedPointer = dynamic_cast<XdmfRegularGrid *>(tempPointer);
    shared_ptr<XdmfRegularGrid> originGrid = shared_ptr<XdmfRegularGrid>(classedPointer, XdmfNullDeleter());
    shared_ptr<XdmfUnstructuredGrid> generatedGrid = XdmfUnstructuredGrid::New(originGrid);
    return (XDMFUNSTRUCTUREDGRID *)((void *)((XdmfItem *)(new XdmfUnstructuredGrid(*generatedGrid.get()))));
  }
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

XDMFGEOMETRY * XdmfUnstructuredGridGetGeometry(XDMFUNSTRUCTUREDGRID * grid)
{
  XdmfItem * tempPointer = (XdmfItem *)grid;
  XdmfUnstructuredGrid * classedPointer = dynamic_cast<XdmfUnstructuredGrid *>(tempPointer);
  return (XDMFGEOMETRY *)((void *)(classedPointer->getGeometry().get()));
}

XDMFTOPOLOGY * XdmfUnstructuredGridGetTopology(XDMFUNSTRUCTUREDGRID * grid)
{
  XdmfItem * tempPointer = (XdmfItem *)grid;
  XdmfUnstructuredGrid * classedPointer = dynamic_cast<XdmfUnstructuredGrid *>(tempPointer);
  return (XDMFTOPOLOGY *)((void *)(classedPointer->getTopology().get()));
}

void XdmfUnstructuredGridSetGeometry(XDMFUNSTRUCTUREDGRID * grid, XDMFGEOMETRY * geometry, int passControl)
{
  XdmfItem * tempPointer = (XdmfItem *)grid;
  XdmfUnstructuredGrid * classedPointer = dynamic_cast<XdmfUnstructuredGrid *>(tempPointer);
  if (passControl) {
    classedPointer->setGeometry(shared_ptr<XdmfGeometry>((XdmfGeometry *)geometry));
  }
  else {
    classedPointer->setGeometry(shared_ptr<XdmfGeometry>((XdmfGeometry *)geometry, XdmfNullDeleter()));
  }
}

void XdmfUnstructuredGridSetTopology(XDMFUNSTRUCTUREDGRID * grid, XDMFTOPOLOGY * topology, int passControl)
{
  XdmfItem * tempPointer = (XdmfItem *)grid;
  XdmfUnstructuredGrid * classedPointer = dynamic_cast<XdmfUnstructuredGrid *>(tempPointer);
  if (passControl) {
    classedPointer->setTopology(shared_ptr<XdmfTopology>((XdmfTopology *)topology));
  }
  else {
    classedPointer->setTopology(shared_ptr<XdmfTopology>((XdmfTopology *)topology, XdmfNullDeleter()));
  }
}

XDMF_ITEM_C_CHILD_WRAPPER(XdmfUnstructuredGrid, XDMFUNSTRUCTUREDGRID)
XDMF_GRID_C_CHILD_WRAPPER(XdmfUnstructuredGrid, XDMFUNSTRUCTUREDGRID)
