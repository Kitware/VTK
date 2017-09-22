/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfGrid.hpp                                                        */
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

#ifndef XDMFGRID_HPP_
#define XDMFGRID_HPP_

// C Compatible Includes
#include "Xdmf.hpp"
#include "XdmfItem.hpp"
#include "XdmfAttribute.hpp"
#include "XdmfGeometry.hpp"
#include "XdmfGridController.hpp"
#include "XdmfTopology.hpp"
#include "XdmfMap.hpp"
#include "XdmfSet.hpp"

#ifdef __cplusplus

// Forward Declarations
class XdmfSubGrid;
class XdmfTime;

/**
 * @brief A mesh containing elements, points, and fields attached to
 * the mesh.
 *
 * XdmfGrid represents a mesh. It is required to contain two other
 * Xdmf data structures, an XdmfGeometry that stores point locations
 * and an XdmfTopology that store connectivity
 * information. XdmfAttributes can be inserted into the XdmfGrid to
 * specify fields centered on various parts of the mesh.  XdmfSets can
 * be inserted into XdmfGrids to specify collections of mesh elements.
 *
 * XdmfGrid is an abstract base class. There are several
 * implementations for representing both structured and unstructured
 * grids.
 */
class XDMF_EXPORT XdmfGrid : public virtual XdmfItem {

public:

  virtual ~XdmfGrid();

  LOKI_DEFINE_VISITABLE(XdmfGrid, XdmfItem)
  XDMF_CHILDREN(XdmfGrid, XdmfAttribute, Attribute, Name)
  XDMF_CHILDREN(XdmfGrid, XdmfSet, Set, Name)
  XDMF_CHILDREN(XdmfGrid, XdmfMap, Map, Name)
  static const std::string ItemTag;

  /**
   * Get the geometry associated with this grid.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfGrid.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getGeometry
   * @until //#getGeometry
   *
   * Python
   *
   * @dontinclude XdmfExampleGrid.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getGeometry
   * @until #//getGeometry
   *
   * @return    The geometry associated with this grid.
   */
  virtual shared_ptr<const XdmfGeometry> getGeometry() const;

  /**
   * Gets the current external reference for this grid.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfGrid.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setGridController
   * @until //#setGridController
   * @skipline //#getGridController
   * @until //#getGridController
   *
   * Python
   *
   * @dontinclude XdmfExampleGrid.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setGridController
   * @until #//setGridController
   * @skipline #//getGridController
   * @until #//getGridController
   *
   * @return    The current reference.
   */
  shared_ptr<XdmfGridController> getGridController();

  std::map<std::string, std::string> getItemProperties() const;

  virtual std::string getItemTag() const;

  /**
   * Get the name of the grid.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfGrid.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setName
   * @until //#setName
   * @skipline //#getName
   * @until //#getName
   *
   * Python
   *
   * @dontinclude XdmfExampleGrid.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setName
   * @until #//setName
   * @skipline #//getName
   * @until #//getName
   *
   * @return    The name of the grid.
   */
  std::string getName() const;

  /**
   * Get the time associated with this grid.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfGrid.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setTime
   * @until //#setTime
   * @skipline //#getTime
   * @until //#getTime
   *
   * Python
   *
   * @dontinclude XdmfExampleGrid.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setTime
   * @until #//setTime
   * @skipline #//getTime
   * @until #//getTime
   *
   * @return    Pointer to the XdmfTime attached to this grid. If no
   *            XdmfTime is attached, return a NULL pointer.
   */
  virtual shared_ptr<XdmfTime> getTime();

  /**
   * Get the time associated with this grid (const version).
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfGrid.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setTime
   * @until //#setTime
   * @skipline //#getTimeconst
   * @until //#getTimeconst
   *
   * Python: Python doesn't have a constant version
   * 
   * @return    Pointer to the XdmfTime attached to this grid. If no
   *            XdmfTime is attached, return a NULL pointer.
   */
  virtual shared_ptr<const XdmfTime> getTime() const;

