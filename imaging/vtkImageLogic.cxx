/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageLogic.cxx
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
#include "vtkImageLogic.h"
#include <math.h>



//----------------------------------------------------------------------------
vtkImageLogic::vtkImageLogic()
{
  this->SetExecutionAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS);
  this->Operation = VTK_AND;
}



//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
// Handles the one input operations
template <class T>
static void vtkImageLogicExecute1(vtkImageLogic *self,
				  vtkImageRegion *in1Region, T *in1Ptr,
				  vtkImageRegion *outRegion, 
				  unsigned char *outPtr)
{
  int op;
  int min0, max0, min1, max1;
  int idx0, idx1;
  int in1Inc0, in1Inc1;
  int outInc0, outInc1;
  T *in1Ptr0, *in1Ptr1;
  unsigned char *outPtr0, *outPtr1;
  unsigned char trueValue = self->GetOutputTrueValue();

  op = self->GetOperation();
  
  // Get information to march through data 
  in1Region->GetIncrements(in1Inc0, in1Inc1);
  outRegion->GetIncrements(outInc0, outInc1);
  outRegion->GetExtent(min0, max0, min1, max1);

  // Loop through ouput pixels
  in1Ptr1 = in1Ptr;
  outPtr1 = outPtr;
  for (idx1 = min1; idx1 <= max1; ++idx1)
    {
    outPtr0 = outPtr1;
    in1Ptr0 = in1Ptr1;
    for (idx0 = min0; idx0 <= max0; ++idx0)
      {
      
      // Pixel operaton
      switch (op)
	{
	case VTK_NOT:
	  if ( ! *outPtr0)
	    {
	    *outPtr0 = trueValue;
	    }
	  else
	    {
	    *outPtr0 = 0;
	    }
	  break;
	case VTK_NOP:
	  if (*outPtr0)
	    {
	    *outPtr0 = trueValue;
	    }
	  else
	    {
	    *outPtr0 = 0;
	    }
	  break;
	}
      
      outPtr0 += outInc0;
      in1Ptr0 += in1Inc0;
      }
    outPtr1 += outInc1;
    in1Ptr1 += in1Inc1;
    }
}



//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
// Handles the two input operations
template <class T>
static void vtkImageLogicExecute2(vtkImageLogic *self,
				  vtkImageRegion *in1Region, T *in1Ptr,
				  vtkImageRegion *in2Region, T *in2Ptr,
				  vtkImageRegion *outRegion, 
				  unsigned char *outPtr)
{
  int op;
  int min0, max0, min1, max1;
  int idx0, idx1;
  int in1Inc0, in1Inc1;
  int in2Inc0, in2Inc1;
  int outInc0, outInc1;
  T *in1Ptr0, *in1Ptr1;
  T *in2Ptr0, *in2Ptr1;
  unsigned char *outPtr0, *outPtr1;
  unsigned char trueValue = self->GetOutputTrueValue();
  
  op = self->GetOperation();
  
  // Get information to march through data 
  in1Region->GetIncrements(in1Inc0, in1Inc1);
  in2Region->GetIncrements(in2Inc0, in2Inc1);
  outRegion->GetIncrements(outInc0, outInc1);
  outRegion->GetExtent(min0, max0, min1, max1);

  // Loop through ouput pixels
  in1Ptr1 = in1Ptr;
  in2Ptr1 = in2Ptr;
  outPtr1 = outPtr;
  for (idx1 = min1; idx1 <= max1; ++idx1)
    {
    outPtr0 = outPtr1;
    in1Ptr0 = in1Ptr1;
    in2Ptr0 = in2Ptr1;
    for (idx0 = min0; idx0 <= max0; ++idx0)
      {
      
      // Pixel operaton
      switch (op)
	{
	case VTK_AND:
	  if (*in1Ptr0 && *in2Ptr0)
	    {
	    *outPtr0 = trueValue;
	    }
	  else
	    {
	    *outPtr0 = 0;
	    }
	  break;
	case VTK_OR:
	  if (*in1Ptr0 || *in2Ptr0)
	    {
	    *outPtr0 = trueValue;
	    }
	  else
	    {
	    *outPtr0 = 0;
	    }
	  break;
	case VTK_XOR:
	  if (( ! *in1Ptr0 && *in2Ptr0) || (*in1Ptr0 && ! *in2Ptr0))
	    {
	    *outPtr0 = trueValue;
	    }
	  else
	    {
	    *outPtr0 = 0;
	    }
	  break;
	case VTK_NAND:
	  if ( ! (*in1Ptr0 && *in2Ptr0))
	    {
	    *outPtr0 = trueValue;
	    }
	  else
	    {
	    *outPtr0 = 0;
	    }
	  break;
	case VTK_NOR:
	  if ( ! (*in1Ptr0 || *in2Ptr0))
	    {
	    *outPtr0 = trueValue;
	    }
	  else
	    {
	    *outPtr0 = 0;
	    }
	  break;
	}
      
      
      outPtr0 += outInc0;
      in1Ptr0 += in1Inc0;
      in2Ptr0 += in2Inc0;
      }
    outPtr1 += outInc1;
    in1Ptr1 += in1Inc1;
    in2Ptr1 += in2Inc1;
    }
}



