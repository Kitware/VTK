/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderWindow.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTransform.h"
#include "vtkMath.h"
#include "vtkGraphicsFactory.h"
#include "vtkCommand.h"

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
  this->StereoStatus = 0;
  this->StereoCapableWindow = 0;
  this->Interactor = NULL;
  this->AAFrames = 0;
  this->FDFrames = 0;
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
  this->AbortCheckMethod = NULL;
  this->AbortCheckMethodArg = NULL;
  this->AbortCheckMethodArgDelete = NULL;
  this->Renderers = vtkRendererCollection::New();
  this->NumberOfLayers = 1;
}

vtkRenderWindow::~vtkRenderWindow()
{
  this->SetInteractor(NULL);
  
  if (this->AccumulationBuffer) 
    {
    delete [] this->AccumulationBuffer;
    this->AccumulationBuffer = NULL;
    this->AccumulationBufferSize = 0;
    }
  if (this->ResultFrame) 
    {
    delete [] this->ResultFrame;
    this->ResultFrame = NULL;
    }
  // delete the current arg if there is one and a delete meth
  if ((this->AbortCheckMethodArg)&&(this->AbortCheckMethodArgDelete))
    {
    (*this->AbortCheckMethodArgDelete)(this->AbortCheckMethodArg);
    }
  
  this->Renderers->Delete();
}

// Specify a function to be called to check and see if an abort
// of the rendering in progress is desired.
void vtkRenderWindow::SetAbortCheckMethod(void (*f)(void *), void *arg)
{
  if ( f != this->AbortCheckMethod || arg != this->AbortCheckMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->AbortCheckMethodArg)&&(this->AbortCheckMethodArgDelete))
      {
      (*this->AbortCheckMethodArgDelete)(this->AbortCheckMethodArg);
      }
    this->AbortCheckMethod = f;
    this->AbortCheckMethodArg = arg;
    this->Modified();
    }
}

// Set the arg delete method. This is used to free user memory.
void vtkRenderWindow::SetAbortCheckMethodArgDelete(void (*f)(void *))
{
  if ( f != this->AbortCheckMethodArgDelete)
    {
    this->AbortCheckMethodArgDelete = f;
    this->Modified();
    }
}

// return the correct type of RenderWindow 
vtkRenderWindow *vtkRenderWindow::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkGraphicsFactory::CreateInstance("vtkRenderWindow");
  return (vtkRenderWindow*)ret;
}

// Create an interactor that will work with this renderer.
vtkRenderWindowInteractor *vtkRenderWindow::MakeRenderWindowInteractor()
{
  this->Interactor = vtkRenderWindowInteractor::New();
  this->Interactor->SetRenderWindow(this);
  return this->Interactor;
}

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
      if (this->Interactor->GetRenderWindow() != this)
        {
        this->Interactor->SetRenderWindow(this);
        }
      }
    }
}

void vtkRenderWindow::SetDesiredUpdateRate(float rate)
{
  vtkRenderer *aren;

  if (this->DesiredUpdateRate != rate)
    {
    for (this->Renderers->InitTraversal(); 
         (aren = this->Renderers->GetNextItem()); )
      {
      aren->SetAllocatedRenderTime(1.0/
                                   (rate*this->Renderers->GetNumberOfItems()));
      }
    this->DesiredUpdateRate = rate;
    this->Modified();
    }
}


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



