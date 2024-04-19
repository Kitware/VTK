// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkStitchImageDataWithGhosts
 * @brief Stitches vtkImageData that are one voxel away
 *
 * Given a set of `vtkImageData` inputs, either across MPI ranks or within a `vtkCompositeDataSet`,
 * this filter will stitch inputs that are spaced by one voxel. The inputs CANNOT have any
 * `vtkCellData` arrays, as the cells filling the gaps don't have any way to have values. As a
 * consequence, if the input `vtkCellData` is not empty, the filter will not run.
 *
 * Ghost points are set at the interfaces between the images, such that one and only one image
 * has a non-ghost version of the point.
 *
 * One can optionally generate extra layers of ghost cells. If `NumberOfGhostLayers` is set to 1,
 * the images will be stitched. If it is set to n, the images will be stitched and have n - 1
 * layers of ghost cells.
 */

#ifndef vtkStitchImageDataWithGhosts_h
#define vtkStitchImageDataWithGhosts_h

#include "vtkFiltersParallelDIY2Module.h" // for export macros
#include "vtkGhostCellsGenerator.h"

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSPARALLELDIY2_EXPORT vtkStitchImageDataWithGhosts : public vtkGhostCellsGenerator
{
public:
  static vtkStitchImageDataWithGhosts* New();
  vtkTypeMacro(vtkStitchImageDataWithGhosts, vtkGhostCellsGenerator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Initialize() override;

protected:
  vtkStitchImageDataWithGhosts();

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkStitchImageDataWithGhosts(const vtkStitchImageDataWithGhosts&) = delete;
  void operator=(const vtkStitchImageDataWithGhosts&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
