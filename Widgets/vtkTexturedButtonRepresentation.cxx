/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTexturedButtonRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTexturedButtonRepresentation.h"
#include "vtkActor.h"
#include "vtkFollower.h"
#include "vtkTexture.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkCellPicker.h"
#include "vtkRenderer.h"
#include "vtkProperty.h"
#include "vtkAssemblyPath.h"
#include "vtkInteractorObserver.h"
#include "vtkCoordinate.h"
#include "vtkRenderWindow.h"
#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include <vtkstd/map>

vtkStandardNewMacro(vtkTexturedButtonRepresentation);

vtkCxxSetObjectMacro(vtkTexturedButtonRepresentation,Property,vtkProperty);
vtkCxxSetObjectMacro(vtkTexturedButtonRepresentation,HoveringProperty,vtkProperty);
vtkCxxSetObjectMacro(vtkTexturedButtonRepresentation,SelectingProperty,vtkProperty);

// Map of textures
class vtkTextureArray : public vtkstd::map<int,vtkSmartPointer<vtkImageData> > {};
typedef vtkstd::map<int,vtkSmartPointer<vtkImageData> >::iterator vtkTextureArrayIterator;


//----------------------------------------------------------------------
vtkTexturedButtonRepresentation::vtkTexturedButtonRepresentation()
{
  this->Mapper = vtkPolyDataMapper::New();
  this->Texture = vtkTexture::New();
  this->Texture->SetBlendingMode(vtkTexture::VTK_TEXTURE_BLENDING_MODE_ADD);
  this->Actor = vtkActor::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetTexture(this->Texture);
  this->Follower = vtkFollower::New();
  this->Follower->SetMapper(this->Mapper);
  this->Follower->SetTexture(this->Texture);

  // Following
  this->FollowCamera = 0;

  // Set up the initial properties
  this->CreateDefaultProperties();

  // List of textures
  this->TextureArray = new vtkTextureArray;

  this->Picker = vtkCellPicker::New();
  this->Picker->AddPickList(this->Actor);
  this->Picker->AddPickList(this->Follower);
  this->Picker->PickFromListOn();
}

//----------------------------------------------------------------------
vtkTexturedButtonRepresentation::~vtkTexturedButtonRepresentation()
{
  this->Actor->Delete();
  this->Follower->Delete();
  this->Mapper->Delete();
  this->Texture->Delete();

  if ( this->Property )
    {
    this->Property->Delete();
    this->Property = NULL;
    }

  if ( this->HoveringProperty )
    {
    this->HoveringProperty->Delete();
    this->HoveringProperty = NULL;
    }

  if ( this->SelectingProperty )
    {
    this->SelectingProperty->Delete();
    this->SelectingProperty = NULL;
    }

  delete this->TextureArray;

  this->Picker->Delete();
}

//-------------------------------------------------------------------------
void vtkTexturedButtonRepresentation::
SetButtonGeometry(vtkPolyData *pd)
{
  this->Mapper->SetInput(pd);
}


//-------------------------------------------------------------------------
void vtkTexturedButtonRepresentation::
SetButtonGeometryConnection(vtkAlgorithmOutput* algOutput)
{
  this->Mapper->SetInputConnection(algOutput);
}


//-------------------------------------------------------------------------
vtkPolyData *vtkTexturedButtonRepresentation::GetButtonGeometry()
{
  return this->Mapper->GetInput();
}


//-------------------------------------------------------------------------
void vtkTexturedButtonRepresentation::
SetButtonTexture(int i, vtkImageData *image)
{
  if ( i < 0 )
    {
    i = 0;
    }
  if ( i >= this->NumberOfStates )
    {
    i = this->NumberOfStates - 1;
    }

  (*this->TextureArray)[i] = image;
}


//-------------------------------------------------------------------------
vtkImageData *vtkTexturedButtonRepresentation::
GetButtonTexture(int i)
{
  if ( i < 0 )
    {
    i = 0;
    }
  if ( i >= this->NumberOfStates )
    {
    i = this->NumberOfStates - 1;
    }

  vtkTextureArrayIterator iter = this->TextureArray->find(i);
  if ( iter != this->TextureArray->end() )
    {
    return (*iter).second;
    }
  else
    {
    return NULL;
    }
}

