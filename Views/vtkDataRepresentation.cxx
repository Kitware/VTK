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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkDataRepresentation.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCommand.h"
#include "vtkConvertSelectionDomain.h"
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSelectionLink.h"
#include "vtkTable.h"

vtkCxxRevisionMacro(vtkDataRepresentation, "1.4");
vtkStandardNewMacro(vtkDataRepresentation);
vtkCxxSetObjectMacro(vtkDataRepresentation, InputConnectionInternal, vtkAlgorithmOutput);
vtkCxxSetObjectMacro(vtkDataRepresentation, SelectionLinkInternal, vtkSelectionLink);
//----------------------------------------------------------------------------
vtkDataRepresentation::vtkDataRepresentation()
{
  this->Selectable = true;
  this->InputConnectionInternal = 0;
  this->SelectionLinkInternal = vtkSelectionLink::New();
  this->ConvertDomain = vtkConvertSelectionDomain::New();

  this->ConvertDomain->SetInputConnection(0, this->SelectionLinkInternal->GetOutputPort(0));
  this->ConvertDomain->SetInputConnection(1, this->SelectionLinkInternal->GetOutputPort(1));
}

//----------------------------------------------------------------------------
vtkDataRepresentation::~vtkDataRepresentation()
{
  this->SetInputConnectionInternal(0);
  this->SetSelectionLinkInternal(0);
  this->ConvertDomain->Delete();
}

//----------------------------------------------------------------------------
void vtkDataRepresentation::SetSelectionLink(vtkSelectionLink* link)
{
  this->SetSelectionLinkInternal(link);
  if (this->SelectionLinkInternal)
    {
    this->ConvertDomain->SetInputConnection(0, this->SelectionLinkInternal->GetOutputPort(0));
    this->ConvertDomain->SetInputConnection(1, this->SelectionLinkInternal->GetOutputPort(1));
    }
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
void vtkDataRepresentation::SetInputConnection(vtkAlgorithmOutput* conn)
{
  this->SetInputConnectionInternal(conn);
  if (this->InputConnectionInternal)
    {
    this->ConvertDomain->SetInputConnection(2, this->InputConnectionInternal);
    }
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkDataRepresentation::GetSelectionConnection()
{
  return this->ConvertDomain->GetOutputPort();
}

//----------------------------------------------------------------------------
void vtkDataRepresentation::Select(
  vtkView* view, vtkSelection* selection)
{
  if (this->Selectable)
    {
    vtkSelection* converted = this->ConvertSelection(view, selection);
    if (converted)
      {
      this->UpdateSelection(converted);
      converted->Delete();
      }
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
  this->SelectionLinkInternal->SetSelection(selection);
  this->InvokeEvent(vtkCommand::SelectionChangedEvent, reinterpret_cast<void*>(selection));
}

//----------------------------------------------------------------------------
void vtkDataRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InputConnection: " << (this->InputConnectionInternal ? "" : "(null)") << endl;
  if (this->InputConnectionInternal)
    {
    this->InputConnectionInternal->PrintSelf(os, indent.GetNextIndent());
    }
  os << indent << "SelectionLink: " << (this->SelectionLinkInternal ? "" : "(null)") << endl;
  if (this->SelectionLinkInternal)
    {
    this->SelectionLinkInternal->PrintSelf(os, indent.GetNextIndent());
    }  
  os << indent << "Selectable: " << this->Selectable << endl;
}
