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

#include "vtkAssemblyNode.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellPicker.h"
#include "vtkDataSetMapper.h"
#include "vtkImageData.h"
#include "vtkImageMapToColors.h"
#include "vtkImageReslice.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPlaneSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTexture.h"
#include "vtkTextureMapToPlane.h"
#include "vtkTransform.h"

vtkCxxRevisionMacro(vtkImagePlaneWidget, "1.26");
vtkStandardNewMacro(vtkImagePlaneWidget);

vtkCxxSetObjectMacro(vtkImagePlaneWidget, PlaneProperty,vtkProperty);
vtkCxxSetObjectMacro(vtkImagePlaneWidget, SelectedPlaneProperty,vtkProperty);

vtkImagePlaneWidget::vtkImagePlaneWidget()
{
  this->State = vtkImagePlaneWidget::Start;
  this->EventCallbackCommand->SetCallback(vtkImagePlaneWidget::ProcessEvents);
  
  this->PlaneOrientation = 0; //default align normal to x-axis
  this->RestrictPlaneToVolume = 1;
  this->OriginalWindow = 1.0;
  this->OriginalLevel = 0.5;
  this->TextureInterpolate = 1;
  this->ResliceInterpolate = 1; // default linear interpolation
  this->UserPickerEnabled = 0;
  this->UserLookupTableEnabled = 0;

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
  this->PlaneProperty = 0;
  this->SelectedPlaneProperty = 0;
  this->CreateDefaultProperties();

  this->SetRepresentation();

  this->LookupTable = vtkLookupTable::New();
  this->ColorMap = vtkImageMapToColors::New();
  this->Reslice = vtkImageReslice::New();
  this->ResliceAxes = vtkMatrix4x4::New();
  this->Texture = vtkTexture::New();
  this->TexturePlaneCoords = vtkTextureMapToPlane::New();
  this->TexturePlaneMapper = vtkDataSetMapper::New();
  this->TexturePlaneActor = vtkActor::New();
  this->DummyTransform = vtkTransform::New();

  this->ImageData = 0;
  this->GenerateTexturePlane();
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
    this->PlanePicker = 0;
    }

  if ( this->PlaneProperty )
    {
    this->PlaneProperty->Delete();
    }
  if ( this->SelectedPlaneProperty )
    {
    this->SelectedPlaneProperty->Delete();
    }

  this->ResliceAxes->Delete();
  this->DummyTransform->Delete();
  this->Reslice->Delete();

  if ( !this->UserLookupTableEnabled )
    {
    this->LookupTable->Delete();
    }
  else
    {
    this->LookupTable = 0;
    }

  this->TexturePlaneCoords->Delete();
  this->TexturePlaneMapper->Delete();
  this->TexturePlaneActor->Delete();
  this->ColorMap->Delete();
  this->Texture->Delete();

  if ( this->ImageData )
    {
    this->ImageData = 0;
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
      this->Interactor->FindPokedRenderer(this->Interactor->GetLastEventPosition()[0],
                                          this->Interactor->GetLastEventPosition()[1]);
    if ( this->CurrentRenderer == 0 )
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

    if ( this->PlanePicker )
      {
      this->PlaneActor->PickableOn();
      }

    this->InvokeEvent(vtkCommand::EnableEvent,0);

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

    if ( this->PlanePicker )
      {
      this->PlaneActor->PickableOn();
      }

    this->InvokeEvent(vtkCommand::DisableEvent,0);
    }

  this->Interactor->Render();
}

