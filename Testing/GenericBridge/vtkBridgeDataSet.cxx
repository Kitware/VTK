/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBridgeDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBridgeDataSet - Implementation of vtkGenericDataSet.
// .SECTION Description
// It is just an example that show how to implement the Generic. It is also
// used for testing and evaluating the Generic.

#include "vtkBridgeDataSet.h"

#include <cassert>

#include "vtkObjectFactory.h"
#include "vtkDataSet.h"
#include "vtkCellTypes.h"
#include "vtkCell.h"
#include "vtkBridgeCellIterator.h"
#include "vtkBridgePointIterator.h"
#include "vtkBridgeCell.h"
#include "vtkGenericCell.h"
#include "vtkMath.h"
#include "vtkGenericAttributeCollection.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkBridgeAttribute.h"
#include "vtkGenericCellTessellator.h"
#include "vtkGenericEdgeTable.h"
#include "vtkSimpleCellTessellator.h"

vtkStandardNewMacro(vtkBridgeDataSet);

//----------------------------------------------------------------------------
// Default constructor.
vtkBridgeDataSet::vtkBridgeDataSet(  )
{
  this->Implementation = 0;
  this->Types = vtkCellTypes::New();
  this->Tessellator = vtkSimpleCellTessellator::New();
}

//----------------------------------------------------------------------------
vtkBridgeDataSet::~vtkBridgeDataSet(  )
{
  if(this->Implementation)
  {
    this->Implementation->Delete();
  }
  this->Types->Delete();
  // this->Tessellator is deleted in the superclass
}

//----------------------------------------------------------------------------
void vtkBridgeDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "implementation: ";
  if(this->Implementation==0)
  {
    os << "(none)" << endl;
  }
  else
  {
    this->Implementation->PrintSelf(os << endl, indent.GetNextIndent());
  }
}

//----------------------------------------------------------------------------
// Description:
// Return the dataset that will be manipulated through the adaptor interface.
vtkDataSet *vtkBridgeDataSet::GetDataSet()
{
  return this->Implementation;
}

