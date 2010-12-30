/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLImageActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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
  vtkTypeMacro(vtkOpenGLImageActor,vtkImageActor);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implement base class method.
  void Load(vtkRenderer *ren);
  
  // Description:
  // Implement base class method.
  void Render(vtkRenderer *ren);

  // Description:
  // Release any graphics resources that are being consumed by this texture.
  // The parameter window could be used to determine which graphic
  // resources to release. Using the same texture object in multiple
  // render windows is NOT currently supported. 
  void ReleaseGraphicsResources(vtkWindow *);

protected:
  vtkOpenGLImageActor();
  ~vtkOpenGLImageActor();

  unsigned char *MakeDataSuitable(int &xsize, int &ysize,
                                  int &release, int &reuseTexture);

  vtkTimeStamp   LoadTime;
  long          Index;
  vtkRenderWindow *RenderWindow;   // RenderWindow used for previous render
  double Coords[12];
  double TCoords[8];
  int TextureSize[2];
  int TextureBytesPerPixel;
  
  // Non-recursive internal method
  void InternalRender(vtkRenderer *ren);
  
  // Is a certain size texture supported?
  int TextureSizeOK( int size[2] );
  
private:
  vtkOpenGLImageActor(const vtkOpenGLImageActor&);  // Not implemented.
  void operator=(const vtkOpenGLImageActor&);  // Not implemented.
};

#endif
