// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPointSetToOctreeImageFilter
 * @brief   convert a point set to an octree image
 *
 * vtkPointSetToOctreeImageFilter is a filter that converts a vtkPointSet to an
 * a vtkPartitionedDataset with one vtkImageData with a number of points per cell target.
 *
 * The reason we output a vtkPartitionedDataset is because the WHOLE_EXTENT needs to be dynamic.
 *
 * The scalars of the vtkImageData are an octree unsigned char cell data array. Each bit of the
 * unsigned char indicates if the point-set had a point close to one of the 8 corners of the cell.
 *
 * It can optionally also output a cell data array based on an input point-data scalar array by
 * setting SetInputArrayToProcess. This array will have 1 or many components that represent
 * different functions i.e. last value, min, max, count, sum, mean.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa vtkOctreeImageToPointSetFilter
 */
#ifndef vtkPointSetToOctreeImageFilter_h
#define vtkPointSetToOctreeImageFilter_h

#include "vtkFiltersGeometryPreviewModule.h" // For export macro
#include "vtkPartitionedDataSetAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGEOMETRYPREVIEW_EXPORT vtkPointSetToOctreeImageFilter
  : public vtkPartitionedDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkPointSetToOctreeImageFilter, vtkPartitionedDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkPointSetToOctreeImageFilter* New();

  ///@{
  /**
   * Specify the average number of points in each cell of the output image.
   * This data member is used to determine the number dimensions of the output image.
   *
   * The default is 1.
   */
  vtkSetClampMacro(NumberOfPointsPerCell, int, 1, VTK_INT_MAX);
  vtkGetMacro(NumberOfPointsPerCell, int);
  ///@}

  ///@{
  /**
   * Set/Get if array defined using SetInputArrayToProcess, which MUST be a point data array, will
   * be processed.
   *
   * The default is off.
   */
  vtkSetMacro(ProcessInputPointArray, bool);
  vtkGetMacro(ProcessInputPointArray, bool);
  vtkBooleanMacro(ProcessInputPointArray, bool);
  ///@}

  ///@{
  /**
   * Set/Get if the last value for each cell id of the point data array will be computed.
   *
   * The default is false.
   *
   * Note: Because multithreading is employed the last value computation is not deterministic.
   */
  vtkSetMacro(ComputeLastValue, bool);
  vtkGetMacro(ComputeLastValue, bool);
  vtkBooleanMacro(ComputeLastValue, bool);
  ///@}

  ///@{
  /**
   * Set/Get if the min value for each cell id of the point data array will be computed.
   *
   * The default is false.
   */
  vtkSetMacro(ComputeMin, bool);
  vtkGetMacro(ComputeMin, bool);
  vtkBooleanMacro(ComputeMin, bool);
  ///@}

  ///@{
  /**
   * Set/Get if the max value for each cell id of the point data array will be computed.
   *
   * The default is false.
   */
  vtkSetMacro(ComputeMax, bool);
  vtkGetMacro(ComputeMax, bool);
  vtkBooleanMacro(ComputeMax, bool);
  ///@}

  ///@{
  /**
   * Set/Get if the count of the values for each cell id of the point data array will be computed.
   *
   * The default is false.
   */
  vtkSetMacro(ComputeCount, bool);
  vtkGetMacro(ComputeCount, bool);
  vtkBooleanMacro(ComputeCount, bool);
  ///@}

  ///@{
  /**
   * Set/Get if the sum of the values for each cell id of the point data array will be computed.
   *
   * The default is false.
   */
  vtkSetMacro(ComputeSum, bool);
  vtkGetMacro(ComputeSum, bool);
  vtkBooleanMacro(ComputeSum, bool);
  ///@}

  ///@{
  /**
   * Set/Get if the mean value for each cell id of the point data array will be computed.
   *
   * The default is false.
   *
   * Note: if ComputeMean is true, the sum and count will be computed regardless if they are on or
   * not.
   */
  vtkSetMacro(ComputeMean, bool);
  vtkGetMacro(ComputeMean, bool);
  vtkBooleanMacro(ComputeMean, bool);
  ///@}

protected:
  vtkPointSetToOctreeImageFilter();
  ~vtkPointSetToOctreeImageFilter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPointSetToOctreeImageFilter(const vtkPointSetToOctreeImageFilter&) = delete;
  void operator=(const vtkPointSetToOctreeImageFilter&) = delete;

  vtkIdType NumberOfPointsPerCell = 1;
  bool ProcessInputPointArray = false;
  bool ComputeLastValue = true;
  bool ComputeMin = false;
  bool ComputeMax = false;
  bool ComputeCount = false;
  bool ComputeSum = false;
  bool ComputeMean = false;

  enum class FieldFunctions
  {
    LAST_VALUE,
    MIN,
    MAX,
    COUNT,
    SUM,
    MEAN
  };

  template <typename TPointsArray>
  struct PointSetToImageFunctor;
  struct PointSetToImageWorker;
};

VTK_ABI_NAMESPACE_END
#endif // vtkPointSetToOctreeImageFilter_h
