/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXdmf3HeavyDataHandler.cxx
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkXdmf3HeavyDataHandler.h"

#include "vtkCompositeDataSet.h"
#include "vtkDataObject.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkUniformGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXdmf3ArrayKeeper.h"
#include "vtkXdmf3ArraySelection.h"
#include "vtkXdmf3DataSet.h"

#include "XdmfCurvilinearGrid.hpp"
#include "XdmfDomain.hpp"
#include "XdmfGraph.hpp"
#include "XdmfGrid.hpp"
#include "XdmfGridCollection.hpp"
#include "XdmfGridCollectionType.hpp"
#include "XdmfItem.hpp"
#include "XdmfRectilinearGrid.hpp"
#include "XdmfRegularGrid.hpp"
#include "XdmfSet.hpp"
#include "XdmfUnstructuredGrid.hpp"

//------------------------------------------------------------------------------
shared_ptr<vtkXdmf3HeavyDataHandler> vtkXdmf3HeavyDataHandler::New (
   vtkXdmf3ArraySelection *fs,
   vtkXdmf3ArraySelection *cs,
   vtkXdmf3ArraySelection *ps,
   vtkXdmf3ArraySelection *gc,
   vtkXdmf3ArraySelection *sc,
   unsigned int processor, unsigned int nprocessors,
   bool dt, double t,
   vtkXdmf3ArrayKeeper *keeper,
   bool asTime)
{
  shared_ptr<vtkXdmf3HeavyDataHandler> p(new vtkXdmf3HeavyDataHandler());
  p->FieldArrays = fs;
  p->CellArrays = cs;
  p->PointArrays = ps;
  p->GridsCache = gc;
  p->SetsCache = sc;
  p->Rank = processor;
  p->NumProcs = nprocessors;
  p->doTime = dt;
  p->time = t;
  p->Keeper = keeper;
  p->AsTime = asTime;
  return p;
}

//------------------------------------------------------------------------------
vtkXdmf3HeavyDataHandler::vtkXdmf3HeavyDataHandler()
{
}

//------------------------------------------------------------------------------
vtkXdmf3HeavyDataHandler::~vtkXdmf3HeavyDataHandler()
{
}

