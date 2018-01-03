/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkComputeMoments.h

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
 * @class   vtkComputeMoments
 * @brief   rotation invariant pattern detetction
 *
 * vtkComputeMoments is a filter that computes moments for 2D or 3D datasets
 * (field) that contain scalars, vectors or matrices in their pointdata. The
 * moments are evaluated at the points in grid. The output contains
 * scalar fields at the points of grid. Each scalar field corresponds to a
 * different component of the moment tensor up to Order. The respective
 * indices that identify each scalar field are stored in their respective
 * information the theory and the algorithm is described in Roxana Bujack and
 * Hans Hagen: "Moment Invariants for Multi-Dimensional Data"
 * http://www.informatik.uni-leipzig.de/~bujack/2017TensorDagstuhl.pdf
 * @par Thanks:
 * Developed by Roxana Bujack at Los Alamos National Laboratory.
 */

#ifndef vtkComputeMoments_h
#define vtkComputeMoments_h

#include "vtkFiltersMomentInvariantsModule.h" // For export macro

#include "vtkDataSetAlgorithm.h"

#include <string>    // for std::string
#include <vector>    // for std::vector

class VTKFILTERSMOMENTINVARIANTS_EXPORT vtkComputeMoments : public vtkDataSetAlgorithm
{
public:
  static vtkComputeMoments* New();