//----------------------------------------------------------------------------
// Description:
// Set the dataset that will be manipulated through the adaptor interface.
// \pre ds_exists: ds!=0
void vtkBridgeDataSet::SetDataSet(vtkDataSet *ds)
{
  int i;
  int c;
  vtkPointData *pd;
  vtkCellData *cd;
  vtkBridgeAttribute *a;

  vtkSetObjectBodyMacro(Implementation,vtkDataSet,ds);
  // refresh the attribute collection
  this->Attributes->Reset();
  if(ds!=0)
  {
    // point data
    pd=ds->GetPointData();
    c=pd->GetNumberOfArrays();
    i=0;
    while(i<c)
    {
      a=vtkBridgeAttribute::New();
      a->InitWithPointData(pd,i);
      this->Attributes->InsertNextAttribute(a);
      a->Delete();
      ++i;
    }
    // same thing for cell data.
    cd=ds->GetCellData();
    c=cd->GetNumberOfArrays();
    i=0;
    while(i<c)
    {
      a=vtkBridgeAttribute::New();
      a->InitWithCellData(cd,i);
      this->Attributes->InsertNextAttribute(a);
      a->Delete();
      ++i;
    }
    this->Tessellator->Initialize(this);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
// Description:
// Number of points composing the dataset. See NewPointIterator for more
// details.
// \post positive_result: result>=0
vtkIdType vtkBridgeDataSet::GetNumberOfPoints()
{
  vtkIdType result = 0;
  if(this->Implementation)
  {
    result = this->Implementation->GetNumberOfPoints();
  }
  assert("post: positive_result" && result>=0);
  return result;
}

//----------------------------------------------------------------------------
// Description:
// Compute the number of cells for each dimension and the list of types of
// cells.
// \pre implementation_exists: this->Implementation!=0
void vtkBridgeDataSet::ComputeNumberOfCellsAndTypes()
{
  unsigned char type;
  vtkIdType cellId;
  vtkIdType numCells;
  vtkCell *c;

  if ( this->GetMTime() > this->ComputeNumberOfCellsTime ) // cache is obsolete
  {
     numCells = this->GetNumberOfCells();
     this->NumberOf0DCells=0;
     this->NumberOf1DCells=0;
     this->NumberOf2DCells=0;
     this->NumberOf3DCells=0;

     this->Types->Reset();

     if(this->Implementation!=0)
     {
       cellId=0;
       while(cellId<numCells)
       {
         c=this->Implementation->GetCell(cellId);
         switch(c->GetCellDimension())
         {
           case 0:
             this->NumberOf0DCells++;
             break;
           case 1:
             this->NumberOf1DCells++;
             break;
           case 2:
             this->NumberOf2DCells++;
             break;
           case 3:
             this->NumberOf3DCells++;
             break;
         }
         type=c->GetCellType();
         if(!this->Types->IsType(type))
         {
           this->Types->InsertNextType(type);
         }
         cellId++;
       }
     }

     this->ComputeNumberOfCellsTime.Modified(); // cache is up-to-date
     assert("check: positive_dim0" && this->NumberOf0DCells>=0);
     assert("check: valid_dim0" && this->NumberOf0DCells<=numCells);
     assert("check: positive_dim1" && this->NumberOf1DCells>=0);
     assert("check: valid_dim1" && this->NumberOf1DCells<=numCells);
     assert("check: positive_dim2" && this->NumberOf2DCells>=0);
     assert("check: valid_dim2" && this->NumberOf2DCells<=numCells);
     assert("check: positive_dim3" && this->NumberOf3DCells>=0);
     assert("check: valid_dim3" && this->NumberOf3DCells<=numCells);
  }
}

//----------------------------------------------------------------------------
// Description:
// Number of cells that explicitly define the dataset. See NewCellIterator
// for more details.
// \pre valid_dim_range: (dim>=-1) && (dim<=3)
// \post positive_result: result>=0
vtkIdType vtkBridgeDataSet::GetNumberOfCells(int dim)
{
  assert("pre: valid_dim_range" && (dim>=-1) && (dim<=3));

  vtkIdType result=0;
  if(this->Implementation!=0)
  {
    if(dim==-1)
    {
      result=this->Implementation->GetNumberOfCells();
    }
    else
    {
      this->ComputeNumberOfCellsAndTypes();
      switch(dim)
      {
        case 0:
          result=this->NumberOf0DCells;
          break;
        case 1:
          result=this->NumberOf1DCells;
          break;
        case 2:
          result=this->NumberOf2DCells;
          break;
        case 3:
          result=this->NumberOf3DCells;
          break;
      }
    }
  }

  assert("post: positive_result" && result>=0);
  return result;
}

//----------------------------------------------------------------------------
// Description:
// Return -1 if the dataset is explicitly defined by cells of several
// dimensions or if there is no cell. If the dataset is explicitly defined by
// cells of a unique dimension, return this dimension.
// \post valid_range: (result>=-1) && (result<=3)
int vtkBridgeDataSet::GetCellDimension()
{
  int result=0;
  int accu=0;

  this->ComputeNumberOfCellsAndTypes();

  if(this->NumberOf0DCells!=0)
  {
    accu++;
    result=0;
  }
  if(this->NumberOf1DCells!=0)
  {
    accu++;
    result=1;
  }
  if(this->NumberOf2DCells!=0)
  {
    accu++;
    result=2;
  }
  if(this->NumberOf3DCells!=0)
  {
    accu++;
    result=3;
  }
  if(accu!=1) // no cells at all or several dimensions
  {
    result=-1;
  }
  assert("post: valid_range" && (result>=-1) && (result<=3));
  return result;
}

//----------------------------------------------------------------------------
// Description:
// Get a list of types of cells in a dataset. The list consists of an array
// of types (not necessarily in any order), with a single entry per type.
// For example a dataset 5 triangles, 3 lines, and 100 hexahedra would
// result a list of three entries, corresponding to the types VTK_TRIANGLE,
// VTK_LINE, and VTK_HEXAHEDRON.
// THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
// THE DATASET IS NOT MODIFIED
// \pre types_exist: types!=0
void vtkBridgeDataSet::GetCellTypes(vtkCellTypes *types)
{
  assert("pre: types_exist" && types!=0);

  int i;
  int c;
  this->ComputeNumberOfCellsAndTypes();

  // copy from `this->Types' to `types'.
  types->Reset();
  c=this->Types->GetNumberOfTypes();
  i=0;
  while(i<c)
  {
    types->InsertNextType(this->Types->GetCellType(i));
    ++i;
  }
}

//----------------------------------------------------------------------------
// Description:
// Cells of dimension `dim' (or all dimensions if -1) that explicitly define
// the dataset. For instance, it will return only tetrahedra if the mesh is
// defined by tetrahedra. If the mesh is composed of two parts, one with
// tetrahedra and another part with triangles, it will return both, but will
// not return edges and vertices.
// \pre valid_dim_range: (dim>=-1) && (dim<=3)
// \post result_exists: result!=0
vtkGenericCellIterator *vtkBridgeDataSet::NewCellIterator(int dim)
{
  assert("pre: valid_dim_range" && (dim>=-1) && (dim<=3));

  vtkBridgeCellIterator *result=vtkBridgeCellIterator::New();
  result->InitWithDataSet(this,dim); // vtkBridgeCellIteratorOnDataSetCells

  assert("post: result_exists" && result!=0);
  return result;
}

//----------------------------------------------------------------------------
// Description:
// Boundaries of dimension `dim' (or all dimensions if -1) of the dataset.
// If `exteriorOnly' is true, only  the exterior boundaries of the dataset
// will be returned, otherwise it will return exterior and interior
// boundaries.
// \pre valid_dim_range: (dim>=-1) && (dim<=2)
// \post result_exists: result!=0
vtkGenericCellIterator *vtkBridgeDataSet::NewBoundaryIterator(int dim,
                                                       int exteriorOnly)
{
  assert("pre: valid_dim_range" && (dim>=-1) && (dim<=2));

  vtkBridgeCellIterator *result=vtkBridgeCellIterator::New();
  result->InitWithDataSetBoundaries(this,dim,exteriorOnly); //vtkBridgeCellIteratorOnDataSetBoundaries(dim,exterior_only);

  assert("post: result_exists" && result!=0);
  return result;
}

//----------------------------------------------------------------------------
// Description:
// Points composing the dataset; they can be on a vertex or isolated.
// \post result_exists: result!=0
vtkGenericPointIterator *vtkBridgeDataSet::NewPointIterator()
{
  vtkBridgePointIterator *result=vtkBridgePointIterator::New();
  result->InitWithDataSet(this);
  assert("post: result_exists" && result!=0);
  return result;
}

//----------------------------------------------------------------------------
// Description:
// Estimated size needed after tessellation (or special operation)
vtkIdType vtkBridgeDataSet::GetEstimatedSize()
{
  return this->GetNumberOfPoints()*this->GetNumberOfCells();
}

//----------------------------------------------------------------------------
// Description:
// Locate closest cell to position `x' (global coordinates) with respect to
// a tolerance squared `tol2' and an initial guess `cell' (if valid). The
// result consists in the `cell', the `subId' of the sub-cell (0 if primary
// cell), the parametric coordinates `pcoord' of the position. It returns
// whether the position is inside the cell or not. Tolerance is used to
// control how close the point is to be considered "in" the cell.
// THIS METHOD IS NOT THREAD SAFE.
// \pre not_empty: GetNumberOfCells()>0
// \pre cell_exists: cell!=0
// \pre positive_tolerance: tol2>0
// \post clamped_pcoords: result implies (0<=pcoords[0]<=1 && )

int vtkBridgeDataSet::FindCell(double x[3],
                               vtkGenericCellIterator* &cell,
                               double tol2,
                               int &subId,
                               double pcoords[3])
{
  assert("pre: not_empty" && GetNumberOfCells()>0);
  assert("pre: cell_exists" && cell!=0);
  assert("pre: positive_tolerance" && tol2>0);

  vtkIdType cellid;
  vtkBridgeCell *c;
  vtkBridgeCellIterator *it=static_cast<vtkBridgeCellIterator *>(cell);
  vtkCell *c2;

  double *ignoredWeights=new double[this->Implementation->GetMaxCellSize()];

  if(cell->IsAtEnd())
  {
    cellid=this->Implementation->FindCell(x,0,0,tol2,subId,pcoords,
                                          ignoredWeights);
  }
  else
  {
    c=static_cast<vtkBridgeCell *>(cell->GetCell());
    c2=c->Cell; // bridge
    cellid=c->GetId(); // adaptor
    cellid=this->Implementation->FindCell(x,c2,cellid,tol2,subId,pcoords,
                                          ignoredWeights);
  }
  delete [] ignoredWeights;
  if(cellid>=0)
  {
    it->InitWithOneCell(this,cellid); // at end
    it->Begin();
    // clamp:
    int i=0;
    while(i<3)
    {
      if(pcoords[i]<0)
      {
        pcoords[i]=0;
      }
      else if(pcoords[i]>1)
      {
        pcoords[i]=1;
      }
      ++i;
    }
  }

  // A=>B: !A || B
  // result => clamped pcoords
  assert("post: clamped_pcoords" && ((cellid<0)||(pcoords[0]>=0
                                                 && pcoords[0]<=1
                                                 && pcoords[1]>=0
                                                 && pcoords[1]<=1
                                                 && pcoords[2]>=0
                                                 && pcoords[2]<=1)));

  return cellid>=0; // bool
}

//----------------------------------------------------------------------------
// Description:
// Locate closest point `p' to position `x' (global coordinates)
// \pre not_empty: GetNumberOfPoints()>0
// \pre p_exists: p!=0
void vtkBridgeDataSet::FindPoint(double x[3],
                                 vtkGenericPointIterator *p)
{
  assert("pre: not_empty" && GetNumberOfPoints()>0);
  assert("pre: p_exists" && p!=0);
  vtkBridgePointIterator *bp=static_cast<vtkBridgePointIterator *>(p);

  if(this->Implementation!=0)
  {
    vtkIdType pt=this->Implementation->FindPoint(x);
    bp->InitWithOnePoint(this,pt);
  }
  else
  {
    bp->InitWithOnePoint(this,-1);
  }
}

//----------------------------------------------------------------------------
// Description:
// Datasets are composite objects and need to check each part for MTime.
vtkMTimeType vtkBridgeDataSet::GetMTime()
{
  vtkMTimeType result;
  vtkMTimeType mtime;

  result = this->Superclass::GetMTime();

  if(this->Implementation!=0)
  {
    mtime = this->Implementation->GetMTime();
    result = ( mtime > result ? mtime : result );
  }
  return result;
}

//----------------------------------------------------------------------------
// Description:
// Compute the geometry bounding box.
void vtkBridgeDataSet::ComputeBounds()
{
  double *bounds;

  if ( this->GetMTime() > this->ComputeTime )
  {
    if(this->Implementation!=0)
    {
      this->Implementation->ComputeBounds();
      this->ComputeTime.Modified();
      bounds=this->Implementation->GetBounds();
      memcpy(this->Bounds,bounds,sizeof(double)*6);
    }
    else
    {
      vtkMath::UninitializeBounds(this->Bounds);
    }
    this->ComputeTime.Modified();
  }
}
