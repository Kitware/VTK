/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXdmf3LightDataHandler.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkXdmf3LightDataHandler.h"

#include "vtkXdmf3SILBuilder.h"
#include "vtkXdmf3ArraySelection.h"
#include "vtksys/SystemTools.hxx"

#include "XdmfAttribute.hpp"
#include "XdmfAttributeCenter.hpp"
#include "XdmfCurvilinearGrid.hpp"
#include "XdmfDomain.hpp"
#include "XdmfGraph.hpp"
#include "XdmfGridCollection.hpp"
#include "XdmfGridCollectionType.hpp"
#include "XdmfReader.hpp"
#include "XdmfRectilinearGrid.hpp"
#include "XdmfRegularGrid.hpp"
#include "XdmfSet.hpp"
#include "XdmfTime.hpp"
#include "XdmfUnstructuredGrid.hpp"
#include "XdmfVisitor.hpp"

#include <iostream>

//------------------------------------------------------------------------------
shared_ptr<vtkXdmf3LightDataHandler> vtkXdmf3LightDataHandler::New(
  vtkXdmf3SILBuilder *sb,
  vtkXdmf3ArraySelection* f,
  vtkXdmf3ArraySelection* ce,
  vtkXdmf3ArraySelection* pn,
  vtkXdmf3ArraySelection* gc,
  vtkXdmf3ArraySelection* sc,
  unsigned int processor,
  unsigned int nprocessors)
{
  shared_ptr<vtkXdmf3LightDataHandler> p(new vtkXdmf3LightDataHandler());
  p->SILBuilder = sb;
  p->FieldArrays = f;
  p->CellArrays = ce;
  p->PointArrays = pn;
  p->GridsCache = gc;
  p->SetsCache = sc;
  p->MaxDepth = 0;
  p->Rank = processor;
  p->NumProcs = nprocessors;
  return p;
}

//------------------------------------------------------------------------------
vtkXdmf3LightDataHandler::~vtkXdmf3LightDataHandler() {}

//------------------------------------------------------------------------------
vtkXdmf3LightDataHandler::vtkXdmf3LightDataHandler() {}

