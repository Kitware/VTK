/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericDataSet.h"

#include "vtkCellTypes.h"
#include "vtkGenericAdaptorCell.h"
#include "vtkGenericAttributeCollection.h"
#include "vtkGenericCellIterator.h"
#include "vtkGenericCellTessellator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"

#include <cassert>

vtkCxxSetObjectMacro(vtkGenericDataSet,Tessellator,vtkGenericCellTessellator);

//----------------------------------------------------------------------------
vtkGenericDataSet::vtkGenericDataSet()
{
  this->Tessellator = 0;
  this->Attributes = vtkGenericAttributeCollection::New();
  vtkMath::UninitializeBounds(this->Bounds);
}

//----------------------------------------------------------------------------
vtkGenericDataSet::~vtkGenericDataSet()
{
  if(this->Tessellator!=0)
    {
    this->Tessellator->Delete();
    }
  this->Attributes->Delete();
}

//----------------------------------------------------------------------------
void vtkGenericDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number Of Points: " << this->GetNumberOfPoints() << "\n";
  os << indent << "Number Of Cells: " << this->GetNumberOfCells() << "\n";

  os << indent << "Attributes:\n";
  this->GetAttributes()->PrintSelf(os,indent.GetNextIndent());

  this->ComputeBounds();
  os << indent << "Bounds: \n";
  os << indent << "  Xmin,Xmax: (" <<this->Bounds[0] << ", " << this->Bounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" <<this->Bounds[2] << ", " << this->Bounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" <<this->Bounds[4] << ", " << this->Bounds[5] << ")\n";

  os << indent << "Tessellator:" << this->Tessellator << endl;
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
void vtkGenericDataSet::GetCellTypes(vtkCellTypes *types)
{
  assert("pre: types_exist" && types!=0);

  unsigned char type;
  vtkGenericCellIterator *it = this->NewCellIterator(-1);
  vtkGenericAdaptorCell *c = it->NewCell();

  types->Reset();
  it->Begin();
  while(!it->IsAtEnd())
    {
    it->GetCell(c);
    type=c->GetType();
    if ( ! types->IsType(type) )
      {
      types->InsertNextType(type);
      }
    it->Next();
    }
  c->Delete();
  it->Delete();
}

//----------------------------------------------------------------------------
// Return a pointer to the geometry bounding box in the form
// (xmin,xmax, ymin,ymax, zmin,zmax).
// The return value is VOLATILE.
// \post result_exists: result!=0
double *vtkGenericDataSet::GetBounds()
{
  this->ComputeBounds();
  return this->Bounds;
}

//----------------------------------------------------------------------------
 // Description:
// Return the geometry bounding box in global coordinates in
// the form (xmin,xmax, ymin,ymax, zmin,zmax) into `bounds'.
void vtkGenericDataSet::GetBounds(double bounds[6])
{
  this->ComputeBounds();
  memcpy(bounds,this->Bounds,sizeof(double)*6);
}

//----------------------------------------------------------------------------
// Description:
// Get the center of the bounding box in global coordinates.
// The return value is VOLATILE.
// - \post result_exists: result!=0
double *vtkGenericDataSet::GetCenter()
{
  this->ComputeBounds();
  for (int i=0; i<3; i++)
    {
    this->Center[i] = (this->Bounds[2*i+1] + this->Bounds[2*i]) * 0.5;
    }
  return this->Center;
}

//----------------------------------------------------------------------------
// Description:
// Get the center of the bounding box in global coordinates.
void vtkGenericDataSet::GetCenter(double center[3])
{
  this->ComputeBounds();
  for (int i=0; i<3; i++)
    {
    center[i] = (this->Bounds[2*i+1] + this->Bounds[2*i]) * 0.5;
    }
}

//----------------------------------------------------------------------------
// Description:
// Length of the diagonal of the bounding box.
double vtkGenericDataSet::GetLength()
{
  double result, l=0.0;
  int i;

  this->ComputeBounds();
  for (i=0; i<3; i++)
    {
    result = this->Bounds[2*i+1] - this->Bounds[2*i];
    l += result * result;
    }
  result = sqrt(l);
  assert("post: positive_result" && result>=0);
  return result;
}

//----------------------------------------------------------------------------
unsigned long int vtkGenericDataSet::GetMTime()
{
  unsigned long result;
  unsigned long mtime;

  result = this->Superclass::GetMTime();

  mtime = this->Attributes->GetMTime();
  result = ( mtime > result ? mtime : result );

  if(this->Tessellator)
    {
    mtime = this->Tessellator->GetMTime();
    result = ( mtime > result ? mtime : result );
    }

  return result;
}

//----------------------------------------------------------------------------
// Description:
// Actual size of the data in kilobytes; only valid after the pipeline has
// updated. It is guaranteed to be greater than or equal to the memory
// required to represent the data.
unsigned long vtkGenericDataSet::GetActualMemorySize()
{
  unsigned long result = this->Superclass::GetActualMemorySize();
  result += this->Attributes->GetActualMemorySize();
  return result;
}

//----------------------------------------------------------------------------
// Description:
// Return the type of data object.
int vtkGenericDataSet::GetDataObjectType()
{
  return VTK_GENERIC_DATA_SET;
}

//----------------------------------------------------------------------------
vtkGenericDataSet* vtkGenericDataSet::GetData(vtkInformation* info)
{
  return info ? vtkGenericDataSet::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkGenericDataSet* vtkGenericDataSet::GetData(vtkInformationVector* v, int i)
{
  return vtkGenericDataSet::GetData(v->GetInformationObject(i));
}
