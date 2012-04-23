/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProp3DButtonRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProp3DButtonRepresentation.h"
#include "vtkProp3D.h"
#include "vtkPropPicker.h"
#include "vtkProp3DFollower.h"
#include "vtkRenderer.h"
#include "vtkAssemblyPath.h"
#include "vtkInteractorObserver.h"
#include "vtkCoordinate.h"
#include "vtkRenderWindow.h"
#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkMath.h"
#include <map>

vtkStandardNewMacro(vtkProp3DButtonRepresentation);

struct vtkScaledProp
{
  vtkSmartPointer<vtkProp3D> Prop;
  double Origin[3];
  double Scale;
  double Translation[3];
  vtkScaledProp()
    {
      this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
      this->Scale = 1.0;
      this->Translation[0] = this->Translation[1] = this->Translation[2] = 0.0;
    }
};

// Map of textures
class vtkPropArray : public std::map<int,vtkScaledProp> {};
typedef std::map<int,vtkScaledProp>::iterator vtkPropArrayIterator;


//----------------------------------------------------------------------
vtkProp3DButtonRepresentation::vtkProp3DButtonRepresentation()
{
  // Current button representation
  this->CurrentProp = NULL;

  // Following
  this->FollowCamera = 0;
  this->Follower = vtkProp3DFollower::New();

  // List of textures
  this->PropArray = new vtkPropArray;

  this->Picker = vtkPropPicker::New();
  this->Picker->PickFromListOn();
}

//----------------------------------------------------------------------
vtkProp3DButtonRepresentation::~vtkProp3DButtonRepresentation()
{
  this->Follower->Delete();

  delete this->PropArray;

  this->Picker->Delete();
}

//-------------------------------------------------------------------------
void vtkProp3DButtonRepresentation::SetState(int state)
{
  this->Superclass::SetState(state);

  this->CurrentProp = this->GetButtonProp(this->State);
  this->Follower->SetProp3D(this->CurrentProp);

  this->Picker->InitializePickList();
  if ( this->CurrentProp )
    {
    this->Picker->AddPickList(this->CurrentProp);
    }
}

//-------------------------------------------------------------------------
void vtkProp3DButtonRepresentation::
SetButtonProp(int i, vtkProp3D *prop)
{
  if ( i < 0 )
    {
    i = 0;
    }
  if ( i >= this->NumberOfStates )
    {
    i = this->NumberOfStates - 1;
    }

  vtkScaledProp sprop;
  sprop.Prop = prop;

  (*this->PropArray)[i] = sprop;
}


//-------------------------------------------------------------------------
vtkProp3D *vtkProp3DButtonRepresentation::
GetButtonProp(int i)
{
  if ( i < 0 )
    {
    i = 0;
    }
  if ( i >= this->NumberOfStates )
    {
    i = this->NumberOfStates - 1;
    }

  vtkPropArrayIterator iter = this->PropArray->find(i);
  if ( iter != this->PropArray->end() )
    {
    return (*iter).second.Prop;
    }
  else
    {
    return NULL;
    }
}

//-------------------------------------------------------------------------
void vtkProp3DButtonRepresentation::PlaceWidget(double bds[6])
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

  this->SetState(this->State);

  vtkProp3D *prop;
  vtkPropArrayIterator iter;
  for ( iter=this->PropArray->begin(); iter != this->PropArray->end(); ++iter )
    {
    prop = (*iter).second.Prop;

    prop->GetBounds(aBds);
    aCenter[0] = (aBds[0]+aBds[1]) / 2.0;
    aCenter[1] = (aBds[2]+aBds[3]) / 2.0;;
    aCenter[2] = (aBds[4]+aBds[5]) / 2.0;;

    // Now fit the actor bounds in the place bounds by tampering with its
    // transform.
    (*iter).second.Origin[0] = aCenter[0];
    (*iter).second.Origin[1] = aCenter[1];
    (*iter).second.Origin[2] = aCenter[2];

    (*iter).second.Translation[0] = center[0]-aCenter[0];
    (*iter).second.Translation[1] = center[1]-aCenter[1];
    (*iter).second.Translation[2] = center[2]-aCenter[2];

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

    (*iter).second.Scale = sMin;
    }
}

