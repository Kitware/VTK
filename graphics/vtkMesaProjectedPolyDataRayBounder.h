/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaProjectedPolyDataRayBounder.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Lisa Sobierajski Avila who developed this class.

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
// .NAME vtkMesaProjectedPolyDataRayBounder - Open GL ray bounder
//
// .SECTION Description
// vtkMesaProjectedPolyDataRayBounder is the specific Open GL 
// implementation of the superclass vtkProjectedPolyDataRayBounder.
// It is responsible for building its own internal structure from the 
// generic vtkPolyData structure (it builds a display list) and for rendering
// its internal structure and creating near and far depth buffers.
// It has no public methods, and should not be created directly - the
// New();
// create the correct subclass given the current VTK_RENDERER

// .SECTION see also
// vtkProjectedPolyDataRayBounder

#ifndef __vtkMesaProjectedPolyDataRayBounder_h
#define __vtkMesaProjectedPolyDataRayBounder_h

#include "vtkToolkits.h"
#include "vtkProjectedPolyDataRayBounder.h"
#include "vtkPolyData.h"

#ifdef VTK_MANGLE_MESA
#define USE_MGL_NAMESPACE
#include "mesagl.h"
#else
#include "GL/gl.h"
#endif

class vtkWindow;

class VTK_EXPORT vtkMesaProjectedPolyDataRayBounder : public vtkProjectedPolyDataRayBounder
{
public:
  vtkTypeMacro(vtkMesaProjectedPolyDataRayBounder,vtkProjectedPolyDataRayBounder);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct a new vtkMesaProjectedPolyDataRayBounder.  The depth range
  // buffer is initially NULL and no display list has been created
  static vtkMesaProjectedPolyDataRayBounder *New();

  // Description:
  // Release any graphics resources that are being consumed by this ray bounder.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);


protected:
  vtkMesaProjectedPolyDataRayBounder();
  ~vtkMesaProjectedPolyDataRayBounder();
  void operator=(const vtkMesaProjectedPolyDataRayBounder&) {};

  GLuint    DisplayList;
  float     *DepthRangeBuffer;

  // Description:
  // Create a display list from the poly data.
  void Build( vtkPolyData *pdata );

  // Description:
  // Render the display list and create the near and far buffers
  float *Draw( vtkRenderer *ren, vtkMatrix4x4 *matrix );

};

#endif
