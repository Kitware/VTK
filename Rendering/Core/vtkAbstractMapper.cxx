/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAbstractMapper.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPlaneCollection.h"
#include "vtkPlanes.h"
#include "vtkPointData.h"
#include "vtkTimerLog.h"


vtkCxxSetObjectMacro(vtkAbstractMapper,ClippingPlanes,vtkPlaneCollection);


// Construct object.
vtkAbstractMapper::vtkAbstractMapper()
{
  this->TimeToDraw = 0.0;
  this->LastWindow = NULL;
  this->ClippingPlanes = NULL;
  this->Timer = vtkTimerLog::New();
  this->SetNumberOfOutputPorts(0);
  this->SetNumberOfInputPorts(1);
}

vtkAbstractMapper::~vtkAbstractMapper()
{
  this->Timer->Delete();
  if (this->ClippingPlanes)
    {
    this->ClippingPlanes->UnRegister(this);
    }
}

// Description:
// Override Modifiedtime as we have added Clipping planes
unsigned long vtkAbstractMapper::GetMTime()
{
  unsigned long mTime = this->Superclass::GetMTime();
  unsigned long clipMTime;

  if ( this->ClippingPlanes != NULL )
    {
    clipMTime = this->ClippingPlanes->GetMTime();
    mTime = ( clipMTime > mTime ? clipMTime : mTime );
    }

  return mTime;
}

void vtkAbstractMapper::AddClippingPlane(vtkPlane *plane)
{
  if (this->ClippingPlanes == NULL)
    {
    this->ClippingPlanes = vtkPlaneCollection::New();
    this->ClippingPlanes->Register(this);
    this->ClippingPlanes->Delete();
    }

  this->ClippingPlanes->AddItem(plane);
  this->Modified();
}

void vtkAbstractMapper::RemoveClippingPlane(vtkPlane *plane)
{
  if (this->ClippingPlanes == NULL)
    {
    vtkErrorMacro(<< "Cannot remove clipping plane: mapper has none");
    }
  this->ClippingPlanes->RemoveItem(plane);
  this->Modified();
}

void vtkAbstractMapper::RemoveAllClippingPlanes()
{
  if ( this->ClippingPlanes )
    {
    this->ClippingPlanes->RemoveAllItems();
    }
}

void vtkAbstractMapper::SetClippingPlanes(vtkPlanes *planes)
{
  vtkPlane *plane;
  if (!planes)
    {
    return;
    }

  int numPlanes = planes->GetNumberOfPlanes();

  this->RemoveAllClippingPlanes();
  for (int i=0; i<numPlanes && i<6; i++)
    {
    plane = vtkPlane::New();
    planes->GetPlane(i, plane);
    this->AddClippingPlane(plane);
    plane->Delete();
    }
}

vtkDataArray *vtkAbstractMapper::GetScalars(vtkDataSet *input,
                                            int scalarMode,
                                            int arrayAccessMode,
                                            int arrayId,
                                            const char *arrayName,
                                            int& cellFlag)
{
  vtkDataArray *scalars=NULL;
  vtkPointData *pd;
  vtkCellData *cd;
  vtkFieldData *fd;

  // make sure we have an input
  if ( !input )
    {
    return NULL;
    }

  // get and scalar data according to scalar mode
  if ( scalarMode == VTK_SCALAR_MODE_DEFAULT )
    {
    scalars = input->GetPointData()->GetScalars();
    cellFlag = 0;
    if (!scalars)
      {
      scalars = input->GetCellData()->GetScalars();
      cellFlag = 1;
      }
    }
  else if ( scalarMode == VTK_SCALAR_MODE_USE_POINT_DATA )
    {
    scalars = input->GetPointData()->GetScalars();
    cellFlag = 0;
    }
  else if ( scalarMode == VTK_SCALAR_MODE_USE_CELL_DATA )
    {
    scalars = input->GetCellData()->GetScalars();
    cellFlag = 1;
    }
  else if ( scalarMode == VTK_SCALAR_MODE_USE_POINT_FIELD_DATA )
    {
    pd = input->GetPointData();
    if (arrayAccessMode == VTK_GET_ARRAY_BY_ID)
      {
      scalars = pd->GetArray(arrayId);
      }
    else
      {
      scalars = pd->GetArray(arrayName);
      }
    cellFlag = 0;
    }
  else if ( scalarMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA )
    {
    cd = input->GetCellData();
    if (arrayAccessMode == VTK_GET_ARRAY_BY_ID)
      {
      scalars = cd->GetArray(arrayId);
      }
    else
      {
      scalars = cd->GetArray(arrayName);
      }
    cellFlag = 1;
    }
  else if ( scalarMode == VTK_SCALAR_MODE_USE_FIELD_DATA )
    {
    fd = input->GetFieldData();
    if (arrayAccessMode == VTK_GET_ARRAY_BY_ID)
      {
      scalars = fd->GetArray(arrayId);
      }
    else
      {
      scalars = fd->GetArray(arrayName);
      }
    cellFlag = 2;
    }

  return scalars;
}


// Shallow copy of vtkProp.
void vtkAbstractMapper::ShallowCopy(vtkAbstractMapper *mapper)
{
  this->SetClippingPlanes( mapper->GetClippingPlanes() );
}

void vtkAbstractMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "TimeToDraw: " << this->TimeToDraw << "\n";

  if ( this->ClippingPlanes )
    {
    os << indent << "ClippingPlanes:\n";
    this->ClippingPlanes->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "ClippingPlanes: (none)\n";
    }
}


