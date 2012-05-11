/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkPainter.h"

#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDataSet.h"
#include "vtkDebugLeaks.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkTimerLog.h"

vtkCxxSetObjectMacro(vtkPainter, Input, vtkDataObject);
vtkCxxSetObjectMacro(vtkPainter, Information, vtkInformation);
vtkInformationKeyMacro(vtkPainter, STATIC_DATA, Integer);
vtkInformationKeyMacro(vtkPainter, CONSERVE_MEMORY, Integer);
vtkInformationKeyMacro(vtkPainter, HIGH_QUALITY, Integer);
//-----------------------------------------------------------------------------
class vtkPainterObserver : public vtkCommand
{
public:
  static vtkPainterObserver* New()
    { return new vtkPainterObserver; }

  virtual void Execute(vtkObject *caller,
    unsigned long event, void* vtkNotUsed(v))
    {
    vtkPainter* delegate = vtkPainter::SafeDownCast(caller);
    if (delegate && event == vtkCommand::ProgressEvent && this->Self)
      {
      this->Self->UpdateDelegateProgress(delegate, delegate->GetProgress());
      }
    }

  vtkPainter* Self;
};

//-----------------------------------------------------------------------------
vtkPainter::vtkPainter()
{
  this->Input = 0;
  this->DelegatePainter = NULL;
  this->LastWindow = 0;

  this->Progress = 0.0;
  this->ProgressOffset = 0.0;
  this->ProgressScaleFactor = 1.0;

  this->Observer = vtkPainterObserver::New();
  this->Observer->Self = this;

  this->TimeToDraw = 0.0;
  this->Timer = vtkTimerLog::New();

  this->Information = NULL;
  vtkInformation* temp = vtkInformation::New();
  this->SetInformation(temp);
  temp->Delete();

  vtkPainter::STATIC_DATA()->Set(this->Information, 0);
  vtkPainter::CONSERVE_MEMORY()->Set(this->Information, 0);
  vtkPainter::HIGH_QUALITY()->Set(this->Information, 1);
}

//-----------------------------------------------------------------------------
vtkPainter::~vtkPainter()
{
  this->SetInput(0);
  this->Observer->Self = NULL;
  this->Observer->Delete();

  this->SetDelegatePainter(NULL);
  this->SetInformation(NULL);

  if (this->LastWindow)
    {
    this->ReleaseGraphicsResources(this->LastWindow);
    this->LastWindow = 0;
    }

  this->Timer->Delete();
}

//-----------------------------------------------------------------------------
void vtkPainter::UpdateProgress(double amount)
{
  this->Progress = amount;
  this->InvokeEvent(vtkCommand::ProgressEvent,
    reinterpret_cast<void *>(&amount));
}

//-----------------------------------------------------------------------------
void vtkPainter::UpdateDelegateProgress(vtkPainter* vtkNotUsed(delegate),
  double amount)
{
  double scaled_amount = this->ProgressOffset +
    this->ProgressScaleFactor * amount;
  this->UpdateProgress(scaled_amount);
}

//-----------------------------------------------------------------------------
double vtkPainter::GetTimeToDraw()
{
  double time = this->TimeToDraw;
  if (this->DelegatePainter)
    {
    time += this->DelegatePainter->GetTimeToDraw();
    }
  return time;
}
//-----------------------------------------------------------------------------
void vtkPainter::ReleaseGraphicsResources(vtkWindow *w)
{
  if (this->DelegatePainter)
    {
    this->DelegatePainter->ReleaseGraphicsResources(w);
    }
}

//-----------------------------------------------------------------------------
void vtkPainter::Register(vtkObjectBase *o)
{
  this->RegisterInternal(o, 1);
}

//-----------------------------------------------------------------------------
void vtkPainter::UnRegister(vtkObjectBase *o)
{
  this->UnRegisterInternal(o, 1);
}

//-----------------------------------------------------------------------------
void vtkPainter::SetDelegatePainter(vtkPainter* delegate)
{
  if (this->DelegatePainter)
    {
    this->DelegatePainter->RemoveObserver(this->Observer);
    }

  vtkSetObjectBodyMacro(DelegatePainter, vtkPainter, delegate);

  if (this->DelegatePainter)
    {
    this->ObserverPainterProgress(this->DelegatePainter);
    }
}

//-----------------------------------------------------------------------------
void vtkPainter::ObserverPainterProgress(vtkPainter* p)
{
  p->AddObserver(vtkCommand::ProgressEvent, this->Observer);
}

//-----------------------------------------------------------------------------
void vtkPainter::ReportReferences(vtkGarbageCollector *collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->DelegatePainter,
    "Delegate Painter");
  vtkGarbageCollectorReport(collector, this->Input, "Input");
}