//------------------------------------------------------------------------------
vtkDataObject *vtkXdmf3HeavyDataHandler::Populate(
  shared_ptr<XdmfItem> item, vtkDataObject *toFill)
{
  assert(toFill);

  shared_ptr<XdmfDomain> group = shared_dynamic_cast<XdmfDomain>(item);

  if (!group)
  {
    shared_ptr<XdmfUnstructuredGrid> unsGrid =
      shared_dynamic_cast<XdmfUnstructuredGrid>(item);
    if (unsGrid)
    {
      unsigned int nSets = unsGrid->getNumberSets();
      if (nSets > 0)
      {
        vtkMultiBlockDataSet *mbds =
          vtkMultiBlockDataSet::SafeDownCast(toFill);
        vtkUnstructuredGrid *child = vtkUnstructuredGrid::New();
        mbds->SetBlock
          (0,
           this->MakeUnsGrid
           (unsGrid, child, this->Keeper));
        mbds->GetMetaData((unsigned int)0)->Set(vtkCompositeDataSet::NAME(),
          unsGrid->getName().c_str());
        for (unsigned int i = 0; i < nSets; i++)
        {
          vtkUnstructuredGrid *sub = vtkUnstructuredGrid::New();
          mbds->SetBlock
            (i+1,
             this->ExtractSet
             (i, unsGrid, child, sub, this->Keeper));
          mbds->GetMetaData(i+1)->Set(vtkCompositeDataSet::NAME(),
            unsGrid->getSet(i)->getName().c_str());
          sub->Delete();
        }
        child->Delete();
        return mbds;
      }
      return this->MakeUnsGrid(unsGrid,
                               vtkUnstructuredGrid::SafeDownCast(toFill),
                               this->Keeper);
    }

    shared_ptr<XdmfRectilinearGrid> recGrid =
      shared_dynamic_cast<XdmfRectilinearGrid>(item);
    if (recGrid)
    {
      unsigned int nSets = recGrid->getNumberSets();
      if (nSets > 0)
      {
        vtkMultiBlockDataSet *mbds =
          vtkMultiBlockDataSet::SafeDownCast(toFill);
        vtkRectilinearGrid *child = vtkRectilinearGrid::New();
        mbds->SetBlock
          (0,
           this->MakeRecGrid
           (recGrid, child, this->Keeper));
        mbds->GetMetaData((unsigned int)0)->Set(vtkCompositeDataSet::NAME(),
          recGrid->getName().c_str());
        for (unsigned int i = 0; i < nSets; i++)
        {
          vtkUnstructuredGrid *sub = vtkUnstructuredGrid::New();
          mbds->SetBlock
            (i+1,
             this->ExtractSet
             (i, recGrid, child, sub, this->Keeper));
          mbds->GetMetaData(i+1)->Set(vtkCompositeDataSet::NAME(),
            recGrid->getSet(i)->getName().c_str());
          sub->Delete();
        }
        child->Delete();
        return mbds;
      }
      return this->MakeRecGrid(recGrid,
                               vtkRectilinearGrid::SafeDownCast(toFill),
                               this->Keeper);
    }

    shared_ptr<XdmfCurvilinearGrid> crvGrid =
      shared_dynamic_cast<XdmfCurvilinearGrid>(item);
    if (crvGrid)
    {
      unsigned int nSets = crvGrid->getNumberSets();
      if (nSets > 0)
      {
        vtkMultiBlockDataSet *mbds =
          vtkMultiBlockDataSet::SafeDownCast(toFill);
        vtkStructuredGrid *child = vtkStructuredGrid::New();
        mbds->SetBlock
          (0,
           this->MakeCrvGrid
           (crvGrid, child, this->Keeper));
        mbds->GetMetaData((unsigned int)0)->Set(vtkCompositeDataSet::NAME(),
          crvGrid->getName().c_str());
        for (unsigned int i = 0; i < nSets; i++)
        {
          vtkUnstructuredGrid *sub = vtkUnstructuredGrid::New();
          mbds->SetBlock
            (i+1,
             this->ExtractSet
             (i, crvGrid, child, sub, this->Keeper));
          mbds->GetMetaData(i+1)->Set(vtkCompositeDataSet::NAME(),
            crvGrid->getSet(i)->getName().c_str());
          sub->Delete();
        }
        child->Delete();
        return mbds;
      }
      return this->MakeCrvGrid(crvGrid,
                               vtkStructuredGrid::SafeDownCast(toFill),
                               this->Keeper);
    }

    shared_ptr<XdmfRegularGrid> regGrid =
      shared_dynamic_cast<XdmfRegularGrid>(item);
    if (regGrid)
    {
      unsigned int nSets = regGrid->getNumberSets();
      if (nSets > 0)
      {
        vtkMultiBlockDataSet *mbds =
          vtkMultiBlockDataSet::SafeDownCast(toFill);
        vtkImageData *child = vtkImageData::New();
        mbds->SetBlock
          (0,
           this->MakeRegGrid
           (regGrid, child, this->Keeper));
        mbds->GetMetaData((unsigned int)0)->Set(vtkCompositeDataSet::NAME(),
            regGrid->getName().c_str());
        for (unsigned int i = 0; i < nSets; i++)
        {
          vtkUnstructuredGrid *sub = vtkUnstructuredGrid::New();
          mbds->SetBlock
            (i+1,
             this->ExtractSet
             (i, regGrid, child, sub, this->Keeper));
          mbds->GetMetaData(i+1)->Set(vtkCompositeDataSet::NAME(),
            crvGrid->getSet(i)->getName().c_str());
          sub->Delete();
        }
        child->Delete();
        return mbds;
      }
      return this->MakeRegGrid(regGrid,
                               vtkImageData::SafeDownCast(toFill),
                               this->Keeper);
    }

    shared_ptr<XdmfGraph> graph = shared_dynamic_cast<XdmfGraph>(item);
    if (graph)
    {
      return this->MakeGraph(graph,
                             vtkMutableDirectedGraph::SafeDownCast(toFill),
                             this->Keeper);
    }

    return NULL; //already spit a warning out before this
  }

  shared_ptr<XdmfGridCollection> asGC =
    shared_dynamic_cast<XdmfGridCollection>(item);
  bool isDomain = asGC?false:true;
  bool isTemporal = false;
  if (asGC && asGC->getType() == XdmfGridCollectionType::Temporal())
  {
    isTemporal = true;
  }

  //ignore groups that are not in timestep we were asked for
  //but be sure to return everything within them
  bool lastTime = this->doTime;
  if (this->doTime && !(isDomain || isTemporal) && asGC->getTime())
  {
    if (asGC->getTime()->getValue() != this->time)
    {
      //don't return MB that doesn't match the requested time
      return NULL;
    }

    //inside a match, make sure we get everything underneath
    this->doTime = false;
  }

  vtkMultiBlockDataSet *topB = vtkMultiBlockDataSet::SafeDownCast(toFill);
  vtkDataObject *result;
  unsigned int cnt = 0;
  unsigned int nGridCollections = group->getNumberGridCollections();

  for (unsigned int i = 0; i < nGridCollections; i++)
  {
    if (!this->AsTime)
    {
      if (isDomain && !this->ShouldRead(i,nGridCollections))
      {
        topB->SetBlock(cnt++, NULL);
        continue;
      }
      vtkMultiBlockDataSet *child = vtkMultiBlockDataSet::New();
      result = this->Populate(group->getGridCollection(i), child);
      topB->SetBlock(cnt++, result);
      child->Delete();
    }
    else
    {
      vtkMultiBlockDataSet *child = vtkMultiBlockDataSet::New();
      result = this->Populate(group->getGridCollection(i), child);
      if (result)
      {
        topB->SetBlock(cnt++, result);
      }
      child->Delete();
    }
  }
  unsigned int nUnstructuredGrids = group->getNumberUnstructuredGrids();
  for (unsigned int i = 0; i < nUnstructuredGrids; i++)
  {
    if (this->AsTime && !isTemporal && !this->ShouldRead(i,nUnstructuredGrids))
    {
      topB->SetBlock(cnt++, NULL);
      continue;
    }
    shared_ptr<XdmfUnstructuredGrid> cGrid = group->getUnstructuredGrid(i);
    unsigned int nSets = cGrid->getNumberSets();
    vtkDataObject *child;
    if (nSets > 0)
    {
      child = vtkMultiBlockDataSet::New();
    }
    else
    {
      child = vtkUnstructuredGrid::New();
    }
    result = this->Populate(group->getUnstructuredGrid(i), child);
    if (result)
    {
      topB->SetBlock(cnt, result);
      topB->GetMetaData(cnt)->Set(vtkCompositeDataSet::NAME(),
        cGrid->getName().c_str());
      cnt++;
    }
    child->Delete();
  }
  unsigned int nRectilinearGrids = group->getNumberRectilinearGrids();
  for (unsigned int i = 0; i < nRectilinearGrids; i++)
  {
    if (this->AsTime && !isTemporal && !this->ShouldRead(i,nRectilinearGrids))
    {
      topB->SetBlock(cnt++, NULL);
      continue;
    }
    shared_ptr<XdmfRectilinearGrid> cGrid = group->getRectilinearGrid(i);
    unsigned int nSets = cGrid->getNumberSets();
    vtkDataObject *child;
    if (nSets > 0)
    {
      child = vtkMultiBlockDataSet::New();
    }
    else
    {
      child = vtkRectilinearGrid::New();
    }
    result = this->Populate(cGrid, child);
    if (result)
    {
      topB->SetBlock(cnt, result);
      topB->GetMetaData(cnt)->Set(vtkCompositeDataSet::NAME(),
        cGrid->getName().c_str());
      cnt++;
    }
    child->Delete();
  }
  unsigned int nCurvilinearGrids= group->getNumberCurvilinearGrids();
  for (unsigned int i = 0; i < nCurvilinearGrids; i++)
  {
    if (this->AsTime && !isTemporal && !this->ShouldRead(i,nCurvilinearGrids))
    {
      topB->SetBlock(cnt++, NULL);
      continue;
    }
    shared_ptr<XdmfCurvilinearGrid> cGrid = group->getCurvilinearGrid(i);
    unsigned int nSets = cGrid->getNumberSets();
    vtkDataObject *child;
    if (nSets > 0)
    {
      child = vtkMultiBlockDataSet::New();
    }
    else
    {
      child = vtkStructuredGrid::New();
    }
    result = this->Populate(cGrid, child);
    if (result)
    {
      topB->SetBlock(cnt, result);
      topB->GetMetaData(cnt)->Set(vtkCompositeDataSet::NAME(),
        cGrid->getName().c_str());
      cnt++;
    }
    child->Delete();
  }
  unsigned int nRegularGrids = group->getNumberRegularGrids();
  for (unsigned int i = 0; i < nRegularGrids; i++)
  {
    if (this->AsTime && !isTemporal && !this->ShouldRead(i,nRegularGrids))
    {
      topB->SetBlock(cnt++, NULL);
      continue;
    }
    shared_ptr<XdmfRegularGrid> cGrid = group->getRegularGrid(i);
    unsigned int nSets = cGrid->getNumberSets();
    vtkDataObject *child;
    if (nSets > 0)
    {
      child = vtkMultiBlockDataSet::New();
    }
    else
    {
      child = vtkUniformGrid::New();
    }
    result = this->Populate(cGrid, child);
    if (result)
    {
      topB->SetBlock(cnt, result);
      topB->GetMetaData(cnt)->Set(vtkCompositeDataSet::NAME(),
        cGrid->getName().c_str());
      cnt++;
    }
    child->Delete();
  }
  unsigned int nGraphs = group->getNumberGraphs();
  for (unsigned int i = 0; i < nGraphs; i++)
  {
    if (this->AsTime && !isTemporal && !this->ShouldRead(i,nGraphs))
    {
      topB->SetBlock(cnt++, NULL);
      continue;
    }
    vtkMutableDirectedGraph *child = vtkMutableDirectedGraph::New();
    result = this->Populate(group->getGraph(i), child);
    if (result)
    {
      topB->SetBlock(cnt, result);
      topB->GetMetaData(cnt)->Set(vtkCompositeDataSet::NAME(),
        group->getGraph(i)->getName().c_str());
      cnt++;
    }
    child->Delete();
  }

  if (lastTime)
  {
    //restore time search now that we've done the group contents
    this->doTime = true;
  }

  if (isTemporal && topB->GetNumberOfBlocks()==1)
  {
    //temporal collection is just a place holder for its content
    return topB->GetBlock(0);
  }

  return topB;
}

