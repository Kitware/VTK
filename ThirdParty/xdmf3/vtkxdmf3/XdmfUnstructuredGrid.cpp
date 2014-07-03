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
}

XdmfUnstructuredGrid::XdmfUnstructuredGrid(const shared_ptr<XdmfRegularGrid> regularGrid) :
  XdmfGrid(XdmfGeometry::New(), XdmfTopology::New())
{
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

XdmfUnstructuredGrid::~XdmfUnstructuredGrid()
{
}

const std::string XdmfUnstructuredGrid::ItemTag = "Grid";

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
XdmfUnstructuredGrid::setGeometry(const shared_ptr<XdmfGeometry> geometry)
{
  mGeometry = geometry;
}

void
XdmfUnstructuredGrid::setTopology(const shared_ptr<XdmfTopology> topology)
{
  mTopology = topology;
}
