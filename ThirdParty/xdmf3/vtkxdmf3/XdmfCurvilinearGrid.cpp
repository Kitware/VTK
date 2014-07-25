/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfCurviliniearGrid.cpp                                            */
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

#include <cmath>
#include "XdmfArray.hpp"
#include "XdmfCurvilinearGrid.hpp"
#include "XdmfGeometry.hpp"
#include "XdmfTopology.hpp"
#include "XdmfTopologyType.hpp"
#include "XdmfError.hpp"

/**
 * PIMPL
 */
class XdmfCurvilinearGrid::XdmfCurvilinearGridImpl {

public:

  class XdmfTopologyCurvilinear : public XdmfTopology
  {

  public:

    static shared_ptr<XdmfTopologyCurvilinear>
    New(const XdmfCurvilinearGrid * const curvilinearGrid)
    {
      shared_ptr<XdmfTopologyCurvilinear> 
        p(new XdmfTopologyCurvilinear(curvilinearGrid));
      return p;
    }

    unsigned int
    getNumberElements() const
    {
      const shared_ptr<const XdmfArray> dimensions = 
        mCurvilinearGrid->getDimensions();
      if(dimensions->getSize() == 0) {
        return 0;
      }
      unsigned int toReturn = 1;
      for(unsigned int i=0; i<dimensions->getSize(); ++i) {
        toReturn *= (dimensions->getValue<unsigned int>(i) - 1);
      }
      return toReturn;
    }

  private:

    XdmfTopologyCurvilinear(const XdmfCurvilinearGrid * const curvilinearGrid) :
      mCurvilinearGrid(curvilinearGrid)
    {
      this->setType(XdmfTopologyTypeCurvilinear::New(curvilinearGrid));
    }

    const XdmfCurvilinearGrid * const mCurvilinearGrid;
  };

  class XdmfTopologyTypeCurvilinear : public XdmfTopologyType
  {

  public:

    static shared_ptr<const XdmfTopologyTypeCurvilinear>
    New(const XdmfCurvilinearGrid * const curvilinearGrid)
    {
      shared_ptr<const XdmfTopologyTypeCurvilinear>
        p(new XdmfTopologyTypeCurvilinear(curvilinearGrid));
      return p;
    }

    unsigned int
    getEdgesPerElement() const
    {
      const unsigned int dimensions = 
        mCurvilinearGrid->getDimensions()->getSize();
      if(dimensions == 2) {
        return 4;
      }
      else if(dimensions == 3) {
        return 12;
      }
      else {
        XdmfError::message(XdmfError::FATAL, 
                           "Grid dimensions not 2 or 3 in "
                           "XdmfTopologyTypeCurvilinear::getEdgesPerElement");
      }
      return 0;
    }

    unsigned int
    getFacesPerElement() const
    {
      const unsigned int dimensions = 
        mCurvilinearGrid->getDimensions()->getSize();
      if(dimensions == 2) {
        return 1;
      }
      else if(dimensions == 3) {
        return 6;
      }
      else {
        XdmfError::message(XdmfError::FATAL, 
                           "Grid dimensions not 2 or 3 in "
                           "XdmfTopologyTypeCurvilinear::getFacesPerElement");
      }
      return 0;
    }

    unsigned int
    getNodesPerElement() const
    {
      // 2^Dimensions
      // e.g. 1D = 2 nodes per element and 2D = 4 nodes per element.
      return (unsigned int)
        std::pow(2, (double)mCurvilinearGrid->getDimensions()->getSize());
    }

    void
    getProperties(std::map<std::string, std::string> & collectedProperties) const
    {
      shared_ptr<const XdmfArray> dimensions =
        mCurvilinearGrid->getDimensions();
      if(dimensions->getSize() == 3) {
        collectedProperties["Type"] = "3DSMesh";
      }
      else if(dimensions->getSize() == 2) {
        collectedProperties["Type"] = "2DSMesh";
      }
      else {
        XdmfError::message(XdmfError::FATAL, 
                           "Grid dimensions not 2 or 3 in "
                           "XdmfTopologyTypeCurvilinear::getProperties");
      }
      collectedProperties["Dimensions"] = dimensions->getValuesString();
    }

  private:

