/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagePlaneWidget.cxx
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
#include "vtkImagePlaneWidget.h"
#include "vtkMath.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkFloatArray.h"
#include "vtkCellPicker.h"
#include "vtkAssemblyNode.h"
#include "vtkDoubleArray.h"
#include "vtkPlanes.h"
#include "vtkCallbackCommand.h"
#include "vtkObjectFactory.h"
#include "vtkLookupTable.h"
#include "vtkImageReslice.h"
#include "vtkImageMapToColors.h"
#include "vtkTextureMapToPlane.h"
#include "vtkTexture.h"
#include "vtkDataSetMapper.h"
#include "vtkImageData.h"
#include "vtkTransform.h"
#include "vtkMatrix4x4.h"

vtkCxxRevisionMacro(vtkImagePlaneWidget, "1.6");
vtkStandardNewMacro(vtkImagePlaneWidget);

vtkImagePlaneWidget::vtkImagePlaneWidget()
{
  this->State = vtkImagePlaneWidget::Start;
  this->EventCallbackCommand->SetCallback(vtkImagePlaneWidget::ProcessEvents);
  
  this->PlaneOrientation = -1; //aligned normal to x axis
  this->RestrictPlaneToVolume = 1;
  this->OriginalWindow = 1.0;
  this->OriginalLevel = 0.5;
  this->TextureInterpolate = 1;
  this->ResliceInterpolate = VTK_CUBIC_RESLICE;
  this->UserPickerEnabled = 0;

  // Represent the plane
  this->PlaneSource = vtkPlaneSource::New();
  this->PlaneSource->SetXResolution(4);
  this->PlaneSource->SetYResolution(4);
  this->PlaneOutline = vtkPolyData::New();
  vtkPoints *pts = vtkPoints::New();
  pts->SetNumberOfPoints(4);
  vtkCellArray *outline = vtkCellArray::New();
  outline->InsertNextCell(4);
  outline->InsertCellPoint(0);
  outline->InsertCellPoint(1);
  outline->InsertCellPoint(2);
  outline->InsertCellPoint(3);
  this->PlaneOutline->SetPoints(pts);
  pts->Delete();
  this->PlaneOutline->SetPolys(outline);
  outline->Delete();
  this->PlaneMapper = vtkPolyDataMapper::New();
  this->PlaneMapper->SetInput( this->PlaneSource->GetOutput() );

  this->PlaneActor = vtkActor::New();
  this->PlaneActor->SetMapper(this->PlaneMapper);

  // Define the point coordinates
  float bounds[6];
  bounds[0] = -0.5;
  bounds[1] = 0.5;
  bounds[2] = -0.5;
  bounds[3] = 0.5;
  bounds[4] = -0.5;
  bounds[5] = 0.5;

  // Initial creation of the widget, serves to initialize it
  this->PlaceWidget(bounds);

  //Manage the picking stuff
  this->PlanePicker = vtkCellPicker::New();
  this->PlanePicker->SetTolerance(0.005); //need some fluff
  this->PlanePicker->AddPickList(this->PlaneActor);
  this->PlanePicker->PickFromListOn();

  // Set up the initial properties
  this->PlaneProperty = NULL;
  this->SelectedPlaneProperty = NULL;
  this->CreateDefaultProperties();

  this->SetRepresentation();

  this->ResliceAxes = NULL;
  this->Reslice = NULL;
  this->TexturePlaneCoords = NULL;
  this->TexturePlaneMapper = NULL;
  this->TexturePlaneActor = NULL;
  this->Texture = NULL;
  this->LookupTable = NULL;
  this->ColorMap = NULL;
}

vtkImagePlaneWidget::~vtkImagePlaneWidget()
{
  this->PlaneActor->Delete();
  this->PlaneMapper->Delete();
  this->PlaneSource->Delete();
  this->PlaneOutline->Delete();

  if ( !this->UserPickerEnabled )
    {
    this->PlanePicker->Delete();
    }
  else
    {
    this->PlanePicker = NULL;
    }

  if ( this->PlaneProperty )
    {
    this->PlaneProperty->Delete();
    }
  if ( this->SelectedPlaneProperty )
    {
    this->SelectedPlaneProperty->Delete();
    }

  if ( this->ResliceAxes )
    {
    this->ResliceAxes->Delete();
    }
  if ( this->DummyTransform )
    {
    this->DummyTransform->Delete();
    }
  if ( this->Reslice )
    {
    this->Reslice->Delete();
    }
  if ( this->LookupTable )
    {
    this->LookupTable->Delete();
    }
  if ( this->TexturePlaneCoords )
    {
    this->TexturePlaneCoords->Delete();
    }
  if ( this->TexturePlaneMapper )
    {
    this->TexturePlaneMapper->Delete();
    }
  if ( this->TexturePlaneActor )
    {
    this->TexturePlaneActor->Delete();
    }
  if ( this->ColorMap )
    {
    this->ColorMap->Delete();
    }
  if ( this->Texture )
    {
    this->Texture->Delete();
    }
  if ( this->ImageData )
    {
    this->ImageData = NULL;
    }
}

