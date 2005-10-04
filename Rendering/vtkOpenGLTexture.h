/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLTexture.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLTexture - OpenGL texture map
// .SECTION Description
// vtkOpenGLTexture is a concrete implementation of the abstract class 
// vtkTexture. vtkOpenGLTexture interfaces to the OpenGL rendering library.

#ifndef __vtkOpenGLTexture_h
#define __vtkOpenGLTexture_h

#include "vtkTexture.h"

class vtkWindow;
class vtkOpenGLRenderer;
class vtkRenderWindow;

class VTK_RENDERING_EXPORT vtkOpenGLTexture : public vtkTexture
{
public:
  static vtkOpenGLTexture *New();
  vtkTypeRevisionMacro(vtkOpenGLTexture,vtkTexture);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implement base class method.
  void Load(vtkRenderer *ren);
  
  // Description:
  // Release any graphics resources that are being consumed by this texture.
  // The parameter window could be used to determine which graphic
  // resources to release. Using the same texture object in multiple
  // render windows is NOT currently supported. 
  void ReleaseGraphicsResources(vtkWindow *);

  
  // Description:
  // Get the openGL texture name to which this texture is bound.
  // This is available only if GL version >= 1.1
  vtkGetMacro(Index, long);
protected:
  vtkOpenGLTexture();
  ~vtkOpenGLTexture();

  unsigned char *ResampleToPowerOfTwo(int &xsize, int &ysize, 
                                      unsigned char *dptr, int bpp);

  vtkTimeStamp   LoadTime;
  long          Index;
  vtkRenderWindow *RenderWindow;   // RenderWindow used for previous render
private:
  vtkOpenGLTexture(const vtkOpenGLTexture&);  // Not implemented.
  void operator=(const vtkOpenGLTexture&);  // Not implemented.
};

#endif
