/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkTreeCompositeCrop.cxx
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

// This software and ancillary information known as vtk_ext (and
// herein called "SOFTWARE") is made available under the terms
// described below.  The SOFTWARE has been approved for release with
// associated LA_CC Number 99-44, granted by Los Alamos National
// Laboratory in July 1999.
//
// Unless otherwise indicated, this SOFTWARE has been authored by an
// employee or employees of the University of California, operator of
// the Los Alamos National Laboratory under Contract No. W-7405-ENG-36
// with the United States Department of Energy.
//
// The United States Government has rights to use, reproduce, and
// distribute this SOFTWARE.  The public may copy, distribute, prepare
// derivative works and publicly display this SOFTWARE without charge,
// provided that this Notice and any statement of authorship are
// reproduced on all copies.
//
// Neither the U. S. Government, the University of California, nor the
// Advanced Computing Laboratory makes any warranty, either express or
// implied, nor assumes any liability or responsibility for the use of
// this SOFTWARE.
//
// If SOFTWARE is modified to produce derivative works, such modified
// SOFTWARE should be clearly marked, so as not to confuse it with the
// version available from Los Alamos National Laboratory.

#include "vtkTreeCompositeCrop.h"
#include "vtkObjectFactory.h"


//-------------------------------------------------------------------------
vtkTreeCompositeCrop* vtkTreeCompositeCrop::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkTreeComposite");
  if(ret)
    {
    return (vtkTreeCompositeCrop*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkTreeCompositeCrop;
}

//-------------------------------------------------------------------------
vtkTreeCompositeCrop::vtkTreeCompositeCrop()
{
  this->LocalPData = NULL;
  this->LocalZData = NULL;
}

  
//-------------------------------------------------------------------------
vtkTreeCompositeCrop::~vtkTreeCompositeCrop()
{
  // Superclasses call to "SetWindowSize"
  // deletes the local buffers.
}

     
//-------------------------------------------------------------------------
void vtkTreeCompositeCrop::SetRendererSize(int x, int y)
{
  if (this->RendererSize[0] == x && this->RendererSize[1] == y)
    {
    return;
    }
  
  if (this->RemotePData)
    {
    delete this->RemotePData;
    this->RemotePData = NULL;
    }
  if (this->RemoteZData)
    {
    delete this->RemoteZData;
    this->RemoteZData = NULL;
    }
  if (this->LocalPData)
    {
    delete this->LocalPData;
    this->LocalPData = NULL;
    }  
  if (this->LocalZData)
    {
    delete this->LocalZData;
    this->LocalZData = NULL;
    }
  
  int numPixels = x * y;
  if (numPixels > 0)
    {
    this->RemotePData = new float[4*numPixels];
    this->RemoteZData = new float[numPixels];
    this->LocalPData = new float[4*numPixels];
    this->LocalZData = new float[numPixels];
    }
  this->RendererSize[0] = x;
  this->RendererSize[1] = y;
}





#define vtkTCPow2(j) (1 << (j))




//########################################################
// Special stuff


// It may be better to composite 1 - N and then composite 0.
// Na!!!


//----------------------------------------------------------------------------
void vtkTreeCompositeCrop::Composite()
{
  float *renZdata = NULL;
  float *renPdata = NULL;
  int total_pixels;
  int pixel_size;
  int myId, numProcs;
  int i, id;
  int extent[4];
  int remoteExt[4];
  int length;

  
  vtkTimerLog *timer = vtkTimerLog::New();
  
  myId = this->Controller->GetLocalProcessId();
  numProcs = this->Controller->GetNumberOfProcesses();
  total_pixels = this->RendererSize[0] * this->RendererSize[1];

  // Find the extent of the window with geometry.
  extent[0] = extent[2] = 0;
  extent[1] = this->RendererSize[0]-1;
  extent[3] = this->RendererSize[1]-1;
  if (myId > 0)
    {
    this->ComputeRenderExtent(extent);
    }

  // Get the z buffer.
  timer->StartTimer();
  renZdata = this->RenderWindow->GetZbufferData(extent[0], extent[1],
                                                extent[2], extent[3]);

  // Get the pixel data.
  if (this->UseChar) 
    { 
    renPdata = (float*)this->RenderWindow->GetRGBACharPixelData(
                                                extent[0], extent[1],
						extent[2], extent[3], 0);
    pixel_size = 1;
    } 
  else 
    {
    renPdata = this->RenderWindow->GetRGBAPixelData(extent[0], extent[1],
			                            extent[2], extent[3], 0);
    pixel_size = 4;
    }
  
  timer->StopTimer();
  this->GetBuffersTime = timer->GetElapsedTime();

  // Copy the memory into a full sized array.
  length = (extent[1]-extent[0]+1)*(extent[3]-extent[2]+1);
  memcpy(renZdata, this->LocalZData, 4*length);
  memcpy(renPdata, this->LocalPData, pixel_size*length);
  delete [] renZdata;
  delete [] renPdata;
  
  double doubleLogProcs = log((double)numProcs)/log((double)2);
  int logProcs = (int)doubleLogProcs;

  // not a power of 2 -- need an additional level
  if (doubleLogProcs != (double)logProcs) 
    {
    logProcs++;
    }

  timer->StartTimer();
  
  for (i = 0; i < logProcs; i++) 
    {
    if ((myId % (int)vtkTCPow2(i)) == 0) 
      { // Find participants
      if ((myId % (int)vtkTCPow2(i+1)) < vtkTCPow2(i)) 
	{
	// receivers
	id = myId+vtkTCPow2(i);
	
	// only send or receive if sender or receiver id is valid
	// (handles non-power of 2 cases)
	if (id < numProcs) 
	  {
	  this->Controller->Receive(remoteExt, 4, id, 97);
          // Grow the local extent to include the remote extent
          this->ReformatLocalData(remoteExt, extent);
          // Receive the remote data.
	  length = (remoteExt[1]-remoteExt[0]+1)*(remoteExt[3]-remoteExt[2]+1);
          this->Controller->Receive(this->RemoteZData, length*4, id, 98);
	  this->Controller->Receive(this->RemotePData, length*pixel_size, id, 99);
	  
	  // Notice the result is stored as the local data
          // also, extent becomes union of extent and remote extent.
	  this->CompositeImagePair(remoteExt, extent);
	  }
	}
      else 
	{
	id = myId-vtkTCPow2(i);
	if (id < numProcs) 
	  {
	  length = (extent[1]-extent[0]+1)*(extent[3]-extent[2]+1);
          this->Controller->Send(extent, 4, id, 97);
	  this->Controller->Send(this->LocalZData, 4*length, id, 98);
	  this->Controller->Send(this->LocalPData, pixel_size * length, id, 99);
	  }
	}
      }
    }
  
  timer->StopTimer();
  this->TransmitTime = timer->GetElapsedTime();
    
  if (myId == 0) 
    {
    int windowSize[2];
    // Default value (no reduction).
    windowSize[0] = this->RendererSize[0];
    windowSize[1] = this->RendererSize[1];

    if (this->ReductionFactor > 1)
      {
      // localPdata gets freed (new memory is allocated and returned.
      // windowSize get modified.
      this->LocalPData = this->MagnifyBuffer(this->LocalPData, windowSize);
      
      vtkRenderer* renderer =
	((vtkRenderer*)this->RenderWindow->GetRenderers()->GetItemAsObject(0));
      renderer->SetViewport(0, 0, 1.0, 1.0);
      renderer->GetActiveCamera()->UpdateViewport(renderer);
      }

    // Save the ZData for picking.
    memcpy(this->RemoteZData, this->LocalZData, 
	   this->RendererSize[0]*this->RendererSize[1]*sizeof(float));
  

    
    timer->StartTimer();
    if (this->UseChar) 
      {
      this->RenderWindow->SetRGBACharPixelData(0, 0, windowSize[0]-1, 
                                       windowSize[1]-1,
                                       (unsigned char*)this->LocalPData, 0);
      } 
    else 
      {
      this->RenderWindow->SetRGBAPixelData(0, 0, windowSize[0]-1, 
                                      windowSize[1]-1,
                                      this->LocalPData, 0);
      }
    timer->StopTimer();
    this->SetBuffersTime = timer->GetElapsedTime();
    }
    
  timer->Delete();
  timer = NULL;
}




//-------------------------------------------------------------------------
void vtkTreeCompositeCrop::ComputeRenderExtent(int *ext)
{
  vtkRendererCollection *rens;
  vtkRenderer* ren;
  float bounds[6];
  float dp[3];
  int tmp, rExt[4];
  
  rens = this->RenderWindow->GetRenderers();
  rens->InitTraversal();
  ren = rens->GetNextItem();

  ren->ComputeVisiblePropBounds(bounds);

  ren->SetWorldPoint(bounds[0], bounds[2], bounds[4], 1.0);
  ren->WorldToDisplay();
  ren->GetDisplayPoint(dp);
  rExt[0] = rExt[1] = (int)dp[0];
  rExt[2] = rExt[3] = (int)dp[1];

  // Consider each corner of the bounds.
  ren->SetWorldPoint(bounds[0], bounds[2], bounds[5], 1.0);
  ren->WorldToDisplay();
  ren->GetDisplayPoint(dp);
  tmp = (int)(dp[0]);
  if (tmp < rExt[0])
    {
    rExt[0] = tmp;
    }
  if (tmp > rExt[1])
    {
    rExt[1] = tmp;
    }
  tmp = (int)(dp[1]);
  if (tmp < rExt[2])
    {
    rExt[2] = tmp;
    }
  if (tmp > rExt[3])
    {
    rExt[3] = tmp;
    }

  ren->SetWorldPoint(bounds[0], bounds[3], bounds[4], 1.0);
  ren->WorldToDisplay();
  ren->GetDisplayPoint(dp);
  tmp = (int)(dp[0]);
  if (tmp < rExt[0])
    {
    rExt[0] = tmp;
    }
  if (tmp > rExt[1])
    {
    rExt[1] = tmp;
    }
  tmp = (int)(dp[1]);
  if (tmp < rExt[2])
    {
    rExt[2] = tmp;
    }
  if (tmp > rExt[3])
    {
    rExt[3] = tmp;
    }

  ren->SetWorldPoint(bounds[0], bounds[3], bounds[5], 1.0);
  ren->WorldToDisplay();
  ren->GetDisplayPoint(dp);
  tmp = (int)(dp[0]);
  if (tmp < rExt[0])
    {
    rExt[0] = tmp;
    }
  if (tmp > rExt[1])
    {
    rExt[1] = tmp;
    }
  tmp = (int)(dp[1]);
  if (tmp < rExt[2])
    {
    rExt[2] = tmp;
    }
  if (tmp > rExt[3])
    {
    rExt[3] = tmp;
    }

  ren->SetWorldPoint(bounds[1], bounds[2], bounds[4], 1.0);
  ren->WorldToDisplay();
  ren->GetDisplayPoint(dp);
  tmp = (int)(dp[0]);
  if (tmp < rExt[0])
    {
    rExt[0] = tmp;
    }
  if (tmp > rExt[1])
    {
    rExt[1] = tmp;
    }
  tmp = (int)(dp[1]);
  if (tmp < rExt[2])
    {
    rExt[2] = tmp;
    }
  if (tmp > rExt[3])
    {
    rExt[3] = tmp;
    }

  ren->SetWorldPoint(bounds[1], bounds[2], bounds[5], 1.0);
  ren->WorldToDisplay();
  ren->GetDisplayPoint(dp);
  tmp = (int)(dp[0]);
  if (tmp < rExt[0])
    {
    rExt[0] = tmp;
    }
  if (tmp > rExt[1])
    {
    rExt[1] = tmp;
    }
  tmp = (int)(dp[1]);
  if (tmp < rExt[2])
    {
    rExt[2] = tmp;
    }
  if (tmp > rExt[3])
    {
    rExt[3] = tmp;
    }

  ren->SetWorldPoint(bounds[1], bounds[3], bounds[4], 1.0);
  ren->WorldToDisplay();
  ren->GetDisplayPoint(dp);
  tmp = (int)(dp[0]);
  if (tmp < rExt[0])
    {
    rExt[0] = tmp;
    }
  if (tmp > rExt[1])
    {
    rExt[1] = tmp;
    }
  tmp = (int)(dp[1]);
  if (tmp < rExt[2])
    {
    rExt[2] = tmp;
    }
  if (tmp > rExt[3])
    {
    rExt[3] = tmp;
    }

  ren->SetWorldPoint(bounds[1], bounds[3], bounds[5], 1.0);
  ren->WorldToDisplay();
  ren->GetDisplayPoint(dp);
  tmp = (int)(dp[0]);
  if (tmp < rExt[0])
    {
    rExt[0] = tmp;
    }
  if (tmp > rExt[1])
    {
    rExt[1] = tmp;
    }
  tmp = (int)(dp[1]);
  if (tmp < rExt[2])
    {
    rExt[2] = tmp;
    }
  if (tmp > rExt[3])
    {
    rExt[3] = tmp;
    }


  // Intersection between ren extent and visible extent.
  if (ext[0] < rExt[0])
    {
    ext[0] = rExt[0];
    }
  if (ext[1] > rExt[1])
    {
    ext[1] = rExt[1];
    }
  if (ext[2] < rExt[2])
    {
    ext[2] = rExt[2];
    }
  if (ext[3] > rExt[3])
    {
    ext[3] = rExt[3];
    }
}




//-------------------------------------------------------------------------
// This destroys the remote buffers.
void vtkTreeCompositeCrop::ReformatLocalData(int *remoteExt, 
                                            int *localExt)
{
  int flag = 1;
  int i, j, length, pSkip, zSkip, offset;
  int ext[4];
  float *ptr, *end;
  float *inZ, *outZ;
  float *inP, *outP;

  // Compute the union of the extent.
  memcpy(localExt,ext, 4*sizeof(int));
  if (remoteExt[0] < ext[0])
    {
    ext[0] = remoteExt[0];
    flag = 0;
    }
  if (remoteExt[1] > ext[1])
    {
    ext[1] = remoteExt[1];
    flag = 0;
    }
  if (remoteExt[2] < ext[2])
    {
    ext[2] = remoteExt[2];
    flag = 0;
    }
  if (remoteExt[3] > ext[3])
    {
    ext[3] = remoteExt[3];
    flag = 0;
    }
  if (flag)
    {
    return;
    }

  // Swap the two buffers.
  ptr = this->RemoteZData;
  this->RemoteZData = this->LocalZData;
  this->LocalZData = ptr;
  ptr = this->RemotePData;
  this->RemotePData = this->LocalPData;
  this->LocalPData = ptr;

  // Initialize the array.
  length = (ext[1]-ext[0]+1)*(ext[3]-ext[2]+1);
  ptr = this->LocalZData;
  end = ptr + length;
  while(ptr != end)
    {
    *ptr++ = 10.0;
    }

  // Copy The data.
  inZ = this->RemoteZData;
  zSkip = (ext[1]-ext[0])-(localExt[1]-localExt[0]);
  offset = (localExt[0]-ext[0])+((localExt[2]-ext[2])*(ext[1]-ext[0]+1));
  outZ = this->LocalZData + offset;

  if (this->UseChar)
    {
    inP = this->RemotePData;
    outP = this->LocalPData + offset;
    pSkip = zSkip;
    for (j = localExt[2]; j <= localExt[3]; ++j)
      {
      for (i = localExt[0]; i <= localExt[1]; ++ i)
        {
        *outP++ = *inP++;
        *outZ++ = *inZ++;
        }
      inP += pSkip;
      inZ += zSkip;
      }
    }
  else 
    {
    inP = this->RemotePData;
    outP = this->LocalPData + (4*offset);
    pSkip = zSkip * 4;  // rgba
    for (j = localExt[2]; j <= localExt[3]; ++j)
      {
      for (i = localExt[0]; i <= localExt[1]; ++ i)
        {
        *outP++ = *inP++;
        *outP++ = *inP++;
        *outP++ = *inP++;
        *outP++ = *inP++;

        *outZ++ = *inZ++;
        }
      inP += pSkip;
      inZ += zSkip;
      }
    }
    
  localExt[0] = ext[0];
  localExt[1] = ext[1];
  localExt[2] = ext[2];
  localExt[3] = ext[3];
}


//-------------------------------------------------------------------------
// Results are put in the local data.
void vtkTreeCompositeCrop::CompositeImagePair(int *remoteExt, int *localExt) 
{
  int i, j, pSkip, zSkip, offset;
  float *ptr, *end;
  float *inZ, *outZ;
  float *inP, *outP;

  inZ = this->RemoteZData;
  zSkip = (localExt[1]-localExt[0])-(remoteExt[1]-remoteExt[0]);
  offset = (remoteExt[0]-localExt[0])
    + ((remoteExt[2]-localExt[2])*(localExt[1]-localExt[0]+1));
  outZ = this->LocalZData + offset;

  if (this->UseChar)
    {
    inP = this->RemotePData;
    outP = this->LocalPData + offset;
    pSkip = zSkip;
    for (j = localExt[2]; j <= localExt[3]; ++j)
      {
      for (i = localExt[0]; i <= localExt[1]; ++ i)
        {
        if (*inZ < *outZ) 
          { // Copy and move on to the next pixel.
          *outP++ = *inP++;
          *outZ++ = *inZ++;
          }
        else
          { // NOP.  Move on to the next pixel.
          ++outP;
          ++outZ;
          ++inP;
          ++inZ;
          }
        }
      inP += pSkip;
      inZ += zSkip;
      }
    }
  else 
    {
    inP = this->RemotePData;
    outP = this->LocalPData + offset;
    pSkip = zSkip * 4;  // rgba
    for (j = localExt[2]; j <= localExt[3]; ++j)
      {
      for (i = localExt[0]; i <= localExt[1]; ++ i)
        {
        if (*inZ < *outZ) 
          { // Copy and move on to the next pixel.
          *outP++ = *inP++;
          *outP++ = *inP++;
          *outP++ = *inP++;
          *outP++ = *inP++;

          *outZ++ = *inZ++;
          }
        else
          { // NOP.  Move on to the next pixel.
          outP += 4;
          inP += 4;
          ++outZ;
          ++inZ;
          }
        }
      inP += pSkip;
      inZ += zSkip;
      }
    }
}


