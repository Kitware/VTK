/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfRegularGrid.hpp                                                 */
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

#ifndef XDMFREGULARGRID_HPP_
#define XDMFREGULARGRID_HPP_

// C Compatible Includes
#include "Xdmf.hpp"
#include "XdmfGrid.hpp"

#ifdef __cplusplus

// Forward Declarations
class XdmfArray;

/**
 * @brief A regular grid consists of congruent points arranged
 * regularly in space.
 *
 * XdmfRegularGrid represents a regular mesh of congruent points
 * arranged in space. In order to define a regular grid, three sets of
 * terms need to be supplied:
 *
 * Brick Size (Dx, Dy, (Dz)) - Size of an individual brick.
 * Dimensions (X, Y, (Z)) - Number of points in X, Y, and Z directions
 * Origin Location (X, Y, (Z)) - Location of the origin of the mesh in space.
 */
class XDMF_EXPORT XdmfRegularGrid : public XdmfGrid {

public:

  /**
   * Create a new structured grid (Two dimensional).
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfRegularGrid.cpp
   * @skipline //#initvalue
   * @until //#initvalue
   * @skipline //#initialization2
   * @until //#initialization2
   *
   * Python
   *
   * @dontinclude XdmfExampleRegularGrid.py
   * @skipline #//initvalue
   * @until #//initvalue
   * @skipline #//initialization2
   * @until #//initialization2
   *
   * @param     xBrickSize      The size of the brick in the x direction.
   * @param     yBrickSize      The size of the brick in the y direction.
   * @param     xNumPoints      The number of points in the x direction.
   * @param     yNumPoints      The number of points in the y direction.
   * @param     xOrigin         The x coordinate of the origin.
   * @param     yOrigin         The y coordinate of the origin.
   *
   * @return                    Constructed structured grid.
   */
  static shared_ptr<XdmfRegularGrid> New(const double xBrickSize,
                                         const double yBrickSize,
                                         const unsigned int xNumPoints,
                                         const unsigned int yNumPoints,
                                         const double xOrigin,
                                         const double yOrigin);

  /**
   * Create a new structured grid (Three dimensional).
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfRegularGrid.cpp
   * @skipline //#initvalue
   * @until //#initvalue
   * @skipline //#initialization3
   * @until //#initialization3
   *
   * Python
   *
   * @dontinclude XdmfExampleRegularGrid.py
   * @skipline #//initvalue
   * @until #//initvalue
   * @skipline #//initialization3
   * @until #//initialization3
   *
   * @param     xBrickSize      The size of the brick in the x direction.
   * @param     yBrickSize      The size of the brick in the y direction.
   * @param     zBrickSize      The size of the brick in the z direction.
   * @param     xNumPoints      The number of points in the x direction.
   * @param     yNumPoints      The number of points in the y direction.
   * @param     zNumPoints      The number of points in the z direction.
   * @param     xOrigin         The x coordinate of the origin.
   * @param     yOrigin         The y coordinate of the origin.
   * @param     zOrigin         The z coordinate of the origin.
   *
   * @return                    Constructed structured grid.
   */
  static shared_ptr<XdmfRegularGrid> New(const double xBrickSize,
                                         const double yBrickSize,
                                         const double zBrickSize,
                                         const unsigned int xNumPoints,
                                         const unsigned int yNumPoints,
                                         const unsigned int zNumPoints,
                                         const double xOrigin,
                                         const double yOrigin,
                                         const double zOrigin);

  /**
   * Create a new structured grid (N dimensional).
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfRegularGrid.cpp
   * @skipline //#initializationvector
   * @until //#initializationvector
   *
   * Python
   *
   * @dontinclude XdmfExampleRegularGrid.py
   * @skipline #//initializationvector
   * @until #//initializationvector
   *
   * @param     brickSize       The size of the brick in each direction.
   * @param     numPoints       The number of points in each direction.
   * @param     origin          The coordinates of the origin.
   *
   * @return                    Constructed structured grid.
   */
  static shared_ptr<XdmfRegularGrid>
  New(const shared_ptr<XdmfArray> brickSize,
      const shared_ptr<XdmfArray> numPoints,
      const shared_ptr<XdmfArray> origin);

  virtual ~XdmfRegularGrid();

