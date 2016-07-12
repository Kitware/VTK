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

XdmfDomain::XdmfDomain(XdmfDomain & refDomain) :
  XdmfItem(refDomain),
  mGridCollections(refDomain.mGridCollections),
  mGraphs(refDomain.mGraphs),
  mCurvilinearGrids(refDomain.mCurvilinearGrids),
  mRectilinearGrids(refDomain.mRectilinearGrids),
  mRegularGrids(refDomain.mRegularGrids),
  mUnstructuredGrids(refDomain.mUnstructuredGrids)
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
  try
  {
    shared_ptr<XdmfDomain> generatedDomain = XdmfDomain::New();
    return (XDMFDOMAIN *)((void *)((XdmfItem *)(new XdmfDomain(*generatedDomain.get()))));
  }
  catch (...)
  {
    shared_ptr<XdmfDomain> generatedDomain = XdmfDomain::New();
    return (XDMFDOMAIN *)((void *)((XdmfItem *)(new XdmfDomain(*generatedDomain.get()))));
  }
}

XDMFGRIDCOLLECTION * XdmfDomainGetGridCollection(XDMFDOMAIN * domain, unsigned int index)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  return (XDMFGRIDCOLLECTION *)((void *)((XdmfItem *)(domainPointer->getGridCollection(index).get())));
}

XDMFGRIDCOLLECTION * XdmfDomainGetGridCollectionByName(XDMFDOMAIN * domain, char * Name)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  return (XDMFGRIDCOLLECTION *)((void *)((XdmfItem *)(domainPointer->getGridCollection(Name).get())));
}

unsigned int XdmfDomainGetNumberGridCollections(XDMFDOMAIN * domain)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  return domainPointer->getNumberGridCollections();
}

void XdmfDomainInsertGridCollection(XDMFDOMAIN * domain, XDMFGRIDCOLLECTION * GridCollection, int passControl)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  if (passControl) {
    domainPointer->insert(shared_ptr<XdmfGridCollection>((XdmfGridCollection *)GridCollection));
  }
  else {
    domainPointer->insert(shared_ptr<XdmfGridCollection>((XdmfGridCollection *)GridCollection, XdmfNullDeleter()));
  }
}

void XdmfDomainRemoveGridCollection(XDMFDOMAIN * domain, unsigned int index)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  domainPointer->removeGridCollection(index);
}

void XdmfDomainRemoveGridCollectionByName(XDMFDOMAIN * domain, char * Name)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  domainPointer->removeGridCollection(Name);
}

XDMFGRAPH * XdmfDomainGetGraph(XDMFDOMAIN * domain, unsigned int index)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  return (XDMFGRAPH *)((void *)(domainPointer->getGraph(index).get()));
}

XDMFGRAPH * XdmfDomainGetGraphByName(XDMFDOMAIN * domain, char * Name)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  return (XDMFGRAPH *)((void *)(domainPointer->getGraph(Name).get()));
}

unsigned int XdmfDomainGetNumberGraphs(XDMFDOMAIN * domain)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  return domainPointer->getNumberGraphs();
}

void XdmfDomainInsertGraph(XDMFDOMAIN * domain, XDMFGRAPH * Graph, int passControl)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  if (passControl) {
    domainPointer->insert(shared_ptr<XdmfGraph>((XdmfGraph *)Graph));
  }
  else {
    domainPointer->insert(shared_ptr<XdmfGraph>((XdmfGraph *)Graph, XdmfNullDeleter()));
  }
}

void XdmfDomainRemoveGraph(XDMFDOMAIN * domain, unsigned int index)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  domainPointer->removeGraph(index);
}

void XdmfDomainRemoveGraphByName(XDMFDOMAIN * domain, char * Name)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  domainPointer->removeGraph(Name);
}

XDMFCURVILINEARGRID * XdmfDomainGetCurvilinearGrid(XDMFDOMAIN * domain, unsigned int index)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  return (XDMFCURVILINEARGRID *)((void *)((XdmfItem *)(domainPointer->getCurvilinearGrid(index).get())));
}

XDMFCURVILINEARGRID * XdmfDomainGetCurvilinearGridByName(XDMFDOMAIN * domain, char * Name)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  return (XDMFCURVILINEARGRID *)((void *)((XdmfItem *)(domainPointer->getCurvilinearGrid(Name).get())));
}

unsigned int XdmfDomainGetNumberCurvilinearGrids(XDMFDOMAIN * domain)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  return domainPointer->getNumberCurvilinearGrids();
}

void XdmfDomainInsertCurvilinearGrid(XDMFDOMAIN * domain, XDMFCURVILINEARGRID * CurvilinearGrid, int passControl)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  if (passControl) {
    domainPointer->insert(shared_ptr<XdmfCurvilinearGrid>((XdmfCurvilinearGrid *)CurvilinearGrid));
  }
  else {
    domainPointer->insert(shared_ptr<XdmfCurvilinearGrid>((XdmfCurvilinearGrid *)CurvilinearGrid, XdmfNullDeleter()));
  }
}

void XdmfDomainRemoveCurvilinearGrid(XDMFDOMAIN * domain, unsigned int index)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  domainPointer->removeCurvilinearGrid(index);
}

