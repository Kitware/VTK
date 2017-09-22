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
  return const_pointer_cast<XdmfTime>
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
  shared_ptr<XdmfGraph> * p = 
    new shared_ptr<XdmfGraph>(XdmfGraph::New(numberNodes));
  return (XDMFGRAPH *) p;
}

XDMFATTRIBUTE * XdmfGraphGetAttribute(XDMFGRAPH * graph, unsigned int index)
{
  shared_ptr<XdmfGraph> & refGraph = *(shared_ptr<XdmfGraph> *)(graph);
  shared_ptr<XdmfAttribute> * p =  new shared_ptr<XdmfAttribute>(refGraph->getAttribute(index));
  return (XDMFATTRIBUTE *) p;
}

XDMFATTRIBUTE * XdmfGraphGetAttributeByName(XDMFGRAPH * graph, char * Name)
{
  shared_ptr<XdmfGraph> & refGraph = *(shared_ptr<XdmfGraph> *)(graph);
  shared_ptr<XdmfAttribute> * p =  new shared_ptr<XdmfAttribute>(refGraph->getAttribute(Name));
  return (XDMFATTRIBUTE *) p;
}

unsigned int XdmfGraphGetNumberAttributes(XDMFGRAPH * graph)
{
  shared_ptr<XdmfGraph> & refGraph = *(shared_ptr<XdmfGraph> *)(graph);
  return refGraph->getNumberAttributes();
}

void XdmfGraphInsertAttribute(XDMFGRAPH * graph, XDMFATTRIBUTE * Attribute, int passControl)
{
  shared_ptr<XdmfGraph> & refGraph = *(shared_ptr<XdmfGraph> *)(graph);
  shared_ptr<XdmfAttribute> & refAttribute = *(shared_ptr<XdmfAttribute> *)(Attribute);
  refGraph->insert(refAttribute);
}

void XdmfGraphRemoveAttribute(XDMFGRAPH * graph, unsigned int index)
{
  shared_ptr<XdmfGraph> & refGraph = *(shared_ptr<XdmfGraph> *)(graph);
  refGraph->removeAttribute(index);
}

void XdmfGraphRemoveAttributeByName(XDMFGRAPH * graph, char * Name)
{
  shared_ptr<XdmfGraph> & refGraph = *(shared_ptr<XdmfGraph> *)(graph);
  refGraph->removeAttribute(Name);
}

unsigned int XdmfGraphGetNumberNodes(XDMFGRAPH * graph)
{
  shared_ptr<XdmfGraph> & refGraph = *(shared_ptr<XdmfGraph> *)(graph);
  return refGraph->getNumberNodes();
}

XDMF_ITEM_C_CHILD_WRAPPER(XdmfGraph, XDMFGRAPH)
XDMF_SPARSEMATRIX_C_CHILD_WRAPPER(XdmfGraph, XDMFGRAPH)
