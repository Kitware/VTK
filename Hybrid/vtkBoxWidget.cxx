/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoxWidget.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBoxWidget.h"
#include "vtkMath.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkSphereSource.h"
#include "vtkFloatArray.h"
#include "vtkCellPicker.h"
#include "vtkAssemblyNode.h"
#include "vtkDoubleArray.h"
#include "vtkPlanes.h"
#include "vtkCallbackCommand.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkBoxWidget, "1.10");
vtkStandardNewMacro(vtkBoxWidget);

vtkBoxWidget::vtkBoxWidget()
{
  this->EventCallbackCommand->SetCallback(vtkBoxWidget::ProcessEvents);
  
  // Enable/disable the translation, rotation, and scaling of the widget
  this->TranslationEnabled = 1;
  this->RotationEnabled = 1;
  this->ScalingEnabled = 1;

  //Build the representation of the widget
  int i;

  // Control orientation of normals
  this->InsideOut = 0;
  this->OutlineFaceWires = 0;
  this->OutlineCursorWires = 1;

  // Construct the poly data representing the hex
  this->HexPolyData = vtkPolyData::New();
  this->HexMapper = vtkPolyDataMapper::New();
  this->HexMapper->SetInput(HexPolyData);
  this->HexActor = vtkActor::New();
  this->HexActor->SetMapper(this->HexMapper);

  // Construct initial points
  this->Points = vtkPoints::New(VTK_DOUBLE);
  this->Points->SetNumberOfPoints(15);//8 corners; 6 faces; 1 center
  this->HexPolyData->SetPoints(this->Points);
  
  // Construct connectivity for the faces. These are used to perform
  // the picking.
  vtkIdType pts[4];
  vtkCellArray *cells = vtkCellArray::New();
  cells->Allocate(cells->EstimateSize(6,4));
  pts[0] = 3; pts[1] = 0; pts[2] = 4; pts[3] = 7;
  cells->InsertNextCell(4,pts);
  pts[0] = 1; pts[1] = 2; pts[2] = 6; pts[3] = 5;
  cells->InsertNextCell(4,pts);
  pts[0] = 0; pts[1] = 1; pts[2] = 5; pts[3] = 4;
  cells->InsertNextCell(4,pts);
  pts[0] = 2; pts[1] = 3; pts[2] = 7; pts[3] = 6;
  cells->InsertNextCell(4,pts);
  pts[0] = 0; pts[1] = 3; pts[2] = 2; pts[3] = 1;
  cells->InsertNextCell(4,pts);
  pts[0] = 4; pts[1] = 5; pts[2] = 6; pts[3] = 7;
  cells->InsertNextCell(4,pts);
  this->HexPolyData->SetPolys(cells);
  cells->Delete();
  this->HexPolyData->BuildCells();
  
  // The face of the hexahedra
  cells = vtkCellArray::New();
  cells->Allocate(cells->EstimateSize(1,4));
  cells->InsertNextCell(4,pts); //temporary, replaced later
  this->HexFacePolyData = vtkPolyData::New();
  this->HexFacePolyData->SetPoints(this->Points);
  this->HexFacePolyData->SetPolys(cells);
  this->HexFaceMapper = vtkPolyDataMapper::New();
  this->HexFaceMapper->SetInput(HexFacePolyData);
  this->HexFace = vtkActor::New();
  this->HexFace->SetMapper(this->HexFaceMapper);
  cells->Delete();

  // Create the outline for the hex
  this->OutlinePolyData = vtkPolyData::New();
  this->OutlinePolyData->SetPoints(this->Points);
  this->OutlineMapper = vtkPolyDataMapper::New();
  this->OutlineMapper->SetInput(this->OutlinePolyData);
  this->HexOutline = vtkActor::New();
  this->HexOutline->SetMapper(this->OutlineMapper);
  cells = vtkCellArray::New();
  cells->Allocate(cells->EstimateSize(15,2));
  this->OutlinePolyData->SetLines(cells);
  cells->Delete();

  // Set up the initial properties
  this->HandleProperty = NULL;
  this->SelectedHandleProperty = NULL;
  this->FaceProperty = NULL;
  this->SelectedFaceProperty = NULL;
  this->OutlineProperty = NULL;
  this->SelectedOutlineProperty = NULL;
  this->CreateDefaultProperties();

  this->GenerateOutline();

  // Create the handles
  this->Handle = new vtkActor* [7];
  this->HandleMapper = new vtkPolyDataMapper* [7];
  this->HandleGeometry = new vtkSphereSource* [7];
  for (i=0; i<7; i++)
    {
    this->HandleGeometry[i] = vtkSphereSource::New();
    this->HandleGeometry[i]->SetThetaResolution(16);
    this->HandleGeometry[i]->SetPhiResolution(8);
    this->HandleMapper[i] = vtkPolyDataMapper::New();
    this->HandleMapper[i]->SetInput(this->HandleGeometry[i]->GetOutput());
    this->Handle[i] = vtkActor::New();
    this->Handle[i]->SetMapper(this->HandleMapper[i]);
    }
  
  // Define the point coordinates
  float bounds[6];
  bounds[0] = -0.5;
  bounds[1] = 0.5;
  bounds[2] = -0.5;
  bounds[3] = 0.5;
  bounds[4] = -0.5;
  bounds[5] = 0.5;
  // Points 8-14 are down by PositionHandles();
  this->PlaceWidget(bounds);

  //Manage the picking stuff
  this->HandlePicker = vtkCellPicker::New();
  this->HandlePicker->SetTolerance(0.001);
  for (i=0; i<7; i++)
    {
    this->HandlePicker->AddPickList(this->Handle[i]);
    }
  this->HandlePicker->PickFromListOn();

  this->HexPicker = vtkCellPicker::New();
  this->HexPicker->SetTolerance(0.001);
  this->HexPicker->AddPickList(HexActor);
  this->HexPicker->PickFromListOn();
  
  this->CurrentHandle = NULL;

  this->Transform = vtkTransform::New();
 
}

