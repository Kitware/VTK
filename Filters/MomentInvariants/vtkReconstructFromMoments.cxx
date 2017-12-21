/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReconstructFromMoments.cxx

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
#include "vtkReconstructFromMoments.h"
#include "vtkMomentsHelper.h"
#include "vtkMomentsTensor.h"

#include "vtkCell.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTuple.h"

#include <Eigen/Dense>
#include <vector>

/**
 * standard vtk new operator
 */
vtkStandardNewMacro(vtkReconstructFromMoments);

/**
 * constructor setting defaults
 */
vtkReconstructFromMoments::vtkReconstructFromMoments()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
  this->AllowExtrapolation = 0;
}

/**
 * destructor
 */
vtkReconstructFromMoments::~vtkReconstructFromMoments() {}

/** standard vtk print function
 * @param os: the way how to print
 * @param indent: how far to the right the text shall appear
 */
void vtkReconstructFromMoments::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

/**
 * the agorithm has two input ports
 * port 0 is the moments Data, which is a vtkImageData and the 0th output of vtkMomentInvariants
 * port 1 a finer grid that is used for the drawing of the circles and balls
 */
int vtkReconstructFromMoments::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
  }
  if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
  }
  return 1;
}

/**
 * the agorithm generates 1 output of the topology of the gridData
 */
int vtkReconstructFromMoments::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataSet");
  return 1;
}

/**
 * Euclidean distance between two 3D vectors
 * @param p: vector
 * @param q: vector
 * @return: the distance
 */
double dist(double* p, double* q)
{
  return sqrt(pow(p[0] - q[0], 2) + pow(p[1] - q[1], 2) + pow(p[2] - q[2], 2));
}

/**
 * Euclidean distance between two vtkTuples of arbitrary size
 * @param p: vtkTuple
 * @param q: vtkTuple
 * @return: the distance
 */
template<size_t S>
double dist(vtkTuple<double, S> p, vtkTuple<double, S> q)
{
  double dist = 0;
  for (int i = 0; i < S; ++i)
  {
    dist += pow(p[i] - q[i], 2);
  }
  return sqrt(dist);
}

/**
 * Find out the dimension and the data type of the moments dataset.
 * @param momentsData: the moments from which the reconstruction will be computed
 */
void vtkReconstructFromMoments::InterpretGridData(vtkDataSet* gridData)
{
  // dimension
  double bounds[6];
  gridData->GetBounds(bounds);
  if (bounds[5] - bounds[4] < 1e-10)
  {
    this->Dimension = 2;
  }
  else
  {
    this->Dimension = 3;
  }
  // std::cout << "this->Dimension =" << this->Dimension << "\n";
}

/**
 * Find out the dimension and the data type of the moments dataset.
 * @param momentsData: the moments from which the reconstruction will be computed
 */
