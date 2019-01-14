/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPanoramicProjectionPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPanoramicProjectionPass
 * @brief   Render pass that render the scene in a cubemap and project
 * these six renderings to a single quad.
 * There are currently two differents projections implemented (Equirectangular and Azimuthal).
 * This pass can be used to produce images that can be visualize with specific devices that re-maps
 * the distorted image to a panoramic view (for instance VR headsets, domes, panoramic screens)
 *
 * Note that it is often necessary to disable frustum cullers in order to render
 * properly objects that are behind the camera.
 *
 * @sa
 * vtkRenderPass
 */

#ifndef vtkPanoramicProjectionPass_h
#define vtkPanoramicProjectionPass_h

#include "vtkImageProcessingPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro

class vtkOpenGLFramebufferObject;
class vtkOpenGLQuadHelper;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT vtkPanoramicProjectionPass : public vtkImageProcessingPass
{
public:
  static vtkPanoramicProjectionPass* New();
  vtkTypeMacro(vtkPanoramicProjectionPass, vtkImageProcessingPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state.
   */
  void Render(const vtkRenderState* s) override;

  /**
   * Release graphics resources and ask components to release their own resources.
   */
  void ReleaseGraphicsResources(vtkWindow* w) override;

  //@{
  /**
   * Get/Set the cubemap textures resolution used to render (offscreen) all directions.
   * Default is 300.
   */
  vtkGetMacro(CubeResolution, unsigned int);
  vtkSetMacro(CubeResolution, unsigned int);
  //@}

  /**
   * Enumeration of projection types.
   */
  enum : int
  {
    Equirectangular = 1, /**< Equirectangular projection */
    Azimuthal = 2 /**< Azimuthal equidistant projection */
  };

  //@{
  /**
   * Get/Set the type of projection.
   * Equirectangular projection maps meridians to vertical straight lines and circles of latitude to
   * horizontal straight lines.
   * Azimuthal equidistant projection maps all points of the scene based on their distance to the
   * view direction. This projection produces a fisheye effect.
   * Default is Equirectangular.
   */
  vtkGetMacro(ProjectionType, int);
  vtkSetClampMacro(ProjectionType, int, Equirectangular, Azimuthal);
  void SetProjectionTypeToEquirectangular() { this->SetProjectionType(Equirectangular); }
  void SetProjectionTypeToAzimuthal() { this->SetProjectionType(Azimuthal); }
  //@}

  //@{
  /**
   * Get/Set the vertical angle of projection.
   * 180 degrees is a half sphere, 360 degrees is a full sphere,
   * but any values in the range (90;360) can be set.
   * Default is 180 degrees.
   */
  vtkGetMacro(Angle, double);
  vtkSetClampMacro(Angle, double, 90.0, 360.0);
  //@}

protected:
  vtkPanoramicProjectionPass() = default;
  ~vtkPanoramicProjectionPass() override = default;

  void RenderOnFace(const vtkRenderState* s, int index);

  void Project(vtkOpenGLRenderWindow* renWin);

  void InitOpenGLResources(vtkOpenGLRenderWindow* renWin);

  /**
   * Graphics resources.
   */
  vtkOpenGLFramebufferObject* FrameBufferObject = nullptr;
  vtkTextureObject* CubeMapTexture = nullptr;
  vtkOpenGLQuadHelper* QuadHelper = nullptr;

  unsigned int CubeResolution = 300;
  int ProjectionType = Equirectangular;
  double Angle = 180.0;

private:
  vtkPanoramicProjectionPass(const vtkPanoramicProjectionPass&) = delete;
  void operator=(const vtkPanoramicProjectionPass&) = delete;
};

#endif
