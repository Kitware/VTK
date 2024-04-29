// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkEuclideanClusterExtraction
 * @brief   perform segmentation based on geometric
 * proximity and optional scalar threshold
 *
 * vtkEuclideanClusterExtraction is a filter that extracts points that are in
 * close geometric proximity, and optionally satisfies a scalar threshold
 * criterion. (Points extracted in this way are referred to as clusters.)
 * The filter works in one of five ways: 1) extract the largest cluster in the
 * dataset; 2) extract specified cluster number(s); 3) extract all clusters
 * containing specified point ids; 4) extract the cluster closest to a specified
 * point; or 5) extract all clusters (which can be used for coloring the clusters).
 *
 * Note that geometric proximity is defined by setting the Radius instance
 * variable. This variable defines a local sphere around each point; other
 * points contained in this sphere are considered "connected" to the
 * point. Setting this number too large will connect clusters that should not
 * be; setting it too small will fragment the point cloud into myriad
 * clusters. To accelerate the geometric proximity operations, a point
 * locator may be specified. By default, a vtkStaticPointLocator is used, but
 * any vtkAbstractPointLocator may be specified.
 *
 * The behavior of vtkEuclideanClusterExtraction can be modified by turning
 * on the boolean ivar ScalarConnectivity. If this flag is on, the clustering
 * algorithm is modified so that points are considered part of a cluster if
 * they satisfy both the geometric proximity measure, and the points scalar
 * values falls into the scalar range specified. This use of
 * ScalarConnectivity is particularly useful for data with intensity or color
 * information, serving as a simple "connected segmentation" algorithm. For
 * example, by using a seed point in a known cluster, clustering will pull
 * out all points "representing" the local structure.
 *
 * @sa
 * vtkConnectivityFilter vtkPolyDataConnectivityFilter
 */

#ifndef vtkEuclideanClusterExtraction_h
#define vtkEuclideanClusterExtraction_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#define VTK_EXTRACT_POINT_SEEDED_CLUSTERS 1
#define VTK_EXTRACT_SPECIFIED_CLUSTERS 2
#define VTK_EXTRACT_LARGEST_CLUSTER 3
#define VTK_EXTRACT_ALL_CLUSTERS 4
#define VTK_EXTRACT_CLOSEST_POINT_CLUSTER 5

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkFloatArray;
class vtkIdList;
class vtkIdTypeArray;
class vtkAbstractPointLocator;

class VTKFILTERSPOINTS_EXPORT vtkEuclideanClusterExtraction : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkEuclideanClusterExtraction, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct with default extraction mode to extract largest clusters.
   */
  static vtkEuclideanClusterExtraction* New();

  ///@{
  /**
   * Specify the local search radius.
   */
  vtkSetClampMacro(Radius, double, 0.0, VTK_FLOAT_MAX);
  vtkGetMacro(Radius, double);
  ///@}

  ///@{
  /**
   * Turn on/off connectivity based on scalar value. If on, points are
   * connected only if the are proximal AND the scalar value of a candidate
   * point falls in the scalar range specified. Of course input point scalar
   * data must be provided.
   */
  vtkSetMacro(ScalarConnectivity, bool);
  vtkGetMacro(ScalarConnectivity, bool);
  vtkBooleanMacro(ScalarConnectivity, bool);
  ///@}

  ///@{
  /**
   * Set the scalar range used to extract points based on scalar connectivity.
   */
  vtkSetVector2Macro(ScalarRange, double);
  vtkGetVector2Macro(ScalarRange, double);
  ///@}

  ///@{
  /**
   * Control the extraction of connected surfaces.
   */
  vtkSetClampMacro(
    ExtractionMode, int, VTK_EXTRACT_POINT_SEEDED_CLUSTERS, VTK_EXTRACT_CLOSEST_POINT_CLUSTER);
  vtkGetMacro(ExtractionMode, int);
  void SetExtractionModeToPointSeededClusters()
  {
    this->SetExtractionMode(VTK_EXTRACT_POINT_SEEDED_CLUSTERS);
  }
  void SetExtractionModeToLargestCluster() { this->SetExtractionMode(VTK_EXTRACT_LARGEST_CLUSTER); }
  void SetExtractionModeToSpecifiedClusters()
  {
    this->SetExtractionMode(VTK_EXTRACT_SPECIFIED_CLUSTERS);
  }
  void SetExtractionModeToClosestPointCluster()
  {
    this->SetExtractionMode(VTK_EXTRACT_CLOSEST_POINT_CLUSTER);
  }
  void SetExtractionModeToAllClusters() { this->SetExtractionMode(VTK_EXTRACT_ALL_CLUSTERS); }
  const char* GetExtractionModeAsString();
  ///@}

  /**
   * Initialize the list of point ids used to seed clusters.
   */
  void InitializeSeedList();

  /**
   * Add a seed id (point id). Note: ids are 0-offset.
   */
  void AddSeed(vtkIdType id);

  /**
   * Delete a seed id.a
   */
  void DeleteSeed(vtkIdType id);

  /**
   * Initialize the list of cluster ids to extract.
   */
  void InitializeSpecifiedClusterList();

  /**
   * Add a cluster id to extract. Note: ids are 0-offset.
   */
  void AddSpecifiedCluster(int id);

  /**
   * Delete a cluster id to extract.
   */
  void DeleteSpecifiedCluster(int id);

  ///@{
  /**
   * Used to specify the x-y-z point coordinates when extracting the cluster
   * closest to a specified point.
   */
  vtkSetVector3Macro(ClosestPoint, double);
  vtkGetVectorMacro(ClosestPoint, double, 3);
  ///@}

  /**
   * Obtain the number of connected clusters. This value is valid only after filter execution.
   */
  int GetNumberOfExtractedClusters();

  ///@{
  /**
   * Turn on/off the coloring of connected clusters.
   */
  vtkSetMacro(ColorClusters, bool);
  vtkGetMacro(ColorClusters, bool);
  vtkBooleanMacro(ColorClusters, bool);
  ///@}

  ///@{
  /**
   * Specify a point locator. By default a vtkStaticPointLocator is
   * used. The locator performs efficient proximity searches near a
   * specified interpolation position.
   */
  void SetLocator(vtkAbstractPointLocator* locator);
  vtkGetObjectMacro(Locator, vtkAbstractPointLocator);
  ///@}

