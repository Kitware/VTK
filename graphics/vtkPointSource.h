/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSource.h
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
// .NAME vtkPointSource - create a random cloud of points
// .SECTION Description
// vtkPointSource is a source object that creates a user-specified number 
// of points within a specified radius about a specified center point. 
// By default location of the points is random within the sphere. 

#ifndef __vtkPointSource_h
#define __vtkPointSource_h

#include "vtkPolyDataSource.h"

#define VTK_POINT_UNIFORM   1
#define VTK_POINT_SHELL     0

class VTK_EXPORT vtkPointSource : public vtkPolyDataSource 
{
public:
  static vtkPointSource *New();
  vtkTypeMacro(vtkPointSource,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set the number of points to generate.
  vtkSetClampMacro(NumberOfPoints,vtkIdType,1,VTK_LARGE_ID);
  vtkGetMacro(NumberOfPoints,vtkIdType);

  // Description:
  // Set the center of the point cloud.
  vtkSetVector3Macro(Center,float);
  vtkGetVectorMacro(Center,float,3);

  // Description:
  // Set the radius of the point cloud.  If you are
  // generating a Gaussian distribution, then this is
  // the standard deviation for each of x, y, and z.
  vtkSetClampMacro(Radius,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(Radius,float);

  // Description:
  // Specify the distribution to use.  The default is a
  // uniform distribution.  The Shell distribution produces
  // and empty sphere.
  vtkSetMacro(Distribution,int);
  void SetDistributionToUniform() {
    this->SetDistribution(VTK_POINT_UNIFORM);};
  void SetDistributionToShell() {
    this->SetDistribution(VTK_POINT_SHELL);};
  vtkGetMacro(Distribution,int);

protected:
  vtkPointSource(vtkIdType numPts=10);
  ~vtkPointSource() {};
  vtkPointSource(const vtkPointSource&) {};
  void operator=(const vtkPointSource&) {};

  void Execute();
  void ExecuteInformation();

  vtkIdType NumberOfPoints;
  float Center[3];
  float Radius;
  int Distribution;
};

#endif
