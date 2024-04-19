// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkProjectedTexture
 * @brief   assign texture coordinates for a projected texture
 *
 * vtkProjectedTexture assigns texture coordinates to a dataset as if
 * the texture was projected from a slide projected located somewhere in the
 * scene.  Methods are provided to position the projector and aim it at a
 * location, to set the width of the projector's frustum, and to set the
 * range of texture coordinates assigned to the dataset.
 *
 * Objects in the scene that appear behind the projector are also assigned
 * texture coordinates; the projected image is left-right and top-bottom
 * flipped, much as a lens' focus flips the rays of light that pass through
 * it.  A warning is issued if a point in the dataset falls at the focus
 * of the projector.
 */

#ifndef vtkProjectedTexture_h
#define vtkProjectedTexture_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersModelingModule.h" // For export macro

#define VTK_PROJECTED_TEXTURE_USE_PINHOLE 0
#define VTK_PROJECTED_TEXTURE_USE_TWO_MIRRORS 1

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSMODELING_EXPORT vtkProjectedTexture : public vtkDataSetAlgorithm
{
public:
  static vtkProjectedTexture* New();
  vtkTypeMacro(vtkProjectedTexture, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the position of the focus of the projector.
   */
  vtkSetVector3Macro(Position, double);
  vtkGetVectorMacro(Position, double, 3);
  ///@}

  ///@{
  /**
   * Set/Get the focal point of the projector (a point that lies along
   * the center axis of the projector's frustum).
   */
  void SetFocalPoint(double focalPoint[3]);
  void SetFocalPoint(double x, double y, double z);
  vtkGetVectorMacro(FocalPoint, double, 3);
  ///@}

  ///@{
  /**
   * Set/Get the camera mode of the projection -- pinhole projection or
   * two mirror projection.
   */
  vtkSetMacro(CameraMode, int);
  vtkGetMacro(CameraMode, int);
  void SetCameraModeToPinhole() { this->SetCameraMode(VTK_PROJECTED_TEXTURE_USE_PINHOLE); }
  void SetCameraModeToTwoMirror() { this->SetCameraMode(VTK_PROJECTED_TEXTURE_USE_TWO_MIRRORS); }
  ///@}

  ///@{
  /**
   * Set/Get the mirror separation for the two mirror system.
   */
  vtkSetMacro(MirrorSeparation, double);
  vtkGetMacro(MirrorSeparation, double);
  ///@}

  ///@{
  /**
   * Get the normalized orientation vector of the projector.
   */
  vtkGetVectorMacro(Orientation, double, 3);
  ///@}

  ///@{
  /**
   * Set/Get the up vector of the projector.
   */
  vtkSetVector3Macro(Up, double);
  vtkGetVectorMacro(Up, double, 3);
  ///@}

  ///@{
  /**
   * Set/Get the aspect ratio of a perpendicular cross-section of the
   * the projector's frustum.  The aspect ratio consists of three
   * numbers:  (x, y, z), where x is the width of the
   * frustum, y is the height, and z is the perpendicular
   * distance from the focus of the projector.

   * For example, if the source of the image is a pinhole camera with
   * view angle A, then you could set x=1, y=1, z=1/tan(A).
   */
  vtkSetVector3Macro(AspectRatio, double);
  vtkGetVectorMacro(AspectRatio, double, 3);
  ///@}

  ///@{
  /**
   * Specify s-coordinate range for texture s-t coordinate pair.
   */
  vtkSetVector2Macro(SRange, double);
  vtkGetVectorMacro(SRange, double, 2);
  ///@}

  ///@{
  /**
   * Specify t-coordinate range for texture s-t coordinate pair.
   */
  vtkSetVector2Macro(TRange, double);
  vtkGetVectorMacro(TRange, double, 2);
  ///@}

protected:
  vtkProjectedTexture();
  ~vtkProjectedTexture() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  void ComputeNormal();

  int CameraMode;

  double Position[3];
  double Orientation[3];
  double FocalPoint[3];
  double Up[3];
  double MirrorSeparation;
  double AspectRatio[3];
  double SRange[2];
  double TRange[2];

private:
  vtkProjectedTexture(const vtkProjectedTexture&) = delete;
  void operator=(const vtkProjectedTexture&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
