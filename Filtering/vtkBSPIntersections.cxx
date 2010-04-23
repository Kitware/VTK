/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBSPIntersections.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkBSPIntersections.h"
#include "vtkBSPCuts.h"
#include "vtkKdNode.h"
#include "vtkObjectFactory.h"
#include "vtkCell.h"
#include "vtkPoints.h"
#include "vtkMath.h"

#include <vtkstd/set>


vtkStandardNewMacro(vtkBSPIntersections);

#define REGIONCHECK(err)     \
if (this->BuildRegionList()) \
  {                          \
  return err;                \
  }

#define REGIONIDCHECK_RETURNERR(id, err) \
if (this->BuildRegionList()) \
  {                          \
  return err;                \
  }                          \
if ((id < 0) || (id >= this->NumberOfRegions)) \
  {                                            \
  vtkErrorMacro(<< "Invalid region ID");       \
  return err;                                  \
  }

#define REGIONIDCHECK(id)    \
if (this->BuildRegionList()) \
  {                          \
  return;                    \
  }                          \
if ((id < 0) || (id >= this->NumberOfRegions)) \
  {                                            \
  vtkErrorMacro(<< "Invalid region ID");       \
  return;                                      \
  }

//----------------------------------------------------------------------------

vtkCxxSetObjectMacro(vtkBSPIntersections, Cuts, vtkBSPCuts)

//----------------------------------------------------------------------------

// Don't use vtkSetMacro or vtkBooleanMacro on these.  They will
// update the Mtime, which is incorrect.

void vtkBSPIntersections::SetComputeIntersectionsUsingDataBounds(int c)
{
  this->ComputeIntersectionsUsingDataBounds = (c != 0);
}

void vtkBSPIntersections::ComputeIntersectionsUsingDataBoundsOn()
{
  this->ComputeIntersectionsUsingDataBounds = 1;
}

void vtkBSPIntersections::ComputeIntersectionsUsingDataBoundsOff()
{
  this->ComputeIntersectionsUsingDataBounds = 0;
}

//----------------------------------------------------------------------------
vtkBSPIntersections::vtkBSPIntersections()
{
  this->Cuts = NULL;
  this->NumberOfRegions = 0;
  this->RegionList = NULL;
  this->ComputeIntersectionsUsingDataBounds = 0;
  vtkMath::UninitializeBounds(this->CellBoundsCache);
}

//----------------------------------------------------------------------------
vtkBSPIntersections::~vtkBSPIntersections()
{
  this->SetCuts(NULL);
  if (this->RegionList)
    {
    delete [] this->RegionList;
    }
}
//----------------------------------------------------------------------------
int vtkBSPIntersections::BuildRegionList()
{
  if ((this->RegionList != NULL) &&
      (this->RegionListBuildTime > this->GetMTime()))
    {
    return 0;
    }
      
  if (this->RegionList)
    {
    delete [] this->RegionList;
    this->RegionList = NULL;
    }

  vtkKdNode *top = NULL;
  if (this->Cuts)
    {
    top = this->Cuts->GetKdNodeTree();
    }

  if (!top)
    {
    return 1;
    }

  this->NumberOfRegions = vtkBSPIntersections::NumberOfLeafNodes(top);

  if (this->NumberOfRegions < 1)
    {
    vtkErrorMacro(<< "vtkBSPIntersections::BuildRegionList no cuts in vtkBSPCut object");
    return 1;
    }

  this->RegionList = new vtkKdNode * [this->NumberOfRegions];

  if (!this->RegionList)
    {
    vtkErrorMacro(<< "vtkBSPIntersections::BuildRegionList memory allocation");
    return 1;
    }

  int fail = this->SelfRegister(top);

  if (fail)
    {
    vtkErrorMacro(<< "vtkBSPIntersections::BuildRegionList bad ids in vtkBSPCut object");
    return 1;
    }

  int min=0;
  int max=0;

  vtkBSPIntersections::SetIDRanges(top, min, max);

  this->RegionListBuildTime.Modified();

  return 0;
}
//----------------------------------------------------------------------------
int vtkBSPIntersections::SelfRegister(vtkKdNode *kd)
{
  int fail = 0;

  if (kd->GetLeft() == NULL)
    {
    int id = kd->GetID();

    if ((id < 0) || (id >= this->NumberOfRegions))
      {
      return 1;
      }
    this->RegionList[id] = kd; 
    }
  else
    {
    fail = this->SelfRegister(kd->GetLeft());

    if (!fail)
      {
      fail = this->SelfRegister(kd->GetRight());
      }
    }

  return fail;
}
//----------------------------------------------------------------------------
int vtkBSPIntersections::NumberOfLeafNodes(vtkKdNode *kd)
{
  int nLeafNodes=1;

  if (kd->GetLeft() != NULL)
    {
    int numLeft = vtkBSPIntersections::NumberOfLeafNodes(kd->GetLeft());
    int numRight = vtkBSPIntersections::NumberOfLeafNodes(kd->GetRight());

    nLeafNodes = numLeft + numRight;
    }

  return nLeafNodes;
}
//----------------------------------------------------------------------------
void vtkBSPIntersections::SetIDRanges(vtkKdNode *kd, int &min, int &max)
{
  int tempMin=0;
  int tempMax=0;

  if (kd->GetLeft() == NULL)
    {
    min = kd->GetID();
    max = kd->GetID();
    }
  else
    {
    vtkBSPIntersections::SetIDRanges(kd->GetLeft(), min, max);
    vtkBSPIntersections::SetIDRanges(kd->GetRight(), tempMin, tempMax);
    max = (tempMax > max) ? tempMax : max;
    min = (tempMin < min) ? tempMin : min;
    }

  kd->SetMinID(min);
  kd->SetMaxID(max);
}
//----------------------------------------------------------------------------
int vtkBSPIntersections::GetBounds(double *bounds)
{
  REGIONCHECK(1);

  this->Cuts->GetKdNodeTree()->GetBounds(bounds);

  return 0;
}
//----------------------------------------------------------------------------
int vtkBSPIntersections::GetNumberOfRegions()
{
  REGIONCHECK(0)

  return this->NumberOfRegions;
}

