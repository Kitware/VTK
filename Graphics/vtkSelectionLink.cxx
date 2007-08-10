/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSelectionLink.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSelectionLink.h"

#include "vtkCommand.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"

vtkCxxRevisionMacro(vtkSelectionLink, "1.1");
vtkStandardNewMacro(vtkSelectionLink);
//----------------------------------------------------------------------------
vtkSelectionLink::vtkSelectionLink()
{
  this->SetNumberOfInputPorts(0);
  
  // Start with an empty index selection
  this->Selection = vtkSelection::New();
  this->Selection->SetContentType(vtkSelection::INDICES);
  vtkIdTypeArray* ids = vtkIdTypeArray::New();
  this->Selection->SetSelectionList(ids);
  ids->Delete();
}

//----------------------------------------------------------------------------
vtkSelectionLink::~vtkSelectionLink()
{
  this->Selection->Delete();
}

//----------------------------------------------------------------------------
void vtkSelectionLink::SetSelection(vtkSelection* selection)
{
  if (!selection)
    {
    vtkErrorMacro("Cannot set a null selection.");
    return;
    }
  this->Selection->ShallowCopy(selection);
  this->Modified();
  this->InvokeEvent(vtkCommand::SelectionChangedEvent);
}

//----------------------------------------------------------------------------
int vtkSelectionLink::RequestData(
  vtkInformation *vtkNotUsed(info),
  vtkInformationVector **vtkNotUsed(inVector),
  vtkInformationVector *outVector)
{
  this->InvokeEvent(vtkCommand::StartEvent);
  
  vtkInformation *outInfo = outVector->GetInformationObject(0);
  vtkSelection* output = vtkSelection::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  if (this->Selection)
    {
    output->ShallowCopy(this->Selection);
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkSelectionLink::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Selection: " << (this->Selection ? "" : "null") << endl;
  if (this->Selection)
    {
    this->Selection->PrintSelf(os, indent.GetNextIndent());
    }
}