vtkBoxWidget::~vtkBoxWidget()
{
  this->HexActor->Delete();
  this->HexMapper->Delete();
  this->HexPolyData->Delete();
  this->Points->Delete();

  this->HexFace->Delete();
  this->HexFaceMapper->Delete();
  this->HexFacePolyData->Delete();

  this->HexOutline->Delete();
  this->OutlineMapper->Delete();
  this->OutlinePolyData->Delete();
  
  for (int i=0; i<7; i++)
    {
    this->HandleGeometry[i]->Delete();
    this->HandleMapper[i]->Delete();
    this->Handle[i]->Delete();
    }
  delete [] this->Handle;
  delete [] this->HandleMapper;
  delete [] this->HandleGeometry;
  
  this->HandlePicker->Delete();
  this->HexPicker->Delete();

  this->Transform->Delete();
  
  if ( this->HandleProperty )
    {
    this->HandleProperty->Delete();
    }
  if ( this->SelectedHandleProperty )
    {
    this->SelectedHandleProperty->Delete();
    }
  if ( this->FaceProperty )
    {
    this->FaceProperty->Delete();
    }
  if ( this->SelectedFaceProperty )
    {
    this->SelectedFaceProperty->Delete();
    }
  if ( this->OutlineProperty )
    {
    this->OutlineProperty->Delete();
    }
  if ( this->SelectedOutlineProperty )
    {
    this->SelectedOutlineProperty->Delete();
    }
}

void vtkBoxWidget::SetEnabled(int enabling)
{
  if ( ! this->Interactor )
    {
    vtkErrorMacro(<<"The interactor must be set prior to enabling/disabling widget");
    return;
    }

  if ( enabling ) //------------------------------------------------------------
    {
    vtkDebugMacro(<<"Enabling widget");

    if ( this->Enabled ) //already enabled, just return
      {
      return;
      }
    
    this->CurrentRenderer = 
      this->Interactor->FindPokedRenderer(this->Interactor->GetLastEventPosition()[0],
                                          this->Interactor->GetLastEventPosition()[1]);
    if (this->CurrentRenderer == NULL)
      {
      return;
      }

    this->Enabled = 1;

    // listen to the following events
    vtkRenderWindowInteractor *i = this->Interactor;
    i->AddObserver(vtkCommand::MouseMoveEvent, this->EventCallbackCommand, 
                   this->Priority);
    i->AddObserver(vtkCommand::LeftButtonPressEvent, this->EventCallbackCommand,
                   this->Priority);
    i->AddObserver(vtkCommand::LeftButtonReleaseEvent, this->EventCallbackCommand,
                   this->Priority);
    i->AddObserver(vtkCommand::MiddleButtonPressEvent, this->EventCallbackCommand,
                   this->Priority);
    i->AddObserver(vtkCommand::MiddleButtonReleaseEvent, this->EventCallbackCommand,
                   this->Priority);
    i->AddObserver(vtkCommand::RightButtonPressEvent, this->EventCallbackCommand,
                   this->Priority);
    i->AddObserver(vtkCommand::RightButtonReleaseEvent, this->EventCallbackCommand,
                   this->Priority);

    // Add the various actors
    // Add the outline
    this->CurrentRenderer->AddActor(this->HexActor);
    this->CurrentRenderer->AddActor(this->HexOutline);
    this->HexActor->SetProperty(this->OutlineProperty);
    this->HexOutline->SetProperty(this->OutlineProperty);

    // Add the hex face
    this->CurrentRenderer->AddActor(this->HexFace);
    this->HexFace->SetProperty(this->FaceProperty);

    // turn on the handles
    for (int j=0; j<7; j++)
      {
      this->CurrentRenderer->AddActor(this->Handle[j]);
      this->Handle[j]->SetProperty(this->HandleProperty);
      }

    this->InvokeEvent(vtkCommand::EnableEvent,NULL);
    }

  else //disabling-------------------------------------------------------------
    {
    vtkDebugMacro(<<"Disabling widget");

    if ( ! this->Enabled ) //already disabled, just return
      {
      return;
      }
    
    this->Enabled = 0;

    // don't listen for events any more
    this->Interactor->RemoveObserver(this->EventCallbackCommand);

    // turn off the outline
    this->CurrentRenderer->RemoveActor(this->HexActor);
    this->CurrentRenderer->RemoveActor(this->HexOutline);

    // turn off the hex face
    this->CurrentRenderer->RemoveActor(this->HexFace);

    // turn off the handles
    for (int i=0; i<7; i++)
      {
      this->CurrentRenderer->RemoveActor(this->Handle[i]);
      }

    this->CurrentHandle = NULL;
    this->InvokeEvent(vtkCommand::DisableEvent,NULL);
    }
  
  this->Interactor->Render();
}

