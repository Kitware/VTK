/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSampleFunction.h
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
// .NAME vtkSampleFunction - sample an implicit function over a structured point set
// .SECTION Description
// vtkSampleFunction is a source object that evaluates an implicit function
// and normals at each point in a vtkStructuredPoints. The user can specify
// the sample dimensions and location in space to perform the sampling. To
// create closed surfaces (in conjunction with the vtkContourFilter), capping
// can be turned on to set a particular value on the boundaries of the sample
// space.

// .SECTION See Also
// vtkImplicitModeller

#ifndef __vtkSampleFunction_h
#define __vtkSampleFunction_h

#include "vtkStructuredPointsSource.h"
#include "vtkImplicitFunction.h"

class VTK_EXPORT vtkSampleFunction : public vtkStructuredPointsSource
{
public:
  vtkTypeMacro(vtkSampleFunction,vtkStructuredPointsSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with ModelBounds=(-1,1,-1,1,-1,1), SampleDimensions=(50,50,50),
  // Capping turned off, and normal generation on.
  static vtkSampleFunction *New();

  // Description:
  // Specify the implicit function to use to generate data.
  vtkSetObjectMacro(ImplicitFunction,vtkImplicitFunction);
  vtkGetObjectMacro(ImplicitFunction,vtkImplicitFunction);

  // Description:
  // Control the type of the scalars object by explicitly providing a scalar
  // object.  vtkSampleFunction() will allocate space (as necessary)
  // in the scalar object.
  vtkSetObjectMacro(Scalars,vtkDataArray);
  void SetScalars(vtkScalars* scalars)
    {
      this->SetScalars(scalars->GetData());
    }

  // Description:
  // Specify the dimensions of the data on which to sample.
  void SetSampleDimensions(int i, int j, int k);

  // Description:
  // Specify the dimensions of the data on which to sample.
  void SetSampleDimensions(int dim[3]);
  vtkGetVectorMacro(SampleDimensions,int,3);

  // Description:
  // Specify the region in space over which the sampling occurs.
  vtkSetVector6Macro(ModelBounds,float);
  vtkGetVectorMacro(ModelBounds,float,6);

  // Description:
  // Turn on/off capping. If capping is on, then the outer boundaries of the
  // structured point set are set to cap value. This can be used to insure
  // surfaces are closed.
  vtkSetMacro(Capping,int);
  vtkGetMacro(Capping,int);
  vtkBooleanMacro(Capping,int);
  
  // Description:
  // Set the cap value.
  vtkSetMacro(CapValue,float);
  vtkGetMacro(CapValue,float);

  // Description:
  // Turn on/off the computation of normals.
  vtkSetMacro(ComputeNormals,int);
  vtkGetMacro(ComputeNormals,int);
  vtkBooleanMacro(ComputeNormals,int);

  // Description:
  // Return the MTime also considering the implicit function.
  unsigned long GetMTime();

protected:
  vtkSampleFunction();
  ~vtkSampleFunction();
  vtkSampleFunction(const vtkSampleFunction&);
  void operator=(const vtkSampleFunction&);

  void Execute();
  void ExecuteInformation();
  void Cap(vtkDataArray *s);

  int SampleDimensions[3];
  float ModelBounds[6];
  vtkDataArray *Scalars;
  int Capping;
  float CapValue;
  vtkImplicitFunction *ImplicitFunction;
  int ComputeNormals;
};

#endif


