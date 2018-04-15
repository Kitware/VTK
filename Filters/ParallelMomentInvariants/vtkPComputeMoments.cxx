/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPComputeMoments.cxx

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
#include "vtkPComputeMoments.h"

#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkPointData.h"
#include "vtkPResampleWithDataSet.h"
#include "vtkProbeFilter.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkMomentsHelper.h"
#include "vtkMomentsTensor.h"

#include <vector>

/**
 * standard vtk new operator
 */
vtkStandardNewMacro(vtkPComputeMoments);
vtkSetObjectImplementationMacro(vtkPComputeMoments, Controller, vtkMultiProcessController);

/**
 * constructior setting defaults
 */
vtkPComputeMoments::vtkPComputeMoments()
{
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
  this->Dimension = 0;
  this->FieldRank = 0;
  this->Order = 2;
  this->Radii = std::vector<double>(0);
  this->RelativeRadii = std::vector<double>(0);
  for (int k = 5; k > 2; --k)
  {
    RelativeRadii.push_back(1. / pow(2.0, k));
  }
  this->NumberOfIntegrationSteps = 5;
  this->NameOfPointData = "no name set by user";
  this->Extent = 0;
  this->UseFFT = false;
}

/**
 * destructor
 */
vtkPComputeMoments::~vtkPComputeMoments()
{
}

/**
 * the agorithm has two input ports
 * port 0 is the dataset of which the moments are computed
 * port 1 is the grid at whose locations the moments are computed. if not set, the original grid is
 * chosen
 */
int vtkPComputeMoments::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
  }
  if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

/**
 * the agorithm generates a field of vtkImageData storing the moments. It will
 * have numberOfFields scalar arrays in its pointdata it has the same dimensions
 * and topology as the second inputport
 */
int vtkPComputeMoments::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  return 1;
}

/**
 * This function moves the stencil to the current location, where the integration is supposed o be
 * performed
 * @param center: the location
 * @param field: the dataset
 * @param stencil: contains the locations at which the dataset is evaluated for the integration
 * @param numberOfIntegrationSteps: how fine the discrete integration done in each dimension
 * @return 0 if the stencil lies completely outside the field
 */
bool vtkPComputeMoments::CenterStencil(double center[3], vtkDataSet* field, vtkImageData* stencil,
  int numberOfIntegrationSteps, std::string nameOfPointData)
{
  // put the center to the point where the moments shall be calculated
  if (numberOfIntegrationSteps == 1)
  {
    stencil->SetOrigin(center);
  }
  else
  {
    double bounds[6];
    stencil->GetBounds(bounds);
    stencil->SetOrigin(center[0] - 0.5 * (bounds[1] - bounds[0]),
      center[1] - 0.5 * (bounds[3] - bounds[2]), center[2] - 0.5 * (bounds[5] - bounds[4]));
  }

  //  int subId = 0;
  //
  //  // interpolation of the field data at the integration points
  //  for (vtkIdType ptId = 0; ptId < stencil->GetNumberOfPoints(); ++ptId)
  //  {
  //    // find point coordinates
  //    double x[3];
  //    stencil->GetPoint(ptId, x);
  //
  //    // find cell
  //    double pcoords[3];
  //    double* weights = new double[field->GetMaxCellSize()];
  //    vtkIdType cellId = field->FindCell(x, NULL, -1, 1, subId, pcoords, weights);
  //    vtkCell* cell;
  //    if (cellId >= 0)
  //    {
  //      cell = field->GetCell(cellId);
  //    }
  //    else
  //    {
  //      cell = 0;
  //    }
  //    if (cell)
  //    {
  //      // Interpolate the point data
  //      stencil->GetPointData()->InterpolatePoint(
  //                                                field->GetPointData(), ptId, cell->PointIds,
  //                                                weights);
  //    }
  //    else
  //    {
  //      return (false);
  //    }
  //  }

  vtkNew<vtkPResampleWithDataSet> resample;
  resample->SetController(this->Controller);
  resample->SetInputData(stencil);
  resample->SetSourceData(field);
  resample->Update();

  //  vtkNew<vtkProbeFilter> resample;
  //  resample->SetInputData(stencil);
  //  resample->SetSourceData(field);
  //  resample->Update();

  if (vtkImageData::SafeDownCast(resample->GetOutput())
        ->GetPointData()
        ->GetArray("vtkValidPointMask")
        ->GetRange()[1] == 0)
  {
    return (false);
  }

  stencil->GetPointData()->RemoveArray(nameOfPointData.c_str());
  stencil->GetPointData()->AddArray(vtkImageData::SafeDownCast(resample->GetOutput())
                                      ->GetPointData()
                                      ->GetArray(nameOfPointData.c_str()));

  //  if( this->Controller->GetLocalProcessId() == 0 && center[0] == 0 && center[1] == 0 )
  //  {
  //    std::ostream stream(std::cout.rdbuf());
  //    std::cout<<"stencil=";
  //    stencil->PrintSelf(stream, vtkIndent(0));
  //    std::cout<<"\n";
  //    std::cout<<"point="<<center[0]<<" "<<center[1]<<" range="<<stencil->GetScalarRange()[0]<<"
  //    "<<stencil->GetScalarRange()[1]<<" bounds="<<stencil->GetBounds()[0]<<"
  //    "<<stencil->GetBounds()[1]<<"\n";
  //    for (vtkIdType ptId = 0; ptId < stencil->GetNumberOfPoints(); ++ptId)
  //    {
  //      std::cout<<stencil->GetPointData()->GetArray(0)->GetTuple(ptId)[0]<<" ";
  //    }
  //    std::cout<<"\n";
  //  }

  return (true);
}

