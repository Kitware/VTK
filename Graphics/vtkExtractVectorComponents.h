/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractVectorComponents.h
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
// .NAME vtkExtractVectorComponents - extract components of vector as separate scalars
// .SECTION Description
// vtkExtractVectorComponents is a filter that extracts vector components as
// separate scalars. This is accomplished by creating three different outputs.
// Each output is the same as the input, except that the scalar values will be
// one of the three components of the vector. These can be found in the
// VxComponent, VyComponent, and VzComponent.
// Alternatively, if the ExtractToFieldData flag is set, the filter will
// put all the components in the field data. The first component will be
// the scalar and the others will be non-attribute arrays.

// .SECTION Caveats
// This filter is unusual in that it creates multiple outputs. 
// If you use the GetOutput() method, you will be retrieving the x vector 
// component.

#ifndef __vtkExtractVectorComponents_h
#define __vtkExtractVectorComponents_h

#include "vtkSource.h"
#include "vtkDataSet.h"

class VTK_EXPORT vtkExtractVectorComponents : public vtkSource
{
public:
  static vtkExtractVectorComponents *New();
  vtkTypeMacro(vtkExtractVectorComponents,vtkSource);

  // Description:
  // Specify the input data or filter.
  virtual void SetInput(vtkDataSet *input);

  // Description:
  // Get the input data or filter.
  vtkDataSet *GetInput();

  // Description:
  // Get the output dataset representing velocity x-component. If output is
  // NULL then input hasn't been set, which is necessary for abstract
  // objects. (Note: this method returns the same information as the
  // GetOutput() method with an index of 0.)
  vtkDataSet *GetVxComponent();

  // Description:
  // Get the output dataset representing velocity y-component. If output is
  // NULL then input hasn't been set, which is necessary for abstract
  // objects. (Note: this method returns the same information as the
  // GetOutput() method with an index of 1.)
  // Note that if ExtractToFieldData is true, this output will be empty.
  vtkDataSet *GetVyComponent();
  
  // Description:
  // Get the output dataset representing velocity z-component. If output is
  // NULL then input hasn't been set, which is necessary for abstract
  // objects. (Note: this method returns the same information as the
  // GetOutput() method with an index of 2.)
  // Note that if ExtractToFieldData is true, this output will be empty.
  vtkDataSet *GetVzComponent();

  // Description:
  // Get the output dataset containing the indicated component. The component
  // is specified by an index between (0,2) corresponding to the x, y, or z
  // vector component. By default, the x component is extracted.
  vtkDataSet *GetOutput(int i=0); //default extracts vector component.

  // Description:
  // Determines whether the vector components will be put
  // in separate outputs or in the first output's field data
  vtkSetMacro(ExtractToFieldData, int);
  vtkGetMacro(ExtractToFieldData, int);
  vtkBooleanMacro(ExtractToFieldData, int);

protected:
  vtkExtractVectorComponents();
  ~vtkExtractVectorComponents();
  vtkExtractVectorComponents(const vtkExtractVectorComponents&) {};
  void operator=(const vtkExtractVectorComponents&) {};

  void Execute();
  int ExtractToFieldData;
};

#endif