//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output regions, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageLogic::Execute(vtkImageRegion *inRegion1, 
			    vtkImageRegion *inRegion2, 
			    vtkImageRegion *outRegion)
{
  void *in1Ptr = inRegion1->GetScalarPointer();
  void *outPtr = outRegion->GetScalarPointer();

  if (this->Operation == VTK_NOT || this->Operation == VTK_NOP)
    {
    // this filter expects that inputs are the same type as output.
    if (inRegion1->GetScalarType() != VTK_FLOAT ||
	outRegion->GetScalarType() != VTK_FLOAT)
      {
      vtkErrorMacro(<< "Execute: input ScalarType, "
        << vtkImageScalarTypeNameMacro(inRegion1->GetScalarType()) 
        << ", and out ScalarType "
        << vtkImageScalarTypeNameMacro(outRegion->GetScalarType())
        << " must be float for choosen operation");
      return;
      }
    switch (inRegion1->GetScalarType())
      {
      case VTK_FLOAT:
	vtkImageLogicExecute1(this, inRegion1, (float *)(in1Ptr), 
			      outRegion, (unsigned char *)(outPtr));
	break;
      case VTK_INT:
	vtkImageLogicExecute1(this, inRegion1, (int *)(in1Ptr), 
			      outRegion, (unsigned char *)(outPtr));
	break;
      case VTK_SHORT:
	vtkImageLogicExecute1(this, inRegion1, (short *)(in1Ptr), 
			      outRegion, (unsigned char *)(outPtr));
	break;
      case VTK_UNSIGNED_SHORT:
	vtkImageLogicExecute1(this, inRegion1, (unsigned short *)(in1Ptr), 
			      outRegion, (unsigned char *)(outPtr));
	break;
      case VTK_UNSIGNED_CHAR:
	vtkImageLogicExecute1(this, inRegion1, (unsigned char *)(in1Ptr), 
			      outRegion, (unsigned char *)(outPtr));
	break;
      default:
	vtkErrorMacro(<< "Execute: Unknown ScalarType");
	return;
      }
    }
  else
    {
    void *in2Ptr = inRegion2->GetScalarPointer();
    // this filter expects that inputs are the same type as output.
    if (inRegion1->GetScalarType() != outRegion->GetScalarType() ||
	inRegion2->GetScalarType() != outRegion->GetScalarType())
      {
      vtkErrorMacro(<< "Execute: input ScalarTypes, " 
        << inRegion1->GetScalarType() << " and " << inRegion2->GetScalarType()
        << ", must match out ScalarType " << outRegion->GetScalarType());
      return;
    }
    
    switch (inRegion1->GetScalarType())
      {
      case VTK_FLOAT:
	vtkImageLogicExecute2(this, inRegion1, (float *)(in1Ptr), 
			      inRegion2, (float *)(in2Ptr), 
			      outRegion, (unsigned char *)(outPtr));
	break;
      case VTK_INT:
	vtkImageLogicExecute2(this, inRegion1, (int *)(in1Ptr), 
			      inRegion2, (int *)(in2Ptr), 
			      outRegion, (unsigned char *)(outPtr));
	break;
      case VTK_SHORT:
	vtkImageLogicExecute2(this, inRegion1, (short *)(in1Ptr), 
			      inRegion2, (short *)(in2Ptr), 
			      outRegion, (unsigned char *)(outPtr));
	break;
      case VTK_UNSIGNED_SHORT:
	vtkImageLogicExecute2(this, inRegion1, (unsigned short *)(in1Ptr), 
			      inRegion2, (unsigned short *)(in2Ptr), 
			      outRegion, (unsigned char *)(outPtr));
	break;
      case VTK_UNSIGNED_CHAR:
	vtkImageLogicExecute2(this, inRegion1, (unsigned char *)(in1Ptr), 
			      inRegion2, (unsigned char *)(in2Ptr), 
			      outRegion, (unsigned char *)(outPtr));
	break;
      default:
	vtkErrorMacro(<< "Execute: Unknown ScalarType");
	return;
      }
    }
}
















