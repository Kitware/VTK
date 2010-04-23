/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompressCompositer.cxx

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

#ifdef VTK_USE_MPI
 #include <mpi.h>
#endif

#include "vtkCompressCompositer.h"
#include "vtkObjectFactory.h"
#include "vtkToolkits.h"
#include "vtkFloatArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkMultiProcessController.h"

#include "vtkTimerLog.h"

vtkStandardNewMacro(vtkCompressCompositer);


// Different pixel types to template.
typedef struct {
  unsigned char r;
  unsigned char g;
  unsigned char b;
} vtkCharRGBType;

typedef struct {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
} vtkCharRGBAType;

typedef struct {
  float r;
  float g;
  float b;
  float a;
} vtkFloatRGBAType;



//-------------------------------------------------------------------------
vtkCompressCompositer::vtkCompressCompositer()
{
  this->InternalPData = NULL;
  this->InternalZData = NULL;
  this->Timer = vtkTimerLog::New();
}

  
//-------------------------------------------------------------------------
vtkCompressCompositer::~vtkCompressCompositer()
{
  if (this->InternalPData)
    {
    this->InternalPData->Delete();
    this->InternalPData = NULL;
    }
  if (this->InternalZData)
    {
    this->InternalZData->Delete();
    this->InternalZData = NULL;
    }

  this->Timer->Delete();
  this->Timer = NULL;
}



//-------------------------------------------------------------------------
// Compress background pixels with runlength encoding.
// z values above 1.0 mean: Repeat background for that many pixels.
// We could easily compress inplace, but it works out better for buffer 
// managment if we do not.  zIn == zOut is allowed....
template <class P>
int vtkCompressCompositerCompress(float *zIn, P *pIn, float *zOut, P *pOut,
                                  int numPixels)
{
  float* endZ;
  int length = 0;
  int compressCount;

  // Do not go past the last pixel (zbuf check/correct)
  endZ = zIn+numPixels-1;
  if (*zIn < 0.0 || *zIn > 1.0)
    {
    *zIn = 1.0;
    } 
  while (zIn < endZ)
    {
    ++length;
    // Always copy the first pixel value.
    *pOut++ = *pIn++;
    // Find the length of any compressed run.
    compressCount = 0;
    while (*zIn == 1.0 && zIn < endZ)
      {
      ++compressCount;
      ++zIn;
      if (*zIn < 0.0 || *zIn > 1.0)
        {
        *zIn = 1.0;
        } 
      }
 
    if (compressCount > 0)
      { // Only compress runs of 2 or more.
      // Move the pixel pointer past compressed region.
      pIn += (compressCount-1);
      // Set the special z value.
      *zOut++ = (float)(compressCount);
      }
    else
      { 
      *zOut++ = *zIn++;
      if (*zIn < 0.0 || *zIn > 1.0)
        {
        *zIn = 1.0;
        } 
      }
    }
  // Put the last pixel in.
  *pOut = *pIn;
  *zOut = *zIn;

  return length;
}

