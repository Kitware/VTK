// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageEuclideanDistance
 * @brief   computes 3D Euclidean DT
 *
 * vtkImageEuclideanDistance implements the Euclidean DT using
 * Saito's algorithm. The distance map produced contains the square of the
 * Euclidean distance values.
 *
 * The algorithm has a o(n^(D+1)) complexity over nxnx...xn images in D
 * dimensions. It is very efficient on relatively small images. Cuisenaire's
 * algorithms should be used instead if n >> 500. These are not implemented
 * yet.
 *
 * For the special case of images where the slice-size is a multiple of
 * 2^N with a large N (typically for 256x256 slices), Saito's algorithm
 * encounters a lot of cache conflicts during the 3rd iteration which can
 * slow it very significantly. In that case, one should use
 * vtkImageEuclideanDistance::SetAlgorithmToSaitoCached() instead for better performance.
 *
 * References:
 *
 * T. Saito and J.I. Toriwaki. New algorithms for Euclidean distance
 * transformations of an n-dimensional digitised picture with applications.
 * Pattern Recognition, 27(11). pp. 1551--1565, 1994.
 *
 * O. Cuisenaire. Distance Transformation: fast algorithms and applications
 * to medical image processing. PhD Thesis, Universite catholique de Louvain,
 * October 1999. http://ltswww.epfl.ch/~cuisenai/papers/oc_thesis.pdf
 */

#ifndef vtkImageEuclideanDistance_h
#define vtkImageEuclideanDistance_h

#include "vtkImageDecomposeFilter.h"
#include "vtkImagingGeneralModule.h" // For export macro

#define VTK_EDT_SAITO_CACHED 0
#define VTK_EDT_SAITO 1

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGGENERAL_EXPORT vtkImageEuclideanDistance : public vtkImageDecomposeFilter
{
public:
  static vtkImageEuclideanDistance* New();
  vtkTypeMacro(vtkImageEuclideanDistance, vtkImageDecomposeFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Used to set all non-zero voxels to MaximumDistance before starting
   * the distance transformation. Setting Initialize off keeps the current
   * value in the input image as starting point. This allows to superimpose
   * several distance maps.
   */
  vtkSetMacro(Initialize, vtkTypeBool);
  vtkGetMacro(Initialize, vtkTypeBool);
  vtkBooleanMacro(Initialize, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Used to define whether Spacing should be used in the computation of the
   * distances
   */
  vtkSetMacro(ConsiderAnisotropy, vtkTypeBool);
  vtkGetMacro(ConsiderAnisotropy, vtkTypeBool);
  vtkBooleanMacro(ConsiderAnisotropy, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Any distance bigger than this->MaximumDistance will not ne computed but
   * set to this->MaximumDistance instead.
   */
  vtkSetMacro(MaximumDistance, double);
  vtkGetMacro(MaximumDistance, double);
  ///@}

  ///@{
  /**
   * Selects a Euclidean DT algorithm.
   * 1. Saito
   * 2. Saito-cached
   * More algorithms will be added later on.
   */
  vtkSetMacro(Algorithm, int);
  vtkGetMacro(Algorithm, int);
  void SetAlgorithmToSaito() { this->SetAlgorithm(VTK_EDT_SAITO); }
  void SetAlgorithmToSaitoCached() { this->SetAlgorithm(VTK_EDT_SAITO_CACHED); }
  ///@}

  int IterativeRequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

protected:
  vtkImageEuclideanDistance();
  ~vtkImageEuclideanDistance() override = default;

  double MaximumDistance;
  vtkTypeBool Initialize;
  vtkTypeBool ConsiderAnisotropy;
  int Algorithm;

  // Replaces "EnlargeOutputUpdateExtent"
  virtual void AllocateOutputScalars(vtkImageData* outData, int outExt[6], vtkInformation* outInfo);

  int IterativeRequestInformation(vtkInformation* in, vtkInformation* out) override;
  int IterativeRequestUpdateExtent(vtkInformation* in, vtkInformation* out) override;

private:
  vtkImageEuclideanDistance(const vtkImageEuclideanDistance&) = delete;
  void operator=(const vtkImageEuclideanDistance&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
