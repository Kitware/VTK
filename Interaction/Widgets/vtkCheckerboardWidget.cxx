/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCheckerboardWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCheckerboardWidget.h"
#include "vtkCheckerboardRepresentation.h"
#include "vtkSliderRepresentation3D.h"
#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkSliderWidget.h"

vtkStandardNewMacro(vtkCheckerboardWidget);


// The checkerboard simply observes the behavior of four vtkSliderWidgets.
// Here we create the command/observer classes to respond to the
// slider widgets.
class vtkCWCallback : public vtkCommand
{
public:
  static vtkCWCallback *New()
    { return new vtkCWCallback; }
  void Execute(vtkObject *, unsigned long eventId, void*) VTK_OVERRIDE
  {
      switch (eventId)
      {
        case vtkCommand::StartInteractionEvent:
          this->CheckerboardWidget->StartCheckerboardInteraction();
          break;
        case vtkCommand::InteractionEvent:
          this->CheckerboardWidget->CheckerboardInteraction(this->SliderNumber);
          break;
        case vtkCommand::EndInteractionEvent:
          this->CheckerboardWidget->EndCheckerboardInteraction();
          break;
      }
  }
  vtkCWCallback():SliderNumber(0),CheckerboardWidget(0) {}
  int SliderNumber; //the number of the currently active slider
  vtkCheckerboardWidget *CheckerboardWidget;
};


//----------------------------------------------------------------------
vtkCheckerboardWidget::vtkCheckerboardWidget()
{
  this->TopSlider    = vtkSliderWidget::New();
  this->TopSlider->KeyPressActivationOff();
  this->RightSlider  = vtkSliderWidget::New();
  this->RightSlider->KeyPressActivationOff();
  this->BottomSlider = vtkSliderWidget::New();
  this->BottomSlider->KeyPressActivationOff();
  this->LeftSlider   = vtkSliderWidget::New();
  this->LeftSlider->KeyPressActivationOff();

  // Set up the callbacks on the sliders
  vtkCWCallback *cwCallback0 = vtkCWCallback::New();
  cwCallback0->CheckerboardWidget = this;
  cwCallback0->SliderNumber = vtkCheckerboardRepresentation::TopSlider;
  this->TopSlider->AddObserver(vtkCommand::StartInteractionEvent, cwCallback0, this->Priority);
  this->TopSlider->AddObserver(vtkCommand::InteractionEvent, cwCallback0, this->Priority);
  this->TopSlider->AddObserver(vtkCommand::EndInteractionEvent, cwCallback0, this->Priority);
  cwCallback0->Delete(); //okay reference counting

  vtkCWCallback *cwCallback1 = vtkCWCallback::New();
  cwCallback1->CheckerboardWidget = this;
  cwCallback1->SliderNumber = vtkCheckerboardRepresentation::RightSlider;
  this->RightSlider->AddObserver(vtkCommand::StartInteractionEvent, cwCallback1, this->Priority);
  this->RightSlider->AddObserver(vtkCommand::InteractionEvent, cwCallback1, this->Priority);
  this->RightSlider->AddObserver(vtkCommand::EndInteractionEvent, cwCallback1, this->Priority);
  cwCallback1->Delete(); //okay reference counting

  vtkCWCallback *cwCallback2 = vtkCWCallback::New();
  cwCallback2->CheckerboardWidget = this;
  cwCallback2->SliderNumber = vtkCheckerboardRepresentation::BottomSlider;
  this->BottomSlider->AddObserver(vtkCommand::StartInteractionEvent, cwCallback2, this->Priority);
  this->BottomSlider->AddObserver(vtkCommand::InteractionEvent, cwCallback2, this->Priority);
  this->BottomSlider->AddObserver(vtkCommand::EndInteractionEvent, cwCallback2, this->Priority);
  cwCallback2->Delete(); //okay reference counting

  vtkCWCallback *cwCallback3 = vtkCWCallback::New();
  cwCallback3->CheckerboardWidget = this;
  cwCallback3->SliderNumber = vtkCheckerboardRepresentation::LeftSlider;
  this->LeftSlider->AddObserver(vtkCommand::StartInteractionEvent, cwCallback3, this->Priority);
  this->LeftSlider->AddObserver(vtkCommand::InteractionEvent, cwCallback3, this->Priority);
  this->LeftSlider->AddObserver(vtkCommand::EndInteractionEvent, cwCallback3, this->Priority);
  cwCallback3->Delete(); //okay reference counting
}

//----------------------------------------------------------------------
vtkCheckerboardWidget::~vtkCheckerboardWidget()
{
  this->TopSlider->Delete();
  this->RightSlider->Delete();
  this->BottomSlider->Delete();
  this->LeftSlider->Delete();
}