// Ask each renderer owned by this RenderWindow to render its image and 
// synchronize this process.
void vtkRenderWindow::Render()
{
  int *size;
  int x,y;
  float *p1;

  // if we are in the middle of an abort check the return now
  if (this->InRender)
    {
    return;
    }

  vtkDebugMacro(<< "Starting Render Method.\n");
  this->InvokeEvent(vtkCommand::StartEvent,NULL);

  // if we are in the middle of an abort check the return now
  if (this->InAbortCheck)
    {
    return;
    }
  // reset the Abort flag
  this->AbortRender = 0;

  this->InRender = 1;
  
  this->NeverRendered = 0;

  if ( this->Interactor && ! this->Interactor->GetInitialized() )
    {
    this->Interactor->Initialize();
    }
  
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
    if (this->CurrentSubFrame == this->SubFrames)
      {
      float num;
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
          *p2 = (unsigned char)(*p1/num); p1++; p2++;
          *p2 = (unsigned char)(*p1/num); p1++; p2++;
          *p2 = (unsigned char)(*p1/num); p1++; p2++;
          }
        }
      
      this->CurrentSubFrame = 0;
      this->CopyResultFrame();

      this->Renderers->RenderOverlay();

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
      float num;
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
          *p2 = (unsigned char)(*p1/num); p1++; p2++;
          *p2 = (unsigned char)(*p1/num); p1++; p2++;
          *p2 = (unsigned char)(*p1/num); p1++; p2++;
          }
        }
      
      delete [] this->AccumulationBuffer;
      this->AccumulationBuffer = NULL;
      }
    
    this->CopyResultFrame();
    this->Renderers->RenderOverlay();
    }  

  if (this->ResultFrame) 
    {
    delete [] this->ResultFrame;
    this->ResultFrame = NULL;
    }

  this->InRender = 0;
  this->InvokeEvent(vtkCommand::EndEvent,NULL);  
}

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
    float *dpoint;
    float offsets[2];
    float origfocus[4];
    float worldOffset[3];

    // get the size
    size = this->GetSize();

    origfocus[3] = 1.0;

    for (i = 0; i < this->AAFrames; i++)
      {
      // jitter the cameras
      offsets[0] = vtkMath::Random() - 0.5;
      offsets[1] = vtkMath::Random() - 0.5;

      for (this->Renderers->InitTraversal(); 
           (aren = this->Renderers->GetNextItem()); )
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
      for (this->Renderers->InitTraversal(); 
           (aren = this->Renderers->GetNextItem()); )
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
            *p1 += (float)*p2; p1++; p2++;
            *p1 += (float)*p2; p1++; p2++;
            *p1 += (float)*p2; p1++; p2++;
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
    float focalDisk;
    double *vpn, *dpoint;
    double vec[3];
    vtkTransform *aTrans = vtkTransform::New();
    double offsets[2];
    double *orig;

    // get the size
    size = this->GetSize();

    orig = new double [3*this->Renderers->GetNumberOfItems()];

    for (i = 0; i < this->FDFrames; i++)
      {
      int j = 0;

      offsets[0] = vtkMath::Random(); // radius
      offsets[1] = vtkMath::Random()*360.0; // angle

      // store offsets for each renderer 
      for (this->Renderers->InitTraversal(); 
           (aren = this->Renderers->GetNextItem()); )
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
      for (this->Renderers->InitTraversal(); 
           (aren = this->Renderers->GetNextItem()); )
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
          *p1 += (float)*p2; p1++; p2++;
          *p1 += (float)*p2; p1++; p2++;
          *p1 += (float)*p2; p1++; p2++;
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


// Handle rendering the two different views for stereo rendering.
void vtkRenderWindow::DoStereoRender()
{
  this->Start();
  this->StereoUpdate();
  if (this->StereoType != VTK_STEREO_RIGHT)
    { // render the left eye
    this->Renderers->Render();
    }

  if (this->StereoRender)
    {
    this->StereoMidpoint();
    if (this->StereoType != VTK_STEREO_LEFT)
      { // render the right eye
      this->Renderers->Render();
      }
    this->StereoRenderComplete();
    }
}

// Add a renderer to the list of renderers.
void vtkRenderWindow::AddRenderer(vtkRenderer *ren)
{
  // we are its parent
  this->MakeCurrent();
  ren->SetRenderWindow(this);
  this->Renderers->AddItem(ren);
  vtkRenderer *aren;
  for (this->Renderers->InitTraversal(); 
       (aren = this->Renderers->GetNextItem()); )
    {
    aren->SetAllocatedRenderTime
      (1.0/(this->DesiredUpdateRate*this->Renderers->GetNumberOfItems()));
    }
}

// Remove a renderer from the list of renderers.
void vtkRenderWindow::RemoveRenderer(vtkRenderer *ren)
{
  // we are its parent 
  this->Renderers->RemoveItem(ren);
}

int vtkRenderWindow::CheckAbortStatus()
{
  if (!this->InAbortCheck)
    {
    this->InAbortCheck = 1;
    if (this->AbortCheckMethod) 
      {
      (*this->AbortCheckMethod)(this->AbortCheckMethodArg);
      }
    this->InAbortCheck = 0;
    }
  return this->AbortRender;
}

void vtkRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkWindow::PrintSelf(os,indent);

  os << indent << "Borders: " << (this->Borders ? "On\n":"Off\n");
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
  if ( this->AbortCheckMethod )
    {
    os << indent << "AbortCheck method defined.\n";
    }
  else
    {
    os << indent << "No AbortCheck method.\n";
    }
}


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
      case VTK_STEREO_DRESDEN:
        this->StereoStatus = 1;
	break;      
      case VTK_STEREO_INTERLACED:
        this->StereoStatus = 1;
      }
    }
  else if ((!this->StereoRender) && this->StereoStatus)
    {
    switch (this->StereoType) 
      {
      case VTK_STEREO_RED_BLUE:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_DRESDEN:
        this->StereoStatus = 0;
	break;
      case VTK_STEREO_INTERLACED:
        this->StereoStatus = 0;
        break;
      }
    }
}

// Intermediate method performs operations required between the rendering
// of the left and right eye.
void vtkRenderWindow::StereoMidpoint(void)
{
  if ((this->StereoType == VTK_STEREO_RED_BLUE) ||
      (this->StereoType == VTK_STEREO_INTERLACED) ||
	  (this->StereoType == VTK_STEREO_DRESDEN) )
    {
    int *size;
    // get the size
    size = this->GetSize();
    // get the data
    this->StereoBuffer = this->GetPixelData(0,0,size[0]-1,size[1]-1,!this->DoubleBuffer);
    }
}

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
    }
}


void vtkRenderWindow::CopyResultFrame(void)
{
  if (this->ResultFrame)
    {
    int *size;

    // get the size
    size = this->GetSize();
    this->SetPixelData(0,0,size[0]-1,size[1]-1,this->ResultFrame,!this->DoubleBuffer);
    }

  this->Frame();
}


// treat renderWindow and interactor as one object.
// it might be easier if the GetReference count method were redefined.
void vtkRenderWindow::UnRegister(vtkObject *o)
{
  if (this->Interactor && this->Interactor->GetRenderWindow() == this &&
      this->Interactor != o)
    {
    if (this->GetReferenceCount() + this->Interactor->GetReferenceCount() == 3)
      {
      this->Interactor->SetRenderWindow(NULL);
      this->SetInteractor(NULL);
      }
    }
  
  this->vtkObject::UnRegister(o);
}

     


