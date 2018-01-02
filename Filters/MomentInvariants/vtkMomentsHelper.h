/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMomentsHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

Copyright (c) 2017, Los Alamos National Security, LLC

All rights reserved.

Copyright 2017. Los Alamos National Security, LLC.
This software was produced under U.S. Government contract DE-AC52-06NA25396
for Los Alamos National Laboratory (LANL), which is operated by
Los Alamos National Security, LLC for the U.S. Department of Energy.
The U.S. Government has rights to use, reproduce, and distribute this software.
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.
If software is modified to produce derivative works, such modified software
should be clearly marked, so as not to confuse it with the version available
from LANL.

Additionally, redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following conditions
are met:
-   Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
/**
 * @class   vtkMomentsHelper
 * @brief   rotation invariant pattern detetction
 *
 * vtkMomentsHelper is a helper class that contains functions that will
 * be used by more than one algorithm in the moments module the theory and
 * the algorithm is described in Roxana Bujack and Hans Hagen: "Moment
 * Invariants for Multi-Dimensional Data"
 * http://www.informatik.uni-leipzig.de/~bujack/2017TensorDagstuhl.pdf
 * @par Thanks:
 * Developed by Roxana Bujack at Los Alamos National Laboratory.
 */

#ifndef vtkMomentsHelper_h
#define vtkMomentsHelper_h
#ifndef __VTK_WRAP__

#include "vtkFiltersMomentInvariantsModule.h" // For export macro

#include "vtkType.h" // for vtkIdType

#include <string>    // for std::string
#include <vector>    // for std::vector

class vtkCell;
class vtkDataSet;
class vtkImageData;
class vtkMomentsTensor;

struct VTKFILTERSMOMENTINVARIANTS_EXPORT vtkMomentsHelper
{
  /**
   * The monomial basis is not orthonormal. We need this function for the reconstruction of the
   * function from the moments. This function uses Gram Schmidt
   * @param dimension: 2D or 3D
   * @param moments: the moments at a point
   * @param radius: the corresponding integration radius
   * @return the orthonormal moments
   */
  static std::vector<vtkMomentsTensor> orthonormalizeMoments(int dimension,
    std::vector<vtkMomentsTensor> moments,
    double radius);

  /**
   * This function computes the moments at a given location and radius
   * the moments are the projections of the function to the monomial basis
   * they are evaluated using a numerical integration over uniformly sampled 2D or 3D space
   * @param dimension: 2D or 3D
   * @param order: the maximal order up to which the moments are computed
   * @param fieldRank: 0 for scalar, 1 for vector and 2 for matrix
   * @param radius: the integration radius at which the moments are computed
   * @param center: location where the moments are computed
   * @param stencil: contains the locations at which the dataset is evaluated for the integration
   * @param nameOfPointData: the name of the array in the point data of which the momens are
   * computed.
   * @return the moments
   */
  static std::vector<vtkMomentsTensor> allMoments(int dimension,
    int order,
    int fieldRank,
    double radius,
    double center[3],
    vtkImageData* stencil,
    std::string nameOfPointData);

  /**
   * This function approximates the volume of a cell
   * for unstructured grids
   * @param cell: the cell
   * @param source: the dataset that contains the cell.
   * @return the volume
   */
  static double getVolume(vtkCell* cell, vtkDataSet* source);

  /**
   * This function computes the moments at a given location and radius
   * the moments are the projections of the function to the monomial basis
   * they are evaluated using a numerical integration over the original unstructured dataset
   * @param dimension: 2D or 3D
   * @param order: the maximal order up to which the moments are computed
   * @param fieldRank: 0 for scalar, 1 for vector and 2 for matrix
   * @param radius: the integration radius at which the moments are computed
   * @param center: location where the moments are computed
   * @param dataset: the dataset of which the moments are computed
   * @param nameOfPointData: the name of the array in the point data of which the momens are
   * computed.
   * @return the moments
   */
  static std::vector<vtkMomentsTensor> allMomentsOrigRes(int dimension,
    int order,
    int fieldRank,
    double radius,
    double center[3],
    vtkDataSet* dataset,
    std::string nameOfPointData);

  /**
   * This function computes the moments at a given location and radius
   * the moments are the projections of the function to the monomial basis
   * they are evaluated using a numerical integration over the original dataset if it is structured
   * data
   * @param dimension: 2D or 3D
   * @param order: the maximal order up to which the moments are computed
   * @param fieldRank: 0 for scalar, 1 for vector and 2 for matrix
   * @param radius: the integration radius at which the moments are computed
   * @param ptID: point id of the location where the moments are computed
   * @param dataset: the dataset of which the moments are computed
   * @param nameOfPointData: the name of the array in the point data of which the momens are
   * computed.
   * @return the moments
   */
  static std::vector<vtkMomentsTensor> allMomentsOrigResImageData(int dimension,
    int order,
    int fieldRank,
    double radius,
    int ptID,
    vtkImageData* dataset,
    std::string nameOfPointData);

  /**
   * This function computes the factor that needs to be removed for the translational normalization
   * it corresponds to the moment of the function identical to one
   * for the lowest orders, we know the analytic solution
   * @param radius: the integration radius at which the moments are computed
   * @param dimension: 2D or 3D
   * @param p: exponent in the basis function x^p*y^q*z^r
   * @param q: exponent in the basis function x^p*y^q*z^r
   * @param r: exponent in the basis function x^p*y^q*z^r
   * @return the translation factor
   */
  static double translationFactorAnalytic(double radius,
    int dimension,
    size_t p,
    size_t q,
    size_t r);

