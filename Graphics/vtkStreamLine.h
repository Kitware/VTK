/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamLine.h
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
// .NAME vtkStreamLine - generate streamline in arbitrary dataset
// .SECTION Description
// vtkStreamLine is a filter that generates a streamline for an arbitrary 
// dataset. A streamline is a line that is everywhere tangent to the vector
// field. Scalar values also are calculated along the streamline and can be 
// used to color the line. Streamlines are calculated by integrating from
// a starting point through the vector field. Integration can be performed
// forward in time (see where the line goes), backward in time (see where the
// line came from), or in both directions. It also is possible to compute
// vorticity along the streamline. Vorticity is the projection (i.e., dot
// product) of the flow rotation on the velocity vector, i.e., the rotation
// of flow around the streamline.
//
// vtkStreamLine defines the instance variable StepLength. This parameter 
// controls the time increment used to generate individual points along
// the streamline(s). Smaller values result in more line 
// primitives but smoother streamlines. The StepLength instance variable is 
// defined in terms of time (i.e., the distance that the particle travels in
// the specified time period). Thus, the line segments will be smaller in areas
// of low velocity and larger in regions of high velocity. (NOTE: This is
// different than the IntegrationStepLength defined by the superclass
// vtkStreamer. IntegrationStepLength is used to control integration step 
// size and is expressed as a fraction of the cell length.) The StepLength
// instance variable is important because subclasses of vtkStreamLine (e.g.,
// vtkDashedStreamLine) depend on this value to build their representation.

// .SECTION See Also
// vtkStreamer vtkDashedStreamLine vtkStreamPoints

#ifndef __vtkStreamLine_h
#define __vtkStreamLine_h

#include "vtkStreamer.h"

class VTK_GRAPHICS_EXPORT vtkStreamLine : public vtkStreamer
{
public:
  vtkTypeMacro(vtkStreamLine,vtkStreamer);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with step size set to 1.0.
  static vtkStreamLine *New();

  // Description:
  // Specify the length of a line segment. The length is expressed in terms of
  // elapsed time. Smaller values result in smoother appearing streamlines, but
  // greater numbers of line primitives.
  vtkSetClampMacro(StepLength,float,0.000001,VTK_LARGE_FLOAT);
  vtkGetMacro(StepLength,float);

protected:
  vtkStreamLine();
  ~vtkStreamLine() {};

  // Convert streamer array into vtkPolyData
  void Execute();

  // the length of line primitives
  float StepLength;

private:
  vtkStreamLine(const vtkStreamLine&);  // Not implemented.
  void operator=(const vtkStreamLine&);  // Not implemented.
};

#endif