//------------------------------------------------------------------------------
void vtkXdmf3LightDataHandler::InspectXDMF
  (shared_ptr<XdmfItem> item, vtkIdType parentVertex, unsigned int depth)
{
  assert(this->SILBuilder);
  assert(this->FieldArrays);
  assert(this->PointArrays);
  assert(this->CellArrays);
  assert(this->GridsCache);
  assert(this->SetsCache);

  if (this->TooDeep(depth))
    {
    return;
    }

  this->InspectArrays(item);
  this->InspectTime(item);

  shared_ptr<XdmfDomain> coll = shared_dynamic_cast<XdmfDomain>(item);
  if (!coll)
    {
    if (this->SILBuilder->IsMaxedOut())
      {
      return;
      }

    shared_ptr<XdmfGrid> grid = shared_dynamic_cast<XdmfGrid>(item);
    if (grid)
      {
      //atomic dataset
      vtkIdType parent = parentVertex;
      if (parentVertex == -1)
        {
        parent = this->SILBuilder->GetHierarchyRoot();
        }
      unsigned int nSets = grid->getNumberSets();
      std::string name = grid->getName();
      if (name.length() != 0 && (nSets > 0 || parentVertex != -1))
        {
        std::string uName = this->UniqueName(name, true);
        grid->setName(uName);
        this->AddNamedBlock(parent, name, uName);
        for (unsigned int s = 0; s < nSets; s++)
          {
          shared_ptr<XdmfSet> set = grid->getSet(s);
          std::string sName = set->getName();
          std::string usName = this->UniqueName(sName, false);
          set->setName(usName);
          this->AddNamedSet(usName);
          }
        }
      return;
      }

    shared_ptr<XdmfGraph> graph = shared_dynamic_cast<XdmfGraph>(item);
    if (graph)
      {
      std::string name = graph->getName();
      if (name.length() != 0 && parentVertex != -1)
        {
        std::string uName = this->UniqueName(name, true);
        graph->setName(uName);
        this->AddNamedBlock(parentVertex, name, uName);
        }
      return;
      }

    std::cerr << "Found unknown Xdmf data type" << std::endl;
    return;
    }
  else
    {
    //four cases: domain, temporal, spatial or hierarchical
    shared_ptr<XdmfGridCollection> asGC =
      shared_dynamic_cast<XdmfGridCollection>(item);
    bool isDomain = asGC?false:true;

    bool isTemporal = false;
    if (asGC && asGC->getType() == XdmfGridCollectionType::Temporal())
      {
      isTemporal = true;
      }

    vtkIdType silVertex = parentVertex;
    if (!isTemporal && !isDomain)
      {
      std::string name = asGC->getName();
      if (name.length() != 0 && !this->SILBuilder->IsMaxedOut())
        {
        silVertex = this->SILBuilder->AddVertex(name.c_str());
        vtkIdType parent = parentVertex;
        if (parentVertex == -1)
          {
          //topmost entry, we are the root
          parent = this->SILBuilder->GetHierarchyRoot();
          }
        this->SILBuilder->AddChildEdge(parent, silVertex);
        }
      }

    unsigned int nGridCollections = coll->getNumberGridCollections();
    for (unsigned int i = 0; i < nGridCollections; i++)
      {
      if (isDomain && !this->ShouldRead(i, nGridCollections))
        {
        continue;
        }
      shared_ptr<XdmfGrid> child = coll->getGridCollection(i);
      this->InspectXDMF(child, silVertex, depth+1);
      }
    for (unsigned int i = 0; i < coll->getNumberUnstructuredGrids(); i++)
      {
      shared_ptr<XdmfGrid> child = coll->getUnstructuredGrid(i);
      this->InspectXDMF(child, silVertex, depth+1);
      }
    for (unsigned int i = 0; i < coll->getNumberRectilinearGrids(); i++)
      {
      shared_ptr<XdmfGrid> child = coll->getRectilinearGrid(i);
      this->InspectXDMF(child, silVertex, depth+1);
      }
    for (unsigned int i = 0; i < coll->getNumberCurvilinearGrids(); i++)
      {
      shared_ptr<XdmfGrid> child = coll->getCurvilinearGrid(i);
      this->InspectXDMF(child, silVertex, depth+1);
      }
    for (unsigned int i = 0; i < coll->getNumberRegularGrids(); i++)
      {
      shared_ptr<XdmfGrid> child = coll->getRegularGrid(i);
      this->InspectXDMF(child, silVertex, depth+1);
      }
    for (unsigned int i = 0; i < coll->getNumberGraphs(); i++)
      {
      shared_ptr<XdmfGraph> child = coll->getGraph(i);
      this->InspectXDMF(child, silVertex, depth+1);
      }
    }
}

//------------------------------------------------------------------------------
void vtkXdmf3LightDataHandler::ClearGridsIfNeeded(shared_ptr<XdmfItem> domain)
{
  if (this->SILBuilder->IsMaxedOut())
    {
    //too numerous to be of use to user for manual selection, so clear out
    this->GridsCache->clear();
    this->SetsCache->clear();
    this->SILBuilder->Initialize();
    this->MaxDepth = 4;
    this->InspectXDMF(domain, -1);
    }
}

//------------------------------------------------------------------------------
std::set<double> vtkXdmf3LightDataHandler::getTimes()
{
  return times;
}

