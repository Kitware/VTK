// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWindowedSincPolyDataFilter
 * @brief   adjust point positions using a windowed sinc function interpolation kernel
 *
 * vtkWindowedSincPolyDataFiler adjusts point coordinates using a windowed
 * sinc function interpolation kernel. The effect is to "relax" or "smooth"
 * the mesh, making the cells better shaped and the vertices more evenly
 * distributed.  Note that this filter operates the lines, polygons, and
 * triangle strips composing an instance of vtkPolyData. Vertex or
 * poly-vertex cells are never modified.
 *
 * The algorithm proceeds as follows. For each vertex v, a topological and
 * geometric analysis is performed to determine which vertices are connected
 * to v, and which cells are connected to v. Then, a connectivity array is
 * constructed for each vertex. (The connectivity array is a list of lists
 * of vertices that directly attach to each vertex, the so-called smoothing
 * stencil.) Next, an iteration phase begins over all vertices. For each
 * vertex v, the coordinates of v are modified using a windowed sinc function
 * interpolation kernel.  Taubin describes this methodology is the IBM tech
 * report RC-20404 (#90237, dated 3/12/96) "Optimal Surface Smoothing as
 * Filter Design" G. Taubin, T. Zhang and G. Golub. (Zhang and Golub are at
 * Stanford University.)
 *
 * This report discusses using standard signal processing low-pass filters
 * (in particular windowed sinc functions) to smooth polyhedra. The
 * transfer functions of the low-pass filters are approximated by
 * Chebyshev polynomials. This facilitates applying the filters in an
 * iterative diffusion process (as opposed to a kernel convolution).  The
 * more smoothing iterations applied, the higher the degree of polynomial
 * approximating the low-pass filter transfer function. Each smoothing
 * iteration, therefore, applies the next higher term of the Chebyshev
 * filter approximation to the polyhedron. This decoupling of the filter
 * into an iteratively applied polynomial is possible since the Chebyshev
 * polynomials are orthogonal, i.e. increasing the order of the
 * approximation to the filter transfer function does not alter the
 * previously calculated coefficients for the low order terms.
 *
 * Note: Care must be taken to avoid smoothing with too few iterations.
 * A Chebyshev approximation with too few terms is a poor approximation.
 * The first few smoothing iterations represent a severe scaling and
 * translation of the data.  Subsequent iterations cause the smoothed
 * polyhedron to converge to the true location and scale of the object.
 * We have attempted to protect against this by automatically adjusting
 * the filter, effectively widening the pass band. This adjustment is only
 * possible if the number of iterations is greater than 1.  Note that this
 * sacrifices some degree of smoothing for model integrity. For those
 * interested, the filter is adjusted by searching for a value sigma
 * such that the actual pass band is k_pb + sigma and such that the
 * filter transfer function evaluates to unity at k_pb, i.e. f(k_pb) = 1
 *
 * To improve the numerical stability of the solution, and minimize the
 * scaling the translation effects, the algorithm can translate and
 * scale the position coordinates to within the unit cube [-1, 1],
 * perform the smoothing, and translate and scale the position
 * coordinates back to the original coordinate frame.  This mode is
 * controlled with the NormalizeCoordinatesOn() /
 * NormalizeCoordinatesOff() methods.  For legacy reasons, the default
 * is NormalizeCoordinatesOff.
 *
 * This implementation is currently limited to using an interpolation
 * kernel based on Hamming windows.  Other windows (such as Hann, Blackman,
 * Kaiser, Lanczos, Gaussian, and exponential windows) could be used
 * instead.
 *
 * There are some special instance variables used to control the execution
 * of this filter. (These ivars basically control what vertices can be
 * smoothed, and the creation of the connectivity array.) The
 * BoundarySmoothing ivar enables/disables the smoothing operation on
 * vertices that are on the "boundary" of the mesh. A boundary vertex is one
 * that is surrounded by a semi-cycle of polygons (or used by a single
 * line).
 *
 * Another important ivar is FeatureEdgeSmoothing. If this ivar is
 * enabled, then interior vertices are classified as either "simple",
 * "interior edge", or "fixed", and smoothed differently. (Interior
 * vertices are manifold vertices surrounded by a cycle of polygons; or used
 * by two line cells.) The classification is based on the number of feature
 * edges attached to v. A feature edge occurs when the angle between the two
 * surface normals of a polygon sharing an edge is greater than the
 * FeatureAngle ivar. Then, vertices used by no feature edges are classified
 * "simple", vertices used by exactly two feature edges are classified
 * "interior edge", and all others are "fixed" vertices.
 *
 * Once the classification is known, the vertices are smoothed
 * differently. Corner (i.e., fixed) vertices are not smoothed at all.
 * Simple vertices are smoothed as before. Interior edge vertices are
 * smoothed only along their two connected edges, and only if the angle
 * between the edges is less than the EdgeAngle ivar.
 *
 * The total smoothing can be controlled by using two ivars. The
 * NumberOfIterations determines the maximum number of smoothing passes.
 * The NumberOfIterations corresponds to the degree of the polynomial that
 * is used to approximate the windowed sinc function. Ten or twenty
 * iterations is all the is usually necessary. Contrast this with
 * vtkSmoothPolyDataFilter which usually requires 100 to 200 smoothing
 * iterations. vtkSmoothPolyDataFilter is also not an approximation to
 * an ideal low-pass filter, which can cause the geometry to shrink as the
 * amount of smoothing increases.
 *
 * The second ivar is the specification of the PassBand for the windowed
 * sinc filter.  By design, the PassBand is specified as a doubling point
 * number between 0 and 2.  Lower PassBand values produce more smoothing.
 * A good default value for the PassBand is 0.1 (for those interested, the
 * PassBand (and frequencies) for PolyData are based on the valence of the
 * vertices, this limits all the frequency modes in a polyhedral mesh to
 * between 0 and 2.)
 *
 * There are two instance variables that control the generation of error
 * data. If the ivar GenerateErrorScalars is on, then a scalar value
 * indicating the distance of each vertex from its original position is
 * computed. If the ivar GenerateErrorVectors is on, then a vector
 * representing change in position is computed.
 *
 * @warning
 * The smoothing operation reduces high frequency information in the
 * geometry of the mesh. With excessive smoothing important details may be
 * lost. Enabling FeatureEdgeSmoothing helps reduce this effect, but cannot
 * entirely eliminate it.
 *
 * @warning
 * For maximum performance, do not enable BoundarySmoothing,
 * NonManifoldSmoothing, or FeatureEdgeSmoothing. FeatureEdgeSmoothing is
 * particularly expensive as it requires building topological links and
 * computing local polygon normals which are relatively expensive
 * operations. BoundarySmoothing and NonManifoldSmoothing have a modest
 * impact on performance.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential execution type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * Another useful documentation resource is this SIGGRAPH publication:
 * Gabriel Taubin. "A Signal Processing Approach To Fair Surface Design".
 *
 * @sa
 * vtkSmoothPolyDataFilter vtkConstrainedSmoothingFilter vtkPointSmoothingFilter
 * vtkAttributeSmoothingFilter vtkDecimate vtkDecimatePro vtkQuadricDecimation
 */

