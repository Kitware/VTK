/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRecursiveDividingCubes.h
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
// .NAME vtkRecursiveDividingCubes - create points lying on isosurface (using recursive approach)
// .SECTION Description
// vtkRecursiveDividingCubes is a filter that generates points lying on a 
// surface of constant scalar value (i.e., an isosurface). Dense point 
// clouds (i.e., at screen resolution) will appear as a surface. Less dense 
// clouds can be used as a source to generate streamlines or to generate 
// "transparent" surfaces.
//
// This implementation differs from vtkDividingCubes in that it uses a 
// recursive procedure. In many cases this can result in generating 
// more points than the procedural implementation of vtkDividingCubes. This is
// because the recursive procedure divides voxels by multiples of powers of 
// two. This can over-constrain subdivision. One of the advantages of the 
// recursive technique is that the recursion is terminated earlier, which in
// some cases can be more efficient.

// .SECTION See Also
// vtkDividingCubes vtkContourFilter vtkMarchingCubes

#ifndef __vtkRecursiveDividingCubes_h
#define __vtkRecursiveDividingCubes_h

#include "vtkStructuredPointsToPolyDataFilter.h"

class VTK_GRAPHICS_EXPORT vtkRecursiveDividingCubes : public vtkStructuredPointsToPolyDataFilter
{
public:
  static vtkRecursiveDividingCubes *New();
  vtkTypeMacro(vtkRecursiveDividingCubes,vtkStructuredPointsToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set isosurface value.
  vtkSetMacro(Value,float);
  vtkGetMacro(Value,float);

  // Description:
  // Specify sub-voxel size at which to generate point.
  vtkSetClampMacro(Distance,float,1.0e-06,VTK_LARGE_FLOAT);
  vtkGetMacro(Distance,float);

  // Description:
  // Every "Increment" point is added to the list of points. This parameter, if
  // set to a large value, can be used to limit the number of points while
  // retaining good accuracy.
  vtkSetClampMacro(Increment,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(Increment,int);

protected:
  vtkRecursiveDividingCubes();
  ~vtkRecursiveDividingCubes();

  void Execute();
  void SubDivide(float origin[3], float h[3], float values[8]);

  float Value;
  float Distance;
  int Increment;

  // working variable
  int Count;
  
  // to replace a static
  vtkVoxel *Voxel;
private:
  vtkRecursiveDividingCubes(const vtkRecursiveDividingCubes&);  // Not implemented.
  void operator=(const vtkRecursiveDividingCubes&);  // Not implemented.
};

#endif


