// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkFillPointCloud
 * @brief   add points to empty regions in an input point cloud
 *
 * vtkFillPointCloud adds points to an existing point cloud (i.e., instance
 * of vtkPointSet), placing points in regions where no input points
 * exist. The fill operation preserves the input points (and any region
 * labels if available), and adds new points with a specified background
 * label.  The output of the filter is a dataset of type vtkPolyData.
 *
 * This utility class may be used improve the performance of Voronoi and
 * Delaunay algorithms, by removing undesirable regions of the tessellation
 * (carve out unimportant space).
 *
 * The MaximumNumberOfPoints data member is used to control the density of
 * the fill operation. Basically, this resolution-related data member
 * affects the binning resolution of an underlying point
 * locator. (Optionally, the resolution of the point locator can be better
 * controlled by using manual access and direct modification of the point
 * locator.)
 *
 * The algorithm provides two strategies to filling the point cloud: 1)
 * uniform binning, and 2) adaptive binning. In both cases, the algorithm
 * starts by binning the input point cloud with a vtkStaticPointLocator
 * class. In a uniform strategy, new points are added to any locator bins not
 * containing any points. The placement of the new point is in the center of
 * the empty bin, with an optional random joggle. In an adaptive approach,
 * the locator is converted to a 3D image with voxel values indicating bin
 * occupancy or not. Then, an instance of vtkLabeledImagePointSampler is used
 * to produce the new / background points. (Direct access to the internal
 * instance of vtkLabeledImagePointSampler is provided in case greater
 * control is desired.)
 *
 * All newly added points are assigned an integral region label id (the
 * BackgroundLabel). Input points, if they are not labeled with input region
 * ids, are assigned an InLabel. Typically BackgroundLabel <0, and
 * InLabel==0.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkVoronoiFlower2D vtkVoronoiFlower3D vtkGeneralizedSurfaceNets3D
 * vtkDelunay2D vtkDelaunay3D vtkStaticPointLocator vtkLabeledImagePointSampler
 * vtkJogglePoints
 */

#ifndef vtkFillPointCloud_h
#define vtkFillPointCloud_h