void vtkReconstructFromMoments::InterpretMomentsData(vtkImageData* moments)
{
  if (moments->GetPointData()->GetNumberOfArrays() == 0)
  {
    vtkErrorMacro("The similarityData does not contain any pointdata.");
    return;
  }

  // numer of fields
  this->NumberOfFields = moments->GetPointData()->GetNumberOfArrays();
  // std::cout << "this->NumberOfFields =" << this->NumberOfFields << "\n";

  // fieldrank
  std::string name = moments->GetPointData()->GetArrayName(0);
  this->FieldRank = name.length() - 1 - name.find("x");
  if (this->FieldRank < 0 || this->FieldRank > 2)
  {
    vtkErrorMacro("the field rank of the moments must be 0, 1, or 2, but is " << this->FieldRank);
  }

  name = moments->GetPointData()->GetArrayName(this->NumberOfFields - 1);
  char buffer = name.back();
  // std::cout<<"buffer="<<buffer <<"end \n";
  if (!(buffer == '2' || buffer == '1' || buffer == 'x'))
  {
    vtkErrorMacro("index of the last moment field must end with 1 or 2.");
  }
  if ((buffer == '2' && this->Dimension == 2) || (buffer == '1' && this->Dimension == 3))
  {
    vtkErrorMacro("the dimensions of the domain and the moments must match.");
  }

  // order
  this->Order = name.length() - 1 - name.find("x") - this->FieldRank;
  // std::cout << "this->Order = " << this->Order << "\n";

  // radii
  this->Radii = std::vector<double>(0);
  for (size_t i = 0; i < this->NumberOfFields; ++i)
  {
    name = moments->GetPointData()->GetArrayName(i);
    char buffer2[40];
    name.copy(buffer2, name.find("index") - name.find("s") - 1, name.find("s") + 1);
    double radius = std::atof(buffer2);
    //        std::cout<<"buffer2="<<buffer2 <<"end \n";
    //        std::cout<<"radius="<<radius <<"\n";
    if (this->Radii.size() == 0 || radius != this->Radii.back())
    {
      this->Radii.push_back(radius);
    }
  }
  // std::cout << "this->Radii.size()=" << this->Radii.size() << "\n";

  // number of basis functions
  if (this->NumberOfFields % this->Radii.size() == 0)
  {
    this->NumberOfBasisFunctions = this->NumberOfFields / this->Radii.size();
  }
  else
  {
    vtkErrorMacro("the number of fields in moments has to be a multiple of the number of radii.");
  }
}

/**
 * Make sure that the user has not entered weird values.
 * @param momentsData: the moments from which the reconstruction will be computed
 * @param gridData: the grid for the reconstruction
 */
void vtkReconstructFromMoments::CheckValidity(vtkImageData* momentsData, vtkDataSet* gridData)
{
  double momentsBounds[6];
  momentsData->GetBounds(momentsBounds);

  double gridBounds[6];
  gridData->GetBounds(gridBounds);

  for (size_t r = 0; r < this->Radii.size(); ++r)
  {
    if (momentsBounds[1] - momentsBounds[0] > 0)
    {
      for (int i = 0; i < 3; ++i)
      {
        if (momentsBounds[2 * i] > gridBounds[2 * i] + 1e-5 ||
          momentsBounds[2 * i + 1] < gridBounds[2 * i + 1] - 1e-5)
        {
          vtkErrorMacro(
            "The grid is bigger than the moments field. The reconstruction will be zero there.");
          return;
        }
      }
    }
    else
    {
      for (int i = 0; i < 3; ++i)
      {
        if (momentsBounds[2 * i] - this->Radii.at(r) > gridBounds[2 * i] + 1e-5 ||
          momentsBounds[2 * i + 1] + this->Radii.at(r) < gridBounds[2 * i + 1] - 1e-5)
        {
          vtkErrorMacro("The grid is more than the moments' radius bigger than the moments field. "
                        "The reconstruction will have to extrapolate, which is highly unreliable.");
          return;
        }
      }
    }
    double spacing = momentsData->GetSpacing()[0];
    for (int i = 1; i < this->Dimension; ++i)
    {
      spacing = std::min(spacing, momentsData->GetSpacing()[i]);
    }
    if (this->Radii.at(r) < spacing - 1e-5)
    {
      vtkErrorMacro("The cells are bigger than at least one of the integration radii. If "
                    "AllowExtrapolation is false, the reconstruction will be zero there.");
      return;
    }
  }
}

/**
 * The monomial basis is not orthonormal. We need this function for the reconstruction of the
 * function from the moments. This function uses Gram Schmidt
 * @param dimension: 2D or 3D
 * @param moments: the moments at a point
 * @param radius: the corresponding integration radius
 * @return the orthonormal moments
 */
