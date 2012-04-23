/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDistanceRepresentation3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDistanceRepresentation3D.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkPolyDataMapper.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkVectorText.h"
#include "vtkFollower.h"
#include "vtkCamera.h"
#include "vtkProperty.h"
#include "vtkCoordinate.h"
#include "vtkRenderer.h"
#include "vtkObjectFactory.h"
#include "vtkInteractorObserver.h"
#include "vtkMath.h"
#include "vtkWindow.h"
#include "vtkSmartPointer.h"
#include "vtkBox.h"
#include "vtkGlyph3D.h"
#include "vtkCylinderSource.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkTransform.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkDistanceRepresentation3D);

//----------------------------------------------------------------------
vtkDistanceRepresentation3D::vtkDistanceRepresentation3D()
{
  // By default, use one of these handles
  this->HandleRepresentation  = vtkPointHandleRepresentation3D::New();

  // The line
  this->LinePoints = vtkPoints::New();
  this->LinePoints->SetDataTypeToDouble();
  this->LinePoints->SetNumberOfPoints(2);
  this->LinePolyData = vtkPolyData::New();
  this->LinePolyData->SetPoints(this->LinePoints);
  vtkSmartPointer<vtkCellArray> line = vtkSmartPointer<vtkCellArray>::New();
  line->InsertNextCell(2);
  line->InsertCellPoint(0);
  line->InsertCellPoint(1);
  this->LinePolyData->SetLines(line);
  this->LineMapper = vtkPolyDataMapper::New();
  this->LineMapper->SetInputData(this->LinePolyData);
  this->LineActor = vtkActor::New();
  this->LineActor->SetMapper(this->LineMapper);

  // The label
  this->LabelText = vtkVectorText::New();
  this->LabelMapper = vtkPolyDataMapper::New();
  this->LabelMapper->SetInputConnection(this->LabelText->GetOutputPort());
  this->LabelActor = vtkFollower::New();
  this->LabelActor->SetMapper(this->LabelMapper);

  // The tick marks
  this->GlyphPoints = vtkPoints::New();
  this->GlyphPoints->SetDataTypeToDouble();
  this->GlyphVectors = vtkDoubleArray::New();
  this->GlyphVectors->SetNumberOfComponents(3);
  this->GlyphPolyData = vtkPolyData::New();
  this->GlyphPolyData->SetPoints(this->GlyphPoints);
  this->GlyphPolyData->GetPointData()->SetVectors(this->GlyphVectors);
  this->GlyphCylinder = vtkCylinderSource::New();
  this->GlyphCylinder->SetRadius(0.5);
  this->GlyphCylinder->SetHeight(0.1);
  this->GlyphCylinder->SetResolution(12);
  vtkSmartPointer<vtkTransform> xform = vtkSmartPointer<vtkTransform>::New();
  this->GlyphXForm = vtkTransformPolyDataFilter::New();
  this->GlyphXForm->SetInputConnection(this->GlyphCylinder->GetOutputPort());
  this->GlyphXForm->SetTransform(xform);
  xform->RotateZ(90);
  this->Glyph3D = vtkGlyph3D::New();
  this->Glyph3D->SetInputData(this->GlyphPolyData);
  this->Glyph3D->SetSourceConnection(this->GlyphXForm->GetOutputPort());
  this->Glyph3D->SetScaleModeToDataScalingOff();
  this->GlyphMapper = vtkPolyDataMapper::New();
  this->GlyphMapper->SetInputConnection(this->Glyph3D->GetOutputPort());
  this->GlyphActor = vtkActor::New();
  this->GlyphActor->SetMapper(this->GlyphMapper);

  this->Distance = 0.0;

  // The bounding box
  this->BoundingBox = vtkBox::New();

  // Scaling the label
  this->LabelScaleSpecified = false;

  // Controlling scaling and label position
  this->GlyphScale = 1.0;
  this->GlyphScaleSpecified = false;
  this->LabelPosition = 0.5;
  this->MaximumNumberOfRulerTicks = 99;
}

//----------------------------------------------------------------------
vtkDistanceRepresentation3D::~vtkDistanceRepresentation3D()
{
  this->LinePoints->Delete();
  this->LinePolyData->Delete();
  this->LineMapper->Delete();
  this->LineActor->Delete();

  this->LabelText->Delete();
  this->LabelMapper->Delete();
  this->LabelActor->Delete();

  this->GlyphPoints->Delete();
  this->GlyphVectors->Delete();
  this->GlyphPolyData->Delete();
  this->GlyphCylinder->Delete();
  this->GlyphXForm->Delete();
  this->Glyph3D->Delete();
  this->GlyphMapper->Delete();
  this->GlyphActor->Delete();

  this->BoundingBox->Delete();
}

//----------------------------------------------------------------------
void vtkDistanceRepresentation3D::GetPoint1WorldPosition(double pos[3])
{
  this->Point1Representation->GetWorldPosition(pos);
}