void vtkImagePlaneWidget::SetEnabled(int enabling)
{

  if ( ! this->Interactor )
    {
    vtkErrorMacro(<<"The interactor must be set prior to enabling/disabling widget");
    return;
    }

  if ( enabling ) //----------------------------------------------------------
    {
    vtkDebugMacro(<<"Enabling plane widget");

    if ( this->Enabled ) //already enabled, just return
      {
      return;
      }
    
    this->CurrentRenderer = 
      this->Interactor->FindPokedRenderer(this->OldX,this->OldY);
    if ( this->CurrentRenderer == NULL )
      {
      return;
      }

    this->Enabled = 1;

    // listen for the following events
    vtkRenderWindowInteractor *i = this->Interactor;
    i->AddObserver(vtkCommand::MouseMoveEvent, this->EventCallbackCommand,
                   this->Priority);
    i->AddObserver(vtkCommand::LeftButtonPressEvent,
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::LeftButtonReleaseEvent,
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::MiddleButtonPressEvent,
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::MiddleButtonReleaseEvent,
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::RightButtonPressEvent,
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::RightButtonReleaseEvent,
                   this->EventCallbackCommand, this->Priority);

    // Add the plane
    this->CurrentRenderer->AddActor(this->PlaneActor);
    this->PlaneActor->SetProperty(this->PlaneProperty);

    //add the TexturePlaneActor
    this->CurrentRenderer->AddActor(this->TexturePlaneActor);

    this->SetRepresentation();

    this->InvokeEvent(vtkCommand::EnableEvent,NULL);

    }

  else //disabling----------------------------------------------------------
    {
    vtkDebugMacro(<<"Disabling plane widget");

    if ( ! this->Enabled ) //already disabled, just return
      {
      return;
      }

    this->Enabled = 0;

    // don't listen for events any more
    this->Interactor->RemoveObserver(this->EventCallbackCommand);

    // turn off the plane
    this->CurrentRenderer->RemoveActor(this->PlaneActor);

    //turn off the texture plane
    this->CurrentRenderer->RemoveActor(this->TexturePlaneActor);

    this->InvokeEvent(vtkCommand::DisableEvent,NULL);
    }

  this->Interactor->Render();
}

void vtkImagePlaneWidget::ProcessEvents(vtkObject* object, unsigned long event,
                                        void* clientdata, void* calldata)
{
  vtkImagePlaneWidget* self = 
    reinterpret_cast<vtkImagePlaneWidget *>( clientdata );
  vtkRenderWindowInteractor* rwi = 
    static_cast<vtkRenderWindowInteractor *>( object );
  int* XY = rwi->GetEventPosition();

  //okay, let's do the right thing
  switch ( event )
    {
    case vtkCommand::LeftButtonPressEvent:
      self->OnLeftButtonDown(rwi->GetControlKey(), rwi->GetShiftKey(), 
                             XY[0], XY[1]);
      break;
    case vtkCommand::LeftButtonReleaseEvent:
      self->OnLeftButtonUp(rwi->GetControlKey(), rwi->GetShiftKey(), 
                           XY[0], XY[1]);
      break;
    case vtkCommand::MiddleButtonPressEvent:
      self->OnMiddleButtonDown(rwi->GetControlKey(), rwi->GetShiftKey(), 
                               XY[0], XY[1]);
      break;
    case vtkCommand::MiddleButtonReleaseEvent:
      self->OnMiddleButtonUp(rwi->GetControlKey(), rwi->GetShiftKey(), 
                             XY[0], XY[1]);
      break;
    case vtkCommand::RightButtonPressEvent:
      self->OnRightButtonDown(rwi->GetControlKey(), rwi->GetShiftKey(), 
                              XY[0], XY[1]);
      break;
    case vtkCommand::RightButtonReleaseEvent:
      self->OnRightButtonUp(rwi->GetControlKey(), rwi->GetShiftKey(), 
                            XY[0], XY[1]);
      break;
    case vtkCommand::MouseMoveEvent:
      self->OnMouseMove(rwi->GetControlKey(), rwi->GetShiftKey(), 
                        XY[0], XY[1]);
      break;
    }
}

void vtkImagePlaneWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->PlaneProperty )
    {
    os << indent << "Plane Property: " << this->PlaneProperty << "\n";
    }
  else
    {
    os << indent << "Plane Property: (none)\n";
    }
  if ( this->SelectedPlaneProperty )
    {
    os << indent << "Selected Plane Property: "
       << this->SelectedPlaneProperty << "\n";
    }
  else
    {
    os << indent << "Selected Plane Property: (none)\n";
    }

  int res = this->PlaneSource->GetXResolution();
  float *o = this->PlaneSource->GetOrigin();
  float *pt1 = this->PlaneSource->GetPoint1();
  float *pt2 = this->PlaneSource->GetPoint2();

  os << indent << "Resolution: " << res << "\n";
  os << indent << "Origin: (" << o[0] << ", "
     << o[1] << ", "
     << o[2] << ")\n";
  os << indent << "Point 1: (" << pt1[0] << ", "
     << pt1[1] << ", "
     << pt1[2] << ")\n";
  os << indent << "Point 2: (" << pt2[0] << ", "
     << pt2[1] << ", "
     << pt2[2] << ")\n";

  os << indent << "Plane Orientation: " << this->PlaneOrientation << "\n";
  os << indent << "Reslice Interpolate: " << this->ResliceInterpolate << "\n";
  os << indent << "Texture Interpolate: " 
     << (this->TextureInterpolate ? "On\n" : "Off\n") ;
}