//-------------------------------------------------------------------------
void vtkTexturedButtonRepresentation::
PlaceWidget(double scale, double xyz[3], double normal[3])
{
  // Translate the center
  double bds[6], center[3];
  this->Actor->GetBounds(bds);
  center[0] = (bds[0]+bds[1]) / 2.0;
  center[1] = (bds[2]+bds[3]) / 2.0;;
  center[2] = (bds[4]+bds[5]) / 2.0;;

  this->Actor->AddPosition(center[0]-xyz[0],
                           center[1]-xyz[1],
                           center[2]-xyz[2]);
  this->Follower->AddPosition(center[0]-xyz[0],
                              center[1]-xyz[1],
                              center[2]-xyz[2]);

  // Scale the button
  this->Actor->SetScale(scale, scale, scale);
  this->Follower->SetScale(scale, scale, scale);

  // Rotate the button to align with the normal Cross the z axis with the
  // normal to get a rotation vector. Then rotate around it.
  double zAxis[3]; zAxis[0] = zAxis[1] = 0.0; zAxis[2] = 1.0;
  double rotAxis[3], angle;

  vtkMath::Normalize(normal);
  vtkMath::Cross(zAxis,normal,rotAxis);
  angle = acos(vtkMath::Dot(zAxis,normal));
  this->Actor->RotateWXYZ(
    vtkMath::DegreesFromRadians(angle),rotAxis[0],rotAxis[1],rotAxis[2]);
  this->Follower->RotateWXYZ(
    vtkMath::DegreesFromRadians(angle),rotAxis[0],rotAxis[1],rotAxis[2]);
}

//-------------------------------------------------------------------------
void vtkTexturedButtonRepresentation::PlaceWidget(double bds[6])
{
  double bounds[6], center[3], aBds[6], aCenter[3];

  this->AdjustBounds(bds, bounds, center);
  for (int i=0; i<6; i++)
    {
    this->InitialBounds[i] = bounds[i];
    }
  this->InitialLength = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                             (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                             (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));

  // Get the bounds of the actor
  this->Actor->GetBounds(aBds);
  aCenter[0] = (aBds[0]+aBds[1]) / 2.0;
  aCenter[1] = (aBds[2]+aBds[3]) / 2.0;;
  aCenter[2] = (aBds[4]+aBds[5]) / 2.0;;

  // Now fit the actor bounds in the place bounds by tampering with its
  // transform.
  this->Actor->AddPosition(center[0]-aCenter[0],
                           center[1]-aCenter[1],
                           center[2]-aCenter[2]);
  this->Follower->AddPosition(center[0]-aCenter[0],
                              center[1]-aCenter[1],
                              center[2]-aCenter[2]);

  double s[3], sMin;
  for (int i=0; i < 3; ++i)
    {
    if ( (bounds[2*i+1]-bounds[2*i]) <= 0.0 || (aBds[2*i+1]-aBds[2*i]) <= 0.0 )
      {
      s[i] = VTK_LARGE_FLOAT;
      }
    else
      {
      s[i] = (bounds[2*i+1]-bounds[2*i]) / (aBds[2*i+1]-aBds[2*i]);
      }
    }
  sMin = (s[0]<s[1] ? (s[0]<s[2] ? s[0] : s[2]) : (s[1]<s[2] ? s[1] : s[2]) );

  this->Actor->SetScale(sMin, sMin, sMin);
  this->Follower->SetScale(sMin, sMin, sMin);
}

//-------------------------------------------------------------------------
int vtkTexturedButtonRepresentation
::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
  this->VisibilityOn(); //actor must be on to be picked
  this->Picker->Pick(X,Y,0.0,this->Renderer);
  vtkAssemblyPath *path = this->Picker->GetPath();

  if ( path != NULL )
    {
    this->InteractionState = vtkButtonRepresentation::Inside;
    }
  else
    {
    this->InteractionState = vtkButtonRepresentation::Outside;
    }

  return this->InteractionState;
}

