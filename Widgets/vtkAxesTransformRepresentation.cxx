/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAxesTransformRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAxesTransformRepresentation.h"
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

vtkStandardNewMacro(vtkAxesTransformRepresentation);

//----------------------------------------------------------------------
vtkAxesTransformRepresentation::vtkAxesTransformRepresentation()
{
  // By default, use one of these handles
  this->OriginRepresentation  = vtkPointHandleRepresentation3D::New();
  this->SelectionRepresentation  = vtkPointHandleRepresentation3D::New();

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

  // The bounding box
  this->BoundingBox = vtkBox::New();

  this->LabelFormat = NULL;

  this->Tolerance = 1;

  this->InteractionState = Outside;
}

//----------------------------------------------------------------------
vtkAxesTransformRepresentation::~vtkAxesTransformRepresentation()
{
  this->OriginRepresentation->Delete();
  this->SelectionRepresentation->Delete();

  this->LinePoints->Delete();
  this->LinePolyData->Delete();
  this->LineMapper->Delete();
  this->LineActor->Delete();

  this->LabelText->Delete();
  this->LabelMapper->Delete();
  this->LabelActor->Delete();

  if (this->LabelFormat)
    {
    delete [] this->LabelFormat;
    this->LabelFormat = NULL;
    }

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
void vtkAxesTransformRepresentation::GetOriginWorldPosition(double pos[3])
{
  this->OriginRepresentation->GetWorldPosition(pos);
}

//----------------------------------------------------------------------
double* vtkAxesTransformRepresentation::GetOriginWorldPosition()
{
  if (!this->OriginRepresentation)
    {
    static double temp[3]=  {0, 0, 0};
    return temp;
    }
  return this->OriginRepresentation->GetWorldPosition();
}

//----------------------------------------------------------------------
void vtkAxesTransformRepresentation::SetOriginDisplayPosition(double x[3])
{
  this->OriginRepresentation->SetDisplayPosition(x);
  double p[3];
  this->OriginRepresentation->GetWorldPosition(p);
  this->OriginRepresentation->SetWorldPosition(p);
}

//----------------------------------------------------------------------
void vtkAxesTransformRepresentation::SetOriginWorldPosition(double x[3])
{
  if (this->OriginRepresentation)
    {
    this->OriginRepresentation->SetWorldPosition(x);
    }
}

//----------------------------------------------------------------------
void vtkAxesTransformRepresentation::GetOriginDisplayPosition(double pos[3])
{
  this->OriginRepresentation->GetDisplayPosition(pos);
  pos[2] = 0.0;
}

//----------------------------------------------------------------------
double *vtkAxesTransformRepresentation::GetBounds()
{
  this->BuildRepresentation();

  this->BoundingBox->SetBounds(this->OriginRepresentation->GetBounds());
  this->BoundingBox->AddBounds(this->SelectionRepresentation->GetBounds());
  this->BoundingBox->AddBounds(this->LineActor->GetBounds());

  return this->BoundingBox->GetBounds();
}

//----------------------------------------------------------------------
void vtkAxesTransformRepresentation::StartWidgetInteraction(double e[2])
{
  // Store the start position
  this->StartEventPosition[0] = e[0];
  this->StartEventPosition[1] = e[1];
  this->StartEventPosition[2] = 0.0;

  // Store the start position
  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;

  // Get the coordinates of the three handles
//   this->OriginRepresentation->GetWorldPosition(this->StartP1);
//   this->SelectionRepresentation->GetWorldPosition(this->StartP2);
}

//----------------------------------------------------------------------
void vtkAxesTransformRepresentation::WidgetInteraction(double e[2])
{

  // Store the start position
  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
}

//----------------------------------------------------------------------------
int vtkAxesTransformRepresentation::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
  // Check if we are on the origin. Use the handle to determine this.
  int p1State = this->OriginRepresentation->ComputeInteractionState(X,Y,0);

  if ( p1State == vtkHandleRepresentation::Nearby )
    {
    this->InteractionState = vtkAxesTransformRepresentation::OnOrigin;
    }
  else
    {
    this->InteractionState = vtkAxesTransformRepresentation::Outside;
    }

  // Okay if we're near a handle return, otherwise test the line
  if ( this->InteractionState != vtkAxesTransformRepresentation::Outside )
    {
    return this->InteractionState;
    }

  return this->InteractionState;
}

//----------------------------------------------------------------------
void vtkAxesTransformRepresentation::BuildRepresentation()
{
  if ( this->GetMTime() > this->BuildTime ||
       this->OriginRepresentation->GetMTime() > this->BuildTime ||
       this->SelectionRepresentation->GetMTime() > this->BuildTime ||
       (this->Renderer && this->Renderer->GetVTKWindow() &&
        this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime) )
    {

    this->BuildTime.Modified();
    }
}

//----------------------------------------------------------------------
void vtkAxesTransformRepresentation::
ReleaseGraphicsResources(vtkWindow *w)
{
  this->LineActor->ReleaseGraphicsResources(w);
  this->LabelActor->ReleaseGraphicsResources(w);
  this->GlyphActor->ReleaseGraphicsResources(w);
}


//----------------------------------------------------------------------
int vtkAxesTransformRepresentation::
RenderOpaqueGeometry(vtkViewport *v)
{
  this->BuildRepresentation();

  this->LineActor->RenderOpaqueGeometry(v);
  this->LabelActor->RenderOpaqueGeometry(v);
  this->GlyphActor->RenderOpaqueGeometry(v);

  return 3;
}

//----------------------------------------------------------------------
int vtkAxesTransformRepresentation::
RenderTranslucentPolygonalGeometry(vtkViewport *v)
{
  this->BuildRepresentation();

  this->LineActor->RenderTranslucentPolygonalGeometry(v);
  this->LabelActor->RenderTranslucentPolygonalGeometry(v);
  this->GlyphActor->RenderTranslucentPolygonalGeometry(v);

  return 3;
}

//----------------------------------------------------------------------
void vtkAxesTransformRepresentation::SetLabelScale( double scale[3] )
{
  this->LabelActor->SetScale( scale );
}

//----------------------------------------------------------------------
double * vtkAxesTransformRepresentation::GetLabelScale()
{
  return this->LabelActor->GetScale();
}

//----------------------------------------------------------------------------
vtkProperty * vtkAxesTransformRepresentation::GetLabelProperty()
{
  return this->LabelActor->GetProperty();
}

//----------------------------------------------------------------------
void vtkAxesTransformRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Label Format: ";
  if ( this->LabelFormat )
    {
    os << this->LabelFormat << endl;
    }
  else
    {
    os << "(none)\n";
    }

  os << indent << "Tolerance: " << this->Tolerance << endl;
  os << indent << "InteractionState: " << this->InteractionState << endl;

  os << indent << "Origin Representation: ";
  if ( this->OriginRepresentation )
    {
    this->OriginRepresentation->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << "(none)\n";
    }

  os << indent << "Selection Representation: " << endl;
  if ( this->SelectionRepresentation )
    {
    this->SelectionRepresentation->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << "(none)\n";
    }

  this->Superclass::PrintSelf(os,indent);
}
