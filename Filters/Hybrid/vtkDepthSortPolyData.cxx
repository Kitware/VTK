/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDepthSortPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDepthSortPolyData.h"

#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkGenericCell.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProp3D.h"
#include "vtkTransform.h"
#include "vtkUnsignedIntArray.h"

vtkStandardNewMacro(vtkDepthSortPolyData);

vtkCxxSetObjectMacro(vtkDepthSortPolyData,Camera,vtkCamera);

vtkDepthSortPolyData::vtkDepthSortPolyData()
{
  this->Camera = NULL;
  this->Prop3D = NULL;
  this->Direction = VTK_DIRECTION_BACK_TO_FRONT;
  this->DepthSortMode = VTK_SORT_FIRST_POINT;
  this->Vector[0] = this->Vector[1] = 0.0;
  this->Vector[2] = 0.0;
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
  this->Transform = vtkTransform::New();
  this->SortScalars = 0;
}

vtkDepthSortPolyData::~vtkDepthSortPolyData()
{
  this->Transform->Delete();

  if ( this->Camera )
    {
    this->Camera->Delete();
    }

  //Note: vtkProp3D is not deleted to avoid reference count cycle
}

// Don't reference count to avoid nasty cycle
void vtkDepthSortPolyData::SetProp3D(vtkProp3D *prop3d)
{
  if ( this->Prop3D != prop3d )
    {
    this->Prop3D = prop3d;
    this->Modified();
    }
}

vtkProp3D *vtkDepthSortPolyData::GetProp3D()
{
  return this->Prop3D;
}

typedef struct _vtkSortValues {
  double z;
  vtkIdType cellId;
} vtkSortValues;

extern "C"
{
  int vtkCompareBackToFront(const void *val1, const void *val2)
  {
    if (((vtkSortValues *)val1)->z > ((vtkSortValues *)val2)->z)
      {
      return (-1);
      }
    else if (((vtkSortValues *)val1)->z < ((vtkSortValues *)val2)->z)
      {
      return (1);
      }
    else
      {
      return (0);
      }
  }
}

extern "C"
{
  int vtkCompareFrontToBack(const void *val1, const void *val2)
  {
    if (((vtkSortValues *)val1)->z < ((vtkSortValues *)val2)->z)
      {
      return (-1);
      }
    else if (((vtkSortValues *)val1)->z > ((vtkSortValues *)val2)->z)
      {
      return (1);
      }
    else
      {
      return (0);
      }
  }
}

int vtkDepthSortPolyData::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkSortValues *depth;
  vtkIdType cellId, id;
  vtkGenericCell *cell;
  vtkIdType numCells=input->GetNumberOfCells();
  vtkCellData *inCD=input->GetCellData();
  vtkCellData *outCD=output->GetCellData();
  vtkUnsignedIntArray *sortScalars = NULL;
  unsigned int *scalars = NULL;
  double x[3];
  double p[3], *bounds, *w = NULL, xf[3];
  double vector[3];
  double origin[3];
  int type, npts, subId;
  vtkIdType newId;
  vtkIdType *pts;

  // Initialize
  //
  vtkDebugMacro(<<"Sorting polygonal data");

  // Compute the sort vector
  if ( this->Direction == VTK_DIRECTION_SPECIFIED_VECTOR )
    {
    for (int i=0; i<3; i++)
      {
      vector[i] = this->Vector[i];
      origin[i] = this->Origin[i];
      }
    }
  else //compute view vector
    {
    if ( this->Camera == NULL)
      {
      vtkErrorMacro(<<"Need a camera to sort");
      return 0;
      }

    this->ComputeProjectionVector(vector, origin);
    }
  cell=vtkGenericCell::New();

  if ( this->DepthSortMode == VTK_SORT_PARAMETRIC_CENTER )
    {
    w = new double [input->GetMaxCellSize()];
    }

  // Create temporary input
  vtkPolyData *tmpInput = vtkPolyData::New();
  tmpInput->CopyStructure(input);

  // Compute the depth value
  depth = new vtkSortValues [numCells];
  for ( cellId=0; cellId < numCells; cellId++ )
    {
    tmpInput->GetCell(cellId, cell);
    if ( this->DepthSortMode == VTK_SORT_FIRST_POINT )
      {
      cell->Points->GetPoint(0,x);
      }
    else if ( this->DepthSortMode == VTK_SORT_BOUNDS_CENTER )
      {
      bounds = cell->GetBounds();
      x[0] = (bounds[0]+bounds[1])/2.0;
      x[1] = (bounds[2]+bounds[3])/2.0;
      x[2] = (bounds[4]+bounds[5])/2.0;
      }
    else // VTK_SORT_PARAMETRIC_CENTER )
      {
      subId = cell->GetParametricCenter(p);
      cell->EvaluateLocation(subId, p, xf, w);
      x[0] = xf[0];
      x[1] = xf[1];
      x[2] = xf[2];
      }
    x[0] -= origin[0];
    x[1] -= origin[1];
    x[2] -= origin[2];
    depth[cellId].z = vtkMath::Dot(x,vector);
    depth[cellId].cellId = cellId;
    }
  if ( this->DepthSortMode == VTK_SORT_PARAMETRIC_CENTER )
    {
    delete [] w;
    }
  this->UpdateProgress(0.20);

  // Sort the depths
  if ( this->Direction == VTK_DIRECTION_FRONT_TO_BACK )
    {
    qsort((void *)depth, numCells, sizeof(vtkSortValues),
          vtkCompareFrontToBack);
    }
  else
    {
    qsort((void *)depth, numCells, sizeof(vtkSortValues),
          vtkCompareBackToFront);
    }
  this->UpdateProgress(0.60);

  // Generate sorted output
  if ( this->SortScalars )
    {
    sortScalars = vtkUnsignedIntArray::New();
    sortScalars->SetNumberOfTuples(numCells);
    scalars = sortScalars->GetPointer(0);
    }
  outCD->CopyAllocate(inCD);
  output->Allocate(tmpInput,numCells);
  for ( cellId=0; cellId < numCells; cellId++ )
    {
    id = depth[cellId].cellId;
    tmpInput->GetCell(id, cell);
    type = cell->GetCellType();
    npts = cell->GetNumberOfPoints();
    pts = cell->GetPointIds()->GetPointer(0);

    // copy cell data
    newId = output->InsertNextCell(type, npts, pts);
    outCD->CopyData(inCD, id, newId);
    if ( this->SortScalars )
      {
      scalars[newId] = newId;
      }
    }
  this->UpdateProgress(0.90);

  // Points are left alone
  output->SetPoints(input->GetPoints());
  output->GetPointData()->PassData(input->GetPointData());
  if ( this->SortScalars )
    {
    int idx = output->GetCellData()->AddArray(sortScalars);
    output->GetCellData()->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    sortScalars->Delete();
    }

  // Clean up and get out
  tmpInput->Delete();
  delete [] depth;
  cell->Delete();
  output->Squeeze();

  return 1;
}

