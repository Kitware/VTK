/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLProjectedPolyDataRayBounder.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Lisa Sobierajski Avila who developed this class.

Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkOpenGLProjectedPolyDataRayBounder - Open GL ray bounder
//
// .SECTION Description
// vtkOpenGLProjectedPolyDataRayBounder is the specific Open GL 
// implementation of the superclass vtkProjectedPolyDataRayBounder.
// It is responsible for building its own internal structure from the 
// generic vtkPolyData structure (it builds a display list) and for rendering
// its internal structure and creating near and far depth buffers.
// It has no public methods, and should not be created directly - the
// New() method of vtkProjectedPolyDataRayBounder will automatically
// create the correct subclass given the current VTK_RENDERER

// .SECTION see also
// vtkProjectedPolyDataRayBounder

#ifndef __vtkOpenGLProjectedPolyDataRayBounder_h
#define __vtkOpenGLProjectedPolyDataRayBounder_h

#include "vtkProjectedPolyDataRayBounder.h"
#include "vtkPolyData.h"

#include <GL/gl.h>

class VTK_EXPORT vtkOpenGLProjectedPolyDataRayBounder : public vtkProjectedPolyDataRayBounder
{
 public:
  vtkOpenGLProjectedPolyDataRayBounder();
  ~vtkOpenGLProjectedPolyDataRayBounder();
  static vtkOpenGLProjectedPolyDataRayBounder *New() {return new vtkOpenGLProjectedPolyDataRayBounder;};
  const char *GetClassName() {return "vtkOpenGLProjectedPolyDataRayBounder";};
  void PrintSelf(ostream& os, vtkIndent indent);

 protected:
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
