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

// .NAME vtkOpenGLGL2PSHelper - Helper functionality for GL2PS exporting.
//
// .SECTION Description
// vtkOpenGLGL2PSHelper provides a number of static variables and methods that
// are used during GL2PS exporting. The rational behind this class is that GL
// does not include all of the information that GL2PS needs into its feedback
// buffer, and in certain situations GL2PS function calls need to be made
// alongside their GL equivalents, notably glLineWidth and glPointSize.
//
// The static variables in this class are set by vtkGL2PSUtilities at the
// beginning of a GL2PS export render. This class fakes GL2PS calls like
// gl2psLineWidth in order to keep GL2PS from being a module dependency for
// vtkRenderingOpenGL.

#ifndef __vtkOpenGLGL2PSHelper_h
#define __vtkOpenGLGL2PSHelper_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkOpenGL.h" // for GL defines.

class VTKRENDERINGOPENGL_EXPORT vtkOpenGLGL2PSHelper
{
public:

  // Description:
  // Call alongside glLineWidth(lineWidth) to inform GL2PS of the change.
  static void SetLineWidth(float lineWidth)
  {
    if (vtkOpenGLGL2PSHelper::InGL2PSRender)
      {
      glPassThrough(vtkOpenGLGL2PSHelper::LineWidthToken);
      glPassThrough(vtkOpenGLGL2PSHelper::LineWidthFactor * lineWidth);
      }
  }

  // Description:
  // Call alongside glPointSize(pointSize) to inform GL2PS of the change.
  static void SetPointSize(float pointSize)
  {
    if (vtkOpenGLGL2PSHelper::InGL2PSRender)
    {
      glPassThrough(vtkOpenGLGL2PSHelper::PointSizeToken);
      glPassThrough(vtkOpenGLGL2PSHelper::PointSizeFactor * pointSize);
    }
  }

  // Description:
  // Call alongside glEnable(GL_LINE_STIPPLE) to inform GL2PS of the change.
  // This must be called *after* calling glLineStipple(factor, pattern).
  static void EnableStipple()
  {
    if (vtkOpenGLGL2PSHelper::InGL2PSRender)
    {
      GLint tmp;
      glPassThrough(vtkOpenGLGL2PSHelper::StippleBeginToken);
      glGetIntegerv(GL_LINE_STIPPLE_PATTERN, &tmp);
      glPassThrough(static_cast<GLfloat>(tmp));
      glGetIntegerv(GL_LINE_STIPPLE_REPEAT, &tmp);
      glPassThrough(static_cast<GLfloat>(tmp));
    }
  }

  // Description:
  // Call alongside glDisable(GL_LINE_STIPPLE) to inform GL2PS of the change.
  static void DisableStipple()
  {
    if (vtkOpenGLGL2PSHelper::InGL2PSRender)
    {
      glPassThrough(vtkOpenGLGL2PSHelper::StippleEndToken);
    }
  }


protected:
  friend class vtkGL2PSUtilities;

  static bool InGL2PSRender;
  static GLfloat PointSizeFactor;
  static GLfloat LineWidthFactor;
  static GLfloat PointSizeToken;
  static GLfloat LineWidthToken;
  static GLfloat StippleBeginToken;
  static GLfloat StippleEndToken;

private:
  // static-only class -- no need to construct/destroy.
  vtkOpenGLGL2PSHelper();
  ~vtkOpenGLGL2PSHelper();
  vtkOpenGLGL2PSHelper(const vtkOpenGLGL2PSHelper &); // Not implemented.
  void operator=(const vtkOpenGLGL2PSHelper &);   // Not implemented.
};

#endif //__vtkOpenGLGL2PSHelper_h
// VTK-HeaderTest-Exclude: vtkOpenGLGL2PSHelper.h
