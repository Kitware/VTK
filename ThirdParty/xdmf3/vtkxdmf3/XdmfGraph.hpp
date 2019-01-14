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

// C Compatible Includes
#include "Xdmf.hpp"
#include "XdmfAttribute.hpp"
#include "XdmfTime.hpp"
#include "XdmfSparseMatrix.hpp"

#ifdef __cplusplus

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

  XdmfGraph(XdmfGraph &);

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

#endif

#ifdef __cplusplus
extern "C" {
#endif

// C wrappers go here

struct XDMFGRAPH; // Simply as a typedef to ensure correct typing
typedef struct XDMFGRAPH XDMFGRAPH;

XDMF_EXPORT XDMFGRAPH * XdmfGraphNew(unsigned int numberNodes);

XDMF_EXPORT XDMFATTRIBUTE * XdmfGraphGetAttribute(XDMFGRAPH * graph, unsigned int index);

XDMF_EXPORT XDMFATTRIBUTE * XdmfGraphGetAttributeByName(XDMFGRAPH * graph, char * Name);

XDMF_EXPORT unsigned int XdmfGraphGetNumberAttributes(XDMFGRAPH * graph);

XDMF_EXPORT void XdmfGraphInsertAttribute(XDMFGRAPH * graph, XDMFATTRIBUTE * Attribute, int passControl);

XDMF_EXPORT void XdmfGraphRemoveAttribute(XDMFGRAPH * graph, unsigned int index);

XDMF_EXPORT void XdmfGraphRemoveAttributeByName(XDMFGRAPH * graph, char * Name);

XDMF_EXPORT unsigned int XdmfGraphGetNumberNodes(XDMFGRAPH * graph);

XDMF_ITEM_C_CHILD_DECLARE(XdmfGraph, XDMFGRAPH, XDMF)
XDMF_SPARSEMATRIX_C_CHILD_DECLARE(XdmfGraph, XDMFGRAPH, XDMF)

#ifdef __cplusplus
}
#endif

#endif /* XDMFGRAPH_HPP_ */
