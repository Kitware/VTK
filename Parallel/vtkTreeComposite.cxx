/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeComposite.cxx
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
#include "vtkFloatArray.h"
#include "vtkUnsignedCharArray.h"

vtkCxxRevisionMacro(vtkTreeComposite, "1.26");
vtkStandardNewMacro(vtkTreeComposite);

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
void vtkCompositeImagePair(vtkFloatArray *localZ, 
			   vtkDataArray *localP, 
                           vtkFloatArray *remoteZ, 
			   vtkDataArray *remoteP, 
                           int total_pixels, int useCharFlag) 
{
  int i,j;
  int pixel_data_size;
  float *pEnd;

  float* remoteZdata = remoteZ->GetPointer(0);
  float* remotePdata = reinterpret_cast<float*>(remoteP->GetVoidPointer(0));
  float* localZdata = localZ->GetPointer(0);
  float* localPdata = reinterpret_cast<float*>(localP->GetVoidPointer(0));

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

static inline int vtkTCLog2(int j, int& exact)
{
  int counter=0;
  exact = 1;
  while(j)
    {
    if ( ( j & 1 ) && (j >> 1) )
      {
      exact = 0;
      }
    j = j >> 1;
    counter++;
    }
  return counter-1;
}

void vtkTreeComposite::CompositeBuffer(int width, int height, int useCharFlag,
				       vtkDataArray *pBuf, 
				       vtkFloatArray *zBuf,
				       vtkDataArray *pTmp, 
				       vtkFloatArray *zTmp)
{
  int myId = this->Controller->GetLocalProcessId();
  int numProcs = this->Controller->GetNumberOfProcesses();
  int totalPixels;
  int pSize, zSize;
  int i, id;

  int exactLog;
  int logProcs = vtkTCLog2(numProcs,exactLog);

  // not a power of 2 -- need an additional level
  if ( !exactLog ) 
    {
    logProcs++;
    }

  totalPixels = width*height;
  zSize = totalPixels;
  pSize = 4*totalPixels;


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
          this->Controller->Receive(zTmp->GetPointer(0), zSize, id, 99);
	  if (this->UseChar)
	    {
	    this->Controller->Receive(reinterpret_cast<unsigned char*>
				      (pTmp->GetVoidPointer(0)), 
				      pSize, id, 99);
	    }
	  else
	    {
	    this->Controller->Receive(reinterpret_cast<float*>
				      (pTmp->GetVoidPointer(0)), 
				      pSize, id, 99);
	    }
          
          // notice the result is stored as the local data
          vtkCompositeImagePair(zBuf, pBuf, zTmp, pTmp, 
                                totalPixels, useCharFlag);
          }
        }
      else 
        {
        id = myId-vtkTCPow2(i);
        if (id < numProcs) 
          {
          this->Controller->Send(zBuf->GetPointer(0), zSize, id, 99);
	  if (this->UseChar)
	    {
	    this->Controller->Send(reinterpret_cast<unsigned char*>
				   (pBuf->GetVoidPointer(0)), 
				   pSize, id, 99);
	    }
	  else
	    {
	    this->Controller->Send(reinterpret_cast<float*>
				   (pBuf->GetVoidPointer(0)), 
				   pSize, id, 99);
	    }
          }
        }
      }
    }
}







void vtkTreeComposite::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkCompositeManager::PrintSelf(os, indent);
}