void vtkDepthSortPolyData::ComputeProjectionVector(double vector[3],
                                                   double origin[3])
{
  double *focalPoint = this->Camera->GetFocalPoint();
  double *position = this->Camera->GetPosition();

  // If a camera is present, use it
  if ( !this->Prop3D )
    {
    for(int i=0; i<3; i++)
      {
      vector[i] = focalPoint[i] - position[i];
      origin[i] = position[i];
      }
    }

  else  //Otherwise, use Prop3D
    {
    double focalPt[4], pos[4];
    int i;

    this->Transform->SetMatrix(this->Prop3D->GetMatrix());
    this->Transform->Push();
    this->Transform->Inverse();

    for(i=0; i<4; i++)
      {
      focalPt[i] = focalPoint[i];
      pos[i] = position[i];
      }

    this->Transform->TransformPoint(focalPt,focalPt);
    this->Transform->TransformPoint(pos,pos);

    for (i=0; i<3; i++)
      {
      vector[i] = focalPt[i] - pos[i];
      origin[i] = pos[i];
      }
    this->Transform->Pop();
  }
}

unsigned long int vtkDepthSortPolyData::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();

  if ( this->Direction != VTK_DIRECTION_SPECIFIED_VECTOR )
    {
    unsigned long time;
    if ( this->Camera != NULL )
      {
      time = this->Camera->GetMTime();
      mTime = ( time > mTime ? time : mTime );
      }

    if ( this->Prop3D != NULL )
      {
      time = this->Prop3D->GetMTime();
      mTime = ( time > mTime ? time : mTime );
      }
    }

  return mTime;
}

void vtkDepthSortPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->Camera )
    {
    os << indent << "Camera:\n";
    this->Camera->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Camera: (none)\n";
    }

  if ( this->Prop3D )
    {
    os << indent << "Prop3D:\n";
    this->Prop3D->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Prop3D: (none)\n";
    }

  os << indent << "Direction: ";
  if ( this->Direction == VTK_DIRECTION_BACK_TO_FRONT )
    {
    os << "Back To Front" << endl;
    }
  else if ( this->Direction == VTK_DIRECTION_FRONT_TO_BACK )
    {
    os << "Front To Back";
    }
  else
    {
    os << "Specified Direction: ";
    os << "(" << this->Vector[0] << ", " << this->Vector[1] << ", "
       << this->Vector[2] << ")\n";
    os << "Specified Origin: ";
    os << "(" << this->Origin[0] << ", " << this->Origin[1] << ", "
       << this->Origin[2] << ")\n";
    }

  os << indent << "Depth Sort Mode: ";
  if ( this->DepthSortMode == VTK_SORT_FIRST_POINT )
    {
    os << "First Point" << endl;
    }
  else if ( this->DepthSortMode == VTK_SORT_BOUNDS_CENTER )
    {
    os << "Bounding Box Center" << endl;
    }
  else
    {
    os << "Paramteric Center" << endl;
    }

  os << indent << "Sort Scalars: " << (this->SortScalars ? "On\n" : "Off\n");
}
