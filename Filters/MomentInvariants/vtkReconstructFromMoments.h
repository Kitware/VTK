/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReconstructFromMoments.h

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
 * @class   vtkReconstructFromMoments
 * @brief   reconstruct the underlying function from its moments
 *
 * vtkReconstructFromMoments is a filter that takes the momentData as
 * produced by vtkComputeMoments or vtkMomentInvariants and a grid. It
 * reconstructs the function from the moments, just like from the
 * coefficients of a taylor series. There are in principal three
 * applications.
 * 1. If we put in the moments of the pattern and the grid of the pattern,
 * we see what the algorithm can actually se during the pattern detection.
 * 2. If we put in the normalized moments of the pattern and the grid of the
 * pattern, we how the standard position looks like.
 * 3. If we put in the moments of the field and the original field data, we
 * can see how well the subset of points, on which the moments were computed,
 * actually represents the field For the reconstruction, we need to
 * orthonormalize the moments first. Then, we multiply the coefficients with
 * their corresponding basis function and add them up.
 * @par Thanks:
 * Developed by Roxana Bujack at Los Alamos National Laboratory.
 */

#ifndef vtkReconstructFromMoments_h
#define vtkReconstructFromMoments_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersMomentInvariantsModule.h" // For export macro

#include <vector> // For internal vector methods

class vtkImageData;

class VTKFILTERSMOMENTINVARIANTS_EXPORT vtkReconstructFromMoments : public vtkDataSetAlgorithm
{
public:
  static vtkReconstructFromMoments* New();

  vtkTypeMacro(vtkReconstructFromMoments, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * standard pipeline input for port 1
   * This is the data as produced by vtkComputeMoments or vtkMomentInvariants.
   */
  void SetMomentsData(vtkDataObject* input) { this->SetInputData(0, input); };

  /**
   * standard pipeline input for port 1
   * This is the data as produced by vtkComputeMoments or vtkMomentInvariants.
   */
  void SetMomentsConnection(vtkAlgorithmOutput* algOutput)
  {
    this->SetInputConnection(0, algOutput);
  };

  /**
   * standard pipeline input for port 1
   * This input steers the topology of the reconstructed field.
   */
  void SetGridData(vtkDataObject* input) { this->SetInputData(1, input); };

  /**
   * standard pipeline input for port 1
   * This input steers the topology of the reconstructed field.
   */
  void SetGridConnection(vtkAlgorithmOutput* algOutput) { this->SetInputConnection(1, algOutput); };

  //@{
  /**
   * Set/Get The reconstruction function returns zero if the point is outside the integration radius
   * of a vertex iff AllowExtrapolation is false
   */
  vtkSetMacro(AllowExtrapolation, bool);
  vtkGetMacro(AllowExtrapolation, bool);
  //@}

protected:
  /**
   * constructor setting defaults
   */
  vtkReconstructFromMoments();

  /**
   * destructor
   */
  ~vtkReconstructFromMoments() override;

  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /** main executive of the program, reads the input, calles the functions, and produces the utput.
   * @param request: ?
   * @param inputVector: the input information
   * @param outputVector: the output information
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkReconstructFromMoments(const vtkReconstructFromMoments&) = delete;
  void operator=(const vtkReconstructFromMoments&) = delete;

  /**
   * the number of fields in the source
   * equals NumberOfBasisFunctions * numberOfRadii
   */
  size_t NumberOfFields;

  /**
   * the number of basis functions in the source
   * equals \sum_{i=0}^order dimension^o
   */
  size_t NumberOfBasisFunctions;

  /**
   * Dimension of the source can be 2 or 3
   */
  int Dimension;

  /**
   * Rank of the source is 0 for scalars, 1 for vectors, 3 for matrices
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
   * If false, reconstruction outside the integration radius is set to zero
   */
  bool AllowExtrapolation;

  /**
   * the agorithm has two input ports
   * port 0 is the moments Data, which is a vtkImageData and the 0th output of vtkMomentInvariants
   * port 1 a finer grid that is used for the drawing of the circles and balls
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * the agorithm generates 1 output of the topology of the gridData
   */
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  /**
   * Make sure that the user has not entered weird values.
   * @param momentsData: the moments from which the reconstruction will be computed
   * @param gridData: the grid for the reconstruction
   */
  void CheckValidity(vtkImageData* momentsData, vtkDataSet* gridData);

  /**
   * Find out the dimension and the data type of the moments dataset.
   * @param momentsData: the moments from which the reconstruction will be computed
   */
  void InterpretGridData(vtkDataSet* gridData);

  /**
   * Find out the dimension and the data type of the moments dataset.
   * @param momentsData: the moments from which the reconstruction will be computed
   */
  void InterpretMomentsData(vtkImageData* momentsData);
};

#endif
