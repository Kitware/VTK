/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageEuclideanToPolar.cxx
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
#include <math.h>
#include "vtkImageRegion.h"
#include "vtkImageEuclideanToPolar.h"



//----------------------------------------------------------------------------
vtkImageEuclideanToPolar::vtkImageEuclideanToPolar()
{
  this->ThetaMaximum = 255.0;
  // One pixel at a time. (sssssslowwww)
  this->SetExecutionAxes(VTK_IMAGE_COMPONENT_AXIS);
}

//----------------------------------------------------------------------------
template <class T>
static void vtkImageEuclideanToPolarExecute(vtkImageEuclideanToPolar *self,
				    vtkImageRegion *inRegion, T *inPtr,
				    vtkImageRegion *outRegion, T *outPtr)
{
  float X, Y, Theta, R;
  float thetaMax = self->GetThetaMaximum();
  int inInc;
  int outInc;
  
  inRegion->GetAxisIncrements(VTK_IMAGE_COMPONENT_AXIS, inInc);
  outRegion->GetAxisIncrements(VTK_IMAGE_COMPONENT_AXIS, outInc);
  
  X = (float)(*inPtr);
  inPtr += inInc;
  Y = (float)(*inPtr);

  Theta = atan2(X, Y);
  R = sqrt(X*X + Y*Y);

  // assign output.
  *outPtr = (T)(Theta);
  outPtr += outInc;
  *outPtr = (T)(R);
}


//----------------------------------------------------------------------------
void vtkImageEuclideanToPolar::Execute(vtkImageRegion *inRegion, 
			       vtkImageRegion *outRegion)
{
  int min, max;
  void *inPtr = inRegion->GetScalarPointer();
  void *outPtr = outRegion->GetScalarPointer();
  
  if (inRegion->GetScalarType() != outRegion->GetScalarType())
    {
    vtkErrorMacro("Scalar type of input, " 
      << vtkImageScalarTypeNameMacro(inRegion->GetScalarType())
      << ", must match scalar type of output, "
      << vtkImageScalarTypeNameMacro(outRegion->GetScalarType()));
    return;
    }
  inRegion->GetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, min, max);
  if (max - min + 1 < 2)
    {
    vtkErrorMacro("Input has too few components");
    return;
    }
  outRegion->GetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, min, max);
  if (max - min + 1 < 2)
    {
    vtkErrorMacro("Output has too few components");
    return;
    }
  
  switch (inRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageEuclideanToPolarExecute(this, inRegion, (float *)inPtr, 
			      outRegion, (float *)outPtr);
      break;
    case VTK_SHORT:
      vtkImageEuclideanToPolarExecute(this, inRegion, (short *)inPtr, 
			      outRegion, (short *)outPtr);
      break;
    case VTK_INT:
      vtkImageEuclideanToPolarExecute(this, inRegion, (int *)inPtr, 
			      outRegion, (int *)outPtr);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageEuclideanToPolarExecute(this, inRegion, (unsigned short *)inPtr, 
			      outRegion, (unsigned short *)outPtr);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageEuclideanToPolarExecute(this, inRegion, (unsigned char*)inPtr, 
			      outRegion, (unsigned char *)outPtr);
      break;
    default:
      vtkErrorMacro("Unknown data type" << inRegion->GetScalarType());
      return;
    }
}

















