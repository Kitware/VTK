/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageGradientDerivative.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include <math.h>
#include "vtkImageGradientDerivative.h"


//----------------------------------------------------------------------------
vtkImageGradientDerivative::vtkImageGradientDerivative()
{
  this->SetAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS, VTK_IMAGE_Z_AXIS,
		VTK_IMAGE_COMPONENT_AXIS);
  this->SetOutputScalarType(VTK_FLOAT);
  
  // 3 + component axis
  this->ExecuteDimensionality = 4;
  // Not used, but the input is 4d output is 3D.  what should I tell
  // the user.  (I really should be consistent, and treat component
  // axis like any other, but I have already started this trend.)
  this->Dimensionality = 3;
}

//----------------------------------------------------------------------------
void vtkImageGradientDerivative::ComputeOutputImageInformation(
		       vtkImageRegion *inRegion, vtkImageRegion *outRegion)
{
  inRegion = inRegion;
  outRegion->SetAxisImageExtent(VTK_IMAGE_COMPONENT_AXIS, 0, 0);
}

//----------------------------------------------------------------------------
void vtkImageGradientDerivative::ComputeRequiredInputRegionExtent(
			  vtkImageRegion *outRegion, vtkImageRegion *inRegion)
{
  int saveAxes[VTK_IMAGE_DIMENSIONS];
  int extent[VTK_IMAGE_EXTENT_DIMENSIONS];
  int *imageExtent;
  int idx;

  // Put component axis first.
  outRegion->GetAxes(saveAxes);
  outRegion->SetAxes(VTK_IMAGE_COMPONENT_AXIS);
  inRegion->SetAxes(VTK_IMAGE_COMPONENT_AXIS);
  
  // Expand all but the component axis. (which gets all)
  imageExtent = inRegion->GetImageExtent();
  outRegion->GetExtent(extent);
  // Component Axis
  extent[0] = imageExtent[0];
  extent[1] = imageExtent[1];
  // Expand rest
  for (idx = 1; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    if (extent[2*idx] > imageExtent[2*idx])
      {
      --extent[2*idx];
      }
    if (extent[2*idx + 1] < imageExtent[2*idx + 1])
      {
      ++extent[2*idx + 1];
      }
    }
  inRegion->SetExtent(extent);
  
  // restore the original axes
  inRegion->SetAxes(saveAxes);
  outRegion->SetAxes(saveAxes);
}

//----------------------------------------------------------------------------
void vtkImageGradientDerivative::Execute(vtkImageRegion *inRegion, 
					 vtkImageRegion *outRegion)
{
  vtkImageRegion *magnitudes;

  if (inRegion->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro(<< "Execute: Can only handle floats");
    return;
    }
  
  // Get the magnitude image.
  magnitudes = vtkImageRegion::New();
  this->ComputeMagnitudes(inRegion, magnitudes);

  // Compute the derivative information
  this->ComputeDerivatives(inRegion, magnitudes, outRegion);

  // GetRid of this temporary image.
  magnitudes->Delete();
}





//----------------------------------------------------------------------------
// This method computes magnitude of the vectors internally.
void vtkImageGradientDerivative::ComputeMagnitudes(vtkImageRegion *vectors,
						   vtkImageRegion *magnitudes)
{
  int idx0, idx1, idx2, idx3;
  int min0, max0, min1, max1, min2, max2, min3, max3;
  int vInc0, vInc1, vInc2, vInc3, mInc0, mInc1, mInc2;
  float *vPtr0, *vPtr1, *vPtr2, *vPtr3, *mPtr0, *mPtr1, *mPtr2;
  float sum;
  
  // Set up the derivative image
  magnitudes->SetScalarType(VTK_FLOAT);
  magnitudes->SetExtent(vectors->GetExtent());
  magnitudes->SetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, 0, 0);
  vectors->GetExtent(min0, max0, min1, max1, min2, max2);
  vectors->GetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, min3, max3);

  // Get information to loop through images.
  mPtr2 = (float *)(magnitudes->GetScalarPointer());
  vPtr2 = (float *)(vectors->GetScalarPointer());
  magnitudes->GetIncrements(mInc0, mInc1, mInc2);
  vectors->GetIncrements(vInc0, vInc1, vInc2);
  vectors->GetAxisIncrements(VTK_IMAGE_COMPONENT_AXIS, vInc3);
  
  for (idx2 = min2; idx2 <= max2; ++idx2)
    {
    vPtr1 = vPtr2;
    mPtr1 = mPtr2;
    for (idx1 = min1; idx1 <= max1; ++idx1)
      {
      vPtr0 = vPtr1;
      mPtr0 = mPtr1;
      for (idx0 = min0; idx0 <= max0; ++idx0)
	{
	sum = 0.0;
	vPtr3 = vPtr0;
	for (idx3 = min3; idx3 <= max3; ++idx3)
	  {
	  sum += *vPtr3 * *vPtr3;
	  vPtr3 += vInc3;
	  }
	*mPtr0 = sqrt(sum);
	vPtr0 += vInc0;
	mPtr0 += mInc0;
	}
      vPtr1 += vInc1;
      mPtr1 += mInc1;
      }
    vPtr2 += vInc2;
    mPtr2 += mInc2;
    }
}

