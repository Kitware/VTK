/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointLoad.h
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
// .NAME vtkPointLoad - compute stress tensors given point load on semi-infinite domain
// .SECTION Description
// vtkPointLoad is a source object that computes stress tensors on a volume. 
// The tensors are computed from the application of a point load on a 
// semi-infinite domain. (The analytical results are adapted from Saada - see 
// text.) It also is possible to compute effective stress scalars if desired.
// This object serves as a specialized data generator for some of the examples
// in the text.

// .SECTION See Also
// vtkTensorGlyph, vtkHyperStreamline

#ifndef __vtkPointLoad_h
#define __vtkPointLoad_h

#include "vtkStructuredPointsSource.h"

class VTK_EXPORT vtkPointLoad :  public vtkStructuredPointsSource
{
public:
  vtkTypeMacro(vtkPointLoad,vtkStructuredPointsSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with ModelBounds=(-1,1,-1,1,-1,1), SampleDimensions=(50,50,50),
  // and LoadValue = 1.
  static vtkPointLoad *New();

  // Description:
  // Set/Get value of applied load.
  vtkSetMacro(LoadValue,float);
  vtkGetMacro(LoadValue,float);

  // Description:
  // Specify the dimensions of the volume. A stress tensor will be computed for
  // each point in the volume.
  void SetSampleDimensions(int i, int j, int k);

  // Description:
  // Specify the dimensions of the volume. A stress tensor will be computed for
  // each point in the volume.
  void SetSampleDimensions(int dim[3]);
  vtkGetVectorMacro(SampleDimensions,int,3);

  // Description:
  // Specify the region in space over which the tensors are computed. The point
  // load is assumed to be applied at top center of the volume.
  vtkSetVector6Macro(ModelBounds,float);
  vtkGetVectorMacro(ModelBounds,float,6);

  // Description:
  // Set/Get Poisson's ratio.
  vtkSetMacro(PoissonsRatio,float);
  vtkGetMacro(PoissonsRatio,float);

  // Description:
  // Turn on/off computation of effective stress scalar.
  vtkSetMacro(ComputeEffectiveStress,int);
  vtkGetMacro(ComputeEffectiveStress,int);
  vtkBooleanMacro(ComputeEffectiveStress,int);

protected:
  vtkPointLoad();
  ~vtkPointLoad() {};
  vtkPointLoad(const vtkPointLoad&);
  void operator=(const vtkPointLoad&);

  void Execute();

  float LoadValue;
  float PoissonsRatio;
  int SampleDimensions[3];
  float ModelBounds[6];
  int ComputeEffectiveStress;

};

#endif


