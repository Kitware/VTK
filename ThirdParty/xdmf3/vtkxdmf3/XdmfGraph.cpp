/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfGraph.cpp                                                       */
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

#include "XdmfAttribute.hpp"
#include "XdmfGraph.hpp"

XDMF_CHILDREN_IMPLEMENTATION(XdmfGraph, XdmfAttribute, Attribute, Name)

shared_ptr<XdmfGraph>
XdmfGraph::New(const unsigned int numberNodes)
{
  shared_ptr<XdmfGraph> p(new XdmfGraph(numberNodes));
  return p;
}

XdmfGraph::XdmfGraph(const unsigned int numberNodes) :
  XdmfSparseMatrix(numberNodes,
                   numberNodes),
  mTime(shared_ptr<XdmfTime>())
{
}

XdmfGraph::XdmfGraph(XdmfGraph & refGraph) :
  XdmfSparseMatrix(refGraph),
  mAttributes(refGraph.mAttributes),
  mTime(refGraph.mTime)
{
}

XdmfGraph::~XdmfGraph()
{
}

const std::string XdmfGraph::ItemTag = "Graph";

std::string
XdmfGraph::getItemTag() const
{
  return ItemTag;
}

shared_ptr<XdmfTime>
XdmfGraph::getTime()
{
  return boost::const_pointer_cast<XdmfTime>
    (static_cast<const XdmfGraph &>(*this).getTime());
}

shared_ptr<const XdmfTime>
XdmfGraph::getTime() const
{
  return mTime;
}

unsigned int
XdmfGraph::getNumberNodes() const
{
  // The number of nodes is equal to the number of rows or columns. Either will work.
  return this->getNumberRows();
}

void
XdmfGraph::populateItem(const std::map<std::string, std::string> & itemProperties,
                        const std::vector<shared_ptr<XdmfItem> > & childItems,
                        const XdmfCoreReader * const reader)
{
  XdmfSparseMatrix::populateItem(itemProperties,
                                 childItems,
                                 reader);
  for(std::vector<shared_ptr<XdmfItem> >::const_iterator iter =
        childItems.begin();
      iter != childItems.end();
      ++iter) {
    if(shared_ptr<XdmfAttribute> attribute =
       shared_dynamic_cast<XdmfAttribute>(*iter)) {
      this->insert(attribute);
    }
  }
}

void
XdmfGraph::setTime(const shared_ptr<XdmfTime> time)
{
  mTime = time;
  this->setIsChanged(true);
}

void
XdmfGraph::traverse(const shared_ptr<XdmfBaseVisitor> visitor)
{
  XdmfSparseMatrix::traverse(visitor);
  for (unsigned int i = 0; i < mAttributes.size(); ++i)
  {
    mAttributes[i]->accept(visitor);
  }
}

// C Wrappers

XDMFGRAPH * XdmfGraphNew(unsigned int numberNodes)
{
  try
  {
    shared_ptr<XdmfGraph> generatedGraph = XdmfGraph::New(numberNodes);
    return (XDMFGRAPH *)((void *)(new XdmfGraph(*generatedGraph.get())));
  }
  catch (...)
  {
    shared_ptr<XdmfGraph> generatedGraph = XdmfGraph::New(numberNodes);
    return (XDMFGRAPH *)((void *)(new XdmfGraph(*generatedGraph.get())));
  }
}

XDMFATTRIBUTE * XdmfGraphGetAttribute(XDMFGRAPH * graph, unsigned int index)
{
  return (XDMFATTRIBUTE *)((void *)(((XdmfGraph *)(graph))->getAttribute(index).get()));
}

XDMFATTRIBUTE * XdmfGraphGetAttributeByName(XDMFGRAPH * graph, char * Name)
{
  return (XDMFATTRIBUTE *)((void *)(((XdmfGraph *)(graph))->getAttribute(std::string(Name)).get()));
}

unsigned int XdmfGraphGetNumberAttributes(XDMFGRAPH * graph)
{
  return ((XdmfGraph *)graph)->getNumberAttributes();
}

void XdmfGraphInsertAttribute(XDMFGRAPH * graph, XDMFATTRIBUTE * Attribute, int passControl)
{
  if (passControl) {
    ((XdmfGraph *)(graph))->insert(shared_ptr<XdmfAttribute>((XdmfAttribute *)Attribute));
  }
  else {
    ((XdmfGraph *)(graph))->insert(shared_ptr<XdmfAttribute>((XdmfAttribute *)Attribute, XdmfNullDeleter()));
  }
}

void XdmfGraphRemoveAttribute(XDMFGRAPH * graph, unsigned int index)
{
  ((XdmfGraph *)graph)->removeAttribute(index);
}

void XdmfGraphRemoveAttributeByName(XDMFGRAPH * graph, char * Name)
{
  ((XdmfGraph *)graph)->removeAttribute(std::string(Name));
}

unsigned int XdmfGraphGetNumberNodes(XDMFGRAPH * graph)
{
  return ((XdmfGraph *)graph)->getNumberNodes();
}

XDMF_ITEM_C_CHILD_WRAPPER(XdmfGraph, XDMFGRAPH)
XDMF_SPARSEMATRIX_C_CHILD_WRAPPER(XdmfGraph, XDMFGRAPH)