void vtkBoxWidget::ProcessEvents(vtkObject* object, unsigned long event,
                                 void* clientdata, void* vtkNotUsed(calldata))
{
  vtkBoxWidget* self = reinterpret_cast<vtkBoxWidget *>( clientdata );
  vtkRenderWindowInteractor* rwi = static_cast<vtkRenderWindowInteractor *>( object );
  int* XY = rwi->GetEventPosition();

  //okay, let's do the right thing
  switch(event)
    {
    case vtkCommand::LeftButtonPressEvent:
      self->OnLeftButtonDown(rwi->GetControlKey(), rwi->GetShiftKey(), XY[0], XY[1]);
      break;
    case vtkCommand::LeftButtonReleaseEvent:
      self->OnLeftButtonUp(rwi->GetControlKey(), rwi->GetShiftKey(), XY[0], XY[1]);
      break;
    case vtkCommand::MiddleButtonPressEvent:
      self->OnMiddleButtonDown(rwi->GetControlKey(), rwi->GetShiftKey(), XY[0], XY[1]);
      break;
    case vtkCommand::MiddleButtonReleaseEvent:
      self->OnMiddleButtonUp(rwi->GetControlKey(), rwi->GetShiftKey(), XY[0], XY[1]);
      break;
    case vtkCommand::RightButtonPressEvent:
      self->OnRightButtonDown(rwi->GetControlKey(), rwi->GetShiftKey(), XY[0], XY[1]);
      break;
    case vtkCommand::RightButtonReleaseEvent:
      self->OnRightButtonUp(rwi->GetControlKey(), rwi->GetShiftKey(), XY[0], XY[1]);
      break;
    case vtkCommand::MouseMoveEvent:
      self->OnMouseMove(rwi->GetControlKey(), rwi->GetShiftKey(), XY[0], XY[1]);
      break;
    }
}

void vtkBoxWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->HandleProperty )
    {
    os << indent << "Handle Property: " << this->HandleProperty << "\n";
    }
  else
    {
    os << indent << "Handle Property: (none)\n";
    }
  if ( this->SelectedHandleProperty )
    {
    os << indent << "Selected Handle Property: " 
       << this->SelectedHandleProperty << "\n";
    }
  else
    {
    os << indent << "SelectedHandle Property: (none)\n";
    }

  if ( this->FaceProperty )
    {
    os << indent << "Face Property: " << this->FaceProperty << "\n";
    }
  else
    {
    os << indent << "Face Property: (none)\n";
    }
  if ( this->SelectedFaceProperty )
    {
    os << indent << "Selected Face Property: " 
       << this->SelectedFaceProperty << "\n";
    }
  else
    {
    os << indent << "Selected Face Property: (none)\n";
    }

  if ( this->OutlineProperty )
    {
    os << indent << "Outline Property: " << this->OutlineProperty << "\n";
    }
  else
    {
    os << indent << "Outline Property: (none)\n";
    }
  if ( this->SelectedOutlineProperty )
    {
    os << indent << "Selected Outline Property: " 
       << this->SelectedOutlineProperty << "\n";
    }
  else
    {
    os << indent << "Selected Outline Property: (none)\n";
    }

  os << indent << "Outline Face Wires: " << (this->OutlineFaceWires ? "On\n" : "Off\n");
  os << indent << "Outline Cursor Wires: " << (this->OutlineCursorWires ? "On\n" : "Off\n");
  os << indent << "Inside Out: " << (this->InsideOut ? "On\n" : "Off\n");
  os << indent << "Translation Enabled: " << (this->TranslationEnabled ? "On\n" : "Off\n");
  os << indent << "Scaling Enabled: " << (this->ScalingEnabled ? "On\n" : "Off\n");
  os << indent << "Rotation Enabled: " << (this->RotationEnabled ? "On\n" : "Off\n");
  
}

#define VTK_AVERAGE(a,b,c) \
  c[0] = (a[0] + b[0])/2.0; \
  c[1] = (a[1] + b[1])/2.0; \
  c[2] = (a[2] + b[2])/2.0;