  /**
   * This function computes the factor that needs to be removed for the translational normalization
   * it corresponds to the moment of the function identical to one
   * if we do not know the analytic solution, we evaluate it numerically
   * @param center: location where the moments are computed
   * @param radius: the integration radius at which the moments are computed
   * @param p: exponent in the basis function x^p*y^q*z^r
   * @param q: exponent in the basis function x^p*y^q*z^r
   * @param r: exponent in the basis function x^p*y^q*z^r
   * @param stencil: contains the locations at which the dataset is evaluated for the integration
   * @return the translation factor
   */
  static double translationFactor(double radius,
    size_t p,
    size_t q,
    size_t r,
    vtkImageData* stencil);

  /**
   * This function generates the stencil, which contains the locations at which the dataset is
   * evaluated for the integration
   * @param stencil: contains the locations at which the dataset is evaluated for the integration
   * @param radius: the integration radius at which the moments are computed
   * @param numberOfIntegrationSteps: how fine the discrete integration done in each dimension
   * @param dimension: 2D or 3D
   * @param source: the dataset
   * @param nameOfPointData: the name of the array in the point data of which the momens are
   * computed.
   * @return the moments
   */
  static void BuildStencil(vtkImageData* stencil,
    double radius,
    int numberOfIntegrationSteps,
    int dimension,
    vtkDataSet* source,
    std::string nameOfPointData);

  /**
   * This function moves the stencil to the current location, where the integration is supposed o be
   * performed
   * @param center: the location
   * @param source: the dataset
   * @param stencil: contains the locations at which the dataset is evaluated for the integration
   * @param numberOfIntegrationSteps: how fine the discrete integration done in each dimension
   * @return the moments
   */
  static bool CenterStencil(double* center,
    vtkDataSet* source,
    vtkImageData* stencil,
    int numberOfIntegrationSteps,
    std::string nameOfPointData);

  /**
   * Inverse function to getFieldIndexFromTensorIndices
   * The output contains a vector with the tensor indices that describe the basis function that
   * belongs to the given output array. they are sorted by increasing order and then by the index as
   * returned by vtkMomentsTensor.getIndices(i)
   * @param index: the index of this output field pointdata array
   * @param dimension: 2D or 3D
   * @param order: the maximal order up to which the moments are computed
   * @param fieldRank: 0 for scalar, 1 for vector and 2 for matrix
   * @return the moments
   */
  static std::vector<size_t> getTensorIndicesFromFieldIndex(size_t index,
    int dimension,
    int order,
    int fieldRank);

  /**
   * The output contains the tensor indices that describe the basis function that belongs to the
   * given output array as string. Convenience function. they are sorted by increasing order and
   * then by the index as returned by vtkMomentsTensor.getIndices(i)
   * @param index: the index of this output field pointdata array
   * @param dimension: 2D or 3D
   * @param order: the maximal order up to which the moments are computed
   * @param fieldRank: 0 for scalar, 1 for vector and 2 for matrix
   * @return the moments
   */
  static std::string getTensorIndicesFromFieldIndexAsString(size_t index,
    int dimension,
    int order,
    int fieldRank);

  /**
   * Inverse function to getTensorIndicesFromFieldIndex
   * given a vector with tensor indices and a radius, this function returns the index in the output
   * of this algorithm that corresponds tothis basis function
   * @param radiusIndex: index of this radius in the radii vector
   * @param indices: the given tensor indices
   * @param dimension: 2D or 3D
   * @param fieldRank: 0 for scalar, 1 for vector and 2 for matrix
   * @param numberOfBasisFunctions: number of basis functions in the source
   * equals \sum_{i=0}^order dimension^o
   */
  static size_t getFieldIndexFromTensorIndices(size_t radiusIndex,
    std::vector<size_t> indices,
    int dimension,
    int fieldRank,
    int numberOfBasisFunctions);

  /**
   * the function returns true if the point lies within radius of the boundary of the dataset
   * @param ptId: ID of the point in question
   * @param field: the field that contains the point
   */
  static bool isCloseToEdge(int dimension, int ptId, double radius, vtkImageData* field);

  /**
   * the function returns true if the point lies on the boundary of the dataset
   * @param ptId: ID of the point in question
   * @param field: the field that contains the point
   */
  static bool isEdge(int dimension, int ptId, vtkImageData* field);

  /**
   * Calculates the index of the coordinate in the 1D array
   * that is treated as dimensions[0] x dimensions[1] x dimensions[2] matrix
   */
  static vtkIdType getArrayIndex(std::vector<int> coord, std::vector<int> dimensions);

  /**
   * Calculates the coordinate as if we are in a dimensions[0] x dimensions[1] x dimensions[2]
   * matrix based on the index of the 1D array
   */
  static std::vector<int> getCoord(vtkIdType index, std::vector<int> dimensions);

  /**
   * Translates the data to the origin (0, 0, 0)
   */
  static vtkImageData* translateToOrigin(vtkImageData* data);

  /**
   * Pad the field to a square where the size is
   * max(field->Dimensions) + max(kernel->Dimensions)
   */
  static vtkImageData* padField(vtkImageData* field,
    vtkImageData* kernel,
    int dimension,
    std::string nameOfPointData);

  /**
   * Pad the kernel to the same size as paddedField
   * The center of the kernel is the origin of the final output and the rest is wrapped accordingly
   */
  static vtkImageData* padKernel(vtkImageData* kernel, vtkImageData* paddedField);
};

#endif // __VTK_WRAP__
#endif
// VTK-HeaderTest-Exclude: vtkMomentsHelper.h
