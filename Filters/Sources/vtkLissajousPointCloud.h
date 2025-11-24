// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLissajousPointCloud
 * @brief   create a set of points distributed within a tube centered on a Lissajous curve.
 *
 * vtkLissajousPointCloud is a source filter that produces a set of points.
 * The points are placed on or surrounding a Lissajous curve. The shape of
 * the curve can be controlled by adjusting its amplitude, frequency, and
 * phase. The total number of points can be specified, as well as the
 * generation of points in a radius surrounding the curve. Point positions
 * can be further modified by applying noise (i.e., jitter).
 *
 * This filter is typically used in modeling objects, or for producing
 * test data for other filters (e.g., vtkVoronoi2D or vtkVoronoi3D). An option
 * for producing "background" points not on the curve and surrounding the
 * Lissajous curve is also available.
 *
 * @sa
 * vtkVoronoi2D vtkVoronoi3D vtkDelaunay2D vtkDelaunay3D
 * vtkGeneralizedSurfaceNets3D
 */

#ifndef vtkLissajousPointCloud_h
#define vtkLissajousPointCloud_h

#include "vtkFiltersSourcesModule.h" // For export macro.
#include "vtkMath.h"                 // For vtkMath::Pi().
#include "vtkPolyDataAlgorithm.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSSOURCES_EXPORT VTK_MARSHALAUTO vtkLissajousPointCloud : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  vtkTypeMacro(vtkLissajousPointCloud, vtkPolyDataAlgorithm);
  static vtkLissajousPointCloud* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Set/get the number of points to produce.
   */
  vtkSetMacro(NumberOfPoints, vtkIdType);
  vtkGetMacro(NumberOfPoints, vtkIdType);
  ///@}

  ///@{
  /**
   * Set/get the size of the curve along each axis.
   */
  vtkSetVector3Macro(Amplitude, double);
  vtkGetVectorMacro(Amplitude, double, 3);
  ///@}

  ///@{
  /**
   * Set/get the number of oscillations in the curve along each axis.
   */
  vtkSetVector3Macro(Frequency, double);
  vtkGetVectorMacro(Frequency, double, 3);
  ///@}

  ///@{
  /**
   * Set/get the phase offset of the oscillations along each axis.
   */
  vtkSetVector3Macro(Phase, double);
  vtkGetVectorMacro(Phase, double, 3);
  ///@}

  ///@{
  /**
   * Set/get the magnitude of the perturbation from the curve to apply to each point.
   */
  vtkSetVector3Macro(Noise, double);
  vtkGetVectorMacro(Noise, double, 3);
  ///@}

  ///@{
  /**
   * Set/get a "tube radius" used to produce additional point pairs near a surface
   * of distance \a Radius from the center of the Lissajous curve.
   */
  vtkGetMacro(Radius, double);
  vtkSetClampMacro(Radius, double, 0, VTK_DOUBLE_MAX);
  ///@}

  ///@{
  /**
   * Set/get whether the Noise ivar above is deterministic or not.
   *
   * If this is false (the default), then the noise added to point coordinates
   * is done in parallel. Otherwise, vtkMinimalStandardRandomSequence is used
   * in serial after point coordinates have been computed so that tests can
   * use this data and exactly reproduce results from run to run.
   */
  vtkGetMacro(DeterministicNoise, vtkTypeBool);
  vtkSetMacro(DeterministicNoise, vtkTypeBool);
  vtkBooleanMacro(DeterministicNoise, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/get the seed used when DeterministicNoise is true.
   *
   * This is available for testing.
   */
  vtkGetMacro(DeterministicSeed, int);
  vtkSetMacro(DeterministicSeed, int);
  ///@}

  ///@{
  /**
   * Set/get whether the curve parameters are output at each point.
   */
  vtkGetMacro(GenerateParameterScalar, vtkTypeBool);
  vtkSetMacro(GenerateParameterScalar, vtkTypeBool);
  vtkBooleanMacro(GenerateParameterScalar, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/get whether a region ID is output at each point.
   *
   * When Radius is 0, all points have a region ID of 0.
   * When Radius > 0, some points have a region ID of -1.
   */
  vtkGetMacro(GenerateRegionScalar, vtkTypeBool);
  vtkSetMacro(GenerateRegionScalar, vtkTypeBool);
  vtkBooleanMacro(GenerateRegionScalar, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Indicate whether to add background points, and region labels
   * to associate with the background points. By default, adding
   * background points is off; and the background label == (-1).
   */
  vtkGetMacro(AddBackgroundPoints, vtkTypeBool);
  vtkSetMacro(AddBackgroundPoints, vtkTypeBool);
  vtkBooleanMacro(AddBackgroundPoints, vtkTypeBool);
  vtkGetMacro(BackgroundLabel, int);
  vtkSetMacro(BackgroundLabel, int);
  ///@}

protected:
  vtkLissajousPointCloud();
  ~vtkLissajousPointCloud() override;

  int RequestData(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo) override;

  vtkIdType NumberOfPoints{ 128 };
  double Amplitude[3] = { 1, 1, 1 };
  double Frequency[3] = { 1, 2, 3 };
  double Phase[3] = { vtkMath::Pi() / 2., 0., vtkMath::Pi() / 8. };
  double Noise[3] = { 0.05, 0.05, 0.05 };
  vtkTypeBool DeterministicNoise{ false };
  vtkTypeBool GenerateParameterScalar{ false };
  vtkTypeBool GenerateRegionScalar{ false };
  int DeterministicSeed{ 0xea7beef };
  double Radius{ 0. };
  vtkTypeBool AddBackgroundPoints{ false };
  int BackgroundLabel{ -1 };
};

VTK_ABI_NAMESPACE_END
#endif
