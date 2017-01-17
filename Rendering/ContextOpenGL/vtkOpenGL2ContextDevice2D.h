/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGL2ContextDevice2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkOpenGL2ContextDevice2D
 * @brief   Class for drawing 2D primitives using
 * OpenGL 2.
 *
 *
 * This class takes care of drawing the 2D primitives for the vtkContext2D class.
 * In general this class should not be used directly, but called by vtkContext2D
 * which takes care of many of the higher level details.
 *
 * It assumes that OpenGL 2 is available, which is taken care of by the
 * vtkContextActor class. If OpenGL 2 is not available, but OpenGL rendering is
 * required the vtkOpenGLContextDevice2D class should be used (employs GL 1.1).
 *
 * @sa
 * vtkOpenGLContextDevice2D
*/

#ifndef vtkOpenGL2ContextDevice2D_h
#define vtkOpenGL2ContextDevice2D_h

#include "vtkOpenGLContextDevice2D.h"

class VTKRENDERINGCONTEXTOPENGL_EXPORT vtkOpenGL2ContextDevice2D :
    public vtkOpenGLContextDevice2D
{
public:
  vtkTypeMacro(vtkOpenGL2ContextDevice2D, vtkOpenGLContextDevice2D);
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Creates a 2D Painter object.
   */
  static vtkOpenGL2ContextDevice2D *New();

  /**
   * Return true if the current rendering context supports this device.
   */
  static bool IsSupported(vtkViewport *viewport);

  /**
   * Draw a series of point sprites, images centred at the points supplied.
   * The supplied vtkImageData is the sprite to be drawn, only squares will be
   * drawn and the size is set using SetPointSize. Points are colored by colors array
   * which has nc_comps components
   */
  void DrawPointSprites(vtkImageData *sprite, float *points, int n,
                                unsigned char* colors = 0, int nc_comps = 0) VTK_OVERRIDE;

  /**
   * Draw the supplied image at the given x, y (p[0], p[1]) (bottom corner),
   * scaled by scale (1.0 would match the image).
   */
  void DrawImage(float p[2], float scale, vtkImageData *image) VTK_OVERRIDE;

  /**
   * Draw the supplied image at the given position. The origin, width, and
   * height are specified by the supplied vtkRectf variable pos. The image
   * will be drawn scaled to that size.
   */
  void DrawImage(const vtkRectf& pos, vtkImageData *image) VTK_OVERRIDE;

  /**
   * Draw the supplied PolyData at the given x, y (p[0], p[1]) (bottom corner),
   * scaled by scale (1.0 would match the actual dataset).
   *
   * @warning This functionality is not supported with the legacy OpenGL backend.
   */
  void DrawPolyData(float p[2], float scale, vtkPolyData* polyData,
    vtkUnsignedCharArray* colors, int scalarMode) VTK_OVERRIDE;

  /**
   * Release any graphics resources that are being consumed by this device.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *window) VTK_OVERRIDE;

protected:
  vtkOpenGL2ContextDevice2D();
  ~vtkOpenGL2ContextDevice2D() VTK_OVERRIDE;

  /**
   * Load the OpenGL extensions we need.
   */
  bool LoadExtensions(vtkOpenGLExtensionManager *m) VTK_OVERRIDE;

private:
  vtkOpenGL2ContextDevice2D(const vtkOpenGL2ContextDevice2D &) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGL2ContextDevice2D &) VTK_DELETE_FUNCTION;
};

#endif //vtkOpenGL2ContextDevice2D_h
