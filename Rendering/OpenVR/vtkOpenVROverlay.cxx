/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenVROverlay.h"

#include "vtkCallbackCommand.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkInteractorStyle3D.h"
#include "vtkJPEGReader.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenVRCamera.h"
#include "vtkOpenVRRenderWindow.h"
#include "vtkOpenVRRenderWindowInteractor.h"
#include "vtkPointData.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkTextureObject.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLUtilities.h"
#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#include "vtkOpenVROverlayInternal.h"

#include "OpenVRDashboard.h"

#include <cmath>

vtkStandardNewMacro(vtkOpenVROverlay);

vtkOpenVROverlay::vtkOpenVROverlay()
{
  this->OverlayHandle = 0;
  this->OverlayThumbnailHandle = 0;
  this->OriginalTextureData = nullptr;
  this->CurrentTextureData = nullptr;
  this->LastSpot = nullptr;
  this->SessionName = "";
  this->VRSystem = nullptr;
  this->DashboardImageFileName = "OpenVRDashboard.jpg";
  this->LastCameraPoseIndex = -1;
  this->LastSpotIntensity = 0.3;
  this->ActiveSpotIntensity = 0.3;
}

vtkOpenVROverlay::~vtkOpenVROverlay()
{
  if (this->OriginalTextureData)
  {
    delete[] this->OriginalTextureData;
    this->OriginalTextureData = 0;
  }
  if (this->CurrentTextureData)
  {
    delete[] this->CurrentTextureData;
    this->CurrentTextureData = 0;
  }
}

vtkOpenVRCameraPose* vtkOpenVROverlay::GetSavedCameraPose(int i)
{
  auto p = this->SavedCameraPoses.find(i);
  if (p != this->SavedCameraPoses.end())
  {
    return &(p->second);
  }
  return nullptr;
}

void vtkOpenVROverlay::WriteCameraPoses(ostream& os)
{
  vtkNew<vtkXMLDataElement> topel;
  topel->SetName("CameraPoses");
  for (auto p : this->SavedCameraPoses)
  {
    vtkOpenVRCameraPose& pose = p.second;
    if (pose.Loaded)
    {
      vtkNew<vtkXMLDataElement> el;
      el->SetName("CameraPose");
      el->SetIntAttribute("PoseNumber", p.first);
      el->SetVectorAttribute("Position", 3, pose.Position);
      el->SetDoubleAttribute("Distance", pose.Distance);
      el->SetDoubleAttribute("MotionFactor", pose.MotionFactor);
      el->SetVectorAttribute("Translation", 3, pose.Translation);
      el->SetVectorAttribute("InitialViewUp", 3, pose.PhysicalViewUp);
      el->SetVectorAttribute("InitialViewDirection", 3, pose.PhysicalViewDirection);
      el->SetVectorAttribute("ViewDirection", 3, pose.ViewDirection);
      topel->AddNestedElement(el);
    }
  }

  vtkXMLUtilities::FlattenElement(topel, os);
}

void vtkOpenVROverlay::WriteCameraPoses()
{
  std::string fname = this->GetSessionName();
  fname += "VTKOpenVRCameraPoses.vovrcp";
  vtksys::ofstream os(fname.c_str(), ios::out);
  this->WriteCameraPoses(os);

  os.flush();
  if (os.fail())
  {
    os.close();
    unlink(fname.c_str());
  }
}

void vtkOpenVROverlay::ReadCameraPoses()
{
  std::string fname = this->GetSessionName();
  fname += "VTKOpenVRCameraPoses.vovrcp";

  if (!vtksys::SystemTools::FileExists(fname.c_str()))
  {
    return;
  }

  vtksys::ifstream is(fname.c_str());
  this->ReadCameraPoses(is);
}

void vtkOpenVROverlay::ReadCameraPoses(istream& is)
{
  vtkXMLDataElement* topel = vtkXMLUtilities::ReadElementFromStream(is);

  this->ReadCameraPoses(topel);
  topel->Delete();
}

