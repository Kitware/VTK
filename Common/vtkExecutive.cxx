/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExecutive.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExecutive.h"

#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkGarbageCollector.h"

vtkCxxRevisionMacro(vtkExecutive, "1.6");

//----------------------------------------------------------------------------
vtkExecutive::vtkExecutive()
{
  this->GarbageCollecting = 0;
}

//----------------------------------------------------------------------------
vtkExecutive::~vtkExecutive()
{
}

//----------------------------------------------------------------------------
void vtkExecutive::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkExecutive::UnRegister(vtkObjectBase* o)
{
  int check = (this->GetReferenceCount() > 1);
  this->Superclass::UnRegister(o);
  if(check && !this->GarbageCollecting)
    {
    vtkGarbageCollector::Check(this);
    }
}

//----------------------------------------------------------------------------
void vtkExecutive::GarbageCollectionStarting()
{
  this->GarbageCollecting = 1;
  this->Superclass::GarbageCollectionStarting();
}

//----------------------------------------------------------------------------
void vtkExecutive::SetOutputDataInternal(vtkAlgorithm* algorithm, int port,
                                         vtkDataObject* output)
{
  if(vtkInformation* info = this->GetOutputInformation(algorithm, port))
    {
    info->Set(vtkDataObject::DATA_OBJECT(), output);
    }
}

//----------------------------------------------------------------------------
vtkDataObject* vtkExecutive::GetOutputDataInternal(vtkAlgorithm* algorithm,
                                                   int port)
{
  if(vtkInformation* info = this->GetOutputInformation(algorithm, port))
    {
    return info->Get(vtkDataObject::DATA_OBJECT());
    }
  return 0;
}
