/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleTerrain.cxx
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
#include "vtkInteractorStyleTerrain.h"
#include "vtkMath.h"
#include "vtkSphereSource.h"
#include "vtkExtractEdges.h"
#include "vtkPolyDataMapper.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkInteractorStyleTerrain, "1.1");
vtkStandardNewMacro(vtkInteractorStyleTerrain);

//----------------------------------------------------------------------------
vtkInteractorStyleTerrain::vtkInteractorStyleTerrain()
{
  this->LatLongLines = 0;

  this->LatLongSphere = NULL;
  this->LatLongExtractEdges = NULL;
  this->LatLongMapper = NULL;
  this->LatLongActor = NULL;

  this->MotionFactor   = 10.0;
}

//----------------------------------------------------------------------------
vtkInteractorStyleTerrain::~vtkInteractorStyleTerrain()
{
  if (this->LatLongSphere != NULL) 
    {
    this->LatLongSphere->Delete();
    }

  if (this->LatLongMapper != NULL) 
    {
    this->LatLongMapper->Delete();
    }

  if (this->LatLongActor != NULL) 
    {
    this->LatLongActor->Delete();
    }

  if (this->LatLongExtractEdges != NULL) 
    {
    this->LatLongExtractEdges->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTerrain::OnMouseMove() 
{ 
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  switch (this->State) 
    {
    case VTKIS_ROTATE:
      this->FindPokedRenderer(x, y);
      this->Rotate();
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;

    case VTKIS_PAN:
      this->FindPokedRenderer(x, y);
      this->Pan();
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;

    case VTKIS_DOLLY:
      this->FindPokedRenderer(x, y);
      this->Dolly();
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTerrain::OnLeftButtonDown () 
{ 
  this->FindPokedRenderer(this->Interactor->GetEventPosition()[0], 
                          this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  this->StartRotate();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTerrain::OnLeftButtonUp ()
{
  switch (this->State) 
    {
    case VTKIS_ROTATE:
      this->EndRotate();
      break;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTerrain::OnMiddleButtonDown () 
{
  this->FindPokedRenderer(this->Interactor->GetEventPosition()[0], 
                          this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  this->StartPan();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTerrain::OnMiddleButtonUp ()
{
  switch (this->State) 
    {
    case VTKIS_PAN:
      this->EndPan();
      break;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTerrain::OnRightButtonDown () 
{
  this->FindPokedRenderer(this->Interactor->GetEventPosition()[0], 
                          this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  this->StartDolly();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTerrain::OnRightButtonUp ()
{
  switch (this->State) 
    {
    case VTKIS_DOLLY:
      this->EndDolly();
      break;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTerrain::Rotate()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;

  int dx = -(rwi->GetEventPosition()[0] - rwi->GetLastEventPosition()[0]);
  int dy = -(rwi->GetEventPosition()[1] - rwi->GetLastEventPosition()[1]);

  int *size = this->CurrentRenderer->GetRenderWindow()->GetSize();

  float a = (float)dx / size[0] * 180.0f;
  float e = (float)dy / size[1] * 180.0f;
  
  if (rwi->GetShiftKey()) 
    {
    if (fabs((float)dx) >= fabs((float)dy))
      {
      e = 0.0;
      }
    else
      {
      a = 0.0;
      }
    }

  // Move the camera. 
  // Make sure that we don't hit the north pole singularity.

  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  camera->Azimuth(a);

  double dop[3], vup[3];
  camera->GetDirectionOfProjection(dop);
  vtkMath::Normalize(dop);
  camera->GetViewUp(vup);
  vtkMath::Normalize(vup);

  double angle = acos(vtkMath::Dot(dop,vup)) / vtkMath::DegreesToRadians();
  if ((angle+e) > 179.0f) 
    {
    e = 0.0f;
    }
  else if ((angle+e) < 1.0f)
    {
    e = 0.0f;
    }

  camera->Elevation(e);

  if (this->AutoAdjustCameraClippingRange)
    {
    this->CurrentRenderer->ResetCameraClippingRange();
    }

  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTerrain::Pan()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;

  // Get the vector of motion

  float fp[3], focalPoint[3], pos[3], v[3], p1[3], p2[3];

  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  camera->GetPosition(pos);
  camera->GetFocalPoint(fp);

  this->ComputeWorldToDisplay(fp[0], fp[1], fp[2], 
                              focalPoint);

  this->ComputeDisplayToWorld((double)rwi->GetEventPosition()[0], 
                              (double)rwi->GetEventPosition()[1],
                              focalPoint[2],
                              p1);

  this->ComputeDisplayToWorld((double)rwi->GetLastEventPosition()[0],
                              (double)rwi->GetLastEventPosition()[1], 
                              focalPoint[2], 
                              p2);
    
  for (int i=0; i<3; i++)
    {
    v[i] = p2[i] - p1[i];
    pos[i] += v[i];
    fp[i] += v[i];
    }

  camera->SetPosition(pos);
  camera->SetFocalPoint(fp);
    
  if (rwi->GetLightFollowCamera()) 
    {
    this->CurrentRenderer->UpdateLightsGeometryToFollowCamera();
    }
    
  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTerrain::Dolly()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;
  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  float *center = this->CurrentRenderer->GetCenter();

  int dy = rwi->GetEventPosition()[1] - rwi->GetLastEventPosition()[1];
  double dyf = this->MotionFactor * (double)(dy) / (double)(center[1]);
  double zoomFactor = pow((double)1.1, dyf);
  
  if (camera->GetParallelProjection())
    {
    camera->SetParallelScale(camera->GetParallelScale()/zoomFactor);
    }
  else
    {
    camera->Dolly(zoomFactor);
    if (this->AutoAdjustCameraClippingRange)
      {
      this->CurrentRenderer->ResetCameraClippingRange();
      }
    }

  if (rwi->GetLightFollowCamera()) 
    {
    this->CurrentRenderer->UpdateLightsGeometryToFollowCamera();
    }
  
  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTerrain::OnChar()
{
  vtkRenderWindowInteractor *rwi = this->Interactor;

  switch (rwi->GetKeyCode())
    {
    case 'l':
      this->FindPokedRenderer(rwi->GetEventPosition()[0],
                              rwi->GetEventPosition()[1]);
      this->CreateLatLong();
      if (this->LatLongLines) 
        {
        this->LatLongLinesOff();
        }
      else 
        {
        float bounds[6];
        this->CurrentRenderer->ComputeVisiblePropBounds(bounds);
        float radius = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                            (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                            (bounds[5]-bounds[4])*(bounds[5]-bounds[4])) / 2.0;
        this->LatLongSphere->SetRadius(radius);
        this->LatLongSphere->SetCenter((bounds[0]+bounds[1])/2.0,
                                       (bounds[2]+bounds[3])/2.0,
                                       (bounds[4]+bounds[5])/2.0);        
        this->LatLongLinesOn();
        }
      this->SelectRepresentation();
      rwi->Render();
      break;

    default:
      this->Superclass::OnChar();
      break;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTerrain::CreateLatLong()
{
  if (this->LatLongSphere == NULL)
    {
    this->LatLongSphere = vtkSphereSource::New();
    this->LatLongSphere->SetPhiResolution(13);
    this->LatLongSphere->SetThetaResolution(25);
    this->LatLongSphere->LatLongTessellationOn();
    }

  if (this->LatLongExtractEdges == NULL)
    {
    this->LatLongExtractEdges = vtkExtractEdges::New();
    this->LatLongExtractEdges->SetInput(this->LatLongSphere->GetOutput());
    }

  if (this->LatLongMapper == NULL)
    {
    this->LatLongMapper = vtkPolyDataMapper::New();
    this->LatLongMapper->SetInput(this->LatLongExtractEdges->GetOutput());
    }

  if (this->LatLongActor == NULL)
    {
    this->LatLongActor = vtkActor::New();
    this->LatLongActor->SetMapper(this->LatLongMapper);
    this->LatLongActor->PickableOff();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTerrain::SelectRepresentation()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  this->CurrentRenderer->RemoveActor(this->LatLongActor);
  
  if (this->LatLongLines)
    {
    this->CurrentRenderer->AddActor(this->LatLongActor);
    this->LatLongActor->VisibilityOn();
    }
  else
    {
    this->LatLongActor->VisibilityOff();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTerrain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Latitude/Longitude Lines: " 
     << (this->LatLongLines ? "On\n" : "Off\n");
}