  /**
   * Get the topology associated with this grid.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfGrid.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getTopology
   * @until //#getTopology
   *
   * Python
   *
   * @dontinclude XdmfExampleGrid.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getTopology
   * @until #//getTopology
   *
   * @return    The topology associated with this grid.
   */
  virtual shared_ptr<const XdmfTopology> getTopology() const;

  using XdmfItem::insert;

  /**
   * Reads the tree structure fromt he grid controller set to this grid
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfGrid.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#read
   * @until //#read
   *
   * Python
   *
   * @dontinclude XdmfExampleGrid.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//read
   * @until #//read
   */
  virtual void read();

  /**
   * Releases the grid structure that this grid contains.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfGrid.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#release
   * @until //#release
   *
   * Python
   *
   * @dontinclude XdmfExampleGrid.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//release
   * @until #//release
   */
  virtual void release();

  /**
   * Sets the reference to an external xdmf tree from which to populate the grid.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfGrid.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setGridController
   * @until //#setGridController
   *
   * Python
   *
   * @dontinclude XdmfExampleGrid.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setGridController
   * @until #//setGridController
   *
   * @param     newController   A reference to the external tree.
   */
  void setGridController(shared_ptr<XdmfGridController> newController);

  /**
   * Set the name of the grid.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfGrid.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setName
   * @until //#setName
   *
   * Python
   *
   * @dontinclude XdmfExampleGrid.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setName
   * @until #//setName
   *
   * @param     name    Name of the grid to set.
   */
  void setName(const std::string & name);

  /**
   * Set the time associated with this grid.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfGrid.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setTime
   * @until //#setTime
   *
   * Python
   *
   * @dontinclude XdmfExampleGrid.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setTime
   * @until #//setTime
   *
   * @param     time    An XdmfTime to associate with this grid.
   */
  virtual void setTime(const shared_ptr<XdmfTime> time);

  virtual void traverse(const shared_ptr<XdmfBaseVisitor> visitor);

protected:

  XdmfGrid(const shared_ptr<XdmfGeometry> geometry,
           const shared_ptr<XdmfTopology> topology,
           const std::string & name = "Grid");

  virtual void
  copyGrid(shared_ptr<XdmfGrid> sourceGrid);

  virtual void
  populateItem(const std::map<std::string, std::string> & itemProperties,
               const std::vector<shared_ptr<XdmfItem> > & childItems,
               const XdmfCoreReader * const reader);

  shared_ptr<XdmfGeometry> mGeometry;
  shared_ptr<XdmfTopology> mTopology;

  shared_ptr<XdmfGridController> mGridController;

private:

  XdmfGrid(const XdmfGrid &);  // Not implemented.
  void operator=(const XdmfGrid &);  // Not implemented.

  std::string mName;
  shared_ptr<XdmfTime> mTime;

};

#endif

