// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkSkybox
 * @brief Renders a skybox environment
 *
 * You must provide a texture cube map using the SetTexture method.
 */

#ifndef vtkSkybox_h
#define vtkSkybox_h

#include "vtkActor.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkSkybox : public vtkActor
{
public:
  static vtkSkybox* New();
  vtkTypeMacro(vtkSkybox, vtkActor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax). (The
   * method GetBounds(double bounds[6]) is available from the superclass.)
   */
  using Superclass::GetBounds;
  double* GetBounds() override;

  ///@{
  /**
   * Set/Get the projection to be used
   */
  enum Projection
  {
    Cube,
    Sphere,
    Floor,
    StereoSphere
  };
  vtkGetMacro(Projection, int);
  vtkSetMacro(Projection, int);
  void SetProjectionToCube() { this->SetProjection(vtkSkybox::Cube); }
  void SetProjectionToSphere() { this->SetProjection(vtkSkybox::Sphere); }
  void SetProjectionToStereoSphere() { this->SetProjection(vtkSkybox::StereoSphere); }
  void SetProjectionToFloor() { this->SetProjection(vtkSkybox::Floor); }
  ///@}

  ///@{
  /**
   * Set/Get the plane equation for the floor.
   */
  vtkSetVector4Macro(FloorPlane, float);
  vtkGetVector4Macro(FloorPlane, float);
  vtkSetVector3Macro(FloorRight, float);
  vtkGetVector3Macro(FloorRight, float);
  ///@}

  ///@{
  /**
   * Set/Get the [u,v] texture coordinate scaling for the floor projection.
   * Defaults to [1, 1] i.e. no scaling, which means the floor texture coordinates are computed
   * based on the view coordinates of the plane points.
   *
   * \sa SetProjectionToFloor()
   */
  vtkGetVector2Macro(FloorTexCoordScale, float);
  vtkSetVector2Macro(FloorTexCoordScale, float);
  ///@}

  ///@{
  /**
   * Define if the colors should be gamma corrected.
   * This is generally required if the input texture is in linear color space.
   * Default is off.
   */
  vtkGetMacro(GammaCorrect, bool);
  vtkSetMacro(GammaCorrect, bool);
  vtkBooleanMacro(GammaCorrect, bool);
  ///@}

protected:
  vtkSkybox();
  ~vtkSkybox() override;

  int Projection;
  float FloorPlane[4];
  float FloorRight[3];
  float FloorTexCoordScale[2];

  bool GammaCorrect = false;

private:
  vtkSkybox(const vtkSkybox&) = delete;
  void operator=(const vtkSkybox&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkSkybox_h
