/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfGraph.hpp                                                       */
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

#ifndef XDMFGRAPH_HPP_
#define XDMFGRAPH_HPP_

// Forward Declarations
class XdmfAttribute;

// Includes
#include "Xdmf.hpp"
#include "XdmfTime.hpp"
#include "XdmfSparseMatrix.hpp"

/**
 * @brief Graph stored in sparse matrix form.
 *
 * Stores graph information in sparse matrix form. Attributes defining
 * node and edge information can be inserted.
 */
class XDMF_EXPORT XdmfGraph : public XdmfSparseMatrix {

public:

  /**
   * Create a new XdmfGraph.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfGraph.cpp
   * @skipline //#initialization
   * @until //#initialization
   *
   * Python
   *
   * @dontinclude XdmfExampleGraph.py
   * @skipline #//initialization
   * @until #//initialization
   *
   * @param numberNodes number of nodes in graph.
   *
   * @return    Constructed XdmfGraph.
   */
  static shared_ptr<XdmfGraph> New(const unsigned int numberNodes);

  virtual ~XdmfGraph();

  LOKI_DEFINE_VISITABLE(XdmfGraph, XdmfSparseMatrix)
  XDMF_CHILDREN(XdmfGraph, XdmfAttribute, Attribute, Name)
  static const std::string ItemTag;

  std::string getItemTag() const;

  shared_ptr<XdmfTime> getTime();

  shared_ptr<const XdmfTime> getTime() const;

  unsigned int getNumberNodes() const;

  using XdmfSparseMatrix::insert;

  void setTime(const shared_ptr<XdmfTime> time);

  void traverse(const shared_ptr<XdmfBaseVisitor> visitor);

protected:

  XdmfGraph(const unsigned int numberNodes);

  virtual void
  populateItem(const std::map<std::string, std::string> & itemProperties,
               const std::vector<shared_ptr<XdmfItem> > & childItems,
               const XdmfCoreReader * const reader);

private:

  XdmfGraph(const XdmfGraph &);  // Not implemented.
  void operator=(const XdmfGraph &);  // Not implemented.

  shared_ptr<XdmfTime> mTime;
};

#ifdef _WIN32
#endif

#endif /* XDMFGRAPH_HPP_ */
