/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageTwoInputFilter.cxx
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
#include "vtkImageTwoInputFilter.h"


//----------------------------------------------------------------------------
vtkImageTwoInputFilter::vtkImageTwoInputFilter()
{
  this->SetNumberOfInputs(2);
  this->Inputs[0] = NULL;
  this->Inputs[1] = NULL;
}

//----------------------------------------------------------------------------
// Description:
// Set the Input1 of this filter. If a ScalarType has not been set,
// then the ScalarType of the input is used.
void vtkImageTwoInputFilter::SetInput1(vtkImageCache *input)
{
  vtkDebugMacro(<< "SetInput1: input = " << input->GetClassName()
		<< " (" << input << ")");

  // does this change anything?
  if (input == this->Inputs[0])
    {
    return;
    }
  
  this->Inputs[0] = input;
  this->Modified();
}



//----------------------------------------------------------------------------
// Description:
// Set the Input2 of this filter. If a ScalarType has not been set,
// then the ScalarType of the input is used.
void vtkImageTwoInputFilter::SetInput2(vtkImageCache *input)
{
  vtkDebugMacro(<< "SetInput2: input = " << input->GetClassName()
		<< " (" << input << ")");

  // does this change anything?
  if (input == this->Inputs[1])
    {
    return;
    }
  
  this->Inputs[1] = input;
  this->Modified();
}




//----------------------------------------------------------------------------
// Description:
//  Just call the less general execute method.
void vtkImageTwoInputFilter::Execute(vtkImageRegion **inRegions, 
				     vtkImageRegion *outRegion)
{
  this->Execute(inRegions[0], inRegions[1], outRegion);
}


//----------------------------------------------------------------------------
// Description:
// The execute method created by the subclass.
void vtkImageTwoInputFilter::Execute(vtkImageRegion *inRegion1, 
				     vtkImageRegion *inRegion2, 
				     vtkImageRegion *outRegion)
{
  inRegion1 = inRegion2 = outRegion;
  vtkErrorMacro(<< "Subclass needs to supply an execute function.");
}

  







