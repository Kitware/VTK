/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageVolume16Reader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder,ill Lorensen.

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
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "vtkImageRegion.h"
#include "vtkImageCache.h"
#include "vtkImageVolume16Reader.h"



//----------------------------------------------------------------------------
vtkImageVolume16Reader::vtkImageVolume16Reader()
{
  this->FileDimensionality = 2;
  this->Transform = NULL;
}

//----------------------------------------------------------------------------
vtkImageVolume16Reader::~vtkImageVolume16Reader()
{ 
  
}

//----------------------------------------------------------------------------
void vtkImageVolume16Reader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageSeriesReader::PrintSelf(os,indent);

  if ( this->Transform )
    {
    os << indent << "Transform:\n";
    this->Transform->PrintSelf(os,indent.GetNextIndent());
    }
  else
    os << indent << "Transform: (None)\n";
}


//----------------------------------------------------------------------------
// Description:
// This method computes WholeExtent (the largest region that can be generated),
// Spacing and Origin.
void vtkImageVolume16Reader::UpdateImageInformation()
{
  int idx;
  int dataAxes[3];
  int dataFlips[3];
  float origin[3];
  
  // Set the axes from the transform
  this->ComputeTransformedDataAxes(dataAxes);
  this->SetExecutionAxes(3, dataAxes);
  this->NumberOfExecutionAxes = 5;

  // Set the flips (Note: "this" is in data coordinate system now)
  this->ComputeTransformedDataFlips(dataFlips);
  this->SetFlips(3, dataFlips);

  this->Output->SetAxesSpacing(3, dataAxes, this->DataSpacing);
  this->Output->SetAxesWholeExtent(3, dataAxes, this->DataExtent);
  
  // Shift the Origin to account for any flips
  for (idx = 0; idx < 3; ++idx)
    {
    if (dataFlips[idx])
      {
      origin[idx] = -this->DataOrigin[idx]
	- (this->DataSpacing[idx] * this->DataExtent[idx*2+1]);
      }
    else
      {
      origin[idx] = this->DataOrigin[idx];
      }
    }
  this->Output->SetAxesOrigin(3, dataAxes, origin);
}


//----------------------------------------------------------------------------
// Assumes no transform is x, y, z.  The output is the labels (order) of the
// file data axes.
void vtkImageVolume16Reader::ComputeTransformedDataAxes(int axes[3])
{
  float transformedAxes[4];
  
  if (!this->Transform)
    {
    axes[0] = VTK_IMAGE_X_AXIS;
    axes[1] = VTK_IMAGE_Y_AXIS;
    axes[2] = VTK_IMAGE_Z_AXIS;
    }
  else
    {
    transformedAxes[0] = (float)(VTK_IMAGE_X_AXIS);
    transformedAxes[1] = (float)(VTK_IMAGE_Y_AXIS);
    transformedAxes[2] = (float)(VTK_IMAGE_Z_AXIS);
    transformedAxes[3] = 1.0;
    this->Transform->Push();
    this->Transform->Inverse();
    this->Transform->MultiplyPoint (transformedAxes, transformedAxes);
    this->Transform->Pop();
    if (transformedAxes[0] < 0) transformedAxes[0] = -transformedAxes[0];
    if (transformedAxes[1] < 0) transformedAxes[1] = -transformedAxes[1];
    if (transformedAxes[2] < 0) transformedAxes[2] = -transformedAxes[2];
    axes[0] = (int) (transformedAxes[0] + .01);
    axes[1] = (int) (transformedAxes[1] + .01);
    axes[2] = (int) (transformedAxes[2] + .01);
    vtkDebugMacro(<< "Tranformed axes (Compute DataAxes) are:" 
        << axes[0] << ", " << axes[1] << ", " << axes[2]);
    }
}

//----------------------------------------------------------------------------
// Assumes no transform is x, y, z.  The output is flips in the Data
// coordinate system.
void vtkImageVolume16Reader::ComputeTransformedDataFlips(int flips[3])
{
  float transformedFlips[4];
  
  if (!this->Transform)
    {
    flips[0] = 0;
    flips[1] = 0;
    flips[2] = 0;
    }
  else
    {
    transformedFlips[0] = 1.0;
    transformedFlips[1] = 1.0;
    transformedFlips[2] = 1.0;
    transformedFlips[3] = 1.0;
    this->Transform->Push();
    this->Transform->Inverse();
    this->Transform->MultiplyPoint (transformedFlips, transformedFlips);
    this->Transform->Pop();
    flips[0] = (transformedFlips[0] > 0.0) ? 0 : 1;
    flips[1] = (transformedFlips[1] > 0.0) ? 0 : 1;
    flips[2] = (transformedFlips[2] > 0.0) ? 0 : 1;
    vtkDebugMacro(<< "Tranformed flips (Compute DataFlips) are:" 
        << flips[0] << ", " << flips[1] << ", " << flips[2]);
    }
}

  
  
  
  







