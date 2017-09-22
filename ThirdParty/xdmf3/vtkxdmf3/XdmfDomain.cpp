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

#ifdef XDMF_BUILD_DSM
  #include "XdmfDSMBuffer.hpp"
  #include "XdmfDSMDriver.hpp"
  #include "XdmfDSMDescription.hpp"
#endif

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
#ifdef XDMF_BUILD_DSM
  // Traverse Data Descriptions before anything
  XdmfDSMBuffer * dsmBuffer = (XdmfDSMBuffer *)xdmf_dsm_get_manager();

  if (dsmBuffer)
  {
    shared_ptr<XdmfDSMDescription> dsmDescription;
    dsmDescription = XdmfDSMDescription::New();
    dsmDescription->setPortDescription(dsmBuffer->GetComm()->GetDsmPortName());

    dsmDescription->accept(visitor);
  }
#endif

  XdmfItem::traverse(visitor);
  for (unsigned int i = 0; i < mGridCollections.size(); ++i)
  {
    mGridCollections[i]->accept(visitor);
  }
  for (unsigned int i = 0; i < mCurvilinearGrids.size(); ++i)
  {
    mCurvilinearGrids[i]->accept(visitor);
  }
  for (unsigned int i = 0; i < mGraphs.size(); ++i)
  {
    mGraphs[i]->accept(visitor);
  }
  for (unsigned int i = 0; i < mRectilinearGrids.size(); ++i)
  {
    mRectilinearGrids[i]->accept(visitor);
  }
  for (unsigned int i = 0; i < mRegularGrids.size(); ++i)
  {
    mRegularGrids[i]->accept(visitor);
  }
  for (unsigned int i = 0; i < mUnstructuredGrids.size(); ++i)
  {
    mUnstructuredGrids[i]->accept(visitor);
  }
}

// C Wrappers

XDMFDOMAIN * XdmfDomainNew()
{
  shared_ptr<XdmfDomain> * p = new shared_ptr<XdmfDomain>(XdmfDomain::New());
  return (XDMFDOMAIN *) p;
}

XDMFGRIDCOLLECTION * XdmfDomainGetGridCollection(XDMFDOMAIN * domain, unsigned int index)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  shared_ptr<XdmfGridCollection> * gridCollection =
    new shared_ptr<XdmfGridCollection>(refDomain->getGridCollection(index));
  return (XDMFGRIDCOLLECTION *) gridCollection;
}

XDMFGRIDCOLLECTION * XdmfDomainGetGridCollectionByName(XDMFDOMAIN * domain, char * Name)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  shared_ptr<XdmfGridCollection> * gridCollection =
    new shared_ptr<XdmfGridCollection>(refDomain->getGridCollection(Name));
  return (XDMFGRIDCOLLECTION *) gridCollection;
}

unsigned int XdmfDomainGetNumberGridCollections(XDMFDOMAIN * domain)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  return refDomain->getNumberGridCollections();
}

void XdmfDomainInsertGridCollection(XDMFDOMAIN * domain, XDMFGRIDCOLLECTION * GridCollection, int passControl)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  shared_ptr<XdmfGridCollection> refGridCollection = *(shared_ptr<XdmfGridCollection> *)(GridCollection);
  refDomain->insert(refGridCollection);
}

void XdmfDomainRemoveGridCollection(XDMFDOMAIN * domain, unsigned int index)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  refDomain->removeGridCollection(index);
}

void XdmfDomainRemoveGridCollectionByName(XDMFDOMAIN * domain, char * Name)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  refDomain->removeGridCollection(Name);
}

XDMFGRAPH * XdmfDomainGetGraph(XDMFDOMAIN * domain, unsigned int index)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  shared_ptr<XdmfGraph> * graph =
    new shared_ptr<XdmfGraph>(refDomain->getGraph(index));
  return (XDMFGRAPH *) graph;
}

XDMFGRAPH * XdmfDomainGetGraphByName(XDMFDOMAIN * domain, char * Name)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  shared_ptr<XdmfGraph> * graph =
    new shared_ptr<XdmfGraph>(refDomain->getGraph(Name));
  return (XDMFGRAPH *) graph;
}

unsigned int XdmfDomainGetNumberGraphs(XDMFDOMAIN * domain)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  return refDomain->getNumberGraphs();
}

void XdmfDomainInsertGraph(XDMFDOMAIN * domain, XDMFGRAPH * Graph, int passControl)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  shared_ptr<XdmfGraph> refGraph = *(shared_ptr<XdmfGraph> *)(Graph);
  refDomain->insert(refGraph);
}

void XdmfDomainRemoveGraph(XDMFDOMAIN * domain, unsigned int index)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  refDomain->removeGraph(index);
}

void XdmfDomainRemoveGraphByName(XDMFDOMAIN * domain, char * Name)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  refDomain->removeGraph(Name);
}

XDMFCURVILINEARGRID * XdmfDomainGetCurvilinearGrid(XDMFDOMAIN * domain, unsigned int index)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  shared_ptr<XdmfCurvilinearGrid> * grid =
    new shared_ptr<XdmfCurvilinearGrid>(refDomain->getCurvilinearGrid(index));
  return (XDMFCURVILINEARGRID *) grid;
}

XDMFCURVILINEARGRID * XdmfDomainGetCurvilinearGridByName(XDMFDOMAIN * domain, char * Name)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  shared_ptr<XdmfCurvilinearGrid> * grid =
    new shared_ptr<XdmfCurvilinearGrid>(refDomain->getCurvilinearGrid(Name));
  return (XDMFCURVILINEARGRID *) grid;
}

unsigned int XdmfDomainGetNumberCurvilinearGrids(XDMFDOMAIN * domain)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  return refDomain->getNumberCurvilinearGrids();
}