void vtkBoxWidget::PositionHandles()
{
  double *pts = ((vtkDoubleArray *)this->Points->GetData())->GetPointer(0);
  double *p0 = pts;
  double *p1 = pts + 3*1;
  double *p2 = pts + 3*2;
  double *p3 = pts + 3*3;
  //double *p4 = pts + 3*4;
  double *p5 = pts + 3*5;
  double *p6 = pts + 3*6;
  double *p7 = pts + 3*7;
  double x[3];

  VTK_AVERAGE(p0,p7,x);
  this->Points->SetPoint(8, x);
  VTK_AVERAGE(p1,p6,x);
  this->Points->SetPoint(9, x);
  VTK_AVERAGE(p0,p5,x);
  this->Points->SetPoint(10, x);
  VTK_AVERAGE(p2,p7,x);
  this->Points->SetPoint(11, x);
  VTK_AVERAGE(p1,p3,x);
  this->Points->SetPoint(12, x);
  VTK_AVERAGE(p5,p7,x);
  this->Points->SetPoint(13, x);
  VTK_AVERAGE(p0,p6,x);
  this->Points->SetPoint(14, x);

  this->HandleGeometry[0]->SetCenter(this->Points->GetPoint(8));
  this->HandleGeometry[1]->SetCenter(this->Points->GetPoint(9));
  this->HandleGeometry[2]->SetCenter(this->Points->GetPoint(10));
  this->HandleGeometry[3]->SetCenter(this->Points->GetPoint(11));
  this->HandleGeometry[4]->SetCenter(this->Points->GetPoint(12));
  this->HandleGeometry[5]->SetCenter(this->Points->GetPoint(13));
  this->HandleGeometry[6]->SetCenter(this->Points->GetPoint(14));
 
  this->HexFacePolyData->Modified();
  this->HexPolyData->Modified();
  this->GenerateOutline();
}
#undef VTK_AVERAGE

int vtkBoxWidget::HighlightHandle(vtkProp *prop)
{
  // first unhighlight anything picked
  this->HighlightOutline(0);
  if ( this->CurrentHandle )
    {
    this->CurrentHandle->SetProperty(this->HandleProperty);
    }

  this->CurrentHandle = (vtkActor *)prop;

  if ( this->CurrentHandle )
    {
    this->CurrentHandle->SetProperty(this->SelectedHandleProperty);
    for (int i=0; i<6; i++) //find attached face
      {
      if ( this->CurrentHandle == this->Handle[i] )
        {
        return i;
        }
      }
    }
  
  if ( this->CurrentHandle == this->Handle[6] )
    {
    this->HighlightOutline(1);
    }
  
  return -1;
}

void vtkBoxWidget::HighlightFace(int cellId)
{
  if ( cellId >= 0 )
    {
    vtkIdType npts;
    vtkIdType *pts;
    vtkCellArray *cells = this->HexFacePolyData->GetPolys();
    this->HexPolyData->GetCellPoints(cellId, npts, pts);
    this->HexFacePolyData->Modified();
    cells->ReplaceCell(0,npts,pts);
    this->CurrentHexFace = cellId;
    this->HexFace->SetProperty(this->SelectedFaceProperty);
    if ( !this->CurrentHandle )
      {
      this->CurrentHandle = this->HexFace;
      }
    }
  else
    {
    this->HexFace->SetProperty(this->FaceProperty);
    this->CurrentHexFace = -1;
    }
}

void vtkBoxWidget::HighlightOutline(int highlight)
{
  if ( highlight )
    {
    this->HexActor->SetProperty(this->SelectedOutlineProperty);
    this->HexOutline->SetProperty(this->SelectedOutlineProperty);
    }
  else
    {
    this->HexActor->SetProperty(this->OutlineProperty);
    this->HexOutline->SetProperty(this->OutlineProperty);
    }
}