    XdmfTopologyTypeCurvilinear(const XdmfCurvilinearGrid * const curvilinearGrid) :
      XdmfTopologyType(0,
                       0,
                       std::vector<shared_ptr<const XdmfTopologyType> >(),
                       0,
                       "foo",
                       XdmfTopologyType::Structured,
                       0x1110),
      mCurvilinearGrid(curvilinearGrid)
    {
    }

    const XdmfCurvilinearGrid * const mCurvilinearGrid;

  };

  XdmfCurvilinearGridImpl(const shared_ptr<XdmfArray> numPoints) :
    mDimensions(numPoints)
  {
  }

  shared_ptr<XdmfArray> mDimensions;

};

shared_ptr<XdmfCurvilinearGrid>
XdmfCurvilinearGrid::New(const unsigned int xNumPoints,
                         const unsigned int yNumPoints)
{
  shared_ptr<XdmfArray> numPoints = XdmfArray::New();
  numPoints->resize<unsigned int>(2);
  numPoints->insert(0, xNumPoints);
  numPoints->insert(1, yNumPoints);
  shared_ptr<XdmfCurvilinearGrid> p(new XdmfCurvilinearGrid(numPoints));
  return p;
}

shared_ptr<XdmfCurvilinearGrid>
XdmfCurvilinearGrid::New(const unsigned int xNumPoints,
                         const unsigned int yNumPoints,
                         const unsigned int zNumPoints)
{
  shared_ptr<XdmfArray> numPoints = XdmfArray::New();
  numPoints->resize<unsigned int>(3);
  numPoints->insert(0, xNumPoints);
  numPoints->insert(1, yNumPoints);
  numPoints->insert(2, zNumPoints);
  shared_ptr<XdmfCurvilinearGrid> p(new XdmfCurvilinearGrid(numPoints));
  return p;
}

shared_ptr<XdmfCurvilinearGrid>
XdmfCurvilinearGrid::New(const shared_ptr<XdmfArray> numPoints)
{
  shared_ptr<XdmfCurvilinearGrid> p(new XdmfCurvilinearGrid(numPoints));
  return p;
}

XdmfCurvilinearGrid::XdmfCurvilinearGrid(const shared_ptr<XdmfArray> numPoints) :
  XdmfGrid(XdmfGeometry::New(),
           XdmfCurvilinearGridImpl::XdmfTopologyCurvilinear::New(this)),
  mImpl(new XdmfCurvilinearGridImpl(numPoints))
{
}

XdmfCurvilinearGrid::~XdmfCurvilinearGrid()
{
  if (mImpl) {
    delete mImpl;
  }
  mImpl = NULL;
}

const std::string XdmfCurvilinearGrid::ItemTag = "Grid";

shared_ptr<XdmfArray>
XdmfCurvilinearGrid::getDimensions()
{
  return boost::const_pointer_cast<XdmfArray>
    (static_cast<const XdmfCurvilinearGrid &>(*this).getDimensions());
}

shared_ptr<const XdmfArray>
XdmfCurvilinearGrid::getDimensions() const
{
  return mImpl->mDimensions;
}

shared_ptr<XdmfGeometry>
XdmfCurvilinearGrid::getGeometry()
{
  return boost::const_pointer_cast<XdmfGeometry>
    (static_cast<const XdmfGrid &>(*this).getGeometry());
}

void
XdmfCurvilinearGrid::populateItem(const std::map<std::string, std::string> & itemProperties,
                                  const std::vector<shared_ptr<XdmfItem> > & childItems,
                                  const XdmfCoreReader * const reader)
{
  XdmfGrid::populateItem(itemProperties, childItems, reader);

  for(std::vector<shared_ptr<XdmfItem> >::const_iterator iter =
        childItems.begin();
      iter != childItems.end();
      ++iter) {
    if(shared_ptr<XdmfCurvilinearGrid> curvilinearGrid =
       shared_dynamic_cast<XdmfCurvilinearGrid>(*iter)) {
      mImpl->mDimensions = curvilinearGrid->getDimensions();
    }
  }
}

void
XdmfCurvilinearGrid::setDimensions(const shared_ptr<XdmfArray> dimensions)
{
  mImpl->mDimensions = dimensions;
}

void
XdmfCurvilinearGrid::setGeometry(const shared_ptr<XdmfGeometry> geometry)
{
  mGeometry = geometry;
}