void vtkImagePlaneWidget::PositionHandles()
{
  float *o = this->PlaneSource->GetOrigin();
  float *pt1 = this->PlaneSource->GetPoint1();
  float *pt2 = this->PlaneSource->GetPoint2();

  float x[3];
  x[0] = o[0] + (pt1[0]-o[0]) + (pt2[0]-o[0]);
  x[1] = o[1] + (pt1[1]-o[1]) + (pt2[1]-o[1]);
  x[2] = o[2] + (pt1[2]-o[2]) + (pt2[2]-o[2]);

  this->PlaneOutline->GetPoints()->SetPoint(0,o);
  this->PlaneOutline->GetPoints()->SetPoint(1,pt1);
  this->PlaneOutline->GetPoints()->SetPoint(2,x);
  this->PlaneOutline->GetPoints()->SetPoint(3,pt2);
  this->PlaneOutline->Modified();

  this->SetRepresentation();

  this->PlaneSource->GetNormal(this->Normal);
  vtkMath::Normalize(this->Normal);
}

void vtkImagePlaneWidget::HighlightPlane(int highlight)
{
  if ( highlight )
    {
    this->PlaneActor->SetProperty(this->SelectedPlaneProperty);
    }
  else
    {
    this->PlaneActor->SetProperty(this->PlaneProperty);
    }
}

void vtkImagePlaneWidget::OnLeftButtonDown (int vtkNotUsed(ctrl),
                                            int vtkNotUsed(shift), int X, int Y)
{
  this->State = vtkImagePlaneWidget::Pushing;

  // Okay, we can process this. If anything is picked, then we
  // can start pushing the plane.
  vtkAssemblyPath *path;
  this->Interactor->FindPokedRenderer(X,Y);
  this->PlanePicker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->PlanePicker->GetPath();

  int found = 0;
  int i;
  if ( path != NULL )
    {
// Deal with the possibility that we may be using a shared picker
    path->InitTraversal();
    vtkAssemblyNode *node;
    for ( i = 0; i < path->GetNumberOfItems() && !found ; i++ )
      {
      node = path->GetNextNode();
      if ( node->GetProp() == (vtkProp*)(this->TexturePlaneActor) )
        {
        found = 1;
        }
      }
    }

  if( ! found || path == NULL )
    {
    this->State = vtkImagePlaneWidget::Outside;
    this->HighlightPlane(0);
    return;
    }

  this->HighlightPlane(1);

  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();

  this->OldX = X;
  this->OldY = Y;
}