std::vector<vtkMomentsTensor> orthonormalizeMoments(int dimension,
  std::vector<vtkMomentsTensor> moments,
  double radius)
{
  Eigen::VectorXd b = Eigen::VectorXd::Zero(moments.size() * moments.back().size());
  for (size_t k = 0; k < moments.size(); ++k)
  {
    for (size_t i = 0; i < moments.at(k).size(); ++i)
    {
      b(i + k * moments.back().size()) = moments.at(k).get(i);
    }
  }

  Eigen::MatrixXd A = Eigen::MatrixXd::Zero(
    moments.size() * moments.back().size(), moments.size() * moments.back().size());
  for (size_t k1 = 0; k1 < moments.size(); ++k1)
  {
    for (size_t i1 = 0; i1 < moments.at(k1).size(); ++i1)
    {
      for (size_t k2 = 0; k2 < moments.size(); ++k2)
      {
        for (size_t i2 = 0; i2 < moments.at(k2).size(); ++i2)
        {
          if (moments.at(k1).getFieldIndices(i1) == moments.at(k2).getFieldIndices(i2))
          {
            if (dimension == 2)
            {
              A(i1 + k1 * moments.back().size(), i2 + k2 * moments.back().size()) =
                vtkMomentsHelper::translationFactorAnalytic(radius,
                  2,
                  moments.at(k1).getOrders(i1).at(0) + moments.at(k2).getOrders(i2).at(0),
                  moments.at(k1).getOrders(i1).at(1) + moments.at(k2).getOrders(i2).at(1),
                  0);
            }
            else
            {
              A(i1 + k1 * moments.back().size(), i2 + k2 * moments.back().size()) =
                vtkMomentsHelper::translationFactorAnalytic(radius,
                  3,
                  moments.at(k1).getOrders(i1).at(0) + moments.at(k2).getOrders(i2).at(0),
                  moments.at(k1).getOrders(i1).at(1) + moments.at(k2).getOrders(i2).at(1),
                  moments.at(k1).getOrders(i1).at(2) + moments.at(k2).getOrders(i2).at(2));
            }
          }
        }
      }
    }
  }

  //        Eigen::MatrixXd IA = Eigen::MatrixXd::Zero( moments.size() * moments.back().size(),
  //                                                   moments.size() * moments.back().size() );
  //        for( size_t k1 = 0; k1 < moments.size(); ++k1 )
  //        {
  //            for( size_t i1 = 0; i1 < moments.at( k1 ).size(); ++i1 )
  //            {
  //                for( size_t k2 = 0; k2 < moments.size(); ++k2 )
  //                {
  //                    for( size_t i2 = 0; i2 < moments.at( k2 ).size(); ++i2 )
  //                    {
  //                        if( moments.at( k1 ).getFieldIndices( i1 ) == moments.at( k2
  //                                                                                 ).getFieldIndices(
  //                                                                                 i2 ) )
  //                        {
  //                            for( size_t d = 0; d < dimension; ++d )
  //                            {
  //                                IA( i1 + k1 * moments.back().size(), i2 + k2 *
  //                                moments.back().size() )
  //                                += ( moments.at( k1 ).getOrders( i1 ).at( d ) + moments.at( k2
  //                                                                                           ).getOrders(
  //                                                                                           i2
  //                                                                                           ).at(
  //                                                                                           d ) )
  //                                                                                           *
  //                                                                                           pow(
  //                                                                                           10, d
  //                                                                                           );
  //                            }
  //                        }
  //                    }
  //                }
  //            }
  //        }
  //        Eigen::VectorXd Ib = Eigen::VectorXd::Zero( moments.size() * moments.back().size() );
  //        for( size_t k = 0; k < moments.size(); ++k )
  //        {
  //            for( size_t i = 0; i < moments.at( k ).size(); ++i )
  //            {
  //                for( size_t d = 0; d < dimension; ++d )
  //                {
  //                    Ib( i + k * moments.back().size() ) += moments.at( k ).getOrders( i ).at( d
  //                    ) * pow( 10, d );
  //                }
  //            }
  //        }
  //        std::cout<<"b="<<b<<endl;
  //        std::cout<<"Ib="<<Ib<<endl;
  //        std::cout<<"A="<<A<<endl;
  //        std::cout<<"IA="<<IA<<endl;

  Eigen::VectorXd x = A.colPivHouseholderQr().solve(b);
  //        std::cout<<"x="<<x<<endl;
  //        assert( b.isApprox( A * x ) );
  std::vector<vtkMomentsTensor> orthonormalMoments(moments.size());
  for (size_t k = 0; k < moments.size(); ++k)
  {
    orthonormalMoments.at(k) = vtkMomentsTensor(moments.at(k));
    for (size_t i = 0; i < moments.at(k).size(); ++i)
    {
      orthonormalMoments.at(k).set(i, x(i + k * moments.back().size()));
    }
  }

  //        std::cout<<"x="<<x<<endl;
  //        std::cout<<"Ax-b="<<A*x-b<<endl;
  //        for( size_t k = 0; k < moments.size(); ++k )
  //        {
  //            moments.at( k ).print();
  //            orthonormalMoments.at( k ).print();
  //        }
  //        cout<<"orthonormalMoments"<<endl;
  //        for( size_t k = 0; k < moments.size(); ++k )
  //        {
  //            orthonormalMoments.at( k ).print();
  //        }

  return orthonormalMoments;
}

