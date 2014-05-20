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

#ifndef __vtkOpenGL2Texture_h
#define __vtkOpenGL2Texture_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkTexture.h"
//BTX
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.
//ETX

class vtkWindow;
class vtkRenderWindow;
class vtkPixelBufferObject;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGL2Texture : public vtkTexture
{
public:
  static vtkOpenGL2Texture *New();
  vtkTypeMacro(vtkOpenGL2Texture, vtkTexture);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implement base class method.
  void Load(vtkRenderer*);

  // Descsription:
  // Clean up after the rendering is complete.
  virtual void PostRender(vtkRenderer*);

  // Description:
  // Release any graphics resources that are being consumed by this texture.
  // The parameter window could be used to determine which graphic
  // resources to release. Using the same texture object in multiple
  // render windows is NOT currently supported.
  void ReleaseGraphicsResources(vtkWindow*);

  // Description:
  // Get the openGL texture name to which this texture is bound.
  vtkGetMacro(Index, long);

protected:
//BTX
  vtkOpenGL2Texture();
  ~vtkOpenGL2Texture();

  vtkTimeStamp   LoadTime;
  unsigned int Index; // actually GLuint
  vtkWeakPointer<vtkRenderWindow> RenderWindow;   // RenderWindow used for previous render
  bool CheckedHardwareSupport;
  bool SupportsNonPowerOfTwoTextures;
  bool SupportsPBO;
  vtkPixelBufferObject *PBO;

  // used when the texture exceeds the GL limit
  unsigned char *ResampleToPowerOfTwo(int &xsize, int &ysize,
                                      unsigned char *dptr, int bpp);



private:
  vtkOpenGL2Texture(const vtkOpenGL2Texture&);  // Not implemented.
  void operator=(const vtkOpenGL2Texture&);  // Not implemented.

  // Description:
  // Handle loading in extension support
  virtual void Initialize(vtkRenderer * ren);

//ETX
};

#endif