void vtkImagePlaneWidget::ProcessEvents(vtkObject* vtkNotUsed(object),
                                        unsigned long event,
                                        void* clientdata,
                                        void* vtkNotUsed(calldata))
{
  vtkImagePlaneWidget* self =
    reinterpret_cast<vtkImagePlaneWidget *>( clientdata );

  //okay, let's do the right thing
  switch ( event )
    {
    case vtkCommand::LeftButtonPressEvent:
      self->OnLeftButtonDown();
      break;
    case vtkCommand::LeftButtonReleaseEvent:
      self->OnLeftButtonUp();
      break;
    case vtkCommand::MiddleButtonPressEvent:
      self->OnMiddleButtonDown();
      break;
    case vtkCommand::MiddleButtonReleaseEvent:
      self->OnMiddleButtonUp();
      break;
    case vtkCommand::RightButtonPressEvent:
      self->OnRightButtonDown();
      break;
    case vtkCommand::RightButtonReleaseEvent:
      self->OnRightButtonUp();
      break;
    case vtkCommand::MouseMoveEvent:
      self->OnMouseMove();
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
  if ( this->LookupTable )
    {
    os << indent << "LookupTable: "
       << this->LookupTable << "\n";
    }
  else
    {
    os << indent << "LookupTable: (none)\n";
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
  os << indent << "Restrict Plane To Volume: " 
     << (this->RestrictPlaneToVolume ? "On\n" : "Off\n") ;
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

void vtkImagePlaneWidget::OnLeftButtonDown()
{
  this->State = vtkImagePlaneWidget::Pushing;

  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, we can process this. If anything is picked, then we
  // can start pushing the plane.
  vtkAssemblyPath *path;
  this->Interactor->FindPokedRenderer(X,Y);
  this->PlanePicker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->PlanePicker->GetPath();

  int found = 0;
  int i;
  if ( path != 0 )
    {
// Deal with the possibility that we may be using a shared picker
    path->InitTraversal();
    vtkAssemblyNode *node;
    for ( i = 0; i < path->GetNumberOfItems() && !found ; i++ )
      {
      node = path->GetNextNode();
      if ( node->GetProp() == vtkProp::SafeDownCast(this->PlaneActor) )
        {
        found = 1;
        }
      }
    }

  if( ! found || path == 0 )
    {
    this->State = vtkImagePlaneWidget::Outside;
    this->HighlightPlane(0);
    return;
    }

  this->HighlightPlane(1);

  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,0);
  this->Interactor->Render();
}

void vtkImagePlaneWidget::OnLeftButtonUp()
{
  if ( this->State == vtkImagePlaneWidget::Outside )
    {
    return;
    }

  this->State = vtkImagePlaneWidget::Start;
  this->HighlightPlane(0);

  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent,0);
  this->Interactor->Render();
}

void vtkImagePlaneWidget::OnMiddleButtonDown()
{
  this->State = vtkImagePlaneWidget::Pushing;

  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, we can process this. If anything is picked, then we
  // can start pushing the plane.
  vtkAssemblyPath *path;
  this->Interactor->FindPokedRenderer(X,Y);
  this->PlanePicker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->PlanePicker->GetPath();

  int found = 0;
  int i;
  if ( path != 0 )
    {
// Deal with the possibility that we may be using a shared picker
    path->InitTraversal();
    vtkAssemblyNode *node;
    for ( i = 0; i < path->GetNumberOfItems() && !found ; i++ )
      {
      node = path->GetNextNode();
      if ( node->GetProp() == vtkProp::SafeDownCast(this->PlaneActor) )
        {
        found = 1;
        }
      }
    }

  if( ! found || path == 0 )
    {
    this->State = vtkImagePlaneWidget::Outside;
    this->HighlightPlane(0);
    return;
    }

  this->HighlightPlane(1);

  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,0);
  this->Interactor->Render();
}

void vtkImagePlaneWidget::OnMiddleButtonUp()
{
  if ( this->State == vtkImagePlaneWidget::Outside )
    {
    return;
    }

  this->State = vtkImagePlaneWidget::Start;
  this->HighlightPlane(0);

  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent,0);
  this->Interactor->Render();
}

void vtkImagePlaneWidget::OnRightButtonDown()
{
  this->State = vtkImagePlaneWidget::WindowLevelling;

  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, we can process this. If anything is picked, then we
  // can start pushing the plane.
  vtkAssemblyPath *path;
  this->Interactor->FindPokedRenderer(X,Y);
  this->PlanePicker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->PlanePicker->GetPath();

  int found = 0;
  int i;
  if ( path != 0 )
    {
// Deal with the possibility that we may be using a shared picker
    path->InitTraversal();
    vtkAssemblyNode *node;
    for ( i = 0; i < path->GetNumberOfItems() && !found ; i++ )
      {
      node = path->GetNextNode();
      if ( node->GetProp() == vtkProp::SafeDownCast(this->PlaneActor) )
        {
        found = 1;
        }
      }
    }

  if( ! found || path == 0 )
    {
    this->State = vtkImagePlaneWidget::Outside;
    this->HighlightPlane(0);
    return;
    }

  this->HighlightPlane(1);

  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,0);
  this->Interactor->Render();
}