void vtkImagePlaneWidget::OnLeftButtonUp (int vtkNotUsed(ctrl), int vtkNotUsed(shift),
                                          int vtkNotUsed(X), int vtkNotUsed(Y))
{
  if ( this->State == vtkImagePlaneWidget::Outside )
    {
    return;
    }

  this->State = vtkImagePlaneWidget::Start;
  this->HighlightPlane(0);

  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkImagePlaneWidget::OnMiddleButtonDown (int vtkNotUsed(ctrl),
                                              int vtkNotUsed(shift), int X, int Y)
{
  this->State = vtkImagePlaneWidget::Pushing;

  // Okay, we can process this. If anything is picked, then we
  // can start pushing the plane.
  vtkAssemblyPath *path;
  this->Interactor->FindPokedRenderer(X,Y);
  this->PlanePicker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->PlanePicker->GetPath();

  int found = 0;
  int i;
  if ( path != NULL )
    {
// Deal with the possibility that we may be using a shared picker
    path->InitTraversal();
    vtkAssemblyNode *node;
    for ( i = 0; i < path->GetNumberOfItems() && !found ; i++ )
      {
      node = path->GetNextNode();
      if ( node->GetProp() == (vtkProp*)(this->TexturePlaneActor) )
        {
        found = 1;
        }
      }
    }

  if( ! found || path == NULL )
    {
    this->State = vtkImagePlaneWidget::Outside;
    this->HighlightPlane(0);
    return;
    }

  this->HighlightPlane(1);

  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();

  this->OldX = X;
  this->OldY = Y;
}

void vtkImagePlaneWidget::OnMiddleButtonUp (int vtkNotUsed(ctrl), int vtkNotUsed(shift),
                                            int vtkNotUsed(X), int vtkNotUsed(Y))
{
  if ( this->State == vtkImagePlaneWidget::Outside )
    {
    return;
    }

  this->State = vtkImagePlaneWidget::Start;
  this->HighlightPlane(0);

  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkImagePlaneWidget::OnRightButtonDown (int vtkNotUsed(ctrl),
                                             int vtkNotUsed(shift), int X, int Y)
{
  this->State = vtkImagePlaneWidget::WindowLevelling;

  // Okay, we can process this. If anything is picked, then we
  // can start pushing the plane.
  vtkAssemblyPath *path;
  this->Interactor->FindPokedRenderer(X,Y);
  this->PlanePicker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->PlanePicker->GetPath();

  int found = 0;
  int i;
  if ( path != NULL )
    {
// Deal with the possibility that we may be using a shared picker
    path->InitTraversal();
    vtkAssemblyNode *node;
    for ( i = 0; i < path->GetNumberOfItems() && !found ; i++ )
      {
      node = path->GetNextNode();
      if ( node->GetProp() == (vtkProp*)(this->TexturePlaneActor) )
        {
        found = 1;
        }
      }
    }

  if( ! found || path == NULL )
    {
    this->State = vtkImagePlaneWidget::Outside;
    this->HighlightPlane(0);
    return;
    }

  this->HighlightPlane(1);

  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();

  this->OldX = X;
  this->OldY = Y;
}

void vtkImagePlaneWidget::OnRightButtonUp (int vtkNotUsed(ctrl), int vtkNotUsed(shift),
                                           int vtkNotUsed(X), int vtkNotUsed(Y))
{
  if ( this->State == vtkImagePlaneWidget::Outside )
    {
    return;
    }

  this->State = vtkImagePlaneWidget::Start;
  this->HighlightPlane(0);

  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}


void vtkImagePlaneWidget::OnMouseMove (int vtkNotUsed(ctrl),
                                       int vtkNotUsed(shift), int X, int Y)
{
  // See whether we're active
  if ( this->State == vtkImagePlaneWidget::Outside ||
       this->State == vtkImagePlaneWidget::Start )
    {
    return;
    }

  // Do different things depending on state
  // Calculations everybody does
  double focalPoint[4], pickPoint[4], prevPickPoint[4];
  double z;

  this->CurrentCamera = this->Interactor->FindPokedCamera(X,Y);
  if ( ! this->CurrentCamera )
    {
    return;
    }

  // Compute the two points defining the motion vector
  this->CurrentCamera->GetFocalPoint(focalPoint);
  this->ComputeWorldToDisplay(focalPoint[0], focalPoint[1],
                              focalPoint[2], focalPoint);
  z = focalPoint[2];
  this->ComputeDisplayToWorld(double(this->OldX),double(this->OldY),
                              z, prevPickPoint);
  this->ComputeDisplayToWorld(double(X), double(Y), z, pickPoint);

  if ( this->State == vtkImagePlaneWidget::WindowLevelling )
    {
    this->WindowLevel(X, Y);
    }
  else if ( this->State == vtkImagePlaneWidget::Pushing )
    {
    this->Push(prevPickPoint, pickPoint);
    this->UpdateOrigin();
    }

  // Interact, if desired
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);

  this->Interactor->Render();
  this->OldX = X;
  this->OldY = Y;
}

void vtkImagePlaneWidget::WindowLevel(int X, int Y)
{
  if ( ! this->LookupTable )
    {
    return;
    }

  float range[2];
  this->LookupTable->GetTableRange(range);
  float window = range[1] - range[0];
  float level = 0.5*(range[0]+range[1]);

  float owin = this->OriginalWindow;

  level = level + (X - this->OldX)*owin/500.0;
  window = window + (this->OldY - Y)*owin/250.0;

  if ( window == 0.0 )
    {
    window = 0.001;
    }
  
  float rmin = level-window*0.5;
  float rmax = level+window*0.5;

  if( rmin < rmax )
    {
    range[0] = rmin;
    range[1] = rmax;
    this->LookupTable->SetTableRange(range);
    }  
}

void vtkImagePlaneWidget::Push(double *p1, double *p2)
{
  //Get the motion vector
  float v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];
  
  this->PlaneSource->Push( vtkMath::Dot(v,this->Normal) );
  this->PlaneSource->Update();
  this->PositionHandles();
}

void vtkImagePlaneWidget::CreateDefaultProperties()
{
  if ( ! this->PlaneProperty )
    {
    this->PlaneProperty = vtkProperty::New();
    this->SetRepresentation();
    this->PlaneProperty->SetAmbient(1.0);
    this->PlaneProperty->SetColor(1.0,1.0,1.0);
    }
  if ( ! this->SelectedPlaneProperty )
    {
    this->SelectedPlaneProperty = vtkProperty::New();
    this->SetRepresentation();
    this->SelectedPlaneProperty->SetAmbient(1.0);
    this->SelectedPlaneProperty->SetColor(0.0,1.0,0.0);
    }
}

