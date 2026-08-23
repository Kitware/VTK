// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkVolumeRepresentation
 * @brief   Renders volumetric data using volume rendering.
 *
 * vtkVolumeRepresentation is a data representation that renders volumetric data
 * (vtkImageData, vtkRectilinearGrid, vtkUnstructuredGrid) using
 * vtkSmartVolumeMapper, which auto-selects the best available volume rendering
 * backend.  When no transfer functions are supplied it builds both from the
 * scalar range of the data, so a volume is visible before anything is
 * configured.
 *
 * The API here covers what it takes to make data visible and legible: which
 * array is drawn, the transfer functions it is drawn through, and shading.
 * Everything else is a property of the objects underneath, reachable through
 * GetVolumeProperty() and GetVolumeMapper(): ambient, diffuse, specular and
 * interpolation type on the property; blend mode and requested render mode on
 * the mapper.
 *
 * @par Internal pipeline:
 * @verbatim
 * Input (vtkImageData / vtkUnstructuredGrid)
 *   -> vtkSmartVolumeMapper
 *     -> vtkVolume
 * @endverbatim
 *
 * @sa vtkScivisDataRepresentation vtkScivisView vtkSurfaceRepresentation
 */

#ifndef vtkVolumeRepresentation_h
#define vtkVolumeRepresentation_h

#include "vtkNew.h" // For ivars
#include "vtkScivisDataRepresentation.h"
#include "vtkViewsScivisModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkColorTransferFunction;
class vtkDataArray;
class vtkDataSet;
class vtkPiecewiseFunction;
class vtkScalarsToColors;
class vtkSmartVolumeMapper;
class vtkVolume;
class vtkVolumeProperty;

class VTKVIEWSSCIVIS_EXPORT vtkVolumeRepresentation : public vtkScivisDataRepresentation
{
public:
  static vtkVolumeRepresentation* New();
  vtkTypeMacro(vtkVolumeRepresentation, vtkScivisDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * The representation stores its properties on the volume mapper and volume it
   * owns rather than in its own ivars.  Those objects are also reachable
   * through GetVolume() and friends, so the modified time reported here is the
   * latest of this object's own and theirs.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Transfer functions for color and opacity.
   *
   * The representation builds both from the scalar range of its input the first
   * time it executes, and stops maintaining whichever one you set here.  The
   * getters always return the function actually in use, generated or your own;
   * before the first execution they return whatever the volume property was
   * constructed with.
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
   * Whether the volume is lit.  For the rest of the shading parameters --
   * ambient, diffuse, specular, specular power, interpolation type -- use
   * GetVolumeProperty().
   */
  void SetShade(bool val);
  bool GetShade();
  ///@}

  ///@{
  /**
   * Which array is rendered.  The mapper draws the data's active scalars until
   * one of these selects an array; ResetColorArray goes back to that.
   *
   * The generated transfer functions span the range of the array being
   * rendered, so selecting a different array rebuilds whichever of them is not
   * yours, exactly as ResetTransferFunctions() would.
   */
  void ColorByPointArray(const char* arrayName);
  void ColorByCellArray(const char* arrayName);
  void ResetColorArray();
  ///@}

  ///@{
  /**
   * The vtkScivisDataRepresentation contract.
   *
   * GetColorMap() reports the color transfer function, so a view labels this
   * volume's scalar bar with the colors actually on screen.  It is the same
   * object GetColorTransferFunction() returns; the two differ only in type, and
   * the typed one is the one to use when you want to add points to it.
   *
   * SetColorMap() takes a vtkColorTransferFunction and nothing else.  A volume
   * colors through a transfer function, so a map that is not one -- a lookup
   * table, say, which is what a view usually has to offer -- cannot be used and
   * is ignored rather than refused, since a view offers the same map to
   * everything drawing an array and expects to be turned down.  Passing a
   * transfer function here is the same as SetColorTransferFunction().
   */
  void SetColorMap(vtkScalarsToColors* map) override;
  void SetVisibility(bool val) override;
  bool GetVisibility() override;
  bool GetBounds(double bounds[6]) override;
  const char* GetRenderedArrayName() override;
  int GetRenderedFieldAssociation() override;
  bool GetDataRange(
    const char* arrayName, int fieldAssoc, double range[2], int component = -1) override;
  vtkScalarsToColors* GetColorMap() override;
  ///@}

  ///@{
  /**
   * The objects this representation is built from, for everything the API above
   * does not cover: ambient, diffuse, specular and interpolation type on the
   * property, blend mode and requested render mode on the mapper.
   *
   * Two things the representation holds them to, which reaching in will undo.
   * It tracks whether each transfer function is yours or one it generated, so
   * setting the property's color or scalar opacity directly leaves it believing
   * the function is still its own, and the next change of array will build over
   * what you set -- use SetColorTransferFunction() and SetScalarOpacity().  And
   * the mapper follows the clipping planes held by vtkScivisRepresentation, so
   * giving it a collection of its own disconnects AddClippingPlane().
   */
  vtkVolume* GetVolume();
  vtkVolumeProperty* GetVolumeProperty();
  vtkSmartVolumeMapper* GetVolumeMapper();
  ///@}

protected:
  vtkVolumeRepresentation();
  ~vtkVolumeRepresentation() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  bool AddToView(vtkScivisView* view) override;
  bool RemoveFromView(vtkScivisView* view) override;

private:
  vtkVolumeRepresentation(const vtkVolumeRepresentation&) = delete;
  void operator=(const vtkVolumeRepresentation&) = delete;

  /**
   * Build the color and opacity functions the representation maintains from the
   * range of the array being rendered.  Leaves alone whichever function was set
   * from outside.
   */
  void CreateDefaultTransferFunctions();

  /**
   * The input, brought up to date.  Note that this must not update the
   * representation itself: it is also reached from RequestData().
   */
  vtkDataSet* GetInputDataSet();

  /**
   * The array being mapped to colors, and the attributes it comes from, as the
   * mapper resolves them from the data.
   */
  vtkDataArray* GetRenderedScalars(int& fieldAssoc);

  /**
   * Return true when the mapper already colors by `arrayName` in `scalarMode`.
   */
  bool IsColoringBy(const char* arrayName, int scalarMode);

  vtkNew<vtkSmartVolumeMapper> VolumeMapper;
  vtkNew<vtkVolume> VolumeActor;
  vtkNew<vtkVolumeProperty> VolumeProperty;
  bool DefaultTransferFunctionsCreated;
  bool UserSetColorTransferFunction;
  bool UserSetScalarOpacity;
};

VTK_ABI_NAMESPACE_END
#endif