/**
 * This function handles the moment computation on the original resolution   * this is where all the
 * communication with the other procs happens
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
 * @param nameOfPointData: the name of the array in the point data of which the momens are computed.
 */
void vtkPComputeMoments::ComputeOrigRes(int dimension, int order, int fieldRank, size_t radiusIndex,
  vtkImageData* field, vtkImageData* grid, vtkImageData* output, std::string vtkNotUsed(nameOfPointData))
{

  double radius = this->Radii.at(radiusIndex);
  double bounds[6];
  field->GetBounds(bounds);
  int procId = 0;
  int numProcs = 1;
  const int MY_RETURN_VALUE_MESSAGE = 0x11;
  if (this->Controller)
  {
    procId = this->Controller->GetLocalProcessId();
    numProcs = this->Controller->GetNumberOfProcesses();
    //    std::cout<<"procId"<<procId<<"numProcs"<<numProcs<<"\n";
  }
  else
  {
    vtkErrorMacro("There is no controller set.");
  }

  // get all bounds in array procId * 6 + boundIndex. last row contains the global bounds
  double allBounds[6 * numProcs + 6];
  for (int p = 0; p < numProcs; ++p)
  {
    this->Controller->Send(bounds, 6, p, MY_RETURN_VALUE_MESSAGE);
    this->Controller->Receive(allBounds + p * 6, 6, p, MY_RETURN_VALUE_MESSAGE);
  }
  for (size_t d = 0; d < 3; ++d)
  {
    allBounds[6 * numProcs + 2 * d] = allBounds[2 * d];
    allBounds[6 * numProcs + 2 * d + 1] = allBounds[2 * d + 1];
  }
  for (int p = 1; p < numProcs; ++p)
  {
    for (size_t d = 0; d < 3; ++d)
    {
      allBounds[6 * numProcs + 2 * d] =
        std::min(allBounds[6 * numProcs + 2 * d], allBounds[6 * p + 2 * d]);
      allBounds[6 * numProcs + 2 * d + 1] =
        std::max(allBounds[6 * numProcs + 2 * d + 1], allBounds[6 * p + 2 * d + 1]);
    }
  }
  //  if( procId == 0 )
  //  {
  //    for (size_t p = 0; p < numProcs + 1; p++)
  //    {
  //      for (size_t d = 0; d < 6; d++)
  //      {
  //        std::cout<<allBounds[p*6+d]<<" ";
  //      }
  //      std::cout<<"\n";
  //    }
  //  }

  // this vector contains a vector for each proc. with the centers that are close to that proc's
  // boundary
  std::vector<std::vector<std::vector<double> > > myBoundaryCenters(numProcs);
  // compute (possibly partial) moments for all centers in this proc
  for (int ptId = 0; ptId < grid->GetNumberOfPoints(); ++ptId)
  {
    double center[3];
    grid->GetPoint(ptId, center);
    // determine the indices i and j per dimension of the center in this dataset.
    int dimPtId[dimension];
    for (size_t d = 0; d < static_cast<size_t>(dimension); ++d)
    {
      dimPtId[d] = (center[d] - bounds[2 * d]) / (field->GetSpacing()[d] - 1e-10);
    }
    // compute this process's part of the moments
    std::vector<vtkMomentsTensor> moments =
      vtkMomentsHelper::allMomentsOrigResImageData(this->Dimension, this->Order, this->FieldRank,
        this->Radii.at(radiusIndex), dimPtId, field, this->NameOfPointData);
    // put the moments into the corresponding array
    for (size_t k = 0; k < moments.size(); ++k)
    {
      for (size_t i = 0; i < moments.at(k).size(); ++i)
      {
        output->GetPointData()
          ->GetArray(vtkMomentsHelper::getFieldIndexFromTensorIndices(radiusIndex,
            moments.at(k).getIndices(i), this->Dimension, this->FieldRank,
            this->NumberOfBasisFunctions))
          ->SetTuple1(ptId, moments.at(k).get(i));
      }
    }

    // if the center is close to an edge, find which process it falls into, and add it to the
    // respective slot in myBoundaryCenters
    bool isBoundaryCenter = false;
    for (size_t d = 0; d < static_cast<size_t>(dimension); ++d)
    {
      if (center[d] - radius < bounds[2 * d] - 1e-10 ||
        center[d] + radius > bounds[2 * d + 1] + 1e-10)
      {
        isBoundaryCenter = true;
      }
    }
    if (isBoundaryCenter)
    {
      for (size_t p = 0; p < numProcs; ++p)
      {
        if (p != procId)
        {
          bool isInThisProc = true;
          for (size_t d = 0; d < static_cast<size_t>(dimension); ++d)
          {
            if ((center[d] + radius - 1e-10 < allBounds[6 * p + 2 * d] &&
                  center[d] - radius - 1e-10 < allBounds[6 * p + 2 * d]) ||
              (center[d] + radius + 1e-10 > allBounds[6 * p + 2 * d + 1] &&
                  center[d] - radius + 1e-10 > allBounds[6 * p + 2 * d + 1]))
            {
              isInThisProc = false;
            }
          }
          if (isInThisProc)
          {
            myBoundaryCenters.at(p).push_back(std::vector<double>(3));
            for (size_t d = 0; d < static_cast<size_t>(dimension); ++d)
            {
              myBoundaryCenters.at(p).back().at(d) = center[d];
            }
          }
        }
      }
    }
  }
  //  if (procId == 0)
  //  {
  //    for (size_t mc = 0; mc < myBoundaryCenters.at(3).size(); ++mc)
  //    {
  //      std::cout<<"myBoundaryCenters.at(3):
  //      "<<myBoundaryCenters.at(3).at(mc).at(0)<<","<<myBoundaryCenters.at(3).at(mc).at(1)<<"\n";
  //    }
  //  }

  // exchange boundary centers between nodes. distance is commutative. so we can send and receive
  // only to the nodes that we share centers with
  // then, compute the partly moments on each node, return them home, and add them up in their home
  // node
  for (int p = 0; p < numProcs; ++p)
  {
    if (myBoundaryCenters.at(p).size() > 0)
    {
      int numMyBoundaryCenters = myBoundaryCenters.at(p).size();
      double myBoundaryCentersArray[numMyBoundaryCenters * 3];
      for (int mc = 0; mc < numMyBoundaryCenters; ++mc)
      {
        for (size_t d = 0; d < 3; ++d)
        {
          *(myBoundaryCentersArray + 3 * mc + d) = myBoundaryCenters.at(p).at(mc).at(d);
        }
      }
      //      for (size_t mc = 0; mc < numMyBoundaryCenters; ++mc)
      //      {
      //        std::cout<<"myBoundaryCentersArray: "<<procId<<" "<<p<<"
      //        "<<*(myBoundaryCentersArray+mc)<<","<<*(myBoundaryCentersArray+mc+1)<<"\n";
      //      }

      int numForeignBoundaryCenters;
      if (procId > p)
      {
        this->Controller->Send(&numMyBoundaryCenters, 1, p, MY_RETURN_VALUE_MESSAGE);
        this->Controller->Receive(&numForeignBoundaryCenters, 1, p, MY_RETURN_VALUE_MESSAGE);
      }
      else
      {
        this->Controller->Receive(&numForeignBoundaryCenters, 1, p, MY_RETURN_VALUE_MESSAGE);
        this->Controller->Send(&numMyBoundaryCenters, 1, p, MY_RETURN_VALUE_MESSAGE);
      }
      //      std::cout<<"numForeignCenters: "<<procId<<" "<<p<<"
      //      "<<numForeignBoundaryCenters<<"\n";

      double foreignBoundaryCentersArray[numForeignBoundaryCenters * 3];
      if (procId > p)
      {
        this->Controller->Send(
          myBoundaryCentersArray, numMyBoundaryCenters * 3, p, MY_RETURN_VALUE_MESSAGE);
        this->Controller->Receive(
          foreignBoundaryCentersArray, numForeignBoundaryCenters * 3, p, MY_RETURN_VALUE_MESSAGE);
      }
      else
      {
        this->Controller->Receive(
          foreignBoundaryCentersArray, numForeignBoundaryCenters * 3, p, MY_RETURN_VALUE_MESSAGE);
        this->Controller->Send(
          myBoundaryCentersArray, numMyBoundaryCenters * 3, p, MY_RETURN_VALUE_MESSAGE);
      }
      std::vector<std::vector<double> > foreignBoundaryCenters(numForeignBoundaryCenters);
      for (int fc = 0; fc < numForeignBoundaryCenters; ++fc)
      {
        foreignBoundaryCenters.at(fc) = std::vector<double>(3);
        for (size_t d = 0; d < static_cast<size_t>(dimension); ++d)
        {
          foreignBoundaryCenters.at(fc).at(d) = *(foreignBoundaryCentersArray + 3 * fc + d);
        }
        //        if (procId == 3 && p == 0)
        //        {
        //          std::cout<<"foreignBoundaryCenters(fc):
        //          "<<foreignBoundaryCenters.at(fc).at(0)<<","<<foreignBoundaryCenters.at(fc).at(1)<<"\n";
        //        }
      }

      // compute the moments of the foreign centers
      std::vector<std::vector<vtkMomentsTensor> > foreignBoundaryMoments(numForeignBoundaryCenters);
      for (int fc = 0; fc < numForeignBoundaryCenters; ++fc)
      {
        foreignBoundaryMoments.at(fc) = std::vector<vtkMomentsTensor>(order + 1);
        for (int o = 0; o < order + 1; o++)
        {
          foreignBoundaryMoments.at(fc).at(o) =
            vtkMomentsTensor(dimension, o + fieldRank, fieldRank);
        }
        // determine the possibly negative indices i and j per dimension of the foreign center in
        // this field.
        int dimPtId[dimension];
        for (size_t d = 0; d < static_cast<size_t>(dimension); ++d)
        {
          dimPtId[d] = (foreignBoundaryCenters.at(fc).at(d) - bounds[2 * d]) /
            (field->GetSpacing()[d] - 1e-10);
        }
        //        if (procId == 3 && p == 0)
        //        {
        //          std::cout<<"centers and bounds: "<<foreignBoundaryCenters.at(fc).at(0)<<"
        //          "<<bounds[2*0]<<" "<<foreignBoundaryCenters.at(fc).at(0) - bounds[2 * 0] -
        //          1e-10<<" "<<field->GetSpacing()[0]<<": "<<dimPtId[0]<<" "<<dimPtId[1]<<"\n";
        //        }
        // compute part of the moments
        foreignBoundaryMoments.at(fc) =
          vtkMomentsHelper::allMomentsOrigResImageData(this->Dimension, this->Order,
            this->FieldRank, this->Radii.at(radiusIndex), dimPtId, field, this->NameOfPointData);
        ////        if (procId == 2)
        //        {
        ////          for (int o = 0; o < order + 1; o++)
        ////          {
        ////            for (int i = 0; i < foreignBoundaryMoments.at(fc).at(o).size(); ++i)
        ////            {
        //              std::cout<<procId<<" foreignBoundaryMoments.at(fc).at(order).at(back):
        //              "<<foreignBoundaryMoments.at(fc).at(order).get(15)<<"\n";
        ////            }
        ////          }
        //        }
      }

      // send the partly moments back home
      double foreignBoundaryMomentsArray[numForeignBoundaryCenters * this->NumberOfBasisFunctions];
      for (int fc = 0; fc < numForeignBoundaryCenters; ++fc)
      {
        size_t index = 0;
        for (int o = 0; o < order + 1; o++)
        {
          for (size_t i = 0; i < foreignBoundaryMoments.at(fc).at(o).size(); ++i)
          {
            foreignBoundaryMomentsArray[fc * this->NumberOfBasisFunctions + index + i] =
              foreignBoundaryMoments.at(fc).at(o).get(i);
          }
          index += foreignBoundaryMoments.at(fc).at(o).size();
        }
      }
      ////      if (procId == 2)
      //      {
      //        for (size_t fc = 0; fc < numForeignBoundaryCenters; ++fc)
      //        {
      ////          for (int i = 0; i < this->NumberOfBasisFunctions; ++i)
      //          {
      //            std::cout<<procId<<" "<<fc<<" foreignBoundaryMomentsArray(fc)(back):
      //            "<<foreignBoundaryMomentsArray[fc * this->NumberOfBasisFunctions +
      //            this->NumberOfBasisFunctions - 1]<<"\n";
      //          }
      //        }
      //      }

      double myBoundaryMomentsArray[numMyBoundaryCenters * this->NumberOfBasisFunctions];
      if (procId > p)
      {
        this->Controller->Send(foreignBoundaryMomentsArray,
          numForeignBoundaryCenters * this->NumberOfBasisFunctions, p, MY_RETURN_VALUE_MESSAGE);
        this->Controller->Receive(myBoundaryMomentsArray,
          numMyBoundaryCenters * this->NumberOfBasisFunctions, p, MY_RETURN_VALUE_MESSAGE);
      }
      else
      {
        this->Controller->Receive(myBoundaryMomentsArray,
          numMyBoundaryCenters * this->NumberOfBasisFunctions, p, MY_RETURN_VALUE_MESSAGE);
        this->Controller->Send(foreignBoundaryMomentsArray,
          numForeignBoundaryCenters * this->NumberOfBasisFunctions, p, MY_RETURN_VALUE_MESSAGE);
      }
      for (int mc = 0; mc < numMyBoundaryCenters; ++mc)
      {
        for (size_t i = 0; i < NumberOfBasisFunctions; ++i)
        {
          int ptId = grid->FindPoint(myBoundaryCentersArray + 3 * mc);
          int fieldIndex = radiusIndex * NumberOfBasisFunctions + i;
          output->GetPointData()
            ->GetArray(fieldIndex)
            ->SetTuple1(ptId, output->GetPointData()->GetArray(fieldIndex)->GetTuple(ptId)[0] +
                *(myBoundaryMomentsArray + NumberOfBasisFunctions * mc + i));
        }
      }
    }
  }
}

