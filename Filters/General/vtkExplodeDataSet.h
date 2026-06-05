// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkExplodeDataSet
 * @brief   Explode input dataset according to a cell scalar isovalue criteria.
 *
 * vtkExplodeDataSet creates a vtkPartitionedDataSetCollection containing
 * isovalue partitions: each partition of the output contains cells that share
 * the same value for the given cell array.
 *
 * The output is a vtkPartitionedDataSetCollection where leaves are vtkUnstructuredGrid,
 * unless input is a vtkPolyData, then leaves stay vtkPolyData.
 *
 * You can call NamePartitionsFromFieldDataOn() to name the parts
 * after a vtkStringArray, provided by SetPartitionNamesArray.
 * To customize the name lookup from the scalar values, you can also provide vtkDataArray
 * name through SetPartitionValuesArray. Without it, a linear integral array is assumed.
 * (default is Off)
 *
 * PointData and CellData are forwarded accordingly.
 *
 * It is similar to a vtkMultiThreshold, configured to extract every single
 * value of the input array.
 *
 * It comes in replacement of vtkSplitByCellScalarFilter to use
 * vtkPartitionedDataSetCollection instead of vtkMultiBlockDataSet as output.
 * Also, vtkExplodeDataSet relies internally on vtkExtractCells to generate
 * output datasets. This benefits from SMPTools threading acceleration.
 *
 * @sa vtkExtractCells, vtkMultiThreshold
 */

#ifndef vtkExplodeDataSet_h
#define vtkExplodeDataSet_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPartitionedDataSetCollectionAlgorithm.h"

#include <map> // for std::map

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkDataSet;
class vtkIdList;
class vtkPointSet;
class vtkStringArray;

class VTKFILTERSGENERAL_EXPORT vtkExplodeDataSet : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  static vtkExplodeDataSet* New();
  vtkTypeMacro(vtkExplodeDataSet, vtkPartitionedDataSetCollectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the use of a FieldData array to name the output partitions.
   *
   * When false, the name is created from the Scalar name in use and an
   * incremental suffix.
   * Default is false.
   *
   * @see SetPartitionNamesArray.
   */
  vtkSetMacro(UsePartitionNamesFromFieldData, bool);
  vtkGetMacro(UsePartitionNamesFromFieldData, bool);
  vtkBooleanMacro(UsePartitionNamesFromFieldData, bool);
  ///@}

  ///@{
  /**
   * Set/Get the name of the FieldData array to use to name the output partitions.
   *
   * Unused if UsePartitionNamesFromFieldData is false.
   * The underlying array should be a vtkStringArray.
   *
   * @see SetNamePartitionsFromFieldData(), SetPartitionValuesArray()
   */
  vtkSetMacro(PartitionNamesArray, std::string);
  vtkGetMacro(PartitionNamesArray, std::string);
  ///@}

  ///@{
  /**
   * Set/Get the name of the FieldData array to use to name the output partitions.
   *
   * Unused if UsePartitionNamesArray is false.
   * The underlying array should be a vtkDataArray subclass.
   * If empty, a linear integral array is assumed.
   *
   * @see SetNamePartitionsFromFieldData(), SetPartitionNamesArray()
   */
  vtkSetMacro(PartitionValuesArray, std::string);
  vtkGetMacro(PartitionValuesArray, std::string);
  ///@}

protected:
  vtkExplodeDataSet();
  ~vtkExplodeDataSet() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Accepts vtkDataSet as input.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkExplodeDataSet(const vtkExplodeDataSet&) = delete;
  void operator=(const vtkExplodeDataSet&) = delete;

  /**
   * Fill the idsByValue map: for each value in scalars, list all of input cells that
   * matches this value.
   */
  std::map<double, vtkSmartPointer<vtkIdList>> GetIdListsByValue(vtkDataArray* scalars);

  /**
   * Create a partition from input cells and given points.
   * If input is a vtkPolyData, returns a vtkPolyData. Otherwise returns a vtkUnstructuredGrid.
   * Caller is responsible for deletion.
   */
  vtkPointSet* CreatePartition(vtkDataSet* input, vtkIdList* partCellIds);

  /**
   * Return a name for the given partition.
   * If value is in providedNames, use it.
   * By default, create one from basename and id.
   */
  std::string GetPartitionName(vtkIdType id, const std::map<double, std::string>& providedNames,
    double value, std::string basename);

  /**
   * Get the provided names in a structure easier to access.
   * If NamePartitionFromFieldData is true, maps value from PartitionValuesArray to name of same
   * index in PartitionNamesArray.
   * Returns an empty map otherwise.
   */
  std::map<double, std::string> GetProvidedNames(vtkDataSet* input);

  /**
   * Return the array containing the names to use for output part.
   *
   * It looks in the FieldData for a vtkStringArray named according to PartitionNamesArray.
   * Return nullptr and warn if no such array.
   */
  vtkStringArray* GetNamesArray(vtkDataSet* input);

  /**
   * Return the array containing the values associated to each name of GetNamesArray().
   *
   * It looks in the FieldData for a vtkDataArray named according to PartitionValuesArray.
   * If no such array, generate a linear integral array starting at 0.
   */
  vtkSmartPointer<vtkDataArray> GetValuesArray(vtkDataSet* input, vtkIdType expectedSize);

  bool UsePartitionNamesFromFieldData = false;
  std::string PartitionNamesArray;
  std::string PartitionValuesArray;
};

VTK_ABI_NAMESPACE_END
#endif
