/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProjectedPolyDataRayBounder.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Lisa Sobierajski Avila who developed this class.

Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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
  vtkProjectedPolyDataRayBounder();
  ~vtkProjectedPolyDataRayBounder();
  static vtkProjectedPolyDataRayBounder *New();
  const char *GetClassName() {return "vtkProjectedPolyDataRayBounder";};
  void PrintSelf(ostream& os, vtkIndent indent);

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
  
protected:

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

