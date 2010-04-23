/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeCompositer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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

#include "vtkTreeCompositer.h"
#include "vtkObjectFactory.h"
#include "vtkToolkits.h"
#include "vtkFloatArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkMultiProcessController.h"

vtkStandardNewMacro(vtkTreeCompositer);

#ifdef VTK_USE_MPI
 #include <mpi.h>
#endif

//-------------------------------------------------------------------------
vtkTreeCompositer::vtkTreeCompositer()
{
}

  
//-------------------------------------------------------------------------
vtkTreeCompositer::~vtkTreeCompositer()
{
}

//-------------------------------------------------------------------------
// Jim's composite stuff
//-------------------------------------------------------------------------
// Results are put in the local data.
void vtkCompositeImagePair(vtkFloatArray *localZ, 
                           vtkDataArray *localP, 
                           vtkFloatArray *remoteZ, 
                           vtkDataArray *remoteP) 
{
  int i,j;
  int pixel_data_size;
  float *pEnd;
  int numComp = localP->GetNumberOfComponents();
  float* remoteZdata = remoteZ->GetPointer(0);
  float* remotePdata = reinterpret_cast<float*>(remoteP->GetVoidPointer(0));
  float* localZdata = localZ->GetPointer(0);
  float* localPdata = reinterpret_cast<float*>(localP->GetVoidPointer(0));

  int total_pixels = localZ->GetNumberOfTuples();
  int useCharFlag = 0;
  
  if (localP->GetDataType() == VTK_UNSIGNED_CHAR)
    {
    useCharFlag = 1;
    } 

  if (useCharFlag) 
    {
    pEnd = remoteZdata + total_pixels;
    if (numComp == 4)
      {
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
    else if (numComp == 3)
      {
      unsigned char* clocalPdata = reinterpret_cast<unsigned char*>(localPdata);
      unsigned char* cremotePdata = reinterpret_cast<unsigned char*>(remotePdata);
      while(remoteZdata != pEnd) 
        {
        if (*remoteZdata < *localZdata) 
          {
          *localZdata++ = *remoteZdata++;
          *clocalPdata++ = *cremotePdata++;
          *clocalPdata++ = *cremotePdata++;
          *clocalPdata++ = *cremotePdata++;
          }
        else
          {
          ++localZdata;
          ++remoteZdata;
          clocalPdata += 3;
          cremotePdata += 3;
          }
        }
      }
    } 
  else 
    {
    pixel_data_size = numComp;
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

void vtkTreeCompositer::CompositeBuffer(vtkDataArray *pBuf, 
                                        vtkFloatArray *zBuf,
                                        vtkDataArray *pTmp, 
                                        vtkFloatArray *zTmp)
{
  int myId = this->Controller->GetLocalProcessId();
  int numProcs = this->NumberOfProcesses;
  int totalPixels;
  int pSize, zSize;
  int i, id;
  int numComp = pBuf->GetNumberOfComponents();
  int exactLog;
  int logProcs = vtkTCLog2(numProcs,exactLog);

  // not a power of 2 -- need an additional level
  if ( !exactLog ) 
    {
    logProcs++;
    }

  totalPixels = zBuf->GetNumberOfTuples();
  zSize = totalPixels;
  pSize = numComp*totalPixels;

#ifdef MPIPROALLOC
  vtkCommunicator::SetUseCopy(0);
#endif
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
          if (pTmp->GetDataType() == VTK_UNSIGNED_CHAR)
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
          vtkCompositeImagePair(zBuf, pBuf, zTmp, pTmp);
          }
        }
      else 
        {
        id = myId-vtkTCPow2(i);
        if (id < numProcs) 
          {
          this->Controller->Send(zBuf->GetPointer(0), zSize, id, 99);
          if (pBuf->GetDataType() == VTK_UNSIGNED_CHAR)
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

#ifdef MPIPROALLOC
  vtkCommunicator::SetUseCopy(1);
#endif

}

void vtkTreeCompositer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}



