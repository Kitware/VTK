/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDecomposeFilter.h
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
// .NAME vtkImageDecomposeFilter - Filters that execute axes in series.
// .SECTION Description
// This superclass molds the vtkImageIterateFilter superclass so
// it iterates over the axes.  The filter uses dimensionality to 
// determine how many axes to execute (starting from x).  
// The filter also provides convenience methods for permuting information
// retrieved from input, output and vtkImageData.

#ifndef __vtkImageDecomposeFilter_h
#define __vtkImageDecomposeFilter_h


#include "vtkImageIterateFilter.h"

class VTK_EXPORT vtkImageDecomposeFilter : public vtkImageIterateFilter
{
public:
  // Description:
  // Construct an instance of vtkImageDecomposeFilter filter with default
  // dimensionality 3.
  vtkTypeMacro(vtkImageDecomposeFilter,vtkImageIterateFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Dimensionality is the number of axes which are considered during
  // execution. To process images dimensionality would be set to 2.
  void SetDimensionality(int dim);
  vtkGetMacro(Dimensionality,int);

  // Description:
  // Private methods kept public for template execute functions.
  void PermuteIncrements(int *increments, int &inc0, int &inc1, int &inc2);
  void PermuteExtent(int *extent, int &min0, int &max0, int &min1, int &max1,
		     int &min2, int &max2);
  
protected:
  vtkImageDecomposeFilter();
  ~vtkImageDecomposeFilter() {};
  vtkImageDecomposeFilter(const vtkImageDecomposeFilter&) {};
  void operator=(const vtkImageDecomposeFilter&) {};

  int Dimensionality;


};

#endif










