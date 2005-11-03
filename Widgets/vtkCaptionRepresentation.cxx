/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCaptionRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCaptionRepresentation.h"
#include "vtkCaptionActor2D.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkConeSource.h"
#include "vtkTextProperty.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkPointWidget.h"
#include "vtkRenderWindow.h"

vtkCxxRevisionMacro(vtkCaptionRepresentation, "1.2");
vtkStandardNewMacro(vtkCaptionRepresentation);

vtkCxxSetObjectMacro(vtkCaptionRepresentation,CaptionActor,vtkCaptionActor2D);

//-------------------------------------------------------------------------
vtkCaptionRepresentation::vtkCaptionRepresentation()
{
  this->AnchorRepresentation = vtkPointHandleRepresentation3D::New();
  this->AnchorRepresentation->AllOff();
  this->AnchorRepresentation->SetHotSpotSize(1.0);
  this->AnchorRepresentation->SetPlaceFactor(1.0);
  this->AnchorRepresentation->TranslationModeOn();
  this->AnchorRepresentation->ActiveRepresentationOn();

  this->CaptionActor = vtkCaptionActor2D::New();
  this->CaptionActor->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
  this->CaptionActor->GetPositionCoordinate()->SetReferenceCoordinate(0);
  this->CaptionActor->GetPosition2Coordinate()->SetCoordinateSystemToDisplay();
  this->CaptionActor->GetPosition2Coordinate()->SetReferenceCoordinate(0);
  this->CaptionActor->GetPositionCoordinate()->SetValue(10,10);
  this->CaptionActor->GetPosition2Coordinate()->SetValue(20,20);
  this->CaptionActor->SetCaption("Caption Here");
  this->CaptionActor->SetAttachmentPoint(0,0,0);
  this->CaptionActor->BorderOn();
  this->CaptionActor->LeaderOn();
  this->CaptionActor->ThreeDimensionalLeaderOn();
  
  this->CaptionGlyph = vtkConeSource::New();
  this->CaptionGlyph->SetResolution(6);
  this->CaptionGlyph->SetCenter(-0.5,0,0);
  this->CaptionActor->SetLeaderGlyph(this->CaptionGlyph->GetOutput());

  this->ShowBorder = vtkBorderRepresentation::BORDER_OFF;
}

//-------------------------------------------------------------------------
vtkCaptionRepresentation::~vtkCaptionRepresentation()
{
  this->SetCaptionActor(0);
  this->CaptionGlyph->Delete();
  this->SetAnchorRepresentation(0);
}

//-------------------------------------------------------------------------
void vtkCaptionRepresentation::SetAnchorRepresentation(vtkPointHandleRepresentation3D *rep)
{
  if ( rep != this->AnchorRepresentation )
    {
    if ( this->AnchorRepresentation )
      {
      this->AnchorRepresentation->Delete();
      }
    this->AnchorRepresentation = rep;
    if ( this->AnchorRepresentation )
      {
      this->AnchorRepresentation->Register(this);
      }
    this->Modified();
    }
}

//-------------------------------------------------------------------------
void vtkCaptionRepresentation::SetAnchorPosition(double pos[3])
{
  this->CaptionActor->GetAttachmentPointCoordinate()->SetValue(pos);
}

//-------------------------------------------------------------------------
void vtkCaptionRepresentation::GetAnchorPosition(double pos[3])
{
  this->CaptionActor->GetAttachmentPointCoordinate()->GetValue(pos);
}

//-------------------------------------------------------------------------
void vtkCaptionRepresentation::BuildRepresentation()
{
  if ( this->GetMTime() > this->BuildTime || 
       (this->Renderer && this->Renderer->GetVTKWindow() &&
        this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime) )
    {
    // Ask the superclass the size and set the caption
    int *pos1 = this->PositionCoordinate->
      GetComputedDisplayValue(this->Renderer);
    int *pos2 = this->Position2Coordinate->
      GetComputedDisplayValue(this->Renderer);

    if ( this->CaptionActor )
      {
      this->CaptionActor->GetPositionCoordinate()->SetValue(pos1[0],pos1[1]);
      this->CaptionActor->GetPosition2Coordinate()->SetValue(pos2[0],pos2[1]);
      }

    // Note that the transform is updated by the superclass
    this->Superclass::BuildRepresentation();
    }
}

//-------------------------------------------------------------------------
void vtkCaptionRepresentation::GetActors2D(vtkPropCollection *pc)
{
  pc->AddItem(this->CaptionActor);
  this->Superclass::GetActors2D(pc);
}

//-------------------------------------------------------------------------
void vtkCaptionRepresentation::ReleaseGraphicsResources(vtkWindow *w)
{
  this->CaptionActor->ReleaseGraphicsResources(w);
  this->Superclass::ReleaseGraphicsResources(w);
}

//-------------------------------------------------------------------------
int vtkCaptionRepresentation::RenderOverlay(vtkViewport *w)
{
  this->BuildRepresentation();
  int count = this->Superclass::RenderOverlay(w);
  count += this->CaptionActor->RenderOverlay(w);
  return count;
}

//-------------------------------------------------------------------------
int vtkCaptionRepresentation::RenderOpaqueGeometry(vtkViewport *w)
{
  this->BuildRepresentation();
  int count = this->Superclass::RenderOpaqueGeometry(w);
  count += this->CaptionActor->RenderOpaqueGeometry(w);
  return count;
}

//-------------------------------------------------------------------------
int vtkCaptionRepresentation::RenderTranslucentGeometry(vtkViewport *w)
{
  this->BuildRepresentation();
  int count = this->Superclass::RenderTranslucentGeometry(w);
  count += this->CaptionActor->RenderTranslucentGeometry(w);
  return count;
}

//-------------------------------------------------------------------------
void vtkCaptionRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Caption Actor: " << this->CaptionActor << "\n";
  
}
