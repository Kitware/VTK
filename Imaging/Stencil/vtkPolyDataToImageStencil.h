// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2008 Atamai, Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolyDataToImageStencil
 * @brief   use polydata to mask an image
 *
 * The vtkPolyDataToImageStencil class will convert polydata into
 * an image stencil.  The polydata can either be a closed surface
 * mesh or a series of polyline contours (one contour per slice).
 * @warning
 * If contours are provided, the contours must be aligned with the
 * Z planes.  Other contour orientations are not supported.
 * @sa
 * vtkImageStencil vtkImageAccumulate vtkImageBlend vtkImageReslice
 */

#ifndef vtkPolyDataToImageStencil_h
#define vtkPolyDataToImageStencil_h

#include "vtkImageStencilSource.h"
#include "vtkImagingStencilModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkIdList;
class vtkMergePoints;
class vtkDataSet;
class vtkPolyData;

class VTKIMAGINGSTENCIL_EXPORT vtkPolyDataToImageStencil : public vtkImageStencilSource
{
public:
  static vtkPolyDataToImageStencil* New();
  vtkTypeMacro(vtkPolyDataToImageStencil, vtkImageStencilSource);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the implicit function to convert into a stencil.
   */
  virtual void SetInputData(vtkPolyData*);
  vtkPolyData* GetInput();
  ///@}

  ///@{
  /**
   * The tolerance for including a voxel inside the stencil.
   * This is in fractions of a voxel, and must be between 0 and 1.
   * Tolerance is only applied in the x and y directions, not in z.
   * Setting the tolerance to zero disables all tolerance checks and
   * might result in faster performance.
   */
  vtkSetClampMacro(Tolerance, double, 0.0, 1.0);
  vtkGetMacro(Tolerance, double);
  ///@}

  ///@{
  /**
   * Enable/Disable SMP for multithreading. SMP is On by default.
   */
  vtkGetMacro(EnableSMP, bool);
  vtkSetMacro(EnableSMP, bool);
  ///@}

protected:
  vtkPolyDataToImageStencil();
  ~vtkPolyDataToImageStencil() override;

  void ThreadedExecute(
    vtkImageStencilData* output, vtkIdList* storage, int extent[6], int threadId);

  static void PolyDataCutter(vtkPolyData* input, vtkPolyData* output, vtkIdList* storage, double z);

  static void PolyDataSelector(
    vtkPolyData* input, vtkPolyData* output, vtkIdList* storage, double z, double thickness);

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillInputPortInformation(int, vtkInformation*) override;

  /**
   * The tolerance distance for favoring the inside of the stencil
   */
  double Tolerance;

  bool EnableSMP;
  class ThreadWorker;

private:
  vtkPolyDataToImageStencil(const vtkPolyDataToImageStencil&) = delete;
  void operator=(const vtkPolyDataToImageStencil&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
