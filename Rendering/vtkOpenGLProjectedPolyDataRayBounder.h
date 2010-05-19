/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLProjectedPolyDataRayBounder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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
// New();
// create the correct subclass given the current VTK_RENDERER

// .SECTION see also
// vtkProjectedPolyDataRayBounder

#ifndef __vtkOpenGLProjectedPolyDataRayBounder_h
#define __vtkOpenGLProjectedPolyDataRayBounder_h

#include "vtkProjectedPolyDataRayBounder.h"
#ifndef VTK_IMPLEMENT_MESA_CXX
  #ifdef __APPLE__
    #include <OpenGL/gl.h> //Needed for GLUint
  #else
    #include <GL/gl.h> //Needed for GLUint
  #endif
#endif

class vtkWindow;

class VTK_RENDERING_EXPORT vtkOpenGLProjectedPolyDataRayBounder : public vtkProjectedPolyDataRayBounder
{
public:
  vtkTypeMacro(vtkOpenGLProjectedPolyDataRayBounder,vtkProjectedPolyDataRayBounder);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct a new vtkOpenGLProjectedPolyDataRayBounder.  The depth range
  // buffer is initially NULL and no display list has been created
  static vtkOpenGLProjectedPolyDataRayBounder *New();

  // Description:
  // Release any graphics resources that are being consumed by this ray bounder.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);


protected:
  vtkOpenGLProjectedPolyDataRayBounder();
  ~vtkOpenGLProjectedPolyDataRayBounder();

  GLuint    DisplayList;
  float     *DepthRangeBuffer;

  // Description:
  // Create a display list from the poly data.
  void Build( vtkPolyData *pdata );

  // Description:
  // Render the display list and create the near and far buffers
  float *Draw( vtkRenderer *ren, vtkMatrix4x4 *matrix );

private:
  vtkOpenGLProjectedPolyDataRayBounder(const vtkOpenGLProjectedPolyDataRayBounder&);  // Not implemented.
  void operator=(const vtkOpenGLProjectedPolyDataRayBounder&);  // Not implemented.
};

#endif