void vtkBoxWidget::OnLeftButtonDown (int vtkNotUsed(ctrl), int shift, 
                                     int X, int Y)
{
  this->State = vtkBoxWidget::Moving;

  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then pick the bounding box.
  vtkAssemblyPath *path;
  this->Interactor->FindPokedRenderer(X,Y);
  this->HandlePicker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->HandlePicker->GetPath();
  if ( path != NULL )
    {
    this->HighlightFace(
      this->HighlightHandle(path->GetFirstNode()->GetProp()));
    }
  else
    {
    this->HexPicker->Pick(X,Y,0.0,this->CurrentRenderer);
    path = this->HexPicker->GetPath();
    if ( path != NULL )
      {
      if ( !shift )
        {
        this->HighlightHandle(NULL);
        this->HighlightFace(this->HexPicker->GetCellId());
        }
      else
        {
        this->CurrentHandle = this->Handle[6];
        this->HighlightOutline(1);
        }
      }
    else
      {
      this->HighlightFace(this->HighlightHandle(NULL));
      this->State = vtkBoxWidget::Outside;
      return;
      }
    }
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkBoxWidget::OnLeftButtonUp (int vtkNotUsed(ctrl), int vtkNotUsed(shift),
                                   int vtkNotUsed(X), int vtkNotUsed(Y))
{
  if ( this->State == vtkBoxWidget::Outside )
    {
    return;
    }

  this->State = vtkBoxWidget::Start;
  this->HighlightFace(this->HighlightHandle(NULL));

  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
  
}

void vtkBoxWidget::OnMiddleButtonDown (int vtkNotUsed(ctrl), 
                                       int vtkNotUsed(shift), int X, int Y)
{
  this->State = vtkBoxWidget::Moving;

  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then pick the bounding box.
  vtkAssemblyPath *path;
  this->Interactor->FindPokedRenderer(X,Y);
  this->HandlePicker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->HandlePicker->GetPath();
  if ( path != NULL )
    {
    this->CurrentHandle = this->Handle[6];
    this->HighlightOutline(1);
    }
  else
    {
    this->HexPicker->Pick(X,Y,0.0,this->CurrentRenderer);
    path = this->HexPicker->GetPath();
    if ( path != NULL )
      {
      this->CurrentHandle = this->Handle[6];
      this->HighlightOutline(1);
      }
    else
      {
      this->HighlightFace(this->HighlightHandle(NULL));
      this->State = vtkBoxWidget::Outside;
      return;
      }
    }
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkBoxWidget::OnMiddleButtonUp (int vtkNotUsed(ctrl), 
                                     int vtkNotUsed(shift), int vtkNotUsed(X), 
                                     int vtkNotUsed(Y))
{
  if ( this->State == vtkBoxWidget::Outside )
    {
    return;
    }

  this->State = vtkBoxWidget::Start;
  this->HighlightFace(this->HighlightHandle(NULL));

  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
  
}

void vtkBoxWidget::OnRightButtonDown (int vtkNotUsed(ctrl), 
                                      int vtkNotUsed(shift), int X, int Y)
{
  this->State = vtkBoxWidget::Scaling;

  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then pick the bounding box.
  vtkAssemblyPath *path;
  this->Interactor->FindPokedRenderer(X,Y);
  this->HandlePicker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->HandlePicker->GetPath();
  if ( path == NULL )
    {
    this->HexPicker->Pick(X,Y,0.0,this->CurrentRenderer);
    path = this->HexPicker->GetPath();
    if ( path == NULL )
      {
      this->State = vtkBoxWidget::Outside;
      this->HighlightOutline(0);
      return;
      }
    else
      {
      this->HighlightOutline(1);
      }
    }
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkBoxWidget::OnRightButtonUp (int vtkNotUsed(ctrl), 
                                    int vtkNotUsed(shift), int vtkNotUsed(X), 
                                    int vtkNotUsed(Y))
{
  if ( this->State == vtkBoxWidget::Outside )
    {
    return;
    }

  this->State = vtkBoxWidget::Start;
  this->HighlightOutline(0);
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkBoxWidget::OnMouseMove (int vtkNotUsed(ctrl), int vtkNotUsed(shift), 
                                int X, int Y)
{
  // See whether we're active
  if ( this->State == vtkBoxWidget::Outside || this->State == vtkBoxWidget::Start )
    {
    return;
    }
  
  // Do different things depending on state
  // Calculations everybody does
  double focalPoint[4], pickPoint[4], prevPickPoint[4];
  double z, vpn[3];

  this->CurrentCamera = this->Interactor->FindPokedCamera(X,Y);
  if ( !this->CurrentCamera )
    {
    return;
    }

  // Compute the two points defining the motion vector
  this->CurrentCamera->GetFocalPoint(focalPoint);
  this->ComputeWorldToDisplay(focalPoint[0], focalPoint[1],
                              focalPoint[2], focalPoint);
  z = focalPoint[2];
  this->ComputeDisplayToWorld(double(this->Interactor->GetLastEventPosition()[0]),
                              double(this->Interactor->GetLastEventPosition()[1]),
                              z, 
                              prevPickPoint);
  this->ComputeDisplayToWorld(double(X), double(Y), z, pickPoint);

  // Process the motion
  if ( this->State == vtkBoxWidget::Moving )
    {
    // Okay to process
    if ( this->CurrentHandle )
      {
      if ( this->RotationEnabled && this->CurrentHandle == this->HexFace )
        {
        this->CurrentCamera->GetViewPlaneNormal(vpn);
        this->Rotate(X, Y, prevPickPoint, pickPoint, vpn);
        }
      else if ( this->TranslationEnabled && this->CurrentHandle == this->Handle[6] )
        {
        this->Translate(prevPickPoint, pickPoint);
        }
      else if ( this->TranslationEnabled && this->ScalingEnabled ) 
        {
        if ( this->CurrentHandle == this->Handle[0] )
          {
          this->MoveMinusXFace(prevPickPoint, pickPoint);
          }
        else if ( this->CurrentHandle == this->Handle[1] )
          {
          this->MovePlusXFace(prevPickPoint, pickPoint);
          }
        else if ( this->CurrentHandle == this->Handle[2] )
          {
          this->MoveMinusYFace(prevPickPoint, pickPoint);
          }
        else if ( this->CurrentHandle == this->Handle[3] )
          {
          this->MovePlusYFace(prevPickPoint, pickPoint);
          }
        else if ( this->CurrentHandle == this->Handle[4] )
          {
          this->MoveMinusZFace(prevPickPoint, pickPoint);
          }
        else if ( this->CurrentHandle == this->Handle[5] )
          {
          this->MovePlusZFace(prevPickPoint, pickPoint);
          }
        }
      }
    }
  else if ( this->ScalingEnabled && this->State == vtkBoxWidget::Scaling )
    {
    this->Scale(prevPickPoint, pickPoint, X, Y);
    }

  // Interact, if desired
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  
  this->Interactor->Render();
}

void vtkBoxWidget::MoveFace(double *p1, double *p2,
                            double *h1, double *h2,
                            double *x1, double *x2,
                            double *x3, double *x4,
                            double *x5)
{
  int i;
  double v[3], v2[3];

  for (i=0; i<3; i++)
    {
    v[i] = p2[i] - p1[i];
    v2[i] = h2[i] - h1[i];
    }

  vtkMath::Normalize(v2);
  double f = vtkMath::Dot(v,v2);

  for (i=0; i<3; i++)
    {
    v[i] = f*v2[i];
  
    x1[i] += v[i];
    x2[i] += v[i];
    x3[i] += v[i];
    x4[i] += v[i];
    x5[i] += v[i];
    }
  this->PositionHandles();
}

void vtkBoxWidget::MovePlusXFace(double *p1, double *p2)
{
  double *pts = ((vtkDoubleArray *)this->Points->GetData())->GetPointer(0);

  double *h1 = pts + 3*9;
  double *h2 = pts + 3*8;

  double *x1 = pts + 3*1;
  double *x2 = pts + 3*2;
  double *x3 = pts + 3*5;
  double *x4 = pts + 3*6;
  
  this->MoveFace(p1, p2, h1, h2, x1, x2, x3, x4, h1);
}

void vtkBoxWidget::MoveMinusXFace(double *p1, double *p2)
{
  double *pts = ((vtkDoubleArray *)this->Points->GetData())->GetPointer(0);

  double *h1 = pts + 3*8;
  double *h2 = pts + 3*9;

  double *x1 = pts + 3*0;
  double *x2 = pts + 3*3;
  double *x3 = pts + 3*4;
  double *x4 = pts + 3*7;
  
  this->MoveFace(p1, p2, h1, h2, x1, x2, x3, x4, h1);
}

void vtkBoxWidget::MovePlusYFace(double *p1, double *p2)
{
  double *pts = ((vtkDoubleArray *)this->Points->GetData())->GetPointer(0);

  double *h1 = pts + 3*11;
  double *h2 = pts + 3*10;

  double *x1 = pts + 3*2;
  double *x2 = pts + 3*3;
  double *x3 = pts + 3*6;
  double *x4 = pts + 3*7;
  
  this->MoveFace(p1, p2, h1, h2, x1, x2, x3, x4, h1);
}

void vtkBoxWidget::MoveMinusYFace(double *p1, double *p2)
{
  double *pts = ((vtkDoubleArray *)this->Points->GetData())->GetPointer(0);

  double *h1 = pts + 3*10;
  double *h2 = pts + 3*11;

  double *x1 = pts + 3*0;
  double *x2 = pts + 3*1;
  double *x3 = pts + 3*4;
  double *x4 = pts + 3*5;
  
  this->MoveFace(p1, p2, h1, h2, x1, x2, x3, x4, h1);
}

void vtkBoxWidget::MovePlusZFace(double *p1, double *p2)
{
  double *pts = ((vtkDoubleArray *)this->Points->GetData())->GetPointer(0);

  double *h1 = pts + 3*13;
  double *h2 = pts + 3*12;

  double *x1 = pts + 3*4;
  double *x2 = pts + 3*5;
  double *x3 = pts + 3*6;
  double *x4 = pts + 3*7;
  
  this->MoveFace(p1, p2, h1, h2, x1, x2, x3, x4, h1);
}

void vtkBoxWidget::MoveMinusZFace(double *p1, double *p2)
{
  double *pts = ((vtkDoubleArray *)this->Points->GetData())->GetPointer(0);

  double *h1 = pts + 3*12;
  double *h2 = pts + 3*13;

  double *x1 = pts + 3*0;
  double *x2 = pts + 3*1;
  double *x3 = pts + 3*2;
  double *x4 = pts + 3*3;
  
  this->MoveFace(p1, p2, h1, h2, x1, x2, x3, x4, h1);
}

// Loop through all points and translate them
void vtkBoxWidget::Translate(double *p1, double *p2)
{
  double *pts = ((vtkDoubleArray *)this->Points->GetData())->GetPointer(0);
  double v[3];

  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];
  
  // Move the corners
  for (int i=0; i<8; i++)
    {
    *pts++ += v[0];
    *pts++ += v[1];
    *pts++ += v[2];
    }
  this->PositionHandles();
}

void vtkBoxWidget::Scale(double* vtkNotUsed(p1), double* vtkNotUsed(p2), 
                         int vtkNotUsed(X), int Y)
{
  double *pts = ((vtkDoubleArray *)this->Points->GetData())->GetPointer(0);
  double *center 
    = ((vtkDoubleArray *)this->Points->GetData())->GetPointer(3*14);
  double sf;

  if ( Y > this->Interactor->GetLastEventPosition()[1] )
    {
    sf = 1.03;
    }
  else
    {
    sf = 0.97;
    }
  
  // Move the corners
  for (int i=0; i<8; i++, pts+=3)
    {
    pts[0] = sf * (pts[0] - center[0]) + center[0];
    pts[1] = sf * (pts[1] - center[1]) + center[1];
    pts[2] = sf * (pts[2] - center[2]) + center[2];
    }
  this->PositionHandles();
}

void vtkBoxWidget::ComputeNormals()
{
  double *pts = ((vtkDoubleArray *)this->Points->GetData())->GetPointer(0);
  double *p0 = pts;
  double *px = pts + 3*1;
  double *py = pts + 3*3;
  double *pz = pts + 3*4;
  int i;
  
  for (i=0; i<3; i++)
    {
    this->N[0][i] = p0[i] - px[i];
    this->N[2][i] = p0[i] - py[i];
    this->N[4][i] = p0[i] - pz[i];
    }
  vtkMath::Normalize(this->N[0]);
  vtkMath::Normalize(this->N[2]);
  vtkMath::Normalize(this->N[4]);
  for (i=0; i<3; i++)
    {
    this->N[1][i] = -this->N[0][i];
    this->N[3][i] = -this->N[2][i];
    this->N[5][i] = -this->N[4][i];
    }
}

void vtkBoxWidget::GetPlanes(vtkPlanes *planes)
{
  if ( ! planes )
    {
    return;
    }
  
  this->ComputeNormals();

  vtkPoints *pts = vtkPoints::New(VTK_DOUBLE);
  pts->SetNumberOfPoints(6);
  
  vtkDoubleArray *normals = vtkDoubleArray::New();
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(6);
  
  planes->SetPoints(pts);
  planes->SetNormals(normals);
  
  // Set the normals and coordinate values
  float factor = (this->InsideOut ? -1.0 : 1.0);
  for (int i=0; i<6; i++)
    {
    pts->SetPoint(i,this->Points->GetPoint(8+i));
    normals->SetTuple3(i, factor*this->N[i][0], factor*this->N[i][1], 
                       factor*this->N[i][2]);
    }
  
  pts->Delete();
  normals->Delete();
}

void vtkBoxWidget::Rotate(int X, int Y, double *p1, double *p2, double *vpn)
{
  double *pts = ((vtkDoubleArray *)this->Points->GetData())->GetPointer(0);
  double *center = ((vtkDoubleArray *)this->Points->GetData())->GetPointer(3*14);
  double v[3]; //vector of motion
  double axis[3]; //axis of rotation
  double theta; //rotation angle
  int i;

  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // Create axis of rotation and angle of rotation
  vtkMath::Cross(vpn,v,axis);
  if ( vtkMath::Normalize(axis) == 0.0 )
    {
    return;
    }
  int *size = this->CurrentRenderer->GetSize();
  double l2 = (X-this->Interactor->GetLastEventPosition()[0])*(X-this->Interactor->GetLastEventPosition()[0]) + (Y-this->Interactor->GetLastEventPosition()[1])*(Y-this->Interactor->GetLastEventPosition()[1]);
  theta = 360.0 * sqrt(l2/((double)size[0]*size[0]+size[1]*size[1]));

  //Manipulate the transform to reflect the rotation
  this->Transform->Identity();
  this->Transform->Translate(center[0],center[1],center[2]);
  this->Transform->RotateWXYZ(theta,axis);
  this->Transform->Translate(-center[0],-center[1],-center[2]);

  //Set the corners
  vtkPoints *newPts = vtkPoints::New(VTK_DOUBLE);
  this->Transform->TransformPoints(this->Points,newPts);

  for (i=0; i<8; i++, pts+=3)
    {
    this->Points->SetPoint(i, newPts->GetPoint(i));
    }

  newPts->Delete();
  this->PositionHandles();
}
  
void vtkBoxWidget::CreateDefaultProperties()
{
  if ( ! this->HandleProperty )
    {
    this->HandleProperty = vtkProperty::New();
    this->HandleProperty->SetColor(1,1,1);
    }
  if ( ! this->SelectedHandleProperty )
    {
    this->SelectedHandleProperty = vtkProperty::New();
    this->SelectedHandleProperty->SetColor(1,0,0);
    }
  
  if ( ! this->FaceProperty )
    {
    this->FaceProperty = vtkProperty::New();
    this->FaceProperty->SetColor(1,1,1);
    this->FaceProperty->SetOpacity(0.0);
    }
  if ( ! this->SelectedFaceProperty )
    {
    this->SelectedFaceProperty = vtkProperty::New();
    this->SelectedFaceProperty->SetColor(1,1,0);
    this->SelectedFaceProperty->SetOpacity(0.25);
    }
  
  if ( ! this->OutlineProperty )
    {
    this->OutlineProperty = vtkProperty::New();
    this->OutlineProperty->SetRepresentationToWireframe();
    this->OutlineProperty->SetAmbient(1.0);
    this->OutlineProperty->SetAmbientColor(1.0,1.0,1.0);
    this->OutlineProperty->SetLineWidth(2.0);
    }
  if ( ! this->SelectedOutlineProperty )
    {
    this->SelectedOutlineProperty = vtkProperty::New();
    this->SelectedOutlineProperty->SetRepresentationToWireframe();
    this->SelectedOutlineProperty->SetAmbient(1.0);
    this->SelectedOutlineProperty->SetAmbientColor(0.0,1.0,0.0);
    this->SelectedOutlineProperty->SetLineWidth(2.0);
    }
}

void vtkBoxWidget::PlaceWidget(float bds[6])
{
  int i;
  float bounds[6], center[3];
  
  this->AdjustBounds(bds,bounds,center);
  
  this->Points->SetPoint(0, bounds[0], bounds[2], bounds[4]);
  this->Points->SetPoint(1, bounds[1], bounds[2], bounds[4]);
  this->Points->SetPoint(2, bounds[1], bounds[3], bounds[4]);
  this->Points->SetPoint(3, bounds[0], bounds[3], bounds[4]);
  this->Points->SetPoint(4, bounds[0], bounds[2], bounds[5]);
  this->Points->SetPoint(5, bounds[1], bounds[2], bounds[5]);
  this->Points->SetPoint(6, bounds[1], bounds[3], bounds[5]);
  this->Points->SetPoint(7, bounds[0], bounds[3], bounds[5]);

  this->PositionHandles();
  this->ComputeNormals();

  for (i=0; i<6; i++)
    {
    this->InitialBounds[i] = bounds[i];
    }
  this->InitialLength = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                             (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                             (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));
  for(i=0; i<7; i++)
    {
    this->HandleGeometry[i]->SetRadius(0.025*this->InitialLength);
    }
}

void vtkBoxWidget::GetTransform(vtkTransform *t)
{
  double *pts = ((vtkDoubleArray *)this->Points->GetData())->GetPointer(0);
  double *p0 = pts;
  double *p1 = pts + 3*1;
  //double *p2 = pts + 3*2;
  double *p3 = pts + 3*3;
  double *p4 = pts + 3*4;
  //double *p5 = pts + 3*5;
  //double *p6 = pts + 3*6;
  //double *p7 = pts + 3*7;
  double *p14 = pts + 3*14;
  double center[3], scale[3], scaleVec[3][3];
  int i;

  // The transformation is relative to the initial bounds.
  // Initial bounds are set when PlaceWidget() is invoked.
  t->Identity();
  
  // Translation
  for (i=0; i<3; i++)
    {
    center[i] = p14[i] - 
      (this->InitialBounds[2*i+1]+this->InitialBounds[2*i]) / 2.0;
    }
  t->Translate(center[0], center[1], center[2]);
  
  // Orientation
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  this->PositionHandles();
  this->ComputeNormals();
  for (i=0; i<3; i++)
    {
    matrix->SetElement(i,0,this->N[1][i]);
    matrix->SetElement(i,1,this->N[3][i]);
    matrix->SetElement(i,2,this->N[5][i]);
    }
  t->Concatenate(matrix);
  matrix->Delete();

  // Scale
  for (i=0; i<3; i++)
    {
    scaleVec[0][i] = (p1[i] - p0[i]);
    scaleVec[1][i] = (p3[i] - p0[i]);
    scaleVec[2][i] = (p4[i] - p0[i]);
    }

  scale[0] = vtkMath::Norm(scaleVec[0]) /
    (this->InitialBounds[1]-this->InitialBounds[0]);
  scale[1] = vtkMath::Norm(scaleVec[1]) /
    (this->InitialBounds[3]-this->InitialBounds[2]);
  scale[2] = vtkMath::Norm(scaleVec[2]) /
    (this->InitialBounds[5]-this->InitialBounds[4]);
  t->Scale(scale[0],scale[1],scale[2]);
  
}

void vtkBoxWidget::GetPolyData(vtkPolyData *pd)
{
  pd->SetPoints(this->HexPolyData->GetPoints());
  pd->SetPolys(this->HexPolyData->GetPolys());
}

void vtkBoxWidget::GenerateOutline()
{
  // Now the outline lines
  if ( ! this->OutlineFaceWires && ! this->OutlineCursorWires )
    {
    return;
    }

  vtkIdType pts[2];
  vtkCellArray *cells = this->OutlinePolyData->GetLines();
  cells->Reset();

  if ( this->OutlineFaceWires )
    {
    pts[0] = 0; pts[1] = 7;       //the -x face
    cells->InsertNextCell(2,pts);
    pts[0] = 3; pts[1] = 4;
    cells->InsertNextCell(2,pts);
    pts[0] = 1; pts[1] = 6;       //the +x face
    cells->InsertNextCell(2,pts);
    pts[0] = 2; pts[1] = 5;
    cells->InsertNextCell(2,pts);
    pts[0] = 1; pts[1] = 4;       //the -y face
    cells->InsertNextCell(2,pts);
    pts[0] = 0; pts[1] = 5;
    cells->InsertNextCell(2,pts);
    pts[0] = 3; pts[1] = 6;       //the +y face
    cells->InsertNextCell(2,pts);
    pts[0] = 2; pts[1] = 7;
    cells->InsertNextCell(2,pts);
    pts[0] = 0; pts[1] = 2;       //the -z face
    cells->InsertNextCell(2,pts);
    pts[0] = 1; pts[1] = 3;
    cells->InsertNextCell(2,pts);
    pts[0] = 4; pts[1] = 6;       //the +Z face
    cells->InsertNextCell(2,pts);
    pts[0] = 5; pts[1] = 7;
    cells->InsertNextCell(2,pts);
    }
  if ( this->OutlineCursorWires )
    {
    pts[0] = 8; pts[1] = 9;         //the x cursor line
    cells->InsertNextCell(2,pts);
    pts[0] = 10; pts[1] = 11;       //the y cursor line
    cells->InsertNextCell(2,pts);
    pts[0] = 12; pts[1] = 13;       //the z cursor line
    cells->InsertNextCell(2,pts);
    }
  this->OutlinePolyData->Modified();
  if ( this->OutlineProperty) 
    {
    this->OutlineProperty->SetRepresentationToWireframe();
    this->SelectedOutlineProperty->SetRepresentationToWireframe();
    }
}
