// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkExtractGrid
 * @brief   select piece (e.g., volume of interest) and/or subsample structured grid dataset
 *
 *
 * vtkExtractGrid is a filter that selects a portion of an input structured
 * grid dataset, or subsamples an input dataset. (The selected portion of
 * interested is referred to as the Volume Of Interest, or VOI.) The output of
 * this filter is a structured grid dataset. The filter treats input data of
 * any topological dimension (i.e., point, line, image, or volume) and can
 * generate output data of any topological dimension.
 *
 * To use this filter set the VOI ivar which are i-j-k min/max indices that
 * specify a rectangular region in the data. (Note that these are 0-offset.)
 * You can also specify a sampling rate to subsample the data.
 *
 * Typical applications of this filter are to extract a plane from a grid for
 * contouring, subsampling large grids to reduce data size, or extracting
 * regions of a grid with interesting data.
 *
 * @sa
 * vtkGeometryFilter vtkExtractGeometry vtkExtractVOI
 * vtkStructuredGridGeometryFilter
 */

#ifndef vtkExtractGrid_h
#define vtkExtractGrid_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkStructuredGridAlgorithm.h"

// Forward Declarations
VTK_ABI_NAMESPACE_BEGIN
class vtkExtractStructuredGridHelper;

class VTKFILTERSEXTRACTION_EXPORT vtkExtractGrid : public vtkStructuredGridAlgorithm
{
public:
  static vtkExtractGrid* New();
  vtkTypeMacro(vtkExtractGrid, vtkStructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify i-j-k (min,max) pairs to extract. The resulting structured grid
   * dataset can be of any topological dimension (i.e., point, line, plane,
   * or 3D grid).
   */
  vtkSetVector6Macro(VOI, int);
  vtkGetVectorMacro(VOI, int, 6);
  ///@}

  ///@{
  /**
   * Set the sampling rate in the i, j, and k directions. If the rate is > 1,
   * then the resulting VOI will be subsampled representation of the input.
   * For example, if the SampleRate=(2,2,2), every other point will be
   * selected, resulting in a volume 1/8th the original size.
   * Initial value is (1,1,1).
   */
  vtkSetVector3Macro(SampleRate, int);
  vtkGetVectorMacro(SampleRate, int, 3);
  ///@}

  ///@{
  /**
   * Control whether to enforce that the "boundary" of the grid is output in
   * the subsampling process. (This ivar only has effect when the SampleRate
   * in any direction is not equal to 1.) When this ivar IncludeBoundary is
   * on, the subsampling will always include the boundary of the grid even
   * though the sample rate is not an even multiple of the grid
   * dimensions. (By default IncludeBoundary is off.)
   */
  vtkSetMacro(IncludeBoundary, vtkTypeBool);
  vtkGetMacro(IncludeBoundary, vtkTypeBool);
  vtkBooleanMacro(IncludeBoundary, vtkTypeBool);
  ///@}

protected:
  vtkExtractGrid();
  ~vtkExtractGrid() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Implementation for RequestData using a specified VOI. This is because the
   * parallel filter needs to muck around with the VOI to get spacing and
   * partitioning to play nice. The VOI is calculated from the output
   * data object's extents in this implementation.
   */
  bool RequestDataImpl(vtkInformationVector** inputVector, vtkInformationVector* outputVector);

  int VOI[6];
  int SampleRate[3];
  vtkTypeBool IncludeBoundary;

  vtkExtractStructuredGridHelper* Internal;

private:
  vtkExtractGrid(const vtkExtractGrid&) = delete;
  void operator=(const vtkExtractGrid&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
