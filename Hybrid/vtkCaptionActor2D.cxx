/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCaptionActor2D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Tim Smith who sponsored and encouraged the development
             of this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkCaptionActor2D.h"
#include "vtkScaledTextActor.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkMath.h"
#include "vtkGlyph2D.h"
#include "vtkGlyph3D.h"
#include "vtkAppendPolyData.h"
#include "vtkActor.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkCaptionActor2D* vtkCaptionActor2D::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkCaptionActor2D");
  if(ret)
    {
    return (vtkCaptionActor2D*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkCaptionActor2D;
}

vtkCaptionActor2D::vtkCaptionActor2D()
{
  // Positioning information
  this->AttachmentPointCoordinate = vtkCoordinate::New();
  this->AttachmentPointCoordinate->SetCoordinateSystemToWorld();
  this->AttachmentPointCoordinate->SetValue(0.0,0.0,0.0);

  this->PositionCoordinate->SetCoordinateSystemToDisplay();
  this->PositionCoordinate->SetReferenceCoordinate(
    this->AttachmentPointCoordinate);
  this->PositionCoordinate->SetValue(10,10);
  
  // This sets up the Position2Coordinate
  this->vtkActor2D::SetWidth(0.25);
  this->vtkActor2D::SetHeight(0.10);

  this->Caption = NULL;
  this->Border = 1;
  this->Leader = 1;
  this->ThreeDimensionalLeader = 1;
  this->LeaderGlyphSize = 0.01;
  this->MaximumLeaderGlyphSize = 20;
  this->LeaderGlyph = NULL;
  
  // Control font properties
  this->Padding = 3;
  this->Bold = 1;
  this->Italic = 1;
  this->Shadow = 1;
  this->FontFamily = VTK_ARIAL;
  this->Justification = VTK_TEXT_LEFT;
  this->VerticalJustification = VTK_TEXT_BOTTOM;

  // What is actually drawn
  this->CaptionMapper = vtkTextMapper::New();
  this->CaptionActor = vtkScaledTextActor::New();
  this->CaptionActor->SetMapper(this->CaptionMapper);
  this->CaptionActor->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
  this->CaptionActor->GetPositionCoordinate()->SetReferenceCoordinate(NULL);
  this->CaptionActor->GetPosition2Coordinate()->SetCoordinateSystemToDisplay();
  this->CaptionActor->GetPosition2Coordinate()->SetReferenceCoordinate(NULL);

  this->BorderPolyData = vtkPolyData::New();
  vtkPoints *pts = vtkPoints::New();
  pts->SetNumberOfPoints(4);
  this->BorderPolyData->SetPoints(pts);
  pts->Delete();
  vtkCellArray *border = vtkCellArray::New();
  border->InsertNextCell(5);
  border->InsertCellPoint(0);
  border->InsertCellPoint(1);
  border->InsertCellPoint(2);
  border->InsertCellPoint(3);
  border->InsertCellPoint(0);
  this->BorderPolyData->SetLines(border);
  border->Delete();
  
  this->BorderMapper = vtkPolyDataMapper2D::New();
  this->BorderMapper->SetInput(this->BorderPolyData);
  this->BorderActor = vtkActor2D::New();
  this->BorderActor->SetMapper(this->BorderMapper);

  // This is for glyphing the head of the leader
  // A single point with a vector for glyph orientation
  this->HeadPolyData = vtkPolyData::New();
  pts = vtkPoints::New();
  pts->SetNumberOfPoints(1);
  this->HeadPolyData->SetPoints(pts);
  pts->Delete();
  vtkFloatArray *vecs = vtkFloatArray::New();
  vecs->SetNumberOfComponents(3);
  vecs->SetNumberOfTuples(1);
  this->HeadPolyData->GetPointData()->SetVectors(vecs);
  vecs->Delete();

  // This is the leader (line) from the attachment point to the caption
  this->LeaderPolyData = vtkPolyData::New();
  pts = vtkPoints::New();
  pts->SetNumberOfPoints(2);
  this->LeaderPolyData->SetPoints(pts);
  pts->Delete();
  vtkCellArray *leader = vtkCellArray::New();
  leader->InsertNextCell(2); 
  leader->InsertCellPoint(0);
  leader->InsertCellPoint(1); //at the attachment point
  this->LeaderPolyData->SetLines(leader);
  leader->Delete();

  // Used to generate the glyph on the leader head
  this->HeadGlyph = vtkGlyph3D::New();
  this->HeadGlyph->SetInput(this->HeadPolyData);
  this->HeadGlyph->SetScaleModeToDataScalingOff();
  this->HeadGlyph->SetScaleFactor(0.1);

  // Appends the leader and the glyph head
  this->AppendLeader = vtkAppendPolyData::New();
  this->AppendLeader->UserManagedInputsOn();
  this->AppendLeader->SetNumberOfInputs(2);
  this->AppendLeader->SetInputByNumber(0,this->LeaderPolyData);
  this->AppendLeader->SetInputByNumber(1,this->HeadGlyph->GetOutput());

  // Used to transform from world to other coordinate systems
  this->MapperCoordinate2D = vtkCoordinate::New();
  this->MapperCoordinate2D->SetCoordinateSystemToWorld();

  // If 2D leader is used, then use this mapper/actor combination
  this->LeaderMapper2D = vtkPolyDataMapper2D::New();
  this->LeaderMapper2D->SetTransformCoordinate(this->MapperCoordinate2D);
  this->LeaderActor2D = vtkActor2D::New();
  this->LeaderActor2D->SetMapper(this->LeaderMapper2D);

  // If 3D leader is used, then use this mapper/actor combination
  this->LeaderMapper3D = vtkPolyDataMapper::New();
  this->LeaderActor3D = vtkActor::New();
  this->LeaderActor3D->SetMapper(this->LeaderMapper3D);
}

vtkCaptionActor2D::~vtkCaptionActor2D()
{
  if ( this->Caption )
    {
    delete [] this->Caption;
    }
  
  this->AttachmentPointCoordinate->Delete();

  this->CaptionMapper->Delete();
  this->CaptionActor->Delete();
  
  if ( this->LeaderGlyph )
    {
    this->LeaderGlyph->Delete();
    }

  this->BorderPolyData->Delete();
  this->BorderMapper->Delete();
  this->BorderActor->Delete();

  this->HeadPolyData->Delete();
  this->LeaderPolyData->Delete();
  this->HeadGlyph->Delete();
  this->AppendLeader->Delete();
  
  this->MapperCoordinate2D->Delete();

  this->LeaderMapper2D->Delete();
  this->LeaderActor2D->Delete();

  this->LeaderMapper3D->Delete();
  this->LeaderActor3D->Delete();
}

// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkCaptionActor2D::ReleaseGraphicsResources(vtkWindow *win)
{
  this->CaptionActor->ReleaseGraphicsResources(win); 
  this->BorderActor->ReleaseGraphicsResources(win); 
  this->LeaderActor2D->ReleaseGraphicsResources(win); 
  this->LeaderActor3D->ReleaseGraphicsResources(win); 
}

int vtkCaptionActor2D::RenderOverlay(vtkViewport *viewport)
{
  int renderedSomething = 0;

  renderedSomething += this->CaptionActor->RenderOverlay(viewport);

  if ( this->Border )
    {
    renderedSomething += this->BorderActor->RenderOverlay(viewport);
    }

  if ( this->Leader )
    {
    if ( this->ThreeDimensionalLeader )
      {
      renderedSomething += this->LeaderActor3D->RenderOverlay(viewport);
      }
    else
      {
      renderedSomething += this->LeaderActor2D->RenderOverlay(viewport);
      }
    }

  return renderedSomething;
}

int vtkCaptionActor2D::RenderOpaqueGeometry(vtkViewport *viewport)
{
  // Build the caption (almost always needed so we don't check mtime)
  vtkDebugMacro(<<"Rebuilding caption");

  // compute coordinates and set point values
  //
  float *w1, *w2;
  int *x1, *x2, *x3;
  float p1[3], p2[3], p3[3];
  x1 = this->AttachmentPointCoordinate->GetComputedDisplayValue(viewport);
  x2 = this->PositionCoordinate->GetComputedDisplayValue(viewport);
  x3 = this->Position2Coordinate->GetComputedDisplayValue(viewport);
  p1[0] = (float)x1[0]; p1[1] = (float)x1[1]; p1[2] = 0.0;
  p2[0] = (float)x2[0]; p2[1] = (float)x2[1]; p2[2] = p1[2];
  p3[0] = (float)x3[0]; p3[1] = (float)x3[1]; p3[2] = p1[2];

  // Set up the scaled text - take into account the padding
  this->CaptionActor->GetPositionCoordinate()->SetValue(
    p2[0]+this->Padding,p2[1]+this->Padding,0.0);
  this->CaptionActor->GetPosition2Coordinate()->SetValue(
    p3[0]-this->Padding,p3[1]-this->Padding,0.0);

  // Define the border
  vtkPoints *pts = this->BorderPolyData->GetPoints();
  pts->SetPoint(0, p2);
  pts->SetPoint(1, p3[0],p2[1],p1[2]);
  pts->SetPoint(2, p3[0],p3[1],p1[2]);
  pts->SetPoint(3, p2[0],p3[1],p1[2]);

  // Define the leader. Have to find the closest point from the
  // border to the attachment point. We look at the four vertices
  // and four edge centers.
  float d2, minD2, pt[3], minPt[3];
  minD2 = VTK_LARGE_FLOAT;

  pt[0] = p2[0]; pt[1] = p2[1]; pt[2] = minPt[2] = 0.0;
  if ( (d2 = vtkMath::Distance2BetweenPoints(p1,pt)) < minD2 )
    {
    minD2 = d2;
    minPt[0] = pt[0]; minPt[1] = pt[1]; 
    }

  pt[0] = (p2[0]+p3[0])/2.0;
  if ( (d2 = vtkMath::Distance2BetweenPoints(p1,pt)) < minD2 )
    {
    minD2 = d2;
    minPt[0] = pt[0]; minPt[1] = pt[1]; 
    }

  pt[0] = p3[0];
  if ( (d2 = vtkMath::Distance2BetweenPoints(p1,pt)) < minD2 )
    {
    minD2 = d2;
    minPt[0] = pt[0]; minPt[1] = pt[1]; 
    }

  pt[1] = (p2[1]+p3[1])/2.0;
  if ( (d2 = vtkMath::Distance2BetweenPoints(p1,pt)) < minD2 )
    {
    minD2 = d2;
    minPt[0] = pt[0]; minPt[1] = pt[1]; 
    }

  pt[1] = p3[1];
  if ( (d2 = vtkMath::Distance2BetweenPoints(p1,pt)) < minD2 )
    {
    minD2 = d2;
    minPt[0] = pt[0]; minPt[1] = pt[1]; 
    }

  pt[0] = (p2[0]+p3[0])/2.0;
  if ( (d2 = vtkMath::Distance2BetweenPoints(p1,pt)) < minD2 )
    {
    minD2 = d2;
    minPt[0] = pt[0]; minPt[1] = pt[1]; 
    }

  pt[0] = p2[0];
  if ( (d2 = vtkMath::Distance2BetweenPoints(p1,pt)) < minD2 )
    {
    minD2 = d2;
    minPt[0] = pt[0]; minPt[1] = pt[1]; 
    }

  pt[1] = (p2[1]+p3[1])/2.0;
  if ( (d2 = vtkMath::Distance2BetweenPoints(p1,pt)) < minD2 )
    {
    minD2 = d2;
    minPt[0] = pt[0]; minPt[1] = pt[1]; 
    }

  // Set the leader coordinates in appropriate coordinate system
  // The pipeline is connected differently depending on the dimension
  // and availability of a leader head.
  if ( this->Leader )
    {
    pts = this->LeaderPolyData->GetPoints();

    w1 = this->AttachmentPointCoordinate->GetComputedWorldValue(viewport);
    viewport->SetWorldPoint(w1[0],w1[1],w1[2],1.0);
    viewport->WorldToView();
    viewport->GetViewPoint(p1);
    
    viewport->SetDisplayPoint(minPt[0],minPt[1],0.0);
    viewport->DisplayToView();
    viewport->GetViewPoint(p2);
    p2[2] = p1[2];
    viewport->SetViewPoint(p2);
    viewport->ViewToWorld();
    float w3[4];
    viewport->GetWorldPoint(w3);
    if ( w3[3] != 0.0 )
      {
      w3[0] /= w3[3]; w3[1] /= w3[3]; w3[2] /= w3[3];
      }
    w2 = w3;
    
    pts->SetPoint(0, w1);
    pts->SetPoint(1, w2);
    this->HeadPolyData->GetPoints()->SetPoint(0,w1);
    this->HeadPolyData->GetPointData()->
      GetVectors()->SetVector(0,w1[0]-w2[0],w1[1]-w2[1],w1[2]-w2[2]);

    pts->Modified();
    this->HeadPolyData->Modified();
    }

  if ( this->LeaderGlyph )
    {
    // compute the scale
    this->LeaderGlyph->Update();
    float length = this->LeaderGlyph->GetLength();
    int *sze = viewport->GetSize();
    int   numPixels = static_cast<int> (this->LeaderGlyphSize * 
      sqrt(static_cast<float>(sze[0]*sze[0] + sze[1]*sze[1])));
    numPixels = (numPixels > this->MaximumLeaderGlyphSize ?
                 this->MaximumLeaderGlyphSize : numPixels );

    this->MapperCoordinate2D->SetValue(sze[0]/2,0,0);
    w1 = this->MapperCoordinate2D->GetComputedWorldValue(viewport);
    p1[0] = w1[0]; p1[1] = w1[1]; p1[2] = w1[2];

    this->MapperCoordinate2D->SetValue((sze[0]/2)+1,0,0);
    w1 = this->MapperCoordinate2D->GetComputedWorldValue(viewport);
    p2[0] = w1[0]; p2[1] = w1[1]; p2[2] = w1[2];
    
    float sf = 0.01 * numPixels / 
               (length * sqrt(vtkMath::Distance2BetweenPoints(p1,p2)) );

    vtkDebugMacro(<<"Scale factor: " << sf);

    this->HeadGlyph->SetSource(this->LeaderGlyph);
    this->HeadGlyph->SetScaleFactor(sf);

    this->LeaderMapper2D->SetInput(this->AppendLeader->GetOutput());
    this->LeaderMapper3D->SetInput(this->AppendLeader->GetOutput());
    this->AppendLeader->Update();
    }
  else
    {
    this->LeaderMapper2D->SetInput(this->LeaderPolyData);
    this->LeaderMapper3D->SetInput(this->LeaderPolyData);
    this->LeaderPolyData->Update();
    }

  // assign properties
  //
  this->CaptionMapper->SetInput(this->Caption);
  this->CaptionMapper->SetBold(this->Bold);
  this->CaptionMapper->SetItalic(this->Italic);
  this->CaptionMapper->SetShadow(this->Shadow);
  this->CaptionMapper->SetFontFamily(this->FontFamily);
  this->CaptionMapper->SetJustification(this->Justification);
  this->CaptionMapper->SetVerticalJustificationToCentered();

  this->CaptionActor->SetProperty(this->GetProperty());
  this->BorderActor->SetProperty(this->GetProperty());
  this->LeaderActor2D->SetProperty(this->GetProperty());
  this->LeaderActor3D->GetProperty()->SetColor(
    this->GetProperty()->GetColor());

  // Okay we are ready to render something
  int renderedSomething = 0;
  renderedSomething += this->CaptionActor->RenderOpaqueGeometry(viewport);
  if ( this->Border )
    {
    renderedSomething += this->BorderActor->RenderOpaqueGeometry(viewport);
    }

  if ( this->Leader )
    {
    if ( this->ThreeDimensionalLeader )
      {
      renderedSomething += this->LeaderActor3D->RenderOpaqueGeometry(viewport);
      }
    else
      {
      renderedSomething += this->LeaderActor2D->RenderOpaqueGeometry(viewport);
      }
    }

  return renderedSomething;
}

void vtkCaptionActor2D::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkActor2D::PrintSelf(os,indent);

  os << indent << "Caption: ";
  if ( this->Caption )
    {
    os << this->Caption << "\n";
    }
  else
    {
    os << "(none)\n";
    }

  os << indent << "Leader: " << (this->Leader ? "On\n" : "Off\n");
  os << indent << "Three Dimensional Leader: " 
     << (this->ThreeDimensionalLeader ? "On\n" : "Off\n");
  os << indent << "Leader Glyph Size: " 
     << this->LeaderGlyphSize << "\n";
  os << indent << "MaximumLeader Glyph Size: " 
     << this->MaximumLeaderGlyphSize << "\n";
  if ( ! this->LeaderGlyph )
    {
    os << indent << "Leader Glyph: (none)\n";
    }
  else
    {
    os << indent << "Leader Glyph: (" << this->LeaderGlyph << ")\n";
    }

  os << indent << "Font Family: ";
  if ( this->FontFamily == VTK_ARIAL )
    {
    os << "Arial\n";
    }
  else if ( this->FontFamily == VTK_COURIER )
    {
    os << "Courier\n";
    }
  else
    {
    os << "Times\n";
    }
  os << indent << "Bold: " << (this->Bold ? "On\n" : "Off\n");
  os << indent << "Italic: " << (this->Italic ? "On\n" : "Off\n");
  os << indent << "Shadow: " << (this->Shadow ? "On\n" : "Off\n");
  os << indent << "Padding: " << this->Padding << "\n";
  os << indent << "Border: " << (this->Border ? "On\n" : "Off\n");
  os << indent << "Justification: ";
  switch (this->Justification)
    {
    case 0: os << "Left  (0)" << endl; break;
    case 1: os << "Centered  (1)" << endl; break;
    case 2: os << "Right  (2)" << endl; break;
    }
  os << indent << "VerticalJustification: ";
  switch (this->VerticalJustification)
    {
    case VTK_TEXT_TOP: os << "Top" << endl; break;
    case VTK_TEXT_CENTERED: os << "Centered" << endl; break;
    case VTK_TEXT_BOTTOM: os << "Bottom" << endl; break;
    }
}


void vtkCaptionActor2D::ShallowCopy(vtkProp *prop)
{
  vtkCaptionActor2D *a = vtkCaptionActor2D::SafeDownCast(prop);
  if ( a != NULL )
    {
    this->SetCaption(a->GetCaption());
    this->SetAttachmentPoint(a->GetAttachmentPoint());
    this->SetBorder(a->GetBorder());
    this->SetLeader(a->GetLeader());
    this->SetThreeDimensionalLeader(a->GetThreeDimensionalLeader());
    this->SetLeaderGlyph(a->GetLeaderGlyph());
    this->SetLeaderGlyphSize(a->GetLeaderGlyphSize());
    this->SetMaximumLeaderGlyphSize(a->GetMaximumLeaderGlyphSize());
    this->SetPadding(a->GetPadding());
    this->SetBold(a->GetBold());
    this->SetItalic(a->GetItalic());
    this->SetShadow(a->GetShadow());
    this->SetFontFamily(a->GetFontFamily());
    this->SetJustification(a->GetJustification());
    this->SetVerticalJustification(a->GetVerticalJustification());
    }

  // Now do superclass
  this->vtkActor2D::ShallowCopy(prop);
}
  


