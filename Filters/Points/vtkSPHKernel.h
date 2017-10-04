/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSPHKernel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSPHKernel
 * @brief   a family of SPH interpolation kernels
 *
 *
 * vtkSPHKernel is an abstract superclass for smoothed-particle hydrodynamics
 * interpolation kernels as described by D.J. Price (see full reference
 * below).
 *
 * Note that the kernel operates over a volume in space defined by a radius
 * at a sampling point. The kernel implicitly assumes that the particles
 * making up the input data satisfies physical properties such as
 * conservation of mass. Therefore subclasses of this kernel are not
 * generally applicable for interpolation processes, and therefore operate in
 * conjunction with the vthSPHInterpolator class.
 *
 * By default the kernel computes local particle volume from the spatial step^3.
 * However, if both an optional mass and density arrays are provided then they are
 * used to compute local volume.
 *
 * Also be default, the local neighborhood around a point to be interpolated is
 * computed as the CutoffFactor * SpatialStep. (Note the CutoffFactor varies for
 * each type of SPH kernel.) However, the user may specify a CutoffArray which
 * enables variable cutoff distances per each point.
 *
 * @warning
 * For more information see D.J. Price, Smoothed particle hydrodynamics and
 * magnetohydrodynamics, J. Comput. Phys. 231:759-794, 2012. Especially
 * equation 49.
 *
 * @par Acknowledgments:
 * The following work has been generously supported by Altair Engineering
 * and FluiDyna GmbH. Please contact Steve Cosgrove or Milos Stanic for
 * more information.
 *
 * @sa
 * vtkSPHKernel vtkSPHQuinticKernel vtkInterpolationKernel vtkGaussianKernel
 * vtkShepardKernel vtkLinearKernel
*/

#ifndef vtkSPHKernel_h
#define vtkSPHKernel_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkInterpolationKernel.h"
#include "vtkStdString.h" // For vtkStdString ivars

class vtkIdList;
class vtkDoubleArray;
class vtkDataArray;
class vtkFloatArray;


class VTKFILTERSPOINTS_EXPORT vtkSPHKernel : public vtkInterpolationKernel
{
public:
  //@{
  /**
   * Standard methods for instantiation, obtaining type information, and printing.
   */
  vtkTypeMacro(vtkSPHKernel,vtkInterpolationKernel);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * The user defined initial particle spatial step. This is also referred to as
   * the smoothing length.
   */
  vtkSetClampMacro(SpatialStep,double,0.0,VTK_FLOAT_MAX);
  vtkGetMacro(SpatialStep,double);
  //@}

  //@{
  /**
   * The domain dimension, default to 3.
   */
  vtkSetClampMacro(Dimension,int,1,3);
  vtkGetMacro(Dimension,int);
  //@}

  //@{
  /**
   * Return the cutoff factor. This is hard wired into the kernel (e.g., the
   * vtkSPHQuinticKernel has a cutoff factor = 3.0).
   */
  vtkGetMacro(CutoffFactor,double);
  //@}

  //@{
  /**
   * Specify the (optional) array defining a cutoff distance. If provided this
   * distance is used to find the interpolating points within the local
   * neighborbood. Otherwise the cutoff distance is defined as the cutoff
   * factor times the spatial step size.
   */
  virtual void SetCutoffArray(vtkDataArray*);
  vtkGetObjectMacro(CutoffArray,vtkDataArray);
  //@}

  //@{
  /**
   * Specify the (optional) density array. Used with the mass array to
   * compute local particle volumes.
   */
  virtual void SetDensityArray(vtkDataArray*);
  vtkGetObjectMacro(DensityArray,vtkDataArray);
  //@}

  //@{
  /**
   * Specify the (optional) mass array. Used with the density array to
   * compute local particle volumes.
   */
  virtual void SetMassArray(vtkDataArray*);
  vtkGetObjectMacro(MassArray,vtkDataArray);
  //@}

  /**
   * Produce the computational parameters for the kernel. Invoke this method
   * after setting initial values like SpatialStep.
   */
  void Initialize(vtkAbstractPointLocator *loc, vtkDataSet *ds,
                          vtkPointData *pd) override;

  /**
   * Given a point x (and optional associated ptId), determine the points
   * around x which form an interpolation basis. The user must provide the
   * vtkIdList pIds, which will be dynamically resized as necessary. The
   * method returns the number of points in the basis. Typically this method
   * is called before ComputeWeights(). Note that while ptId is optional in most
   * cases, if a cutoff array is provided, then ptId must be provided.
   */
  vtkIdType ComputeBasis(double x[3], vtkIdList *pIds, vtkIdType ptId=0) override;

  /**
   * Given a point x, and a list of basis points pIds, compute interpolation
   * weights associated with these basis points.
   */
  vtkIdType ComputeWeights(double x[3], vtkIdList *pIds,
                                   vtkDoubleArray *weights) override;

  /**
   * Given a point x, and a list of basis points pIds, compute interpolation
   * weights, plus derivative weights, associated with these basis points.
   */
  virtual vtkIdType ComputeDerivWeights(double x[3], vtkIdList *pIds,
                                        vtkDoubleArray *weights,
                                        vtkDoubleArray *gradWeights);

  /**
   * Compute weighting factor given a normalized distance from a sample point.
   */
  virtual double ComputeFunctionWeight(const double d) = 0;

  /**
   * Compute weighting factor for derivative quantities given a normalized
   * distance from a sample point.
   */
  virtual double ComputeDerivWeight(const double d) = 0;

  //@{
  /**
   * Return the SPH normalization factor. This also includes the contribution
   * of 1/h^d, where h is the smoothing length (i.e., spatial step) and d is
   * the dimension of the kernel. The returned value is only valid after the
   * kernel is initialized.
   */
  vtkGetMacro(NormFactor,double);
  //@}

protected:
  vtkSPHKernel();
  ~vtkSPHKernel() override;

  // Instance variables
  double SpatialStep; //also known as smoothing length h
  int Dimension; //sptial dimension of the kernel

  // Optional arrays aid in the interpolation process (computes volume)
  vtkDataArray *CutoffArray;
  vtkDataArray *DensityArray;
  vtkDataArray *MassArray;

  // Internal data members generated during construction and initialization
  // Terminology is spatial step = smoothing length h
  double CutoffFactor; //varies across each kernel, e.g. cubic=2, quartic=2.5, quintic=3
  double Cutoff; //the spatial step * cutoff factor
  double Sigma; //normalization constant
  double DistNorm; //distance normalization factor 1/(spatial step)
  double NormFactor; //dimensional normalization factor sigma/(spatial step)^Dimension
  double DefaultVolume; //if mass and density arrays not specified, use this
  bool UseCutoffArray; //if single component cutoff array provided
  bool UseArraysForVolume; //if both mass and density arrays are present

private:
  vtkSPHKernel(const vtkSPHKernel&) = delete;
  void operator=(const vtkSPHKernel&) = delete;
};

#endif
