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
#include "vtkTimerLog.h"
#include "vtkTransform.h"
#include "vtkGraphicsFactory.h"
#include "vtkObjectFactory.h"

#include <cmath>

#ifdef VTK_OPENGL2
class vtkPainterDeviceAdapter : public vtkObject {};
#else
#include "vtkPainterDeviceAdapter.h"
#endif

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
  this->StereoStatus = 0;
  this->StereoCapableWindow = 0;
  this->AlphaBitPlanes = 0;
  this->StencilCapable = 0;
  this->Interactor = NULL;
  this->AAFrames = 0;
  this->FDFrames = 0;
  this->UseConstantFDOffsets = 0;
  this->ConstantFDOffsets[0] = NULL;
  this->ConstantFDOffsets[1] = NULL;
  this->SubFrames = 0;
  this->AccumulationBuffer = NULL;
  this->AccumulationBufferSize = 0;
  this->CurrentSubFrame = 0;
  this->DesiredUpdateRate = 0.0001;
  this->ResultFrame = NULL;
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
  this->PainterDeviceAdapter = NULL;
#ifndef VTK_LEGACY_REMOVE
  this->ReportGraphicErrors = 0; // false
#endif
  this->AbortCheckTime = 0.0;
  this->CapturingGL2PSSpecialProps = 0;
  this->MultiSamples = 0;

#ifdef VTK_USE_OFFSCREEN
  this->OffScreenRendering = 1;
#endif
}

