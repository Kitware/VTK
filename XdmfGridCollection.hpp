/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfGridCollection.hpp                                              */
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

#ifndef XDMFGRIDCOLLECTION_HPP_
#define XDMFGRIDCOLLECTION_HPP_

// Forward Declarations
class XdmfGridCollectionType;

// Includes
#include "Xdmf.hpp"
#include "XdmfDomain.hpp"
#include "XdmfGrid.hpp"

/**
 * @brief A spatial or temporal collection of XdmfGrids.
 *
 * A temporal collection is timestep data.  Each child grid represents
 * the state at a single timestep.  A spatial collection consists of
 * XdmfGrids that are arranged together in space. E.g. a partitioned
 * mesh.
 *
 * It is valid to nest collections. A spatial collection inside a
 * temporal collection is commonly used.
 */
class XDMF_EXPORT XdmfGridCollection : public XdmfDomain,
                                       public XdmfGrid {

public:

  /**
   * Create a new XdmfGridCollection.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfGridCollection.cpp
   * @skipline //#initalization
   * @until //#initalization
   *
   * Python
   *
   * @dontinclude XdmfExampleGridCollection.py
   * @skipline #//initalization
   * @until #//initalization
   *
   * @return    Constructed XdmfGridCollection.
   */
  static shared_ptr<XdmfGridCollection> New();

  virtual ~XdmfGridCollection();

  LOKI_DEFINE_VISITABLE(XdmfGridCollection, XdmfGrid)
  static const std::string ItemTag;

  std::map<std::string, std::string> getItemProperties() const;

  std::string getItemTag() const;

  /**
   * Get the XdmfGridCollectionType associated with this grid collection.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfGridCollection.cpp
   * @skipline //#initalization
   * @until //#initalization
   * @skipline //#getType
   * @until //#getType
   *
   * Python
   *
   * @dontinclude XdmfExampleGridCollection.py
   * @skipline #//initalization
   * @until #//initalization
   * @skipline #//getType
   * @until #//getType
   *
   * @return    XdmfGridCollectionType of this collection.
   */
  shared_ptr<const XdmfGridCollectionType> getType() const;

  using XdmfDomain::insert;
  using XdmfGrid::insert;

  /**
   * Insert an information into the grid collection.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfGridCollection.cpp
   * @skipline //#initalization
   * @until //#initalization
   * @skipline //#insert
   * @until //#insert
   *
   * Python
   *
   * @dontinclude XdmfExampleGridCollection.py
   * @skipline #//initalization
   * @until #//initalization
   * @skipline #//insert
   * @until #//insert
   *
   * @param     information     An XdmfInformation to attach to this item.
   */
  void insert(const shared_ptr<XdmfInformation> information);

  /**
   * Set the XdmfGridCollectionType associated with this grid
   * collection.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfGridCollection.cpp
   * @skipline //#initalization
   * @until //#initalization
   * @skipline //#setType
   * @until //#setType
   *
   * Python
   *
   * @dontinclude XdmfExampleGridCollection.py
   * @skipline #//initalization
   * @until #//initalization
   * @skipline #//setType
   * @until #//setType
   *
   * @param     type    The XdmfGridCollectionType to set.
   */
  void setType(const shared_ptr<const XdmfGridCollectionType> type);

  void traverse(const shared_ptr<XdmfBaseVisitor> visitor);

protected:

  XdmfGridCollection();

  virtual void
  populateItem(const std::map<std::string, std::string> & itemProperties,
               const std::vector<shared_ptr<XdmfItem> > & childItems,
               const XdmfCoreReader * const reader);

private:

  XdmfGridCollection(const XdmfGridCollection &);  // Not implemented.
  void operator=(const XdmfGridCollection &);  // Not implemented.

  shared_ptr<const XdmfGridCollectionType> mType;
};

#ifdef _WIN32
XDMF_TEMPLATE template class XDMF_EXPORT
shared_ptr<const XdmfGridCollectionType>;
#endif

#endif /* XDMFGRID_HPP_ */