void vtkOpenVROverlay::ReadCameraPoses(vtkXMLDataElement* topel)
{
  this->SavedCameraPoses.clear();
  if (topel)
  {
    int numPoses = topel->GetNumberOfNestedElements();
    for (size_t i = 0; i < numPoses; i++)
    {
      vtkXMLDataElement* el = topel->GetNestedElement(static_cast<int>(i));
      int poseNum = 0;
      el->GetScalarAttribute("PoseNumber", poseNum);
      el->GetVectorAttribute("Position", 3, this->SavedCameraPoses[poseNum].Position);
      el->GetVectorAttribute("InitialViewUp", 3, this->SavedCameraPoses[poseNum].PhysicalViewUp);
      el->GetVectorAttribute(
        "InitialViewDirection", 3, this->SavedCameraPoses[poseNum].PhysicalViewDirection);
      el->GetVectorAttribute("ViewDirection", 3, this->SavedCameraPoses[poseNum].ViewDirection);
      el->GetVectorAttribute("Translation", 3, this->SavedCameraPoses[poseNum].Translation);
      el->GetScalarAttribute("Distance", this->SavedCameraPoses[poseNum].Distance);
      el->GetScalarAttribute("MotionFactor", this->SavedCameraPoses[poseNum].MotionFactor);
      this->SavedCameraPoses[poseNum].Loaded = true;
    }
  }
}

void vtkOpenVROverlay::SetSavedCameraPose(int i, vtkOpenVRCameraPose* pose)
{
  if (pose)
  {
    this->SavedCameraPoses[i] = *pose;
  }
}

void vtkOpenVROverlay::SaveCameraPose(int slot)
{
  vtkOpenVRCameraPose* pose = &this->SavedCameraPoses[slot];
  vtkRenderer* ren = static_cast<vtkRenderer*>(this->Window->GetRenderers()->GetItemAsObject(0));
  pose->Set(static_cast<vtkOpenVRCamera*>(ren->GetActiveCamera()), this->Window);
  this->InvokeEvent(vtkCommand::SaveStateEvent, reinterpret_cast<void*>(slot));
}

void vtkOpenVROverlay::LoadCameraPose(int slot)
{
  vtkOpenVRCameraPose* pose = this->GetSavedCameraPose(slot);
  if (pose && pose->Loaded)
  {
    this->LastCameraPoseIndex = slot;
    vtkRenderer* ren = static_cast<vtkRenderer*>(this->Window->GetRenderers()->GetItemAsObject(0));
    pose->Apply(static_cast<vtkOpenVRCamera*>(ren->GetActiveCamera()), this->Window);
    ren->ResetCameraClippingRange();
    this->InvokeEvent(vtkCommand::LoadStateEvent, reinterpret_cast<void*>(slot));
  }
}

void vtkOpenVROverlay::LoadNextCameraPose()
{
  if (this->SavedCameraPoses.size() == 0)
  {
    return;
  }

  int nextValue = -1;
  int firstValue = this->LastCameraPoseIndex;
  // find the next pose index in the map
  for (auto p : this->SavedCameraPoses)
  {
    if (p.first < firstValue)
    {
      firstValue = p.first;
    }
    if (p.first > this->LastCameraPoseIndex)
    {
      nextValue = p.first;
      // is there anything lower than nextValue but still larger than current
      for (auto p2 : this->SavedCameraPoses)
      {
        if (p2.first > this->LastCameraPoseIndex && p2.first < nextValue)
        {
          nextValue = p2.first;
        }
      }
      break;
    }
  }

  if (nextValue == -1)
  {
    nextValue = firstValue;
  }

  this->LoadCameraPose(nextValue);
}

void vtkOpenVROverlay::Show()
{
  vr::VROverlay()->ShowOverlay(this->OverlayHandle);
  this->Render();
}

void vtkOpenVROverlay::Hide()
{
  vr::VROverlay()->HideOverlay(this->OverlayHandle);
}

void vtkOpenVROverlay::SetDashboardImageData(vtkJPEGReader* imgReader)
{
  imgReader->SetMemoryBuffer(OpenVRDashboard);
  imgReader->SetMemoryBufferLength(sizeof(OpenVRDashboard));
  imgReader->Update();
}