void vtkImagePlaneWidget::PlaceWidget(float bds[6])
{
  float bounds[6], center[3];

  this->AdjustBounds(bds, bounds, center);

  if ( this->PlaneOrientation == 1 )
    {
    this->PlaneSource->SetOrigin(bounds[0],center[1],bounds[4]);
    this->PlaneSource->SetPoint1(bounds[1],center[1],bounds[4]);
    this->PlaneSource->SetPoint2(bounds[0],center[1],bounds[5]);
    }
  else if ( this->PlaneOrientation == 2 )
    {
    this->PlaneSource->SetOrigin(bounds[0],bounds[2],center[2]);
    this->PlaneSource->SetPoint1(bounds[1],bounds[2],center[2]);
    this->PlaneSource->SetPoint2(bounds[0],bounds[3],center[2]);
    }
  else //default or x-normal
    {
    this->PlaneSource->SetOrigin(center[0],bounds[2],bounds[4]);
    this->PlaneSource->SetPoint1(center[0],bounds[3],bounds[4]);
    this->PlaneSource->SetPoint2(center[0],bounds[2],bounds[5]);
    }
  this->PlaneSource->Update();
  this->PositionHandles();
}

void vtkImagePlaneWidget::SetPlaneOrientation(int i)
{
  // generate a XY plane if i = 2, z-normal
  // or a YZ plane if i = 0, x-normal
  // or a ZX plane if i = 1, y-normal
  if ( this->PlaneOrientation == i )
    {
    return;
    }
  this->PlaneOrientation = i;
  this->Modified();

  // this method must be called _after_ SetInput
  this->ImageData = this->Reslice->GetInput();
  this->ImageData->UpdateInformation();
  int extent[6];
  this->ImageData->GetWholeExtent(extent);
  float origin[3];
  this->ImageData->GetOrigin(origin);
  float spacing[3];
  this->ImageData->GetSpacing(spacing);

  float xbounds[] = {origin[0] + spacing[0] * (extent[0] - 0.5),
                     origin[0] + spacing[0] * (extent[1] + 0.5)};
  float ybounds[] = {origin[1] + spacing[1] * (extent[2] - 0.5),
                     origin[1] + spacing[1] * (extent[3] + 0.5)};
  float zbounds[] = {origin[2] + spacing[2] * (extent[4] - 0.5),
                     origin[2] + spacing[2] * (extent[5] + 0.5)};

  if ( spacing[0] < 0.0f )
    {
    float t = xbounds[0];
    xbounds[0] = xbounds[1];
    xbounds[1] = t;
    }
  if ( spacing[1] < 0.0f )
    {
    float t = ybounds[0];
    ybounds[0] = ybounds[1];
    ybounds[1] = t;
    }
  if ( spacing[2] < 0.0f )
    {
    float t = zbounds[0];
    zbounds[0] = zbounds[1];
    zbounds[1] = t;
    }

  if (i == 2) //XY, z-normal
    {
    this->PlaneSource->SetOrigin(xbounds[0],ybounds[0],zbounds[0]);
    this->PlaneSource->SetPoint1(xbounds[1],ybounds[0],zbounds[0]);
    this->PlaneSource->SetPoint2(xbounds[0],ybounds[1],zbounds[0]);
    }
  else if ( i == 0 ) //YZ, x-normal
    {
    this->PlaneSource->SetOrigin(xbounds[0],ybounds[0],zbounds[0]);
    this->PlaneSource->SetPoint1(xbounds[0],ybounds[1],zbounds[0]);
    this->PlaneSource->SetPoint2(xbounds[0],ybounds[0],zbounds[1]);
    }
  else  //ZX, y-normal
    {
    this->PlaneSource->SetOrigin(xbounds[0],ybounds[0],zbounds[0]);
    this->PlaneSource->SetPoint1(xbounds[0],ybounds[0],zbounds[1]);
    this->PlaneSource->SetPoint2(xbounds[1],ybounds[0],zbounds[0]);
    }

  this->PlaneSource->Update();
  this->PositionHandles();
  this->UpdateNormal();
  this->UpdateOrigin();
}