void XdmfDomainInsertCurvilinearGrid(XDMFDOMAIN * domain, XDMFCURVILINEARGRID * CurvilinearGrid, int passControl)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  shared_ptr<XdmfCurvilinearGrid> refGrid = *(shared_ptr<XdmfCurvilinearGrid> *)(CurvilinearGrid);
  refDomain->insert(refGrid);
}

void XdmfDomainRemoveCurvilinearGrid(XDMFDOMAIN * domain, unsigned int index)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  refDomain->removeCurvilinearGrid(index);
}

void XdmfDomainRemoveCurvilinearGridByName(XDMFDOMAIN * domain, char * Name)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  refDomain->removeCurvilinearGrid(Name);
}

XDMFRECTILINEARGRID * XdmfDomainGetRectilinearGrid(XDMFDOMAIN * domain, unsigned int index)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  shared_ptr<XdmfRectilinearGrid> * grid =
    new shared_ptr<XdmfRectilinearGrid>(refDomain->getRectilinearGrid(index));
  return (XDMFRECTILINEARGRID *) grid;
}

XDMFRECTILINEARGRID * XdmfDomainGetRectilinearGridByName(XDMFDOMAIN * domain, char * Name)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  shared_ptr<XdmfRectilinearGrid> * grid =
    new shared_ptr<XdmfRectilinearGrid>(refDomain->getRectilinearGrid(Name));
  return (XDMFRECTILINEARGRID *) grid;
}

unsigned int XdmfDomainGetNumberRectilinearGrids(XDMFDOMAIN * domain)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  return refDomain->getNumberRectilinearGrids();
}

void XdmfDomainInsertRectilinearGrid(XDMFDOMAIN * domain, XDMFRECTILINEARGRID * RectilinearGrid, int passControl)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  shared_ptr<XdmfRectilinearGrid> refGrid = *(shared_ptr<XdmfRectilinearGrid> *)(RectilinearGrid);
  refDomain->insert(refGrid);
}

void XdmfDomainRemoveRectilinearGrid(XDMFDOMAIN * domain, unsigned int index)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  refDomain->removeRectilinearGrid(index);
}

void XdmfDomainRemoveRectilinearGridByName(XDMFDOMAIN * domain, char * Name)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  refDomain->removeRectilinearGrid(Name);
}

XDMFREGULARGRID * XdmfDomainGetRegularGrid(XDMFDOMAIN * domain, unsigned int index)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  shared_ptr<XdmfRegularGrid> * grid =
    new shared_ptr<XdmfRegularGrid>(refDomain->getRegularGrid(index));
  return (XDMFREGULARGRID *) grid;
}

XDMFREGULARGRID * XdmfDomainGetRegularGridByName(XDMFDOMAIN * domain, char * Name)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  shared_ptr<XdmfRegularGrid> * grid =
    new shared_ptr<XdmfRegularGrid>(refDomain->getRegularGrid(Name));
  return (XDMFREGULARGRID *) grid;
}

unsigned int XdmfDomainGetNumberRegularGrids(XDMFDOMAIN * domain)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  return refDomain->getNumberRegularGrids();
}

void XdmfDomainInsertRegularGrid(XDMFDOMAIN * domain, XDMFREGULARGRID * RegularGrid, int passControl)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  shared_ptr<XdmfRegularGrid> refGrid = *(shared_ptr<XdmfRegularGrid> *)(RegularGrid);
  refDomain->insert(refGrid);
}

void XdmfDomainRemoveRegularGrid(XDMFDOMAIN * domain, unsigned int index)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  refDomain->removeRegularGrid(index);
}

void XdmfDomainRemoveRegularGridByName(XDMFDOMAIN * domain, char * Name)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  refDomain->removeRegularGrid(Name);
}

XDMFUNSTRUCTUREDGRID * XdmfDomainGetUnstructuredGrid(XDMFDOMAIN * domain, unsigned int index)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  shared_ptr<XdmfUnstructuredGrid> * grid =
    new shared_ptr<XdmfUnstructuredGrid>(refDomain->getUnstructuredGrid(index));
  return (XDMFUNSTRUCTUREDGRID *) grid;
}

XDMFUNSTRUCTUREDGRID * XdmfDomainGetUnstructuredGridByName(XDMFDOMAIN * domain, char * Name)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  shared_ptr<XdmfUnstructuredGrid> * grid =
    new shared_ptr<XdmfUnstructuredGrid>(refDomain->getUnstructuredGrid(Name));
  return (XDMFUNSTRUCTUREDGRID *) grid;
}

unsigned int XdmfDomainGetNumberUnstructuredGrids(XDMFDOMAIN * domain)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  return refDomain->getNumberUnstructuredGrids();
}

void XdmfDomainInsertUnstructuredGrid(XDMFDOMAIN * domain, XDMFUNSTRUCTUREDGRID * UnstructuredGrid, int passControl)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  shared_ptr<XdmfUnstructuredGrid> refGrid = *(shared_ptr<XdmfUnstructuredGrid> *)(UnstructuredGrid);
  refDomain->insert(refGrid);
}

void XdmfDomainRemoveUnstructuredGrid(XDMFDOMAIN * domain, unsigned int index)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  refDomain->removeUnstructuredGrid(index);
}

void XdmfDomainRemoveUnstructuredGridByName(XDMFDOMAIN * domain, char * Name)
{
  shared_ptr<XdmfDomain> & refDomain = *(shared_ptr<XdmfDomain> *)(domain);
  refDomain->removeUnstructuredGrid(Name);
}

XDMF_ITEM_C_CHILD_WRAPPER(XdmfDomain, XDMFDOMAIN)