//-------------------------------------------------------------------------
// Compress background pixels with runlength encoding.
// z values above 1.0 mean: Repeat background for that many pixels.
// We could easily compress inplace, but it works out better for buffer 
// managment if we do not.  zIn == zOut is allowed....
void vtkCompressCompositer::Compress(vtkFloatArray *zIn, vtkDataArray *pIn,
                                     vtkFloatArray *zOut, vtkDataArray *pOut)
{
  float* pzf1 = zIn->GetPointer(0);
  float* pzf2 = zOut->GetPointer(0);
  void*  ppv1 = pIn->GetVoidPointer(0);
  void*  ppv2 = pOut->GetVoidPointer(0);
  int totalPixels = zIn->GetNumberOfTuples();
  int length;
  
  vtkTimerLog::MarkStartEvent("Compress");

  // This is just a complex switch statment 
  // to call the correct templated function.
  if (pIn->GetDataType() == VTK_UNSIGNED_CHAR) 
    {
    if (pIn->GetNumberOfComponents() == 3) 
      {
      length = vtkCompressCompositerCompress(
        pzf1, reinterpret_cast<vtkCharRGBType*>(ppv1),
        pzf2, reinterpret_cast<vtkCharRGBType*>(ppv2),
        totalPixels);
      }
    else if (pIn->GetNumberOfComponents() == 4) 
      {
      length = vtkCompressCompositerCompress(
        pzf1, reinterpret_cast<vtkCharRGBAType*>(ppv1),
        pzf2, reinterpret_cast<vtkCharRGBAType*>(ppv2),
        totalPixels);
      }
    else 
      {
      vtkGenericWarningMacro("Pixels have unexpected number of components.");
      return;
      }
    }
  else if (pIn->GetDataType() == VTK_FLOAT && 
           pIn->GetNumberOfComponents() == 4) 
    {
    length = vtkCompressCompositerCompress(
      pzf1, reinterpret_cast<vtkFloatRGBAType*>(ppv1),
      pzf2, reinterpret_cast<vtkFloatRGBAType*>(ppv2),
      totalPixels);
    }
  else
    {
    vtkGenericWarningMacro("Unexpected pixel type.");
    return;
    }

  zOut->SetNumberOfTuples(length);
  pOut->SetNumberOfTuples(length);

  vtkTimerLog::MarkEndEvent("Compress");
}

//-------------------------------------------------------------------------
//  z values above 1.0 mean: Repeat background for that many pixels.
// Assume that the array has enough allocated space for the uncompressed.
// In place/reverse order.
template <class P>
void vtkCompressCompositerUncompress(float *zIn, P *pIn, float *zOut, P *pOut,
                                     int lengthIn)
{
  float* endZ;
  int count;
  P background;
  
  endZ = zIn + lengthIn;

  while (zIn < endZ)
    {
    // Expand any compressed data.
    if (*zIn > 1.0)
      {
      background = *pIn++;
      count = (int)(*zIn++);
      while (count-- > 0)
        {
        *pOut++ = background;
        *zOut++ = 1.0;
        }
      }
    else
      {
      *pOut++ = *pIn++;
      *zOut++ = *zIn++;
      }
    }
}

//-------------------------------------------------------------------------
// Compress background pixels with runlength encoding.
// z values above 1.0 mean: Repeat background for that many pixels.
// We could easily compress inplace, but it works out better for buffer 
// managment if we do not.  zIn == zOut is allowed....
void vtkCompressCompositer::Uncompress(vtkFloatArray *zIn, vtkDataArray *pIn,
                                       vtkFloatArray *zOut, vtkDataArray *pOut,
                                       int lengthOut)
{
  float* pzf1 = zIn->GetPointer(0);
  float* pzf2 = zOut->GetPointer(0);
  void*  ppv1 = pIn->GetVoidPointer(0);
  void*  ppv2 = pOut->GetVoidPointer(0);
  int lengthIn = zIn->GetNumberOfTuples();
  
  vtkTimerLog::MarkStartEvent("Uncompress");

  // This is just a complex switch statment 
  // to call the correct templated function.
  if (pIn->GetDataType() == VTK_UNSIGNED_CHAR) 
    {
    if (pIn->GetNumberOfComponents() == 3) 
      {
      vtkCompressCompositerUncompress(pzf1, 
                                      reinterpret_cast<vtkCharRGBType*>(ppv1),
                                      pzf2,
                                      reinterpret_cast<vtkCharRGBType*>(ppv2),
                                      lengthIn);
      }
    else if (pIn->GetNumberOfComponents() == 4) 
      {
      vtkCompressCompositerUncompress(pzf1, 
                                      reinterpret_cast<vtkCharRGBAType*>(ppv1),
                                      pzf2,
                                      reinterpret_cast<vtkCharRGBAType*>(ppv2),
                                      lengthIn);
      }
    else 
      {
      vtkGenericWarningMacro("Pixels have unexpected number of components.");
      return;
      }
    }
  else if (pIn->GetDataType() == VTK_FLOAT && 
           pIn->GetNumberOfComponents() == 4) 
    {
    vtkCompressCompositerUncompress(pzf1, 
                                    reinterpret_cast<vtkFloatRGBAType*>(ppv1),
                                    pzf2,
                                    reinterpret_cast<vtkFloatRGBAType*>(ppv2),
                                    lengthIn);
    }
  else
    {
    vtkGenericWarningMacro("Unexpected pixel type.");
    return;
    }

  //zOut->SetNumberOfTuples(lengthOut);
  pOut->SetNumberOfTuples(lengthOut);

  vtkTimerLog::MarkEndEvent("Uncompress");
}