void XdmfDomainRemoveCurvilinearGridByName(XDMFDOMAIN * domain, char * Name)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  domainPointer->removeCurvilinearGrid(Name);
}

XDMFRECTILINEARGRID * XdmfDomainGetRectilinearGrid(XDMFDOMAIN * domain, unsigned int index)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  return (XDMFRECTILINEARGRID *)((void *)((XdmfItem *)(domainPointer->getRectilinearGrid(index).get())));
}

XDMFRECTILINEARGRID * XdmfDomainGetRectilinearGridByName(XDMFDOMAIN * domain, char * Name)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  return (XDMFRECTILINEARGRID *)((void *)((XdmfItem *)(domainPointer->getRectilinearGrid(Name).get())));
}

unsigned int XdmfDomainGetNumberRectilinearGrids(XDMFDOMAIN * domain)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  return domainPointer->getNumberRectilinearGrids();
}

void XdmfDomainInsertRectilinearGrid(XDMFDOMAIN * domain, XDMFRECTILINEARGRID * RectilinearGrid, int passControl)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  if (passControl) {
    domainPointer->insert(shared_ptr<XdmfRectilinearGrid>((XdmfRectilinearGrid *)RectilinearGrid));
  }
  else {
    domainPointer->insert(shared_ptr<XdmfRectilinearGrid>((XdmfRectilinearGrid *)RectilinearGrid, XdmfNullDeleter()));
  }
}

void XdmfDomainRemoveRectilinearGrid(XDMFDOMAIN * domain, unsigned int index)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  domainPointer->removeRectilinearGrid(index);
}

void XdmfDomainRemoveRectilinearGridByName(XDMFDOMAIN * domain, char * Name)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  domainPointer->removeRectilinearGrid(Name);
}

XDMFREGULARGRID * XdmfDomainGetRegularGrid(XDMFDOMAIN * domain, unsigned int index)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  return (XDMFREGULARGRID *)((void *)((XdmfItem *)(domainPointer->getRegularGrid(index).get())));
}

XDMFREGULARGRID * XdmfDomainGetRegularGridByName(XDMFDOMAIN * domain, char * Name)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  return (XDMFREGULARGRID *)((void *)((XdmfItem *)(domainPointer->getRegularGrid(Name).get())));
}

unsigned int XdmfDomainGetNumberRegularGrids(XDMFDOMAIN * domain)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  return domainPointer->getNumberRegularGrids();
}

void XdmfDomainInsertRegularGrid(XDMFDOMAIN * domain, XDMFREGULARGRID * RegularGrid, int passControl)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  if (passControl) {
    domainPointer->insert(shared_ptr<XdmfRegularGrid>((XdmfRegularGrid *)RegularGrid));
  }
  else {
    domainPointer->insert(shared_ptr<XdmfRegularGrid>((XdmfRegularGrid *)RegularGrid, XdmfNullDeleter()));
  }
}

void XdmfDomainRemoveRegularGrid(XDMFDOMAIN * domain, unsigned int index)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  domainPointer->removeRegularGrid(index);
}

void XdmfDomainRemoveRegularGridByName(XDMFDOMAIN * domain, char * Name)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  domainPointer->removeRegularGrid(Name);
}

XDMFUNSTRUCTUREDGRID * XdmfDomainGetUnstructuredGrid(XDMFDOMAIN * domain, unsigned int index)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  return (XDMFUNSTRUCTUREDGRID *)((void *)((XdmfItem *)(domainPointer->getUnstructuredGrid(index).get())));
}

XDMFUNSTRUCTUREDGRID * XdmfDomainGetUnstructuredGridByName(XDMFDOMAIN * domain, char * Name)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  return (XDMFUNSTRUCTUREDGRID *)((void *)((XdmfItem *)(domainPointer->getUnstructuredGrid(Name).get())));
}

unsigned int XdmfDomainGetNumberUnstructuredGrids(XDMFDOMAIN * domain)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  return domainPointer->getNumberUnstructuredGrids();
}

void XdmfDomainInsertUnstructuredGrid(XDMFDOMAIN * domain, XDMFUNSTRUCTUREDGRID * UnstructuredGrid, int passControl)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  if (passControl) {
    domainPointer->insert(shared_ptr<XdmfUnstructuredGrid>((XdmfUnstructuredGrid *)UnstructuredGrid));
  }
  else {
    domainPointer->insert(shared_ptr<XdmfUnstructuredGrid>((XdmfUnstructuredGrid *)UnstructuredGrid, XdmfNullDeleter()));
  }
}

void XdmfDomainRemoveUnstructuredGrid(XDMFDOMAIN * domain, unsigned int index)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  domainPointer->removeUnstructuredGrid(index);
}

void XdmfDomainRemoveUnstructuredGridByName(XDMFDOMAIN * domain, char * Name)
{
  XdmfItem * classedPointer = (XdmfItem *)domain;
  XdmfDomain * domainPointer = dynamic_cast<XdmfDomain *>(classedPointer);
  domainPointer->removeUnstructuredGrid(Name);
}

XDMF_ITEM_C_CHILD_WRAPPER(XdmfDomain, XDMFDOMAIN)