  LOKI_DEFINE_VISITABLE(XdmfRegularGrid, XdmfGrid)
  static const std::string ItemTag;

  /**
   * Get the size of the bricks composing the grid.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfRegularGrid.cpp
   * @skipline //#initvalue
   * @until //#initvalue
   * @skipline //#initialization2
   * @until //#initialization2
   * @skipline //#getBrickSize
   * @until //#getBrickSize
   *
   * Python
   *
   * @dontinclude XdmfExampleRegularGrid.py
   * @skipline #//initvalue
   * @until #//initvalue
   * @skipline #//initialization2
   * @until #//initialization2
   * @skipline #//getBrickSize
   * @until #//getBrickSize
   *
   * @return    XdmfArray containing brick sizes for this grid.
   */
  shared_ptr<XdmfArray> getBrickSize();

  /**
   * Get the size of the bricks composing the grid (const version).
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfRegularGrid.cpp
   * @skipline //#initvalue
   * @until //#initvalue
   * @skipline //#initialization2
   * @until //#initialization2
   * @skipline //#getBrickSizeconst
   * @until //#getBrickSizeconst
   *
   * Python: Does not support a constant version of this function
   *
   * @return    XdmfArray containing brick sizes for this grid.
   */
  shared_ptr<const XdmfArray> getBrickSize() const;

  /**
   * Get the dimensions of the grid, the number of points in each
   * direction.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfRegularGrid.cpp
   * @skipline //#initvalue
   * @until //#initvalue
   * @skipline //#initialization2
   * @until //#initialization2
   * @skipline //#getDimensions
   * @until //#getDimensions
   *
   * Python
   *
   * @dontinclude XdmfExampleRegularGrid.py
   * @skipline #//initvalue
   * @until #//initvalue
   * @skipline #//initialization2
   * @until #//initialization2
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
   * @dontinclude ExampleXdmfRegularGrid.cpp
   * @skipline //#initvalue
   * @until //#initvalue
   * @skipline //#initialization2
   * @until //#initialization2
   * @skipline //#getDimensionsconst
   * @until //#getDimensionsconst
   *
   * Python: Does not support a constant version of this function
   *
   * @return    XdmfArray containing the dimensions of this grid.
   */
  shared_ptr<const XdmfArray> getDimensions() const;

  /**
   * Get the location of the origin of the grid.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfRegularGrid.cpp
   * @skipline //#initvalue
   * @until //#initvalue
   * @skipline //#initialization2
   * @until //#initialization2
   * @skipline //#getOrigin
   * @until //#getOrigin
   *
   * Python
   *
   * @dontinclude XdmfExampleRegularGrid.py
   * @skipline #//initvalue
   * @until #//initvalue
   * @skipline #//initialization2
   * @until #//initialization2
   * @skipline #//getOrigin
   * @until #//getOrigin
   *
   * @return    XdmfArray containing the location of the origin of the
   *            grid.
   */
  shared_ptr<XdmfArray> getOrigin();

  /**
   * Get the location of the origin of the grid.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfRegularGrid.cpp
   * @skipline //#initvalue
   * @until //#initvalue
   * @skipline //#initialization2
   * @until //#initialization2
   * @skipline //#getOriginconst
   * @until //#getOriginconst
   *
   * Python: Does not support a constant version of this function
   *
   * @return    XdmfArray containing the location of the origin of the
   *            grid (const version).
   */
  shared_ptr<const XdmfArray> getOrigin() const;

  virtual void read();

  virtual void release();

  /**
   * Set the size of the points composing the grid.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfRegularGrid.cpp
   * @skipline //#initializationvector
   * @until //#initializationvector
   * @skipline //#setBrickSize
   * @until //#setBrickSize
   *
   * Python
   *
   * @dontinclude XdmfExampleRegularGrid.py
   * @skipline #//initializationvector
   * @until #//initializationvector
   * @skipline #//setBrickSize
   * @until #//setBrickSize
   *
   * @param     brickSize       The sizes of the points composing the mesh. This
   *                            should have the same number of terms as the
   *                            dimensionality of the mesh.
   */
  void setBrickSize(const shared_ptr<XdmfArray> brickSize);

