/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractGeometry.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
// .NAME vtkExtractGeometry - extract cells that lie either entirely inside or outside of a specified implicit function

// .SECTION Description
// vtkExtractGeometry extracts from its input dataset all cells that are either
// completely inside or outside of a specified implicit function. Any type of
// dataset can be input to this filter. On output the filter generates an
// unstructured grid.
//
// To use this filter you must specify an implicit function. You must also
// specify whethter to extract cells lying inside or outside of the implicit 
// function. (The inside of an implicit function is the negative values 
// region.) An option exists to extract cells that are neither inside or
// outside (i.e., boundary).
//
// A more efficient version of this filter is available for vtkPolyData input.
// See vtkExtractPolyDataGeometry.

// .SECTION See Also
// vtkExtractPolyDataGeometry vtkGeometryFilter vtkExtractVOI 

#ifndef __vtkExtractGeometry_h
#define __vtkExtractGeometry_h

#include "vtkDataSetToUnstructuredGridFilter.h"
#include "vtkImplicitFunction.h"

class VTK_EXPORT vtkExtractGeometry : public vtkDataSetToUnstructuredGridFilter
{
public:
  vtkTypeMacro(vtkExtractGeometry,vtkDataSetToUnstructuredGridFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with ExtractInside turned on.
  static vtkExtractGeometry *New();

  // Description:
  // Return the MTime taking into account changes to the implicit function
  unsigned long GetMTime();

  // Description:
  // Specify the implicit function for inside/outside checks.
  vtkSetObjectMacro(ImplicitFunction,vtkImplicitFunction);
  vtkGetObjectMacro(ImplicitFunction,vtkImplicitFunction);

  // Description:
  // Boolean controls whether to extract cells that are inside of implicit 
  // function (ExtractInside == 1) or outside of implicit function 
  // (ExtractInside == 0).
  vtkSetMacro(ExtractInside,int);
  vtkGetMacro(ExtractInside,int);
  vtkBooleanMacro(ExtractInside,int);

  // Description:
  // Boolean controls whether to extract cells that are partially inside.
  // By default, ExtractBoundaryCells is off.
  vtkSetMacro(ExtractBoundaryCells,int);
  vtkGetMacro(ExtractBoundaryCells,int);
  vtkBooleanMacro(ExtractBoundaryCells,int);

protected:
  vtkExtractGeometry(vtkImplicitFunction *f=NULL);
  ~vtkExtractGeometry();
  vtkExtractGeometry(const vtkExtractGeometry&) {};
  void operator=(const vtkExtractGeometry&) {};

  // Usual data generation method
  void Execute();

  vtkImplicitFunction *ImplicitFunction;
  int ExtractInside;
  int ExtractBoundaryCells;
  
};

#endif


