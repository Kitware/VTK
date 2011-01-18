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
#include "vtkTextActor.h"
#include "vtkTextMapper.h"
#include "vtkFreeTypeUtilities.h"

vtkStandardNewMacro(vtkCaptionRepresentation);

//-------------------------------------------------------------------------
vtkCaptionRepresentation::vtkCaptionRepresentation()
{
  this->AnchorRepresentation = vtkPointHandleRepresentation3D::New();
  this->AnchorRepresentation->AllOff();
  this->AnchorRepresentation->SetHotSpotSize(1.0);
  this->AnchorRepresentation->SetPlaceFactor(1.0);
  this->AnchorRepresentation->TranslationModeOn();
  this->AnchorRepresentation->ActiveRepresentationOn();

  this->CaptionActor2D = vtkCaptionActor2D::New();
  this->CaptionActor2D->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
  this->CaptionActor2D->GetPositionCoordinate()->SetReferenceCoordinate(0);
  this->CaptionActor2D->GetPosition2Coordinate()->SetCoordinateSystemToDisplay();
  this->CaptionActor2D->GetPosition2Coordinate()->SetReferenceCoordinate(0);
  this->CaptionActor2D->GetPositionCoordinate()->SetValue(10,10);
  this->CaptionActor2D->GetPosition2Coordinate()->SetValue(20,20);
  this->CaptionActor2D->SetCaption("Caption Here");
  this->CaptionActor2D->SetAttachmentPoint(0,0,0);
  this->CaptionActor2D->BorderOn();
  this->CaptionActor2D->LeaderOn();
  this->CaptionActor2D->ThreeDimensionalLeaderOn();

  this->CaptionGlyph = vtkConeSource::New();
  this->CaptionGlyph->SetResolution(6);
  this->CaptionGlyph->SetCenter(-0.5,0,0);
  this->CaptionActor2D->SetLeaderGlyph(this->CaptionGlyph->GetOutput());

  this->ShowBorder = vtkBorderRepresentation::BORDER_OFF;
  this->FontFactor = 1.0;
}

//-------------------------------------------------------------------------
vtkCaptionRepresentation::~vtkCaptionRepresentation()
{
  this->SetCaptionActor2D(0);
  this->CaptionGlyph->Delete();
  this->SetAnchorRepresentation(0);
}

//-------------------------------------------------------------------------
void vtkCaptionRepresentation::SetCaptionActor2D(vtkCaptionActor2D *capActor)
{
  if ( capActor != this->CaptionActor2D )
    {
    if ( this->CaptionActor2D )
      {
      this->CaptionActor2D->Delete();
      }
    this->CaptionActor2D = capActor;
    if ( this->CaptionActor2D )
      {
      this->CaptionActor2D->Register(this);
      this->CaptionActor2D->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
      this->CaptionActor2D->GetPositionCoordinate()->SetReferenceCoordinate(0);
      this->CaptionActor2D->GetPosition2Coordinate()->SetCoordinateSystemToDisplay();
      this->CaptionActor2D->GetPosition2Coordinate()->SetReferenceCoordinate(0);
      this->CaptionActor2D->GetPositionCoordinate()->SetValue(10,10);
      this->CaptionActor2D->GetPosition2Coordinate()->SetValue(20,20);
      this->CaptionActor2D->SetAttachmentPoint(0,0,0);
      this->CaptionActor2D->BorderOn();
      this->CaptionActor2D->LeaderOn();
      this->CaptionActor2D->ThreeDimensionalLeaderOn();
      this->CaptionActor2D->SetLeaderGlyph(this->CaptionGlyph->GetOutput());
      }
    this->Modified();
    }
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
  this->CaptionActor2D->GetAttachmentPointCoordinate()->SetValue(pos);
  this->AnchorRepresentation->SetWorldPosition(pos);
}

//-------------------------------------------------------------------------
void vtkCaptionRepresentation::GetAnchorPosition(double pos[3])
{
  this->CaptionActor2D->GetAttachmentPointCoordinate()->GetValue(pos);
}

