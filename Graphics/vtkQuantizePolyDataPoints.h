/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuantizePolyDataPoints.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to John Biddiscombe of the Rutherford Appleton Laboratory
             who developed and contributed this class.


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
// .NAME vtkQuantizePolyDataPoints - quantizes x,y,z coordinates of points
// .SECTION Description
// vtkQuantizePolyDataPoints is a subclass of vtkCleanPolyData and
// inherits the functionality of vtkCleanPolyData with the addition that
// it quantizes the point coordinates before inserting into the point list.
// The user should set QFactor to a positive value (0.25 by default) and all
// {x,y,z} coordinates will be quantized to that grain size.
//
// A tolerance of zero is expected, though positive values may be used, the
// quantization will take place before the tolerance is applied.
//
// .SECTION Caveats
// Merging points can alter topology, including introducing non-manifold
// forms. Handling of degenerate cells is controlled by switches in
// vtkCleanPolyData.
//
// .SECTION See Also
// vtkCleanPolyData
#ifndef __vtkQuantizePolyDataPoints_h
#define __vtkQuantizePolyDataPoints_h

#include "vtkCleanPolyData.h"

class VTK_GRAPHICS_EXPORT vtkQuantizePolyDataPoints : public vtkCleanPolyData
{
public:
  static vtkQuantizePolyDataPoints *New();
  vtkTypeMacro(vtkQuantizePolyDataPoints,vtkCleanPolyData);
  void PrintSelf(ostream& os, vtkIndent indent);
  // Description:
  // Specify quantization grain size
  vtkSetClampMacro(QFactor,float,1E-5,VTK_LARGE_FLOAT);
  vtkGetMacro(QFactor,float);
  // Description:
  // Perform quantization on a point
  virtual void OperateOnPoint(float in[3], float out[3]);
  // Description:
  // Perform quantization on bounds
  virtual void OperateOnBounds(float in[6], float out[6]);
protected:
  vtkQuantizePolyDataPoints();
  ~vtkQuantizePolyDataPoints() {};
  vtkQuantizePolyDataPoints(const vtkQuantizePolyDataPoints&);
  void operator=(const vtkQuantizePolyDataPoints&);
  //
  float QFactor;
};

#endif