//------------------------------------------------------------------------------
bool vtkXdmf3HeavyDataHandler::ShouldRead(unsigned int piece,
                                          unsigned int npieces)
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

//------------------------------------------------------------------------------
bool vtkXdmf3HeavyDataHandler::GridEnabled(shared_ptr<XdmfGrid> grid)
{
  return this->GridsCache->ArrayIsEnabled(grid->getName().c_str());
}

//------------------------------------------------------------------------------
bool vtkXdmf3HeavyDataHandler::GridEnabled(shared_ptr<XdmfGraph> graph)
{
  return this->GridsCache->ArrayIsEnabled(graph->getName().c_str());
}

//------------------------------------------------------------------------------
bool vtkXdmf3HeavyDataHandler::SetEnabled(shared_ptr<XdmfSet> set)
{
  return this->SetsCache->ArrayIsEnabled(set->getName().c_str());
}

//------------------------------------------------------------------------------
bool vtkXdmf3HeavyDataHandler::ForThisTime(shared_ptr<XdmfGrid> grid)
{
  return (!this->doTime ||
          (grid->getTime() &&
           grid->getTime()->getValue() == this->time));
}

//------------------------------------------------------------------------------
bool vtkXdmf3HeavyDataHandler::ForThisTime(shared_ptr<XdmfGraph> graph)
{
  return (!this->doTime ||
          (graph->getTime() &&
           graph->getTime()->getValue() == this->time));
}

