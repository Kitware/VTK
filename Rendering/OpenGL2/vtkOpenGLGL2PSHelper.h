/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLGL2PSHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkOpenGLGL2PSHelper
 * @brief   Access GL2PS functionality.
 *
 *
 * This class provides convenience functions that can be used to draw into a
 * GL2PS context. Link to vtkRenderingGL2PSOpenGL2 to bring in the
 * vtkOpenGLGL2PSHelperImpl class, the object factory override that implements
 * this interface.
*/

#ifndef vtkOpenGLGL2PSHelper_h
#define vtkOpenGLGL2PSHelper_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkObject.h"
#include <string> // For string usage

class vtkActor;
class vtkImageData;
class vtkMatrix4x4;
class vtkPath;
class vtkRenderer;
class vtkRenderWindow;
class vtkTextProperty;
class vtkTransformFeedback;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLGL2PSHelper: public vtkObject
{
public:
  static vtkOpenGLGL2PSHelper* New();
  vtkAbstractTypeMacro(vtkOpenGLGL2PSHelper, vtkObject)
  void PrintSelf(ostream &os, vtkIndent indent) override;

  //@{
  /**
   * The global instance. Only set during export.
   */
  static vtkOpenGLGL2PSHelper* GetInstance();
  static void SetInstance(vtkOpenGLGL2PSHelper *);
  //@}

  //@{
  /**
   * Get the renderwindow that's being exported.
   */
  vtkGetMacro(RenderWindow, vtkRenderWindow*)
  //@}

  enum State
  {
    Inactive = 0, //! No export active
    Background, //! Rendering rasterized props for the background.
    Capture //! Capturing vectorized objects.
  };

  //@{
  /**
   * Get the current export state. Vector images are rendered in two passes:
   * First, all non-vectorizable props are rendered, and the resulting image
   * is inserted as a raster image into the background of the exported file
   * (ActiveState == Background). Next, all vectorizable props are drawn
   * and captured into GL2PS, where they are drawn over the background image.
   * Vectorizable props should not draw themselves during the background pass,
   * and use the vtkOpenGLGL2PSHelper API to draw themselves during the capture
   * pass.
   */
  vtkGetMacro(ActiveState, State)
  //@}

  //@{
  /**
   * Set/Get the current point size.
   */
  vtkSetMacro(PointSize, float)
  vtkGetMacro(PointSize, float)
  //@}

  //@{
  /**
   * Set/Get the current line width.
   */
  vtkSetMacro(LineWidth, float)
  vtkGetMacro(LineWidth, float)
  //@}

  //@{
  /**
   * Set/Get the current line stipple pattern per OpenGL convention. Default is
   * 0xffff.
   */
  vtkSetMacro(LineStipple, unsigned short)
  vtkGetMacro(LineStipple, unsigned short)
  //@}

  //@{
  /**
   * Parse the vertex information in tfc and inject primitives into GL2PS.
   * ren is used to obtain viewport information to complete the vertex
   * transformation into pixel coordinates, and act/col are used to color the
   * vertices when tfc does not contain color information.
   */
  virtual void ProcessTransformFeedback(vtkTransformFeedback *tfc,
                                        vtkRenderer *ren, vtkActor *act) = 0;
  virtual void ProcessTransformFeedback(vtkTransformFeedback *tfc,
                                        vtkRenderer *ren,
                                        unsigned char col[4]) = 0;
  virtual void ProcessTransformFeedback(vtkTransformFeedback *tfc,
                                        vtkRenderer *ren,
                                        float col[4]) = 0;
  //@}

  /**
   * Format the text in str according to tprop and instruct GL2PS to draw it at
   * pixel coordinate pos. Background depth is the z value for the background
   * quad, and should be in NDC space.
   * The drawing is always done in the overlay plane.
   * @sa TextAsPath
   */
  virtual void DrawString(const std::string &str, vtkTextProperty *tprop,
                          double pos[3], double backgroundDepth,
                          vtkRenderer *ren) = 0;

  /**
   * Generate PS, EPS, or SVG markup from a vtkPath object, and then inject it
   * into the output using the gl2psSpecial command. The path is translated
   * uniformly in the scene by windowPos. It is scaled by scale and rotated
   * counter-clockwise by rotateAngle. The rasterPos is in world coordinates
   * and determines clipping and depth. If scale is NULL, no scaling is done.
   * If strokeWidth is positive, the path will be stroked with the indicated
   * width. If zero or negative, the path will be filled (default).
   * The label string is inserted into the GL2PS output at the beginning of the
   * path specification as a comment on supported backends.
   */
  virtual void DrawPath(vtkPath *path, double rasterPos[3], double windowPos[2],
                        unsigned char rgba[4], double scale[2] = nullptr,
                        double rotateAngle = 0.0, float strokeWidth = -1,
                        const char *label = nullptr) = 0;

  /**
   * Transform the path using the actor's matrix and current GL state, then
   * draw it to GL2PS. The label string is inserted into the GL2PS output at the
   * beginning of the path specification as a comment on supported backends.
   */
  virtual void Draw3DPath(vtkPath *path, vtkMatrix4x4 *actorMatrix,
                          double rasterPos[3], unsigned char actorColor[4],
                          vtkRenderer *ren, const char *label = nullptr) = 0;

  /**
   * Draw the image at pos.
   * Image must be RGB or RGBA with float scalars.
   */
  virtual void DrawImage(vtkImageData *image, double pos[3]) = 0;

protected:
  friend class vtkOpenGLGL2PSExporter;

  vtkOpenGLGL2PSHelper();
  ~vtkOpenGLGL2PSHelper() override;

  vtkSetMacro(ActiveState, State)
  vtkSetMacro(TextAsPath, bool)
  vtkSetMacro(RenderWindow, vtkRenderWindow*) // Doesn't ref count, not needed.
  vtkSetMacro(PointSizeFactor, float)
  vtkSetMacro(LineWidthFactor, float)

  static vtkOpenGLGL2PSHelper *Instance;

  vtkRenderWindow *RenderWindow;
  State ActiveState;
  bool TextAsPath;
  float PointSize;
  float LineWidth;
  float PointSizeFactor;
  float LineWidthFactor;
  unsigned short LineStipple;

private:
  vtkOpenGLGL2PSHelper(const vtkOpenGLGL2PSHelper &) = delete;
  void operator=(const vtkOpenGLGL2PSHelper &) = delete;
};

#endif // vtkOpenGLGL2PSHelper_h
