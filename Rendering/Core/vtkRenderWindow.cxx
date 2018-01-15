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
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPropCollection.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRendererCollection.h"
#include "vtkRenderTimerLog.h"
#include "vtkTimerLog.h"
#include "vtkTransform.h"
#include "vtkGraphicsFactory.h"
#include "vtkObjectFactory.h"

#include <cmath>

//----------------------------------------------------------------------------
// Use the vtkAbstractObjectFactoryNewMacro to allow the object factory overrides.
vtkAbstractObjectFactoryNewMacro(vtkRenderWindow)
//----------------------------------------------------------------------------

// Construct an instance of  vtkRenderWindow with its screen size
// set to 300x300, borders turned on, positioned at (0,0), double
// buffering turned on, stereo capable off.
vtkRenderWindow::vtkRenderWindow()
{
  this->IsPicking = 0;
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
  this->AccumulationBuffer = nullptr;
  this->AccumulationBufferSize = 0;
  this->DesiredUpdateRate = 0.0001;
  this->ResultFrame = nullptr;
  this->SwapBuffers = 1;
  this->AbortRender = 0;
  this->InAbortCheck = 0;
  this->InRender = 0;
  this->NeverRendered = 1;
  this->Renderers = vtkRendererCollection::New();
  this->NumberOfLayers = 1;
  this->CurrentCursor = VTK_CURSOR_DEFAULT;
  this->AnaglyphColorSaturation = 0.65f;
  this->AnaglyphColorMask[0] = 4;  // red
  this->AnaglyphColorMask[1] = 3;  // cyan

  this->AbortCheckTime = 0.0;
  this->CapturingGL2PSSpecialProps = 0;
  this->MultiSamples = 0;
  this->UseSRGBColorSpace = false;

#ifdef VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN
  this->OffScreenRendering = 1;
#endif
  this->DeviceIndex = 0;
  this->SharedRenderWindow = nullptr;
}

//----------------------------------------------------------------------------
vtkRenderWindow::~vtkRenderWindow()
{
  this->SetInteractor(nullptr);
  this->SetSharedRenderWindow(nullptr);

  delete [] this->AccumulationBuffer;
  this->AccumulationBuffer = nullptr;
  this->AccumulationBufferSize = 0;

  delete [] this->ResultFrame;
  this->ResultFrame = nullptr;

  if (this->Renderers)
  {
    vtkCollectionSimpleIterator rsit;
    this->Renderers->InitTraversal(rsit);
    vtkRenderer *aren;
    while ( (aren = this->Renderers->GetNextRenderer(rsit)) )
    {
      if (aren->GetRenderWindow() == this)
      {
        vtkErrorMacro("Window destructed with renderer still associated with it!");
      }
    }

    this->Renderers->Delete();
  }
}

//----------------------------------------------------------------------------
// Create an interactor that will work with this renderer.
vtkRenderWindowInteractor *vtkRenderWindow::MakeRenderWindowInteractor()
{
  this->Interactor = vtkRenderWindowInteractor::New();
  this->Interactor->SetRenderWindow(this);
  return this->Interactor;
}

