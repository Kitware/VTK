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

#include "vtkOpenGLPropItem.h"

#include "vtkCamera.h"
#include "vtkProp3D.h"
#include "vtkContext2D.h"
#include "vtkContextScene.h"
#include "vtkgl.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkOpenGLPropItem)

vtkOpenGLPropItem::vtkOpenGLPropItem()
{
}

vtkOpenGLPropItem::~vtkOpenGLPropItem()
{
}

void vtkOpenGLPropItem::UpdateTransforms()
{
  // Get the active camera:
  vtkRenderer *ren = this->Scene->GetRenderer();
  vtkCamera *activeCamera = ren->GetActiveCamera();

  // Cache the current state:
  this->CameraCache->DeepCopy(activeCamera);

  // Reset the info that computes the view:
  vtkNew<vtkTransform> identity;
  identity->Identity();
  activeCamera->SetUserViewTransform(identity.GetPointer());
  activeCamera->SetFocalPoint(0.0, 0.0, 0.0);
  activeCamera->SetPosition(0.0, 0.0, 1.0);
  activeCamera->SetViewUp(0.0, 1.0, 0.0);

  // Update the camera model matrix with the current OpenGL modelview matrix:
  GLdouble mv[16];
  glGetDoublev(GL_MODELVIEW_MATRIX, mv);
  vtkMatrix4x4::Transpose(mv, mv);
  activeCamera->SetModelTransformMatrix(mv);

  /* The perspective updates aren't nearly as straight-forward, and take a bit
   * of code-spelunking and algebra. By inspecting the following methods, we
   * see how the perspective matrix gets built at render-time:
   *
   * 1) vtkOpenGLCamera::Render() calls
   *    vtkCamera::GetProjectionTransformMatrix() with zRange = [-1, 1] and
   *    aspect = aspectModification * usize / vsize (see below).
   * 2) vtkCamera::GetProjectionTransformMatrix() calls
   *    vtkCamera::ComputeProjectionTransform with the same arguments.
   * 3) vtkCamera::ComputeProjectionTransform calls
   *    vtkPerspectiveTransform::Ortho with:
   *    xminGL = (vtkCamera::WindowCenter[0] - 1) * vtkCamera::ParallelScale * aspect
   *    xmaxGL = (vtkCamera::WindowCenter[0] + 1) * vtkCamera::ParallelScale * aspect
   *    yminGL = (vtkCamera::WindowCenter[1] - 1) * vtkCamera::ParallelScale
   *    ymaxGL = (vtkCamera::WindowCenter[1] + 1) * vtkCamera::ParallelScale
   *    zminGL = vtkCamera::ClippingRange[0]
   *    zmaxGL = vtkCamera::ClippingRange[1]
   *
   * In vtkOpenGLContext2D::Begin, glOrtho is called with:
   *    xminCTX = 0.5
   *    xmaxCTX = glViewport[0] - 0.5
   *    yminCTX = 0.5
   *    ymaxCTX = glViewport[1] - 0.5
   *    zminCTX = -2000
   *    zmaxCTX = 2000
   *
   * To set the camera parameters to reproduce the Context2D projective matrix,
   * the following set of equations can be built:
   *
   * Using:
   *   Cx = vtkCamera::WindowCenter[0] (unknown)
   *   Cy = vtkCamera::WindowCenter[1] (unknown)
   *   P = vtkCamera::ParallelScale (unknown)
   *   a = aspect (known)
   *
   * The equations are:
   *   xminCTX = (Cx - 1)aP
   *   xmaxCTX = (Cx + 1)aP
   *   yminCTX = (Cy - 1)P
   *   ymaxCTX = (Cy + 1)P
   *
   * Solving simultaneously for the unknowns Cx, Cy, and P, we get:
   *   Cx = (xminCTX * a) / (xmaxCTX - xminCTX) + 1
   *   Cy = a * (yminCTX + ymaxCTX) / (xmaxCTX - xminCTX)
   *   P = (xmaxCTX - xminCTX) / (2 * a)
   */

  // Collect the parameters to compute the projection matrix:
  // (see vtkOpenGLCamera::Render)
  int  lowerLeft[2];
  int usize, vsize;
  double aspect1[2];
  double aspect2[2];
  GLint vp[4];
  glGetIntegerv(GL_VIEWPORT, vp);
  ren->GetTiledSizeAndOrigin(&usize, &vsize, lowerLeft, lowerLeft+1);
  ren->ComputeAspect();
  ren->GetAspect(aspect1);
  ren->vtkViewport::ComputeAspect();
  ren->vtkViewport::GetAspect(aspect2);
  double aspectModification = (aspect1[0] * aspect2[1]) /
                              (aspect1[1] * aspect2[0]);

  // Set the variables for the equations:
  double a = aspectModification * usize / vsize;
  double xminCTX = 0.5;
  double xmaxCTX = vp[2] - 0.5;
  double yminCTX = 0.5;
  double ymaxCTX = vp[3] - 0.5;
  double zminCTX = -2000;
  double zmaxCTX = 2000;

  double Cx = (xminCTX * a) / (xmaxCTX - xminCTX) + 1.;
  double Cy = a * (yminCTX + ymaxCTX) / (xmaxCTX - xminCTX);
  double P = (xmaxCTX - xminCTX) / (2 * a);

  // Set the camera state
  activeCamera->SetParallelProjection(1);
  activeCamera->SetParallelScale(P);
  activeCamera->SetWindowCenter(Cx, Cy);
  activeCamera->SetClippingRange(zminCTX, zmaxCTX);
}

void vtkOpenGLPropItem::ResetTransforms()
{
  // Reset the active camera:
  vtkCamera *activeCamera = this->Scene->GetRenderer()->GetActiveCamera();
  activeCamera->DeepCopy(this->CameraCache.GetPointer());
}
