/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLImageActor.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLImageActor - OpenGL texture map
// .SECTION Description
// vtkOpenGLImageActor is a concrete implementation of the abstract class 
// vtkImageActor. vtkOpenGLImageActor interfaces to the OpenGL rendering library.

#ifndef __vtkOpenGLImageActor_h
#define __vtkOpenGLImageActor_h

#include "vtkImageActor.h"

class vtkWindow;
class vtkOpenGLRenderer;
class vtkRenderWindow;

class VTK_RENDERING_EXPORT vtkOpenGLImageActor : public vtkImageActor
{
public:
  static vtkOpenGLImageActor *New();
  vtkTypeRevisionMacro(vtkOpenGLImageActor,vtkImageActor);

  // Description:
  // Implement base class method.
  void Load(vtkRenderer *ren);
  
  // Description:
  // Release any graphics resources that are being consumed by this texture.
  // The parameter window could be used to determine which graphic
  // resources to release. Using the same texture object in multiple
  // render windows is NOT currently supported. 
  void ReleaseGraphicsResources(vtkWindow *);

protected:
  vtkOpenGLImageActor();
  ~vtkOpenGLImageActor();

  unsigned char *MakeDataSuitable(int &xsize, int &ysize, int &release);

  vtkTimeStamp   LoadTime;
  long          Index;
  vtkRenderWindow *RenderWindow;   // RenderWindow used for previous render
  float Coords[12];
  float TCoords[8];
private:
  vtkOpenGLImageActor(const vtkOpenGLImageActor&);  // Not implemented.
  void operator=(const vtkOpenGLImageActor&);  // Not implemented.
};

#endif