//------------------------------------------------------------------------------
vtkDataObject *vtkXdmf3HeavyDataHandler::MakeUnsGrid
 (
  shared_ptr<XdmfUnstructuredGrid> grid,
  vtkUnstructuredGrid *dataSet,
  vtkXdmf3ArrayKeeper *keeper)
{
  if (dataSet && this->GridEnabled(grid) && this->ForThisTime(grid))
  {
    vtkXdmf3DataSet::XdmfToVTK
      (
       this->FieldArrays, this->CellArrays, this->PointArrays,
       grid.get(), dataSet, keeper);
    return dataSet;
  }
  return NULL;
}

//------------------------------------------------------------------------------
vtkDataObject *vtkXdmf3HeavyDataHandler::MakeRecGrid
  (
   shared_ptr<XdmfRectilinearGrid> grid,
   vtkRectilinearGrid *dataSet,
   vtkXdmf3ArrayKeeper *keeper)
{
  if (dataSet && this->GridEnabled(grid) && this->ForThisTime(grid))
  {
    vtkXdmf3DataSet::XdmfToVTK
      (
       this->FieldArrays, this->CellArrays, this->PointArrays,
       grid.get(), dataSet, keeper);
    return dataSet;
  }
  return NULL;
}

//------------------------------------------------------------------------------
vtkDataObject *vtkXdmf3HeavyDataHandler::MakeCrvGrid
  (
   shared_ptr<XdmfCurvilinearGrid> grid,
   vtkStructuredGrid *dataSet,
   vtkXdmf3ArrayKeeper *keeper)
{
  if (dataSet && this->GridEnabled(grid) && this->ForThisTime(grid))
  {
    vtkXdmf3DataSet::XdmfToVTK
      (
       this->FieldArrays, this->CellArrays, this->PointArrays,
       grid.get(), dataSet, keeper);
    return dataSet;
  }
  return NULL;
}