//----------------------------------------------------------------------
void vtkTexturedButtonRepresentation::Highlight(int highlight)
{
  this->Superclass::Highlight(highlight);

  vtkProperty *initialProperty = this->Actor->GetProperty();
  vtkProperty *selectedProperty;

  if ( highlight == vtkButtonRepresentation::HighlightHovering )
    {
    this->Actor->SetProperty(this->HoveringProperty);
    this->Follower->SetProperty(this->HoveringProperty);
    selectedProperty = this->HoveringProperty;
    }
  else if ( highlight == vtkButtonRepresentation::HighlightSelecting )
    {
    this->Actor->SetProperty(this->SelectingProperty);
    this->Follower->SetProperty(this->SelectingProperty);
    selectedProperty = this->SelectingProperty;
    }
  else //if ( highlight == vtkButtonRepresentation::HighlightNormal )
    {
    this->Actor->SetProperty(this->Property);
    this->Follower->SetProperty(this->Property);
    selectedProperty = this->Property;
    }

  if ( selectedProperty != initialProperty )
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------
void vtkTexturedButtonRepresentation::CreateDefaultProperties()
{
  this->Property = vtkProperty::New();
  this->Property->SetColor(1,1,1);

  this->HoveringProperty = vtkProperty::New();
  this->HoveringProperty->SetAmbient(1.0);

  this->SelectingProperty = vtkProperty::New();
  this->SelectingProperty->SetAmbient(0.2);
  this->SelectingProperty->SetAmbientColor(0.2,0.2,0.2);
}

//----------------------------------------------------------------------
void vtkTexturedButtonRepresentation::BuildRepresentation()
{
  // The net effect is to resize the handle
  if ( this->GetMTime() > this->BuildTime ||
       (this->Renderer && this->Renderer->GetVTKWindow() &&
        this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime) )
    {
    // In case follower is being used
    if ( this->FollowCamera )
      {
      this->Follower->VisibilityOn();
      this->Actor->VisibilityOff();
      this->Follower->SetCamera(this->Renderer->GetActiveCamera());
      }
    else
      {
      this->Follower->VisibilityOff();
      this->Actor->VisibilityOn();
      }

    vtkTextureArrayIterator iter = this->TextureArray->find(this->State);
    if ( iter != this->TextureArray->end() )
      {
      this->Texture->SetInput((*iter).second);
      }
    else
      {
      this->Texture->SetInput(NULL);
      }

    this->BuildTime.Modified();
    }
}


//----------------------------------------------------------------------
void vtkTexturedButtonRepresentation::ShallowCopy(vtkProp *prop)
{
  vtkTexturedButtonRepresentation *rep =
    vtkTexturedButtonRepresentation::SafeDownCast(prop);
  if ( rep )
    {
    this->Mapper->ShallowCopy(rep->Mapper);
    this->Property->DeepCopy(rep->Property);
    this->HoveringProperty->DeepCopy(rep->HoveringProperty);
    this->SelectingProperty->DeepCopy(rep->SelectingProperty);

    vtkTextureArrayIterator iter;
    for ( iter=rep->TextureArray->begin();
          iter != rep->TextureArray->end(); ++iter )
      {
      (*this->TextureArray)[(*iter).first] = (*iter).second;
      }
    }
  this->FollowCamera = rep->FollowCamera;

  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
void vtkTexturedButtonRepresentation::
ReleaseGraphicsResources(vtkWindow *win)
{
  this->Actor->ReleaseGraphicsResources(win);
  this->Follower->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkTexturedButtonRepresentation::
RenderOpaqueGeometry(vtkViewport *viewport)
{
  this->BuildRepresentation();

  if ( this->FollowCamera )
    {
    return this->Follower->RenderOpaqueGeometry(viewport);
    }
  else
    {
    return this->Actor->RenderOpaqueGeometry(viewport);
    }
}

//-----------------------------------------------------------------------------
int vtkTexturedButtonRepresentation::
RenderTranslucentPolygonalGeometry(vtkViewport *viewport)
{
  this->BuildRepresentation();

  if ( this->FollowCamera )
    {
    return this->Follower->RenderTranslucentPolygonalGeometry(viewport);
    }
  else
    {
    return this->Actor->RenderTranslucentPolygonalGeometry(viewport);
    }
}
//-----------------------------------------------------------------------------
int vtkTexturedButtonRepresentation::
HasTranslucentPolygonalGeometry()
{
  this->BuildRepresentation();

  if ( this->FollowCamera )
    {
    return this->Follower->HasTranslucentPolygonalGeometry();
    }
  else
    {
    return this->Actor->HasTranslucentPolygonalGeometry();
    }
}


//----------------------------------------------------------------------
double *vtkTexturedButtonRepresentation::GetBounds()
{
  return this->Actor->GetBounds();
}

//----------------------------------------------------------------------
void vtkTexturedButtonRepresentation::GetActors(vtkPropCollection *pc)
{
  if ( this->FollowCamera )
    {
    this->Follower->GetActors(pc);
    }
  else
    {
    this->Actor->GetActors(pc);
    }
}

//----------------------------------------------------------------------
void vtkTexturedButtonRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Button Geometry: " << this->GetButtonGeometry() << "\n";

  os << indent << "Follow Camera: " << (this->FollowCamera ? "On\n" : "Off\n");

  if ( this->Property )
    {
    os << indent << "Property: " << this->Property << "\n";
    }
  else
    {
    os << indent << "Property: (none)\n";
    }

  if ( this->HoveringProperty )
    {
    os << indent << "Hovering Property: " << this->HoveringProperty << "\n";
    }
  else
    {
    os << indent << "Hovering Property: (none)\n";
    }

  if ( this->SelectingProperty )
    {
    os << indent << "Selecting Property: " << this->SelectingProperty << "\n";
    }
  else
    {
    os << indent << "Selecting Property: (none)\n";
    }
}
