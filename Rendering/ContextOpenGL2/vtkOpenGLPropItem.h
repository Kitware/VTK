/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLPropItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkOpenGLPropItem
 * @brief   Sync Context2D state with vtk camera.
 *
 *
 * The vtkContext2D framework modifies the GL state directly, while some actors
 * and mappers rely on the modelview/projection matrices from vtkCamera. This
 * class is a layer between the two that updates the camera with the current
 * OpenGL state.
*/

#ifndef vtkOpenGLPropItem_h
#define vtkOpenGLPropItem_h

#include "vtkRenderingContextOpenGL2Module.h" // For export macro
#include "vtkPropItem.h"
#include "vtkNew.h" // for vtkNew

class vtkCamera;

class VTKRENDERINGCONTEXTOPENGL2_EXPORT vtkOpenGLPropItem: public vtkPropItem
{
public:
  static vtkOpenGLPropItem *New();
  vtkTypeMacro(vtkOpenGLPropItem, vtkPropItem)

  virtual bool Paint(vtkContext2D *painter);

protected:
  vtkOpenGLPropItem();
  ~vtkOpenGLPropItem();

  // Sync the active vtkCamera with the GL state set by the painter.
  virtual void UpdateTransforms();

  // Restore the vtkCamera state.
  virtual void ResetTransforms();

private:
  vtkNew<vtkCamera> CameraCache;
  vtkContext2D *Painter;

  vtkOpenGLPropItem(const vtkOpenGLPropItem &) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLPropItem &) VTK_DELETE_FUNCTION;
};

#endif //vtkOpenGLPropItem_h
