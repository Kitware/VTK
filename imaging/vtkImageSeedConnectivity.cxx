/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSeedConnectivity.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkImageRegion.h"
#include "vtkImageCache.h"
#include "vtkImageSeedConnectivity.h"


//----------------------------------------------------------------------------
vtkImageSeedConnectivity::vtkImageSeedConnectivity()
{
  this->InputConnectValue = 255;
  this->OutputConnectedValue = 255;
  this->OutputUnconnectedValue = 0;
  this->Seeds = NULL;
  this->Connector = vtkImageConnector::New();
}

//----------------------------------------------------------------------------
vtkImageSeedConnectivity::~vtkImageSeedConnectivity()
{
  this->Connector->Delete();
  this->RemoveAllSeeds();
}

//----------------------------------------------------------------------------
void vtkImageSeedConnectivity::RemoveAllSeeds()
{
  vtkImageConnectorSeed *temp;
  while (this->Seeds)
    {
    temp = this->Seeds;
    this->Seeds = temp->Next;
    delete temp;
    }
}

//----------------------------------------------------------------------------
void vtkImageSeedConnectivity::SetFilteredAxes(int num, int *axes)
{
  if (num > 3)
    {
    vtkWarningMacro("SetFilteredAxes: Only handle up to three axes");
    num = 3;
   }
  this->vtkImageFilter::SetFilteredAxes(num, axes);
}


//----------------------------------------------------------------------------
void vtkImageSeedConnectivity::AddSeed(int num, int *index)
{
  int idx, newIndex[3];
  vtkImageConnectorSeed *seed;
  
  if (num > 3)
    {
    num = 3;
    } 
  for (idx = 0; idx < num; ++idx)
    {
    newIndex[idx] = index[idx];
    }
  for (idx = num; idx < 3; ++idx)
    {
    newIndex[idx] = 0;
    }
  seed = this->Connector->NewSeed(newIndex, NULL);
  seed->Next = this->Seeds;
  this->Seeds = seed;
}

//----------------------------------------------------------------------------
void vtkImageSeedConnectivity::AddSeed(int i0, int i1, int i2)
{
  int index[3];

  index[0] = i0;
  index[1] = i1;
  index[2] = i2;
  this->AddSeed(3, index);
}



//----------------------------------------------------------------------------
// Update the whole image in cache because we will be generating the whole
// image anyway.
void vtkImageSeedConnectivity::InterceptCacheUpdate(vtkImageCache *out)
{
  int idx, axis;
  int min, max;

  for (idx = 0; idx < this->NumberOfFilteredAxes; ++idx)
    {
    axis = this->FilteredAxes[idx];
    out->GetAxisWholeExtent(axis, min, max);
    out->SetAxisUpdateExtent(axis, min, max);
    }    
}


//----------------------------------------------------------------------------
void vtkImageSeedConnectivity::Execute(vtkImageRegion *inRegion,
                                   vtkImageRegion *outRegion)
{
  vtkImageConnectorSeed *seed;
  int idx0, idx1, idx2;
  int inInc0, inInc1, inInc2;
  int outInc0, outInc1, outInc2;
  int min0, max0, min1, max1, min2, max2;
  unsigned char *inPtr0, *inPtr1, *inPtr2;
  unsigned char *outPtr0, *outPtr1, *outPtr2;
  unsigned char intermediateValue;
  int temp;

  if (inRegion->GetScalarType() != VTK_UNSIGNED_CHAR ||
       outRegion->GetScalarType() != VTK_UNSIGNED_CHAR)
    {
    vtkErrorMacro("Execute: Both input and output must have scalar type UnsignedChar");
    return;
    }

  // Pick an intermediate value (In some cases, we could eliminate the last threshold.)
  intermediateValue = 1;

  //-------
  // threshold to eliminate unknown values ( only intermediate and 0)
  inRegion->GetIncrements(inInc0, inInc1, inInc2);
  inRegion->GetExtent(min0, max0, min1, max1, min2, max2);
  outRegion->GetIncrements(outInc0, outInc1, outInc2);
  inPtr2 = (unsigned char *)(inRegion->GetScalarPointer());
  outPtr2 = (unsigned char *)(outRegion->GetScalarPointer());
  for (idx2 = min2; idx2 <= max2; ++idx2)
    {
    inPtr1 = inPtr2;
    outPtr1 = outPtr2;
    for (idx1 = min1; idx1 <= max1; ++idx1)
      {
      inPtr0 = inPtr1;
      outPtr0 = outPtr1;
      for (idx0 = min0; idx0 <= max0; ++idx0)
        {
        if (*inPtr0 == this->InputConnectValue)
          {
          *outPtr0 = intermediateValue;
          }
        else
          {
          *outPtr0 = 0;
          }
        inPtr0 += inInc0;
        outPtr0 += outInc0;
        }
      inPtr1 += inInc1;
      outPtr1 += outInc1;
      }
    inPtr2 += inInc2;
    outPtr2 += outInc2;
    }

  //-------
  // find actual seeds in this image. (only scan along the first axis for now)
  this->Connector->RemoveAllSeeds();
  seed = this->Seeds;
  while (seed)
    {
    temp = seed->Index[0];
    outPtr0 = (unsigned char *)(outRegion->GetScalarPointer(this->NumberOfFilteredAxes, seed->Index));
    for (idx0 = temp; idx0 <= max0; ++idx0)
      {
      if (*outPtr0 == intermediateValue)
        { // we found our seed
        seed->Index[0] = idx0;
        this->Connector->AddSeed(this->Connector->NewSeed(seed->Index, outPtr0));
        seed->Index[0] = temp;
        break;
        }
      outPtr0 += outInc0;
      }
    seed = seed->Next;
    }

  //-------
  // connect
  this->Connector->SetUnconnectedValue(intermediateValue);
  this->Connector->SetConnectedValue(this->OutputConnectedValue);
  this->Connector->MarkRegion(outRegion, this->NumberOfFilteredAxes);

  //-------
  // Threshold to convert intermediate values into OutputUnconnectedValues
  outPtr2 = (unsigned char *)(outRegion->GetScalarPointer());
  for (idx2 = min2; idx2 <= max2; ++idx2)
    {
    outPtr1 = outPtr2;
    for (idx1 = min1; idx1 <= max1; ++idx1)
      {
      outPtr0 = outPtr1;
      for (idx0 = min0; idx0 <= max0; ++idx0)
        {
        if (*outPtr0 == intermediateValue)
          {
          *outPtr0 = this->OutputUnconnectedValue;
          }
        outPtr0 += outInc0;
        }
      outPtr1 += outInc1;
      }
     outPtr2 += outInc2;
    }
}




