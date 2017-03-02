/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenVRDefaultOverlay.h"

#include "vtkCallbackCommand.h"
#include "vtkInteractorStyle3D.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenVRCamera.h"
#include "vtkOpenVRRenderWindow.h"
#include "vtkOpenVRRenderWindowInteractor.h"
#include "vtkOpenVRRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkTextureObject.h"

#include <cmath>
#include <sstream>

#include "vtkOpenVROverlayInternal.h"

vtkStandardNewMacro(vtkOpenVRDefaultOverlay);

void handleMotionFactor(vtkObject *caller, unsigned long eid,
                         void *clientdata, void *calldata)
{
  if (eid == vtkCommand::LeftButtonReleaseEvent)
  {
    vtkOpenVRDefaultOverlay *ovl = static_cast<vtkOpenVRDefaultOverlay *>(caller);
    vtkOpenVRRenderWindow *win = static_cast<vtkOpenVRRenderWindow *>(calldata);
    vtkInteractorStyle3D *is = static_cast<vtkInteractorStyle3D *>(win->GetInteractor()->GetInteractorStyle());
    int option = *(reinterpret_cast<int*>(&clientdata));
    double mf = 0.1;
    switch (option)
    {
      case 0: mf = 0.0; break;
      case 1: mf = 0.1; break;
      case 2: mf = 1.0; break;
      case 3: mf = 10.0; break;
      case 4: mf = 100.0; break;
    }
    is->SetDollyMotionFactor(mf);

    // turn off all motion spots
    std::vector<vtkOpenVROverlaySpot> &spots = ovl->GetSpots();
    std::vector<vtkOpenVROverlaySpot>::iterator it = spots.begin();
    for (; it != spots.end(); ++it)
    {
      if (it->Group == "motion" && it->Active)
      {
        it->Active = false;
        ovl->UpdateSpot(&(*it));
      }
    }
    // turn on this one
    ovl->GetLastSpot()->Active = true;
    ovl->Render();
  }
}

void handleScaleFactor(vtkObject *caller, unsigned long eid,
                         void *clientdata, void *calldata)
{
  if (eid == vtkCommand::LeftButtonReleaseEvent)
  {
    vtkOpenVRDefaultOverlay *ovl = static_cast<vtkOpenVRDefaultOverlay *>(caller);
    vtkOpenVRRenderWindow *win = static_cast<vtkOpenVRRenderWindow *>(calldata);
    vtkInteractorStyle3D *is = static_cast<vtkInteractorStyle3D *>(win->GetInteractor()->GetInteractorStyle());
    int option = *(reinterpret_cast<int*>(&clientdata));
    double mf = 1.0;
    switch (option)
    {
      case 0: mf = 0.01; break;
      case 1: mf = 0.1; break;
      case 2: mf = 1.0; break;
      case 3: mf = 10.0; break;
      case 4: mf = 100.0; break;
    }
    vtkRenderer *ren = static_cast<vtkRenderer *>(
      win->GetRenderers()->GetItemAsObject(0));
    is->SetDistance(ren->GetActiveCamera(), 1.0/mf);
    ren->ResetCameraClippingRange();
    ovl->Render();
  }
}

void handleSaveCamera(vtkObject *caller, unsigned long eid,
                         void *clientdata, void * /*calldata*/)
{
  if (eid == vtkCommand::LeftButtonReleaseEvent)
  {
    vtkOpenVRDefaultOverlay *ovl = static_cast<vtkOpenVRDefaultOverlay *>(caller);
    int option = *(reinterpret_cast<int*>(&clientdata));

    std::ostringstream s;
    s << "Really save the camera pose into slot " << option << " ?";
    if ( vr::VROverlay()->ShowMessageOverlay(s.str().c_str(), "Confirmation", "Yes", "No", NULL, NULL)
      == vr::VRMessageOverlayResponse_ButtonPress_0)
    {
      ovl->SaveCameraPose(option-1);
    }
  }
}

void handleLoadCamera(vtkObject *caller, unsigned long eid,
                         void *clientdata, void * /* calldata */)
{
  if (eid == vtkCommand::LeftButtonReleaseEvent)
  {
    vtkOpenVRDefaultOverlay *ovl = static_cast<vtkOpenVRDefaultOverlay *>(caller);
    ovl->ReadCameraPoses();
    int option = *(reinterpret_cast<int*>(&clientdata));
    ovl->LoadCameraPose(option-1);
  }
}

void handleShowFloor(vtkObject *caller, unsigned long eid,
                     void *clientdata, void *calldata)
{
  if (eid == vtkCommand::LeftButtonReleaseEvent)
  {
    vtkOpenVRDefaultOverlay *ovl = static_cast<vtkOpenVRDefaultOverlay *>(caller);
    int option = *(reinterpret_cast<int*>(&clientdata));
    vtkOpenVRRenderWindow *win = static_cast<vtkOpenVRRenderWindow *>(calldata);
    vtkOpenVRRenderer *ren = static_cast<vtkOpenVRRenderer *>(
      win->GetRenderers()->GetItemAsObject(0));
    ren->SetShowFloor(option != 0);

    // turn off all floor spots
    std::vector<vtkOpenVROverlaySpot> &spots = ovl->GetSpots();
    std::vector<vtkOpenVROverlaySpot>::iterator it = spots.begin();
    for (; it != spots.end(); ++it)
    {
      if (it->Group == "floor" && it->Active)
      {
        it->Active = false;
        ovl->UpdateSpot(&(*it));
      }
    }
    // turn on this one
    ovl->GetLastSpot()->Active = true;
    ovl->Render();
  }
}