//----------------------------------------------------------------------
void vtkDistanceRepresentation3D::GetPoint2WorldPosition(double pos[3])
{
  this->Point2Representation->GetWorldPosition(pos);
}

//----------------------------------------------------------------------
double* vtkDistanceRepresentation3D::GetPoint1WorldPosition()
{
  if (!this->Point1Representation)
    {
    static double temp[3]=  {0, 0, 0};
    return temp;
    }
  return this->Point1Representation->GetWorldPosition();
}

//----------------------------------------------------------------------
double* vtkDistanceRepresentation3D::GetPoint2WorldPosition()
{
  if (!this->Point2Representation)
    {
    static double temp[3]=  {0, 0, 0};
    return temp;
    }
  return this->Point2Representation->GetWorldPosition();
}

//----------------------------------------------------------------------
void vtkDistanceRepresentation3D::SetPoint1DisplayPosition(double x[3])
{
  this->Point1Representation->SetDisplayPosition(x);
  double p[3];
  this->Point1Representation->GetWorldPosition(p);
  this->Point1Representation->SetWorldPosition(p);
}

//----------------------------------------------------------------------
void vtkDistanceRepresentation3D::SetPoint2DisplayPosition(double x[3])
{
  this->Point2Representation->SetDisplayPosition(x);
  double p[3];
  this->Point2Representation->GetWorldPosition(p);
  this->Point2Representation->SetWorldPosition(p);
}

//----------------------------------------------------------------------
void vtkDistanceRepresentation3D::SetPoint1WorldPosition(double x[3])
{
  if (this->Point1Representation)
    {
    this->Point1Representation->SetWorldPosition(x);
    }
}

//----------------------------------------------------------------------
void vtkDistanceRepresentation3D::SetPoint2WorldPosition(double x[3])
{
  if (this->Point2Representation)
    {
    this->Point2Representation->SetWorldPosition(x);
    }
}

//----------------------------------------------------------------------
void vtkDistanceRepresentation3D::GetPoint1DisplayPosition(double pos[3])
{
  this->Point1Representation->GetDisplayPosition(pos);
  pos[2] = 0.0;
}

//----------------------------------------------------------------------
void vtkDistanceRepresentation3D::GetPoint2DisplayPosition(double pos[3])
{
  this->Point2Representation->GetDisplayPosition(pos);
  pos[2] = 0.0;
}

//----------------------------------------------------------------------
double *vtkDistanceRepresentation3D::GetBounds()
{
  if(this->Point1Representation && this->Point2Representation)
    {
    this->BuildRepresentation();
    this->BoundingBox->SetBounds(this->Point1Representation->GetBounds());
    this->BoundingBox->AddBounds(this->Point2Representation->GetBounds());
    }
  this->BoundingBox->AddBounds(this->LineActor->GetBounds());

  return this->BoundingBox->GetBounds();
}

//----------------------------------------------------------------------
void vtkDistanceRepresentation3D::BuildRepresentation()
{
  if ( this->GetMTime() > this->BuildTime ||
       this->LabelActor->GetMTime() > this->BuildTime ||
       this->BoundingBox->GetMTime() > this->BuildTime ||
       this->GlyphActor->GetMTime() > this->BuildTime ||
       this->LineActor->GetMTime() > this->BuildTime ||
       this->Point1Representation->GetMTime() > this->BuildTime ||
       this->Point2Representation->GetMTime() > this->BuildTime ||
       (this->Renderer && this->Renderer->GetVTKWindow() &&
        this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime) )
    {
    this->Superclass::BuildRepresentation();

    // Okay, compute the distance and set the label
    double p1[3], p2[3];
    this->Point1Representation->GetWorldPosition(p1);
    this->Point2Representation->GetWorldPosition(p2);
    this->Distance = sqrt(vtkMath::Distance2BetweenPoints(p1,p2));

    // Line
    this->LinePoints->SetPoint(0,p1);
    this->LinePoints->SetPoint(1,p2);
    this->LinePoints->Modified();

    // Label
    char string[512];
    sprintf(string, this->LabelFormat, this->Distance);
    this->LabelText->SetText(string);
    this->UpdateLabelPosition();
    if (this->Renderer) //make the label face the camera
      {
      this->LabelActor->SetCamera( this->Renderer->GetActiveCamera() );
      }

    if (!this->LabelScaleSpecified)
      {
      // If a font size hasn't been specified by the user, scale the text
      // (font size) according to the length of the line widget.
      this->LabelActor->SetScale(
          this->Distance/20.0, this->Distance/20.0, this->Distance/20.0 );
      }

    // Ticks - generate points that are glyphed
    int i, numTicks;
    double v21[3], x[3];
    v21[0] =  p2[0] - p1[0]; v21[1] =  p2[1] - p1[1]; v21[2] =  p2[2] - p1[2];
    vtkMath::Normalize(v21);
    this->GlyphPoints->Reset();
    this->GlyphPoints->Modified();
    this->GlyphVectors->Reset();
    if (this->GlyphScaleSpecified)
      {
      this->Glyph3D->SetScaleFactor(this->GlyphScale);
      }
    else
      {
      this->Glyph3D->SetScaleFactor(this->Distance/40);
      }
    double distance;
    if ( this->RulerMode ) // specified tick separation
      {
      numTicks = (this->RulerDistance <= 0.0 ? 1 : static_cast<int>(this->Distance / this->RulerDistance));
      numTicks = (numTicks > this->MaximumNumberOfRulerTicks ? this->MaximumNumberOfRulerTicks : numTicks);
      distance = this->RulerDistance;
      }
    else //evenly spaced
      {
      numTicks = this->NumberOfRulerTicks;
      distance = this->Distance / (numTicks + 1);
      }
    for (i=1; i <= numTicks; ++i)
      {
      x[0] = p1[0] + i*v21[0]*distance;
      x[1] = p1[1] + i*v21[1]*distance;
      x[2] = p1[2] + i*v21[2]*distance;
      this->GlyphPoints->InsertNextPoint(x);
      this->GlyphVectors->InsertNextTuple(v21);
      }

    this->BuildTime.Modified();
    }
}

