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
  this->Dimensionality = 3;
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
void vtkImageSeedConnectivity::AddSeed(int i0, int i1)
{
  int index[2];

  index[0] = i0;
  index[1] = i1;
  this->AddSeed(2, index);
}



//----------------------------------------------------------------------------
// Update the whole image in cache because we will be generating the whole
// image anyway.
void vtkImageSeedConnectivity::InterceptCacheUpdate()
{
  // Filter superclass has no control of intercept cache update.
  // a work around
  if (this->Bypass)
    {
    return;
    }
  
  this->Output->SetUpdateExtent(this->Output->GetWholeExtent());
}


//----------------------------------------------------------------------------
void vtkImageSeedConnectivity::Execute(vtkImageData *inData,
				       vtkImageData *outData)
{
  vtkImageConnectorSeed *seed;
  int idx0, idx1, idx2;
  int inInc0, inInc1, inInc2;
  int outInc0, outInc1, outInc2;
  int min0, max0, min1, max1, min2, max2;
  unsigned char *inPtr0, *inPtr1, *inPtr2;
  unsigned char *outPtr0, *outPtr1, *outPtr2;
  unsigned char temp1, temp2;
  int temp;

  if (inData->GetScalarType() != VTK_UNSIGNED_CHAR ||
      outData->GetScalarType() != VTK_UNSIGNED_CHAR)
    {
    vtkErrorMacro("Execute: Both input and output must have scalar type UnsignedChar");
    return;
    }

  // Pick an intermediate value (In some cases, we could eliminate the last threshold.)
  temp1 = 1;
  while (temp1 == this->InputConnectValue || 
	 temp1 == this->OutputUnconnectedValue ||
	 temp1 == this->OutputConnectedValue)
    {
    ++temp1;
    }
  temp2 = temp1 + 1;
  while (temp2 == this->InputConnectValue || 
	 temp2 == this->OutputUnconnectedValue ||
	 temp2 == this->OutputConnectedValue)
    {
    ++temp2;
    }
  
  //-------
  // threshold to eliminate unknown values ( only intermediate and 0)
  inData->GetIncrements(inInc0, inInc1, inInc2);
  this->Output->GetUpdateExtent(min0, max0, min1, max1, min2, max2);
  outData->GetIncrements(outInc0, outInc1, outInc2);
  inPtr2 = (unsigned char *)(inData->GetScalarPointer(min0,min1,min2));
  outPtr2 = (unsigned char *)(outData->GetScalarPointer(min0,min1,min2));
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
          *outPtr0 = temp1;
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
  
  this->UpdateProgress(0.2);
  if (this->AbortExecute)
    {
    return;
    }
  
  //-------
  // find actual seeds in this image. (only scan along the first axis for now)
  this->Connector->RemoveAllSeeds();
  seed = this->Seeds;
  while (seed)
    {
    temp = seed->Index[0];
    // make sure z value of seed is acceptable
    if (seed->Index[2] < min2)
      {
      seed->Index[2] = min2;
      }
    if (seed->Index[2] > max2)
      {
      seed->Index[2] = max2;
      }
    outPtr0 = (unsigned char *)(outData->GetScalarPointer(seed->Index));
    for (idx0 = temp; idx0 <= max0; ++idx0)
      {
      if (*outPtr0 == temp1)
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

  this->UpdateProgress(0.5);
  if (this->AbortExecute)
    {
    return;
    }

  //-------
  // connect
  this->Connector->SetUnconnectedValue(temp1);
  this->Connector->SetConnectedValue(temp2);
  this->Connector->MarkData(outData, this->Dimensionality, 
			    this->Output->GetUpdateExtent());

  this->UpdateProgress(0.9);
  if (this->AbortExecute)
    {
    return;
    }

  //-------
  // Threshold to convert intermediate values into OutputUnconnectedValues
  outPtr2 = (unsigned char *)(outData->GetScalarPointer(min0,min1,min2));
  for (idx2 = min2; idx2 <= max2; ++idx2)
    {
    outPtr1 = outPtr2;
    for (idx1 = min1; idx1 <= max1; ++idx1)
      {
      outPtr0 = outPtr1;
      for (idx0 = min0; idx0 <= max0; ++idx0)
        {
        if (*outPtr0 == temp2)
          {
          *outPtr0 = this->OutputConnectedValue;
          }
	else
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


void vtkImageSeedConnectivity::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageFilter::PrintSelf(os,indent);

  if ( this->Connector )
    {
    os << indent << "Connector: " << this->Connector << "\n";
    }
  else
    {
    os << indent << "Connector: (none)\n";
    }

  os << indent << "Dimensionality: " << this->Dimensionality << "\n";
  os << indent << "InputConnectValue: " << this->InputConnectValue << "\n";
  os << indent << "OutputConnectedValue: " << this->OutputConnectedValue << "\n";
  os << indent << "OutputUnconnectedValue: " << this->OutputUnconnectedValue << "\n";
}

