/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGLUTesselatorTriangleFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Tom Citriniti who implemented this class


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
// .NAME vtkGLUTesselatorTriangleFilter - create triangle polygons from input polygons and triangle strips
// .SECTION Description
// vtkGLUTesselatorTriangleFilter generates triangles from input polygons and
// triangle strips. The filter also will pass through vertices and lines, if
// requested. 
//
// This filter is a specialized version of vtkTriangleFilter. It uses
// glu code to perform the tessellation. To use this filter, you will
// have to link against a GLU library.
//
// .SECTION See Also
// vtkTriangleFilter

#ifndef __vtkGLUTesselatorTriangleFilter_h
#define __vtkGLUTesselatorTriangleFilter_h

#include "vtkPolyDataToPolyDataFilter.h"
#include <GL/GLU.h>

class VTK_EXPORT vtkGLUTesselatorTriangleFilter : public vtkPolyDataToPolyDataFilter
{
public:
  static vtkGLUTesselatorTriangleFilter *New();
  vtkTypeMacro(vtkGLUTesselatorTriangleFilter,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Turn on/off passing vertices through filter.
  vtkBooleanMacro(PassVerts,int);
  vtkSetMacro(PassVerts,int);
  vtkGetMacro(PassVerts,int);

  // Description:
  // Turn on/off passing lines through filter.
  vtkBooleanMacro(PassLines,int);
  vtkSetMacro(PassLines,int);
  vtkGetMacro(PassLines,int);

protected:
  vtkGLUTesselatorTriangleFilter();
  ~vtkGLUTesselatorTriangleFilter();
  vtkGLUTesselatorTriangleFilter(const vtkGLUTesselatorTriangleFilter&);
  void operator=(const vtkGLUTesselatorTriangleFilter&);

  // Usual data generation method
  void Execute();

        // Data
  int PassVerts;
  int PassLines;
  GLUtesselator *GLUTesselator;

};

#endif