void vtkOpenVROverlay::Create(vtkOpenVRRenderWindow* win)
{
  if (!vr::VROverlay())
  {
    vtkErrorMacro("Error creating overlay");
    return;
  }

  if (this->OverlayHandle)
  {
    return;
  }

  this->Window = win;

  this->ReadCameraPoses();

  std::string sKey = std::string("VTK OpenVR Settings");
  vr::VROverlayError overlayError = vr::VROverlay()->CreateDashboardOverlay(
    sKey.c_str(), "VTK", &this->OverlayHandle, &this->OverlayThumbnailHandle);
  if (overlayError != vr::VROverlayError_None)
  {
    vtkErrorMacro("Error creating overlay");
    return;
  }

  vr::VROverlay()->SetOverlayFlag(
    this->OverlayHandle, vr::VROverlayFlags_SortWithNonSceneOverlays, true);
  vr::VROverlay()->SetOverlayFlag(this->OverlayHandle, vr::VROverlayFlags_VisibleInDashboard, true);
  vr::VROverlay()->SetOverlayWidthInMeters(this->OverlayHandle, 2.5f);
  vr::VROverlay()->SetOverlayInputMethod(this->OverlayHandle, vr::VROverlayInputMethod_Mouse);

  win->MakeCurrent();

  this->OverlayTexture->SetContext(win);

  // delete any old texture data
  if (this->OriginalTextureData)
  {
    delete[] this->OriginalTextureData;
    this->OriginalTextureData = 0;
  }

  // if dashboard image exists use it
  vtkNew<vtkJPEGReader> imgReader;
  if (!this->DashboardImageFileName.empty() &&
    imgReader->CanReadFile(this->DashboardImageFileName.c_str()))
  {
    imgReader->SetFileName(this->DashboardImageFileName.c_str());
    imgReader->Update();
  }
  else // use compiled in dashboard
  {
    this->SetDashboardImageData(imgReader);
  }

  vtkImageData* id = imgReader->GetOutput();
  int dims[3];
  id->GetDimensions(dims);
  int numC = id->GetPointData()->GetScalars()->GetNumberOfComponents();

  this->OriginalTextureData = new unsigned char[dims[0] * dims[1] * 4];
  this->CurrentTextureData = new unsigned char[dims[0] * dims[1] * 4];
  unsigned char* dataPtr = this->OriginalTextureData;
  unsigned char* inPtr =
    static_cast<unsigned char*>(id->GetPointData()->GetScalars()->GetVoidPointer(0));
  for (int j = 0; j < dims[1]; j++)
  {
    for (int i = 0; i < dims[0]; i++)
    {
      *(dataPtr++) = *(inPtr++);
      *(dataPtr++) = *(inPtr++);
      *(dataPtr++) = *(inPtr++);
      *(dataPtr++) = (numC == 4 ? *(inPtr++) : 255.0);
    }
  }
  memcpy(this->CurrentTextureData, this->OriginalTextureData, dims[0] * dims[1] * 4);
  this->OverlayTexture->Create2DFromRaw(dims[0], dims[1], 4, VTK_UNSIGNED_CHAR,
    const_cast<void*>(static_cast<const void* const>(this->OriginalTextureData)));

  this->SetupSpots();

  int width = this->OverlayTexture->GetWidth();
  int height = this->OverlayTexture->GetHeight();
  vr::HmdVector2_t vecWindowSize = { static_cast<float>(width), static_cast<float>(height) };
  vr::VROverlay()->SetOverlayMouseScale(this->OverlayHandle, &vecWindowSize);
}

