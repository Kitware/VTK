/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCleanPolyData.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkCleanPolyData - merge duplicate points and remove degenerate primitives
// .SECTION Description
// vtkCleanPolyData is a filter that takes polygonal data as input and 
// generates polygonal as output. vtkCleanPolyData merges duplicate 
// points (within specified tolerance), and transforms degenerate 
// topology into appropriate form (for example, triangle is converted
// into line if two points of triangle are merged).
//
// If tolerance is specified precisely=0.0, then this object will use
// the vtkMergePoints object to merge points (very fast). Otherwise the 
// slower vtkPointLocator is used.

// .SECTION Caveats
// Merging points can alter topology, including introducing non-manifold 
// forms. Tolerance should be chosen carefully to avoid these problems.

#ifndef __vtkCleanPolyData_h
#define __vtkCleanPolyData_h

#include "vtkPolyDataToPolyDataFilter.h"

class VTK_EXPORT vtkCleanPolyData : public vtkPolyDataToPolyDataFilter
{
public:
  static vtkCleanPolyData *New();
  vtkTypeMacro(vtkCleanPolyData,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify tolerance in terms of fraction of bounding box length.
  vtkSetClampMacro(Tolerance,float,0.0,1.0);
  vtkGetMacro(Tolerance,float);

  // Description:
  // Set/Get a spatial locator for speeding the search process. By
  // default an instance of vtkLocator is used.
  void SetLocator(vtkPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkPointLocator);

  // Description:
  // Create default locator. Used to create one when none is specified.
  void CreateDefaultLocator();

  // Description:
  // Get the MTime of this object also considering the locator.
  unsigned long int GetMTime();

  // Description:
  // For legacy compatibility. Do not use.
  void SetLocator(vtkPointLocator& locator) {this->SetLocator(&locator);};  

protected:
  vtkCleanPolyData();
  ~vtkCleanPolyData();
  vtkCleanPolyData(const vtkCleanPolyData&) {};
  void operator=(const vtkCleanPolyData&) {};

  // Usual data generation method
  void Execute();

  float Tolerance;
  vtkPointLocator *Locator;
};

#endif


