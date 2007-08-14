/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkView.cxx

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

#include "vtkView.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkDataRepresentation.h"
#include "vtkObjectFactory.h"
#include "vtkViewTheme.h"

//----------------------------------------------------------------------------
class vtkView::Command : public vtkCommand
{
public:
  static Command* New() {  return new Command(); }
  virtual void Execute(vtkObject *caller, unsigned long eventId,
                       void *callData)
    {
    if (this->Target)
      {
      this->Target->ProcessEvents(caller, eventId, callData);
      }
    }
  void SetTarget(vtkView* t)
    {
    this->Target = t;
    }
private:
  Command() { this->Target = 0; }
  vtkView* Target;
};

vtkCxxRevisionMacro(vtkView, "1.1");
vtkStandardNewMacro(vtkView);
//----------------------------------------------------------------------------
vtkView::vtkView()
{
  this->Representations = vtkCollection::New();
  this->Observer = vtkView::Command::New();
  this->Observer->SetTarget(this);
  
  // Apply default theme
  vtkViewTheme* theme = vtkViewTheme::New();
  this->ApplyViewTheme(theme);
  theme->Delete();  
}

//----------------------------------------------------------------------------
vtkView::~vtkView()
{
  this->Representations->Delete();
  this->Observer->Delete();
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkView::AddRepresentationFromInput(vtkDataObject* input)
{
  return this->AddRepresentationFromInputConnection(input->GetProducerPort());
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkView::AddRepresentationFromInputConnection(vtkAlgorithmOutput* conn)
{
  vtkDataRepresentation* rep = vtkDataRepresentation::New();
  rep->SetInputConnection(conn);
  this->RemoveAllRepresentations();
  this->AddRepresentation(rep);
  rep->Delete();
  return rep;
}

//----------------------------------------------------------------------------
void vtkView::AddRepresentation(vtkDataRepresentation* rep)
{
  if (!this->Representations->IsItemPresent(rep))
    {
    if (rep->AddToView(this))
      {
      rep->AddObserver(vtkCommand::SelectionChangedEvent, this->GetObserver());
      this->AddInputConnection(rep->GetInputConnection());
      this->SetSelectionLink(rep->GetSelectionLink());
      this->Representations->AddItem(rep);
      }
    }
}

//----------------------------------------------------------------------------
void vtkView::RemoveRepresentation(vtkDataRepresentation* rep)
{
  if (this->Representations->IsItemPresent(rep))
    {
    rep->RemoveFromView(this);
    rep->RemoveObserver(this->GetObserver());
    this->RemoveInputConnection(rep->GetInputConnection());
    this->Representations->RemoveItem(rep);
    }
}

//----------------------------------------------------------------------------
void vtkView::RemoveRepresentation(vtkAlgorithmOutput* conn)
{
  for (int i = this->Representations->GetNumberOfItems() - 1; i >= 0; --i)
    {
    vtkDataRepresentation* rep = vtkDataRepresentation::SafeDownCast(
        this->Representations->GetItemAsObject(i));
    if (rep->GetInputConnection() == conn)
      {
      this->RemoveRepresentation(rep);
      }
    }
}

//----------------------------------------------------------------------------
void vtkView::RemoveAllRepresentations()
{
  while (this->Representations->GetNumberOfItems() > 0)
    {
    vtkDataRepresentation* rep = vtkDataRepresentation::SafeDownCast(
        this->Representations->GetItemAsObject(0));
    this->RemoveRepresentation(rep);
    }
}

//----------------------------------------------------------------------------
int vtkView::GetNumberOfRepresentations()
{
  return this->Representations->GetNumberOfItems();
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkView::GetRepresentation(int index)
{
  return vtkDataRepresentation::SafeDownCast(
    this->Representations->GetItemAsObject(index));
}

//----------------------------------------------------------------------------
vtkCommand* vtkView::GetObserver()
{
  return this->Observer;
}

//----------------------------------------------------------------------------
void vtkView::ProcessEvents(vtkObject* caller, unsigned long eventId, 
  void* vtkNotUsed(callData))
{
  if (this->Representations->IsItemPresent(caller) && eventId == vtkCommand::SelectionChangedEvent)
    {
    this->InvokeEvent(vtkCommand::SelectionChangedEvent);
    }
}

//----------------------------------------------------------------------------
void vtkView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