//-------------------------------------------------------------------------
// Can handle compositing compressed buffers.
// z values above 1.0 mean: Repeat background for that many pixels.
template <class P>
int vtkCompressCompositerCompositePair(float *z1, P *p1, float *z2, P *p2,
                                       float *zOut, P *pOut, int length1)
{
  float* startZOut = zOut;
  float* endZ1;
  // These counts keep track of the length of compressed runs.
  // Value -1 means pointer is not on a compression run.
  // Value 0 means pointer is on a used up compression run.
  int cCount1 = 0;
  int cCount2 = 0;
  int cCount3;
  int length3;
  
  // This is for the end test.
  // We are assuming that the uncompressed buffer length of 1 and 2 
  // are the same.
  endZ1 = z1 + length1;

  while(z1 != endZ1) 
    {
    // Initialize a new state if necessary.
    if (cCount1 == 0 && *z1 > 1.0)
      { // Detect a new run in buffer 1.
      cCount1 = (int)(*z1);
      }
    if (cCount2 == 0 && *z2 > 1.0)
      { // Detect a new run in buffer 2.
      cCount2 = (int)(*z2);
      }
       
    // Case 1: Neither buffer is compressed.
    // We could keep the length of uncompressed runs ...
    if (cCount1 == 0 && cCount2 == 0)
      {
      // Loop through buffers doing standard compositing.
      while (*z1 <= 1.0 && *z2 <= 1.0 && z1 != endZ1)
        {
        if (*z1 < *z2)
          {
          *zOut++ = *z1++;
          ++z2;
          *pOut++ = *p1++;
          ++p2;
          }
        else
          {            
          *zOut++ = *z2++;
          ++z1;
          *pOut++ = *p2++;
          ++p1;
          }
        }
      // Let the next iteration determine the new state (counts).
      }
    else if (cCount1 > 0 && cCount2 > 0)
      { // segment where both are compressed
      // Pick the smaller compressed run an duplicate in output.
      cCount3 = (cCount1 < cCount2) ? cCount1 : cCount2;
      cCount2 -= cCount3;
      cCount1 -= cCount3;
      // Set the output pixel.
      *zOut++ = (float)(cCount3);
      // either pixel will do.
      *pOut++ = *p1;
      if (cCount1 == 0)
        {
        ++z1;
        ++p1;
        }
      if (cCount2 == 0)
        {
        ++z2;
        ++p2;
        }
      }
    else if (cCount1 > 0 && cCount2 == 0)
      { //1 is in a compressed run but 2 is not.
      // Copy from 2 until we hit a compressed region, 
      // or we run out of the 1 compressed run.
      while (cCount1 && *z2 <= 1.0)
        {
        *zOut++ = *z2++;
        *pOut++ = *p2++;
        --cCount1;
        }
      if (cCount1 == 0)
        {
        ++z1;
        ++p1;
        }
      }
    else if (cCount1 == 0 && cCount2 > 0)
      { //2 is in a compressed run but 1 is not.
      // Copy from 1 until we hit a compressed region, 
      // or we run out of the 2 compressed run.
      while (cCount2 && *z1 <= 1.0)
        {
        *zOut++ = *z1++;
        *pOut++ = *p1++;
        --cCount2;
        }
      if (cCount2 == 0)
        {
        ++z2;
        ++p2;
        }
      } // end case if.
    } // while not finished (process cases).
  // Here is a scary way to determine the length of the new buffer.
  length3 = zOut - startZOut;

  return length3;
}
         