//------------------------------------------------------------------------------
void vtkXdmf3LightDataHandler::InspectArrays(shared_ptr<XdmfItem> item)
{
  shared_ptr<XdmfGrid> grid = shared_dynamic_cast<XdmfGrid>(item);
  if (grid)
    {
    for (unsigned int cc=0; cc < grid->getNumberAttributes(); cc++)
      {
      shared_ptr<XdmfAttribute> xmfAttribute = grid->getAttribute(cc);
      std::string attrName = xmfAttribute->getName();
      if (attrName.length() == 0)
        {
        std::cerr << "Skipping unnamed array." << std::endl;
        continue;
        }
      shared_ptr<const XdmfAttributeCenter> attrCenter =
        xmfAttribute->getCenter();
      if (attrCenter == XdmfAttributeCenter::Grid())
        {
        if (!this->FieldArrays->HasArray(attrName.c_str()))
          {
          this->FieldArrays->AddArray(attrName.c_str());
          }
        }
      else if (attrCenter == XdmfAttributeCenter::Cell())
        {
        if (!this->CellArrays->HasArray(attrName.c_str()))
          {
          this->CellArrays->AddArray(attrName.c_str());
          }
        }
      else if (attrCenter == XdmfAttributeCenter::Node())
        {
        if (!this->PointArrays->HasArray(attrName.c_str()))
          {
          this->PointArrays->AddArray(attrName.c_str());
          }
        }
      else
        {
        std::cerr << "Skipping " << attrName << " unrecognized association"
                  << std::endl;
        continue;
        }
      }
    }
  else
    {
    shared_ptr<XdmfGraph> graph = shared_dynamic_cast<XdmfGraph>(item);
    if (graph)
      {
      for (unsigned int cc=0; cc < graph->getNumberAttributes(); cc++)
        {
        shared_ptr<XdmfAttribute> xmfAttribute = graph->getAttribute(cc);
        std::string attrName = xmfAttribute->getName();
        if (attrName.length() == 0)
          {
          std::cerr << "Skipping unnamed array." << std::endl;
          continue;
          }
        shared_ptr<const XdmfAttributeCenter> attrCenter =
          xmfAttribute->getCenter();
        if (attrCenter == XdmfAttributeCenter::Grid())
          {
          if (!this->FieldArrays->HasArray(attrName.c_str()))
            {
            this->FieldArrays->AddArray(attrName.c_str());
            }
          }
        else if (attrCenter == XdmfAttributeCenter::Edge())
          {
          if (!this->CellArrays->HasArray(attrName.c_str()))
            {
            this->CellArrays->AddArray(attrName.c_str());
            }
          }
        else if (attrCenter == XdmfAttributeCenter::Node())
          {
          if (!this->PointArrays->HasArray(attrName.c_str()))
            {
            this->PointArrays->AddArray(attrName.c_str());
            }
          }
        else
          {
          std::cerr << "Skipping " << attrName << " unrecognized association"
                    << std::endl;
          continue;
          }
        }
      }
    }
}

//------------------------------------------------------------------------------
bool vtkXdmf3LightDataHandler::TooDeep(unsigned int depth)
{
  if (this->MaxDepth != 0 && depth >= this->MaxDepth)
    {
    return true;
    }
  return false;
}

//------------------------------------------------------------------------------
std::string vtkXdmf3LightDataHandler::UniqueName(std::string name, bool ForGrid)
{
  std::string gridName = name;
  unsigned int count=1;

  vtkXdmf3ArraySelection* cache = (ForGrid?this->GridsCache:this->SetsCache);
  while (cache->HasArray(gridName.c_str()))
    {
    vtksys_ios::ostringstream str;
    str << name << "[" << count << "]";
    gridName = str.str();
    count++;
    }
  return gridName;
}

//------------------------------------------------------------------------------
void vtkXdmf3LightDataHandler::AddNamedBlock
  (vtkIdType parentVertex, std::string originalName, std::string uniqueName)
{
  this->GridsCache->AddArray(uniqueName.c_str());

  vtkIdType silVertex = this->SILBuilder->AddVertex(uniqueName.c_str());
  this->SILBuilder->AddChildEdge(this->SILBuilder->GetBlocksRoot(), silVertex);

  vtkIdType hierarchyVertex = this->SILBuilder->AddVertex(originalName.c_str());
  this->SILBuilder->AddChildEdge(parentVertex, hierarchyVertex);
  this->SILBuilder->AddCrossEdge(hierarchyVertex, silVertex);
}