void handleSetViewUp(vtkObject * /* caller */, unsigned long eid,
                     void *clientdata, void * calldata)
{
  if (eid == vtkCommand::LeftButtonReleaseEvent)
  {
    vtkOpenVRRenderWindow *win = static_cast<vtkOpenVRRenderWindow *>(calldata);
    int option = *(reinterpret_cast<int*>(&clientdata));
    switch (option)
    {
      case 0: win->SetInitialViewUp(-1,0,0); win->SetInitialViewDirection(0,1,0); break;
      case 1: win->SetInitialViewUp(1,0,0); win->SetInitialViewDirection(0,1,0); break;
      case 2: win->SetInitialViewUp(0,-1,0); win->SetInitialViewDirection(0,0,1); break;
      case 3: win->SetInitialViewUp(0,1,0); win->SetInitialViewDirection(0,0,1); break;
      case 4: win->SetInitialViewUp(0,0,-1); win->SetInitialViewDirection(0,1,0); break;
      case 5: win->SetInitialViewUp(0,0,1); win->SetInitialViewDirection(0,1,0); break;
    }
 }
}

vtkOpenVRDefaultOverlay::vtkOpenVRDefaultOverlay()
{
}

vtkOpenVRDefaultOverlay::~vtkOpenVRDefaultOverlay()
{
}

void vtkOpenVRDefaultOverlay::SetupSpots()
{
  // add default spots
  for (int i = 0; i < 6; i++)
  {
    vtkCallbackCommand *cc = vtkCallbackCommand::New();
    cc->SetClientData(reinterpret_cast<char *>(i));
    cc->SetCallback(handleSetViewUp);
    this->Spots.push_back(vtkOpenVROverlaySpot(
      913 + i*91.5, 913 + i*91.5 + 90, 522, 608, cc));
    cc->Delete();
  }
  for (int i = 0; i < 5; i++)
  {
    vtkCallbackCommand *cc = vtkCallbackCommand::New();
    cc->SetClientData(reinterpret_cast<char *>(i));
    cc->SetCallback(handleMotionFactor);
    vtkOpenVROverlaySpot spot(
      913 + i*109.8, 913 + i*109.8 + 108, 48, 134, cc);
    spot.Group = "motion";
    spot.GroupId = i;
    this->Spots.push_back(spot);
    cc->Delete();
  }
  for (int i = 0; i < 5; i++)
  {
    vtkCallbackCommand *cc = vtkCallbackCommand::New();
    cc->SetClientData(reinterpret_cast<char *>(i));
    cc->SetCallback(handleScaleFactor);
    vtkOpenVROverlaySpot spot(
      913 + i*109.8, 913 + i*109.8 + 108, 284, 370, cc);
    spot.Group = "scale";
    spot.GroupId = i;
    this->Spots.push_back(spot);
    cc->Delete();
  }

  for (int i = 0; i < 2; i++)
  {
    vtkCallbackCommand *cc = vtkCallbackCommand::New();
    cc->SetClientData(reinterpret_cast<char *>(i));
    cc->SetCallback(handleShowFloor);
    vtkOpenVROverlaySpot spot(
      600 + i*136, 600 + i*136 + 135, 530, 601, cc);
    spot.GroupId = i;
    spot.Group = "floor";
    this->Spots.push_back(spot);
    cc->Delete();
  }
  for (int i = 0; i < 8; i++)
  {
    vtkCallbackCommand *cc = vtkCallbackCommand::New();
    cc->SetClientData(reinterpret_cast<char *>(i+1));
    cc->SetCallback(handleLoadCamera);
    this->Spots.push_back(vtkOpenVROverlaySpot(
      37 + i*104.5, 37 + i*104.5 + 103, 284, 370, cc));
    cc->Delete();
  }
  for (int i = 0; i < 8; i++)
  {
    vtkCallbackCommand *cc = vtkCallbackCommand::New();
    cc->SetClientData(reinterpret_cast<char *>(i+1));
    cc->SetCallback(handleSaveCamera);
    this->Spots.push_back(vtkOpenVROverlaySpot(
      37 + i*104.5, 37 + i*104.5 + 103, 48, 134, cc));
    cc->Delete();
  }
}

void vtkOpenVRDefaultOverlay::Render()
{
  // update settings
  vtkOpenVRRenderer *ren = static_cast<vtkOpenVRRenderer *>(
    this->Window->GetRenderers()->GetItemAsObject(0));
  bool showFloor = ren->GetShowFloor();

  // set correct floor sports
  std::vector<vtkOpenVROverlaySpot> &spots = this->GetSpots();
  std::vector<vtkOpenVROverlaySpot>::iterator it = spots.begin();
  for (; it != spots.end(); ++it)
  {
    if (it->Group == "floor")
    {
      it->Active = ((it->GroupId == 1) == showFloor);
      this->UpdateSpot(&(*it));
    }
  }

  this->Superclass::Render();
}

void vtkOpenVRDefaultOverlay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