//-------------------------------------------------------------------------
// Can handle compositing compressed buffers.
// z values above 1.0 mean: Repeat background for that many pixels.
void vtkCompressCompositer::CompositeImagePair(
  vtkFloatArray *localZ, vtkDataArray *localP,
  vtkFloatArray *remoteZ, vtkDataArray *remoteP,
  vtkFloatArray *outZ, vtkDataArray *outP) 
{
  float* z1 = localZ->GetPointer(0);
  float* z2 = remoteZ->GetPointer(0);
  float* z3 = outZ->GetPointer(0);
  void*  p1 = localP->GetVoidPointer(0);
  void*  p2 = remoteP->GetVoidPointer(0);
  void*  p3 = outP->GetVoidPointer(0);
  int length1 = localZ->GetNumberOfTuples();
  int l3;
  
  //vtkTimerLog::MarkStartEvent("Coomposite Image Pair");

  // This is just a complex switch statment 
  // to call the correct templated function.
  if (localP->GetDataType() == VTK_UNSIGNED_CHAR) 
    {
    if (localP->GetNumberOfComponents() == 3) 
      {
      l3 = vtkCompressCompositerCompositePair(
        z1, reinterpret_cast<vtkCharRGBType*>(p1),
        z2, reinterpret_cast<vtkCharRGBType*>(p2),
        z3, reinterpret_cast<vtkCharRGBType*>(p3),
                                              length1);
      }
    else if (localP->GetNumberOfComponents() == 4) 
      {
      l3 = vtkCompressCompositerCompositePair(
        z1, reinterpret_cast<vtkCharRGBAType*>(p1),
        z2, reinterpret_cast<vtkCharRGBAType*>(p2),
        z3, reinterpret_cast<vtkCharRGBAType*>(p3),
        length1);
      }
    else 
      {
      vtkGenericWarningMacro("Pixels have unexpected number of components.");
      return;
      }
    }
  else if (localP->GetDataType() == VTK_FLOAT && 
           localP->GetNumberOfComponents() == 4) 
    {
    l3 = vtkCompressCompositerCompositePair(
      z1, reinterpret_cast<vtkFloatRGBAType*>(p1),
      z2, reinterpret_cast<vtkFloatRGBAType*>(p2),
      z3, reinterpret_cast<vtkFloatRGBAType*>(p3),
      length1);
    }
  else
    {
    vtkGenericWarningMacro("Unexpected pixel type.");
    return;
    }

  outZ->SetNumberOfTuples(l3);
  outP->SetNumberOfTuples(l3);

  //vtkTimerLog::MarkEndEvent("Coomposite Image Pair");
}



#define vtkTCPow2(j) (1 << (j))

//----------------------------------------------------------------------------
inline int vtkTCLog2(int j, int& exact)
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