//----------------------------------------------------------------------
void vtkCheckerboardWidget::CreateDefaultRepresentation()
{
  if ( ! this->WidgetRep )
  {
    this->WidgetRep = vtkCheckerboardRepresentation::New();
  }
}

//----------------------------------------------------------------------
void vtkCheckerboardWidget::SetEnabled(int enabling)
{
  if ( ! this->Interactor )
  {
    vtkErrorMacro(<<"The interactor must be set prior to enabling/disabling widget");
    return;
  }

  if ( enabling ) //----------------
  {
    vtkDebugMacro(<<"Enabling checkerboard widget");

    if ( this->Enabled ) //already enabled, just return
    {
      return;
    }

    if ( ! this->CurrentRenderer )
    {
      this->SetCurrentRenderer(
        this->Interactor->FindPokedRenderer(
          this->Interactor->GetLastEventPosition()[0],
          this->Interactor->GetLastEventPosition()[1]));
      if (this->CurrentRenderer == NULL)
      {
        return;
      }
    }

    // Everything is ok, enable the representation
    this->Enabled = 1;
    this->CreateDefaultRepresentation();
    this->WidgetRep->SetRenderer(this->CurrentRenderer);

    // Configure this slider widgets
    this->TopSlider->SetInteractor(this->Interactor);
    this->RightSlider->SetInteractor(this->Interactor);
    this->BottomSlider->SetInteractor(this->Interactor);
    this->LeftSlider->SetInteractor(this->Interactor);

    // Make sure there is a representation
    this->WidgetRep->BuildRepresentation();
    this->TopSlider->SetRepresentation(reinterpret_cast<vtkCheckerboardRepresentation*>
                                       (this->WidgetRep)->GetTopRepresentation());
    this->RightSlider->SetRepresentation(reinterpret_cast<vtkCheckerboardRepresentation*>
                                         (this->WidgetRep)->GetRightRepresentation());
    this->BottomSlider->SetRepresentation(reinterpret_cast<vtkCheckerboardRepresentation*>
                                          (this->WidgetRep)->GetBottomRepresentation());
    this->LeftSlider->SetRepresentation(reinterpret_cast<vtkCheckerboardRepresentation*>
                                        (this->WidgetRep)->GetLeftRepresentation());


    // We temporarily disable the sliders to avoid multiple renders
    this->Interactor->Disable();
    this->TopSlider->SetEnabled(1);
    this->RightSlider->SetEnabled(1);
    this->BottomSlider->SetEnabled(1);
    this->LeftSlider->SetEnabled(1);
    this->Interactor->Enable();

    // Add the actors
    this->InvokeEvent(vtkCommand::EnableEvent,NULL);
  }

  else //disabling------------------
  {
    vtkDebugMacro(<<"Disabling checkerboard widget");

    if ( ! this->Enabled ) //already disabled, just return
    {
      return;
    }

    this->Enabled = 0;

    // Turn off the slider widgets. Temporariliy disable
    this->Interactor->Disable();
    this->TopSlider->SetEnabled(0);
    this->RightSlider->SetEnabled(0);
    this->BottomSlider->SetEnabled(0);
    this->LeftSlider->SetEnabled(0);
    this->Interactor->Enable();

    this->InvokeEvent(vtkCommand::DisableEvent,NULL);
    this->SetCurrentRenderer(NULL);
  }

  this->Render();
}

//----------------------------------------------------------------------
void vtkCheckerboardWidget::StartCheckerboardInteraction()
{
  this->Superclass::StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
}

//----------------------------------------------------------------------
void vtkCheckerboardWidget::CheckerboardInteraction(int sliderNum)
{
  reinterpret_cast<vtkCheckerboardRepresentation*>(this->WidgetRep)->
    SliderValueChanged(sliderNum);
  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
}

//----------------------------------------------------------------------
void vtkCheckerboardWidget::EndCheckerboardInteraction()
{
  this->Superclass::EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
}

//----------------------------------------------------------------------
void vtkCheckerboardWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  if ( this->TopSlider )
  {
    os << indent << "Top Slider: " << this->TopSlider << "\n";
  }
  else
  {
    os << indent << "Top Slider: (none)\n";
  }

  if ( this->BottomSlider )
  {
    os << indent << "Bottom Slider: " << this->BottomSlider << "\n";
  }
  else
  {
    os << indent << "Bottom Slider: (none)\n";
  }

  if ( this->BottomSlider )
  {
    os << indent << "Bottom Slider: " << this->BottomSlider << "\n";
  }
  else
  {
    os << indent << "Bottom Slider: (none)\n";
  }

  if ( this->LeftSlider )
  {
    os << indent << "Left Slider: " << this->LeftSlider << "\n";
  }
  else
  {
    os << indent << "Left Slider: (none)\n";
  }


}
