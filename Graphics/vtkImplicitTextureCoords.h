/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitTextureCoords.h
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
// .NAME vtkImplicitTextureCoords - generate 1D, 2D, or 3D texture coordinates based on implicit function(s)
// .SECTION Description
// vtkImplicitTextureCoords is a filter to generate 1D, 2D, or 3D texture 
// coordinates from one, two, or three implicit functions, respectively. 
// In combinations with a vtkBooleanTexture map (or another texture map of
// your own creation), the texture coordinates can be used to highlight
//(via color or intensity) or cut (via transparency) dataset geometry without
// any complex geometric processing. (Note: the texture coordinates are 
// referred to as r-s-t coordinates.)
//
// The texture coordinates are automatically normalized to lie between (0,1). 
// Thus, no matter what the implicit functions evaluate to, the resulting 
// texture coordinates lie between (0,1), with the zero implicit function 
// value mapped to the 0.5 texture coordinates value. Depending upon the 
// maximum negative/positive implicit function values, the full (0,1) range 
// may not be occupied (i.e., the positive/negative ranges are mapped using 
// the same scale factor).
//
// A boolean variable InvertTexture is available to flip the texture 
// coordinates around 0.5 (value 1.0 becomes 0.0, 0.25->0.75). This is 
// equivalent to flipping the texture map (but a whole lot easier).

// .SECTION Caveats
// You can use the transformation capabilities of vtkImplicitFunction to
// orient, translate, and scale the implicit functions. Also, the dimension of 
// the texture coordinates is implicitly defined by the number of implicit 
// functions defined.

// .SECTION See Also
// vtkImplicitFunction vtkTexture vtkBooleanTexture vtkTransformTexture

#ifndef __vtkImplicitTextureCoords_h
#define __vtkImplicitTextureCoords_h

#include "vtkDataSetToDataSetFilter.h"
#include "vtkImplicitFunction.h"

class VTK_GRAPHICS_EXPORT vtkImplicitTextureCoords : public vtkDataSetToDataSetFilter 
{
public:
  vtkTypeMacro(vtkImplicitTextureCoords,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create object with texture dimension=2 and no r-s-t implicit functions
  // defined and FlipTexture turned off.
  static vtkImplicitTextureCoords *New();
  
  // Description:
  // Specify an implicit function to compute the r texture coordinate.
  vtkSetObjectMacro(RFunction,vtkImplicitFunction);
  vtkGetObjectMacro(RFunction,vtkImplicitFunction);

  // Description:
  // Specify an implicit function to compute the s texture coordinate.
  vtkSetObjectMacro(SFunction,vtkImplicitFunction);
  vtkGetObjectMacro(SFunction,vtkImplicitFunction);

  // Description:
  // Specify an implicit function to compute the t texture coordinate.
  vtkSetObjectMacro(TFunction,vtkImplicitFunction);
  vtkGetObjectMacro(TFunction,vtkImplicitFunction);

  // Description:
  // If enabled, this will flip the sense of inside and outside the implicit
  // function (i.e., a rotation around the r-s-t=0.5 axis).
  vtkSetMacro(FlipTexture,int);
  vtkGetMacro(FlipTexture,int);
  vtkBooleanMacro(FlipTexture,int);  
  
protected:
  vtkImplicitTextureCoords();
  ~vtkImplicitTextureCoords();

  void Execute();

  vtkImplicitFunction *RFunction;
  vtkImplicitFunction *SFunction;
  vtkImplicitFunction *TFunction;
  int FlipTexture;
private:
  vtkImplicitTextureCoords(const vtkImplicitTextureCoords&);  // Not implemented.
  void operator=(const vtkImplicitTextureCoords&);  // Not implemented.
};

#endif