void vtkOpenVROverlay::Render()
{
  // skip rendering if the overlay isn't visible
  if (!vr::VROverlay() ||
    (!vr::VROverlay()->IsOverlayVisible(this->OverlayHandle) &&
      !vr::VROverlay()->IsOverlayVisible(this->OverlayThumbnailHandle)))
  {
    return;
  }

  this->Window->MakeCurrent();
  int dims[2];
  dims[0] = this->OverlayTexture->GetWidth();
  dims[1] = this->OverlayTexture->GetHeight();
  this->OverlayTexture->Create2DFromRaw(dims[0], dims[1], 4, VTK_UNSIGNED_CHAR,
    const_cast<void*>(static_cast<const void* const>(this->CurrentTextureData)));
  this->OverlayTexture->Bind();
  GLuint unTexture = this->OverlayTexture->GetHandle();
  if (unTexture != 0)
  {
    vr::Texture_t texture = { (void*)(uintptr_t)unTexture, vr::TextureType_OpenGL,
      vr::ColorSpace_Auto };
    vr::VROverlay()->SetOverlayTexture(this->OverlayHandle, &texture);
  }
}

void vtkOpenVROverlay::MouseMoved(int x, int y)
{
  // did we leave the last active spot
  bool leftSpot = false;
  if (this->LastSpot &&
    (x < this->LastSpot->xmin || x > this->LastSpot->xmax || y < this->LastSpot->ymin ||
      y > this->LastSpot->ymax))
  {
    leftSpot = true;
    vtkOpenVROverlaySpot* spot = this->LastSpot;
    this->LastSpot = nullptr;
    this->UpdateSpot(spot);
  }

  // if we are in a spot already and did not leave
  // just return
  if (this->LastSpot)
  {
    return;
  }

  // did we enter a new spot?
  bool enteredSpot = false;

  std::vector<vtkOpenVROverlaySpot>::iterator it = this->Spots.begin();
  for (; !enteredSpot && it != this->Spots.end(); ++it)
  {
    if (x >= it->xmin && x <= it->xmax && y >= it->ymin && y <= it->ymax)
    {
      // if we are not already in this spot
      if (this->LastSpot != &(*it))
      {
        this->LastSpot = &(*it);
        enteredSpot = true;
        this->UpdateSpot(this->LastSpot);
      }
    }
  }

  if (!leftSpot && !enteredSpot)
  {
    return;
  }

  this->Render();
}

void vtkOpenVROverlay::UpdateSpot(vtkOpenVROverlaySpot* spot)
{
  int dims[2];
  dims[0] = this->OverlayTexture->GetWidth();
  dims[1] = this->OverlayTexture->GetHeight();
  unsigned char* currPtr = this->CurrentTextureData;
  unsigned char* origPtr = this->OriginalTextureData;

  float shift = 0.0;
  float scale = 1.0;
  if (spot->Active)
  {
    shift = this->ActiveSpotIntensity * 255.0;
    scale = 1.0 - this->ActiveSpotIntensity;
  }
  if (spot == this->LastSpot)
  {
    shift = this->LastSpotIntensity * 255.0;
    scale = 1.0 - this->LastSpotIntensity;
  }

  for (int j = spot->ymin; j <= spot->ymax; j++)
  {
    unsigned char* dataPtr = currPtr + (j * dims[0] + spot->xmin) * 4;
    unsigned char* inPtr = origPtr + (j * dims[0] + spot->xmin) * 4;
    for (int i = spot->xmin; i <= spot->xmax; i++)
    {
      *(dataPtr++) = static_cast<unsigned char>(scale * *(inPtr++) + shift);
      *(dataPtr++) = static_cast<unsigned char>(scale * *(inPtr++) + shift);
      *(dataPtr++) = static_cast<unsigned char>(scale * *(inPtr++) + shift);
      dataPtr++;
      inPtr++;
    }
  }
}

void vtkOpenVROverlay::MouseButtonPress(int x, int y)
{
  this->MouseMoved(x, y);
  if (this->LastSpot && this->LastSpot->Callback)
  {
    this->LastSpot->Callback->Execute(this, vtkCommand::LeftButtonPressEvent, this->Window);
  }
}

void vtkOpenVROverlay::MouseButtonRelease(int, int)
{
  if (this->LastSpot && this->LastSpot->Callback)
  {
    this->LastSpot->Callback->Execute(this, vtkCommand::LeftButtonReleaseEvent, this->Window);
  }
}

void vtkOpenVROverlay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