//-----------------------------------------------------------------------------
void vtkPainter::Render(vtkRenderer* renderer, vtkActor* actor,
                        unsigned long typeflags, bool forceCompileOnly)
{
  this->TimeToDraw = 0.0;
  if (renderer->GetRenderWindow()->CheckAbortStatus())
    {
    return;
    }

  if (this->InformationProcessTime < this->Information->GetMTime())
    {
    // If the information object was modified, some subclass may
    // want to get the modified information.
    // Using ProcessInformation avoids the need to access the Information
    // object during each render, thus reducing unnecessary
    // expensive information key accesses.
    this->ProcessInformation(this->Information);
    this->InformationProcessTime.Modified();
    }

  this->PrepareForRendering(renderer, actor);
  this->RenderInternal(renderer, actor, typeflags,forceCompileOnly);
}

//-----------------------------------------------------------------------------
void vtkPainter::RenderInternal(vtkRenderer* renderer, vtkActor* actor,
                                unsigned long typeflags, bool forceCompileOnly)
{
  if (this->DelegatePainter)
    {
    this->UpdateDelegatePainter();
    this->DelegatePainter->Render(renderer, actor, typeflags,forceCompileOnly);
    }
}

//-----------------------------------------------------------------------------
void vtkPainter::UpdateDelegatePainter()
{
  this->PassInformation(this->DelegatePainter);
}

//-----------------------------------------------------------------------------
void vtkPainter::PassInformation(vtkPainter* toPainter)
{
  /*
  if (this->Information->GetMTime() >
  toPainter->GetInformation()->GetMTime())
  {
  // We have updated information, pass it on to
  // the delegate.
  // We do a shallow copy.
  toPainter->GetInformation()->Copy(this->Information);
  }
  */
  // TODO: I can't decide if the information must be copied
  // or referenced.
  if (this->Information !=  toPainter->GetInformation())
    {
    // We have updated information, pass it on to
    // the delegate.
    toPainter->SetInformation(this->Information);
    }

  // Propagate the data object through the painter chain.
  vtkDataObject* myoutput = this->GetOutput();
  if (myoutput != toPainter->GetInput())
    {
    toPainter->SetInput(myoutput);
    }
}

//-------------------------------------------------------------------------
void vtkPainter::UpdateBounds(double bounds[6])
{
  // only apply UpdateBounds on the delegate painter
  vtkPainter *painter = this->GetDelegatePainter();

  if(painter)
    {
    // delegate the task of updating the bounds
    painter->UpdateBounds(bounds);
    }
}

//-----------------------------------------------------------------------------
// Description:
// Helper method to get input array to process.
vtkAbstractArray* vtkPainter::GetInputArrayToProcess(int fieldAssociation,
  int fieldAttributeType,
  vtkDataSet* inputDS,
  bool *use_cell_data)
{
  if (use_cell_data)
    {
    *use_cell_data = false;
    }
  if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
    return inputDS->GetPointData()->GetAbstractAttribute(fieldAttributeType);
    }

  if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS)
    {
    vtkAbstractArray* array =
      inputDS->GetPointData()->GetAbstractAttribute(fieldAttributeType);
    if (array)
      {
      return array;
      }
    }

  if (use_cell_data)
    {
    *use_cell_data = true;
    }
  return inputDS->GetCellData()->GetAbstractAttribute(fieldAttributeType);
}

//-----------------------------------------------------------------------------
// Description:
// Helper method to get input array to process.
vtkAbstractArray* vtkPainter::GetInputArrayToProcess(int fieldAssociation,
  const char* name, vtkDataSet* inputDS, bool *use_cell_data)
{
  if (use_cell_data)
    {
    *use_cell_data = false;
    }
  if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
    return inputDS->GetPointData()->GetAbstractArray(name);
    }

  if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS)
    {
    vtkAbstractArray* array =
      inputDS->GetPointData()->GetAbstractArray(name);
    if (array)
      {
      return array;
      }
    }

  if (use_cell_data)
    {
    *use_cell_data = true;
    }
  return inputDS->GetCellData()->GetAbstractArray(name);
}

//-----------------------------------------------------------------------------
void vtkPainter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Input: " << this->Input << endl;
  os << indent << "TimeToDraw: " << this->TimeToDraw << endl;
  os << indent << "Progress: " << this->Progress << endl;
  os << indent << "Information: " ;
  if (this->Information)
    {
    os << endl;
    this->Information->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }

  os << indent << "DelegatePainter: " ;
  if (this->DelegatePainter)
    {
    os << endl;
    this->DelegatePainter->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }

}