/**
 * this functions uses the moments, weighs them with their corresponding basis function and adds
 * them up to approximate the value of the original function. The more moments are given, the better
 * the approximation, like in a taylor series
 * @param p: the location (3D point) at which the reconstructed field is evaluated
 * @param moments: the moments at a given location, which is used for the reconstruction
 * @param center: location, where the moments are given
 * @return: the value of the reconstructed function can be scalar, vector, or matrix valued
 */
template<size_t S>
vtkTuple<double, S> Reconstruct(double* p,
  std::vector<vtkMomentsTensor> moments,
  double* center,
  double radius,
  int vtkNotUsed(allowExtrapolation))
{
  vtkTuple<double, S> value;
  for (size_t s = 0; s < S; ++s)
  {
    value[s] = 0;
  }
  if (dist(p, center) <= radius)
  {
    for (size_t k = 0; k < moments.size(); ++k)
    {
      for (size_t i = 0; i < moments.at(k).size(); ++i)
      {
        std::vector<size_t> indices = moments.at(k).getMomentIndices(i);
        double factor = 1;
        for (size_t j = 0; j < moments.at(k).getMomentRank(); ++j)
        {
          factor *= (p[moments.at(k).getMomentIndices(i).at(j)] -
            center[moments.at(k).getMomentIndices(i).at(j)]);
        }
        value[moments.at(k).getFieldIndex(i)] += factor * moments.at(k).get(i);
      }
    }
  }
  return value;
}

/**
 * This function calles Reconstruct for all vertices of the cell in momentsData in which the point
 * lies then it interpolates it returns zero if the point is outside the integration radius of a
 * vertex iff AllowExtrapolation is false
 * @param ptId: the Id in the gridData of the location (3D point) at which the reconstructed field
 * is evaluated
 * @param orthonormalMomentsTensors: the orthonormalized moments stored ad momentsTensors
 * @param momentsData: the moments from which the reconstruction will be computed
 * @param gridData: the topology of the output on which the reconstruction will be computed
 * @param radiusId: the index of the current radius
 */
