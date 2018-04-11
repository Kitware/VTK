/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPComputeMoments.h

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
 * @class   vtkPComputeMoments
 * @brief   rotation invariant pattern detetction
 *
 * vtkPComputeMoments is a the distributed memory version of vtkComputeMoments.
 * Look there for more information.
 * @par Thanks:
 * Developed by Roxana Bujack at Los Alamos National Laboratory.
 */

#ifndef vtkPComputeMoments_h
#define vtkPComputeMoments_h

#include "vtkComputeMoments.h"
#include "vtkFiltersParallelMomentInvariantsModule.h" // For export macro

class vtkMultiProcessController;

class VTKFILTERSPARALLELMOMENTINVARIANTS_EXPORT vtkPComputeMoments : public vtkComputeMoments
{
public:
  static vtkPComputeMoments* New();

  vtkTypeMacro(vtkPComputeMoments, vtkDataSetAlgorithm);

  /**
   * Set/Get the MPI multi process controller object.
   */
  void SetController(vtkMultiProcessController* c);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

protected:
  vtkPComputeMoments();
  ~vtkPComputeMoments() override;

  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * main executive of the program, reads the input, calles the
   * functions, and produces the utput.
   * @param inputVector: the input information
   * @param outputVector: the output information
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  /**
   * MPI multi process controller object.
   */
  vtkMultiProcessController* Controller;

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
   * This function moves the stencil to the current location, where the integration is supposed o be
   * performed
   * @param center: the location
   * @param source: the dataset
   * @param stencil: contains the locations at which the dataset is evaluated for the integration
   * @param numberOfIntegrationSteps: how fine the discrete integration done in each dimension
   * @return 0 if the stencil lies completely outside the field
   */
  bool CenterStencil(double center[3], vtkDataSet* source, vtkImageData* stencil,
    int numberOfIntegrationSteps, std::string vtkNotUsed(nameOfPointData));

  /**
   * This function handles the moment computation on the original resolution     * this is where all
   * the communication with the other procs happens
   * 1. it computes the (partial) moments for all points on this grid
   * 2. it looks where points close to the boundary fall in the bounds of other procs and sends the
   * locations over as partly negative dimension-wise indizes of imageData
   * 3. each proc computes the parts of the momenrts in its domain and sends the results back
   * 4. in each home proc, the native and incoming moment parts are added up
   * the moments are the projections of the function to the monomial basis
   * they are evaluated using a numerical integration over the original dataset if it is structured
   * data
   * @param dimension: 2D or 3D
   * @param order: the maximal order up to which the moments are computed
   * @param fieldRank: 0 for scalar, 1 for vector and 2 for matrix
   * @param radiusIndex: index of this radius in the radii vector
   * @param field: the dataset of which the moments are computed
   * @param grid: the uniform grid on which the moments are computed
   * @param output: this vtkImageData has the same topology as grid and will
   * @param nameOfPointData: the name of the array in the point data of which the momens are
   * computed.
   */
  void ComputeOrigRes(int dimension, int order, int fieldRank, size_t radiusIndex,
    vtkImageData* field, vtkImageData* grid, vtkImageData* output, std::string nameOfPointData);
};

#endif
