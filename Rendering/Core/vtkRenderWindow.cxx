/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRenderWindow.h"

#include "vtkCamera.h"
#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkGraphicsFactory.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPropCollection.h"
#include "vtkRenderTimerLog.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRendererCollection.h"
#include "vtkStereoCompositor.h"
#include "vtkTimerLog.h"
#include "vtkTransform.h"
#include "vtkUnsignedCharArray.h"

#include <cmath>
#include <utility> // for std::swap

//----------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkRenderWindow);

// Construct an instance of  vtkRenderWindow with its screen size
// set to 300x300, borders turned on, positioned at (0,0), double
// buffering turned on, stereo capable off.
vtkRenderWindow::vtkRenderWindow()
{
  this->Borders = 1;
  this->FullScreen = 0;
  this->OldScreen[0] = this->OldScreen[1] = 0;
  this->OldScreen[2] = this->OldScreen[3] = 300;
  this->OldScreen[4] = 1;
  this->DoubleBuffer = 1;
  this->PointSmoothing = 0;
  this->LineSmoothing = 0;
  this->PolygonSmoothing = 0;
  this->StereoRender = 0;
  this->StereoType = VTK_STEREO_RED_BLUE;
  this->StereoCapableWindow = 0;
  this->AlphaBitPlanes = 0;
  this->StencilCapable = 0;
  this->Interactor = nullptr;
  this->DesiredUpdateRate = 0.0001;
  this->StereoBuffer = vtkSmartPointer<vtkUnsignedCharArray>::New();
  this->ResultFrame = vtkSmartPointer<vtkUnsignedCharArray>::New();
  this->SwapBuffers = 1;
  this->AbortRender = 0;
  this->InAbortCheck = 0;
  this->InRender = 0;
  this->NeverRendered = 1;
  this->Renderers = vtkRendererCollection::New();
  this->NumberOfLayers = 1;
  this->CurrentCursor = VTK_CURSOR_DEFAULT;
  this->AnaglyphColorSaturation = 0.65f;
  this->AnaglyphColorMask[0] = 4; // red
  this->AnaglyphColorMask[1] = 3; // cyan

  this->AbortCheckTime = 0.0;
  this->CapturingGL2PSSpecialProps = 0;
  this->MultiSamples = 0;
  this->UseSRGBColorSpace = false;

#ifdef VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN
  this->ShowWindow = false;
  this->UseOffScreenBuffers = true;
#endif
  this->DeviceIndex = 0;
  this->SharedRenderWindow = nullptr;
}

//----------------------------------------------------------------------------
vtkRenderWindow::~vtkRenderWindow()
{
  this->SetInteractor(nullptr);
  this->SetSharedRenderWindow(nullptr);

  if (this->Renderers)
  {
    vtkRenderer* ren;
    vtkCollectionSimpleIterator rit;
    this->Renderers->InitTraversal(rit);
    while ((ren = this->Renderers->GetNextRenderer(rit)))
    {
      ren->SetRenderWindow(nullptr);
    }

    this->Renderers->Delete();
  }
}

void vtkRenderWindow::SetMultiSamples(int val)
{
  if (val == 1)
  {
    val = 0;
  }

  if (val == this->MultiSamples)
  {
    return;
  }

  this->MultiSamples = val;
  this->Modified();
}

//----------------------------------------------------------------------------
// Create an interactor that will work with this renderer.
vtkRenderWindowInteractor* vtkRenderWindow::MakeRenderWindowInteractor()
{
  this->Interactor = vtkRenderWindowInteractor::New();
  this->Interactor->SetRenderWindow(this);
  return this->Interactor;
}

void vtkRenderWindow::SetSharedRenderWindow(vtkRenderWindow* val)
{
  if (this->SharedRenderWindow == val)
  {
    return;
  }

  if (this->SharedRenderWindow)
  {
    // this->ReleaseGraphicsResources();
    this->SharedRenderWindow->UnRegister(this);
  }
  this->SharedRenderWindow = val;
  if (val)
  {
    val->Register(this);
  }
}