void vtkImagePlaneWidget::GenerateTexturePlane()
{
  if ( ! this->ImageData )
    {
    vtkGenericWarningMacro(<<"No vtkImageData to slice through!");
    return;
    }
  float range[2];
  this->ImageData->GetScalarRange(range);

  this->LookupTable = vtkLookupTable::New();
  this->LookupTable->SetTableRange(range[0],range[1]);
  this->LookupTable->SetNumberOfColors( 256);
  this->LookupTable->SetHueRange( 0, 0);
  this->LookupTable->SetSaturationRange( 0, 0);
  this->LookupTable->SetValueRange( 0 ,1);
  this->LookupTable->SetAlphaRange( 1, 1);
  this->LookupTable->Build();

  this->OriginalWindow = range[1] - range[0];
  this->OriginalLevel = 0.5*(range[0] + range[1]);

  this->Reslice = vtkImageReslice::New();
  this->Reslice->SetInput(this->ImageData);
  this->SetResliceInterpolateToCubic();

  this->ColorMap = vtkImageMapToColors::New();
  this->ColorMap->SetLookupTable(this->LookupTable);
  this->ColorMap->SetInput(this->Reslice->GetOutput());

  this->TexturePlaneCoords = vtkTextureMapToPlane::New();
  this->TexturePlaneCoords->SetInput(this->PlaneSource->GetOutput());
  this->TexturePlaneCoords->AutomaticPlaneGenerationOff();

  this->TexturePlaneMapper = vtkDataSetMapper::New();
  this->TexturePlaneMapper->SetInput(this->TexturePlaneCoords->GetOutput());

  this->ResliceAxes = vtkMatrix4x4::New();
  this->DummyTransform = vtkTransform::New();

  this->Texture = vtkTexture::New();
  this->Texture->SetQualityTo32Bit();
  this->Texture->MapColorScalarsThroughLookupTableOff();
  this->Texture->SetInput(this->ColorMap->GetOutput());
  this->Texture->SetInterpolate(1);
  this->Texture->RepeatOff();
  this->Texture->SetLookupTable(this->LookupTable);

  this->TexturePlaneActor = vtkActor::New();
  this->TexturePlaneActor->GetProperty()->SetAmbient( 0.5);
  this->TexturePlaneActor->SetMapper(this->TexturePlaneMapper);
  this->TexturePlaneActor->SetTexture(this->Texture);

  this->TexturePlaneActor->GetTexture()->
    SetInterpolate(this->TextureInterpolate);
  
  this->SetPlaneOrientation(0);
}

void vtkImagePlaneWidget::UpdateOrigin()
{
  int i;

  if ( this->RestrictPlaneToVolume )
    {
    this->ImageData = this->Reslice->GetInput();
    this->ImageData->UpdateInformation();
    float origin[3];
    this->ImageData->GetOrigin(origin);
    float spacing[3];
    this->ImageData->GetSpacing(spacing);
    int extent[6];
    this->ImageData->GetWholeExtent(extent);
    float bounds[] = {origin[0] + spacing[0]*extent[0], 
                      origin[0] + spacing[0]*extent[1],
                      origin[1] + spacing[1]*extent[2], 
                      origin[1] + spacing[1]*extent[3],
                      origin[2] + spacing[2]*extent[4], 
                      origin[2] + spacing[2]*extent[5]};

    for ( i = 0; i <= 4; i += 2 ) // reverse bounds if necessary
      {
      if ( bounds[i] > bounds[i+1] )
        {
        float t = bounds[i+1];
        bounds[i+1] = bounds[i];
        bounds[i] = t;
        }
      }

    float abs_normal[3];
    this->PlaneSource->GetNormal(abs_normal);
    float planeCenter[3];
    this->PlaneSource->GetCenter(planeCenter);
    float nmax = 0.0f;
    int k = 0;
    for ( i = 0; i < 3; i++ )
      {
      abs_normal[i] = abs(abs_normal[i]);
      if ( abs_normal[i] > nmax )
        {
        nmax = abs_normal[i];
        k = i;
        }
      }

    if ( planeCenter[k] > bounds[2*k+1] )
      {
      planeCenter[k] = bounds[2*k+1];
      this->PlaneSource->SetCenter(planeCenter);
      this->PlaneSource->Update();
      this->PositionHandles();
      }
    else if ( planeCenter[k] < bounds[2*k] )
      {
      planeCenter[k] = bounds[2*k];
      this->PlaneSource->SetCenter(planeCenter);
      this->PlaneSource->Update();
      this->PositionHandles();
      }
    }

  this->Reslice->SetResliceAxesOrigin(0.0,0.0,0.0);
  this->ResliceAxes->DeepCopy(this->Reslice->GetResliceAxes());

  // transpose in an exact way to invert a rotation matrix
  this->ResliceAxes->Transpose();

  this->DummyTransform->SetMatrix(this->ResliceAxes);
  float planeOrigin[3];
  this->PlaneSource->GetOrigin(planeOrigin);
  double* out = this->DummyTransform->TransformDoublePoint(
    planeOrigin[0],planeOrigin[1],planeOrigin[2]);

  this->ResliceAxes->Transpose();
  this->DummyTransform->SetMatrix(this->ResliceAxes);
  double* newOrigin = this->DummyTransform->TransformDoublePoint(
    0.0,0.0,out[2]);

  this->Reslice->SetResliceAxes(this->ResliceAxes);
  this->Reslice->SetResliceAxesOrigin(newOrigin);
}