void vtkImagePlaneWidget::OnRightButtonUp()
{
  if ( this->State == vtkImagePlaneWidget::Outside )
    {
    return;
    }

  this->State = vtkImagePlaneWidget::Start;
  this->HighlightPlane(0);

  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent,0);
  this->Interactor->Render();
}


void vtkImagePlaneWidget::OnMouseMove()
{
  // See whether we're active
  if ( this->State == vtkImagePlaneWidget::Outside ||
       this->State == vtkImagePlaneWidget::Start )
    {
    return;
    }

  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Do different things depending on state
  // Calculations everybody does
  double focalPoint[4], pickPoint[4], prevPickPoint[4];
  double z;

  vtkRenderer *renderer = this->Interactor->FindPokedRenderer(X,Y);
  vtkCamera *camera = renderer->GetActiveCamera();
  if ( ! camera )
    {
    return;
    }

  // Compute the two points defining the motion vector
  camera->GetFocalPoint(focalPoint);
  this->ComputeWorldToDisplay(focalPoint[0], focalPoint[1],
                              focalPoint[2], focalPoint);
  z = focalPoint[2];
  this->ComputeDisplayToWorld(double(this->Interactor->GetLastEventPosition()[0]),
                              double(this->Interactor->GetLastEventPosition()[1]),
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
  this->InvokeEvent(vtkCommand::InteractionEvent,0);
  this->Interactor->Render();
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

  level = level + (X - this->Interactor->GetLastEventPosition()[0])*owin/500.0;
  window = window + (this->Interactor->GetLastEventPosition()[1] - Y)*owin/250.0;

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

  this->PlaneOrientation = i;
  this->Modified();

  // this method must be called _after_ SetInput
  this->ImageData = this->Reslice->GetInput();
  if ( !this->ImageData )
    {
    vtkErrorMacro(<<"SetInput() before setting plane orientation.");
    return;
    }
  this->ImageData->UpdateInformation();
  int extent[6];
  this->ImageData->GetWholeExtent(extent);
  float origin[3];
  this->ImageData->GetOrigin(origin);
  float spacing[3];
  this->ImageData->GetSpacing(spacing);

  // prevent obscuring voxels by offsetting the plane geometry
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

  if ( i == 2 ) //XY, z-normal
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
  this->LookupTable->SetNumberOfColors( 256);
  this->LookupTable->SetHueRange( 0, 0);
  this->LookupTable->SetSaturationRange( 0, 0);
  this->LookupTable->SetValueRange( 0 ,1);
  this->LookupTable->SetAlphaRange( 1, 1);
  this->LookupTable->Build();

  this->SetResliceInterpolate(this->ResliceInterpolate);

  this->ColorMap->SetLookupTable(this->LookupTable);
  this->ColorMap->SetOutputFormatToRGB();

  this->TexturePlaneCoords->SetInput(this->PlaneSource->GetOutput());
  this->TexturePlaneCoords->AutomaticPlaneGenerationOff();

  this->TexturePlaneMapper->SetInput(this->TexturePlaneCoords->GetOutput());

  this->Texture->SetQualityTo32Bit();
  this->Texture->MapColorScalarsThroughLookupTableOff();
  this->Texture->SetInterpolate(this->TextureInterpolate);
  this->Texture->RepeatOff();
  this->Texture->SetLookupTable(this->LookupTable);

  this->TexturePlaneActor->SetMapper(this->TexturePlaneMapper);
  this->TexturePlaneActor->GetProperty()->SetAmbient(0.5);
  this->TexturePlaneActor->SetTexture(this->Texture);
  this->TexturePlaneActor->PickableOff();
}

void vtkImagePlaneWidget::SetInput(vtkDataSet* input)
{
  this->Superclass::SetInput(input);
  this->ImageData = vtkImageData::SafeDownCast(this->GetInput());
  if( ! this->ImageData )
    {
    vtkErrorMacro(<<"Must call SetInput() with vtkImageData*!");
    return;
    }
  float range[2];
  this->ImageData->GetScalarRange(range);

  this->LookupTable->SetTableRange(range[0],range[1]);
  this->LookupTable->Build();

  this->OriginalWindow = range[1] - range[0];
  this->OriginalLevel = 0.5*(range[0] + range[1]);

  this->Reslice->SetInput(this->ImageData);
  this->SetResliceInterpolate(this->ResliceInterpolate);

  this->ColorMap->SetInput(this->Reslice->GetOutput());

  this->Texture->SetInput(this->ColorMap->GetOutput());
  this->Texture->SetInterpolate(this->TextureInterpolate);

  this->SetPlaneOrientation(this->PlaneOrientation);
}

void vtkImagePlaneWidget::UpdateOrigin()
{
  int i;

  if ( this->RestrictPlaneToVolume )
    {
    if (! this->Reslice )
      {
      return;  
      }
    this->ImageData = this->Reslice->GetInput();
    if (! this->ImageData )
      {
      return;  
      }

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
      abs_normal[i] = fabs(abs_normal[i]);
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

  float planeAxis1[3];
  float planeAxis2[3];

  int i;
  for( i = 0 ; i < 3 ; i++ )
    {
    planeAxis1[i] = planePoint1[i]-planeOrigin[i];
    planeAxis2[i] = planePoint2[i]-planeOrigin[i];
    }

  // the x,y dimensions of the plane
  float planeSizeX = vtkMath::Normalize(planeAxis1);
  float planeSizeY = vtkMath::Normalize(planeAxis2);

  // generate the slicing matrix
  this->ResliceAxes->Identity();
  for ( i = 0; i < 3; i++ )
     {
     this->ResliceAxes->SetElement(i,0,planeAxis1[i]);
     this->ResliceAxes->SetElement(i,1,planeAxis2[i]);
     this->ResliceAxes->SetElement(i,2,this->Normal[i]);
     }

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

  this->ImageData = this->Reslice->GetInput();
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

vtkImageData* vtkImagePlaneWidget::GetResliceOutput()
{
  if ( ! this->Reslice )
    {
      return 0;
    }
  return this->Reslice->GetOutput();
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

void vtkImagePlaneWidget::SetResliceInterpolate(int i)
{
  if ( this->ResliceInterpolate == i )
    {
    return;
    }
  this->ResliceInterpolate = i;
  this->Modified();

  if ( ! this->Reslice )
    {
    return;
    }
  
  if ( i == 0 )
    {
    this->Reslice->SetInterpolationModeToNearestNeighbor();
    } 
  else if ( i == 1)
    {
    this->Reslice->SetInterpolationModeToLinear(); 
    }
  else
    {
    this->Reslice->SetInterpolationModeToCubic();
    }
}

void vtkImagePlaneWidget::SetPicker(vtkCellPicker* picker)
{
  if ( this->UserPickerEnabled )
    {
    this->PlanePicker = picker;
    if (picker == 0 ) //reset and allocate an internal picker
      {
      this->PlanePicker = vtkCellPicker::New();
      this->UserPickerEnabled = 0;
      }
    }
  else
    {
    if (picker != 0 )
      {
      this->PlanePicker->Delete();
      this->PlanePicker = picker;
      this->UserPickerEnabled = 1;
      }
    else
      {
      return;
      }
    }

  this->PlanePicker->SetTolerance(0.005); //need some fluff
  this->PlanePicker->AddPickList(this->PlaneActor);
  this->PlanePicker->PickFromListOn();
}

void vtkImagePlaneWidget::SetLookupTable(vtkLookupTable* table)
{
  if ( this->UserLookupTableEnabled )
    {
    this->LookupTable = table;
    if ( table == 0 ) //reset and allocate an internal lut
      {
      this->LookupTable = vtkLookupTable::New();
      this->UserLookupTableEnabled = 0;
      }
    }
  else
    {
    if ( table != 0 )
      {
      this->LookupTable->Delete();
      this->LookupTable = table;
      this->UserLookupTableEnabled = 1;
      }
    else
      {
      return;
      }
    }

  this->LookupTable->SetNumberOfColors( 256);
  this->LookupTable->SetHueRange( 0, 0);
  this->LookupTable->SetSaturationRange( 0, 0);
  this->LookupTable->SetValueRange( 0 ,1);
  this->LookupTable->SetAlphaRange( 1, 1);
  this->LookupTable->Build();

  this->ColorMap->SetLookupTable(this->LookupTable);
  this->Texture->SetLookupTable(this->LookupTable);

  if( !this->ImageData )
    {
    return;
    }

  float range[2];
  this->ImageData->GetScalarRange(range);

  this->LookupTable->SetTableRange(range[0],range[1]);
  this->LookupTable->Build();

  this->OriginalWindow = range[1] - range[0];
  this->OriginalLevel = 0.5*(range[0] + range[1]);
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
  if ( ! this->Reslice )
    {
      return;
    }
  this->ImageData = this->Reslice->GetInput();
  if ( ! this->ImageData )
    {
    return;
    } 
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
    planeOrigin[2] = origin[2] + index*spacing[2];
    pt1[2] = planeOrigin[2];
    pt2[2] = planeOrigin[2];
    }
  else if ( this->PlaneOrientation == 1 )
    {
    planeOrigin[1] = origin[1] + index*spacing[1]; 
    pt1[1] = planeOrigin[1];
    pt2[1] = planeOrigin[1];
    }
  else if ( this->PlaneOrientation == 0 )
    {
    planeOrigin[0] = origin[0] + index*spacing[0]; 
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
  if ( ! this->Reslice )
    {
      return 0;
    }
  this->ImageData = this->Reslice->GetInput();
  if ( ! this->ImageData )
    {
    return 0;
    } 
  this->ImageData->UpdateInformation();
  float origin[3];
  this->ImageData->GetOrigin(origin);
  float spacing[3];
  this->ImageData->GetSpacing(spacing);
  float planeOrigin[3];
  this->PlaneSource->GetOrigin(planeOrigin);

  if ( this->PlaneOrientation == 2 )
    {
    return vtkMath::Round((planeOrigin[2]-origin[2])/spacing[2]);
    }
  else if ( this->PlaneOrientation == 1 )
    {
    return vtkMath::Round((planeOrigin[1]-origin[1])/spacing[1]);
    }
  else if ( this->PlaneOrientation == 0 )
    {
    return vtkMath::Round((planeOrigin[0]-origin[0])/spacing[0]);
    }
  else
    {
    vtkGenericWarningMacro("only works for ortho planes: set plane orientation first");
    }

  return 0;
}

void vtkImagePlaneWidget::SetOrigin(float x, float y, float z)
{
  this->PlaneSource->SetOrigin(x,y,z);
}

void vtkImagePlaneWidget::SetOrigin(float x[3])
{
  this->PlaneSource->SetOrigin(x);
}

float* vtkImagePlaneWidget::GetOrigin()
{
  return this->PlaneSource->GetOrigin();
}

void vtkImagePlaneWidget::GetOrigin(float xyz[3])
{
  this->PlaneSource->GetOrigin(xyz);
}

void vtkImagePlaneWidget::SetPoint1(float x, float y, float z)
{
  this->PlaneSource->SetPoint1(x,y,z);
}

void vtkImagePlaneWidget::SetPoint1(float x[3])
{
  this->PlaneSource->SetPoint1(x);
}

float* vtkImagePlaneWidget::GetPoint1()
{
  return this->PlaneSource->GetPoint1();
}
void vtkImagePlaneWidget::GetPoint1(float xyz[3])
{
  this->PlaneSource->GetPoint1(xyz);
}
void vtkImagePlaneWidget::SetPoint2(float x, float y, float z)
{
  this->PlaneSource->SetPoint2(x,y,z);
}

void vtkImagePlaneWidget::SetPoint2(float x[3])
{
  this->PlaneSource->SetPoint2(x);
}

float* vtkImagePlaneWidget::GetPoint2()
{
  return this->PlaneSource->GetPoint2();
}
void vtkImagePlaneWidget::GetPoint2(float xyz[3])
{
  this->PlaneSource->GetPoint2(xyz);
}

float* vtkImagePlaneWidget::GetCenter() 
{
  return this->PlaneSource->GetCenter();
}

void vtkImagePlaneWidget::GetCenter(float xyz[3]) 
{
  this->PlaneSource->GetCenter(xyz);
}

float* vtkImagePlaneWidget::GetNormal() 
{
  return this->PlaneSource->GetNormal();
}

void vtkImagePlaneWidget::GetNormal(float xyz[3]) 
{
  this->PlaneSource->GetNormal(xyz);
}

void vtkImagePlaneWidget::GetPolyData(vtkPolyData *pd)
{ 
  pd->ShallowCopy(this->PlaneSource->GetOutput()); 
}