protected:
  vtkEuclideanClusterExtraction();
  ~vtkEuclideanClusterExtraction() override;

  double Radius;                  // connection radius
  bool ColorClusters;             // boolean turns on/off scalar gen for separate clusters
  int ExtractionMode;             // how to extract clusters
  vtkIdList* Seeds;               // id's of points or cells used to seed clusters
  vtkIdList* SpecifiedClusterIds; // clusters specified for extraction
  vtkIdTypeArray* ClusterSizes;   // size (in cells) of each cluster extracted

  double ClosestPoint[3];

  bool ScalarConnectivity;
  double ScalarRange[2];

  vtkAbstractPointLocator* Locator;

  // Configure the pipeline
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  // Internal method for propagating connected waves.
  void InsertIntoWave(vtkIdList* wave, vtkIdType ptId);
  void TraverseAndMark(vtkPoints* pts);

private:
  vtkEuclideanClusterExtraction(const vtkEuclideanClusterExtraction&) = delete;
  void operator=(const vtkEuclideanClusterExtraction&) = delete;

  // used to support algorithm execution
  vtkFloatArray* NeighborScalars;
  vtkIdList* NeighborPointIds;
  char* Visited;
  vtkIdType* PointMap;
  vtkIdTypeArray* NewScalars;
  vtkIdType ClusterNumber;
  vtkIdType PointNumber;
  vtkIdType NumPointsInCluster;
  vtkDataArray* InScalars;
  vtkIdList* Wave;
  vtkIdList* Wave2;
  vtkIdList* PointIds;
};

/**
 * Return the method of extraction as a string.
 */
inline const char* vtkEuclideanClusterExtraction::GetExtractionModeAsString()
{
  if (this->ExtractionMode == VTK_EXTRACT_POINT_SEEDED_CLUSTERS)
  {
    return "ExtractPointSeededClusters";
  }
  else if (this->ExtractionMode == VTK_EXTRACT_SPECIFIED_CLUSTERS)
  {
    return "ExtractSpecifiedClusters";
  }
  else if (this->ExtractionMode == VTK_EXTRACT_ALL_CLUSTERS)
  {
    return "ExtractAllClusters";
  }
  else if (this->ExtractionMode == VTK_EXTRACT_CLOSEST_POINT_CLUSTER)
  {
    return "ExtractClosestPointCluster";
  }
  else
  {
    return "ExtractLargestCluster";
  }
}

VTK_ABI_NAMESPACE_END
#endif