#ifdef __cplusplus
extern "C" {
#endif

// C wrappers go here

#ifndef XDMFGRIDCDEFINE
#define XDMFGRIDCDEFINE
struct XDMFGRID; // Simply as a typedef to ensure correct typing
typedef struct XDMFGRID XDMFGRID;
#endif

XDMF_EXPORT XDMFATTRIBUTE * XdmfGridGetAttribute(XDMFGRID * grid, unsigned int index);

XDMF_EXPORT XDMFATTRIBUTE * XdmfGridGetAttributeByName(XDMFGRID * grid, char * Name);

XDMF_EXPORT unsigned int XdmfGridGetNumberAttributes(XDMFGRID * grid);

XDMF_EXPORT void XdmfGridInsertAttribute(XDMFGRID * grid, XDMFATTRIBUTE * Attribute, int passControl);

XDMF_EXPORT void XdmfGridRemoveAttribute(XDMFGRID * grid, unsigned int index);

XDMF_EXPORT void XdmfGridRemoveAttributeByName(XDMFGRID * grid, char * Name);

XDMF_EXPORT XDMFSET * XdmfGridGetSet(XDMFGRID * grid, unsigned int index);

XDMF_EXPORT XDMFSET * XdmfGridGetSetByName(XDMFGRID * grid, char * Name);

XDMF_EXPORT unsigned int XdmfGridGetNumberSets(XDMFGRID * grid);

XDMF_EXPORT void XdmfGridInsertSet(XDMFGRID * grid, XDMFSET * Set, int passControl);

XDMF_EXPORT void XdmfGridRemoveSet(XDMFGRID * grid, unsigned int index);

XDMF_EXPORT void XdmfGridRemoveSetByName(XDMFGRID * grid, char * Name);

XDMF_EXPORT XDMFMAP * XdmfGridGetMap(XDMFGRID * grid, unsigned int index);

XDMF_EXPORT XDMFMAP * XdmfGridGetMapByName(XDMFGRID * grid, char * Name);

XDMF_EXPORT unsigned int XdmfGridGetNumberMaps(XDMFGRID * grid); 

XDMF_EXPORT void XdmfGridInsertMap(XDMFGRID * grid, XDMFMAP * Map, int passControl);

XDMF_EXPORT void XdmfGridRemoveMap(XDMFGRID * grid, unsigned int index);

XDMF_EXPORT void XdmfGridRemoveMapByName(XDMFGRID * grid, char * Name);

XDMF_EXPORT XDMFGRIDCONTROLLER * XdmfGridGetGridController(XDMFGRID * grid);

XDMF_EXPORT char * XdmfGridGetName(XDMFGRID * grid);

XDMF_EXPORT XDMFTIME * XdmfGridGetTime(XDMFGRID * grid);

XDMF_EXPORT void XdmfGridRead(XDMFGRID * grid, int * status);

XDMF_EXPORT void XdmfGridRelease(XDMFGRID * grid);

XDMF_EXPORT void XdmfGridSetGridController(XDMFGRID * grid, XDMFGRIDCONTROLLER * controller, int passControl);

XDMF_EXPORT void XdmfGridSetName(XDMFGRID * grid, char * name, int * status);

XDMF_EXPORT void XdmfGridSetTime(XDMFGRID * grid, XDMFTIME * time, int passControl);

XDMF_ITEM_C_CHILD_DECLARE(XdmfGrid, XDMFGRID, XDMF)

#define XDMF_GRID_C_CHILD_DECLARE(ClassName, CClassName, Level)                                                        \
                                                                                                                       \
Level##_EXPORT XDMFATTRIBUTE * ClassName##GetAttribute(CClassName * grid, unsigned int index);                         \
Level##_EXPORT XDMFATTRIBUTE * ClassName##GetAttributeByName(CClassName * grid, char * Name);                          \
Level##_EXPORT unsigned int ClassName##GetNumberAttributes(CClassName * grid);                                         \
Level##_EXPORT void ClassName##InsertAttribute(CClassName * grid, XDMFATTRIBUTE * Attribute, int passControl);         \
Level##_EXPORT void ClassName##RemoveAttribute(CClassName * grid, unsigned int index);                                 \
Level##_EXPORT void ClassName##RemoveAttributeByName(CClassName * grid, char * Name);                                  \
Level##_EXPORT XDMFSET * ClassName##GetSet(CClassName * grid, unsigned int index);                                     \
Level##_EXPORT XDMFSET * ClassName##GetSetByName(CClassName * grid, char * Name);                                      \
Level##_EXPORT unsigned int ClassName##GetNumberSets(CClassName * grid);                                               \
Level##_EXPORT void ClassName##InsertSet(CClassName * grid, XDMFSET * Set, int passControl);                           \
Level##_EXPORT void ClassName##RemoveSet(CClassName * grid, unsigned int index);                                       \
Level##_EXPORT void ClassName##RemoveSetByName(CClassName * grid, char * Name);                                        \
Level##_EXPORT XDMFMAP * ClassName##GetMap(CClassName * grid, unsigned int index);                                     \
Level##_EXPORT XDMFMAP * ClassName##GetMapByName(CClassName * grid, char * Name);                                      \
Level##_EXPORT unsigned int ClassName##GetNumberMaps(CClassName * grid);                                               \
Level##_EXPORT void ClassName##InsertMap(CClassName * grid, XDMFMAP * Map, int passControl);                           \
Level##_EXPORT void ClassName##RemoveMap(CClassName * grid, unsigned int index);                                       \
Level##_EXPORT void ClassName##RemoveMapByName(CClassName * grid, char * Name);                                        \
Level##_EXPORT XDMFGRIDCONTROLLER * ClassName##GetGridController(CClassName * grid);                                   \
Level##_EXPORT char * ClassName##GetName(CClassName * grid);                                                           \
Level##_EXPORT XDMFTIME * ClassName##GetTime(CClassName * grid);                                                       \
Level##_EXPORT void ClassName##Read( CClassName * grid, int * status);                                                 \
Level##_EXPORT void ClassName##Release( CClassName * grid);                                                            \
Level##_EXPORT void ClassName##SetGridController(CClassName * grid, XDMFGRIDCONTROLLER * controller, int passControl); \
Level##_EXPORT void ClassName##SetName(CClassName * grid, char * name, int * status);                                  \
Level##_EXPORT void ClassName##SetTime(CClassName * grid, XDMFTIME * time, int passControl);



#define XDMF_GRID_C_CHILD_WRAPPER(ClassName, CClassName)                                                       \
                                                                                                               \
XDMFATTRIBUTE * ClassName##GetAttribute(CClassName * grid, unsigned int index)                                 \
{                                                                                                              \
  shared_ptr< XdmfGrid > baseGrid = *(shared_ptr< ClassName > *) grid;                                         \
  return XdmfGridGetAttribute((XDMFGRID *)((void *)&baseGrid), index);                                         \
}                                                                                                              \
                                                                                                               \
XDMFATTRIBUTE * ClassName##GetAttributeByName(CClassName * grid, char * Name)                                  \
{                                                                                                              \
  shared_ptr< XdmfGrid > baseGrid = *(shared_ptr< ClassName > *) grid;                                         \
  return XdmfGridGetAttributeByName((XDMFGRID *)((void *)&baseGrid), Name);                                         \
}                                                                                                              \
                                                                                                               \
unsigned int ClassName##GetNumberAttributes(CClassName * grid)                                                 \
{                                                                                                              \
  shared_ptr< XdmfGrid > baseGrid = *(shared_ptr< ClassName > *) grid;                                         \
  return XdmfGridGetNumberAttributes((XDMFGRID *)((void *)&baseGrid));                                              \
}                                                                                                              \
                                                                                                               \
void ClassName##InsertAttribute(CClassName * grid, XDMFATTRIBUTE * Attribute, int passControl)                 \
{                                                                                                              \
  shared_ptr< XdmfGrid > baseGrid = *(shared_ptr< ClassName > *) grid;                                         \
  XdmfGridInsertAttribute((XDMFGRID *)((void *)&baseGrid), Attribute, passControl);                                 \
}                                                                                                              \
                                                                                                               \
void ClassName##RemoveAttribute(CClassName * grid, unsigned int index)                                         \
{                                                                                                              \
  shared_ptr< XdmfGrid > baseGrid = *(shared_ptr< ClassName > *) grid;                                         \
  XdmfGridRemoveAttribute((XDMFGRID *)((void *)&baseGrid), index);                                                  \
}                                                                                                              \
                                                                                                               \
void ClassName##RemoveAttributeByName(CClassName * grid, char * Name)                                          \
{                                                                                                              \
  shared_ptr< XdmfGrid > baseGrid = *(shared_ptr< ClassName > *) grid;                                         \
  XdmfGridRemoveAttributeByName((XDMFGRID *)((void *)&baseGrid), Name);                                             \
}                                                                                                              \
                                                                                                               \
XDMFSET * ClassName##GetSet(CClassName * grid, unsigned int index)                                             \
{                                                                                                              \
  shared_ptr< XdmfGrid > baseGrid = *(shared_ptr< ClassName > *) grid;                                         \
  return XdmfGridGetSet((XDMFGRID *)((void *)&baseGrid), index);                                                    \
}                                                                                                              \
                                                                                                               \
XDMFSET * ClassName##GetSetByName(CClassName * grid, char * Name)                                              \
{                                                                                                              \
  shared_ptr< XdmfGrid > baseGrid = *(shared_ptr< ClassName > *) grid;                                         \
  return XdmfGridGetSetByName((XDMFGRID *)((void *)&baseGrid), Name);                                               \
}                                                                                                              \
                                                                                                               \
unsigned int ClassName##GetNumberSets(CClassName * grid)                                                       \
{                                                                                                              \
  shared_ptr< XdmfGrid > baseGrid = *(shared_ptr< ClassName > *) grid;                                         \
  return XdmfGridGetNumberSets((XDMFGRID *)((void *)&baseGrid));                                                    \
}                                                                                                              \
                                                                                                               \
void ClassName##InsertSet(CClassName * grid, XDMFSET * Set, int passControl)                                   \
{                                                                                                              \
  shared_ptr< XdmfGrid > baseGrid = *(shared_ptr< ClassName > *) grid;                                         \
  XdmfGridInsertSet((XDMFGRID *)((void *)&baseGrid), Set, passControl);                                             \
}                                                                                                              \
                                                                                                               \
void ClassName##RemoveSet(CClassName * grid, unsigned int index)                                               \
{                                                                                                              \
  shared_ptr< XdmfGrid > baseGrid = *(shared_ptr< ClassName > *) grid;                                         \
  XdmfGridRemoveSet((XDMFGRID *)((void *)&baseGrid), index);                                                        \
}                                                                                                              \
                                                                                                               \
void ClassName##RemoveSetByName(CClassName * grid, char * Name)                                                \
{                                                                                                              \
  shared_ptr< XdmfGrid > baseGrid = *(shared_ptr< ClassName > *) grid;                                         \
  XdmfGridRemoveSetByName((XDMFGRID *)((void *)&baseGrid), Name);                                                   \
}                                                                                                              \
                                                                                                               \
XDMFMAP * ClassName##GetMap(CClassName * grid, unsigned int index)                                             \
{                                                                                                              \
  shared_ptr< XdmfGrid > baseGrid = *(shared_ptr< ClassName > *) grid;                                         \
  return XdmfGridGetMap((XDMFGRID *)((void *)&baseGrid), index);                                                    \
}                                                                                                              \
                                                                                                               \
XDMFMAP * ClassName##GetMapByName(CClassName * grid, char * Name)                                              \
{                                                                                                              \
  shared_ptr< XdmfGrid > baseGrid = *(shared_ptr< ClassName > *) grid;                                         \
  return XdmfGridGetMapByName((XDMFGRID *)((void *)&baseGrid), Name);                                               \
}                                                                                                              \
                                                                                                               \
unsigned int ClassName##GetNumberMaps(CClassName * grid)                                                       \
{                                                                                                              \
  shared_ptr< XdmfGrid > baseGrid = *(shared_ptr< ClassName > *) grid;                                         \
  return XdmfGridGetNumberMaps((XDMFGRID *)((void *)&baseGrid));                                                    \
}                                                                                                              \
                                                                                                               \
void ClassName##InsertMap(CClassName * grid, XDMFMAP * Map, int passControl)                                   \
{                                                                                                              \
  shared_ptr< XdmfGrid > baseGrid = *(shared_ptr< ClassName > *) grid;                                         \
  XdmfGridInsertMap((XDMFGRID *)((void *)&baseGrid), Map, passControl);                                             \
}                                                                                                              \
                                                                                                               \
void ClassName##RemoveMap(CClassName * grid, unsigned int index)                                               \
{                                                                                                              \
  shared_ptr< XdmfGrid > baseGrid = *(shared_ptr< ClassName > *) grid;                                         \
  XdmfGridRemoveMap((XDMFGRID *)((void *)&baseGrid), index);                                                        \
}                                                                                                              \
                                                                                                               \
void ClassName##RemoveMapByName(CClassName * grid, char * Name)                                                \
{                                                                                                              \
  shared_ptr< XdmfGrid > baseGrid = *(shared_ptr< ClassName > *) grid;                                         \
  XdmfGridRemoveMapByName((XDMFGRID *)((void *)&baseGrid), Name);                                                   \
}                                                                                                              \
                                                                                                               \
XDMFGRIDCONTROLLER * ClassName##GetGridController(CClassName * grid)                                           \
{                                                                                                              \
  shared_ptr< XdmfGrid > baseGrid = *(shared_ptr< ClassName > *) grid;                                         \
  return XdmfGridGetGridController((XDMFGRID *)((void *)&baseGrid));                                                \
}                                                                                                              \
                                                                                                               \
char * ClassName##GetName(CClassName * grid)                                                                   \
{                                                                                                              \
  shared_ptr< XdmfGrid > baseGrid = *(shared_ptr< ClassName > *) grid;                                         \
  return XdmfGridGetName((XDMFGRID *)((void *)&baseGrid));                                                          \
}                                                                                                              \
                                                                                                               \
XDMFTIME * ClassName##GetTime(CClassName * grid)                                                               \
{                                                                                                              \
  shared_ptr< XdmfGrid > baseGrid = *(shared_ptr< ClassName > *) grid;                                         \
  return XdmfGridGetTime((XDMFGRID *)((void *)&baseGrid));                                                          \
}                                                                                                              \
                                                                                                               \
void                                                                                                           \
ClassName##Read( CClassName * grid, int * status)                                                              \
{                                                                                                              \
  shared_ptr< XdmfGrid > baseGrid = *(shared_ptr< ClassName > *) grid;                                         \
  XdmfGridRead((XDMFGRID *)((void *)&baseGrid), status);                                                            \
}                                                                                                              \
                                                                                                               \
void                                                                                                           \
ClassName##Release( CClassName * grid)                                                                         \
{                                                                                                              \
  shared_ptr< XdmfGrid > baseGrid = *(shared_ptr< ClassName > *) grid;                                         \
  XdmfGridRelease((XDMFGRID *)((void *)&baseGrid));                                                                 \
}                                                                                                              \
                                                                                                               \
void ClassName##SetGridController(CClassName * grid, XDMFGRIDCONTROLLER * controller, int passControl)         \
{                                                                                                              \
  shared_ptr< XdmfGrid > baseGrid = *(shared_ptr< ClassName > *) grid;                                         \
  XdmfGridSetGridController((XDMFGRID *)((void *)&baseGrid), controller, passControl);                              \
}                                                                                                              \
                                                                                                               \
void ClassName##SetName(CClassName * grid, char * name, int * status)                                          \
{                                                                                                              \
  shared_ptr< XdmfGrid > baseGrid = *(shared_ptr< ClassName > *) grid;                                         \
  XdmfGridSetName((XDMFGRID *)((void *)&baseGrid), name, status);                                                   \
}                                                                                                              \
                                                                                                               \
void ClassName##SetTime(CClassName * grid, XDMFTIME * time, int passControl)                                   \
{                                                                                                              \
  shared_ptr< XdmfGrid > baseGrid = *(shared_ptr< ClassName > *) grid;                                         \
  XdmfGridSetTime((XDMFGRID *)((void *)&baseGrid), time, passControl);                                              \
}

#ifdef __cplusplus
}
#endif

#endif /* XDMFGRID_HPP_ */
