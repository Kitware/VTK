/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfDomain.cpp                                                      */
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

#include "XdmfDomain.hpp"
#include "XdmfCurvilinearGrid.hpp"
#include "XdmfGraph.hpp"
#include "XdmfGridCollection.hpp"
#include "XdmfRectilinearGrid.hpp"
#include "XdmfRegularGrid.hpp"
#include "XdmfUnstructuredGrid.hpp"

XDMF_CHILDREN_IMPLEMENTATION(XdmfDomain,
                             XdmfGridCollection,
                             GridCollection,
                             Name)
XDMF_CHILDREN_IMPLEMENTATION(XdmfDomain,
                             XdmfCurvilinearGrid,
                             CurvilinearGrid,
                             Name)
XDMF_CHILDREN_IMPLEMENTATION(XdmfDomain,
                             XdmfGraph,
                             Graph,
                             Name)
XDMF_CHILDREN_IMPLEMENTATION(XdmfDomain,
                             XdmfRectilinearGrid,
                             RectilinearGrid,
                             Name)
XDMF_CHILDREN_IMPLEMENTATION(XdmfDomain,
                             XdmfRegularGrid,
                             RegularGrid,
                             Name)
XDMF_CHILDREN_IMPLEMENTATION(XdmfDomain,
                             XdmfUnstructuredGrid,
                             UnstructuredGrid,
                             Name)

shared_ptr<XdmfDomain>
XdmfDomain::New()
{
  shared_ptr<XdmfDomain> p(new XdmfDomain());
  return p;
}

XdmfDomain::XdmfDomain()
{
}

XdmfDomain::~XdmfDomain()
{
}

const std::string XdmfDomain::ItemTag = "Domain";

std::map<std::string, std::string>
XdmfDomain::getItemProperties() const
{
  std::map<std::string, std::string> domainProperties;
  return domainProperties;
}

std::string
XdmfDomain::getItemTag() const
{
  return ItemTag;
}

void
XdmfDomain::populateItem(const std::map<std::string, std::string> & itemProperties,
                         const std::vector<shared_ptr<XdmfItem> > & childItems,
                         const XdmfCoreReader * const reader)
{
  XdmfItem::populateItem(itemProperties, childItems, reader);
  for(std::vector<shared_ptr<XdmfItem> >::const_iterator iter =
        childItems.begin();
      iter != childItems.end();
      ++iter) {
    if(shared_ptr<XdmfGridCollection> gridCollection =
       shared_dynamic_cast<XdmfGridCollection>(*iter)) {
      this->insert(gridCollection);
    }
    else if(shared_ptr<XdmfCurvilinearGrid> grid =
            shared_dynamic_cast<XdmfCurvilinearGrid>(*iter)) {
      this->insert(grid);
    }
    else if(shared_ptr<XdmfGraph> graph =
            shared_dynamic_cast<XdmfGraph>(*iter)) {
      this->insert(graph);
    }
    else if(shared_ptr<XdmfRectilinearGrid> grid =
            shared_dynamic_cast<XdmfRectilinearGrid>(*iter)) {
      this->insert(grid);
    }
    else if(shared_ptr<XdmfRegularGrid> grid =
            shared_dynamic_cast<XdmfRegularGrid>(*iter)) {
      this->insert(grid);
    }
    else if(shared_ptr<XdmfUnstructuredGrid> grid =
            shared_dynamic_cast<XdmfUnstructuredGrid>(*iter)) {
      this->insert(grid);
    }
  }
}

void
XdmfDomain::traverse(const shared_ptr<XdmfBaseVisitor> visitor)
{
  XdmfItem::traverse(visitor);
  for(std::vector<shared_ptr<XdmfGridCollection> >::const_iterator iter =
        mGridCollections.begin();
      iter != mGridCollections.end();
      ++iter) {
    (*iter)->accept(visitor);
  }
  for(std::vector<shared_ptr<XdmfCurvilinearGrid> >::const_iterator iter =
        mCurvilinearGrids.begin();
      iter != mCurvilinearGrids.end();
      ++iter) {
    (*iter)->accept(visitor);
  }
  for(std::vector<shared_ptr<XdmfGraph> >::const_iterator iter =
        mGraphs.begin();
      iter != mGraphs.end();
      ++iter) {
    (*iter)->accept(visitor);
  }
  for(std::vector<shared_ptr<XdmfRectilinearGrid> >::const_iterator iter =
        mRectilinearGrids.begin();
      iter != mRectilinearGrids.end();
      ++iter) {
    (*iter)->accept(visitor);
  }
  for(std::vector<shared_ptr<XdmfRegularGrid> >::const_iterator iter =
        mRegularGrids.begin();
      iter != mRegularGrids.end();
      ++iter) {
    (*iter)->accept(visitor);
  }
  for(std::vector<shared_ptr<XdmfUnstructuredGrid> >::const_iterator iter =
        mUnstructuredGrids.begin();
      iter != mUnstructuredGrids.end();
      ++iter) {
    (*iter)->accept(visitor);
  }
}