void vtkImagePlaneWidget::UpdateNormal()
{
  float planeOrigin[3];
  this->PlaneSource->GetOrigin(planeOrigin);
  float planePoint1[3];
  this->PlaneSource->GetPoint1(planePoint1);
  float planePoint2[3];
  this->PlaneSource->GetPoint2(planePoint2);
  this->PlaneSource->GetNormal(this->Normal);
  vtkMath::Normalize(this->Normal);

  float planeAxis1[3];
  float planeAxis2[3];

  for( int i = 0 ; i < 3 ; i++ )
    {
    planeAxis1[i] = planePoint1[i]-planeOrigin[i];
    planeAxis2[i] = planePoint2[i]-planeOrigin[i];
    }

  // the x,y dimensions of the plane
  float planeSizeX = vtkMath::Normalize(planeAxis1);
  float planeSizeY = vtkMath::Normalize(planeAxis2);

  // generate the slicing matrix
  this->ResliceAxes->Identity();
  this->ResliceAxes->SetElement(0,0,planeAxis1[0]);
  this->ResliceAxes->SetElement(1,0,planeAxis1[1]);
  this->ResliceAxes->SetElement(2,0,planeAxis1[2]);
  this->ResliceAxes->SetElement(0,1,planeAxis2[0]);
  this->ResliceAxes->SetElement(1,1,planeAxis2[1]);
  this->ResliceAxes->SetElement(2,1,planeAxis2[2]);
  this->ResliceAxes->SetElement(0,2,this->Normal[0]);
  this->ResliceAxes->SetElement(1,2,this->Normal[1]);
  this->ResliceAxes->SetElement(2,2,this->Normal[2]);

  // transpose in an exact way to invert a rotation matrix
  this->ResliceAxes->Transpose();
  this->DummyTransform->SetMatrix(this->ResliceAxes);
  double *out = this->DummyTransform->TransformDoublePoint(
    planeOrigin[0],planeOrigin[1],planeOrigin[2]);
  this->ResliceAxes->Transpose();
  this->DummyTransform->SetMatrix(this->ResliceAxes);
  double *newOrigin = this->DummyTransform->TransformDoublePoint(
    0.0, 0.0, out[2]);

  this->Reslice->SetResliceAxes(this->ResliceAxes);
  this->Reslice->SetResliceAxesOrigin(newOrigin);

  this->ImageData = Reslice->GetInput();
  // calculate appropriate pixel spacing for the reslicing

  this->ImageData->UpdateInformation();
  float spacing[3];
  this->ImageData->GetSpacing(spacing);

  float spacingX = fabs(planeAxis1[0]*spacing[0]) + \
                   fabs(planeAxis1[1]*spacing[1]) + \
                   fabs(planeAxis1[2]*spacing[2]);

  float spacingY = fabs(planeAxis2[0]*spacing[0]) + \
                   fabs(planeAxis2[1]*spacing[1]) + \
                   fabs(planeAxis2[2]*spacing[2]);

  // pad extent up to a power of two for efficient texture mapping
  int extentX = 1;
  int exMax = vtkMath::Round(planeSizeX/spacingX);
  while ( extentX < exMax )
    extentX = extentX << 1;

  int extentY = 1;
  int eyMax = vtkMath::Round(planeSizeY/spacingY);
  while ( extentY < eyMax )
    extentY = extentY << 1;

  this->Reslice->SetOutputSpacing(spacingX, spacingY, 1.0);
  this->Reslice->SetOutputOrigin(out[0], out[1], 0.0);
  this->Reslice->SetOutputExtent(0, extentX-1, 0, extentY-1, 0, 0);


  // find expansion factor to account for increasing the extent
  // to a power of two
  float expand1 = extentX*spacingX;
  float expand2 = extentY*spacingY;

  // set the texture coordinates to map the image to the plane
  this->TexturePlaneCoords->SetOrigin(planeOrigin[0], planeOrigin[1], planeOrigin[2]);
  this->TexturePlaneCoords->SetPoint1(planeOrigin[0] + planeAxis1[0]*expand1,
                                      planeOrigin[1] + planeAxis1[1]*expand1,
                                      planeOrigin[2] + planeAxis1[2]*expand1);
  this->TexturePlaneCoords->SetPoint2(planeOrigin[0] + planeAxis2[0]*expand2,
                                      planeOrigin[1] + planeAxis2[1]*expand2,
                                      planeOrigin[2] + planeAxis2[2]*expand2);

}

void vtkImagePlaneWidget::SetRepresentation()
{
  if ( ! this->CurrentRenderer )
    {
    return;
    }

  this->CurrentRenderer->RemoveActor(this->PlaneActor);
  this->CurrentRenderer->AddActor(this->PlaneActor);
  this->PlaneMapper->SetInput( this->PlaneOutline );
  this->PlaneActor->GetProperty()->SetRepresentationToWireframe();
  
  if ( this->PlaneOrientation < 0 )
    {
    this->SetPlaneOrientation(0);
    }
}

void vtkImagePlaneWidget::SetResliceInterpolateToNearestNeighbour()
{
  if ( ! this->Reslice )
    {
    return;
    }

  this->Reslice->SetInterpolationModeToNearestNeighbor();
  this->ResliceInterpolate = VTK_NEAREST_RESLICE;  
}

void vtkImagePlaneWidget::SetResliceInterpolateToLinear()
{
  if ( ! this->Reslice )
    {
    return;
    }

  this->Reslice->SetInterpolationModeToLinear();
  this->ResliceInterpolate = VTK_LINEAR_RESLICE;
}

