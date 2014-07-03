/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfCurvilinearGrid.hpp                                             */
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

#ifndef XDMFCURVILINEARGRID_HPP_
#define XDMFCURVILINEARGRID_HPP_

// Forward Declarations
class XdmfArray;

// Includes
#include "Xdmf.hpp"
#include "XdmfGrid.hpp"

/**
 * @brief A curvilinear (or structured) grid consisting of cells and
 * points arranged on a regular lattice in space.
 *
 * XdmfCurvilinearGrid represents a mesh of cells and points arranged
 * with regular topology and irregular geometry.
 *
 * In order to define a curvilinear grid, the dimensions of the grid
 * must be supplied along with the coordinates of each point.
 *
 */
class XDMF_EXPORT XdmfCurvilinearGrid : public XdmfGrid {

public:

  /**
   * Create a new curvilinear grid (Two dimensional).
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfCurvilinearGrid.cpp
   * @skipline //#initializationdim2
   * @until //#initializationdim2
   *
   * Python
   *
   * @dontinclude XdmfExampleCurvilinearGrid.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//constructor2
   * @until #//constructor2
   *
   * @param     xNumPoints      The number of points in the x direction.
   * @param     yNumPoints      The number of points in the y direction.
   *
   * @return                    Constructed curvilinear grid.
   */
  static shared_ptr<XdmfCurvilinearGrid>
  New(const unsigned int xNumPoints,
      const unsigned int yNumPoints);

  /**
   * Create a new curvilinear grid (Three dimensional).
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfCurvilinearGrid.cpp
   * @skipline //#initializationdim3
   * @until //#initializationdim3
   *
   * Python
   *
   * @dontinclude XdmfExampleCurvilinearGrid.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//constructor3
   * @until #//constructor3
   *
   * @param     xNumPoints      The number of points in the x direction.
   * @param     yNumPoints      The number of points in the y direction.
   * @param     zNumPoints      The number of points in the z direction.
   *
   * @return                    Constructed curvilinear grid.
   */
  static shared_ptr<XdmfCurvilinearGrid>
  New(const unsigned int xNumPoints,
      const unsigned int yNumPoints,
      const unsigned int zNumPoints);

  /**
   * Create a new curvilinear grid (N dimensional).
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfCurvilinearGrid.cpp
   * @skipline //#initializationvector
   * @until //#initializationvector
   *
   * Python
   *
   * @dontinclude XdmfExampleCurvilinearGrid.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//constructorvector
   * @until #//constructorvector
   *
   * @param     numPoints       The number of points in each direction.
   *
   * @return                    Constructed curvilinear grid.
   */
  static shared_ptr<XdmfCurvilinearGrid>
  New(const shared_ptr<XdmfArray> numPoints);

  virtual ~XdmfCurvilinearGrid();

  LOKI_DEFINE_VISITABLE(XdmfCurvilinearGrid, XdmfGrid);
  static const std::string ItemTag;

  /**
   * Get the dimensions of the grid, the number of points in each
   * direction.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfCurvilinearGrid.cpp
   * @skipline //#initializationdim3
   * @until //#initializationdim3
   * @skipline //#setDimensions
   * @until //#setDimensions
   * @skipline //#getDimensions
   * @until //#getDimensions
   *
   * Python
   *
   * @dontinclude XdmfExampleCurvilinearGrid.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//constructorvector
   * @until #//constructorvector
   * @skipline #//setDimensions
   * @until #//setDimensions
   * @skipline #//getDimensions
   * @until #//getDimensions
   *
   * @return    XdmfArray containing dimensions of this grid.
   */
  shared_ptr<XdmfArray> getDimensions();

  /**
   * Get the dimensions of the grid, the number of points in each
   * direction (const version).
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfCurvilinearGrid.cpp
   * @skipline //#initializationdim3
   * @until //#initializationdim3
   * @skipline //#setDimensions
   * @until //#setDimensions
   * @skipline //#getDimensionsconst
   * @until //#getDimensionsconst
   *
   * Python: Python doesn't have a constant version
   *
   * @return    XdmfArray containing the dimensions of this grid.
   */
  shared_ptr<const XdmfArray> getDimensions() const;

  /**
   * Get the geometry associated with this grid.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfCurvilinearGrid.cpp
   * @skipline //#initializationdim3
   * @until //#initializationdim3
   * @skipline //#setGeometry
   * @until //#setGeometry
   * @skipline //#getGeometry
   * @until //#getGeometry
   *
   * Python
   *
   * @dontinclude XdmfExampleCurvilinearGrid.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//constructorvector
   * @until #//constructorvector
   * @skipline #//setGeometry
   * @until #//setGeometry
   * @skipline #//getGeometry
   * @until #//getGeometry
   *
   * @return    The geometry associated with this grid.
   */
  shared_ptr<XdmfGeometry> getGeometry();

  /**
   * Set the dimensions of the grid, the number of points in each
   * direction.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfCurvilinearGrid.cpp
   * @skipline //#initializationdim3
   * @until //#initializationdim3
   * @skipline //#setDimensions
   * @until //#setDimensions
   *
   * Python
   *
   * @dontinclude XdmfExampleCurvilinearGrid.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//constructorvector
   * @until #//constructorvector
   * @skipline #//setDimensions
   * @until #//setDimensions
   *
   * @param     dimensions      The dimension of the grid.
   */
  void setDimensions(const shared_ptr<XdmfArray> dimensions);

  /**
   * Set the geometry associated with this grid.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfCurvilinearGrid.cpp
   * @skipline //#initializationdim3
   * @until //#initializationdim3
   * @skipline //#setGeometry
   * @until //#setGeometry
   *
   * Python
   *
   * @dontinclude XdmfExampleCurvilinearGrid.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//constructorvector
   * @until #//constructorvector
   * @skipline #//setGeometry
   * @until #//setGeometry
   *
   * @param     geometry        An XdmfGeometry to associate with this grid.
   */
  void setGeometry(const shared_ptr<XdmfGeometry> geometry);

protected:

  XdmfCurvilinearGrid(const shared_ptr<XdmfArray> numPoints);

  void
  populateItem(const std::map<std::string, std::string> & itemProperties,
               const std::vector<shared_ptr<XdmfItem> > & childItems,
               const XdmfCoreReader * const reader);

private:

  /**
   * PIMPL
   */
  class XdmfCurvilinearGridImpl;

  XdmfCurvilinearGrid(const XdmfCurvilinearGrid &);  // Not implemented.
  void operator=(const XdmfCurvilinearGrid &);  // Not implemented.

  XdmfCurvilinearGridImpl * mImpl;

};

#ifdef _WIN32
XDMF_TEMPLATE template class XDMF_EXPORT
shared_ptr<XdmfGeometry>;
XDMF_TEMPLATE template class XDMF_EXPORT
shared_ptr<XdmfArray>;
XDMF_TEMPLATE template class XDMF_EXPORT
shared_ptr<const XdmfArray>;
#endif

#endif /* XDMFCURVILINEARGRID_HPP_ */