//----------------------------------------------------------------------------
void vtkCompressCompositer::CompositeBuffer(vtkDataArray *pBuf, 
                                            vtkFloatArray *zBuf,
                                            vtkDataArray *pTmp, 
                                            vtkFloatArray *zTmp)
{
  int myId = this->Controller->GetLocalProcessId();
  int numProcs = this->NumberOfProcesses;
  int i, id;
  int exactLog;
  int logProcs = vtkTCLog2(numProcs,exactLog);
  int uncompressedLength = zBuf->GetNumberOfTuples();
  int bufSize=0;
  int numComps = pBuf->GetNumberOfComponents();
  vtkDataArray  *p1, *p2, *p3;
  vtkFloatArray *z1, *z2, *z3;

  //this->Timer->StartTimer();

  // Make sure we have an internal buffer of the correct length.
  if (this->InternalPData == NULL || 
      this->InternalPData->GetDataType() != pBuf->GetDataType() ||
      this->InternalPData->GetNumberOfTuples() != pBuf->GetNumberOfTuples() ||
      this->InternalPData->GetSize() < pBuf->GetSize())
    {
    if (this->InternalPData)
      {
      vtkCompositer::DeleteArray(this->InternalPData);
      this->InternalPData = NULL;
      }
    if (pBuf->GetDataType() == VTK_UNSIGNED_CHAR)
      {
      this->InternalPData = vtkUnsignedCharArray::New();
      vtkCompositer::ResizeUnsignedCharArray(
        static_cast<vtkUnsignedCharArray*>(this->InternalPData),
        numComps, pBuf->GetSize());
      }
    else 
      {
      this->InternalPData = vtkFloatArray::New();
      vtkCompositer::ResizeFloatArray(
        static_cast<vtkFloatArray*>(this->InternalPData),
        numComps, pBuf->GetSize());
      }
    }
  // Now float array.
  if (this->InternalZData == NULL || 
      this->InternalZData->GetSize() < zBuf->GetSize())
    {
    if (this->InternalZData)
      {
      vtkCompositer::DeleteArray(this->InternalZData);
      this->InternalZData = NULL;
      }
    this->InternalZData = vtkFloatArray::New();
    vtkCompositer::ResizeFloatArray(
      static_cast<vtkFloatArray*>(this->InternalZData),
      1, zBuf->GetSize());
    }

  // Compress the incoming buffers (in place operation).
  this->Compress(zBuf, pBuf, zTmp, pTmp);

  // We are going to need to shuffle these around during compositing.
  p1 = pTmp;
  z1 = zTmp;
  p2 = this->InternalPData;
  z2 = this->InternalZData;

  // not a power of 2 -- need an additional level
  if ( !exactLog ) 
    {
    logProcs++;
    }

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
          this->Controller->Receive(&bufSize, 1, id, 98);
          this->Controller->Receive(zBuf->GetPointer(0), bufSize, id, 99);
          this->Controller->Receive(&bufSize, 1, id, 98);
          if (pTmp->GetDataType() == VTK_UNSIGNED_CHAR)
            {
            this->Controller->Receive(reinterpret_cast<unsigned char*>
                                      (pBuf->GetVoidPointer(0)), 
                                      bufSize, id, 99);
            }
          else
            {
            this->Controller->Receive(reinterpret_cast<float*>
                                      (pBuf->GetVoidPointer(0)), 
                                      bufSize, id, 99);
            }
          
          // notice the result is stored as the local data
          this->CompositeImagePair(z1, p1, zBuf, pBuf, z2, p2);
          // Swap the temp buffers (p3/z3 are just temporary storage).
          p3 = p1;
          z3 = z1;
          p1 = p2;
          z1 = z2;
          p2 = p3;
          z2 = z3;
          }
        }
      else 
        { // The current data is always in buffer 1.
        id = myId-vtkTCPow2(i);
        if (id < numProcs) 
          {
          bufSize = z1->GetNumberOfTuples();
          this->Controller->Send(&bufSize, 1, id, 98);
          this->Controller->Send(z1->GetPointer(0), bufSize, id, 99);
          bufSize = p1->GetNumberOfTuples() * numComps;
          this->Controller->Send(&bufSize, 1, id, 98);
          if (p1->GetDataType() == VTK_UNSIGNED_CHAR)
            {
            this->Controller->Send(reinterpret_cast<unsigned char*>
                                   (p1->GetVoidPointer(0)), 
                                   bufSize, id, 99);
            }
          else
            {
            this->Controller->Send(reinterpret_cast<float*>
                                   (p1->GetVoidPointer(0)), 
                                   bufSize, id, 99);
            }
          }
        }
      }
    }

#ifdef MPIPROALLOC
  vtkCommunicator::SetUseCopy(1);
#endif


  if (myId == 0)
    {
    // Now we want to decompress into the original buffers.
    this->Uncompress(z1, p1, zBuf, pBuf, uncompressedLength);
    }

  //this->Timer->StopTimer();
  //float time = this->Timer->GetElapsedTime();
  //cerr << "Composite " << " took " << time << " seconds.\n";

}




//----------------------------------------------------------------------------
void vtkCompressCompositer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}



