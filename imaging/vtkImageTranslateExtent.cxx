/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageTranslateExtent.cxx
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
#include "vtkImageTranslateExtent.h"


//----------------------------------------------------------------------------
vtkImageTranslateExtent::vtkImageTranslateExtent()
{
  int idx;

  for (idx = 0; idx < 3; ++idx)
    {
    this->Translation[idx]  = 0;
    }
}


//----------------------------------------------------------------------------
void vtkImageTranslateExtent::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageFilter::PrintSelf(os,indent);

  os << indent << "Translation: (" << this->Translation[0]
     << "," << this->Translation[1] << "," << this->Translation[2] << endl;
}
  



//----------------------------------------------------------------------------
// Change the WholeExtent
void vtkImageTranslateExtent::ExecuteImageInformation()
{
  int idx, extent[6];
  float *spacing, origin[3];
  
  this->Input->GetWholeExtent(extent);
  this->Input->GetOrigin(origin);
  spacing = this->Input->GetSpacing();

  if ( ! this->Bypass)
    {
    // TranslateExtent the OutputWholeExtent with the input WholeExtent
    for (idx = 0; idx < 3; ++idx)
      {
      // change extent
      extent[2*idx] += this->Translation[idx];
      extent[2*idx+1] += this->Translation[idx];
      // change origin so the data does not shift
      origin[idx] -= (float)(this->Translation[idx]) * spacing[idx];
      }
    }
  
  this->Output->SetWholeExtent(extent);
  this->Output->SetOrigin(origin);
}


//----------------------------------------------------------------------------
// Description:
// This method simply copies by reference the input data to the output.
void vtkImageTranslateExtent::InternalUpdate(vtkImageData *outData)
{
  vtkImageData *inData;
  int extent[6], idx;
  
  // Make sure the Input has been set.
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "Input is not set.");
    return;
    }

  this->Output->GetUpdateExtent(extent);
  for (idx = 0; idx < 3; ++idx)
    {
    extent[idx*2] -= this->Translation[idx];
    extent[idx*2+1] -= this->Translation[idx];
    }
  
  this->Input->SetUpdateExtent(extent);

  inData = this->Input->UpdateAndReturnData();
  // since inData can be larger than update extent.
  inData->GetExtent(extent);
  for (idx = 0; idx < 3; ++idx)
    {
    extent[idx*2] += this->Translation[idx];
    extent[idx*2+1] += this->Translation[idx];
    }
  outData->SetExtent(extent);
  outData->GetPointData()->PassData(inData->GetPointData());
  
  // release input data
  if (this->Input->ShouldIReleaseData())
    {
    this->Input->ReleaseData();
    }
}


