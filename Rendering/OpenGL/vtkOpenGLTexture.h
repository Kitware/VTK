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
/**
 * @class   vtkOpenGLTexture
 * @brief   OpenGL texture map
 *
 * vtkOpenGLTexture is a concrete implementation of the abstract class
 * vtkTexture. vtkOpenGLTexture interfaces to the OpenGL rendering library.
*/

#ifndef vtkOpenGLTexture_h
#define vtkOpenGLTexture_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkTexture.h"

#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkWindow;
class vtkOpenGLRenderer;
class vtkRenderWindow;
class vtkPixelBufferObject;

class VTKRENDERINGOPENGL_EXPORT vtkOpenGLTexture : public vtkTexture
{
public:
  static vtkOpenGLTexture *New();
  vtkTypeMacro(vtkOpenGLTexture, vtkTexture);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Implement base class method.
   */
  void Load(vtkRenderer*);

  // Descsription:
  // Clean up after the rendering is complete.
  virtual void PostRender(vtkRenderer*);

  /**
   * Release any graphics resources that are being consumed by this texture.
   * The parameter window could be used to determine which graphic
   * resources to release. Using the same texture object in multiple
   * render windows is NOT currently supported.
   */
  void ReleaseGraphicsResources(vtkWindow*);

  //@{
  /**
   * Get the openGL texture name to which this texture is bound.
   * This is available only if GL version >= 1.1
   */
  vtkGetMacro(Index, long);
  //@}

protected:

  vtkOpenGLTexture();
  ~vtkOpenGLTexture();

  unsigned char *ResampleToPowerOfTwo(int &xsize, int &ysize,
                                      unsigned char *dptr, int bpp);

  vtkTimeStamp   LoadTime;
  unsigned int Index; // actually GLuint
  vtkWeakPointer<vtkRenderWindow> RenderWindow;   // RenderWindow used for previous render
  bool CheckedHardwareSupport;
  bool SupportsNonPowerOfTwoTextures;
  bool SupportsPBO;
  vtkPixelBufferObject *PBO;

private:
  vtkOpenGLTexture(const vtkOpenGLTexture&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLTexture&) VTK_DELETE_FUNCTION;

  /**
   * Handle loading in extension support
   */
  virtual void Initialize(vtkRenderer * ren);

};

#endif
