/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTriangularTexture.h
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
// .NAME vtkTriangularTexture - generate 2D triangular texture map
// .SECTION Description
// vtkTriangularTexture is a filter that generates a 2D texture map based on 
// the paper "Opacity-modulating Triangular Textures for Irregular Surfaces,"
// by Penny Rheingans, IEEE Visualization '96, pp. 219-225.
// The textures assume texture coordinates of (0,0), (1.0) and
// (.5, sqrt(3)/2). The sequence of texture values is the same along each
// edge of the triangular texture map. So, the assignment order of texture
// coordinates is arbitrary.

// .SECTION See Also
// vtkTriangularTCoords

#ifndef __vtkTriangularTexture_h
#define __vtkTriangularTexture_h

#include "vtkStructuredPointsSource.h"

class VTK_EXPORT vtkTriangularTexture : public vtkStructuredPointsSource
{
public:
  vtkTypeMacro(vtkTriangularTexture,vtkStructuredPointsSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate object with XSize and YSize = 64; the texture pattern =1
  // (opaque at centroid); and the scale factor set to 1.0.
  static vtkTriangularTexture *New();

  // Description:
  // Set a Scale Factor.
  vtkSetMacro(ScaleFactor,float);
  vtkGetMacro(ScaleFactor,float);

  // Description:
  // Set the X texture map dimension. Default is 64.
  vtkSetMacro(XSize,int);
  vtkGetMacro(XSize,int);

  // Description:
  // Set the Y texture map dimension. Default is 64.
  vtkSetMacro(YSize,int);
  vtkGetMacro(YSize,int);

  // Description:
  // Set the texture pattern.
  //    1 = opaque at centroid (default)
  //    2 = opaque at vertices
  //    3 = opaque in rings around vertices
  vtkSetClampMacro(TexturePattern,int,1,3);
  vtkGetMacro(TexturePattern,int);

protected:
  vtkTriangularTexture();
  ~vtkTriangularTexture() {};
  vtkTriangularTexture(const vtkTriangularTexture&);
  void operator=(const vtkTriangularTexture&);

  void Execute();

  int XSize;
  int YSize;
  float ScaleFactor;

  int TexturePattern;
};

#endif