template<size_t S>
vtkTuple<double, S> ReconstructFromCell(int ptId,
  std::vector<std::vector<vtkMomentsTensor> >& orthonormalMomentsTensors,
  vtkImageData* momentsData,
  vtkDataSet* gridData,
  int radiusId,
  std::vector<double> radii,
  bool allowExtrapolation)
{
  int subId = 0;
  double pcoords[3];
  double* weights = new double[momentsData->GetMaxCellSize()];
  vtkCell* cell;
  vtkTuple<double, S> value;
  for (size_t s = 0; s < S; ++s)
  {
    value[s] = 0;
  }
  cell = momentsData->GetCell(
    momentsData->FindCell(gridData->GetPoint(ptId), NULL, -1, 1, subId, pcoords, weights));
  if (cell)
  {
    for (int i = 0; i < cell->GetNumberOfPoints(); ++i)
    {
      if (!allowExtrapolation &&
        dist(gridData->GetPoint(ptId), momentsData->GetPoint(cell->GetPointId(i))) >
          radii.at(radiusId))
      {
        for (size_t s = 0; s < S; ++s)
        {
          value[s] = 0;
        }
        break;
      }
      else
      {
        for (size_t s = 0; s < S; ++s)
        {
          value[s] += weights[i] *
            Reconstruct<S>(gridData->GetPoint(ptId),
              orthonormalMomentsTensors.at(cell->GetPointId(i)),
              momentsData->GetPoint(cell->GetPointId(i)),
              radii.at(radiusId),
              allowExtrapolation)[s];
        }
      }
    }
  }
  return value;
}

/**
 * This is the main function. It computes the reconstructed values in reconstructionData
 * its topology will be theone of grid
 * it will have a number of fields equal to the radii
 * @param similarityData: the similarity over the different radii
 * @param momentsData: the moments from which the reconstruction will be computed
 * @param gridData: the topology of the output on which the reconstruction will be computed
 */
