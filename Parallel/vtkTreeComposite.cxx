/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkTreeComposite.cxx
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

#include "vtkTreeComposite.h"
#include "vtkObjectFactory.h"
#include "vtkToolkits.h"


//-------------------------------------------------------------------------
vtkTreeComposite* vtkTreeComposite::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkTreeComposite");
  if(ret)
    {
    return (vtkTreeComposite*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkTreeComposite;
}

//-------------------------------------------------------------------------
vtkTreeComposite::vtkTreeComposite()
{
}

  
//-------------------------------------------------------------------------
vtkTreeComposite::~vtkTreeComposite()
{
}



     
//-------------------------------------------------------------------------
// Jim's composite stuff
//-------------------------------------------------------------------------
// Results are put in the local data.
void vtkCompositeImagePair(float *localZdata, float *localPdata, 
			   float *remoteZdata, float *remotePdata, 
			   int total_pixels, int useCharFlag) 
{
  int i,j;
  int pixel_data_size;
  float *pEnd;

  if (useCharFlag) 
    {
    pEnd = remoteZdata + total_pixels;
    while(remoteZdata != pEnd) 
      {
      if (*remoteZdata < *localZdata) 
        {
        *localZdata++ = *remoteZdata++;
        *localPdata++ = *remotePdata++;
        }
      else
        {
        ++localZdata;
        ++remoteZdata;
        ++localPdata;
        ++remotePdata;
        }
      }
    } 
  else 
    {
    pixel_data_size = 4;
    for (i = 0; i < total_pixels; i++) 
      {
      if (remoteZdata[i] < localZdata[i]) 
        {
        localZdata[i] = remoteZdata[i];
        for (j = 0; j < pixel_data_size; j++) 
          {
          localPdata[i*pixel_data_size+j] = remotePdata[i*pixel_data_size+j];
          }
        }
      }
    }
}








#define vtkTCPow2(j) (1 << (j))

void vtkTreeComposite::CompositeBuffer(int width, int height, int useCharFlag,
                                      void *pBuf, float *zBuf,
                                      void *pTmp, float *zTmp)
{
  int myId = this->Controller->GetLocalProcessId();
  int numProcs = this->Controller->GetNumberOfProcesses();
  double doubleLogProcs = log((double)numProcs)/log((double)2);
  int logProcs = (int)doubleLogProcs;
  int totalPixels;
  int pSize, zSize;
  int i, id;

  totalPixels = width*height;
  zSize = totalPixels;
  if (this->UseChar) 
    { 
    pSize = totalPixels;
    } 
  else 
    {
    pSize = 4*totalPixels;
    }

  // not a power of 2 -- need an additional level
  if (doubleLogProcs != (double)logProcs) 
    {
    logProcs++;
    }

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
          this->Controller->Receive(zTmp, zSize, id, 99);
          this->Controller->Receive((float*)pTmp, pSize, id, 99);
	  
          // notice the result is stored as the local data
          vtkCompositeImagePair(zBuf, (float*)pBuf, zTmp, (float*)pTmp, 
                                totalPixels, useCharFlag);
          }
        }
      else 
        {
        id = myId-vtkTCPow2(i);
        if (id < numProcs) 
          {
          this->Controller->Send(zBuf, zSize, id, 99);
          this->Controller->Send((float*)pBuf, pSize, id, 99);
          }
        }
      }
    }
}







void vtkTreeComposite::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkCompositeManager::PrintSelf(os, indent);
}