  vtkTypeMacro(vtkComputeMoments, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the field of which the moments are calculated.
   */
  void SetFieldData(vtkDataObject* input) { this->SetInputData(0, input); };

  /**
   * Set the field of which the moments are calculated.
   */
  void SetFieldConnection(vtkAlgorithmOutput* algOutput)
  {
    this->SetInputConnection(0, algOutput);
  };

  /**
   * Set the locations at which the moments are calculated.
   */
  void SetGridData(vtkDataObject* input) { this->SetInputData(1, input); };

  /**
   * Set the locations at which the moments are calculated.
   */
  void SetGridConnection(vtkAlgorithmOutput* algOutput) { this->SetInputConnection(1, algOutput); };

  //@{
  /**
   * Set/Get the maximal order up to which the moments are computed.
   */
  vtkSetMacro(Order, int);
  vtkGetMacro(Order, int);
  //@}

  //@{
  /**
   * Set/Get the resolution of the integration.
   */
  vtkSetMacro(NumberOfIntegrationSteps, int);
  vtkGetMacro(NumberOfIntegrationSteps, int);
  //@}

  /**
   * Get the number of basis functions in the field
   * equals \sum_{i=0}^order dimension^o
   */
  vtkGetMacro(NumberOfBasisFunctions, int);

  //@{
  /**
   * Set/Get the name of the array in the point data of which the momens are computed.
   */
  vtkSetMacro(NameOfPointData, std::string);
  vtkGetMacro(NameOfPointData, std::string);
  //@}

  //@{
  /**
   * Set/Get the flag of UseFFT.
   */
  vtkSetMacro(UseFFT, bool);
  vtkGetMacro(UseFFT, bool);
  //@}

  /**
   * Set the radii of the integration.
   */
  void SetRadii(const std::vector<double>& radii);

  /**
   * Get the radii of the integration.
   */
  std::vector<double> GetRadii() { return this->Radii; };

  /**
   * Get the radius of the integration.
   */
  double GetRadius(int i) { return this->Radii.at(i); };

  /**
   * Set the different integration radii from the field as constant length
   * array for python wrapping
   * @param radiiArray: array of size 10 containing the radii. if less radii
   * are desired, fill the
   * remaining entries with zeros
   */
  void SetRadiiArray(double radiiArray[10]);

  /**
   * Get the different integration radii from the field as constant length
   * array for python wrapping
   * @param radiiArray: array of size 10 containing the radii. if less radii
   * are desired, fill the remaining entries with zeros
   */
  void GetRadiiArray(double radiiArray[10]);

  /**
   * Set the relative radii of the integration, i.e. radius / min extent of the
   * dataset.
   */
  void SetRelativeRadii(const std::vector<double>& radii);

  /**
   * Get the relative radii of the integration, i.e. radius / min extent of the
   * dataset.
   */
  std::vector<double> GetRelativeRadii() { return this->RelativeRadii; };

  /**
   * Get the relative radii of the integration, i.e. radius / min extent of the
   * dataset.
   */
  double GetRelativeRadius(int i) { return this->RelativeRadii.at(i); };

  /**
   * Set the different relative integration radii from the field as constant
   * length array for python wrapping
   * @param radiiArray: array of size 10 containing the radii. if less radii
   * are desired, fill the remaining entries with zeros
   */
  void SetRelativeRadiiArray(double relativeRadiiArray[10]);

  /**
   * Get the different relative integration radii from the field as constant
   * length array for python wrapping
   * @param radiiArray: array of size 10 containing the radii. if less radii
   * are desired, fill the remaining entries with zeros
   */
  void GetRelativeRadiiArray(double relativeRadiiArray[10]);

  /**
   * Get the number of the different integration radii from the field
   */
  int GetNumberOfRadii() { return this->Radii.size(); };

  /**
   * Get the different integration radii from the field as string. Convenience
   * function.
   */
  std::string GetStringRadii(int i) { return std::to_string(this->Radii.at(i)).c_str(); };

  /**
   * Get the indices of a tensor component as a string. Convenience function.
   * @param index: the place in the tensor array described in vtkMomentsTensor.h
   * @param dimension: 2D or 3D
   * @param order: the maximal order up to which the moments are computed
   * @param fieldRank: 0 for scalar, 1 for vector and 2 for matrix
   */
  std::string GetStringTensorIndices(size_t index, int dimension, int order, int fieldRank);

protected:
  vtkComputeMoments();
  ~vtkComputeMoments() override;

  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /** main executive of the program, reads the input, calles the functions, and
   * produces the utput.
   * @param inputVector: the input information
   * @param outputVector: the output information
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkComputeMoments(const vtkComputeMoments&) = delete;
  void operator=(const vtkComputeMoments&) = delete;

  /**
   * the number of fields in the field
   * equals NumberOfBasisFunctions * numberOfRadii
   */
  size_t NumberOfFields;

  /**
   * the number of basis functions in the field
   * equals \sum_{i=0}^order dimension^o
   */
  size_t NumberOfBasisFunctions;

  /**
   * Dimension of the field can be 2 or 3
   */
  int Dimension;

  /**
   * Rank of the field is 0 for scalars, 1 for vectors, 3 for matrices
   */
  int FieldRank;

  /**
   * Maximal order up to which the moments are calculated
   */
  int Order;

  /**
   * Different integration radii in a vector
   */
  std::vector<double> Radii;

  /**
   * Different integration radii in a vector
   */
  std::vector<double> RelativeRadii;

  /**
   * How fine the discrete integration done in each dimension
   */
  int NumberOfIntegrationSteps;

  /**
   * If the field has multiple fields in its point data, you can choose the one
   * that the moments
   * shall be calculated of
   */
  std::string NameOfPointData;

  /**
   * The minimal extent of the dataset
   */
  double Extent;

  /**
   * Flag for using FFT option
   */
  bool UseFFT;

  /**
   * the agorithm has two input ports
   * port 0 is the dataset of which the moments are computed
   * port 1 is the grid at whose locations the moments are computed.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * the agorithm generates a field of vtkImageData storing the moments. It
   * will have numberOfFields scalar arrays in its pointdata it has the same
   * dimensions and topology as the second inputport
   */
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  /**
   * This Method is the main part that computes the moments.
   * @param radiusIndex: index of this radius in the radii vector
   * @param grid: the uniform grid on which the moments are computed
   * @param field: function of which the moments are computed
   * @param output: this vtkImageData has the same topology as grid and will
   * contain numberOfFields scalar fields, each containing one moment at all
   * positions
   */
  void Compute(size_t radiusIndex, vtkImageData* grid, vtkImageData* field, vtkImageData* output);

  /**
   * Make sure that the user has not entered weird values.
   * @param field: function of which the moments are computed
   */
  void CheckValidity(vtkImageData* field);

  /**
   * Find out the dimension and the date type of the field dataset.
   * @param field: function of which the moments are computed
   */
  void InterpretField(vtkImageData* field);

  /**
   * Build the output dataset.
   * @param grid: the uniform grid on which the moments are computed
   * @param output: this vtkImageData has the same topology as grid and will
   * contain numberOfFields scalar fields, each containing one moment at all
   * positions
   */
  void BuildOutput(vtkImageData* grid, vtkImageData* output);
};

#endif
