/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGlyph2D.h
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
// .NAME vtkGlyph2D - copy oriented and scaled glyph geometry to every input point (2D specialization)
// .SECTION Description
// This subclass of vtkGlyph3D is a specialization to 2D. Transformations
// (i.e., translation, scaling, and rotation) are constrained to the plane.
// For example, rotations due to a vector are computed from the x-y
// coordinates of the vector only, and are assumed to occur around the
// z-axis. (See vtkGlyph3D for documentation on the interface to this
// class.)
// 
// .SECTION See Also
// vtkTensorGlyph vtkGlyph3D vtkProgrammableGlyphFilter

#ifndef __vtkGlyph2D_h
#define __vtkGlyph2D_h

#include "vtkGlyph3D.h"

class VTK_GRAPHICS_EXPORT vtkGlyph2D : public vtkGlyph3D
{
public:
  vtkTypeMacro(vtkGlyph2D,vtkGlyph3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Construct object with scaling on, scaling mode is by scalar value, 
  // scale factor = 1.0, the range is (0,1), orient geometry is on, and
  // orientation is by vector. Clamping and indexing are turned off. No
  // initial sources are defined.
  static vtkGlyph2D *New();

protected:
  vtkGlyph2D() {};
  ~vtkGlyph2D() {};

  void Execute();
private:
  vtkGlyph2D(const vtkGlyph2D&);  // Not implemented.
  void operator=(const vtkGlyph2D&);  // Not implemented.
};

#endif
