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

  XdmfGrid(XdmfGrid &);

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

  class XdmfGridImpl
  {
    public:
    XdmfGridImpl()
    {
    }

    ~XdmfGridImpl()
    {
    }

    virtual XdmfGridImpl * duplicate() = 0;

    std::string getGridType() const
    {
      return mGridType;
    }

    std::string mGridType;
  };

  XdmfGridImpl * mImpl;

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
XDMFATTRIBUTE * ClassName##GetAttribute(CClassName * grid, unsigned int index)                                 \
{                                                                                                              \
  return XdmfGridGetAttribute((XDMFGRID *)((void *)grid), index);                                              \
}                                                                                                              \
                                                                                                               \
XDMFATTRIBUTE * ClassName##GetAttributeByName(CClassName * grid, char * Name)                                  \
{                                                                                                              \
  return XdmfGridGetAttributeByName((XDMFGRID *)((void *)grid), Name);                                         \
}                                                                                                              \
                                                                                                               \
unsigned int ClassName##GetNumberAttributes(CClassName * grid)                                                 \
{                                                                                                              \
  return XdmfGridGetNumberAttributes((XDMFGRID *)((void *)grid));                                              \
}                                                                                                              \
                                                                                                               \
void ClassName##InsertAttribute(CClassName * grid, XDMFATTRIBUTE * Attribute, int passControl)                 \
{                                                                                                              \
  XdmfGridInsertAttribute((XDMFGRID *)((void *)grid), Attribute, passControl);                                 \
}                                                                                                              \
                                                                                                               \
void ClassName##RemoveAttribute(CClassName * grid, unsigned int index)                                         \
{                                                                                                              \
  XdmfGridRemoveAttribute((XDMFGRID *)((void *)grid), index);                                                  \
}                                                                                                              \
                                                                                                               \
void ClassName##RemoveAttributeByName(CClassName * grid, char * Name)                                          \
{                                                                                                              \
  XdmfGridRemoveAttributeByName((XDMFGRID *)((void *)grid), Name);                                             \
}                                                                                                              \
                                                                                                               \
XDMFSET * ClassName##GetSet(CClassName * grid, unsigned int index)                                             \
{                                                                                                              \
  return XdmfGridGetSet((XDMFGRID *)((void *)grid), index);                                                    \
}                                                                                                              \
                                                                                                               \
XDMFSET * ClassName##GetSetByName(CClassName * grid, char * Name)                                              \
{                                                                                                              \
  return XdmfGridGetSetByName((XDMFGRID *)((void *)grid), Name);                                               \
}                                                                                                              \
                                                                                                               \
unsigned int ClassName##GetNumberSets(CClassName * grid)                                                       \
{                                                                                                              \
  return XdmfGridGetNumberSets((XDMFGRID *)((void *)grid));                                                    \
}                                                                                                              \
                                                                                                               \
void ClassName##InsertSet(CClassName * grid, XDMFSET * Set, int passControl)                                   \
{                                                                                                              \
  XdmfGridInsertSet((XDMFGRID *)((void *)grid), Set, passControl);                                             \
}                                                                                                              \
                                                                                                               \
void ClassName##RemoveSet(CClassName * grid, unsigned int index)                                               \
{                                                                                                              \
  XdmfGridRemoveSet((XDMFGRID *)((void *)grid), index);                                                        \
}                                                                                                              \
                                                                                                               \
void ClassName##RemoveSetByName(CClassName * grid, char * Name)                                                \
{                                                                                                              \
  XdmfGridRemoveSetByName((XDMFGRID *)((void *)grid), Name);                                                   \
}                                                                                                              \
                                                                                                               \
XDMFMAP * ClassName##GetMap(CClassName * grid, unsigned int index)                                             \
{                                                                                                              \
  return XdmfGridGetMap((XDMFGRID *)((void *)grid), index);                                                    \
}                                                                                                              \
                                                                                                               \
XDMFMAP * ClassName##GetMapByName(CClassName * grid, char * Name)                                              \
{                                                                                                              \
  return XdmfGridGetMapByName((XDMFGRID *)((void *)grid), Name);                                               \
}                                                                                                              \
                                                                                                               \
unsigned int ClassName##GetNumberMaps(CClassName * grid)                                                       \
{                                                                                                              \
  return XdmfGridGetNumberMaps((XDMFGRID *)((void *)grid));                                                    \
}                                                                                                              \
                                                                                                               \
void ClassName##InsertMap(CClassName * grid, XDMFMAP * Map, int passControl)                                   \
{                                                                                                              \
  XdmfGridInsertMap((XDMFGRID *)((void *)grid), Map, passControl);                                             \
}                                                                                                              \
                                                                                                               \
void ClassName##RemoveMap(CClassName * grid, unsigned int index)                                               \
{                                                                                                              \
  XdmfGridRemoveMap((XDMFGRID *)((void *)grid), index);                                                        \
}                                                                                                              \
                                                                                                               \
void ClassName##RemoveMapByName(CClassName * grid, char * Name)                                                \
{                                                                                                              \
  XdmfGridRemoveMapByName((XDMFGRID *)((void *)grid), Name);                                                   \
}                                                                                                              \
                                                                                                               \
XDMFGRIDCONTROLLER * ClassName##GetGridController(CClassName * grid)                                           \
{                                                                                                              \
  return XdmfGridGetGridController((XDMFGRID *)((void *)grid));                                                \
}                                                                                                              \
                                                                                                               \
char * ClassName##GetName(CClassName * grid)                                                                   \
{                                                                                                              \
  return XdmfGridGetName((XDMFGRID *)((void *)grid));                                                          \
}                                                                                                              \
                                                                                                               \
XDMFTIME * ClassName##GetTime(CClassName * grid)                                                               \
{                                                                                                              \
  return XdmfGridGetTime((XDMFGRID *)((void *)grid));                                                          \
}                                                                                                              \
                                                                                                               \
void                                                                                                           \
ClassName##Read( CClassName * grid, int * status)                                                              \
{                                                                                                              \
  XdmfGridRead((XDMFGRID *)((void *)grid), status);                                                            \
}                                                                                                              \
                                                                                                               \
void                                                                                                           \
ClassName##Release( CClassName * grid)                                                                         \
{                                                                                                              \
  XdmfGridRelease((XDMFGRID *)((void *)grid));                                                                 \
}                                                                                                              \
                                                                                                               \
void ClassName##SetGridController(CClassName * grid, XDMFGRIDCONTROLLER * controller, int passControl)         \
{                                                                                                              \
  XdmfGridSetGridController((XDMFGRID *)((void *)grid), controller, passControl);                              \
}                                                                                                              \
                                                                                                               \
void ClassName##SetName(CClassName * grid, char * name, int * status)                                          \
{                                                                                                              \
  XdmfGridSetName((XDMFGRID *)((void *)grid), name, status);                                                   \
}                                                                                                              \
                                                                                                               \
void ClassName##SetTime(CClassName * grid, XDMFTIME * time, int passControl)                                   \
{                                                                                                              \
  XdmfGridSetTime((XDMFGRID *)((void *)grid), time, passControl);                                              \
}

#ifdef __cplusplus
}
#endif

#endif /* XDMFGRID_HPP_ */
