/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLGL2PSHelperImpl.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkOpenGLGL2PSHelperImpl
 * @brief   vtkOpenGLGL2PSHelper override
 * implementation.
*/

#ifndef vtkOpenGLGL2PSHelperImpl_h
#define vtkOpenGLGL2PSHelperImpl_h

#include "vtkRenderingGL2PSOpenGL2Module.h" // For export macro
#include "vtkOpenGLGL2PSHelper.h"

class vtkMatrix4x4;
class vtkPoints;

class VTKRENDERINGGL2PSOPENGL2_EXPORT vtkOpenGLGL2PSHelperImpl
    : public vtkOpenGLGL2PSHelper
{
public:
  static vtkOpenGLGL2PSHelperImpl *New();
  vtkTypeMacro(vtkOpenGLGL2PSHelperImpl, vtkOpenGLGL2PSHelper)
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  virtual void ProcessTransformFeedback(vtkTransformFeedback *tfc,
                                        vtkRenderer *ren, vtkActor *act);
  virtual void ProcessTransformFeedback(vtkTransformFeedback *tfc,
                                        vtkRenderer *ren,
                                        unsigned char col[4]);
  virtual void ProcessTransformFeedback(vtkTransformFeedback *tfc,
                                        vtkRenderer *ren,
                                        float col[4]);

  virtual void DrawString(const std::string &str, vtkTextProperty *tprop,
                          double pos[3], double backgroundDepth,
                          vtkRenderer *ren);

  virtual void DrawPath(vtkPath *path, double rasterPos[3], double windowPos[2],
                        unsigned char rgba[4], double scale[2] = NULL,
                        double rotateAngle = 0.0, float strokeWidth = -1,
                        const char *label = NULL);

  virtual void Draw3DPath(vtkPath *path, vtkMatrix4x4 *actorMatrix,
                          double rasterPos[3], unsigned char actorColor[4],
                          vtkRenderer *ren, const char *label = NULL);

  virtual void DrawImage(vtkImageData *image, double pos[3]);

protected:
  vtkOpenGLGL2PSHelperImpl();
  ~vtkOpenGLGL2PSHelperImpl();

  /**
   * Translate the tprop's fontname into a Postscript font name.
   */
  static const char* TextPropertyToPSFontName(vtkTextProperty *tprop);

  /**
   * Convert the alignment hint in tprop to a GL2PS text alignment constant.
   */
  static int TextPropertyToGL2PSAlignment(vtkTextProperty *tprop);

  /**
   * Extracts the information needed for transforming and projecting points
   * from a renderer.
   */
  static void GetTransformParameters(vtkRenderer *ren,
                                     vtkMatrix4x4 *actorMatrix,
                                     vtkMatrix4x4 *xform, double vpOrigin[2],
                                     double halfSize[2], double zfact[2]);

  //@{
  /**
   * Project the point from world coordinates into device coordinates.
   */
  static void ProjectPoint(double point[3], vtkRenderer *ren,
                           vtkMatrix4x4 *actorMatrix = NULL);
  static void ProjectPoint(double point[4], vtkMatrix4x4 *transformMatrix,
                           double viewportOrigin[2], double halfWidth,
                           double halfHeight, double zfact1, double zfact2);
  static void ProjectPoints(vtkPoints *points, vtkRenderer *ren,
                            vtkMatrix4x4 *actorMatrix = NULL);
  //@}

  //@{
  /**
   * Unproject the point from device coordinates into world coordinates.
   * Input Z coordinate should be in NDC space.
   */
  static void UnprojectPoint(double point[4], vtkMatrix4x4 *invTransformMatrix,
                             double viewportOrigin[2], double halfWidth,
                             double halfHeight, double zfact1, double zfact2);
  static void UnprojectPoints(double *points3D, vtkIdType numPoints,
                              vtkRenderer *ren,
                              vtkMatrix4x4 *actorMatrix = NULL);
  //@}

  void DrawPathPS(vtkPath *path, double rasterPos[3], double windowPos[2],
                  unsigned char rgba[4], double scale[2], double rotateAngle,
                  float strokeWidth, const std::string &label);
  void DrawPathPDF(vtkPath *path, double rasterPos[3], double windowPos[2],
                   unsigned char rgba[4], double scale[2], double rotateAngle,
                   float strokeWidth, const std::string &label);
  void DrawPathSVG(vtkPath *path, double rasterPos[3], double windowPos[2],
                   unsigned char rgba[4], double scale[2], double rotateAngle,
                   float strokeWidth, const std::string &label);

private:
  vtkOpenGLGL2PSHelperImpl(const vtkOpenGLGL2PSHelperImpl &) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLGL2PSHelperImpl &) VTK_DELETE_FUNCTION;
};

#endif // vtkOpenGLGL2PSHelperImpl_h
