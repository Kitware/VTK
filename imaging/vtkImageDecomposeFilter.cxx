/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDecomposeFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <math.h>

#include "vtkImageDecomposeFilter.h"
#include "vtkObjectFactory.h"


//----------------------------------------------------------------------------
// Construct an instance of vtkImageDecomposeFilter fitler.
vtkImageDecomposeFilter::vtkImageDecomposeFilter()
{
  this->Dimensionality = 3;
  this->SetNumberOfIterations(3);
}


//----------------------------------------------------------------------------
void vtkImageDecomposeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageIterateFilter::PrintSelf(os,indent);

  os << indent << "Dimensionality: " << this->Dimensionality << "\n";
}

//----------------------------------------------------------------------------
void vtkImageDecomposeFilter::SetDimensionality(int dim)
{  
  if (this->Dimensionality == dim)
    {
    return;
    }
  
  if (dim < 1 || dim > 3)
    {
    vtkErrorMacro("SetDimensionality: Bad dim: " << dim);
    return;
    }
  
  this->Dimensionality = dim;
  this->SetNumberOfIterations(dim);
  this->Modified();
}



//----------------------------------------------------------------------------
void vtkImageDecomposeFilter::PermuteIncrements(int *increments, int &inc0, 
						int &inc1, int &inc2)
{
  switch (this->Iteration)
    {
    case 0:
      inc0 = increments[0];
      inc1 = increments[1];
      inc2 = increments[2];
      break;
    case 1:
      inc1 = increments[0];
      inc0 = increments[1];
      inc2 = increments[2];
      break;
    case 2:
      inc1 = increments[0];
      inc2 = increments[1];
      inc0 = increments[2];
      break;
    }
}


//----------------------------------------------------------------------------
void vtkImageDecomposeFilter::PermuteExtent(int *extent, int &min0, int &max0, 
					    int &min1, int &max1,
					    int &min2, int &max2)
{
  switch (this->Iteration)
    {
    case 0:
      min0 = extent[0];       max0 = extent[1];
      min1 = extent[2];       max1 = extent[3];
      min2 = extent[4];       max2 = extent[5];
      break;
    case 1:
      min1 = extent[0];       max1 = extent[1];
      min0 = extent[2];       max0 = extent[3];
      min2 = extent[4];       max2 = extent[5];
      break;
    case 2:
      min1 = extent[0];       max1 = extent[1];
      min2 = extent[2];       max2 = extent[3];
      min0 = extent[4];       max0 = extent[5];
      break;
    }
}


















