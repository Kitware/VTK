/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCaptionActor.cxx
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
#include "vtkCaptionActor.h"
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
vtkCaptionActor* vtkCaptionActor::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkCaptionActor");
  if(ret)
    {
    return (vtkCaptionActor*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkCaptionActor;
}

vtkCaptionActor::vtkCaptionActor()
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
  this->LeaderGlyph = NULL;
  
  // Control font properties
  this->Padding = 3;
  this->Bold = 1;
  this->Italic = 1;
  this->Shadow = 1;
  this->FontFamily = VTK_ARIAL;
  this->Border = 1;

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

  this->HeadPolyData = vtkPolyData::New();//for glyphing
  pts = vtkPoints::New();
  pts->SetNumberOfPoints(1);
  this->HeadPolyData->SetPoints(pts);
  pts->Delete();
  vtkFloatArray *vecs = vtkFloatArray::New();
  vecs->SetNumberOfTuples(2);
  this->HeadPolyData->GetPointData()->SetVectors(vecs);
  vecs->Delete();

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

  this->HeadGlyph2D = vtkGlyph2D::New();
  this->HeadGlyph2D->SetInput(this->HeadPolyData);
  this->HeadGlyph3D = vtkGlyph3D::New();
  this->HeadGlyph3D->SetInput(this->HeadPolyData);
  this->AppendLeader = vtkAppendPolyData::New();
  this->AppendLeader->UserManagedInputsOn();
  this->AppendLeader->SetNumberOfInputs(2);
  this->AppendLeader->SetInput(this->LeaderPolyData);

  this->LeaderMapper2D = vtkPolyDataMapper2D::New();
  this->LeaderActor2D = vtkActor2D::New();
  this->LeaderActor2D->SetMapper(this->LeaderMapper2D);

  this->LeaderMapper3D = vtkPolyDataMapper::New();
  this->LeaderActor3D = vtkActor::New();
  this->LeaderActor3D->SetMapper(this->LeaderMapper3D);
}

vtkCaptionActor::~vtkCaptionActor()
{
  if ( this->Caption )
    {
    delete [] this->Caption;
    }
  
  this->AttachmentPointCoordinate->Delete();

  this->CaptionMapper->Delete();
  this->CaptionActor->Delete();
  
  this->BorderActor->Delete();
  this->BorderMapper->Delete();
  this->BorderPolyData->Delete();

  this->HeadPolyData->Delete();
  this->HeadGlyph2D->Delete();
  this->HeadGlyph3D->Delete();
  this->LeaderPolyData->Delete(); //line represents the leader
  this->AppendLeader->Delete(); //append head and leader
  
  this->LeaderActor2D->Delete();
  this->LeaderMapper2D->Delete();

  this->LeaderActor3D->Delete();
  this->LeaderMapper3D->Delete();
}

// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkCaptionActor::ReleaseGraphicsResources(vtkWindow *win)
{
  this->CaptionActor->ReleaseGraphicsResources(win); 
  this->BorderActor->ReleaseGraphicsResources(win); 
  this->LeaderActor2D->ReleaseGraphicsResources(win); 
}

int vtkCaptionActor::RenderOverlay(vtkViewport *viewport)
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

int vtkCaptionActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
  // Build the caption (almost always needed so we don't check mtime)
  vtkDebugMacro(<<"Rebuilding caption");

  // compute coordinates and set point values
  //
  int *x1, *x2, *x3;
  float p1[3], p2[3], p3[3];
  x1 = this->AttachmentPointCoordinate->GetComputedDisplayValue(viewport);
  x2 = this->PositionCoordinate->GetComputedDisplayValue(viewport);
  x3 = this->Position2Coordinate->GetComputedDisplayValue(viewport);
  p1[0] = (float)x1[0]; p1[1] = (float)x1[1]; p1[2] = 0.0;
  p2[0] = (float)x2[0]; p2[1] = (float)x2[1]; p2[2] = 0.0;
  p3[0] = (float)x3[0]; p3[1] = (float)x3[1]; p3[2] = 0.0;
  float *w1 = this->AttachmentPointCoordinate->GetComputedWorldValue(viewport);
  float *w2 = this->AttachmentPointCoordinate->GetComputedWorldValue(viewport);

  // Set up the scaled text - take into account the padding
  this->CaptionActor->GetPositionCoordinate()->SetValue(
    p2[0]+this->Padding,p2[1]+this->Padding,0.0);
  this->CaptionActor->GetPosition2Coordinate()->SetValue(
    p3[0]-this->Padding,p3[1]-this->Padding,0.0);

  // Define the border
  vtkPoints *pts = this->BorderPolyData->GetPoints();
  pts->SetPoint(0, p2);
  pts->SetPoint(1, p3[0],p2[1],0.0);
  pts->SetPoint(2, p3[0],p3[1],0.0);
  pts->SetPoint(3, p2[0],p3[1],0.0);

  // Update the info for later glyphing
  this->HeadPolyData->GetPoints()->SetPoint(0,w1);
  this->HeadPolyData->GetPointData()->GetVectors()->SetVector(0,
                      w2[0]-w1[0],w2[1]-w1[1],w2[2]-w1[2]);
     
  // The pipeline is connected differently depending on the dimension
  // and availability of a leader head.
  if ( this->LeaderGlyph )
    {
    if ( this->ThreeDimensionalLeader )
      {
      this->HeadGlyph3D->SetSource(this->HeadPolyData);
      this->AppendLeader->SetInput(this->HeadGlyph3D->GetOutput());
      this->LeaderMapper3D->SetInput(this->AppendLeader->GetOutput());
      }
    else
      {
      this->HeadGlyph2D->SetSource(this->HeadPolyData);
      this->AppendLeader->SetInput(this->HeadGlyph2D->GetOutput());
      this->LeaderMapper2D->SetInput(this->AppendLeader->GetOutput());
      }
    }
  else
    {
    this->LeaderMapper2D->SetInput(this->LeaderPolyData);
    this->LeaderMapper3D->SetInput(this->LeaderPolyData);
    }

  // Define the leader. Have to find the closest point from the
  // border to the attachment point. We look at the four vertices
  // and four edge centers.
  float d2, minD2, pt[3], minPt[3];
  minD2 = VTK_LARGE_FLOAT;

  pt[0] = p2[0]; pt[1] = p2[1]; pt[2] = 0.0;
  if ( (d2 = vtkMath::Distance2BetweenPoints(p1,pt)) < minD2 )
    {
    minD2 = d2;
    minPt[0] = pt[0]; minPt[1] = pt[1]; minPt[2] = pt[2];
    }

  pt[0] = (p2[0]+p3[0])/2.0;
  if ( (d2 = vtkMath::Distance2BetweenPoints(p1,pt)) < minD2 )
    {
    minD2 = d2;
    minPt[0] = pt[0]; minPt[1] = pt[1]; minPt[2] = pt[2];
    }

  pt[0] = p3[0];
  if ( (d2 = vtkMath::Distance2BetweenPoints(p1,pt)) < minD2 )
    {
    minD2 = d2;
    minPt[0] = pt[0]; minPt[1] = pt[1]; minPt[2] = pt[2];
    }

  pt[1] = (p2[1]+p3[1])/2.0;
  if ( (d2 = vtkMath::Distance2BetweenPoints(p1,pt)) < minD2 )
    {
    minD2 = d2;
    minPt[0] = pt[0]; minPt[1] = pt[1]; minPt[2] = pt[2];
    }

  pt[1] = p3[1];
  if ( (d2 = vtkMath::Distance2BetweenPoints(p1,pt)) < minD2 )
    {
    minD2 = d2;
    minPt[0] = pt[0]; minPt[1] = pt[1]; minPt[2] = pt[2];
    }

  pt[0] = (p2[0]+p3[0])/2.0;
  if ( (d2 = vtkMath::Distance2BetweenPoints(p1,pt)) < minD2 )
    {
    minD2 = d2;
    minPt[0] = pt[0]; minPt[1] = pt[1]; minPt[2] = pt[2];
    }

  pt[0] = p2[0];
  if ( (d2 = vtkMath::Distance2BetweenPoints(p1,pt)) < minD2 )
    {
    minD2 = d2;
    minPt[0] = pt[0]; minPt[1] = pt[1]; minPt[2] = pt[2];
    }

  pt[1] = (p2[1]+p3[1])/2.0;
  if ( (d2 = vtkMath::Distance2BetweenPoints(p1,pt)) < minD2 )
    {
    minD2 = d2;
    minPt[0] = pt[0]; minPt[1] = pt[1]; minPt[2] = pt[2];
    }

  pts = this->LeaderPolyData->GetPoints();
  pts->SetPoint(0, minPt);
  pts->SetPoint(1, p1);//the attachment point

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

void vtkCaptionActor::PrintSelf(ostream& os, vtkIndent indent)
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

  os << indent << "Leader: " << (this->Border ? "On\n" : "Off\n");
  os << indent << "Three Dimensional Leader: " 
     << (this->ThreeDimensionalLeader ? "On\n" : "Off\n");
  os << indent << "Leader Glyph Size: " 
     << this->LeaderGlyphSize << "\n";
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


void vtkCaptionActor::ShallowCopy(vtkProp *prop)
{
  vtkCaptionActor *a = vtkCaptionActor::SafeDownCast(prop);
  if ( a != NULL )
    {
    this->SetCaption(a->GetCaption());
    this->SetBorder(a->GetBorder());
    this->SetLeader(a->GetLeader());
    this->SetPadding(a->GetPadding());
    this->SetAttachmentPoint(a->GetAttachmentPoint());
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
  


