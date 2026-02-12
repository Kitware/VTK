// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLabeledImagePointSampler
 * @brief   produce sampled points from a segmented image
 *
 * vtkLabeledImagePointSampler is a utility filter that produces points from
 * a labeled input 3D volume or 2D xy image. This can be used to reduce the
 * overall size of the data, and/or convert data (from a segmented image to
 * a sampled vtkPointSet) for filters like vtkVoronoiFlower2D/3D or
 * vtkGeneralizedSurfaceNets3D that require input points. The required input
 * image data is a integer region id point data array (i.e., the labels).
 * (The filter is akin to dividing cubes, which is a way to represent
 * contours with point clouds.) The output of the filter is represented by
 * the vtkPointSet subclass vtkPolyData containing a list of points, and a
 * point data array identifying the region from which each point is
 * generated.
 *
 * The filter produces points in an adaptive manner, more dense near
 * segmented region boundaries, and less dense moving away from region
 * boundaries. The process of adaption can be controlled by specifying either
 * a linear density distribution, choosing every Nth point, or an exponential
 * distribution, with power N (e.g., point density reduced as a power of 2).
 *
 * Segmented regions within the input image are identified by specifying a
 * set of one or more values/labels. Points not in any defined (labeled)
 * regions are called background points. As expected, labeled points are
 * generated from the input set of voxel/pixel labels.  Background points can
 * be given a "BackgroundPointLabel" which can be used by other filters for
 * thresholding, or for generating tessellations (e.g., Voronoi filters). (By
 * default, the BackgroundPointLabel is a large negative integral number.) By
 * default, both labeled and background points are output; optionally, only
 * points within labeled or background regions can be output.
 *
 * In some applications, it may be advantageous to randomly joggle
 * (alternatively: perturb, jitter, jiggle) the generated output
 * points. Because a image is being sampled, it can produce regularly spaced
 * points that can affect the performance and numerical stability of certain
 * filters like Voronoi and Delaunay.
 *
 * @note
 * The term "joggle" was first used in the context of Voronoi degeneracies by
 * the popular QuickHull algorithm (and the associated QHull implementation
 * qhull.org).
 *
 * @warning
 * If the input labels (region ids) are not of type int, they will be
 * converted to type int. This is because several important filters (e.g.,
 * vtkVoronoiFlower2D, vtkVoronoi3D, vtkGeneralizedSurfaceNets2D) currently
 * require int region labels as input. This could be extended using templates
 * at the cost of significant bloat, as long as the representational types
 * support both negative and positive values (e.g., unsigned will not work
 * correctly).
 *
 * @warning
 * This filter can produce large numbers of points. It is recommended that
 * the filter parameter N and density distribution function is chosen
 * carefully to manage the size of the output.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkVoronoiFlower2D vtkVoronoiFlower3D vtkGeneralizedSurfaceNets3D
 * vtkJogglePoints vtkFillPointCloud
 */

#ifndef vtkLabeledImagePointSampler_h
#define vtkLabeledImagePointSampler_h

#include "vtkContourValues.h"        // Manage countour values
#include "vtkFiltersMeshingModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;