//------------------------------------------------------------------------------
void vtkXdmf3LightDataHandler::AddNamedSet
  (std::string uniqueName)
{
  this->SetsCache->AddArray(uniqueName.c_str());
}

//records times that xdmf grids supply data at
//if timespecs are only implied we add them to make things simpler later on
void vtkXdmf3LightDataHandler::InspectTime(shared_ptr<XdmfItem> item)
{
  shared_ptr<XdmfGridCollection> gc =
    shared_dynamic_cast<XdmfGridCollection>(item);
  if (gc && gc->getType() == XdmfGridCollectionType::Temporal())
    {
    unsigned int cnt = 0;
    for (unsigned int i = 0; i < gc->getNumberGridCollections(); i++)
      {
      shared_ptr<XdmfGrid> child = gc->getGridCollection(i);
      this->GetSetTime(child, cnt);
      }
    for (unsigned int i = 0; i < gc->getNumberUnstructuredGrids(); i++)
      {
      shared_ptr<XdmfGrid> child = gc->getUnstructuredGrid(i);
      this->GetSetTime(child, cnt);
      }
    for (unsigned int i = 0; i < gc->getNumberRectilinearGrids(); i++)
      {
      shared_ptr<XdmfGrid> child = gc->getRectilinearGrid(i);
      this->GetSetTime(child, cnt);
      }
    for (unsigned int i = 0; i < gc->getNumberCurvilinearGrids(); i++)
      {
      shared_ptr<XdmfGrid> child = gc->getCurvilinearGrid(i);
      this->GetSetTime(child, cnt);
      }
    for (unsigned int i = 0; i < gc->getNumberRegularGrids(); i++)
      {
      shared_ptr<XdmfGrid> child = gc->getRegularGrid(i);
      this->GetSetTime(child, cnt);
      }
    for (unsigned int i = 0; gc->getNumberGraphs(); i++)
      {
      shared_ptr<XdmfGraph> child = gc->getGraph(i);
      this->GetSetTime(child, cnt);
      }
    }
}

//------------------------------------------------------------------------------
void vtkXdmf3LightDataHandler::GetSetTime
  (shared_ptr<XdmfGrid> child, unsigned int &cnt)
{
  if (!child->getTime())
    {
    //grid collections without explicit times are implied to go 0...N
    //so we add them here if not present
    shared_ptr<XdmfTime> time = XdmfTime::New(cnt++);
    child->setTime(time);
    }
  times.insert(child->getTime()->getValue());
}

//------------------------------------------------------------------------------
void vtkXdmf3LightDataHandler::GetSetTime
  (shared_ptr<XdmfGraph> child, unsigned int &cnt)
{
  if (!child->getTime())
    {
    //grid collections without explicit times are implied to go 0...N
    //so we add them here if not present
    shared_ptr<XdmfTime> time = XdmfTime::New(cnt++);
    child->setTime(time);
    }
  times.insert(child->getTime()->getValue());
}

//------------------------------------------------------------------------------
bool vtkXdmf3LightDataHandler::ShouldRead
  (unsigned int piece, unsigned int npieces)
{
  if (this->NumProcs<1)
    {
    //no parallel information given to us, assume serial
    return true;
    }
  if (npieces == 1)
    {
    return true;
    }
  if (npieces < this->NumProcs)
    {
    if (piece == this->Rank)
      {
      return true;
      }
    return false;
    }

#if 1
  unsigned int mystart = this->Rank*npieces/this->NumProcs;
  unsigned int myend = (this->Rank+1)*npieces/this->NumProcs;
  if (piece >= mystart)
    {
    if (piece < myend || (this->Rank==this->NumProcs-1))
      {
      return true;
      }
    }
  return false;
#else
  if ((piece % this->NumProcs) == this->Rank)
    {
    return true;
    }
  else
    {
    return false;
    }
#endif
}
