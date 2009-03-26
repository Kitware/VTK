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

#include "vtkAnnotationLink.h"
#include "vtkCommand.h"
#include "vtkConvertSelectionDomain.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkSelectionLink.h"
#include "vtkSmartPointer.h"

#include <vtkstd/vector>

//---------------------------------------------------------------------------
// vtkDataRepresentationInput
//---------------------------------------------------------------------------

class vtkDataRepresentationInput : public vtkObject
{
public:
  static vtkDataRepresentationInput* New();
  vtkTypeRevisionMacro(vtkDataRepresentationInput, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent)
    {
    this->Superclass::PrintSelf(os, indent);
    }

  vtkDataRepresentationInput()
    {
    this->ConvertDomain = vtkSmartPointer<vtkConvertSelectionDomain>::New();
    }

  ~vtkDataRepresentationInput()
    {
    }

  void SetInput(vtkDataObject* input, vtkDataRepresentation* rep)
    {
    this->ConvertDomain->SetInputConnection(0,
      rep->GetSelectionLink()->GetOutputPort(0));
    this->ConvertDomain->SetInputConnection(1,
      rep->GetSelectionLink()->GetOutputPort(1));
    this->ConvertDomain->SetInput(2, input);
    }

  vtkAlgorithmOutput* GetSelectionConnection()
    {
    return this->ConvertDomain->GetOutputPort();
    }

  vtkDataObject* GetInput()
    {
    return this->ConvertDomain->GetInputDataObject(2, 0);
    }

protected:
  vtkSmartPointer<vtkConvertSelectionDomain> ConvertDomain;
};
vtkCxxRevisionMacro(vtkDataRepresentationInput, "1.6");
vtkStandardNewMacro(vtkDataRepresentationInput);

//---------------------------------------------------------------------------
// vtkDataRepresentation::Internals
//---------------------------------------------------------------------------

class vtkDataRepresentation::Internals {
public:
  vtkstd::vector<vtkstd::vector<
    vtkSmartPointer<vtkDataRepresentationInput> > > Inputs;
};

//----------------------------------------------------------------------------
// vtkDataRepresentation
//----------------------------------------------------------------------------

vtkCxxRevisionMacro(vtkDataRepresentation, "1.6");
vtkStandardNewMacro(vtkDataRepresentation);
vtkCxxSetObjectMacro(vtkDataRepresentation,
  SelectionLinkInternal, vtkSelectionLink);
vtkCxxSetObjectMacro(vtkDataRepresentation,
  AnnotationLinkInternal, vtkAnnotationLink);

//----------------------------------------------------------------------------
vtkDataRepresentation::vtkDataRepresentation() :
  Implementation(new Internals())
{
  this->Selectable = true;
  this->SelectionLinkInternal = vtkSelectionLink::New();
  this->AnnotationLinkInternal = vtkAnnotationLink::New();
  this->SetNumberOfOutputPorts(0);
}

//----------------------------------------------------------------------------
vtkDataRepresentation::~vtkDataRepresentation()
{
  delete this->Implementation;
  this->SetSelectionLinkInternal(0);
  this->SetAnnotationLinkInternal(0);
}

//----------------------------------------------------------------------------
void vtkDataRepresentation::SetSelectionLink(vtkSelectionLink* link)
{
  this->SetSelectionLinkInternal(link);
}

//----------------------------------------------------------------------------
void vtkDataRepresentation::SetAnnotationLink(vtkAnnotationLink* link)
{
  this->SetAnnotationLinkInternal(link);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkDataRepresentation::GetInput(int port, int conn)
{
  if (port >= 0 && port < this->Implementation->Inputs.size() &&
      conn >= 0 && conn < this->Implementation->Inputs[port].size())
    {
    return this->Implementation->Inputs[port][conn]->GetInput();
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkDataRepresentation::GetSelectionConnection(
  int port, int conn)
{
  if (port >= 0 && port < this->Implementation->Inputs.size() &&
      conn >= 0 && conn < this->Implementation->Inputs[port].size())
    {
    return this->Implementation->Inputs[port][conn]->GetSelectionConnection();
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkDataRepresentation::GetAnnotationConnection(
  int port, int conn)
{
  if (port >= 0 && port < this->Implementation->Inputs.size() &&
      conn >= 0 && conn < this->Implementation->Inputs[port].size())
    {
    return this->AnnotationLinkInternal->GetOutputPort();
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkDataRepresentation::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  for (int i = this->Implementation->Inputs.size();
       i < this->GetNumberOfInputPorts(); ++i)
    {
    this->Implementation->Inputs.push_back(
      vtkstd::vector<vtkSmartPointer<vtkDataRepresentationInput> >());
    int connections = inputVector[i]->GetNumberOfInformationObjects();
    for (int j = this->Implementation->Inputs[i].size();
         j < connections; ++j)
      {
      vtkSmartPointer<vtkDataRepresentationInput> dri =
        vtkSmartPointer<vtkDataRepresentationInput>::New();
      this->Implementation->Inputs[i].push_back(dri);
      }
    }

  for (int i = 0; i < this->GetNumberOfInputPorts(); ++i)
    {
    int connections = inputVector[i]->GetNumberOfInformationObjects();
    for (int j = 0; j < connections; ++j)
      {
      vtkInformation* inInfo = inputVector[i]->GetInformationObject(j);
      vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
      vtkDataObject* inputCopy = input->NewInstance();
      inputCopy->ShallowCopy(input);
      this->Implementation->Inputs[i][j]->SetInput(inputCopy, this);
      inputCopy->Delete();
      }
    }
  this->SetupInputConnections();
  return 1;
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
      if (converted != selection)
        {
        converted->Delete();
        }
      }
    }
}

//----------------------------------------------------------------------------
vtkSelection* vtkDataRepresentation::ConvertSelection(
  vtkView* vtkNotUsed(view), vtkSelection* selection)
{
  return selection;
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
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SelectionLink: " << (this->SelectionLinkInternal ? "" : "(null)") << endl;
  if (this->SelectionLinkInternal)
    {
    this->SelectionLinkInternal->PrintSelf(os, indent.GetNextIndent());
    }  
  os << indent << "AnnotationLink: " << (this->AnnotationLinkInternal ? "" : "(null)") << endl;
  if (this->AnnotationLinkInternal)
    {
    this->AnnotationLinkInternal->PrintSelf(os, indent.GetNextIndent());
    }  
  os << indent << "Selectable: " << this->Selectable << endl;
}