/**
 * This Method is the main part that computes the moments.
 * @param radiusIndex: index of this radius in the radii vector
 * @param grid: the uniform grid on which the moments are computed
 * @param field: function of which the moments are computed
 * @param output: this vtkImageData has the same topology as grid and will contain numberOfFields
 * scalar fields, each containing one moment at all positions
 */
void vtkPComputeMoments::Compute(
  size_t radiusIndex, vtkImageData* grid, vtkImageData* field, vtkImageData* output)
{
  // std::cout << "vtkPComputeMoments::Compute \n";
  if (this->UseFFT)
  {
    vtkErrorMacro("The FFT option is currently not available in a parallel setting.");
    return;
  }
  else if (this->NumberOfIntegrationSteps == 0)
  {
    this->ComputeOrigRes(this->Dimension, this->Order, this->FieldRank, radiusIndex, field, grid,
      output, this->NameOfPointData);
  }
  else
  {
    // std::cout << "sampling active for stencil, because numberOfIntegratioSteps > 0 \n";
    vtkImageData* stencil = vtkImageData::New();
    vtkMomentsHelper::BuildStencil(stencil, this->Radii.at(radiusIndex),
      this->NumberOfIntegrationSteps, this->Dimension, field, this->NameOfPointData);
    for (int ptId = 0; ptId < grid->GetNumberOfPoints(); ++ptId)
    {
      // Get the xyz coordinate of the point in the grid dataset
      double center[3];
      grid->GetPoint(ptId, center);
      if (this->CenterStencil(
            center, field, stencil, this->NumberOfIntegrationSteps, this->NameOfPointData))
      {
        // get all the moments
        std::vector<vtkMomentsTensor> tensorVector =
          vtkMomentsHelper::allMoments(this->Dimension, this->Order, this->FieldRank,
            this->Radii.at(radiusIndex), center, stencil, this->NameOfPointData);

        // std::vector< vtkMomentsTensor > orthonormalTensorVector = this->orthonormalizeMoments(
        // this->Dimension, tensorVector, this->Radii.at(radiusIndex), stencil );
        //            tensorVector = orthonormalTensorVector;
        //            if(ptId == 100)
        //            {
        //                for( size_t i = 0; i < tensorVector.size(); ++i )
        //                {
        //                    tensorVector.at(i).print();
        //                }
        //                std::vector< vtkMomentsTensor > orthonormalTensorVector =
        //                orthonormalizeMoments( this->Dimension, tensorVector,
        //                this->Radii.at(radiusIndex) ); for( size_t i = 0; i < tensorVector.size();
        //                ++i )
        //                {
        //                    orthonormalTensorVector.at(i).print();
        //                }
        //            }

        // put them into the corresponding array
        for (size_t k = 0; k < tensorVector.size(); ++k)
        {
          for (size_t i = 0; i < tensorVector.at(k).size(); ++i)
          {
            output->GetPointData()
              ->GetArray(vtkMomentsHelper::getFieldIndexFromTensorIndices(radiusIndex,
                tensorVector.at(k).getIndices(i), this->Dimension, this->FieldRank,
                this->NumberOfBasisFunctions))
              ->SetTuple1(ptId, tensorVector.at(k).get(i));
            // std::cout<<"radius="<<this->Radii.at(radiusIndex)<<"
            // fieldIndex="<<vtkMomentsHelper::getFieldIndexFromTensorIndices( radiusIndex,
            // tensorVector.at(k).getIndices(i), this->Dimension, this->FieldRank,
            // this->NumberOfBasisFunctions )<<" value="<<tensorVector.at(k).get(i)<<"\n";
          }
        }
      }
      else
      {
        for (size_t i = 0; i < NumberOfBasisFunctions; ++i)
        {
          output->GetPointData()
            ->GetArray(radiusIndex * NumberOfBasisFunctions + i)
            ->SetTuple1(ptId, 0.0);
        }
      }
      // cout<<ptId<<" center="<<center[0]<<" "<<center[1]<<" "<<center[2]<<"
      // "<<outPD->GetArray(0)->GetTuple(ptId)[0]<<"\n";
    }
    stencil->Delete();
  }
  //#ifdef MYDEBUG
  //    std::ostream stream(std::cout.rdbuf());
  //    std::cout<<"output=";
  //    output->PrintSelf(stream, vtkIndent(0));
  //    std::cout<<"\n";
  //#endif
}

