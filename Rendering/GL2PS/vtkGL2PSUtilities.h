/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGL2PSUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGL2PSUtilities
 * @brief   Helper functions for using GL2PS within VTK
 *
 * vtkGL2PSUtilities implements some static helper function that simplify
 * calling GL2PS routines on VTK objects. This class is meant for internal use
 * only and is subject to change.
*/

#ifndef vtkGL2PSUtilities_h
#define vtkGL2PSUtilities_h

#include "vtkObject.h"
#include "vtkRenderingGL2PSModule.h" // For export macro

class vtkImageData;
class vtkMatrix4x4;
class vtkPath;
class vtkPoints;
class vtkRenderWindow;
class vtkTextProperty;

class VTKRENDERINGGL2PS_EXPORT vtkGL2PSUtilities : public vtkObject
{
public:
  static vtkGL2PSUtilities *New();
  vtkTypeMacro(vtkGL2PSUtilities, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent)
  {
    this->Superclass::PrintSelf(os, indent);
  }

  /**
   * Format the text in str according to tprop and instruct GL2PS to draw it at
   * world coordinate pos. Background depth is no longer used.
   */
  static void DrawString(const char *str, vtkTextProperty *tprop, double pos[3],
                         double backgroundDepth);

  /**
   * Translate the tprop's fontname into a Postscript font name.
   */
  static const char * TextPropertyToPSFontName(vtkTextProperty *tprop);

  /**
   * Convert the alignment hint in tprop to a GL2PS text alignment constant.
   */
  static int TextPropertyToGL2PSAlignment(vtkTextProperty *tprop);

  /**
   * Get the current RenderWindow that is being exported
   */
  static vtkRenderWindow *GetRenderWindow()
  {
    return vtkGL2PSUtilities::RenderWindow;
  }

  /**
   * Transform the path using the actor's matrix and current GL state, then
   * draw it to GL2PS. The label string is inserted into the GL2PS output at the
   * beginning of the path specification as a comment on supported backends.
   */
  static void Draw3DPath(vtkPath *path, vtkMatrix4x4 *actorMatrix,
                         double rasterPos[3], unsigned char actorColor[4],
                         const char *label = NULL);
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
  static void DrawPath(vtkPath *path, double rasterPos[3], double windowPos[2],
                       unsigned char rgba[4], double scale[2] = NULL,
                       double rotateAngle = 0.0, float strokeWidth = -1,
                       const char *label = NULL);

  /**
   * Get whether all text will be exported as paths.
   */
  static bool GetTextAsPath()
  {
    return vtkGL2PSUtilities::TextAsPath;
  }

  /**
   * Get a scaling factor for the point size or line width used by GL2PS.
   * Default value: 5/7.
   */
  static float GetPointSizeFactor()
    { return vtkGL2PSUtilities::PointSizeFactor; }
  static float GetLineWidthFactor()
    { return vtkGL2PSUtilities::LineWidthFactor; }

protected:
  friend class vtkOpenGLGL2PSExporter;

  static void StartExport();
  static void FinishExport();

  static void SetPointSizeFactor(float f)
    { vtkGL2PSUtilities::PointSizeFactor = f; }

  static void SetLineWidthFactor(float f)
    { vtkGL2PSUtilities::LineWidthFactor = f; }

  static void SetTextAsPath(bool b)
  {
    vtkGL2PSUtilities::TextAsPath = b;
  }

  static void SetRenderWindow(vtkRenderWindow *renWin)
  {
    vtkGL2PSUtilities::RenderWindow = renWin;
  }

  static void DrawPathPS(vtkPath *path, double rasterPos[3],
                         double windowPos[2], unsigned char rgba[4],
                         double scale[2] = NULL, double rotateAngle = 0.0,
                         float strokeWidth = -1, const char *label = NULL);
  static void DrawPathPDF(vtkPath *path, double rasterPos[3],
                          double windowPos[2], unsigned char rgba[4],
                          double scale[2] = NULL, double rotateAngle = 0.0,
                          float strokeWidth = -1, const char *label = NULL);
  static void DrawPathSVG(vtkPath *path, double rasterPos[3],
                          double windowPos[2], unsigned char rgba[4],
                          double scale[2] = NULL, double rotateAngle = 0.0,
                          float strokeWidth = -1, const char *label = NULL);

  vtkGL2PSUtilities() {}
  ~vtkGL2PSUtilities() {}

private:
  vtkGL2PSUtilities(const vtkGL2PSUtilities &) VTK_DELETE_FUNCTION;
  void operator=(const vtkGL2PSUtilities&) VTK_DELETE_FUNCTION;

  static vtkRenderWindow *RenderWindow;
  static bool TextAsPath;
  static float PointSizeFactor;
  static float LineWidthFactor;

  //@{
  /**
   * Project the point from world coordinates into device coordinates.
   */
  static void ProjectPoint(double point[4], vtkMatrix4x4 *actorMatrix = NULL);
  static void ProjectPoint(double point[4], vtkMatrix4x4 * transformMatrix,
                           double viewportOrigin[2], double halfWidth,
                           double halfHeight, double zfact1, double zfact2);
  static void ProjectPoints(vtkPoints *points,
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
                              vtkMatrix4x4 *actorMatrix = NULL);
};
  //@}

#endif
