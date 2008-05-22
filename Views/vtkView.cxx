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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkView.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkDataRepresentation.h"
#include "vtkObjectFactory.h"
#include "vtkViewTheme.h"

#include <vtkstd/map>
#include <vtkstd/string>

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

//----------------------------------------------------------------------------
class vtkView::vtkInternal
{
public:
  vtkstd::map<vtkObject*, vtkstd::string> RegisteredProgress;
};


vtkCxxRevisionMacro(vtkView, "1.3");
vtkStandardNewMacro(vtkView);
//----------------------------------------------------------------------------
vtkView::vtkView()
{
  this->Internal = new vtkView::vtkInternal();
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
  this->Observer->SetTarget(0);
  this->Observer->Delete();
  delete this->Internal;
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
  void* callData)
{
  if (this->Representations->IsItemPresent(caller) && eventId == vtkCommand::SelectionChangedEvent)
    {
    this->InvokeEvent(vtkCommand::SelectionChangedEvent);
    }

  if (eventId == vtkCommand::ViewProgressEvent)
    {
    vtkstd::map<vtkObject*, vtkstd::string>::iterator iter = 
      this->Internal->RegisteredProgress.find(caller);
    if (iter != this->Internal->RegisteredProgress.end())
      {
      ViewProgressEventCallData eventdata(iter->second.c_str(),
        *(reinterpret_cast<const double*>(callData)));
      this->InvokeEvent(vtkCommand::ViewProgressEvent, &eventdata);
      }
    }
}

//----------------------------------------------------------------------------
void vtkView::RegisterProgress(vtkObject* algorithm, const char* message/*=NULL*/)
{
  if (algorithm)
    {
    const char* used_message = message? message : algorithm->GetClassName();
    this->Internal->RegisteredProgress[algorithm] = used_message;
    algorithm->AddObserver(vtkCommand::ProgressEvent, this->Observer);
    }
}

//----------------------------------------------------------------------------
void vtkView::UnRegisterProgress(vtkObject* algorithm)
{
  if (algorithm)
    {
    vtkstd::map<vtkObject*, vtkstd::string>::iterator iter = 
      this->Internal->RegisteredProgress.find(algorithm);
    if (iter != this->Internal->RegisteredProgress.end())
      {
      this->Internal->RegisteredProgress.erase(iter);
      algorithm->RemoveObservers(vtkCommand::ProgressEvent, this->Observer);
      }
    }
}

//----------------------------------------------------------------------------
void vtkView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
