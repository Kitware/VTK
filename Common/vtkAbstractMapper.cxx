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
#include "vtkPlaneCollection.h"
#include "vtkTimerLog.h"

vtkCxxRevisionMacro(vtkAbstractMapper, "1.26");

vtkCxxSetObjectMacro(vtkAbstractMapper,ClippingPlanes,vtkPlaneCollection);

// Construct object.
vtkAbstractMapper::vtkAbstractMapper()
{
  this->TimeToDraw = 0.0;
  this->LastWindow = NULL;
  this->ClippingPlanes = NULL;
  this->Timer = vtkTimerLog::New();
  this->NumberOfConsumers = 0;
  this->Consumers = 0;
}

vtkAbstractMapper::~vtkAbstractMapper()
{
  this->Timer->Delete();
  if (this->ClippingPlanes)
    {
    this->ClippingPlanes->UnRegister(this);
    }
  if (this->Consumers)
    {
    delete [] this->Consumers;
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

  os << indent << "NumberOfConsumers: " << this->NumberOfConsumers << endl;
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


void vtkAbstractMapper::AddConsumer(vtkObject *c)
{
  // make sure it isn't already there
  if (this->IsConsumer(c))
    {
    return;
    }
  // add it to the list, reallocate memory
  vtkObject **tmp = this->Consumers;
  this->NumberOfConsumers++;
  this->Consumers = new vtkObject* [this->NumberOfConsumers];
  for (int i = 0; i < (this->NumberOfConsumers-1); i++)
    {
    this->Consumers[i] = tmp[i];
    }
  this->Consumers[this->NumberOfConsumers-1] = c;
  // free old memory
  delete [] tmp;
}

void vtkAbstractMapper::RemoveConsumer(vtkObject *c)
{
  // make sure it is already there
  if (!this->IsConsumer(c))
    {
    return;
    }
  // remove it from the list, reallocate memory
  vtkObject **tmp = this->Consumers;
  this->NumberOfConsumers--;
  this->Consumers = new vtkObject* [this->NumberOfConsumers];
  int cnt = 0;
  int i;
  for (i = 0; i <= this->NumberOfConsumers; i++)
    {
    if (tmp[i] != c)
      {
      this->Consumers[cnt] = tmp[i];
      cnt++;
      }
    }
  // free old memory
  delete [] tmp;
}

int vtkAbstractMapper::IsConsumer(vtkObject *c)
{
  int i;
  for (i = 0; i < this->NumberOfConsumers; i++)
    {
    if (this->Consumers[i] == c)
      {
      return 1;
      }
    }
  return 0;
}

vtkObject *vtkAbstractMapper::GetConsumer(int i)
{
  if (i >= this->NumberOfConsumers)
    {
    return 0;
    }
  return this->Consumers[i];
}
