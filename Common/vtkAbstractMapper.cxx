/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractMapper.cxx
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
#include "vtkAbstractMapper.h"
#include "vtkDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPlanes.h"

vtkCxxRevisionMacro(vtkAbstractMapper, "1.24");

// Construct object.
vtkAbstractMapper::vtkAbstractMapper()
{
  this->TimeToDraw = 0.0;
  this->LastWindow = NULL;
  this->ClippingPlanes = NULL;
  this->Timer = vtkTimerLog::New();
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
  unsigned long mTime = vtkProcessObject::GetMTime();
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
}

void vtkAbstractMapper::RemoveClippingPlane(vtkPlane *plane)
{
  if (this->ClippingPlanes == NULL)
    {
    vtkErrorMacro(<< "Cannot remove clipping plane: mapper has none");
    }
  this->ClippingPlanes->RemoveItem(plane);
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
    plane = planes->GetPlane(i);
    this->AddClippingPlane(plane);
    }
}

vtkDataArray *vtkAbstractMapper::GetScalars(vtkDataSet *input,
                                            int scalarMode,
                                                                                        int arrayAccessMode,
                                            int arrayId, 
                                            const char *arrayName,
                                            int& offset)
{
  vtkDataArray *scalars=NULL;
  vtkPointData *pd;
  vtkCellData *cd;
  
  // make sure we have an input
  if ( !input )
    {
    return NULL;
    }
    
  // get and scalar data according to scalar mode
  if ( scalarMode == VTK_SCALAR_MODE_DEFAULT )
    {
    scalars = input->GetPointData()->GetScalars();
    if (!scalars)
      {
      scalars = input->GetCellData()->GetScalars();
      }
    }
  else if ( scalarMode == VTK_SCALAR_MODE_USE_POINT_DATA )
    {
    scalars = input->GetPointData()->GetScalars();
    }
  else if ( scalarMode == VTK_SCALAR_MODE_USE_CELL_DATA )
    {
    scalars = input->GetCellData()->GetScalars();
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
    
    if ( !scalars ||
         !(offset < scalars->GetNumberOfComponents()) )
      {
      offset=0;
      //vtkGenericWarningMacro(<<"Data array (used for coloring) not found");
      }
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

    if ( !scalars ||
         !(offset < scalars->GetNumberOfComponents()) )
      {
      offset=0;
      //vtkGenericWarningMacro(<<"Data array (used for coloring) not found");
      }
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

