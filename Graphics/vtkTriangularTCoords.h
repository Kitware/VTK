/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTriangularTCoords.h
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
// .NAME vtkTriangularTCoords - 2D texture coordinates based for triangles.
// .SECTION Description
// vtkTriangularTCoords is a filter that generates texture coordinates
// for triangles. Texture coordinates for each triangle are:
// (0,0), (1,0) and (.5,sqrt(3)/2). This filter assumes that the triangle
// texture map is symmetric about the center of the triangle. Thus the order
// Of the texture coordinates is not important. The procedural texture
// in vtkTriangularTexture is designed with this symmetry. For more information
// see the paper "Opacity-modulating Triangular Textures for Irregular 
// Surfaces,"  by Penny Rheingans, IEEE Visualization '96, pp. 219-225.
// .SECTION See Also
// vtkTriangularTexture vtkThresholdPoints vtkTextureMapToPlane 
// vtkTextureMapToSphere vtkTextureMapToCylinder vtkTextureMapToBox

#ifndef __vtkTriangularTCoords_h
#define __vtkTriangularTCoords_h

#include "vtkPolyDataToPolyDataFilter.h"

class VTK_GRAPHICS_EXPORT vtkTriangularTCoords : public vtkPolyDataToPolyDataFilter
{
public:
  static vtkTriangularTCoords *New();
  vtkTypeMacro(vtkTriangularTCoords,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkTriangularTCoords() {};
  ~vtkTriangularTCoords() {};
  vtkTriangularTCoords(const vtkTriangularTCoords&);
  void operator=(const vtkTriangularTCoords&);

  // Usual data generation method
  void Execute();
};

#endif