//------------------------------------------------------------------------------
vtkDataObject *vtkXdmf3HeavyDataHandler::MakeRegGrid
  (
   shared_ptr<XdmfRegularGrid> grid,
   vtkImageData *dataSet,
   vtkXdmf3ArrayKeeper *keeper)
{
  if (dataSet && this->GridEnabled(grid) && this->ForThisTime(grid))
  {
    vtkXdmf3DataSet::XdmfToVTK
      (
       this->FieldArrays, this->CellArrays, this->PointArrays,
       grid.get(), dataSet, keeper);
    return dataSet;
  }
  return NULL;
}

//------------------------------------------------------------------------------
vtkDataObject *vtkXdmf3HeavyDataHandler::MakeGraph
  (
   shared_ptr<XdmfGraph> grid,
   vtkMutableDirectedGraph *dataSet,
   vtkXdmf3ArrayKeeper *keeper)
{
  if (dataSet && this->GridEnabled(grid) && this->ForThisTime(grid))
  {
    vtkXdmf3DataSet::XdmfToVTK
      (
       this->FieldArrays, this->CellArrays, this->PointArrays,
       grid.get(), dataSet, keeper);
    return dataSet;
  }
  return NULL;
}

//------------------------------------------------------------------------------
vtkDataObject *vtkXdmf3HeavyDataHandler::ExtractSet
  (
   unsigned int setnum, shared_ptr<XdmfGrid> grid,
   vtkDataSet *dataSet,
   vtkUnstructuredGrid *subSet, vtkXdmf3ArrayKeeper *keeper)
{
  shared_ptr<XdmfSet> set = grid->getSet(setnum);
  if (dataSet && subSet && SetEnabled(set) && this->ForThisTime(grid))
  {
    vtkXdmf3DataSet::XdmfSubsetToVTK
      (
       grid.get(), setnum, dataSet, subSet, keeper);
    return subSet;
  }
  return NULL;
}
