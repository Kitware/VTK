/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpScalar.h
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
// .NAME vtkWarpScalar - deform geometry with scalar data
// .SECTION Description
// vtkWarpScalar is a filter that modifies point coordinates by moving
// points along point normals by the scalar amount times the scale factor.
// Useful for creating carpet or x-y-z plots.
//
// If normals are not present in data, the Normal instance variable will
// be used as the direction along which to warp the geometry. If normals are
// present but you would like to use the Normal instance variable, set the 
// UseNormal boolean to true.
//
// If XYPlane boolean is set true, then the z-value is considered to be 
// a scalar value (still scaled by scale factor), and the displacement is
// along the z-axis. If scalars are also present, these are copied through
// and can be used to color the surface.
//
// Note that the filter passes both its point data and cell data to
// its output, except for normals, since these are distorted by the
// warping.


#ifndef __vtkWarpScalar_h
#define __vtkWarpScalar_h

#include "vtkPointSetToPointSetFilter.h"

class VTK_EXPORT vtkWarpScalar : public vtkPointSetToPointSetFilter
{
public:
  static vtkWarpScalar *New();
  vtkTypeMacro(vtkWarpScalar,vtkPointSetToPointSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify value to scale displacement.
  vtkSetMacro(ScaleFactor,float);
  vtkGetMacro(ScaleFactor,float);

  // Description:
  // Turn on/off use of user specified normal. If on, data normals
  // will be ignored and instance variable Normal will be used instead.
  vtkSetMacro(UseNormal,int);
  vtkGetMacro(UseNormal,int);
  vtkBooleanMacro(UseNormal,int);

  // Description:
  // Normal (i.e., direction) along which to warp geometry. Only used
  // if UseNormal boolean set to true or no normals available in data.
  vtkSetVector3Macro(Normal,float);
  vtkGetVectorMacro(Normal,float,3);

  // Description:
  // Turn on/off flag specifying that input data is x-y plane. If x-y plane,
  // then the z value is used to warp the surface in the z-axis direction 
  // (times the scale factor) and scalars are used to color the surface.
  vtkSetMacro(XYPlane,int);
  vtkGetMacro(XYPlane,int);
  vtkBooleanMacro(XYPlane,int);

protected:
  vtkWarpScalar();
  ~vtkWarpScalar() {};
  vtkWarpScalar(const vtkWarpScalar&);
  void operator=(const vtkWarpScalar&);

  void Execute();

  float ScaleFactor;
  int UseNormal;
  float Normal[3];
  int XYPlane;

  //BTX
  float *(vtkWarpScalar::*PointNormal)(vtkIdType id, vtkDataArray *normals);
  float *DataNormal(vtkIdType id, vtkDataArray *normals=NULL);
  float *InstanceNormal(vtkIdType id, vtkDataArray *normals=NULL);
  float *ZNormal(vtkIdType id, vtkDataArray *normals=NULL);
  //ETX
};

#endif
