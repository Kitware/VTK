// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkVolumeRepresentation
 * @brief   Renders volumetric data using volume rendering.
 *
 * vtkVolumeRepresentation is a data representation that renders volumetric
 * data (vtkImageData, vtkRectilinearGrid, vtkUnstructuredGrid) using
 * vtkSmartVolumeMapper, which auto-selects the best available volume
 * rendering backend (GPU ray casting, etc.).
 *
 * Transfer functions, shading, and blend modes are exposed through a flat
 * API.  When no color or opacity transfer functions are provided, sensible
 * defaults are created based on the scalar data range.
 *
 * @par Internal pipeline:
 * @verbatim
 * Input (vtkImageData / vtkUnstructuredGrid)
 *   -> vtkSmartVolumeMapper
 *     -> vtkVolume
 * @endverbatim
 *
 * @sa vtkDataRepresentation vtkStandardRenderView vtkSurfaceRepresentation
 */

#ifndef vtkVolumeRepresentation_h
#define vtkVolumeRepresentation_h

#include "vtkDataRepresentation.h"
#include "vtkNew.h"               // For ivars
#include "vtkViewsScivisModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkColorTransferFunction;
class vtkPiecewiseFunction;
class vtkScalarBarActor;
class vtkSmartVolumeMapper;
class vtkVolume;
class vtkVolumeProperty;

class VTKVIEWSSCIVIS_EXPORT vtkVolumeRepresentation : public vtkDataRepresentation
{
public:
  static vtkVolumeRepresentation* New();
  vtkTypeMacro(vtkVolumeRepresentation, vtkDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * The representation stores its properties on the volume mapper, volume and
   * scalar bar it owns rather than in its own ivars.  Those objects are also
   * reachable through GetVolume(), GetScalarBarActor() and friends, so the
   * modified time reported here is the latest of this object's own and theirs.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Transfer functions for color and opacity.
   *
   * The representation builds both from the scalar range of its input the
   * first time it executes, and stops maintaining whichever one you set here.
   * The getters always return the function actually in use, generated or your
   * own; before the first execution they return whatever the volume property
   * was constructed with.
   */
  void SetColorTransferFunction(vtkColorTransferFunction* ctf);
  vtkColorTransferFunction* GetColorTransferFunction();
  void SetScalarOpacity(vtkPiecewiseFunction* pf);
  vtkPiecewiseFunction* GetScalarOpacity();
  void SetScalarOpacityUnitDistance(double distance);
  double GetScalarOpacityUnitDistance();
  ///@}

  ///@{
  /**
   * Give a transfer function back to the representation: discard the one set
   * with SetColorTransferFunction() or SetScalarOpacity() and rebuild the
   * generated one from the current scalar range.  Resetting one function
   * leaves the other alone, whether it is generated or your own.
   *
   * These also serve as a way to re-derive the generated functions after the
   * input data changes, which the representation does not do on its own.
   */
  void ResetColorTransferFunction();
  void ResetScalarOpacity();
  void ResetTransferFunctions();
  ///@}

  ///@{
  /**
   * Volume shading properties.
   */
  void SetShade(bool val);
  bool GetShade();
  void SetAmbient(double val);
  double GetAmbient();
  void SetDiffuse(double val);
  double GetDiffuse();
  void SetSpecular(double val);
  double GetSpecular();
  void SetSpecularPower(double val);
  double GetSpecularPower();
  void SetInterpolationType(int val);
  int GetInterpolationType();
  ///@}

  ///@{
  /**
   * Volume mapper properties.
   */
  void SetBlendMode(int mode);
  int GetBlendMode();
  void SetRequestedRenderMode(int mode);
  int GetRequestedRenderMode();
  ///@}

  ///@{
  /**
   * Visibility and actor transforms.
   */
  void SetVisibility(bool val);
  bool GetVisibility();
  void SetPosition(double x, double y, double z);
  double* GetPosition() VTK_SIZEHINT(3);
  void SetOrientation(double x, double y, double z);
  double* GetOrientation() VTK_SIZEHINT(3);
  void SetScale(double x, double y, double z);
  double* GetScale() VTK_SIZEHINT(3);
  ///@}

  ///@{
  /**
   * Scalar bar control.
   */
  void SetScalarBarVisibility(bool val);
  bool GetScalarBarVisibility();
  vtkScalarBarActor* GetScalarBarActor();
  ///@}

  /**
   * Provide access to the internal volume for advanced usage.
   */
  vtkVolume* GetVolume();

protected:
  vtkVolumeRepresentation();
  ~vtkVolumeRepresentation() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  bool AddToView(vtkView* view) override;
  bool RemoveFromView(vtkView* view) override;

private:
  vtkVolumeRepresentation(const vtkVolumeRepresentation&) = delete;
  void operator=(const vtkVolumeRepresentation&) = delete;

  void CreateDefaultTransferFunctions();

  vtkNew<vtkSmartVolumeMapper> VolumeMapper;
  vtkNew<vtkVolume> VolumeActor;
  vtkNew<vtkVolumeProperty> VolumeProperty;
  vtkNew<vtkScalarBarActor> ScalarBar;
  bool ScalarBarVisible;
  bool DefaultTransferFunctionsCreated;
  bool UserSetColorTransferFunction;
  bool UserSetScalarOpacity;
};

VTK_ABI_NAMESPACE_END
#endif
