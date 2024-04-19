// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPointSetStreamer
 * @brief   stream points as buckets
 *
 * vtkPointSetStreamer is a filter that sorts points into buckets and it returns the points
 * included in the chosen bucket. The bucket is chosen by setting the BucketId. The purpose
 * of this class is to allow streaming of points. The bucket size is determined by the
 * NumberOfPointsPerBucket.
 *
 * The typical usage is to call this filter the first time to perform the sorting and get the points
 * in the first bucket and then to call it again to get the points in the remaining buckets. The
 * sorting is performed only the first time, assuming that the dataset or NumberOfPointsPerBucket
 * don't change. The number of buckets can be obtained by calling GetNumberOfBuckets.
 *
 * @sa vtkPointSetToOctreeImageFilter vtkStaticPointLocator
 */

#ifndef vtkPointSetStreamer_h
#define vtkPointSetStreamer_h

#include "vtkFiltersGeometryPreviewModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkStaticPointLocator;

class VTKFILTERSGEOMETRYPREVIEW_EXPORT vtkPointSetStreamer : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkPointSetStreamer, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkPointSetStreamer* New();

  ///@{
  /**
   * Set/Get the average number of points in each bucket.
   * This data member is used to determine the number of buckets.
   *
   * The default is 1.
   */
  vtkSetClampMacro(NumberOfPointsPerBucket, int, 1, VTK_INT_MAX);
  vtkGetMacro(NumberOfPointsPerBucket, int);
  ///@}

  ///@{
  /**
   * Set/Get the bucket id to stream.
   * This data member is used to determine the number of buckets.
   *
   * The default is 0.
   */
  vtkSetClampMacro(BucketId, vtkIdType, 0, VTK_ID_MAX);
  vtkGetMacro(BucketId, vtkIdType);
  ///@}

  /**
   * Get the number of buckets.
   *
   * Note: This method must be called after the first pass.
   */
  vtkGetMacro(NumberOfBuckets, vtkIdType);

  ///@{
  /**
   * Set/Get if a cell array of vertices will be created.
   *
   * The default is on.
   */
  vtkSetMacro(CreateVerticesCellArray, bool);
  vtkGetMacro(CreateVerticesCellArray, bool);
  vtkBooleanMacro(CreateVerticesCellArray, bool);
  ///@}
protected:
  vtkPointSetStreamer();
  ~vtkPointSetStreamer() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPointSetStreamer(const vtkPointSetStreamer&) = delete;
  void operator=(const vtkPointSetStreamer&) = delete;

  int NumberOfPointsPerBucket = 1;
  vtkIdType BucketId = 0;
  vtkIdType NumberOfBuckets = 0;
  bool CreateVerticesCellArray = true;

  vtkNew<vtkStaticPointLocator> PointLocator;
};

VTK_ABI_NAMESPACE_END
#endif // vtkPointSetStreamer_h
