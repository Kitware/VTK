/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamPoints.h
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
// .NAME vtkStreamPoints - generate points along streamer separated by constant time increment
// .SECTION Description
// vtkStreamPoints is a filter that generates points along a streamer.
// The points are separated by a constant time increment. The resulting visual
// effect (especially when coupled with vtkGlyph3D) is an indication of 
// particle speed.

// .SECTION See Also
// vtkStreamer vtkStreamLine vtkDashedStreamLine

#ifndef __vtkStreamPoints_h
#define __vtkStreamPoints_h

#include "vtkStreamer.h"

class VTK_EXPORT vtkStreamPoints : public vtkStreamer
{
public:
  vtkTypeMacro(vtkStreamPoints,vtkStreamer);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with time increment set to 1.0.
  static vtkStreamPoints *New();

  // Description:
  // Specify the separation of points in terms of absolute time.
  vtkSetClampMacro(TimeIncrement,float,0.000001,VTK_LARGE_FLOAT);
  vtkGetMacro(TimeIncrement,float);

protected:
  vtkStreamPoints();
  ~vtkStreamPoints() {};
  vtkStreamPoints(const vtkStreamPoints&);
  void operator=(const vtkStreamPoints&);

  // Convert streamer array into vtkPolyData
  void Execute();

  // the separation of points
  float TimeIncrement;
  
};

#endif