//----------------------------------------------------------------------------
int vtkBSPIntersections::GetRegionBounds(int regionID, double bounds[6])
{
  REGIONIDCHECK_RETURNERR(regionID, 1)

  vtkKdNode *node = this->RegionList[regionID];

  node->GetBounds(bounds);

  return 0;
}

//----------------------------------------------------------------------------
int vtkBSPIntersections::GetRegionDataBounds(int regionID, double bounds[6])
{
  REGIONIDCHECK_RETURNERR(regionID, 1)

  vtkKdNode *node = this->RegionList[regionID];

  node->GetDataBounds(bounds);

  return 0;
}

//----------------------------------------------------------------------------
//  Query functions ----------------------------------------------------
//    K-d Trees are particularly efficient with region intersection
//    queries, like finding all regions that intersect a view frustum
//
// Intersection with axis-aligned box----------------------------------
//

//----------------------------------------------------------------------------
int vtkBSPIntersections::IntersectsBox(int regionId, double *x)
{
  return this->IntersectsBox(regionId, x[0], x[1], x[2], x[3], x[4], x[5]);
}

//----------------------------------------------------------------------------
int vtkBSPIntersections::IntersectsBox(int regionId, double x0, double x1, 
                           double y0, double y1, double z0, double z1)
{
  REGIONIDCHECK_RETURNERR(regionId, 0);

  vtkKdNode *node = this->RegionList[regionId];

  return node->IntersectsBox(x0, x1, y0, y1, z0, z1,
                     this->ComputeIntersectionsUsingDataBounds);
}

//----------------------------------------------------------------------------
int vtkBSPIntersections::IntersectsBox(int *ids, int len, double *x)
{
  return this->IntersectsBox(ids, len,  x[0], x[1], x[2], x[3], x[4], x[5]);
}

//----------------------------------------------------------------------------
int vtkBSPIntersections::IntersectsBox(int *ids, int len, 
                             double x0, double x1,
                             double y0, double y1, double z0, double z1)
{
  REGIONCHECK(0);

  int nnodes = 0;

  if (len > 0)
    {
    nnodes = this->_IntersectsBox(this->Cuts->GetKdNodeTree(), ids, len,
                             x0, x1, y0, y1, z0, z1);
    }
  return nnodes;
}

//----------------------------------------------------------------------------
int vtkBSPIntersections::_IntersectsBox(vtkKdNode *node, int *ids, int len, 
                             double x0, double x1,
                             double y0, double y1, double z0, double z1)
{
  int result, nnodes1, nnodes2, listlen;
  int *idlist;

  result = node->IntersectsBox(x0, x1, y0, y1, z0, z1,
                             this->ComputeIntersectionsUsingDataBounds);

  if (!result) 
    {
    return 0;
    }

  if (node->GetLeft() == NULL)
    {
    ids[0] = node->GetID();
    return 1;
    }

  nnodes1 = _IntersectsBox(node->GetLeft(), ids, len, x0, x1, y0, y1, z0, z1);

  idlist = ids + nnodes1;
  listlen = len - nnodes1;

  if (listlen > 0)
    {
    nnodes2 = _IntersectsBox(node->GetRight(), idlist, listlen, x0, x1, y0, y1, z0, z1);
    }
  else
    {
    nnodes2 = 0;
    }

  return (nnodes1 + nnodes2);
}

//----------------------------------------------------------------------------
// Intersection with a sphere---------------------------------------
//
int vtkBSPIntersections::IntersectsSphere2(int regionId, double x, double y, double z, 
                                double rSquared) 
{
  REGIONIDCHECK_RETURNERR(regionId, 0);

  vtkKdNode *node = this->RegionList[regionId];

  return node->IntersectsSphere2(x, y, z, rSquared,
                     this->ComputeIntersectionsUsingDataBounds);
}

