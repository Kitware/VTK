/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfDomain.hpp                                                      */
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

#ifndef XDMFDOMAIN_HPP_
#define XDMFDOMAIN_HPP_

// Forward Declarations
class XdmfCurvilinearGrid;
class XdmfGraph;
class XdmfGridCollection;
class XdmfRectilinearGrid;
class XdmfRegularGrid;
class XdmfUnstructuredGrid;

// Includes
#include "Xdmf.hpp"
#include "XdmfItem.hpp"

/**
 * @brief The root XdmfItem that holds XdmfGrids.
 *
 * XdmfDomain is the top XdmfItem in an Xdmf structure.  It can store
 * a number of grids and provides methods to insert, retrieve, and
 * remove these grids.
 */
class XDMF_EXPORT XdmfDomain : public virtual XdmfItem {

public:

  /**
   * Create a new XdmfDomain.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDomain.cpp
   * @skipline //#initialization
   * @until //#initialization
   *
   * Python
   *
   * @dontinclude XdmfExampleDomain.py
   * @skipline #//initialization
   * @until #//initialization
   *
   * @return    Constructed XdmfDomain.
   */
  static shared_ptr<XdmfDomain> New();

  virtual ~XdmfDomain();

  LOKI_DEFINE_VISITABLE(XdmfDomain, XdmfItem);
  XDMF_CHILDREN(XdmfDomain, XdmfGridCollection, GridCollection, Name);
  XDMF_CHILDREN(XdmfDomain, XdmfGraph, Graph, Name);
  XDMF_CHILDREN(XdmfDomain, XdmfCurvilinearGrid, CurvilinearGrid, Name);
  XDMF_CHILDREN(XdmfDomain, XdmfRectilinearGrid, RectilinearGrid, Name);
  XDMF_CHILDREN(XdmfDomain, XdmfRegularGrid, RegularGrid, Name);
  XDMF_CHILDREN(XdmfDomain, XdmfUnstructuredGrid, UnstructuredGrid, Name);
  static const std::string ItemTag;

  std::map<std::string, std::string> getItemProperties() const;

  virtual std::string getItemTag() const;

  using XdmfItem::insert;

  virtual void traverse(const shared_ptr<XdmfBaseVisitor> visitor);

protected:

  XdmfDomain();
  virtual void
  populateItem(const std::map<std::string, std::string> & itemProperties,
               const std::vector<shared_ptr<XdmfItem> > & childItems,
               const XdmfCoreReader * const reader);

private:

  XdmfDomain(const XdmfDomain &);  // Not implemented.
  void operator=(const XdmfDomain &);  // Not implemented.

};

#ifdef _WIN32
XDMF_TEMPLATE template class XDMF_EXPORT
std::allocator<shared_ptr<XdmfGridCollection> >;
XDMF_TEMPLATE template class XDMF_EXPORT
std::vector<shared_ptr<XdmfGridCollection>,
            std::allocator<shared_ptr<XdmfGridCollection> > >;
XDMF_TEMPLATE template class XDMF_EXPORT
std::allocator<shared_ptr<XdmfCurvilinearGrid> >;
XDMF_TEMPLATE template class XDMF_EXPORT
std::vector<shared_ptr<XdmfCurvilinearGrid>,
            std::allocator<shared_ptr<XdmfCurvilinearGrid> > >;
XDMF_TEMPLATE template class XDMF_EXPORT
std::allocator<shared_ptr<XdmfRectilinearGrid> >;
XDMF_TEMPLATE template class XDMF_EXPORT
std::vector<shared_ptr<XdmfRectilinearGrid>,
            std::allocator<shared_ptr<XdmfRectilinearGrid> > >;
XDMF_TEMPLATE template class XDMF_EXPORT
std::allocator<shared_ptr<XdmfRegularGrid> >;
XDMF_TEMPLATE template class XDMF_EXPORT
std::vector<shared_ptr<XdmfRegularGrid>,
            std::allocator<shared_ptr<XdmfRegularGrid> > >;
XDMF_TEMPLATE template class XDMF_EXPORT
std::allocator<shared_ptr<XdmfUnstructuredGrid> >;
XDMF_TEMPLATE template class XDMF_EXPORT
std::vector<shared_ptr<XdmfUnstructuredGrid>,
            std::allocator<shared_ptr<XdmfUnstructuredGrid> > >;
#endif

#endif /* XDMFDOMAIN_HPP_ */