int vtkPComputeMoments::RequestUpdateExtent(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
{
  // We need to ask for the whole extent from this input.
  vtkInformation* momentsInfo = inputVector[0]->GetInformationObject(0);

  momentsInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  momentsInfo->Remove(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  if (momentsInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
  {
    momentsInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      momentsInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  }

  momentsInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  momentsInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  momentsInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);

  vtkInformation* gridInfo = inputVector[1]->GetInformationObject(0);

  if (gridInfo)
  {
    gridInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

    gridInfo->Remove(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
    if (gridInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
    {
      gridInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        gridInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
    }

    gridInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
    gridInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
    gridInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  }
  return 1;
}

/**
 * main executive of the program, reads the input, calles the
 * functions, and produces the utput.
 * @param inputVector: the input information
 * @param outputVector: the output information
 */
int vtkPComputeMoments::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* fieldInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* gridInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkImageData* field = vtkImageData::SafeDownCast(fieldInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData* grid;
  if (gridInfo)
  {
    grid = vtkImageData::SafeDownCast(gridInfo->Get(vtkDataObject::DATA_OBJECT()));
  }
  else
  {
    grid = field;
  }
  vtkImageData* output = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (field)
  {
    this->InterpretField(field);
    this->CheckValidity(field, grid);
    this->BuildOutput(grid, output);
    for (size_t radiusIndex = 0; radiusIndex < this->Radii.size(); ++radiusIndex)
    {
      this->Compute(radiusIndex, grid, field, output);
    }
  }
  return 1;
}