void ComputeReconstruction(vtkDataSet* reconstructionData,
  vtkImageData* momentsData,
  vtkDataSet* gridData,
  std::vector<double> radii,
  bool allowExtrapolation,
  int order,
  int dimension,
  int numberOfBasisFunctions,
  int fieldRank)
{
  double momentsBounds[6];
  momentsData->GetBounds(momentsBounds);
  reconstructionData->CopyStructure(gridData);
  // fill tensors and orthonormalize them
  for (size_t r = 0; r < radii.size(); ++r)
  {
    std::vector<std::vector<vtkMomentsTensor> > momentsTensors(momentsData->GetNumberOfPoints());
    std::vector<std::vector<vtkMomentsTensor> > orthonormalMomentsTensors(
      momentsData->GetNumberOfPoints());
    for (int ptId = 0; ptId < momentsData->GetNumberOfPoints(); ++ptId)
    {
      momentsTensors.at(ptId) = std::vector<vtkMomentsTensor>(order + 1);
      for (int o = 0; o < order + 1; ++o)
      {
        momentsTensors.at(ptId).at(o) = vtkMomentsTensor(dimension, o + fieldRank, fieldRank);
        for (size_t i = 0; i < momentsTensors.at(ptId).at(o).size(); ++i)
        {
          momentsTensors.at(ptId).at(o).set(i,
            momentsData->GetPointData()
              ->GetArray(vtkMomentsHelper::getFieldIndexFromTensorIndices(r,
                momentsTensors.at(ptId).at(o).getIndices(i),
                dimension,
                fieldRank,
                numberOfBasisFunctions))
              ->GetTuple(ptId)[0]);
        }
      }
      orthonormalMomentsTensors.at(ptId) =
        orthonormalizeMoments(dimension, momentsTensors.at(ptId), radii.at(r));
    }
    vtkDoubleArray* reconstructionArray = vtkDoubleArray::New();
    reconstructionArray->SetName(("radius" + std::to_string(radii.at(r))).c_str());
    if (fieldRank == 0)
    {
      vtkTuple<double, 1> value;
      reconstructionArray->SetNumberOfComponents(1);
      reconstructionArray->SetNumberOfTuples(gridData->GetNumberOfPoints());
      for (int ptId = 0; ptId < gridData->GetNumberOfPoints(); ++ptId)
      {
        if (momentsBounds[1] - momentsBounds[0] > 0)
        {
          value = ReconstructFromCell<1>(
            ptId, orthonormalMomentsTensors, momentsData, gridData, r, radii, allowExtrapolation);
        }
        else
        {
          value = Reconstruct<1>(gridData->GetPoint(ptId),
            orthonormalMomentsTensors.at(0),
            momentsData->GetPoint(0),
            radii.at(r),
            allowExtrapolation);
        }
        reconstructionArray->SetTuple1(ptId, value[0]);
      }
      reconstructionData->GetPointData()->AddArray(reconstructionArray);
      reconstructionArray->Delete();
    }
    else if (fieldRank == 1)
    {
      vtkTuple<double, 3> value;
      reconstructionArray->SetNumberOfComponents(3);
      reconstructionArray->SetNumberOfTuples(gridData->GetNumberOfPoints());
      for (int ptId = 0; ptId < gridData->GetNumberOfPoints(); ++ptId)
      {
        if (momentsBounds[1] - momentsBounds[0] > 0)
        {
          value = ReconstructFromCell<3>(
            ptId, orthonormalMomentsTensors, momentsData, gridData, r, radii, allowExtrapolation);
        }
        else
        {
          value = Reconstruct<3>(gridData->GetPoint(ptId),
            orthonormalMomentsTensors.at(0),
            momentsData->GetPoint(0),
            radii.at(r),
            allowExtrapolation);
        }
        reconstructionArray->SetTuple3(ptId, value[0], value[1], value[2]);
      }
      reconstructionData->GetPointData()->AddArray(reconstructionArray);
      reconstructionArray->Delete();
    }
    else if (fieldRank == 2)
    {
      vtkTuple<double, 9> value;
      reconstructionArray->SetNumberOfComponents(9);
      reconstructionArray->SetNumberOfTuples(gridData->GetNumberOfPoints());

      for (int ptId = 0; ptId < gridData->GetNumberOfPoints(); ++ptId)
      {
        if (momentsBounds[1] - momentsBounds[0] > 0)
        {
          value = ReconstructFromCell<9>(
            ptId, orthonormalMomentsTensors, momentsData, gridData, r, radii, allowExtrapolation);
        }
        else
        {
          value = Reconstruct<9>(gridData->GetPoint(ptId),
            orthonormalMomentsTensors.at(0),
            momentsData->GetPoint(0),
            radii.at(r),
            allowExtrapolation);
        }
        reconstructionArray->SetTuple9(ptId,
          value[0],
          value[1],
          value[2],
          value[3],
          value[4],
          value[5],
          value[6],
          value[7],
          value[8]);
      }
      reconstructionData->GetPointData()->AddArray(reconstructionArray);
      reconstructionArray->Delete();
    }
  }
}

int vtkReconstructFromMoments::RequestUpdateExtent(vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector*)
{
  // We need to ask for the whole extent from this input.
  vtkInformation* momentsInfo = inputVector[0]->GetInformationObject(0);

  momentsInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  momentsInfo->Remove(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  if (momentsInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
  {
    momentsInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      momentsInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
      6);
  }

  momentsInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  momentsInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  momentsInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);

  return 1;
}

/** main executive of the program, reads the input, calles the functions, and produces the utput.
 * @param request: ?
 * @param inputVector: the input information
 * @param outputVector: the output information
 */
int vtkReconstructFromMoments::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* momentsInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* gridInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkImageData* momentsData =
    vtkImageData::SafeDownCast(momentsInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* gridData = vtkDataSet::SafeDownCast(gridInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* reconstructionData =
    vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (momentsData && gridData)
  {
    this->InterpretGridData(gridData);
    this->InterpretMomentsData(momentsData);
    this->CheckValidity(momentsData, gridData);
    ComputeReconstruction(reconstructionData,
      momentsData,
      gridData,
      this->Radii,
      this->AllowExtrapolation,
      this->Order,
      this->Dimension,
      this->NumberOfBasisFunctions,
      this->FieldRank);
  }
  return 1;
}