//-------------------------------------------------------------------------
void vtkCaptionRepresentation::BuildRepresentation()
{
  if ( this->GetMTime() > this->BuildTime ||
       this->CaptionActor2D->GetMTime() > this->BuildTime ||
       (this->Renderer && this->Renderer->GetVTKWindow() &&
        this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime) )
    {

    // If the text actor's text scaling is off, we still want to be able
    // to change the caption's text size programmatically by changing a
    // *relative* font size factor. We will also need to change the
    // caption's boundary size accordingly.

    if(!this->Moving && this->CaptionActor2D
        && this->CaptionActor2D->GetCaption()
        && (this->CaptionActor2D->GetTextActor()->GetTextScaleMode()
            == vtkTextActor::TEXT_SCALE_MODE_NONE ))
      {
      // Create a dummy text mapper for getting font sizes
      vtkTextMapper *textMapper = vtkTextMapper::New();
      vtkTextProperty *tprop = textMapper->GetTextProperty();

      tprop->ShallowCopy(this->CaptionActor2D->GetCaptionTextProperty());
      textMapper->SetInput(this->CaptionActor2D->GetCaption());
      int textsize[2];
      int fsize = vtkTextMapper::SetRelativeFontSize(textMapper,
        this->Renderer, this->Renderer->GetSize(), textsize,
        0.015*this->FontFactor);
      this->CaptionActor2D->GetCaptionTextProperty()->SetFontSize(fsize);
      textMapper->Delete();
      this->AdjustCaptionBoundary();
      }

    // Ask the superclass the size and set the caption
    int *pos1 = this->PositionCoordinate->
      GetComputedDisplayValue(this->Renderer);
    int *pos2 = this->Position2Coordinate->
      GetComputedDisplayValue(this->Renderer);

    if ( this->CaptionActor2D )
      {
      this->CaptionActor2D->GetPositionCoordinate()->SetValue(pos1[0],pos1[1]);
      this->CaptionActor2D->GetPosition2Coordinate()->SetValue(pos2[0],pos2[1]);
      }

    // Note that the transform is updated by the superclass
    this->Superclass::BuildRepresentation();
    }
}

//----------------------------------------------------------------------------
void vtkCaptionRepresentation::AdjustCaptionBoundary()
{
  if(this->CaptionActor2D->GetCaption())
    {
    vtkFreeTypeUtilities* ftu = vtkFreeTypeUtilities::GetInstance();
    if (!ftu)
      {
      vtkErrorMacro(<<"Failed getting the FreeType utilities instance");
      return;
      }

    int text_bbox[4];
    ftu->GetBoundingBox(this->CaptionActor2D->GetCaptionTextProperty(),
      this->CaptionActor2D->GetCaption(), text_bbox);
    if (!ftu->IsBoundingBoxValid(text_bbox))
      {
      return;
      }

    // The bounding box was the area that is going to be filled with pixels
    // given a text origin of (0, 0). Now get the real size we need, i.e.
    // the full extent from the origin to the bounding box.

    double text_size[2];
    text_size[0] = (text_bbox[1] - text_bbox[0] + 5);
    text_size[1] = (text_bbox[3] - text_bbox[2] + 5);

    this->GetRenderer()->DisplayToNormalizedDisplay (text_size[0], text_size[1]);
    this->GetRenderer()->NormalizedDisplayToViewport (text_size[0], text_size[1]);
    this->GetRenderer()->ViewportToNormalizedViewport (text_size[0], text_size[1]);

    // update the PositionCoordinate
    //this->NeedToAdjustSize = 1;

    double* pos2 = this->Position2Coordinate->GetValue();
    if(pos2[0] != text_size[0] || pos2[1] != text_size[1])
      {
      this->Position2Coordinate->SetValue(text_size[0], text_size[1], 0);
      this->Modified();
      }
    }
}

//-------------------------------------------------------------------------
void vtkCaptionRepresentation::GetActors2D(vtkPropCollection *pc)
{
  pc->AddItem(this->CaptionActor2D);
  this->Superclass::GetActors2D(pc);
}

//-------------------------------------------------------------------------
void vtkCaptionRepresentation::ReleaseGraphicsResources(vtkWindow *w)
{
  this->CaptionActor2D->ReleaseGraphicsResources(w);
  this->Superclass::ReleaseGraphicsResources(w);
}

//-------------------------------------------------------------------------
int vtkCaptionRepresentation::RenderOverlay(vtkViewport *w)
{
  this->BuildRepresentation();
  int count = this->Superclass::RenderOverlay(w);
  count += this->CaptionActor2D->RenderOverlay(w);
  return count;
}

//-------------------------------------------------------------------------
int vtkCaptionRepresentation::RenderOpaqueGeometry(vtkViewport *w)
{
  this->BuildRepresentation();
  int count = this->Superclass::RenderOpaqueGeometry(w);
  count += this->CaptionActor2D->RenderOpaqueGeometry(w);
  return count;
}

//-------------------------------------------------------------------------
int vtkCaptionRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport *w)
{
  this->BuildRepresentation();
  int count = this->Superclass::RenderTranslucentPolygonalGeometry(w);
  count += this->CaptionActor2D->RenderTranslucentPolygonalGeometry(w);
  return count;
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
int vtkCaptionRepresentation::HasTranslucentPolygonalGeometry()
{
  this->BuildRepresentation();
  int result = this->Superclass::HasTranslucentPolygonalGeometry();
  result |= this->CaptionActor2D->HasTranslucentPolygonalGeometry();
  return result;
}

//-------------------------------------------------------------------------
void vtkCaptionRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Caption Actor: " << this->CaptionActor2D << "\n";
  os << indent << "Font Factor: " << this->FontFactor << "\n";

  os << indent << "Anchor Representation:\n";
  this->AnchorRepresentation->PrintSelf(os,indent.GetNextIndent());
}
