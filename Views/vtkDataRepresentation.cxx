/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataRepresentation.cxx

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

#include "vtkDataRepresentation.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkSelectionLink.h"

vtkCxxRevisionMacro(vtkDataRepresentation, "1.1");
vtkStandardNewMacro(vtkDataRepresentation);
vtkCxxSetObjectMacro(vtkDataRepresentation, InputConnection, vtkAlgorithmOutput);
vtkCxxSetObjectMacro(vtkDataRepresentation, SelectionLink, vtkSelectionLink);
//----------------------------------------------------------------------------
vtkDataRepresentation::vtkDataRepresentation()
{
  this->InputConnection = 0;
  this->SelectionLink = vtkSelectionLink::New();
}

//----------------------------------------------------------------------------
vtkDataRepresentation::~vtkDataRepresentation()
{
  this->SetInputConnection(0);
  this->SetSelectionLink(0);
}

//----------------------------------------------------------------------------
void vtkDataRepresentation::SetInput(vtkDataObject* input)
{
  if (!input)
    {
    vtkErrorMacro("Input cannot be NULL");
    return;
    }
  this->SetInputConnection(input->GetProducerPort());
}

//----------------------------------------------------------------------------
void vtkDataRepresentation::Select(
  vtkView* view, vtkSelection* selection)
{
  vtkSelection* converted = this->ConvertSelection(view, selection);
  if (converted)
    {
    this->UpdateSelection(converted);
    converted->Delete();
    }
}

//----------------------------------------------------------------------------
vtkSelection* vtkDataRepresentation::ConvertSelection(
  vtkView* vtkNotUsed(view), vtkSelection* selection)
{
  vtkSelection* converted = vtkSelection::New();
  converted->ShallowCopy(selection);
  return converted;
}

//----------------------------------------------------------------------------
void vtkDataRepresentation::UpdateSelection(vtkSelection* selection)
{
  this->SelectionLink->SetSelection(selection);
  this->InvokeEvent(vtkCommand::SelectionChangedEvent, reinterpret_cast<void*>(selection));
}

//----------------------------------------------------------------------------
void vtkDataRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InputConnection: " << (this->InputConnection ? "" : "(null)") << endl;
  if (this->InputConnection)
    {
    this->InputConnection->PrintSelf(os, indent.GetNextIndent());
    }
  os << indent << "SelectionLink: " << (this->SelectionLink ? "" : "(null)") << endl;
  if (this->SelectionLink)
    {
    this->SelectionLink->PrintSelf(os, indent.GetNextIndent());
    }  
}
