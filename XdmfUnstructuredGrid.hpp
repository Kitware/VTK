/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfUnstructuredGrid.hpp                                            */
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

#ifndef XDMFUNSTRUCTUREDGRID_HPP_
#define XDMFUNSTRUCTUREDGRID_HPP_

// Forward Declarations
class XdmfRegularGrid;

// Includes
#include "Xdmf.hpp"
#include "XdmfGrid.hpp"

/**
 * @brief An unstructured grid that consists of elements, points, and
 * fields attached to the mesh.
 *
 * After creating an unstructured grid, the XdmfGeometry and
 * XdmfTopology must be set. The XdmfTopology describes the element
 * types contained in the grid and their connectivity. The
 * XdmfGeometry describes the positions of nodes.
 */
class XDMF_EXPORT XdmfUnstructuredGrid : public XdmfGrid {

public:

  /**
   * Create a new XdmfUnstructuredGrid.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfUnstructuredGrid.cpp
   * @skipline //#initialization
   * @until //#initialization
   *
   * Python
   *
   * @dontinclude XdmfExampleUnstructuredGrid.py
   * @skipline #//initialization
   * @until #//initialization
   *
   * @return    Constructed XdmfUnstructuredGrid.
   */
  static shared_ptr<XdmfUnstructuredGrid> New();

  /**
   * Create a new XdmfUnstructuredGrid from a XdmfRegularGrid.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfUnstructuredGrid.cpp
   * @skipline //#initializationregular
   * @until //#initializationregular
   *
   * Python
   *
   * @dontinclude XdmfExampleUnstructuredGrid.py
   * @skipline #//initializationregular
   * @until #//initializationregular
   *
   * @param     regularGrid     The grid that the unstructured grid will be created from
   *
   * @return                    Constructed XdmfUnstructuredGrid.
   */
  static shared_ptr<XdmfUnstructuredGrid> 
  New(const shared_ptr<XdmfRegularGrid> regularGrid);

  virtual ~XdmfUnstructuredGrid();

  static const std::string ItemTag;

  /**
   * Get the geometry associated with this grid.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfUnstructuredGrid.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setGeometry
   * @until //#setGeometry
   * @skipline //#getGeometry
   * @until //#getGeometry
   *
   * Python
   *
   * @dontinclude XdmfExampleUnstructuredGrid.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setGeometry
   * @until #//setGeometry
   * @skipline #//getGeometry
   * @until #//getGeometry
   *
   * @return    The geometry associated with this grid.
   */
  shared_ptr<XdmfGeometry> getGeometry();

  virtual std::string getItemTag() const;

  /**
   * Get the topology associated with this grid.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfUnstructuredGrid.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setTopology
   * @until //#setTopology
   * @skipline //#getTopology
   * @until //#getTopology
   *
   * Python
   *
   * @dontinclude XdmfExampleUnstructuredGrid.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setTopology
   * @until #//setTopology
   * @skipline #//getTopology
   * @until #//getTopology
   *
   * @return    The topology associated with this grid.
   */
  shared_ptr<XdmfTopology> getTopology();

  /**
   * Set the geometry associated with this grid.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfUnstructuredGrid.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setGeometry
   * @until //#setGeometry
   *
   * Python
   *
   * @dontinclude XdmfExampleUnstructuredGrid.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setGeometry
   * @until #//setGeometry
   *
   * @param     geometry        An XdmfGeometry to associate with this grid.
   */
  void setGeometry(const shared_ptr<XdmfGeometry> geometry);

  /**
   * Set the topology associated with this grid.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfUnstructuredGrid.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setTopology
   * @until //#setTopology
   *
   * Python
   *
   * @dontinclude XdmfExampleUnstructuredGrid.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setTopology
   * @until #//setTopology
   *
   * @param     topology        An XdmfTopology to associate with this grid.
   */
  void setTopology(const shared_ptr<XdmfTopology> topology);

protected:

  XdmfUnstructuredGrid();
  XdmfUnstructuredGrid(const shared_ptr<XdmfRegularGrid> regularGrid);

private:

  XdmfUnstructuredGrid(const XdmfUnstructuredGrid &);  // Not implemented.
  void operator=(const XdmfUnstructuredGrid &);  // Not implemented.

};

#ifdef _WIN32
XDMF_TEMPLATE template class XDMF_EXPORT
shared_ptr<XdmfTopology>;
XDMF_TEMPLATE template class XDMF_EXPORT
shared_ptr<XdmfGeometry>;
#endif

#endif /* XDMFUNSTRUCTUREDGRID_HPP_ */