class VTKFILTERSMESHING_EXPORT vtkLabeledImagePointSampler : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, obtaining type information, and printing
   * information.
   */
  static vtkLabeledImagePointSampler* New();
  vtkTypeMacro(vtkLabeledImagePointSampler, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * These methods are used to control how output region labels are assigned
   * to background points. When BackgroundPointMapping is enabled, points
   * with region labels not in the input list of segmented values are
   * assigned the BackgroundPointLabel. By default, BackgroundPointMapping is
   * off, so the input image labels are used. If however,
   * BackgroundPointMapping is on, then the BackgroundPointLabel is assigned
   * to unlabeled points. (Recall that background points have input
   * labels/values that do not exist in the specified set of
   * labels/values). By default, the BackgroundPointLabel is set to a
   * negative integral number that typically denotes the point is outside of
   * the segmented points. (Make sure that the background pt id is not a
   * value in the set of labels/values or havoc may ensue).
   */
  vtkSetMacro(BackgroundPointMapping, int);
  vtkGetMacro(BackgroundPointMapping, int);
  vtkBooleanMacro(BackgroundPointMapping, int);
  vtkSetMacro(BackgroundPointLabel, int);
  vtkGetMacro(BackgroundPointLabel, int);
  ///@}

  ///@{
  /**
   * Used to indicate the type of point density distribution. By default, an exponential
   * density distribution with N=2 is used.
   */
  enum DensityDistributionType
  {
    LINEAR = 0,
    EXPONENTIAL = 1
  };
  /**
   * Specify the type of point selection distribution (i.e., the specification
   * of the point density).
   */
  vtkSetClampMacro(DensityDistribution, int, LINEAR, EXPONENTIAL);
  vtkGetMacro(DensityDistribution, int);
  void SetDensityDistributionToLinear() { this->SetDensityDistribution(LINEAR); }
  void SetDensityDistributionToExponential() { this->SetDensityDistribution(EXPONENTIAL); }
  ///@}

  ///@{
  /**
   * Specify the parameter N which controls the generation of points, and the
   * resulting point density. In a linear distribution, N indicates every Nth
   * point; in an exponential distribution, N indicates is the exponent of
   * density variation. By default, N=2.
   */
  vtkSetClampMacro(N, unsigned int, 1, 100);
  vtkGetMacro(N, unsigned int);
  ///@}

  ///@{
  /**
   * The production of output points can be randomized. This can be useful
   * because the sampling of a regular grid (i.e., the volume) can output
   * points that are strongly coherent, which is undesirable in some
   * applications.  If randomize if enabled, the points are output with
   * probability 1/Dist, where Dist is the distance from a labeled region
   * boundary. The probability can be further modified by applying the
   * probability range, which has the effect of clamping the minimum and
   * maximum selection probabilities. By default, Randomize is true.
   */
  vtkSetMacro(Randomize, vtkTypeBool);
  vtkGetMacro(Randomize, vtkTypeBool);
  vtkBooleanMacro(Randomize, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Modifiy the probability of the point randomization process. The range
   * should be within [0,1], and it's effect is to clamp the minimum and
   * maximum selection probabilities. By default, the range is (0,1). These
   * default values mean that points directly adjacent to the region
   * boundaries have probability 1 of be selected; while distant points
   * approach a probability of zero to be selected (using the 1/Dist
   * randomization function mentioned previously).
   */
  vtkSetVector2Macro(RandomProbabilityRange, double);
  vtkGetVectorMacro(RandomProbabilityRange, double, 2);
  ///@}

  ///@{
  /**
   * Used to indicate which generated points to output.
   */
  enum OutputLabelsSelection
  {
    ALL_POINTS = 0,
    LABELED_POINTS = 1,
    BACKGROUND_POINTS = 2
  };
  /**
   * Indicate what points to output. By default, all points including both
   * labeled points, as well as background points, are produced on output.
   * However it is possible to output only points in the labeled regions,
   * or output points in background regions.
   */
  vtkSetClampMacro(OutputType, int, ALL_POINTS, BACKGROUND_POINTS);
  vtkGetMacro(OutputType, int);
  void SetOutputTypeToAllPoints() { this->SetOutputType(ALL_POINTS); }
  void SetOutputTypeToLabeledPoints() { this->SetOutputType(LABELED_POINTS); }
  void SetOutputTypeToBackgroundPoints() { this->SetOutputType(BACKGROUND_POINTS); }
  ///@}

  ///@{
  /**
   * Enable/disable point joggling. By default, the joggle radius is a
   * fraction of the diagonal length of the input image data. However, an
   * absolute joggle radius can be specified. Optionally, the joggle radius
   * can be constrained so that it remains in the originating pixel/voxel.
   * If a joggle constraint is enabled, then a factor can be applied that
   * specifies the maximum distance a point can be perturbed relative to
   * the minimum length of the pixel/voxel.
   */
  vtkSetMacro(Joggle, vtkTypeBool);
  vtkGetMacro(Joggle, vtkTypeBool);
  vtkBooleanMacro(Joggle, vtkTypeBool);
  vtkSetClampMacro(JoggleRadius, double, 0, 0.5);
  vtkGetMacro(JoggleRadius, double);
  vtkSetMacro(JoggleRadiusIsAbsolute, vtkTypeBool);
  vtkBooleanMacro(JoggleRadiusIsAbsolute, vtkTypeBool);
  vtkGetMacro(JoggleRadiusIsAbsolute, vtkTypeBool);
  vtkSetMacro(ConstrainJoggle, vtkTypeBool);
  vtkBooleanMacro(ConstrainJoggle, vtkTypeBool);
  vtkGetMacro(ConstrainJoggle, vtkTypeBool);
  vtkSetClampMacro(JoggleConstraint, double, 0, 1);
  vtkGetMacro(JoggleConstraint, double);
  ///@}

  //---------------The following are methods used to set/get and generate
  //---------------label values.
  ///@{
  /**
   * Set a particular label value at label number i. The index i ranges
   * between (0 <= i < NumberOfLabels). (Note: while labels values are
   * expressed as doubles, the underlying scalar data may be a different
   * type. During execution the label values are cast to the type of the
   * scalar data.)  Note the use of "Value" and "Label" when specifying
   * regions to extract. The use of "Value" is consistent with other VTK
   * continuous-scalar field isocontouring algorithms; however the term
   * "Label" is more consistent with label maps.  Warning: make sure that the
   * label value >= 0 as any label < 0 is considered a background, i.e.,
   * outside, label.
   */
  void SetValue(int i, double value) { this->Labels->SetValue(i, value); }
  void SetLabel(int i, double value) { this->Labels->SetValue(i, value); }
  ///@}

  ///@{
  /**
   * Get the ith label value.
   */
  double GetValue(int i) { return this->Labels->GetValue(i); }
  double GetLabel(int i) { return this->Labels->GetValue(i); }
  ///@}

  ///@{
  /**
   * Get a pointer to an array of labels. There will be
   * GetNumberOfLabels() values in the list.
   */
  double* GetValues() { return this->Labels->GetValues(); }
  double* GetLabels() { return this->Labels->GetValues(); }
  ///@}

  ///@{
  /**
   * Fill a supplied list with label values. There will be
   * GetNumberOfLabels() values in the list. Make sure you allocate enough
   * memory to hold the list.
   */
  void GetValues(double* contourValues) { this->Labels->GetValues(contourValues); }
  void GetLabels(double* contourValues) { this->Labels->GetValues(contourValues); }
  ///@}

  ///@{
  /**
   * Set the number of labels to place into the list. You only really need to
   * use this method to reduce list size. The method SetValue() will
   * automatically increase list size as needed. Note that for consistency
   * with other isocountoring-related algorithms, some methods use
   * "Labels" and "Contours" interchangeably.
   */
  void SetNumberOfLabels(int number) { this->Labels->SetNumberOfContours(number); }
  void SetNumberOfContours(int number) { this->Labels->SetNumberOfContours(number); }
  ///@}

  ///@{
  /**
   * Get the number of labels in the list of label values.
   */
  vtkIdType GetNumberOfLabels() { return this->Labels->GetNumberOfContours(); }
  vtkIdType GetNumberOfContours() { return this->Labels->GetNumberOfContours(); }
  ///@}

  ///@{
  /**
   * Generate numLabels equally spaced labels between the specified
   * range. The labels will include the min/max range values.
   */
  void GenerateLabels(int numLabels, double range[2])
  {
    this->Labels->GenerateValues(numLabels, range);
  }
  void GenerateValues(int numContours, double range[2])
  {
    this->Labels->GenerateValues(numContours, range);
  }
  void GenerateLabels(int numLabels, double rangeStart, double rangeEnd)
  {
    this->Labels->GenerateValues(numLabels, rangeStart, rangeEnd);
  }
  void GenerateValues(int numContours, double rangeStart, double rangeEnd)
  {
    this->Labels->GenerateValues(numContours, rangeStart, rangeEnd);
  }
  ///@}
  //---------------Done defining label-related methods.

  ///@{
  /**
   * Specify whether to create output cell vertices (i.e., a Verts
   * vtkCellArray) as well as output points. Cell vertices are necessary for
   * rendering, but may not be needed by other filters. Producing vertices
   * (which is on by default) incurs extra memory and compute time.
   */
  vtkSetMacro(GenerateVerts, vtkTypeBool);
  vtkGetMacro(GenerateVerts, vtkTypeBool);
  vtkBooleanMacro(GenerateVerts, vtkTypeBool);
  ///@}

  /**
   * Because we delegate to vtkContourValues.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkLabeledImagePointSampler();
  ~vtkLabeledImagePointSampler() override = default;

  vtkTypeBool BackgroundPointMapping;
  int BackgroundPointLabel;
  int DensityDistribution;
  unsigned int N;
  int OutputType;

  vtkTypeBool Randomize;
  double RandomProbabilityRange[2];

  vtkTypeBool Joggle;
  double JoggleRadius;
  vtkTypeBool JoggleRadiusIsAbsolute;
  vtkTypeBool ConstrainJoggle;
  double JoggleConstraint;

  vtkSmartPointer<vtkContourValues> Labels;

  vtkTypeBool GenerateVerts;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkLabeledImagePointSampler(const vtkLabeledImagePointSampler&) = delete;
  void operator=(const vtkLabeledImagePointSampler&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