void vtkRenderWindow::SetSharedRenderWindow(vtkRenderWindow *val)
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
void vtkRenderWindow::SetInteractor(vtkRenderWindowInteractor *rwi)
{
  if (this->Interactor != rwi)
  {
    // to avoid destructor recursion
    vtkRenderWindowInteractor *temp = this->Interactor;
    this->Interactor = rwi;
    if (temp != nullptr) {temp->UnRegister(this);}
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
  vtkRenderer *aren;

  if (this->DesiredUpdateRate != rate)
  {
    vtkCollectionSimpleIterator rsit;
    for (this->Renderers->InitTraversal(rsit);
         (aren = this->Renderers->GetNextRenderer(rsit)); )
    {
      aren->SetAllocatedRenderTime(1.0/
                                  (rate*this->Renderers->GetNumberOfItems()));
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

  if (this->StereoCapableWindow ||
      (this->StereoType != VTK_STEREO_CRYSTAL_EYES))
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
  this->InvokeEvent(vtkCommand::StartEvent,nullptr);

  this->NeverRendered = 0;

  if ( this->Interactor && ! this->Interactor->GetInitialized() )
  {
    this->Interactor->Initialize();
  }

  vtkRenderTimerLog::ScopedEventLogger event;
  if (this->RenderTimer->GetLoggingEnabled())
  {
    this->Start(); // Ensure context exists
    this->RenderTimer->MarkFrame();
    event = this->RenderTimer->StartScopedEvent("vtkRenderWindow::Render");
  }

  this->DoStereoRender();
  this->CopyResultFrame();

  delete [] this->ResultFrame;
  this->ResultFrame = nullptr;

  // Stop the render timer before invoking the EndEvent.
  event.Stop();

  this->InRender = 0;
  this->InvokeEvent(vtkCommand::EndEvent,nullptr);
}

//----------------------------------------------------------------------------
// Handle rendering the two different views for stereo rendering.
void vtkRenderWindow::DoStereoRender()
{
  vtkCollectionSimpleIterator rsit;

  this->Start();
  this->StereoUpdate();

  if (this->StereoType != VTK_STEREO_RIGHT)
  { // render the left eye
    vtkRenderer *aren;
    for (this->Renderers->InitTraversal(rsit);
         (aren = this->Renderers->GetNextRenderer(rsit)); )
    {
      // Ugly piece of code - we need to know if the camera already
      // exists or not. If it does not yet exist, we must reset the
      // camera here - otherwise it will never be done (missing its
      // oppportunity to be reset in the Render method of the
      // vtkRenderer because it will already exist by that point...)
      if ( !aren->IsActiveCameraCreated() )
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
      vtkRenderer *aren;
      for (this->Renderers->InitTraversal(rsit);
           (aren = this->Renderers->GetNextRenderer(rsit)); )
      {
        // Duplicate the ugly code here too. Of course, most
        // times the left eye will have been rendered before
        // the right eye, but it is possible that the user sets
        // everything up and renders just the right eye - so we
        // need this check here too.
        if ( !aren->IsActiveCameraCreated() )
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
void vtkRenderWindow::AddRenderer(vtkRenderer *ren)
{
  if (this->HasRenderer(ren))
  {
    return;
  }
  // we are its parent
  this->MakeCurrent();
  ren->SetRenderWindow(this);
  this->Renderers->AddItem(ren);
  vtkRenderer *aren;
  vtkCollectionSimpleIterator rsit;

  for (this->Renderers->InitTraversal(rsit);
       (aren = this->Renderers->GetNextRenderer(rsit)); )
  {
    aren->SetAllocatedRenderTime
      (1.0/(this->DesiredUpdateRate*this->Renderers->GetNumberOfItems()));
  }
}

//----------------------------------------------------------------------------
// Remove a renderer from the list of renderers.
void vtkRenderWindow::RemoveRenderer(vtkRenderer *ren)
{
  // we are its parent
  if (ren->GetRenderWindow() == this)
  {
    ren->ReleaseGraphicsResources(this);
    ren->SetRenderWindow(nullptr);
  }
  this->Renderers->RemoveItem(ren);
}

int vtkRenderWindow::HasRenderer(vtkRenderer *ren)
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
      this->InvokeEvent(vtkCommand::AbortCheckEvent,nullptr);
      this->InAbortCheck = 0;
      this->AbortCheckTime = vtkTimerLog::GetUniversalTime();
    }
  }
  return this->AbortRender;
}

//----------------------------------------------------------------------------
void vtkRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Borders: " << (this->Borders ? "On\n":"Off\n");
  os << indent << "IsPicking: " << (this->IsPicking ? "On\n":"Off\n");
  os << indent << "Double Buffer: " << (this->DoubleBuffer ? "On\n":"Off\n");
  os << indent << "Full Screen: " << (this->FullScreen ? "On\n":"Off\n");
  os << indent << "Renderers:\n";
  this->Renderers->PrintSelf(os,indent.GetNextIndent());
  os << indent << "Stereo Capable Window Requested: "
     << (this->StereoCapableWindow ? "Yes\n":"No\n");
  os << indent << "Stereo Render: "
     << (this->StereoRender ? "On\n":"Off\n");

  os << indent << "Point Smoothing: "
     << (this->PointSmoothing ? "On\n":"Off\n");
  os << indent << "Line Smoothing: "
     << (this->LineSmoothing ? "On\n":"Off\n");
  os << indent << "Polygon Smoothing: "
     << (this->PolygonSmoothing ? "On\n":"Off\n");
  os << indent << "Abort Render: " << this->AbortRender << "\n";
  os << indent << "Current Cursor: " << this->CurrentCursor << "\n";
  os << indent << "Desired Update Rate: " << this->DesiredUpdateRate << "\n";
  os << indent << "In Abort Check: " << this->InAbortCheck << "\n";
  os << indent << "NeverRendered: " << this->NeverRendered << "\n";
  os << indent << "Interactor: " << this->Interactor << "\n";
  os << indent << "Swap Buffers: " << (this->SwapBuffers ? "On\n":"Off\n");
  os << indent << "Stereo Type: " << this->GetStereoTypeAsString() << "\n";
  os << indent << "Number of Layers: " << this->NumberOfLayers << "\n";
  os << indent << "AccumulationBuffer Size " << this->AccumulationBufferSize << "\n";
  os << indent << "AlphaBitPlanes: " << (this->AlphaBitPlanes ? "On" : "Off")
     << endl;
  os << indent << "UseSRGBColorSpace: " << (this->UseSRGBColorSpace ? "On" : "Off")
     << endl;

  os << indent << "AnaglyphColorSaturation: "
     << this->AnaglyphColorSaturation << "\n";
  os << indent << "AnaglyphColorMask: "
     << this->AnaglyphColorMask[0] << " , "
     << this->AnaglyphColorMask[1] << "\n";

  os << indent << "MultiSamples: " << this->MultiSamples << "\n";
  os << indent << "StencilCapable: " <<
    (this->StencilCapable ? "True" : "False") << endl;
}

//----------------------------------------------------------------------------
// Update the system, if needed, due to stereo rendering. For some stereo
// methods, subclasses might need to switch some hardware settings here.
void vtkRenderWindow::StereoUpdate(void)
{
}

//----------------------------------------------------------------------------
// Intermediate method performs operations required between the rendering
// of the left and right eye.
void vtkRenderWindow::StereoMidpoint(void)
{
  vtkRenderer * aren;
  /* For IceT stereo */
  for (Renderers->InitTraversal() ; (aren = Renderers->GetNextItem()) ; )
  {
    aren->StereoMidpoint();
  }
  if ((this->StereoType == VTK_STEREO_RED_BLUE) ||
      (this->StereoType == VTK_STEREO_INTERLACED) ||
      (this->StereoType == VTK_STEREO_DRESDEN) ||
      (this->StereoType == VTK_STEREO_ANAGLYPH) ||
      (this->StereoType == VTK_STEREO_CHECKERBOARD) ||
      (this->StereoType == VTK_STEREO_SPLITVIEWPORT_HORIZONTAL))
  {
    int *size;
    // get the size
    size = this->GetSize();
    // get the data
    this->StereoBuffer = this->GetPixelData(0,0,size[0]-1,size[1]-1,!this->DoubleBuffer);
  }
}

//----------------------------------------------------------------------------
// Handles work required once both views have been rendered when using
// stereo rendering.
void vtkRenderWindow::StereoRenderComplete(void)
{
  switch (this->StereoType)
  {
    case VTK_STEREO_RED_BLUE:
    {
      unsigned char *buff;
      unsigned char *p1, *p2, *p3;
      unsigned char* result;
      int *size;
      int x,y;
      int res;

      // get the size
      size = this->GetSize();
      // get the data
      buff = this->GetPixelData(0,0,size[0]-1,size[1]-1,!this->DoubleBuffer);
      p1 = this->StereoBuffer;
      p2 = buff;

      // allocate the result
      result = new unsigned char [size[0]*size[1]*3];
      if (!result)
      {
        vtkErrorMacro(<<"Couldn't allocate memory for RED BLUE stereo.");
        return;
      }
      p3 = result;

      // now merge the two images
      for (x = 0; x < size[0]; x++)
      {
        for (y = 0; y < size[1]; y++)
        {
          res = p1[0] + p1[1] + p1[2];
          p3[0] = res/3;
          res = p2[0] + p2[1] + p2[2];
          p3[1] = 0;
          p3[2] = res/3;

          p1 += 3;
          p2 += 3;
          p3 += 3;
        }
      }
      this->ResultFrame = result;
      delete [] this->StereoBuffer;
      this->StereoBuffer = nullptr;
      delete [] buff;
    }
      break;
    case VTK_STEREO_ANAGLYPH:
    {
      unsigned char *buff;
      unsigned char *p0, *p1, *p2;
      unsigned char* result;
      int *size;
      int x,y;
      int m0, m1, ave0, ave1;
      int avecolor[256][3], satcolor[256];
      float a;

      // get the size
      size = this->GetSize();
      // get the data
      buff = this->GetPixelData(0,0,size[0]-1,size[1]-1,!this->DoubleBuffer);
      p0 = this->StereoBuffer;
      p1 = buff;

      // allocate the result
      result = new unsigned char [size[0]*size[1]*3];
      if (!result)
      {
        vtkErrorMacro(<<"Couldn't allocate memory for ANAGLYPH stereo.");
        return;
      }
      p2 = result;

      // build some tables
      a = this->AnaglyphColorSaturation;
      m0 = this->AnaglyphColorMask[0];
      m1 = this->AnaglyphColorMask[1];

      for(x = 0; x < 256; x++)
      {
        avecolor[x][0] = int((1.0-a)*x*0.3086);
        avecolor[x][1] = int((1.0-a)*x*0.6094);
        avecolor[x][2] = int((1.0-a)*x*0.0820);

        satcolor[x] = int(a*x);
      }

      // now merge the two images
      for (x = 0; x < size[0]; x++)
      {
        for (y = 0; y < size[1]; y++)
        {
            ave0 = avecolor[p0[0]][0] + avecolor[p0[1]][1] + avecolor[p0[2]][2];
            ave1 = avecolor[p1[0]][0] + avecolor[p1[1]][1] + avecolor[p1[2]][2];
            if (m0 & 0x4)
            {
              p2[0] = satcolor[p0[0]] + ave0;
            }
            if (m0 & 0x2)
            {
              p2[1] = satcolor[p0[1]] + ave0;
            }
            if (m0 & 0x1)
            {
              p2[2] = satcolor[p0[2]] + ave0;
            }
            if (m1 & 0x4)
            {
              p2[0] = satcolor[p1[0]] + ave1;
            }
            if (m1 & 0x2)
            {
              p2[1] = satcolor[p1[1]] + ave1;
            }
            if (m1 & 0x1)
            {
              p2[2] = satcolor[p1[2]] + ave1;
            }
            p0 += 3;
            p1 += 3;
            p2 += 3;
        }
      }
      this->ResultFrame = result;
      delete [] this->StereoBuffer;
      this->StereoBuffer = nullptr;
      delete [] buff;
    }
      break;
    case VTK_STEREO_INTERLACED:
    {
      unsigned char *buff;
      unsigned char *p1, *p2, *p3;
      unsigned char* result;
      int *size, line;
      int x,y;

      // get the size
      size = this->GetSize();
      // get the data
      buff = this->GetPixelData(0,0,size[0]-1,size[1]-1,!this->DoubleBuffer);
      p1 = this->StereoBuffer;
      p2 = buff;
      line = size[0] * 3;

      // allocate the result
      result = new unsigned char [size[0]*size[1]*3];
      if (!result)
      {
        vtkErrorMacro(<<"Couldn't allocate memory for interlaced stereo.");
        return;
      }

      // now merge the two images
      p3 = result;
      for (y = 0; y < size[1]; y += 2)
      {
        for (x = 0; x < size[0]; x++)
        {
          *p3++ = *p1++;
          *p3++ = *p1++;
          *p3++ = *p1++;
        }
        // skip a line
        p3 += line;
        p1 += line;
      }
      // now the other eye
      p3 = result + line;
      p2 += line;
      for (y = 1; y < size[1]; y += 2)
      {
        for (x = 0; x < size[0]; x++)
        {
          *p3++ = *p2++;
          *p3++ = *p2++;
          *p3++ = *p2++;
        }
        // skip a line
        p3 += line;
        p2 += line;
      }

      this->ResultFrame = result;
      delete [] this->StereoBuffer;
      this->StereoBuffer = nullptr;
      delete [] buff;
    }
      break;

    case VTK_STEREO_DRESDEN:
    {
      unsigned char *buff;
      unsigned char *p1, *p2, *p3;
      unsigned char* result;
      int *size;
      int x,y;

      // get the size
      size = this->GetSize();
      // get the data
      buff = this->GetPixelData(0,0,size[0]-1,size[1]-1,!this->DoubleBuffer);
      p1 = this->StereoBuffer;
      p2 = buff;

      // allocate the result
      result = new unsigned char [size[0]*size[1]*3];
      if (!result)
      {
        vtkErrorMacro(
          <<"Couldn't allocate memory for dresden display stereo.");
        return;
      }

      // now merge the two images
      p3 = result;

      for (y = 0; y < size[1]; y++ )
      {
        for (x = 0; x < size[0]; x+=2)
        {
          *p3++ = *p1++;
          *p3++ = *p1++;
          *p3++ = *p1++;

          p3+=3;
          p1+=3;
        }
        if( size[0] % 2 == 1 )
        {
          p3 -= 3;
          p1 -= 3;
        }
      }

      // now the other eye
      p3 = result + 3;
      p2 += 3;

      for (y = 0; y < size[1]; y++)
      {
        for (x = 1; x < size[0]; x+=2)
        {
          *p3++ = *p2++;
          *p3++ = *p2++;
          *p3++ = *p2++;

          p3+=3;
          p2+=3;
        }
        if( size[0] % 2 == 1 )
        {
          p3 += 3;
          p2 += 3;
        }
      }

      this->ResultFrame = result;
      delete [] this->StereoBuffer;
      this->StereoBuffer = nullptr;
      delete [] buff;
    }
      break;

    case VTK_STEREO_CHECKERBOARD: {
      unsigned char *left, *right;
      unsigned char *sleft, *sright;
      int *size;

      // get the size
      size = this->GetSize();
      // get the data
      sleft = this->StereoBuffer;
      sright = this->GetPixelData(0, 0, size[0] - 1, size[1] - 1,
                                  !this->DoubleBuffer);

      // copy right pixels onto the left pixel buffer
      for(int y = 0; y < size[1]; y = y + 1) {
        // set up the pointers
        // right starts on x = 1 on even scanlines
        // right starts on x = 0 on odd scanlines
        if (y % 2 == 0) {
          left = sleft + y * 3 * size[0] + 3;
          right = sright + y * 3 * size[0] + 3;
        }
        else {
          left = sleft + y * 3 * size[0];
          right = sright + y * 3 * size[0];
        }

        // skip every other pixel
        for(int x = (y + 1) % 2; x < size[0]; x = x + 2) {
          *left++ = *right++;
          *left++ = *right++;
          *left++ = *right++;

          // skip pixel
          left = left + 3;
          right = right + 3;
        }
      }

      // cleanup
      this->ResultFrame = sleft;

      this->StereoBuffer = nullptr;
      delete [] sright;
    }
    break;
    case VTK_STEREO_SPLITVIEWPORT_HORIZONTAL:
    {
      unsigned char *left, *leftTemp, *right;
      unsigned char *sleft, *sright;
      int *size;

      // get the size
      size = this->GetSize();

      // get the data
      sleft = this->StereoBuffer;
      sright = this->GetPixelData(0, 0, size[0] - 1, size[1] - 1,
                                  !this->DoubleBuffer);

      int midX = static_cast<int>(size[0] / 2.0);

      // If the row size is even, reduce the row copy by
      // one. Otherwise the pointer will overflow when we fill the
      // right hand part of the stereo.
      if (size[0] % 2 == 0)
      {
        midX--;
      }

      int offsetX = static_cast<int>(ceil(size[0] / 2.0));

      // copy pixel data
      for (int y = 0; y <= (size[1] - 1); ++y)
      {
          for (int x = 1; x <= midX; ++x)
          {
            left = sleft + (x * 3) + (y * size[0] * 3);
            leftTemp = sleft + ((2 * x) * 3) + (y * size[0] * 3);
            *left++ = *leftTemp++;
            *left++ = *leftTemp++;
            *left++ = *leftTemp++;
          }
      }

      for (int y = 0; y <= (size[1] - 1); ++y)
      {
          for (int x = 0; x < midX; ++x)
          {
            left = sleft + ((x + offsetX) * 3) + (y * size[0] * 3);
            right = sright + ((2 * x) * 3) + (y * size[0] * 3);
            *left++ = *right++;
            *left++ = *right++;
            *left++ = *right++;
          }
      }

      // cleanup
      this->ResultFrame = sleft;

      this->StereoBuffer = nullptr;
      delete [] sright;
    }
    break;
  }
}

//----------------------------------------------------------------------------
void vtkRenderWindow::CopyResultFrame(void)
{
  if (this->ResultFrame)
  {
    int *size;

    // get the size
    size = this->GetSize();
    this->SetPixelData(0,0,size[0]-1,size[1]-1,this->ResultFrame,!this->DoubleBuffer);
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
void vtkRenderWindow::UnRegister(vtkObjectBase *o)
{
  if (this->Interactor && this->Interactor->GetRenderWindow() == this &&
      this->Interactor != o)
  {
    if (this->GetReferenceCount() + this->Interactor->GetReferenceCount() == 3)
    {
      this->vtkObject::UnRegister(o);
      vtkRenderWindowInteractor *tmp = this->Interactor;
      tmp->Register(nullptr);
      this->Interactor->SetRenderWindow(nullptr);
      tmp->UnRegister(nullptr);
      return;
    }
  }

  this->vtkObject::UnRegister(o);
}

//----------------------------------------------------------------------------
const char *vtkRenderWindow::GetRenderLibrary()
{
  return vtkGraphicsFactory::GetRenderLibrary();
}

//----------------------------------------------------------------------------
const char *vtkRenderWindow::GetRenderingBackend()
{
  return "Unknown";
}

//----------------------------------------------------------------------------
void vtkRenderWindow::CaptureGL2PSSpecialProps(vtkCollection *result)
{
  if (result == nullptr)
  {
    vtkErrorMacro(<<"CaptureGL2PSSpecialProps was passed a nullptr pointer.");
    return;
  }

  result->RemoveAllItems();

  if (this->CapturingGL2PSSpecialProps)
  {
    vtkDebugMacro(<<"Called recursively.")
    return;
  }
  this->CapturingGL2PSSpecialProps = 1;

  vtkRenderer *ren;
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
const char *vtkRenderWindow::GetStereoTypeAsString()
{
  switch ( this->StereoType )
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
    default:
      return "";
  }
}