//-------------------------------------------------------------------------
int vtkProp3DButtonRepresentation
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
void vtkProp3DButtonRepresentation::BuildRepresentation()
{
  // The net effect is to resize the handle
  if ( this->GetMTime() > this->BuildTime ||
       (this->Renderer && this->Renderer->GetVTKWindow() &&
        this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime) )
    {
    this->SetState(this->State); //side effect sets CurrentProp
    vtkPropArrayIterator iter = this->PropArray->find(this->State);
    if ( this->CurrentProp == NULL || iter == this->PropArray->end() )
      {
      return;
      }

    // In case follower is being used
    if ( this->FollowCamera )
      {
      this->Follower->SetCamera(this->Renderer->GetActiveCamera());
      this->Follower->SetProp3D(this->CurrentProp);
      this->Follower->SetOrigin((*iter).second.Origin);
      this->Follower->SetPosition((*iter).second.Translation);
      this->Follower->SetScale((*iter).second.Scale);
      }
    else
      {
      this->CurrentProp->SetOrigin((*iter).second.Origin);
      this->CurrentProp->SetPosition((*iter).second.Translation);
      this->CurrentProp->SetScale((*iter).second.Scale);
      }

    this->BuildTime.Modified();
    }
}


//----------------------------------------------------------------------
void vtkProp3DButtonRepresentation::ShallowCopy(vtkProp *prop)
{
  vtkProp3DButtonRepresentation *rep =
    vtkProp3DButtonRepresentation::SafeDownCast(prop);
  if ( rep )
    {
    vtkPropArrayIterator iter;
    for ( iter=rep->PropArray->begin();
          iter != rep->PropArray->end(); ++iter )
      {
      (*this->PropArray)[(*iter).first] = (*iter).second;
      }
    }
  this->FollowCamera = rep->FollowCamera;

  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
void vtkProp3DButtonRepresentation::
ReleaseGraphicsResources(vtkWindow *win)
{
  this->Follower->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkProp3DButtonRepresentation::
RenderVolumetricGeometry(vtkViewport *viewport)
{
  this->BuildRepresentation();

  if ( !this->CurrentProp )
    {
    return 0;
    }

  if ( this->FollowCamera )
    {
    return this->Follower->RenderVolumetricGeometry(viewport);
    }
  else
    {
    return this->CurrentProp->RenderVolumetricGeometry(viewport);
    }
}

//----------------------------------------------------------------------
int vtkProp3DButtonRepresentation::
RenderOpaqueGeometry(vtkViewport *viewport)
{
  this->BuildRepresentation();

  if ( !this->CurrentProp )
    {
    return 0;
    }

  if ( this->FollowCamera )
    {
    return this->Follower->RenderOpaqueGeometry(viewport);
    }
  else
    {
    return this->CurrentProp->RenderOpaqueGeometry(viewport);
    }
}

//-----------------------------------------------------------------------------
int vtkProp3DButtonRepresentation::
RenderTranslucentPolygonalGeometry(vtkViewport *viewport)
{
  this->BuildRepresentation();

  if ( !this->CurrentProp )
    {
    return 0;
    }

  if ( this->FollowCamera )
    {
    return this->Follower->RenderTranslucentPolygonalGeometry(viewport);
    }
  else
    {
    return this->CurrentProp->RenderTranslucentPolygonalGeometry(viewport);
    }
}
//-----------------------------------------------------------------------------
int vtkProp3DButtonRepresentation::
HasTranslucentPolygonalGeometry()
{
  this->BuildRepresentation();

  if ( this->CurrentProp )
    {
    return this->CurrentProp->HasTranslucentPolygonalGeometry();
    }
  else
    {
    return 0;
    }
}


//----------------------------------------------------------------------
double *vtkProp3DButtonRepresentation::GetBounds()
{
  if ( !this->CurrentProp )
    {
    return NULL;
    }

  if ( this->FollowCamera )
    {
    return this->Follower->GetBounds();
    }
  else
    {
    return this->CurrentProp->GetBounds();
    }
}

//----------------------------------------------------------------------
void vtkProp3DButtonRepresentation::GetActors(vtkPropCollection *pc)
{
  if ( this->CurrentProp )
    {
    this->CurrentProp->GetActors(pc);
    }
}

//----------------------------------------------------------------------
void vtkProp3DButtonRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Follow Camera: " << (this->FollowCamera ? "On\n" : "Off\n");

  os << indent << "3D Props: \n";
  vtkPropArrayIterator iter;
  int i;
  for ( i=0, iter=this->PropArray->begin();
        iter != this->PropArray->end(); ++iter, ++i )
    {
    os << indent << "  (" << i << "): " << (*iter).second.Prop << "\n";
    }
}