  /**
   * Set the dimensions of the grid, the number of points in each
   * direction.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfRegularGrid.cpp
   * @skipline //#initializationvector
   * @until //#initializationvector
   * @skipline //#setDimensions
   * @until //#setDimensions
   *
   * Python
   *
   * @dontinclude XdmfExampleRegularGrid.py
   * @skipline #//initializationvector
   * @until #//initializationvector
   * @skipline #//setDimensions
   * @until #//setDimensions
   *
   * @param     dimensions      The dimension of the grid.
   */
  void setDimensions(const shared_ptr<XdmfArray> dimensions);

  /**
   * Set the origin of the grid.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfRegularGrid.cpp
   * @skipline //#initializationvector
   * @until //#initializationvector
   * @skipline //#setOrigin
   * @until //#setOrigin
   *
   * Python
   *
   * @dontinclude XdmfExampleRegularGrid.py
   * @skipline #//initializationvector
   * @until #//initializationvector
   * @skipline #//setOrigin
   * @until #//setOrigin
   *
   * @param     origin  Location of the origin of the grid.  This should
   *                    have the same number of terms as the dimensionality
   *                    of the mesh.
   */
  void setOrigin(const shared_ptr<XdmfArray> origin);

  XdmfRegularGrid(XdmfRegularGrid &);

protected:

  XdmfRegularGrid(const shared_ptr<XdmfArray> brickSize,
                  const shared_ptr<XdmfArray> numPoints,
                  const shared_ptr<XdmfArray> origin);

  virtual void
  copyGrid(shared_ptr<XdmfGrid> sourceGrid);

  void populateItem(const std::map<std::string, std::string> & itemProperties,
                    const std::vector<shared_ptr<XdmfItem> > & childItems,
                    const XdmfCoreReader * const reader);

private:

  /**
   * PIMPL
   */
  class XdmfRegularGridImpl;

  XdmfRegularGrid(const XdmfRegularGrid &);  // Not implemented.
  void operator=(const XdmfRegularGrid &);  // Not implemented.

};

#endif

#ifdef __cplusplus
extern "C" {
#endif

// C wrappers go here

struct XDMFREGULARGRID; // Simply as a typedef to ensure correct typing
typedef struct XDMFREGULARGRID XDMFREGULARGRID;

XDMF_EXPORT XDMFREGULARGRID * XdmfRegularGridNew2D(double xBrickSize,
                                                   double yBrickSize,
                                                   unsigned int xNumPoints,
                                                   unsigned int yNumPoints,
                                                   double xOrigin,
                                                   double yOrigin);

XDMF_EXPORT XDMFREGULARGRID * XdmfRegularGridNew3D(double xBrickSize,
                                                   double yBrickSize,
                                                   double zBrickSize,
                                                   unsigned int xNumPoints,
                                                   unsigned int yNumPoints,
                                                   unsigned int zNumPoints,
                                                   double xOrigin,
                                                   double yOrigin,
                                                   double zOrigin);

XDMF_EXPORT XDMFREGULARGRID * XdmfRegularGridNew(XDMFARRAY * brickSize,
                                                 XDMFARRAY * numPoints,
                                                 XDMFARRAY * origin,
                                                 int passControl);

XDMF_EXPORT XDMFARRAY * XdmfRegularGridGetBrickSize(XDMFREGULARGRID * grid, int * status);

XDMF_EXPORT XDMFARRAY * XdmfRegularGridGetDimensions(XDMFREGULARGRID * grid, int * status);

XDMF_EXPORT XDMFARRAY * XdmfRegularGridGetOrigin(XDMFREGULARGRID * grid, int * status);

XDMF_EXPORT void XdmfRegularGridSetBrickSize(XDMFREGULARGRID * grid, XDMFARRAY * brickSize, int passControl, int * status);

XDMF_EXPORT void XdmfRegularGridSetDimensions(XDMFREGULARGRID * grid, XDMFARRAY * dimensions, int passControl, int * status);

XDMF_EXPORT void XdmfRegularGridSetOrigin(XDMFREGULARGRID * grid, XDMFARRAY * origin, int passControl, int * status);

XDMF_ITEM_C_CHILD_DECLARE(XdmfRegularGrid, XDMFREGULARGRID, XDMF)
XDMF_GRID_C_CHILD_DECLARE(XdmfRegularGrid, XDMFREGULARGRID, XDMF)

#ifdef __cplusplus
}
#endif

#endif /* XDMFREGULARGRID_HPP_ */