//----------------------------------------------------------------------
void vtkDistanceRepresentation3D::
ReleaseGraphicsResources(vtkWindow *w)
{
  this->LineActor->ReleaseGraphicsResources(w);
  this->LabelActor->ReleaseGraphicsResources(w);
  this->GlyphActor->ReleaseGraphicsResources(w);
}


//----------------------------------------------------------------------
int vtkDistanceRepresentation3D::
RenderOpaqueGeometry(vtkViewport *v)
{
  this->BuildRepresentation();

  this->LineActor->RenderOpaqueGeometry(v);
  this->LabelActor->RenderOpaqueGeometry(v);
  this->GlyphActor->RenderOpaqueGeometry(v);

  return 3;
}

//----------------------------------------------------------------------
int vtkDistanceRepresentation3D::
RenderTranslucentPolygonalGeometry(vtkViewport *v)
{
  this->BuildRepresentation();

  this->LineActor->RenderTranslucentPolygonalGeometry(v);
  this->LabelActor->RenderTranslucentPolygonalGeometry(v);
  this->GlyphActor->RenderTranslucentPolygonalGeometry(v);

  return 3;
}

//----------------------------------------------------------------------
void vtkDistanceRepresentation3D::SetLabelScale( double scale[3] )
{
  this->LabelActor->SetScale( scale );
  this->LabelScaleSpecified = true;
}

//----------------------------------------------------------------------
double * vtkDistanceRepresentation3D::GetLabelScale()
{
  return this->LabelActor->GetScale();
}

//----------------------------------------------------------------------------
vtkProperty * vtkDistanceRepresentation3D::GetLabelProperty()
{
  return this->LabelActor->GetProperty();
}

//----------------------------------------------------------------------
void vtkDistanceRepresentation3D::SetGlyphScale( double scale )
{
  this->GlyphScale = scale;
  this->GlyphScaleSpecified = true;
}

//----------------------------------------------------------------------------
vtkProperty * vtkDistanceRepresentation3D::GetLineProperty()
{
  return this->LineActor->GetProperty();
}

//----------------------------------------------------------------------
void vtkDistanceRepresentation3D::SetLabelPosition(double labelPosition)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting LabelPosition to " << labelPosition);

  if (this->LabelPosition == labelPosition)
    {
    ;
    }
  else
    {
    this->LabelPosition = labelPosition;
    }
  this->UpdateLabelPosition();
 }

//----------------------------------------------------------------------
void vtkDistanceRepresentation3D::UpdateLabelPosition()
{
  if (!this->Point1Representation ||
      !this->Point2Representation)
    {
    return;
    }

  // get the end points
  double p1[3], p2[3];
  this->Point1Representation->GetWorldPosition(p1);
  this->Point2Representation->GetWorldPosition(p2);
  double pos[3];

  pos[0] = p1[0] + (p2[0] - p1[0]) * this->LabelPosition;
  pos[1] = p1[1] + (p2[1] - p1[1]) * this->LabelPosition;
  pos[2] = p1[2] + (p2[2] - p1[2]) * this->LabelPosition;

  // and set it on the actor
  double * actorPos = this->LabelActor->GetPosition();
  double diff = sqrt(vtkMath::Distance2BetweenPoints(pos, actorPos));
  if (diff > 0.001)
    {
    this->LabelActor->SetPosition(pos);
    }
 }

//----------------------------------------------------------------------
void vtkDistanceRepresentation3D::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Distance: " << this->Distance << endl;
  os << indent << "Label Scale Specified: " << (this->LabelScaleSpecified ? "true" : "false") << endl;
  os << indent << "Label Position: " << this->LabelPosition << endl;
  os << indent << "Maximum Number Of Ticks: " << this->MaximumNumberOfRulerTicks << endl;
  os << indent << "Glyph Scale: " << this->GlyphScale << endl;
  os << indent << "LabelActor: " << this->LabelActor << endl;
  os << indent << "GlyphActor: " << this->GlyphActor << endl;
}
