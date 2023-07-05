// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellSizeFilter
 * @brief   Computes cell sizes.
 *
 * Computes the cell sizes for all types of cells in VTK. For triangles,
 * quads, tets and hexes the static methods in vtkMeshQuality are used.
 * This is done through Verdict for higher accuracy.
 * Other cell types are individually done analytically where possible
 * and breaking into triangles or tets when not possible. When cells are
 * broken into triangles or tets the accuracy may be diminished. By default
 * all sizes are computed but vertex count, length, area and volumetric cells
 * can each be optionally ignored. Individual arrays are used for each
 * requested size (e.g. if length and volume are requested there will be
 * two arrays outputted from this filter). The 4 arrays can be individually
 * named with defaults of VertexCount, Length, Area and Volme. For dimensions
 * of cells that do not have their size computed, a value of 0 will be given.
 * For cells that should have their size computed but can't, the filter will return -1.
 * The ComputeSum option will sum the cell sizes (excluding ghost cells)
 * and put the value into vtkFieldData arrays named with the corresponding cell
 * data array name. For composite datasets the total sum over all blocks will
 * also be added to the top-level block's field data for the summation.
 */

#ifndef vtkCellSizeFilter_h
#define vtkCellSizeFilter_h

#include "vtkFiltersVerdictModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkDoubleArray;
class vtkIdList;
class vtkImageData;
class vtkPointSet;

class VTKFILTERSVERDICT_EXPORT vtkCellSizeFilter : public vtkPassInputTypeAlgorithm
{
public:
  vtkTypeMacro(vtkCellSizeFilter, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkCellSizeFilter* New();

  ///@{
  /**
   * Specify whether or not to compute sizes for vertex and polyvertex
   * cells. The computed value is the number of points in the cell.
   * This option is enabled by default.
   */
  vtkSetMacro(ComputeVertexCount, bool);
  vtkGetMacro(ComputeVertexCount, bool);
  vtkBooleanMacro(ComputeVertexCount, bool);
  ///@}

  ///@{
  /**
   * Specify whether or not to compute sizes for 1D cells
   * cells. The computed value is the length of the cell.
   * This option is enabled by default.
   */
  vtkSetMacro(ComputeLength, bool);
  vtkGetMacro(ComputeLength, bool);
  vtkBooleanMacro(ComputeLength, bool);
  ///@}

  ///@{
  /**
   * Specify whether or not to compute sizes for 2D cells
   * cells. The computed value is the area of the cell.
   * This option is enabled by default.
   */
  vtkSetMacro(ComputeArea, bool);
  vtkGetMacro(ComputeArea, bool);
  vtkBooleanMacro(ComputeArea, bool);
  ///@}

  ///@{
  /**
   * Specify whether or not to compute sizes for 3D cells
   * cells. The computed value is the volume of the cell.
   * This option is enabled by default.
   */
  vtkSetMacro(ComputeVolume, bool);
  vtkGetMacro(ComputeVolume, bool);
  vtkBooleanMacro(ComputeVolume, bool);
  ///@}

  ///@{
  /**
   * Specify whether to sum the computed sizes and put the result in
   * a field data array. This option is disabled by default.
   */
  vtkSetMacro(ComputeSum, bool);
  vtkGetMacro(ComputeSum, bool);
  vtkBooleanMacro(ComputeSum, bool);
  ///@}

  ///@{
  /**
   * Set/Get the name of the computed arrays. Default names are VertexCount,
   * Length, Area and Volume.
   */
  vtkSetStringMacro(VertexCountArrayName);
  vtkGetStringMacro(VertexCountArrayName);
  vtkSetStringMacro(LengthArrayName);
  vtkGetStringMacro(LengthArrayName);
  vtkSetStringMacro(AreaArrayName);
  vtkGetStringMacro(AreaArrayName);
  vtkSetStringMacro(VolumeArrayName);
  vtkGetStringMacro(VolumeArrayName);
  ///@}

protected:
  vtkCellSizeFilter();
  ~vtkCellSizeFilter() override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  bool ComputeDataSet(vtkDataSet* input, vtkDataSet* output, double sum[4]);

  void IntegrateImageData(vtkImageData* input, vtkImageData* output, double sum[4]);
  void ExecuteBlock(vtkDataSet* input, vtkDataSet* output, double sum[4]);

  ///@{
  /**
   * Specify whether to sum the computed sizes and put the result in
   * a field data array. This option is disabled by default.
   */
  double IntegratePolyLine(vtkDataSet* input, vtkIdList* cellPtIds);
  double IntegratePolygon(vtkPointSet* input, vtkIdList* cellPtIds);
  double IntegrateTriangleStrip(vtkPointSet* input, vtkIdList* cellPtIds);
  double IntegratePixel(vtkDataSet* input, vtkIdList* cellPtIds);
  double IntegrateVoxel(vtkDataSet* input, vtkIdList* cellPtIds);
  double IntegrateGeneral1DCell(vtkDataSet* input, vtkIdList* cellPtIds);
  double IntegrateGeneral2DCell(vtkPointSet* input, vtkIdList* cellPtIds);
  double IntegrateGeneral3DCell(vtkPointSet* input, vtkIdList* cellPtIds);
  ///@}

  ///@{
  /**
   * Method to add the computed sum to the field data of the data object.
   */
  void AddSumFieldData(vtkDataObject*, double sum[4]);
  ///@}

  ///@{
  /**
   * Method to compute the global sum information. For serial operation this is a no-op.
   */
  virtual void ComputeGlobalSum(double sum[4]) { (void)sum; }
  ///@}

private:
  vtkCellSizeFilter(const vtkCellSizeFilter&) = delete;
  void operator=(const vtkCellSizeFilter&) = delete;

  bool ComputeVertexCount;
  bool ComputeLength;
  bool ComputeArea;
  bool ComputeVolume;
  bool ComputeSum;

  char* VertexCountArrayName;
  char* LengthArrayName;
  char* AreaArrayName;
  char* VolumeArrayName;
};

VTK_ABI_NAMESPACE_END
#endif