#include "vtkFiltersMeshingModule.h"     // For export macro
#include "vtkLabeledImagePointSampler.h" // For adaptive sampling
#include "vtkPolyDataAlgorithm.h"
#include "vtkSmartPointer.h"       // For vtkSmartPointer
#include "vtkStaticPointLocator.h" // For point locator

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSMESHING_EXPORT vtkFillPointCloud : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing information.
   */
  static vtkFillPointCloud* New();
  vtkTypeMacro(vtkFillPointCloud, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Specify the fill strategy to use when generating new points.
   * The uniform simply adds new points to empty bins. Adaptive
   * strategy uses an instance of vtkLabeledImagePointSampler to
   * create exponential or interval distributions of added points.
   */
  enum FillStrategyOptions
  {
    UNIFORM = 0,
    ADAPTIVE = 1
  };
  vtkSetClampMacro(FillStrategy, int, UNIFORM, ADAPTIVE);
  vtkGetMacro(FillStrategy, int);
  void SetFillStrategyToUniform() { this->SetFillStrategy(UNIFORM); }
  void SetFillStrategyToAdaptive() { this->SetFillStrategy(ADAPTIVE); }
  ///@}

  ///@{
  /**
   * Specify the in region label (i.e., apply a label to the existing
   * input points if "Regions Ids" input labels are not provided).
   * By default, the in label is set to zero; if Regions Ids are provided
   * as input, then the in label is not used.
   */
  vtkSetMacro(InLabel, int);
  vtkGetMacro(InLabel, int);
  ///@}

  ///@{
  /**
   * Specify the fill (or background) label (i.e., the segmented region label
   * for newly added points outside of the existing point cloud). By default,
   * it is set to a negative integral number that typically denotes the point
   * is outside of the input point cloud.
   */
  vtkSetMacro(BackgroundLabel, int);
  vtkGetMacro(BackgroundLabel, int);
  ///@}

  ///@{
  /**
   * Specify the maximum number of points that can be added to fill the point
   * cloud.  This is in effect a way to control the resolution of the
   * resulting combined and filled point cloud. Note that this resolution is
   * not a continuous measure; it controls the resolution of the underlying
   * locator which adjusts in a discrete fashion.
   */
  vtkSetClampMacro(MaximumNumberOfPoints, int, 1, VTK_INT_MAX);
  vtkGetMacro(MaximumNumberOfPoints, int);
  ///@}

  ///@{
  /**
   * Indicate whether manual control of the underlying locator is desired.
   * Enabling this data member means that the MaximumNumberOfPoints()
   * method  has no effect and the user directly controls the underlying
   * locator. By default, maual control is disabled.
   */
  vtkSetMacro(ManualLocatorControl, vtkTypeBool);
  vtkGetMacro(ManualLocatorControl, vtkTypeBool);
  vtkBooleanMacro(ManualLocatorControl, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Retrieve the internal locator to manually configure it, for example
   * specifying the number of points per bucket, or specifying the bin
   * resolution.
   */
  vtkStaticPointLocator* GetLocator() { return this->Locator; }
  ///@}

  ///@{
  /**
   * Enable/disable point joggling. By default, the joggle radius is a
   * fraction of the shortest x-y-z edge length of vtkStaticPointLocator
   * bins; alternatively an absolute radius can be specified. By default,
   * joggling is enabled with a relative joggle radius. (Note that these
   * are convenience methods that forward to the internal instance of
   * vtkLabeledImagePointSampler.)
   */
  void SetJoggle(vtkTypeBool onoff)
  {
    this->PointSampler->SetJoggle(onoff);
    this->UpdateJoggleInfo();
  }
  vtkTypeBool GetJoggle() const { return this->PointSampler->GetJoggle(); }
  void JoggleOn()
  {
    this->PointSampler->JoggleOn();
    this->UpdateJoggleInfo();
  }
  void JoggleOff()
  {
    this->PointSampler->JoggleOff();
    this->UpdateJoggleInfo();
  }

  void SetJoggleRadiusIsAbsolute(vtkTypeBool onoff)
  {
    this->PointSampler->SetJoggleRadiusIsAbsolute(onoff);
    this->UpdateJoggleInfo();
  }
  vtkTypeBool GetJoggleRadiusIsAbsolute() const
  {
    return this->PointSampler->GetJoggleRadiusIsAbsolute();
  }
  void JoggleRadiusIsAbsoluteOn()
  {
    this->PointSampler->JoggleRadiusIsAbsoluteOn();
    this->UpdateJoggleInfo();
  }
  void JoggleRadiusIsAbsoluteOff()
  {
    this->PointSampler->JoggleRadiusIsAbsoluteOff();
    this->UpdateJoggleInfo();
  }

  void SetJoggleRadius(double r)
  {
    this->PointSampler->SetJoggleRadius(r);
    this->UpdateJoggleInfo();
  }
  double GetJoggleRadius() const { return this->PointSampler->GetJoggleRadius(); }
  ///@}

  ///@{
  /**
   * Retrieve the internal instance of vtkLabeledImagePointSampler in
   * order to better control the adaptive fill process.
   */
  vtkLabeledImagePointSampler* GetPointSampler() { return this->PointSampler; }
  ///@}

  /**
   * Get the number of points added. This method only returns useful
   * information after the filter executes.
   */
  vtkIdType GetNumberOfAddedPoints() { return this->NumberOfAddedPoints; }

  /**
   * Get the MTime of this object also considering the locator and
   * point sampler.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkFillPointCloud();
  ~vtkFillPointCloud() override;

  int FillStrategy;
  int InLabel;
  int BackgroundLabel;
  int MaximumNumberOfPoints;

  // These data members are maintained by the internal point sampler.
  vtkTypeBool Joggle;
  double JoggleRadius;
  vtkTypeBool JoggleRadiusIsAbsolute;

  vtkTypeBool ManualLocatorControl;
  vtkSmartPointer<vtkStaticPointLocator> Locator; // locator for finding proximal points

  vtkSmartPointer<vtkLabeledImagePointSampler> PointSampler;

  vtkIdType NumberOfAddedPoints;

  // Internal helper methods
  void UpdateJoggleInfo();

  vtkPoints* CreatePointsAndRegions(vtkPointSet* input, vtkInformationVector** inputVector,
    vtkIdType numNewPts, vtkPolyData* output);

  // Support second input
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkFillPointCloud(const vtkFillPointCloud&) = delete;
  void operator=(const vtkFillPointCloud&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
