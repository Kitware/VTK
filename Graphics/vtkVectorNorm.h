/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectorNorm.h
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
// .NAME vtkVectorNorm - generate scalars from Euclidean norm of vectors
// .SECTION Description
// vtkVectorNorm is a filter that generates scalar values by computing
// Euclidean norm of vector triplets. Scalars can be normalized 
// 0<=s<=1 if desired.
//
// Note that this filter operates on point or cell attribute data, or
// both.  By default, the filter operates on both point and cell data
// if vector point and cell data, respectively, are available from the
// input. Alternatively, you can choose to generate scalar norm values
// for just cell or point data.

#ifndef __vtkVectorNorm_h
#define __vtkVectorNorm_h

#define VTK_ATTRIBUTE_MODE_DEFAULT 0
#define VTK_ATTRIBUTE_MODE_USE_POINT_DATA 1
#define VTK_ATTRIBUTE_MODE_USE_CELL_DATA 2

#include "vtkDataSetToDataSetFilter.h"

class VTK_GRAPHICS_EXPORT vtkVectorNorm : public vtkDataSetToDataSetFilter 
{
public:
  vtkTypeMacro(vtkVectorNorm,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with normalize flag off.
  static vtkVectorNorm *New();

  // Description:
  // Specify whether to normalize scalar values.
  vtkSetMacro(Normalize,int);
  vtkGetMacro(Normalize,int);
  vtkBooleanMacro(Normalize,int);

  // Description:
  // Control how the filter works to generate scalar data from the
  // input vector data. By default, (AttributeModeToDefault) the
  // filter will generate the scalar norm for point and cell data (if
  // vector data present in the input). Alternatively, you can
  // explicitly set the filter to generate point data
  // (AttributeModeToUsePointData) or cell data
  // (AttributeModeToUseCellData).
  vtkSetMacro(AttributeMode,int);
  vtkGetMacro(AttributeMode,int);
  void SetAttributeModeToDefault() 
    {this->SetAttributeMode(VTK_ATTRIBUTE_MODE_DEFAULT);};
  void SetAttributeModeToUsePointData() 
    {this->SetAttributeMode(VTK_ATTRIBUTE_MODE_USE_POINT_DATA);};
  void SetAttributeModeToUseCellData() 
    {this->SetAttributeMode(VTK_ATTRIBUTE_MODE_USE_CELL_DATA);};
  const char *GetAttributeModeAsString();

protected:
  vtkVectorNorm();
  ~vtkVectorNorm() {};

  void Execute();

  int Normalize;  // normalize 0<=n<=1 if true.
  int AttributeMode; //control whether to use point or cell data, or both
private:
  vtkVectorNorm(const vtkVectorNorm&);  // Not implemented.
  void operator=(const vtkVectorNorm&);  // Not implemented.
};

#endif
