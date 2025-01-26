// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkResampleWithDataSet
 * @brief   sample point and cell data of a dataset on
 * points from another dataset.
 *
 * Similar to vtkCompositeDataProbeFilter, vtkResampleWithDataSet takes two
 * inputs - Input and Source, and samples the point and cell values of Source
 * on to the point locations of Input. The output has the same structure as
 * Input but its point data have the resampled values from Source. Unlike
 * vtkCompositeDataProbeFilter, this filter support composite datasets for both
 * Input and Source.
 * @sa
 * vtkCompositeDataProbeFilter vtkResampleToImage
 */

#ifndef vtkResampleWithDataSet_h
#define vtkResampleWithDataSet_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkNew.h"               // For vtkCompositeDataProbeFilter member variable
#include "vtkPassInputTypeAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractCellLocator;
class vtkCompositeDataProbeFilter;
class vtkDataSet;

class VTKFILTERSCORE_EXPORT vtkResampleWithDataSet : public vtkPassInputTypeAlgorithm
{
public:
  vtkTypeMacro(vtkResampleWithDataSet, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkResampleWithDataSet* New();

  /**
   * Specify the data set that will be probed at the input points.
   * The Input gives the geometry (the points and cells) for the output,
   * while the Source is probed (interpolated) to generate the scalars,
   * vectors, etc. for the output points based on the point locations.
   */
  void SetSourceData(vtkDataObject* source);

  /**
   * Specify the data set that will be probed at the input points.
   * The Input gives the geometry (the points and cells) for the output,
   * while the Source is probed (interpolated) to generate the scalars,
   * vectors, etc. for the output points based on the point locations.
   */
  void SetSourceConnection(vtkAlgorithmOutput* algOutput);

  ///@{
  /**
   * Control whether the source point data is to be treated as categorical. If
   * the data is categorical, then the resultant data will be determined by
   * a nearest neighbor interpolation scheme.
   */
  void SetCategoricalData(bool arg);
  bool GetCategoricalData();
  ///@}

  ///@{
  /**
   * Shallow copy the input cell data arrays to the output.
   * Off by default.
   */
  void SetPassCellArrays(bool arg);
  bool GetPassCellArrays();
  vtkBooleanMacro(PassCellArrays, bool);
  ///@}

  ///@{
  /**
   * Shallow copy the input point data arrays to the output
   * Off by default.
   */
  void SetPassPointArrays(bool arg);
  bool GetPassPointArrays();
  vtkBooleanMacro(PassPointArrays, bool);
  ///@}

  ///@{
  /**
   * Set whether to pass the field-data arrays from the Input i.e. the input
   * providing the geometry to the output. On by default.
   */
  void SetPassFieldArrays(bool arg);
  bool GetPassFieldArrays();
  vtkBooleanMacro(PassFieldArrays, bool);
  ///@}

  ///@{
  /**
   * When sampling from composite datasets, partial arrays are common i.e.
   * data-arrays that are not available in all of the blocks. By default, this
   * filter only passes those point and cell data-arrays that are available in
   * all the blocks i.e. partial arrays are removed.  When PassPartialArrays is
   * turned on, this behavior is changed to take a union of all arrays present
   * thus partial arrays are passed as well. However, for composite dataset
   * input, this filter still produces a non-composite output. For all those
   * locations in a block of where a particular data array is missing, this
   * filter uses vtkMath::Nan() for double and float arrays, and 0 for all
   * other types of arrays e.g. int, char, etc. Off by default.
   */
  void SetPassPartialArrays(bool arg);
  bool GetPassPartialArrays();
  vtkBooleanMacro(PassPartialArrays, bool);
  ///@}

  ///@{
  /**
   * Set the tolerance used to compute whether a point in the
   * source is in a cell of the input.  This value is only used
   * if ComputeTolerance is off.
   */
  void SetTolerance(double arg);
  double GetTolerance();
  ///@}

  ///@{
  /**
   * Set whether to use the Tolerance field or precompute the tolerance.
   * When on, the tolerance will be computed and the field
   * value is ignored. On by default.
   */
  void SetComputeTolerance(bool arg);
  bool GetComputeTolerance();
  vtkBooleanMacro(ComputeTolerance, bool);
  ///@}

  ///@{
  /**
   * Set whether points without resampled values, and their corresponding cells,
   * should be marked as Blank. Default is On.
   */
  vtkSetMacro(MarkBlankPointsAndCells, bool);
  vtkGetMacro(MarkBlankPointsAndCells, bool);
  vtkBooleanMacro(MarkBlankPointsAndCells, bool);
  ///@}

  ///@{
  /**
   * Set/Get whether to snap to the cell with the closest point, if no cell has been found while
   * FindCell is executed.
   *
   * Default is off.
   *
   * Note: This is useful only when the source is a vtkPointSet.
   */
  void SetSnapToCellWithClosestPoint(bool arg);
  bool GetSnapToCellWithClosestPoint();
  vtkBooleanMacro(SnapToCellWithClosestPoint, bool);
  ///@}

  ///@{
  /**
   * Get/Set whether or not the filter should use implicit arrays.
   * If set to true, probed values will not be copied to the output
   * but retrieved from the source through indexation (thanks to indexed arrays).
   * This can lower the memory consumption, especially if the probed source contains
   * a lot of data arrays. Note that it will also increase the computation time.
   *
   * @attention
   * This option only concern Hyper Tree Grids.
   * This option has no effect for source or blocks (in the case of a composite input)
   * that are not vtkHyperTreeGrid instances.
   */
  void SetUseImplicitArrays(bool arg);
  bool GetUseImplicitArrays();
  vtkBooleanMacro(UseImplicitArrays, bool);
  ///@}

  ///@{
  /*
   * Set/Get the prototype cell locator to use for probing the source dataset.
   * The value is forwarded to the underlying probe filter.
   */
  virtual void SetCellLocatorPrototype(vtkAbstractCellLocator*);
  virtual vtkAbstractCellLocator* GetCellLocatorPrototype() const;
  ///@}

  vtkMTimeType GetMTime() override;

protected:
  vtkResampleWithDataSet();
  ~vtkResampleWithDataSet() override;

  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;
  int FillOutputPortInformation(int, vtkInformation*) override;

  // Garbage collection method
  void ReportReferences(vtkGarbageCollector*) override;

  /**
   * Get the name of the valid-points mask array.
   */
  const char* GetMaskArrayName() const;

  /**
   * Mark invalid points and cells of output DataSet as hidden
   */
  void SetBlankPointsAndCells(vtkDataSet* data);

  vtkNew<vtkCompositeDataProbeFilter> Prober;
  bool MarkBlankPointsAndCells;

private:
  vtkResampleWithDataSet(const vtkResampleWithDataSet&) = delete;
  void operator=(const vtkResampleWithDataSet&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkResampleWithDataSet_h