#ifndef vtkWindowedSincPolyDataFilter_h
#define vtkWindowedSincPolyDataFilter_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkWindowedSincPolyDataFilter : public vtkPolyDataAlgorithm
{
public:
  /**
   * Construct object with number of iterations 20; passband .1; feature edge
   * smoothing turned off; feature angle 45 degrees; edge angle 15 degrees;
   * and boundary smoothing turned on. Error scalars and vectors are not
   * generated (by default).
   */
  static vtkWindowedSincPolyDataFilter* New();

  ///@{
  /**
   * Standard methods to obtain information, and print information about the
   * the object.
   */
  vtkTypeMacro(vtkWindowedSincPolyDataFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Specify the number of iterations (i.e., the degree of the polynomial
   * approximating the windowed sinc function). Typically values around 20
   * are used.
   */
  vtkSetClampMacro(NumberOfIterations, int, 0, VTK_INT_MAX);
  vtkGetMacro(NumberOfIterations, int);
  ///@}

  ///@{
  /**
   * Set the passband value for the windowed sinc filter.
   */
  vtkSetClampMacro(PassBand, double, 0.0, 2.0);
  vtkGetMacro(PassBand, double);
  ///@}

  ///@{
  /**
   * Turn on/off coordinate normalization.  The positions can be translated
   * and scaled such that they fit within a [-1, 1] prior to the smoothing
   * computation. The default is off.  The numerical stability of the
   * solution can be improved by turning normalization on.  If normalization
   * is on, the coordinates will be rescaled to the original coordinate
   * system after smoothing has completed.
   */
  vtkSetMacro(NormalizeCoordinates, vtkTypeBool);
  vtkGetMacro(NormalizeCoordinates, vtkTypeBool);
  vtkBooleanMacro(NormalizeCoordinates, vtkTypeBool);
  ///@}

  /**
   * WindowFunction types
   */
  enum
  {
    NUTTALL = 0,
    BLACKMAN,
    HANNING,
    HAMMING
  };

  ///@{
  /**
   * Window function used for approximating the low-pass filter they determine how many iterations
   * are needed and how accurate the smoothing is.
   * - NUTTALL function is recommended (this is the default), as it provides
   *   accurate scale even when number of iterations is low.
   * - BLACKMAN function has similar performance to NUTTALL but may shrink the output a bit more.
   * - HANNING function tends to converge to the correct scale but only above 40-60 iterations.
   * - HAMMING function was the default in VTK for a long time, but is prone to scaling errors.
   *   Up to about 1% scaling error may occur (either increase or decrease overall size)
   *   and the error does not reduce to zero when the number of iterations is increased.
   */
  vtkSetClampMacro(WindowFunction, int, NUTTALL, HAMMING);
  vtkGetMacro(WindowFunction, int);
  void SetWindowFunctionToNuttall()
  {
    this->SetWindowFunction(vtkWindowedSincPolyDataFilter::NUTTALL);
  }
  void SetWindowFunctionToBlackman()
  {
    this->SetWindowFunction(vtkWindowedSincPolyDataFilter::BLACKMAN);
  }
  void SetWindowFunctionoHanning()
  {
    this->SetWindowFunction(vtkWindowedSincPolyDataFilter::HANNING);
  }
  void SetWindowFunctionToHamming()
  {
    this->SetWindowFunction(vtkWindowedSincPolyDataFilter::HAMMING);
  }
  ///@}

  ///@{
  /**
   * Turn on/off smoothing points along sharp interior edges. Enabling this
   * option has a performance impact on the algorithm since neihborhood
   * information (cell links) and polygon normals must be computed.
   */
  vtkSetMacro(FeatureEdgeSmoothing, vtkTypeBool);
  vtkGetMacro(FeatureEdgeSmoothing, vtkTypeBool);
  vtkBooleanMacro(FeatureEdgeSmoothing, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Specify the feature angle for sharp edge identification. It only affects
   * the filter when FeatureEdgeSmoothing is enabled.
   */
  vtkSetClampMacro(FeatureAngle, double, 0.0, 180.0);
  vtkGetMacro(FeatureAngle, double);
  ///@}

  ///@{
  /**
   * Specify the edge angle to control smoothing along edges (either interior
   * or boundary).
   */
  vtkSetClampMacro(EdgeAngle, double, 0.0, 180.0);
  vtkGetMacro(EdgeAngle, double);
  ///@}

  ///@{
  /**
   * Turn on/off the smoothing of points on the boundary of the mesh.
   * Enabled this option has a modest impact on performance.
   */
  vtkSetMacro(BoundarySmoothing, vtkTypeBool);
  vtkGetMacro(BoundarySmoothing, vtkTypeBool);
  vtkBooleanMacro(BoundarySmoothing, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Smooth non-manifold points. Enabling this option has a modest
   * impact on performance.
   */
  vtkSetMacro(NonManifoldSmoothing, vtkTypeBool);
  vtkGetMacro(NonManifoldSmoothing, vtkTypeBool);
  vtkBooleanMacro(NonManifoldSmoothing, vtkTypeBool);
  ///@}

  ///@{
  /**
   * When non-manifold smoothing is enabled, better smoothing performance may
   * be possible by providing extra weighting to non-manifold edges. By default,
   * WeightNonManifoldEdges is enabled (this is to preserve consistent behavior
   * with previous versions of this filter).
   */
  vtkSetMacro(WeightNonManifoldEdges, vtkTypeBool);
  vtkGetMacro(WeightNonManifoldEdges, vtkTypeBool);
  vtkBooleanMacro(WeightNonManifoldEdges, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off the generation of scalar distance values.
   */
  vtkSetMacro(GenerateErrorScalars, vtkTypeBool);
  vtkGetMacro(GenerateErrorScalars, vtkTypeBool);
  vtkBooleanMacro(GenerateErrorScalars, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off the generation of error vectors.
   */
  vtkSetMacro(GenerateErrorVectors, vtkTypeBool);
  vtkGetMacro(GenerateErrorVectors, vtkTypeBool);
  vtkBooleanMacro(GenerateErrorVectors, vtkTypeBool);
  ///@}

protected:
  vtkWindowedSincPolyDataFilter();
  ~vtkWindowedSincPolyDataFilter() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int NumberOfIterations;
  double PassBand;

  vtkTypeBool NormalizeCoordinates;
  int WindowFunction;

  vtkTypeBool FeatureEdgeSmoothing;
  double FeatureAngle;
  double EdgeAngle;
  vtkTypeBool BoundarySmoothing;
  vtkTypeBool NonManifoldSmoothing;

  vtkTypeBool WeightNonManifoldEdges;

  vtkTypeBool GenerateErrorScalars;
  vtkTypeBool GenerateErrorVectors;

private:
  vtkWindowedSincPolyDataFilter(const vtkWindowedSincPolyDataFilter&) = delete;
  void operator=(const vtkWindowedSincPolyDataFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