void vtkImagePlaneWidget::SetResliceInterpolateToCubic()
{
  if ( ! this->Reslice )
    {
    return;
    }

  this->Reslice->SetInterpolationModeToCubic();
  this->ResliceInterpolate = VTK_CUBIC_RESLICE;  
}

void vtkImagePlaneWidget::SetPicker(vtkCellPicker* picker)
{
  if( this->PlanePicker )
    {
    this->PlanePicker->Delete();
    }
  else
    {
    vtkGenericWarningMacro(<<"SetInput() with vtkImageData* must be called first!");
    return;
    }

  this->PlanePicker = picker;
  this->PlanePicker->AddPickList(this->TexturePlaneActor);
  this->UserPickerEnabled = 1;
}

void vtkImagePlaneWidget::SetSlicePosition(float position)
{
  float amount = 0.0f; 
  float planeOrigin[3];
  this->PlaneSource->GetOrigin(planeOrigin);

  if ( this->PlaneOrientation == 2 ) // z axis
    {
    amount = position - planeOrigin[2];
    }
  else if ( this->PlaneOrientation == 0 ) // x axis
    {
    amount = position - planeOrigin[0];
    }
  else if ( this->PlaneOrientation == 1 )  //y axis
    {
    amount = position - planeOrigin[1];
    }
  else
    {
    vtkGenericWarningMacro("only works for ortho planes: set plane orientation first");
    return;
    } 

  this->PlaneSource->Push(amount);
  this->PlaneSource->Update();
  this->PositionHandles();
  this->UpdateOrigin();
}

float vtkImagePlaneWidget::GetSlicePosition()
{
  float planeOrigin[3];
  this->PlaneSource->GetOrigin(planeOrigin);

  if ( this->PlaneOrientation == 2 )
    {
    return planeOrigin[2];
    }  
  else if ( this->PlaneOrientation == 1 )
    { 
    return planeOrigin[1];
    }
  else if ( this->PlaneOrientation == 0 )
    {  
    return planeOrigin[0];
    } 
  else
    {
    vtkGenericWarningMacro("only works for ortho planes: set plane orientation first");
    } 
   return 0.0f;
}

void vtkImagePlaneWidget::SetSliceIndex(int index)
{
  this->ImageData = this->Reslice->GetInput();
  this->ImageData->UpdateInformation();
  float origin[3];
  this->ImageData->GetOrigin(origin);
  float spacing[3];
  this->ImageData->GetSpacing(spacing);
  float planeOrigin[3];
  this->PlaneSource->GetOrigin(planeOrigin);
  float pt1[3];
  this->PlaneSource->GetPoint1(pt1);
  float pt2[3];
  this->PlaneSource->GetPoint2(pt2);

  if ( this->PlaneOrientation == 2 )
    {
    planeOrigin[2] = origin[2] + (index + 0.5)*spacing[2];
    pt1[2] = planeOrigin[2];
    pt2[2] = planeOrigin[2];
    }
  else if ( this->PlaneOrientation == 1 )
    {
    planeOrigin[1] = origin[1] + (index + 0.5)*spacing[1]; 
    pt1[1] = planeOrigin[1];
    pt2[1] = planeOrigin[1];
    }
  else if ( this->PlaneOrientation == 0 )
    {
    planeOrigin[0] = origin[0] + (index + 0.5)*spacing[0]; 
    pt1[0] = planeOrigin[0];
    pt2[0] = planeOrigin[0];
    }
  else
    {
    vtkGenericWarningMacro("only works for ortho planes: set plane orientation first");
    return; 
    } 

  this->PlaneSource->SetOrigin(planeOrigin);
  this->PlaneSource->SetPoint1(pt1);
  this->PlaneSource->SetPoint2(pt2);
  this->PlaneSource->Update();
  this->PositionHandles();
  this->UpdateOrigin();
}

int vtkImagePlaneWidget::GetSliceIndex()
{
  this->ImageData = this->Reslice->GetInput();
  this->ImageData->UpdateInformation();
  float origin[3];
  this->ImageData->GetOrigin(origin);
  float spacing[3];
  this->ImageData->GetSpacing(spacing);
  float planeOrigin[3];
  this->PlaneSource->GetOrigin(planeOrigin);

  if ( this->PlaneOrientation == 2 )
    {
    return vtkMath::Round((planeOrigin[2]-origin[2])/spacing[2] + 0.5);
    }
  else if ( this->PlaneOrientation == 1 )
    {
    return vtkMath::Round((planeOrigin[1]-origin[1])/spacing[1] + 0.5);
    }
  else if ( this->PlaneOrientation == 0 )
    {
    return vtkMath::Round((planeOrigin[0]-origin[0])/spacing[0] + 0.5);
    }
  else
    {
    vtkGenericWarningMacro("only works for ortho planes: set plane orientation first");
    } 

  return 0;
}