//----------------------------------------------------------------------------
// Set the interactor that will work with this renderer.
void vtkRenderWindow::SetInteractor(vtkRenderWindowInteractor* rwi)
{
  if (this->Interactor != rwi)
  {
    // to avoid destructor recursion
    vtkRenderWindowInteractor* temp = this->Interactor;
    this->Interactor = rwi;
    if (temp != nullptr)
    {
      temp->UnRegister(this);
    }
    if (this->Interactor != nullptr)
    {
      this->Interactor->Register(this);

      int isize[2];
      this->Interactor->GetSize(isize);
      if (0 == isize[0] && 0 == isize[1])
      {
        this->Interactor->SetSize(this->GetSize());
      }

      if (this->Interactor->GetRenderWindow() != this)
      {
        this->Interactor->SetRenderWindow(this);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkRenderWindow::SetDesiredUpdateRate(double rate)
{
  vtkRenderer* aren;

  if (this->DesiredUpdateRate != rate)
  {
    vtkCollectionSimpleIterator rsit;
    for (this->Renderers->InitTraversal(rsit); (aren = this->Renderers->GetNextRenderer(rsit));)
    {
      aren->SetAllocatedRenderTime(1.0 / (rate * this->Renderers->GetNumberOfItems()));
    }
    this->DesiredUpdateRate = rate;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkRenderWindow::SetStereoType(int stereoType)
{
  if (this->StereoType == stereoType)
  {
    return;
  }

  this->StereoType = stereoType;
  this->InvokeEvent(vtkCommand::WindowStereoTypeChangedEvent);

  this->Modified();
}

//----------------------------------------------------------------------------
//
// Set the variable that indicates that we want a stereo capable window
// be created. This method can only be called before a window is realized.
//
void vtkRenderWindow::SetStereoCapableWindow(vtkTypeBool capable)
{
  if (this->StereoCapableWindow != capable)
  {
    this->StereoCapableWindow = capable;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
// Turn on stereo rendering
void vtkRenderWindow::SetStereoRender(vtkTypeBool stereo)
{
  if (stereo == this->StereoRender)
  {
    return;
  }

  if (this->StereoCapableWindow || (this->StereoType != VTK_STEREO_CRYSTAL_EYES))
  {
    this->StereoRender = stereo;
    this->Modified();
  }
  else
  {
    vtkWarningMacro(<< "Adjusting stereo mode on a window that does not "
                    << "support stereo type " << this->GetStereoTypeAsString()
                    << " is not possible.");
  }
}

//----------------------------------------------------------------------------
// Ask each renderer owned by this RenderWindow to render its image and
// synchronize this process.
void vtkRenderWindow::Render()
{
  // if we are in the middle of an abort check then return now
  if (this->InAbortCheck)
  {
    return;
  }

  // if we are in a render already from somewhere else abort now
  if (this->InRender)
  {
    return;
  }

  // if SetSize has not yet been called (from a script, possible off
  // screen use, other scenarios?) then call it here with reasonable
  // default values
  if (0 == this->Size[0] && 0 == this->Size[1])
  {
    this->SetSize(300, 300);
  }

  // reset the Abort flag
  this->AbortRender = 0;
  this->InRender = 1;

  vtkDebugMacro(<< "Starting Render Method.\n");
  this->InvokeEvent(vtkCommand::StartEvent, nullptr);

  this->NeverRendered = 0;

  if (this->Interactor && !this->Interactor->GetInitialized())
  {
    this->Interactor->Initialize();
  }

  this->Start(); // Ensure context exists
  vtkRenderTimerLog::ScopedEventLogger event;
  if (this->RenderTimer->GetLoggingEnabled())
  {
    this->RenderTimer->MarkFrame();
    event = this->RenderTimer->StartScopedEvent("vtkRenderWindow::Render");
  }

  this->DoStereoRender();

  this->End(); // restores original bindings

  this->CopyResultFrame();

  // reset the buffer size without freeing any memory.
  this->ResultFrame->Reset();

  // Stop the render timer before invoking the EndEvent.
  event.Stop();

  this->InRender = 0;
  this->InvokeEvent(vtkCommand::EndEvent, nullptr);
}

//----------------------------------------------------------------------------
// Handle rendering the two different views for stereo rendering.
void vtkRenderWindow::DoStereoRender()
{
  vtkCollectionSimpleIterator rsit;

  this->StereoUpdate();

  if (!this->StereoRender || (this->StereoType != VTK_STEREO_RIGHT))
  { // render the left eye
    vtkRenderer* aren;
    for (this->Renderers->InitTraversal(rsit); (aren = this->Renderers->GetNextRenderer(rsit));)
    {
      // Ugly piece of code - we need to know if the camera already
      // exists or not. If it does not yet exist, we must reset the
      // camera here - otherwise it will never be done (missing its
      // oppportunity to be reset in the Render method of the
      // vtkRenderer because it will already exist by that point...)
      if (!aren->IsActiveCameraCreated())
      {
        aren->ResetCamera();
      }
      aren->GetActiveCamera()->SetLeftEye(1);
    }
    this->Renderers->Render();
  }

  if (this->StereoRender)
  {
    this->StereoMidpoint();
    if (this->StereoType != VTK_STEREO_LEFT)
    { // render the right eye
      vtkRenderer* aren;
      for (this->Renderers->InitTraversal(rsit); (aren = this->Renderers->GetNextRenderer(rsit));)
      {
        // Duplicate the ugly code here too. Of course, most
        // times the left eye will have been rendered before
        // the right eye, but it is possible that the user sets
        // everything up and renders just the right eye - so we
        // need this check here too.
        if (!aren->IsActiveCameraCreated())
        {
          aren->ResetCamera();
        }
        if (this->StereoType != VTK_STEREO_FAKE)
        {
          aren->GetActiveCamera()->SetLeftEye(0);
        }
      }
      this->Renderers->Render();
    }
    this->StereoRenderComplete();
  }
}

//----------------------------------------------------------------------------
// Add a renderer to the list of renderers.
void vtkRenderWindow::AddRenderer(vtkRenderer* ren)
{
  if (this->HasRenderer(ren))
  {
    return;
  }
  // we are its parent
  this->MakeCurrent();
  ren->SetRenderWindow(this);
  this->Renderers->AddItem(ren);
  vtkRenderer* aren;
  vtkCollectionSimpleIterator rsit;

  for (this->Renderers->InitTraversal(rsit); (aren = this->Renderers->GetNextRenderer(rsit));)
  {
    aren->SetAllocatedRenderTime(
      1.0 / (this->DesiredUpdateRate * this->Renderers->GetNumberOfItems()));
  }
}

//----------------------------------------------------------------------------
// Remove a renderer from the list of renderers.
void vtkRenderWindow::RemoveRenderer(vtkRenderer* ren)
{
  // we are its parent
  if (ren->GetRenderWindow() == this)
  {
    ren->ReleaseGraphicsResources(this);
    ren->SetRenderWindow(nullptr);
  }
  this->Renderers->RemoveItem(ren);
}

int vtkRenderWindow::HasRenderer(vtkRenderer* ren)
{
  return (ren && this->Renderers->IsItemPresent(ren));
}

//----------------------------------------------------------------------------
int vtkRenderWindow::CheckAbortStatus()
{
  if (!this->InAbortCheck)
  {
    // Only check for abort at most 5 times per second.
    if (vtkTimerLog::GetUniversalTime() - this->AbortCheckTime > 0.2)
    {
      this->InAbortCheck = 1;
      this->InvokeEvent(vtkCommand::AbortCheckEvent, nullptr);
      this->InAbortCheck = 0;
      this->AbortCheckTime = vtkTimerLog::GetUniversalTime();
    }
  }
  return this->AbortRender;
}

//----------------------------------------------------------------------------
void vtkRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Borders: " << (this->Borders ? "On\n" : "Off\n");
  os << indent << "Double Buffer: " << (this->DoubleBuffer ? "On\n" : "Off\n");
  os << indent << "Full Screen: " << (this->FullScreen ? "On\n" : "Off\n");
  os << indent << "Renderers:\n";
  this->Renderers->PrintSelf(os, indent.GetNextIndent());
  os << indent
     << "Stereo Capable Window Requested: " << (this->StereoCapableWindow ? "Yes\n" : "No\n");
  os << indent << "Stereo Render: " << (this->StereoRender ? "On\n" : "Off\n");

  os << indent << "Point Smoothing: " << (this->PointSmoothing ? "On\n" : "Off\n");
  os << indent << "Line Smoothing: " << (this->LineSmoothing ? "On\n" : "Off\n");
  os << indent << "Polygon Smoothing: " << (this->PolygonSmoothing ? "On\n" : "Off\n");
  os << indent << "Abort Render: " << this->AbortRender << "\n";
  os << indent << "Current Cursor: " << this->CurrentCursor << "\n";
  os << indent << "Desired Update Rate: " << this->DesiredUpdateRate << "\n";
  os << indent << "In Abort Check: " << this->InAbortCheck << "\n";
  os << indent << "NeverRendered: " << this->NeverRendered << "\n";
  os << indent << "Interactor: " << this->Interactor << "\n";
  os << indent << "Swap Buffers: " << (this->SwapBuffers ? "On\n" : "Off\n");
  os << indent << "Stereo Type: " << this->GetStereoTypeAsString() << "\n";
  os << indent << "Number of Layers: " << this->NumberOfLayers << "\n";
  os << indent << "AlphaBitPlanes: " << (this->AlphaBitPlanes ? "On" : "Off") << endl;
  os << indent << "UseSRGBColorSpace: " << (this->UseSRGBColorSpace ? "On" : "Off") << endl;

  os << indent << "AnaglyphColorSaturation: " << this->AnaglyphColorSaturation << "\n";
  os << indent << "AnaglyphColorMask: " << this->AnaglyphColorMask[0] << " , "
     << this->AnaglyphColorMask[1] << "\n";

  os << indent << "MultiSamples: " << this->MultiSamples << "\n";
  os << indent << "StencilCapable: " << (this->StencilCapable ? "True" : "False") << endl;
}

//----------------------------------------------------------------------------
// Update the system, if needed, due to stereo rendering. For some stereo
// methods, subclasses might need to switch some hardware settings here.
void vtkRenderWindow::StereoUpdate() {}

//----------------------------------------------------------------------------
// Intermediate method performs operations required between the rendering
// of the left and right eye.
void vtkRenderWindow::StereoMidpoint()
{
  vtkRenderer* aren;
  /* For IceT stereo */
  for (Renderers->InitTraversal(); (aren = Renderers->GetNextItem());)
  {
    aren->StereoMidpoint();
  }
  if ((this->StereoType == VTK_STEREO_RED_BLUE) || (this->StereoType == VTK_STEREO_INTERLACED) ||
    (this->StereoType == VTK_STEREO_DRESDEN) || (this->StereoType == VTK_STEREO_ANAGLYPH) ||
    (this->StereoType == VTK_STEREO_CHECKERBOARD) ||
    (this->StereoType == VTK_STEREO_SPLITVIEWPORT_HORIZONTAL))
  {
    int* size;
    // get the size
    size = this->GetSize();
    // get the data
    this->GetPixelData(0, 0, size[0] - 1, size[1] - 1, !this->DoubleBuffer, this->StereoBuffer);
  }
}

//----------------------------------------------------------------------------
// Handles work required once both views have been rendered when using
// stereo rendering.
void vtkRenderWindow::StereoRenderComplete()
{
  const int* size = this->GetSize();
  switch (this->StereoType)
  {
    case VTK_STEREO_RED_BLUE:
      this->GetPixelData(0, 0, size[0] - 1, size[1] - 1, !this->DoubleBuffer, this->ResultFrame);
      this->StereoCompositor->RedBlue(this->StereoBuffer, this->ResultFrame);
      std::swap(this->StereoBuffer, this->ResultFrame);
      break;

    case VTK_STEREO_ANAGLYPH:
      this->GetPixelData(0, 0, size[0] - 1, size[1] - 1, !this->DoubleBuffer, this->ResultFrame);
      this->StereoCompositor->Anaglyph(this->StereoBuffer, this->ResultFrame,
        this->AnaglyphColorSaturation, this->AnaglyphColorMask);
      std::swap(this->StereoBuffer, this->ResultFrame);
      break;

    case VTK_STEREO_INTERLACED:
      this->GetPixelData(0, 0, size[0] - 1, size[1] - 1, !this->DoubleBuffer, this->ResultFrame);
      this->StereoCompositor->Interlaced(this->StereoBuffer, this->ResultFrame, size);
      std::swap(this->StereoBuffer, this->ResultFrame);
      break;

    case VTK_STEREO_DRESDEN:
      this->GetPixelData(0, 0, size[0] - 1, size[1] - 1, !this->DoubleBuffer, this->ResultFrame);
      this->StereoCompositor->Dresden(this->StereoBuffer, this->ResultFrame, size);
      std::swap(this->StereoBuffer, this->ResultFrame);
      break;

    case VTK_STEREO_CHECKERBOARD:
      this->GetPixelData(0, 0, size[0] - 1, size[1] - 1, !this->DoubleBuffer, this->ResultFrame);
      this->StereoCompositor->Checkerboard(this->StereoBuffer, this->ResultFrame, size);
      std::swap(this->StereoBuffer, this->ResultFrame);
      break;

    case VTK_STEREO_SPLITVIEWPORT_HORIZONTAL:
      this->GetPixelData(0, 0, size[0] - 1, size[1] - 1, !this->DoubleBuffer, this->ResultFrame);
      this->StereoCompositor->SplitViewportHorizontal(this->StereoBuffer, this->ResultFrame, size);
      std::swap(this->StereoBuffer, this->ResultFrame);
      break;
  }

  this->StereoBuffer->Reset();
}

//----------------------------------------------------------------------------
void vtkRenderWindow::CopyResultFrame()
{
  if (this->ResultFrame->GetNumberOfTuples() > 0)
  {
    int* size;

    // get the size
    size = this->GetSize();

    assert(this->ResultFrame->GetNumberOfTuples() == size[0] * size[1]);

    this->SetPixelData(0, 0, size[0] - 1, size[1] - 1, this->ResultFrame, !this->DoubleBuffer);
  }

  // Just before we swap buffers (in case of double buffering), we fire the
  // RenderEvent marking that a render call has concluded successfully. We
  // separate this from EndEvent since some applications may want to put some
  // more elements on the "draw-buffer" before calling the rendering complete.
  // This event gives them that opportunity.
  this->InvokeEvent(vtkCommand::RenderEvent);
  this->Frame();
}

//----------------------------------------------------------------------------
// treat renderWindow and interactor as one object.
// it might be easier if the GetReference count method were redefined.
void vtkRenderWindow::UnRegister(vtkObjectBase* o)
{
  if (this->Interactor && this->Interactor->GetRenderWindow() == this && this->Interactor != o)
  {
    if (this->GetReferenceCount() + this->Interactor->GetReferenceCount() == 3)
    {
      this->vtkObject::UnRegister(o);
      vtkRenderWindowInteractor* tmp = this->Interactor;
      tmp->Register(nullptr);
      this->Interactor->SetRenderWindow(nullptr);
      tmp->UnRegister(nullptr);
      return;
    }
  }

  this->vtkObject::UnRegister(o);
}

//----------------------------------------------------------------------------
const char* vtkRenderWindow::GetRenderLibrary()
{
  return vtkGraphicsFactory::GetRenderLibrary();
}

//----------------------------------------------------------------------------
const char* vtkRenderWindow::GetRenderingBackend()
{
  return "Unknown";
}

//----------------------------------------------------------------------------
void vtkRenderWindow::CaptureGL2PSSpecialProps(vtkCollection* result)
{
  if (result == nullptr)
  {
    vtkErrorMacro(<< "CaptureGL2PSSpecialProps was passed a nullptr pointer.");
    return;
  }

  result->RemoveAllItems();

  if (this->CapturingGL2PSSpecialProps)
  {
    vtkDebugMacro(<< "Called recursively.");
    return;
  }
  this->CapturingGL2PSSpecialProps = 1;

  vtkRenderer* ren;
  for (Renderers->InitTraversal(); (ren = Renderers->GetNextItem());)
  {
    vtkNew<vtkPropCollection> props;
    result->AddItem(props);
    ren->SetGL2PSSpecialPropCollection(props);
  }

  this->Render();

  for (Renderers->InitTraversal(); (ren = Renderers->GetNextItem());)
  {
    ren->SetGL2PSSpecialPropCollection(nullptr);
  }
  this->CapturingGL2PSSpecialProps = 0;
}

// Description: Return the stereo type as a character string.
// when this method was inlined, static linking on BlueGene failed
// (symbol referenced which is defined in discarded section)
const char* vtkRenderWindow::GetStereoTypeAsString()
{
  return vtkRenderWindow::GetStereoTypeAsString(this->StereoType);
}

const char* vtkRenderWindow::GetStereoTypeAsString(int type)
{
  switch (type)
  {
    case VTK_STEREO_CRYSTAL_EYES:
      return "CrystalEyes";
    case VTK_STEREO_RED_BLUE:
      return "RedBlue";
    case VTK_STEREO_LEFT:
      return "Left";
    case VTK_STEREO_RIGHT:
      return "Right";
    case VTK_STEREO_DRESDEN:
      return "DresdenDisplay";
    case VTK_STEREO_ANAGLYPH:
      return "Anaglyph";
    case VTK_STEREO_CHECKERBOARD:
      return "Checkerboard";
    case VTK_STEREO_SPLITVIEWPORT_HORIZONTAL:
      return "SplitViewportHorizontal";
    case VTK_STEREO_FAKE:
      return "Fake";
    case VTK_STEREO_EMULATE:
      return "Emulate";
    default:
      return "";
  }
}

#if !defined(VTK_LEGACY_REMOVE)
vtkTypeBool vtkRenderWindow::GetIsPicking()
{
  VTK_LEGACY_BODY(vtkRenderWindow::GetIsPicking, "VTK 8.3");
  return false;
}
void vtkRenderWindow::SetIsPicking(vtkTypeBool)
{
  VTK_LEGACY_BODY(vtkRenderWindow::SetIsPicking, "VTK 8.3");
}
void vtkRenderWindow::IsPickingOn()
{
  VTK_LEGACY_BODY(vtkRenderWindow::IsPickingOn, "VTK 8.3");
}
void vtkRenderWindow::IsPickingOff()
{
  VTK_LEGACY_BODY(vtkRenderWindow::IsPickingOff, "VTK 8.3");
}
#endif