//----------------------------------------------------------------------------
vtkRenderWindow::~vtkRenderWindow()
{
  this->SetInteractor(NULL);

  delete [] this->AccumulationBuffer;
  this->AccumulationBuffer = NULL;
  this->AccumulationBufferSize = 0;

  delete [] this->ResultFrame;
  this->ResultFrame = NULL;

  for (int i = 0; i < 2; ++i)
    {
    delete [] this->ConstantFDOffsets[i];
    this->ConstantFDOffsets[i] = NULL;
    }

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

  if (this->PainterDeviceAdapter)
    {
    this->PainterDeviceAdapter->Delete();
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

//----------------------------------------------------------------------------
// Set the interactor that will work with this renderer.
void vtkRenderWindow::SetInteractor(vtkRenderWindowInteractor *rwi)
{
  if (this->Interactor != rwi)
    {
    // to avoid destructor recursion
    vtkRenderWindowInteractor *temp = this->Interactor;
    this->Interactor = rwi;
    if (temp != NULL) {temp->UnRegister(this);}
    if (this->Interactor != NULL)
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
void vtkRenderWindow::SetFDFrames(int fdFrames)
{
  if (this->FDFrames != fdFrames)
    {
    this->FDFrames = fdFrames;

    for (int i = 0; i < 2; i++)
      {
      delete [] this->ConstantFDOffsets[i];
      this->ConstantFDOffsets[i] = NULL;

      if (this->FDFrames > 0)
        {
        this->ConstantFDOffsets[i] = new double[this->FDFrames];
        for (int fi = 0; fi < this->FDFrames; fi++)
          {
          this->ConstantFDOffsets[i][fi] = vtkMath::Random();
          }
        }
      }

    vtkDebugMacro(<< this->GetClassName() << " (" << this
      << "): setting FDFrames to " << fdFrames);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkRenderWindow::SetSubFrames(int subFrames)
{
  if (this->SubFrames != subFrames)
    {
    this->SubFrames = subFrames;
    if (this->CurrentSubFrame >= this->SubFrames)
      {
      this->CurrentSubFrame = 0;
      }
    vtkDebugMacro(<< this->GetClassName() << " (" << this
      << "): setting SubFrames to " << subFrames);
    this->Modified();
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
//
// Set the variable that indicates that we want a stereo capable window
// be created. This method can only be called before a window is realized.
//
void vtkRenderWindow::SetStereoCapableWindow(int capable)
{
  if (this->StereoCapableWindow != capable)
    {
    this->StereoCapableWindow = capable;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// Turn on stereo rendering
void vtkRenderWindow::SetStereoRender(int stereo)
{
  if (stereo == this->StereoRender)
    {
    return;
    }

  if (this->StereoCapableWindow ||
      (!this->StereoCapableWindow
       && this->StereoType != VTK_STEREO_CRYSTAL_EYES))
    {
    if (stereo != this->StereoRender)
      {
      this->StereoRender = stereo;
      this->Modified();
      }
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
  int *size;
  int x,y;
  float *p1;

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
  this->InvokeEvent(vtkCommand::StartEvent,NULL);

  this->NeverRendered = 0;

  if ( this->Interactor && ! this->Interactor->GetInitialized() )
    {
    this->Interactor->Initialize();
    }

  // CAUTION:
  // This method uses this->GetSize() and allocates buffers using that size.
  // Remember that GetSize() will returns a size scaled by the TileScale factor.
  // We should use GetActualSize() when we don't want the size to be scaled.

  // if there is a reason for an AccumulationBuffer
  if ( this->SubFrames || this->AAFrames || this->FDFrames)
    {
    // check the current size
    size = this->GetSize();
    unsigned int bufferSize = 3*size[0]*size[1];
    // If there is not a buffer or the size is too small
    // re-allocate it
    if( !this->AccumulationBuffer
        || bufferSize > this->AccumulationBufferSize)
      {
      // it is OK to delete null, no sense in two if's
      delete [] this->AccumulationBuffer;
      // Save the size of the buffer
      this->AccumulationBufferSize = 3*size[0]*size[1];
      this->AccumulationBuffer = new float [this->AccumulationBufferSize];
      memset(this->AccumulationBuffer,0,this->AccumulationBufferSize*sizeof(float));
      }
    }

  // handle any sub frames
  if (this->SubFrames)
    {
    // get the size
    size = this->GetSize();

    // draw the images
    this->DoAARender();

    // now accumulate the images
    if ((!this->AAFrames) && (!this->FDFrames))
      {
      p1 = this->AccumulationBuffer;
      unsigned char *p2;
      unsigned char *p3;
      if (this->ResultFrame)
        {
        p2 = this->ResultFrame;
        }
      else
        {
        p2 = this->GetPixelData(0,0,size[0]-1,size[1]-1,!this->DoubleBuffer);
        }
      p3 = p2;
      for (y = 0; y < size[1]; y++)
        {
        for (x = 0; x < size[0]; x++)
          {
          *p1 += *p2; p1++; p2++;
          *p1 += *p2; p1++; p2++;
          *p1 += *p2; p1++; p2++;
          }
        }
      delete [] p3;
      }

    // if this is the last sub frame then convert back into unsigned char
    this->CurrentSubFrame++;
    if (this->CurrentSubFrame >= this->SubFrames)
      {
      double num;
      unsigned char *p2 = new unsigned char [3*size[0]*size[1]];

      num = this->SubFrames;
      if (this->AAFrames)
        {
        num *= this->AAFrames;
        }
      if (this->FDFrames)
        {
        num *= this->FDFrames;
        }

      this->ResultFrame = p2;
      p1 = this->AccumulationBuffer;
      for (y = 0; y < size[1]; y++)
        {
        for (x = 0; x < size[0]; x++)
          {
          *p2 = static_cast<unsigned char>(*p1/num);
          p1++;
          p2++;
          *p2 = static_cast<unsigned char>(*p1/num);
          p1++;
          p2++;
          *p2 = static_cast<unsigned char>(*p1/num);
          p1++;
          p2++;
          }
        }

      this->CurrentSubFrame = 0;
      this->CopyResultFrame();

      // free any memory
      delete [] this->AccumulationBuffer;
      this->AccumulationBuffer = NULL;
      }
    }
  else // no subframes
    {
    // get the size
    size = this->GetSize();

    this->DoAARender();
    // if we had some accumulation occur
    if (this->AccumulationBuffer)
      {
      double num;
      unsigned char *p2 = new unsigned char [3*size[0]*size[1]];

      if (this->AAFrames)
        {
        num = this->AAFrames;
        }
      else
        {
        num = 1;
        }
      if (this->FDFrames)
        {
        num *= this->FDFrames;
        }

      this->ResultFrame = p2;
      p1 = this->AccumulationBuffer;
      for (y = 0; y < size[1]; y++)
        {
        for (x = 0; x < size[0]; x++)
          {
          *p2 = static_cast<unsigned char>(*p1/num);
          p1++;
          p2++;
          *p2 = static_cast<unsigned char>(*p1/num);
          p1++;
          p2++;
          *p2 = static_cast<unsigned char>(*p1/num);
          p1++;
          p2++;
          }
        }

      delete [] this->AccumulationBuffer;
      this->AccumulationBuffer = NULL;
      }

    this->CopyResultFrame();
    }

  delete [] this->ResultFrame;
  this->ResultFrame = NULL;

  this->InRender = 0;
  this->InvokeEvent(vtkCommand::EndEvent,NULL);
}

//----------------------------------------------------------------------------
// Handle rendering any antialiased frames.
void vtkRenderWindow::DoAARender()
{
  int i;

  // handle any anti aliasing
  if (this->AAFrames)
    {
    int *size;
    int x,y;
    float *p1;
    vtkRenderer *aren;
    vtkCamera *acam;
    double *dpoint;
    double offsets[2];
    double origfocus[4];
    double worldOffset[3];

    // get the size
    size = this->GetSize();

    origfocus[3] = 1.0;

    for (i = 0; i < this->AAFrames; i++)
      {
      // jitter the cameras
      offsets[0] = vtkMath::Random() - 0.5;
      offsets[1] = vtkMath::Random() - 0.5;

      vtkCollectionSimpleIterator rsit;
      for (this->Renderers->InitTraversal(rsit);
           (aren = this->Renderers->GetNextRenderer(rsit)); )
        {
        acam = aren->GetActiveCamera();

        // calculate the amount to jitter
        acam->GetFocalPoint(origfocus);
        aren->SetWorldPoint(origfocus);
        aren->WorldToDisplay();
        dpoint = aren->GetDisplayPoint();
        aren->SetDisplayPoint(dpoint[0] + offsets[0],
                              dpoint[1] + offsets[1],
                              dpoint[2]);
        aren->DisplayToWorld();
        dpoint = aren->GetWorldPoint();
        dpoint[0] /= dpoint[3];
        dpoint[1] /= dpoint[3];
        dpoint[2] /= dpoint[3];
        acam->SetFocalPoint(dpoint);

        worldOffset[0] = dpoint[0] - origfocus[0];
        worldOffset[1] = dpoint[1] - origfocus[1];
        worldOffset[2] = dpoint[2] - origfocus[2];

        acam->GetPosition(dpoint);
        acam->SetPosition(dpoint[0]+worldOffset[0],
                          dpoint[1]+worldOffset[1],
                          dpoint[2]+worldOffset[2]);
        }

      // draw the images
      this->DoFDRender();

      // restore the jitter to normal
      for (this->Renderers->InitTraversal(rsit);
           (aren = this->Renderers->GetNextRenderer(rsit)); )
        {
        acam = aren->GetActiveCamera();

        // calculate the amount to jitter
        acam->GetFocalPoint(origfocus);
        aren->SetWorldPoint(origfocus);
        aren->WorldToDisplay();
        dpoint = aren->GetDisplayPoint();
        aren->SetDisplayPoint(dpoint[0] - offsets[0],
                              dpoint[1] - offsets[1],
                              dpoint[2]);
        aren->DisplayToWorld();
        dpoint = aren->GetWorldPoint();
        dpoint[0] /= dpoint[3];
        dpoint[1] /= dpoint[3];
        dpoint[2] /= dpoint[3];
        acam->SetFocalPoint(dpoint);

        worldOffset[0] = dpoint[0] - origfocus[0];
        worldOffset[1] = dpoint[1] - origfocus[1];
        worldOffset[2] = dpoint[2] - origfocus[2];

        acam->GetPosition(dpoint);
        acam->SetPosition(dpoint[0]+worldOffset[0],
                          dpoint[1]+worldOffset[1],
                          dpoint[2]+worldOffset[2]);
        }


      // now accumulate the images
      p1 = this->AccumulationBuffer;
      if (!this->FDFrames)
        {
        unsigned char *p2;
        unsigned char *p3;
        if (this->ResultFrame)
          {
          p2 = this->ResultFrame;
          }
        else
          {
          p2 = this->GetPixelData(0,0,size[0]-1,size[1]-1,!this->DoubleBuffer);
          }
        p3 = p2;
        for (y = 0; y < size[1]; y++)
          {
          for (x = 0; x < size[0]; x++)
            {
            *p1 += static_cast<float>(*p2);
            p1++;
            p2++;
            *p1 += static_cast<float>(*p2);
            p1++;
            p2++;
            *p1 += static_cast<float>(*p2);
            p1++;
            p2++;
            }
          }
        delete [] p3;
        }
      }
    }
  else
    {
    this->DoFDRender();
    }
}

//----------------------------------------------------------------------------
// Handle rendering any focal depth frames.
void vtkRenderWindow::DoFDRender()
{
  int i;

  // handle any focal depth
  if (this->FDFrames)
    {
    int *size;
    int x,y;
    unsigned char *p2;
    unsigned char *p3;
    float *p1;
    vtkRenderer *aren;
    vtkCamera *acam;
    double focalDisk;
    double *vpn, *dpoint;
    double vec[3];
    vtkTransform *aTrans = vtkTransform::New();
    double offsets[2];
    double *orig;
    vtkCollectionSimpleIterator rsit;

    // get the size
    size = this->GetSize();

    orig = new double [3*this->Renderers->GetNumberOfItems()];

    for (i = 0; i < this->FDFrames; i++)
      {
      int j = 0;

      if (this->UseConstantFDOffsets)
        {
        offsets[0] = this->ConstantFDOffsets[0][i]; // radius
        offsets[1] = this->ConstantFDOffsets[1][i]*360.0; // angle
        }
      else
        {
        offsets[0] = vtkMath::Random(); // radius
        offsets[1] = vtkMath::Random()*360.0; // angle
        }

      // store offsets for each renderer
      for (this->Renderers->InitTraversal(rsit);
           (aren = this->Renderers->GetNextRenderer(rsit)); )
        {
        acam = aren->GetActiveCamera();
        focalDisk = acam->GetFocalDisk()*offsets[0];

        vpn = acam->GetViewPlaneNormal();
        aTrans->Identity();
        aTrans->Scale(focalDisk,focalDisk,focalDisk);
        aTrans->RotateWXYZ(-offsets[1],vpn[0],vpn[1],vpn[2]);
        aTrans->TransformVector(acam->GetViewUp(),vec);

        dpoint = acam->GetPosition();

        // store the position for later
        memcpy(orig + j*3,dpoint,3 * sizeof (double));
        j++;

        acam->SetPosition(dpoint[0]+vec[0],
                          dpoint[1]+vec[1],
                          dpoint[2]+vec[2]);
        }

      // draw the images
      this->DoStereoRender();

      // restore the jitter to normal
      j = 0;
      for (this->Renderers->InitTraversal(rsit);
           (aren = this->Renderers->GetNextRenderer(rsit)); )
        {
        acam = aren->GetActiveCamera();
        acam->SetPosition(orig + j*3);
        j++;
        }

      // get the pixels for accumulation
      // now accumulate the images
      p1 = this->AccumulationBuffer;
      if (this->ResultFrame)
        {
        p2 = this->ResultFrame;
        }
      else
        {
        p2 = this->GetPixelData(0,0,size[0]-1,size[1]-1,!this->DoubleBuffer);
        }
      p3 = p2;
      for (y = 0; y < size[1]; y++)
        {
        for (x = 0; x < size[0]; x++)
          {
          *p1 += static_cast<float>(*p2);
          p1++;
          p2++;
          *p1 += static_cast<float>(*p2);
          p1++;
          p2++;
          *p1 += static_cast<float>(*p2);
          p1++;
          p2++;
          }
        }
      delete [] p3;
      }

    // free memory
    delete [] orig;
    aTrans->Delete();
    }
  else
    {
    this->DoStereoRender();
    }
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
        aren->GetActiveCamera()->SetLeftEye(0);
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
    ren->SetRenderWindow(NULL);
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
      this->InvokeEvent(vtkCommand::AbortCheckEvent,NULL);
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
  os << indent << "Anti Aliased Frames: " << this->AAFrames << "\n";
  os << indent << "Abort Render: " << this->AbortRender << "\n";
  os << indent << "Current Cursor: " << this->CurrentCursor << "\n";
  os << indent << "Desired Update Rate: " << this->DesiredUpdateRate << "\n";
  os << indent << "Focal Depth Frames: " << this->FDFrames << "\n";
  os << indent << "In Abort Check: " << this->InAbortCheck << "\n";
  os << indent << "NeverRendered: " << this->NeverRendered << "\n";
  os << indent << "Interactor: " << this->Interactor << "\n";
  os << indent << "Motion Blur Frames: " << this->SubFrames << "\n";
  os << indent << "Swap Buffers: " << (this->SwapBuffers ? "On\n":"Off\n");
  os << indent << "Stereo Type: " << this->GetStereoTypeAsString() << "\n";
  os << indent << "Number of Layers: " << this->NumberOfLayers << "\n";
  os << indent << "AccumulationBuffer Size " << this->AccumulationBufferSize << "\n";
  os << indent << "AlphaBitPlanes: " << (this->AlphaBitPlanes ? "On" : "Off")
     << endl;

  os << indent << "AnaglyphColorSaturation: "
     << this->AnaglyphColorSaturation << "\n";
  os << indent << "AnaglyphColorMask: "
     << this->AnaglyphColorMask[0] << " , "
     << this->AnaglyphColorMask[1] << "\n";

  os << indent << "PainterDeviceAdapter: ";
  if (this->PainterDeviceAdapter)
    {
    os << endl;
    this->PainterDeviceAdapter->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }

  os << indent << "MultiSamples: " << this->MultiSamples << "\n";
  os << indent << "StencilCapable: " <<
    (this->StencilCapable ? "True" : "False") << endl;
#ifndef VTK_LEGACY_REMOVE
  os << indent << "ReportGraphicErrors: "
     << (this->ReportGraphicErrors ? "On" : "Off")<< "\n";
#endif
}

#ifndef VTK_LEGACY_REMOVE
//----------------------------------------------------------------------------
void vtkRenderWindow::SetReportGraphicErrors(int val)
{
  VTK_LEGACY_BODY(vtkRenderWindow::SetReportGraphicsErrors, "VTK 6.1");
  if (this->ReportGraphicErrors != val)
    {
    this->ReportGraphicErrors = val;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkRenderWindow::SetReportGraphicErrorsOn()
{
  VTK_LEGACY_BODY(vtkRenderWindow::SetReportGraphicsErrorsOn, "VTK 6.1");
  if (this->ReportGraphicErrors == 0)
    {
    this->ReportGraphicErrors = 1;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkRenderWindow::SetReportGraphicErrorsOff()
{
  VTK_LEGACY_BODY(vtkRenderWindow::GetReportGraphicsErrorsOff, "VTK 6.1");
  if (this->ReportGraphicErrors != 0)
    {
    this->ReportGraphicErrors = 0;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
int vtkRenderWindow::GetReportGraphicErrors()
{
  VTK_LEGACY_BODY(vtkRenderWindow::GetReportGraphicsErrors, "VTK 6.1");
  return this->ReportGraphicErrors;
}
#endif

//----------------------------------------------------------------------------
// Update the system, if needed, due to stereo rendering. For some stereo
// methods, subclasses might need to switch some hardware settings here.
void vtkRenderWindow::StereoUpdate(void)
{
  // if stereo is on and it wasn't before
  if (this->StereoRender && (!this->StereoStatus))
    {
    switch (this->StereoType)
      {
      case VTK_STEREO_RED_BLUE:
        this->StereoStatus = 1;
        break;
      case VTK_STEREO_ANAGLYPH:
        this->StereoStatus = 1;
        break;
      case VTK_STEREO_DRESDEN:
        this->StereoStatus = 1;
        break;
      case VTK_STEREO_INTERLACED:
        this->StereoStatus = 1;
        break;
      case VTK_STEREO_CHECKERBOARD:
        this->StereoStatus = 1;
        break;
      case VTK_STEREO_SPLITVIEWPORT_HORIZONTAL:
        this->StereoStatus = 1;
        break;
      }
    }
  else if ((!this->StereoRender) && this->StereoStatus)
    {
    switch (this->StereoType)
      {
      case VTK_STEREO_RED_BLUE:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_ANAGLYPH:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_DRESDEN:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_INTERLACED:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_CHECKERBOARD:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_SPLITVIEWPORT_HORIZONTAL:
        this->StereoStatus = 0;
        break;
      }
    }
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
      this->StereoBuffer = NULL;
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
      this->StereoBuffer = NULL;
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
      this->StereoBuffer = NULL;
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
      this->StereoBuffer = NULL;
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
        if(y % 2) {
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

      this->StereoBuffer = NULL;
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

      this->StereoBuffer = NULL;
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
      tmp->Register(0);
      this->Interactor->SetRenderWindow(NULL);
      tmp->UnRegister(0);
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
void vtkRenderWindow::CaptureGL2PSSpecialProps(vtkCollection *result)
{
  if (result == NULL)
    {
    vtkErrorMacro(<<"CaptureGL2PSSpecialProps was passed a NULL pointer.");
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
    result->AddItem(props.GetPointer());
    ren->SetGL2PSSpecialPropCollection(props.GetPointer());
    }

  this->Render();

  for (Renderers->InitTraversal(); (ren = Renderers->GetNextItem());)
    {
    ren->SetGL2PSSpecialPropCollection(NULL);
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
    default:
      return "";
  }
}