//----------------------------------------------------------------------------
int vtkBSPIntersections::IntersectsSphere2(int *ids, int len,
                       double x, double y, double z, double rSquared)
{                            
  REGIONCHECK(0)

  int nnodes = 0;
  
  if (len > 0)
    {
    nnodes = this->_IntersectsSphere2(this->Cuts->GetKdNodeTree(), 
      ids, len, x, y, z, rSquared);
    }                        
  return nnodes;
} 

//----------------------------------------------------------------------------
int vtkBSPIntersections::_IntersectsSphere2(vtkKdNode *node, int *ids, int len,
                                  double x, double y, double z, double rSquared)
{                            
  int result, nnodes1, nnodes2, listlen;
  int *idlist;
  
  result = node->IntersectsSphere2(x, y, z, rSquared,
                             this->ComputeIntersectionsUsingDataBounds);
                             
  if (!result) 
    {
    return 0;
    }
  
  if (node->GetLeft() == NULL)
    {
    ids[0] = node->GetID();
    return 1;
    }
    
  nnodes1 = _IntersectsSphere2(node->GetLeft(), ids, len, x, y, z, rSquared);
  
  idlist = ids + nnodes1;
  listlen = len - nnodes1;
  
  if (listlen > 0)
    {
    nnodes2 = _IntersectsSphere2(node->GetRight(), idlist, listlen, x, y, z, rSquared);
    }
  else
    {
    nnodes2 = 0;
    }

  return (nnodes1 + nnodes2);
}

//----------------------------------------------------------------------------
// Intersection with arbitrary vtkCell -----------------------------
//

//----------------------------------------------------------------------------
int vtkBSPIntersections::IntersectsCell(int regionId, vtkCell *cell, int cellRegion)
{                            
  REGIONIDCHECK_RETURNERR(regionId, 0);

  vtkKdNode *node = this->RegionList[regionId];

  return node->IntersectsCell(cell, this->ComputeIntersectionsUsingDataBounds,
                              cellRegion);
}
//----------------------------------------------------------------------------
void vtkBSPIntersections::SetCellBounds(vtkCell *cell, double *bounds)
{   
  vtkPoints *pts = cell->GetPoints();
  pts->Modified();         // VTK bug - so bounds will be re-calculated
  pts->GetBounds(bounds);            
}
//----------------------------------------------------------------------------
int vtkBSPIntersections::IntersectsCell(int *ids, int len, vtkCell *cell, int cellRegion)
{
  REGIONCHECK(0)

  vtkBSPIntersections::SetCellBounds(cell, this->CellBoundsCache);

  return this->_IntersectsCell(this->Cuts->GetKdNodeTree(), ids, len, cell, cellRegion);
}
//----------------------------------------------------------------------------
int vtkBSPIntersections::_IntersectsCell(vtkKdNode *node, int *ids, int len,
                                 vtkCell *cell, int cellRegion)
{
  int result, nnodes1, nnodes2, listlen, intersects;
  int *idlist;

  intersects = node->IntersectsCell(cell,
                                this->ComputeIntersectionsUsingDataBounds,
                                cellRegion, this->CellBoundsCache);

  if (intersects)
    {
    if (node->GetLeft())
      {
      nnodes1 = this->_IntersectsCell(node->GetLeft(), ids, len, cell,
                                cellRegion);

      idlist = ids + nnodes1;
      listlen = len - nnodes1;
  
      if (listlen > 0) 
        {       
        nnodes2 = this->_IntersectsCell(node->GetRight(), idlist, listlen, cell,
                                  cellRegion);
        }
      else
        {
        nnodes2 = 0;
        }
  
      result = nnodes1 + nnodes2;
      }
    else
      {
      ids[0] = node->GetID();     // leaf node (spatial region)

      result = 1;
      }
    } 
  else
    {
    result = 0;
    }

  return result;
}

//----------------------------------------------------------------------------
void vtkBSPIntersections::PrintSelf(ostream& os, vtkIndent indent)
{ 
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Cuts: ";
  if( this->Cuts )
  {
    this->Cuts->PrintSelf(os << endl, indent.GetNextIndent() );
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "NumberOfRegions: " << this->NumberOfRegions << endl;
  os << indent << "RegionList: " << this->RegionList << endl;
  os << indent << "RegionListBuildTime: " << this->RegionListBuildTime << endl;
  os << indent << "ComputeIntersectionsUsingDataBounds: " << this->ComputeIntersectionsUsingDataBounds << endl;
  double *d = this->CellBoundsCache;
  os << indent << "CellBoundsCache " << d[0] << " " << d[1] << " " << d[2] << " " << d[3] << " " << d[4] << " " << d[5] << " " << endl;
}
