/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProjectedPolyDataRayBounder.h
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
// .NAME vtkProjectedPolyDataRayBounder - Bound a ray according to polydata
// .SECTION Description
// The vtkProjectedPolyDataRayBounder can be used to clip viewing rays
// against the polygons in a vtkPolyData. This is done by projecting the
// vtkPolyData twice - first capturing a near Z buffer, then capturing 
// a far Z buffer. The values from the Z buffers are decoded according to
// the current viewing transformation, and the decoded pairs of values
// (near,far) are returned as distance from the view point for perspective
// viewing, or distance from the view plane for parallel viewing.
// 

// .SECTION see also
// vtkOpenGLProjectedPolyDataRayBounder

#ifndef __vtkProjectedPolyDataRayBounder_h
#define __vtkProjectedPolyDataRayBounder_h

#include "vtkObject.h"
#include "vtkRenderer.h"
#include "vtkPolyData.h"
#include "vtkVolume.h"
#include "vtkActor.h"
#include "vtkRayBounder.h"

class VTK_EXPORT vtkProjectedPolyDataRayBounder : public vtkRayBounder
{
public:
  vtkTypeMacro(vtkProjectedPolyDataRayBounder,vtkRayBounder);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // New method for the class which will return the correct type of 
  // ProjectPolyDataRayBounder
  static vtkProjectedPolyDataRayBounder *New();

  // Description:
  // Get the ray bounds given a renderer. The ray bounds are a two 
  // dimensional array of (near,far) values, with the width and height of
  // the array being equal to the width and height of the current viewport
  // in pixel.
  float *GetRayBounds( vtkRenderer *ren );

  // Description:
  // Set/Get the PolyData that will be projected for clipping
  vtkSetObjectMacro( PolyData, vtkPolyData );
  vtkGetObjectMacro( PolyData, vtkPolyData );
  
  // Description:
  // Set a matrix source as either an actor or a volume. If a matrix
  // source is set, then the PolyData will first be transformed according
  // to the matrix of the given actor or volume.
  void SetMatrixSource( vtkActor *actor );
  void SetMatrixSource( vtkVolume *volume );

  // Description:
  // Return the MTime also considering the ivars' MTimes.
  unsigned long GetMTime();

protected:
  vtkProjectedPolyDataRayBounder();
  ~vtkProjectedPolyDataRayBounder();
  vtkProjectedPolyDataRayBounder(const vtkProjectedPolyDataRayBounder&);
  void operator=(const vtkProjectedPolyDataRayBounder&);

  vtkPolyData   *PolyData;
  vtkActor      *ActorMatrixSource;
  vtkVolume     *VolumeMatrixSource;

  vtkTimeStamp  BuildTime;

  // Description:
  // Create a display list from the poly data.
  virtual void Build( vtkPolyData *pdata );

  // Description:
  // Render the display list and create the near and far buffers
  virtual float *Draw( vtkRenderer *ren, vtkMatrix4x4 *matrix );

};

#endif