//----------------------------------------------------------------------------
// This method computes the scalar derivative from vector and magnitude 
// images.  Derivitive is computed by taking the dot product of the vector 
// with the magnitude gradient.
void vtkImageGradientDerivative::ComputeDerivatives(vtkImageRegion *vectors,
					     vtkImageRegion *magnitudes,
					     vtkImageRegion *derivatives)
{
  int idx0, idx1, idx2, idx3, idxs[3], *mIncs;
  int min0, max0, min1, max1, min2, max2, min3, max3;
  int *mExtent, *temp;
  int vInc0, vInc1, vInc2, vInc3, mInc0, mInc1, mInc2, dInc0, dInc1, dInc2;
  float *vPtr0, *vPtr1, *vPtr2, *vPtr3;
  float *mPtr0, *mPtr1, *mPtr2, *dPtr0, *dPtr1, *dPtr2;
  float valLeft, valRight;
  float dot;

  // Get information to loop through images.
  derivatives->GetExtent(min0, max0, min1, max1, min2, max2);
  vectors->GetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, min3, max3);
  vPtr2 = (float *)(vectors->GetScalarPointer(min0, min1, min2));
  mPtr2 = (float *)(magnitudes->GetScalarPointer(min0, min1, min2));
  dPtr2 = (float *)(derivatives->GetScalarPointer());
  vectors->GetIncrements(vInc0, vInc1, vInc2);
  vectors->GetAxisIncrements(VTK_IMAGE_COMPONENT_AXIS, vInc3);
  magnitudes->GetIncrements(mInc0, mInc1, mInc2);
  mExtent = magnitudes->GetExtent();
  mIncs = magnitudes->GetIncrements();
  derivatives->GetIncrements(dInc0, dInc1, dInc2);

  if (max3 > 2 || min3 < 0)
    {
    vtkErrorMacro(<< "Component extent is out of range (0,2)");
    return;
    }
  
  for (idx2 = min2; idx2 <= max2; ++idx2)
    {
    vPtr1 = vPtr2;
    mPtr1 = mPtr2;
    dPtr1 = dPtr2;
    idxs[2] = idx2;
    for (idx1 = min1; idx1 <= max1; ++idx1)
      {
      vPtr0 = vPtr1;
      mPtr0 = mPtr1;
      dPtr0 = dPtr1;
      idxs[1] = idx1;
      for (idx0 = min0; idx0 <= max0; ++idx0)
	{
	vPtr3 = vPtr0;
	temp = mExtent;
	dot = 0.0;
	idxs[0] = idx0;
	for (idx3 = min3; idx3 <= max3; ++idx3)
	  {
	  // Central differences and dot product
	  valLeft = (idxs[idx3] == *temp++) ? *mPtr0 : mPtr0[-mIncs[idx3]];
	  valRight = (idxs[idx3] == *temp++) ? *mPtr0 : mPtr0[mIncs[idx3]];
	  dot += (valRight - valLeft) * *vPtr3;
	  vPtr3 += vInc3;
	  }
	// Save dot in derivative (normalize gradient for dot)
	*dPtr0 = dot / *mPtr0;

	vPtr0 += vInc0;
	mPtr0 += mInc0;
	dPtr0 += dInc0;
	}
      vPtr1 += vInc1;
      mPtr1 += mInc1;
      dPtr1 += dInc1;
      }
    vPtr2 += vInc2;
    mPtr2 += mInc2;
    dPtr2 += dInc2;
    }
}


