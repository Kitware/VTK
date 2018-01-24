/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMomentsHelper.cxx

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

Copyright 2007. Los Alamos National Security, LLC.
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

#include "vtkMomentsHelper.h"
#include "vtkCell.h"
#include "vtkDoubleArray.h"
#include "vtkImageConstantPad.h"
#include "vtkImageData.h"
#include "vtkImageTranslateExtent.h"
#include "vtkMomentsTensor.h"
#include "vtkNew.h"
#include "vtkPixel.h"
#include "vtkPointData.h"
#include "vtkQuad.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"

#include <vector>

/**
 * The monomial basis is not orthonormal. We need this function for the reconstruction of the
 * function from the moments. This function uses Gram Schmidt
 * @param dimension: 2D or 3D
 * @param moments: the moments at a point
 * @param radius: the corresponding integration radius
 * @return the orthonormal moments
 */
std::vector<vtkMomentsTensor> vtkMomentsHelper::orthonormalizeMoments(int dimension,
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
                translationFactorAnalytic(radius,
                  2,
                  moments.at(k1).getOrders(i1).at(0) + moments.at(k2).getOrders(i2).at(0),
                  moments.at(k1).getOrders(i1).at(1) + moments.at(k2).getOrders(i2).at(1),
                  0);
            }
            else
            {
              A(i1 + k1 * moments.back().size(), i2 + k2 * moments.back().size()) =
                translationFactorAnalytic(radius,
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

  //#ifdef MYDEBUG
  //    Eigen::MatrixXd IA = Eigen::MatrixXd::Zero( moments.size() * moments.back().size(),
  //                                               moments.size() * moments.back().size() );
  //    for( size_t k1 = 0; k1 < moments.size(); ++k1 )
  //    {
  //        for( size_t i1 = 0; i1 < moments.at( k1 ).size(); ++i1 )
  //        {
  //            for( size_t k2 = 0; k2 < moments.size(); ++k2 )
  //            {
  //                for( size_t i2 = 0; i2 < moments.at( k2 ).size(); ++i2 )
  //                {
  //                    if( moments.at( k1 ).getFieldIndices( i1 ) == moments.at( k2
  //                                                                             ).getFieldIndices(
  //                                                                             i2 ) )
  //                    {
  //                        for( size_t d = 0; d < dimension; ++d )
  //                        {
  //                            IA( i1 + k1 * moments.back().size(), i2 + k2 * moments.back().size()
  //                            )
  //                            += ( moments.at( k1 ).getOrders( i1 ).at( d ) + moments.at( k2
  //                                                                                       ).getOrders(
  //                                                                                       i2 ).at(
  //                                                                                       d ) ) *
  //                                                                                       pow( 10,
  //                                                                                       d );
  //                        }
  //                    }
  //                }
  //            }
  //        }
  //    }
  //    Eigen::VectorXd Ib = Eigen::VectorXd::Zero( moments.size() * moments.back().size() );
  //    for( size_t k = 0; k < moments.size(); ++k )
  //    {
  //        for( size_t i = 0; i < moments.at( k ).size(); ++i )
  //        {
  //            for( size_t d = 0; d < dimension; ++d )
  //            {
  //                Ib( i + k * moments.back().size() ) += moments.at( k ).getOrders( i ).at( d ) *
  //                pow( 10, d );
  //            }
  //        }
  //    }
  //    std::cout<<"b="<<b<<endl;
  //    std::cout<<"Ib="<<Ib<<endl;
  //    std::cout<<"A="<<A<<endl;
  //    std::cout<<"IA="<<IA<<endl;
  //#endif

  Eigen::VectorXd x = A.colPivHouseholderQr().solve(b);
  //    std::cout<<"x="<<x<<endl;
  //    assert( b.isApprox( A * x ) );
  std::vector<vtkMomentsTensor> orthonormalMoments(moments.size());
  for (size_t k = 0; k < moments.size(); ++k)
  {
    orthonormalMoments.at(k) = vtkMomentsTensor(moments.at(k));
    for (size_t i = 0; i < moments.at(k).size(); ++i)
    {
      orthonormalMoments.at(k).set(i, x(i + k * moments.back().size()));
    }
  }

  //#ifdef MYDEBUG
  //    std::cout<<"x="<<x<<endl;
  //    std::cout<<"Ax-b="<<A*x-b<<endl;
  //    for( size_t k = 0; k < moments.size(); ++k )
  //    {
  //        moments.at( k ).print();
  //        orthonormalMoments.at( k ).print();
  //    }
  //    cout<<"orthonormalMoments"<<endl;
  //    for( size_t k = 0; k < moments.size(); ++k )
  //    {
  //        orthonormalMoments.at( k ).print();
  //    }
  //#endif

  return orthonormalMoments;
}

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
 * @param nameOfPointData: the name of the array in the point data of which the momens are computed.
 * @return the moments
 */
std::vector<vtkMomentsTensor> vtkMomentsHelper::allMomentsOrigResImageData(int dimension,
  int order,
  int fieldRank,
  double radius,
  int ptId,
  vtkImageData* source,
  std::string nameOfPointData)
{
  std::vector<vtkMomentsTensor> tensors(order + 1);
  for (int o = 0; o < order + 1; o++)
  {
    tensors.at(o) = vtkMomentsTensor(dimension, o + fieldRank, fieldRank);
  }
  if (ptId < 0)
  {
    return tensors;
  }

  double center[3];
  source->GetPoint(ptId, center);
  double bounds[6];
  source->GetBounds(bounds);
  for (size_t d = 0; d < static_cast<size_t>(dimension); d++)
  {
    if (center[d] - radius < bounds[2 * d] - 1e-10 ||
      center[d] + radius > bounds[2 * d + 1] + 1e-10)
    {
      return tensors;
    }
  }

  double argument[3];
  double relArgument[3];
  double* h = source->GetSpacing();
  if (dimension == 3)
  {
    for (int i = -radius / h[0] - 1e-5; i <= radius / h[0] + 1e-5; i++)
    {
      for (int j = -radius / h[1] - 1e-5; j <= radius / h[1] + 1e-5; j++)
      {
        for (int k = -radius / h[2] - 1e-5; k <= radius / h[2] + 1e-5; k++)
        {
          int index = ptId + i + j * source->GetDimensions()[0] +
            k * source->GetDimensions()[1] * source->GetDimensions()[1];
          source->GetPoint(index, argument);
          for (int d = 0; d < 3; ++d)
          {
            relArgument[d] = argument[d] - source->GetPoint(ptId)[d];
          }
          if (vtkMath::Norm(relArgument) <= radius + 1e-5)
          {
            for (int o = 0; o < order + 1; o++)
            {
              for (size_t s = 0; s < tensors.at(o).size(); s++)
              {
                double faktor = 1;
                for (size_t mi = 0; mi < tensors.at(o).getMomentIndices(s).size(); mi++)
                {
                  faktor *= relArgument[tensors.at(o).getMomentIndices(s).at(mi)];
                }
                tensors.at(o).set(s,
                  tensors.at(o).get(s) +
                    h[0] * h[1] * h[2] * faktor *
                      source->GetPointData()
                        ->GetArray(nameOfPointData.c_str())
                        ->GetTuple(index)[tensors.at(o).getFieldIndex(s)]);
                // std::cout << "outputFieldIndex="<<getFieldIndexFromTensorIndices(tensors.at( o
                // ).getIndices(s))
                //<<
                //"="<<getTensorIndicesFromFieldIndexAsString(getFieldIndexFromTensorIndices(tensors.at(
                // o ).getIndices(s)))<< " tensors.at( o ).getFieldIndex( s )="<<tensors.at( o
                //).getFieldIndex( s )<<"\n";  std::cout<<"radius="<<radius<<"
                // value="<<tensors.at(o).get(s)<<"\n";
              }
            }
          }
        }
      }
    }
  }
  else
  {
    for (int j = -radius / h[1] - 1e-5; j <= radius / h[1] + 1e-5; j++)
    {
      for (int i = -radius / h[0] - 1e-5; i <= radius / h[0] + 1e-5; i++)
      {
        int index = ptId + (i + j * source->GetDimensions()[0]);
        source->GetPoint(index, argument);
        for (int d = 0; d < 3; ++d)
        {
          relArgument[d] = argument[d] - source->GetPoint(ptId)[d];
        }

        if (vtkMath::Norm(relArgument) <= radius + 1e-5)
        {
          //                    if(center[0]==16&&center[1]==-3)
          //                        std::cout<<"i,j "<<radius/h[0]<<" "<<radius/h[1]<<" "<<i<<"
          //                        "<<j<<" "<<relArgument[0]<<" "<<relArgument[1]<<"
          //                        "<<source->GetPointData()->GetArray(nameOfPointData.c_str())->GetTuple(index)[0]<<"
          //                        "<<source->GetPointData()->GetArray(nameOfPointData.c_str())->GetTuple(index)[1]<<"\n";
          for (int o = 0; o < order + 1; o++)
          {
            for (size_t s = 0; s < tensors.at(o).size(); s++)
            {
              double faktor = 1;
              for (size_t mi = 0; mi < tensors.at(o).getMomentIndices(s).size(); mi++)
              {
                faktor *= relArgument[tensors.at(o).getMomentIndices(s).at(mi)];
              }
              //                            if(center[0]==16&&center[1]==-3)
              //                                std::cout<<relArgument[0]<<" "<<relArgument[1]<<"
              //                                o="<<o<<" s="<<s<<" faktor="<<faktor<<"\n";
              tensors.at(o).set(s,
                tensors.at(o).get(s) +
                  h[0] * h[1] * faktor *
                    source->GetPointData()
                      ->GetArray(nameOfPointData.c_str())
                      ->GetTuple(index)[tensors.at(o).getFieldIndex(s)]);
              //                        std::cout <<
              //                        "outputFieldIndex="<<getFieldIndexFromTensorIndices(tensors.at(
              //                        o ).getIndices(s))
              //                        <<
              //                        "="<<getTensorIndicesFromFieldIndexAsString(getFieldIndexFromTensorIndices(tensors.at(
              //                        o ).getIndices(s)))<< " tensors.at( o ).getFieldIndex( s
              //                        )="<<tensors.at( o ).getFieldIndex( s )<<"\n";
              // std::cout<<"radius="<<radius<<" value="<<tensors.at(o).get(s)<<"\n";
            }
          }
        }
      }
    }
  }
  // std::cout<<"radius="<<radius<<" hiervalue="<<integral<<" source"<<stencil->GetSpacing()<<"\n";
  return tensors;
}

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
 * @param nameOfPointData: the name of the array in the point data of which the momens are computed.
 * @return the moments
 */
std::vector<vtkMomentsTensor> vtkMomentsHelper::allMoments(int dimension,
  int order,
  int fieldRank,
  double radius,
  double center[3],
  vtkImageData* stencil,
  std::string nameOfPointData)
{
  std::vector<vtkMomentsTensor> tensors(order + 1);
  for (int k = 0; k < order + 1; ++k)
  {
    tensors.at(k) = vtkMomentsTensor(dimension, k + fieldRank, fieldRank);
  }
  double argument[3];
  double relArgument[3];
  double* h = stencil->GetSpacing();
  for (vtkIdType ptId = 0; ptId < stencil->GetNumberOfPoints(); ++ptId)
  {
    stencil->GetPoint(ptId, argument);
    for (int i = 0; i < 3; ++i)
    {
      relArgument[i] = argument[i] - center[i];
    }
    //        if(center[0]==16&&center[1]==-3) std::cout<<relArgument[0]<<" "<<relArgument[1]<<"\n";
    if (vtkMath::Norm(relArgument) <= radius + 1e-5)
    {
      for (int o = 0; o < order + 1; o++)
      {
        for (size_t s = 0; s < tensors.at(o).size(); s++)
        {
          double faktor = 1;
          for (size_t i = 0; i < tensors.at(o).getMomentIndices(s).size(); ++i)
          {
            faktor *= relArgument[tensors.at(o).getMomentIndices(s).at(i)];
          }
          //                    if(center[0]==16&&center[1]==-3)
          //                        std::cout<<relArgument[0]<<" "<<relArgument[1]<<" o="<<o<<"
          //                        s="<<s<<" faktor="<<faktor<<"\n";
          tensors.at(o).set(s,
            tensors.at(o).get(s) +
              h[0] * h[1] * h[2] * faktor *
                stencil->GetPointData()
                  ->GetArray(nameOfPointData.c_str())
                  ->GetTuple(ptId)[tensors.at(o).getFieldIndex(s)]);
        }
      }
    }
  }
  return tensors;
}

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
double vtkMomentsHelper::translationFactor(double radius,
  size_t p,
  size_t q,
  size_t r,
  vtkImageData* stencil)
{
  if (p % 2 == 1 || q % 2 == 1 || r % 2 == 1)
  {
    return 0;
  }
  double center[3];
  for (int d = 0; d < 3; ++d)
  {
    center[d] = 0.5 * (stencil->GetBounds()[2 * d + 1] + stencil->GetBounds()[2 * d]);
  }
  double argument[3];
  double relArgument[3];
  double* h = stencil->GetSpacing();
  double integral = 0;
  for (int ptId = 0; ptId < stencil->GetNumberOfPoints(); ++ptId)
  {
    stencil->GetPoint(ptId, argument);
    for (int d = 0; d < 3; ++d)
    {
      relArgument[d] = argument[d] - center[d];
    }
    //        std::cout<<"relArgument="<<relArgument[0]<<" "<<relArgument[1]<<"\n";
    if (vtkMath::Norm(relArgument) < radius + 1e-5)
    {
      integral += h[0] * h[1] * h[2] * pow(relArgument[0], p) * pow(relArgument[1], q) *
        pow(relArgument[2], r);
    }
  }
  //      std::cout<<"radius="<<radius<<" p="<<p<<" q="<<q<<" r="<<r<<"
  //      translationFactor="<<integral<<"\n";
  return (integral);
}

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
double vtkMomentsHelper::translationFactorAnalytic(double radius,
  int dimension,
  size_t p,
  size_t q,
  size_t r)
{
  if (dimension == 2)
  {
    if (p % 2 == 0 && q % 2 == 0)
    {
      if (p + q == 0)
      {
        return vtkMath::Pi() * pow(radius, 2 + p + q);
      }
      else if (p + q == 2)
      {
        return 1. / 4 * vtkMath::Pi() * pow(radius, 2 + p + q);
      }
      else if (p + q == 4)
      {
        if (p == 4 || q == 4)
        {
          return 1. / 8 * vtkMath::Pi() * pow(radius, 2 + p + q);
        }
        else
        {
          return 1. / 24 * vtkMath::Pi() * pow(radius, 2 + p + q);
        }
      }
      else if (p + q == 6)
      {
        if (p == 6 || q == 6)
        {
          return 5. / 64 * vtkMath::Pi() * pow(radius, 2 + p + q);
        }
        else
        {
          return 1. / 64 * vtkMath::Pi() * pow(radius, 2 + p + q);
        }
      }
      else if (p + q == 8)
      {
        if (p == 8 || q == 8)
        {
          return 7. / 128 * vtkMath::Pi() * pow(radius, 2 + p + q);
        }
        else if (p == 6 || q == 6)
        {
          return 1. / 128 * vtkMath::Pi() * pow(radius, 2 + p + q);
        }
        else
        {
          return 3. / 640 * vtkMath::Pi() * pow(radius, 2 + p + q);
        }
      }
      else
      {
        double center[3] = { 0.0, 0.0, 0.0 };
        vtkNew<vtkImageData> stencil;
        stencil->SetDimensions(25, 25, 1);
        stencil->SetSpacing(2. * radius / 25, 2. * radius / 25, 1);
        stencil->SetOrigin(center);
        return (translationFactor(radius, p, q, r, stencil));
      }
    }
    else
    {
      return 0;
    }
  }
  else
  {
    if (p % 2 == 0 && q % 2 == 0 && r % 2 == 0)
    {
      if (p + q + r == 0)
      {
        return 4. / 3 * vtkMath::Pi() * pow(radius, 3 + p + q + r);
      }
      else if (p + q + r == 2)
      {
        return 4. / 15 * vtkMath::Pi() * pow(radius, 3 + p + q + r);
      }
      else if (p + q + r == 4)
      {
        if (p == 4 || q == 4 || r == 4)
        {
          return 4. / 35 * vtkMath::Pi() * pow(radius, 3 + p + q + r);
        }
        else
        {
          return 4. / 105 * vtkMath::Pi() * pow(radius, 3 + p + q + r);
        }
      }
      else if (p + q + r == 6)
      {
        if (p == 6 || q == 6 || r == 6)
        {
          return 4. / 63 * vtkMath::Pi() * pow(radius, 3 + p + q + r);
        }
        else if (p == 4 || q == 4 || r == 4)
        {
          return 4. / 315 * vtkMath::Pi() * pow(radius, 3 + p + q + r);
        }
        else
        {
          return 4. / 945 * vtkMath::Pi() * pow(radius, 3 + p + q + r);
        }
      }
      else
      {
        double center[3] = { 0.0, 0.0, 0.0 };
        vtkNew<vtkImageData> stencil;
        stencil->SetDimensions(25, 25, 25);
        stencil->SetSpacing(2. * radius / 25, 2. * radius / 25, 2. * radius / 25);
        stencil->SetOrigin(center);
        return (translationFactor(radius, p, q, r, stencil));
      }
    }
    else
    {
      return 0;
    }
  }
}

/**
 * This function generates the stencil, which contains the locations at which the dataset is
 * evaluated for the integration
 * @param stencil: contains the locations at which the dataset is evaluated for the integration
 * @param radius: the integration radius at which the moments are computed
 * @param numberOfIntegrationSteps: how fine the discrete integration done in each dimension
 * @param dimension: 2D or 3D
 * @param source: the dataset
 * @param nameOfPointData: the name of the array in the point data of which the momens are computed.
 * @return the moments
 */
void vtkMomentsHelper::BuildStencil(vtkImageData* stencil,
  double radius,
  int numberOfIntegrationSteps,
  int dimension,
  vtkDataSet* source,
  std::string nameOfPointData)
{
  double spacing = 2. * radius / numberOfIntegrationSteps;

  if (dimension == 2)
  {
    stencil->SetDimensions(numberOfIntegrationSteps, numberOfIntegrationSteps, 1);
    stencil->SetSpacing(spacing, spacing, 1);
  }
  else
  {
    stencil->SetDimensions(
      numberOfIntegrationSteps, numberOfIntegrationSteps, numberOfIntegrationSteps);
    stencil->SetSpacing(spacing, spacing, spacing);
  }

  // set the copy attribute to tell interpolatePoint, which array to use
  stencil->GetPointData()->CopyAllOff();
  int idx;
  vtkDataSetAttributes* sPD = source->GetPointData();
  sPD->GetArray(nameOfPointData.c_str(), idx);
  int attIdx = sPD->IsArrayAnAttribute(idx);
  if (attIdx >= 0)
  {
    stencil->GetPointData()->SetCopyAttribute(attIdx, 1);
  }
  stencil->GetPointData()->CopyFieldOn(nameOfPointData.c_str());

  stencil->GetPointData()->InterpolateAllocate(
    sPD, stencil->GetNumberOfPoints(), stencil->GetNumberOfPoints());

  double bounds[6];
  stencil->GetBounds(bounds);
  stencil->SetOrigin(
    -0.5 * (bounds[1] - bounds[0]), -0.5 * (bounds[3] - bounds[2]), -0.5 * (bounds[5] - bounds[4]));

  //        std::ostream stream(std::cout.rdbuf());
  //        std::cout<<"Stencil\n";
  //        std::cout<<"StencilSpacing="<<stencil->GetSpacing()[0]<<",
  //        "<<stencil->GetSpacing()[1]<<", "<<stencil->GetSpacing()[2]<<"\n";
  //        stencil->GetPointData()->PrintSelf(stream, vtkIndent(0));
  //        std::cout<<"\n";
  //        double x[3];
  //        for( int ptId = 0; ptId < stencil->GetNumberOfPoints(); ++ptId )
  //        {
  //            stencil->GetPoint(ptId, x);
  //            std::cout<<ptId<<" x="<<x[0]<<" "<<x[1]<<"\n";
  //        }
}

/**
 * This function moves the stencil to the current location, where the integration is supposed o be
 * performed
 * @param center: the location
 * @param source: the dataset
 * @param stencil: contains the locations at which the dataset is evaluated for the integration
 * @param numberOfIntegrationSteps: how fine the discrete integration done in each dimension
 * @return the moments
 */
bool vtkMomentsHelper::CenterStencil(double center[3],
  vtkDataSet* source,
  vtkImageData* stencil,
  int numberOfIntegrationSteps,
  std::string vtkNotUsed(nameOfPointData))
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
      center[1] - 0.5 * (bounds[3] - bounds[2]),
      center[2] - 0.5 * (bounds[5] - bounds[4]));
  }
  //        if( stencil->GetBounds()[0] < source->GetBounds()[0] || stencil->GetBounds()[2] <
  //        source->GetBounds()[2] || stencil->GetBounds()[4] < source->GetBounds()[4] ||
  //        stencil->GetBounds()[1] > source->GetBounds()[1] || stencil->GetBounds()[3] >
  //        source->GetBounds()[3] || stencil->GetBounds()[5] > source->GetBounds()[5] )
  //        {
  //            return( false );
  //        }

  int subId = 0;

  // interpolation of the source data at the integration points
  for (vtkIdType ptId = 0; ptId < stencil->GetNumberOfPoints(); ++ptId)
  {
    // find point coordinates
    double x[3];
    stencil->GetPoint(ptId, x);

    // find cell
    double pcoords[3];
    double* weights = new double[source->GetMaxCellSize()];
    vtkIdType cellId = source->FindCell(x, NULL, -1, 1, subId, pcoords, weights);
    vtkCell* cell;
    if (cellId >= 0)
    {
      cell = source->GetCell(cellId);
    }
    else
    {
      cell = 0;
    }
    if (cell)
    {
      // Interpolate the point data
      stencil->GetPointData()->InterpolatePoint(
        source->GetPointData(), ptId, cell->PointIds, weights);
    }
    else
    {
      return (false);
    }
  }
  //        std::ostream stream(std::cout.rdbuf());
  //        std::cout<<"Stencil\n";
  //        std::cout<<"Stencil:"<<stencil->GetCenter()[0]<<" "<<stencil->GetCenter()[0]<<"\n";
  //        std::cout<<"Stencil:"<<stencil->GetBounds()[0]<<" "<<stencil->GetBounds()[1]<<"\n";
  //        stencil->PrintSelf(stream, vtkIndent(0));
  //        std::cout<<"\n";
  //        double x[3];
  //        for( int ptId = 0; ptId < stencil->GetNumberOfPoints(); ++ptId )
  //        {
  //            stencil->GetPoint(ptId, x);
  //            std::cout<<ptId<<" x="<<x[0]<<" "<<x[1]<<"\n";
  //        }

  //    if( stencil->GetBounds()[0] < source->GetBounds()[0] || stencil->GetBounds()[2] <
  //    source->GetBounds()[2] || stencil->GetBounds()[4] < source->GetBounds()[4] ||
  //    stencil->GetBounds()[1] > source->GetBounds()[1] || stencil->GetBounds()[3] >
  //    source->GetBounds()[3] || stencil->GetBounds()[5] > source->GetBounds()[5] )
  //    {
  //        std::cout<<"StencilCenter:"<<stencil->GetCenter()[0]<<"
  //        "<<stencil->GetCenter()[1]<<"\n";
  //        std::cout<<"StencilBounds:"<<stencil->GetBounds()[0]<<" "<<stencil->GetBounds()[1]<<"
  //        "<<stencil->GetBounds()[2]<<" "<<stencil->GetBounds()[3]<<"\n";
  //
  //    }
  return (true);
}

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
std::vector<size_t> vtkMomentsHelper::getTensorIndicesFromFieldIndex(size_t index,
  int dimension,
  int order,
  int fieldRank)
{
  vtkIdType i = index;
  int r = fieldRank;
  while (i >= 0 && r <= order + fieldRank)
  {
    i -= pow(dimension, r);
    ++r;
  }
  --r;
  i += pow(dimension, r);
  // std::cout<<" rawindexToI="<<index-i;
  // std::cout<<" remainingindexToI="<<i<<": ";

  vtkMomentsTensor dumvtkMomentsTensor = vtkMomentsTensor(dimension, r, fieldRank);
  //    for( size_t j = 0; j < dumvtkMomentsTensor.getIndices(i).size(); ++j )
  //    {
  //        cout<<dumvtkMomentsTensor.getIndices(i).at(j);
  //    }
  std::vector<size_t> indices = dumvtkMomentsTensor.getIndices(i);
  // std::cout<<"getTensorIndicesFromFieldIndex i="<<i<<" "<<"r="<<r<<" index="<<index<<" "<<
  // getFieldIndexFromTensorIndices(dumvtkMomentsTensor.getIndices( i ))<<"\n";

  return dumvtkMomentsTensor.getIndices(i);
}

/**
 * Inverse function to getTensorIndicesFromFieldIndex
 * given a vector with tensor indices and a radius, this function returns the index in the output of
 * this algorithm that corresponds tothis basis function
 * @param radiusIndex: index of this radius in the radii vector
 * @param indices: the given tensor indices
 * @param dimension: 2D or 3D
 * @param fieldRank: 0 for scalar, 1 for vector and 2 for matrix
 * @param numberOfBasisFunctions: number of basis functions in the source
 * equals \sum_{i=0}^order dimension^o
 */
size_t vtkMomentsHelper::getFieldIndexFromTensorIndices(size_t radiusIndex,
  std::vector<size_t> indices,
  int dimension,
  int fieldRank,
  int numberOfBasisFunctions)
{
  // cout<<"\n getFieldIndexFromTensorIndices indices=";
  //    for( size_t j = 0; j < indices.size(); ++j )
  //    {
  //        cout<<indices.at(j);
  //    }
  vtkMomentsTensor dumvtkMomentsTensor = vtkMomentsTensor(dimension, indices.size(), fieldRank);
  size_t index = 0;
  for (size_t i = fieldRank; i < indices.size(); ++i)
  {
    index += pow(dimension, i);
  }
  // std::cout<<" rawindexFromI="<<index;
  // std::cout<<" remainingindexFromI="<<dumvtkMomentsTensor.getIndex( indices )<<": ";
  //    for( size_t j = 0; j < indices.size(); ++j )
  //    {
  //        cout<<indices.at(j);
  //    }
  return index + dumvtkMomentsTensor.getIndex(indices) + radiusIndex * numberOfBasisFunctions;
}

/**
 * The output contains the tensor indices that describe the basis function that belongs to the given
 * output array as string. Convenience function. they are sorted by increasing order and then by the
 * index as returned by vtkMomentsTensor.getIndices(i)
 * @param index: the index of this output field pointdata array
 * @param dimension: 2D or 3D
 * @param order: the maximal order up to which the moments are computed
 * @param fieldRank: 0 for scalar, 1 for vector and 2 for matrix
 * @return the moments
 */
std::string vtkMomentsHelper::getTensorIndicesFromFieldIndexAsString(size_t index,
  int dimension,
  int order,
  int fieldRank)
{
  std::string indexString = "";
  std::vector<size_t> indices =
    vtkMomentsHelper::getTensorIndicesFromFieldIndex(index, dimension, order, fieldRank);
  for (size_t i = 0; i < indices.size(); ++i)
  {
    indexString += std::to_string(indices.at(i));
  }
  return indexString;
}

/**
 * the function returns true if the point lies within radius of the boundary of the dataset
 * @param ptId: ID of the point in question
 * @param field: the field that contains the point
 */
bool vtkMomentsHelper::isCloseToEdge(int dimension, int ptId, double radius, vtkImageData* field)
{
  if (dimension == 2)
  {
    return !(ptId % field->GetDimensions()[0] >= radius / field->GetSpacing()[0] &&
      ptId % field->GetDimensions()[0] <=
        field->GetDimensions()[0] - 1 - radius / field->GetSpacing()[0] &&
      ptId / field->GetDimensions()[0] >= radius / field->GetSpacing()[1] &&
      ptId / field->GetDimensions()[0] <=
        field->GetDimensions()[1] - 1 - radius / field->GetSpacing()[1]);
  }
  else
  {
    return !(ptId % field->GetDimensions()[0] >= radius / field->GetSpacing()[0] &&
      ptId % field->GetDimensions()[0] <=
        field->GetDimensions()[0] - 1 - radius / field->GetSpacing()[0] &&
      (ptId / field->GetDimensions()[0]) % field->GetDimensions()[1] >=
        radius / field->GetSpacing()[1] &&
      (ptId / field->GetDimensions()[0]) % field->GetDimensions()[1] <=
        field->GetDimensions()[1] - 1 - radius / field->GetSpacing()[1] &&
      ptId / field->GetDimensions()[0] / field->GetDimensions()[1] >=
        radius / field->GetSpacing()[2] &&
      ptId / field->GetDimensions()[0] / field->GetDimensions()[1] <=
        field->GetDimensions()[2] - 1 - radius / field->GetSpacing()[2]);
  }
}

bool vtkMomentsHelper::isEdge(int dimension, int ptId, vtkImageData* field)
{
  if (dimension == 2)
  {
    //        cout << ptId << " " << field->GetPoint(ptId)[0] << " " << field->GetPoint(ptId)[1] <<
    //        " " << !(ptId % field->GetDimensions()[0] > 0
    //        && ptId % field->GetDimensions()[0] < field->GetDimensions()[0]-1
    //        && ptId / field->GetDimensions()[0] > 0
    //        && ptId / field->GetDimensions()[0] < field->GetDimensions()[1]-1) << endl;
    return !(ptId % field->GetDimensions()[0] > 0 &&
      ptId % field->GetDimensions()[0] < field->GetDimensions()[0] - 1 &&
      ptId / field->GetDimensions()[0] > 0 &&
      ptId / field->GetDimensions()[0] < field->GetDimensions()[1] - 1);
  }
  else
  {
    //        cout << ptId << " " << field->GetPoint(ptId)[0] << " " << field->GetPoint(ptId)[1] <<
    //        " " << field->GetPoint(ptId)[2] << " " <<
    //        !(ptId % field->GetDimensions()[0] > 0
    //          && ptId % field->GetDimensions()[0] < field->GetDimensions()[0]-1
    //          && (ptId / field->GetDimensions()[0]) % field->GetDimensions()[1] > 0
    //          && (ptId / field->GetDimensions()[0]) % field->GetDimensions()[1]  <
    //          field->GetDimensions()[1]-1
    //          && ptId / field->GetDimensions()[0] / field->GetDimensions()[1] > 0
    //          && ptId / field->GetDimensions()[0] / field->GetDimensions()[1]  <
    //          field->GetDimensions()[2]-1)
    //        << endl;
    return !(ptId % field->GetDimensions()[0] > 0 &&
      ptId % field->GetDimensions()[0] < field->GetDimensions()[0] - 1 &&
      (ptId / field->GetDimensions()[0]) % field->GetDimensions()[1] > 0 &&
      (ptId / field->GetDimensions()[0]) % field->GetDimensions()[1] <
        field->GetDimensions()[1] - 1 &&
      ptId / field->GetDimensions()[0] / field->GetDimensions()[1] > 0 &&
      ptId / field->GetDimensions()[0] / field->GetDimensions()[1] < field->GetDimensions()[2] - 1);
  }
}

//----------------------------------------------------------------------------------
vtkIdType vtkMomentsHelper::getArrayIndex(std::vector<int> coord, std::vector<int> dimensions)
{
  // std::cout << "Coord: " << coord[0] << ", " << coord[1] << ", " << coord[2] << std::endl;
  return coord[0] + coord[1] * dimensions[0] + coord[2] * dimensions[0] * dimensions[1];
}

//----------------------------------------------------------------------------------
std::vector<int> vtkMomentsHelper::getCoord(vtkIdType index, std::vector<int> dimensions)
{
  int z = 0;
  if (dimensions[2] > 1)
  {
    z = index / dimensions[0] / dimensions[1];
  }

  int y = (index - z * dimensions[0] * dimensions[1]) / dimensions[0];

  std::vector<int> arr(3);
  arr[0] = index - z * dimensions[0] * dimensions[1] - y * dimensions[0];
  arr[1] = y;
  arr[2] = z;

  return arr;
}

//--------------------------------------------------------------------
vtkImageData* vtkMomentsHelper::translateToOrigin(vtkImageData* data)
{
  // std::cout << "Extent: [" << data->GetExtent()[0] << ", " << data->GetExtent()[1] << ", " <<
  // data->GetExtent()[2] << ", " << data->GetExtent()[3] << ", " << data->GetExtent()[4] << ", " <<
  // data->GetExtent()[5] << "]" << std::endl;

  // Translate to the origin
  vtkImageTranslateExtent* trans = vtkImageTranslateExtent::New();
  trans->SetTranslation(-data->GetExtent()[0], -data->GetExtent()[2], -data->GetExtent()[4]);
  trans->SetInputData(data);
  trans->Update();

  // std::cout << "Translated extent: [" << trans->GetOutput()->GetExtent()[0] << ", " <<
  // trans->GetOutput()->GetExtent()[1] << ", " << trans->GetOutput()->GetExtent()[2] << ", " <<
  // trans->GetOutput()->GetExtent()[3] << ", " << trans->GetOutput()->GetExtent()[4] << ", " <<
  // trans->GetOutput()->GetExtent()[5] << "]" << std::endl;  trans->GetOutput()->Print(std::cout);

  return trans->GetOutput();
}

//------------------------------------------------------------------------------------------
vtkImageData* vtkMomentsHelper::padField(vtkImageData* field,
  vtkImageData* kernel,
  int dimension,
  std::string nameOfPointData)
{
  // Translate to the origin
  vtkImageData* transR = translateToOrigin(field);
  vtkImageData* transK = translateToOrigin(kernel);

  // std::cout << "Finished translating..." << std::endl;
  // std::cout << transR->GetExtent()[1] << std::endl;
  // std::cout << transR->GetExtent()[3] << std::endl;
  // std::cout << transK->GetExtent()[1] << std::endl;
  // std::cout << transK->GetExtent()[3] << std::endl;

  int dataMinExtent =
    std::min(std::min(transR->GetExtent()[0], transR->GetExtent()[2]), transR->GetExtent()[4]);
  int dataMaxExtent =
    std::max(std::max(transR->GetExtent()[1], transR->GetExtent()[3]), transR->GetExtent()[5]);
  int patternMinExtent =
    std::min(std::min(transK->GetExtent()[0], transK->GetExtent()[2]), transK->GetExtent()[4]);
  int patternMaxExtent =
    std::max(std::max(transK->GetExtent()[1], transK->GetExtent()[3]), transK->GetExtent()[5]);
  int minExtent = dataMinExtent - patternMinExtent;
  int maxExtent = dataMaxExtent + patternMaxExtent;

  int dataExtentPad[6] = { 0, 0, 0, 0, 0, 0 };
  for (int i = 0; i < dimension; i++)
  {
    dataExtentPad[2 * i] = minExtent;
    dataExtentPad[2 * i + 1] = maxExtent;
  }
  // std::cout << "Data extent padded: [" << dataExtentPad[0] << ", " << dataExtentPad[1] << ", " <<
  // dataExtentPad[2] << ", " << dataExtentPad[3] << ", " << dataExtentPad[4] << ", " <<
  // dataExtentPad[5] << "]" << std::endl;
  // transR->Print(std::cout);

  vtkImageData* output = vtkImageData::New();
  output->SetOrigin(0, 0, 0);
  output->SetSpacing(transR->GetSpacing());
  output->SetExtent(dataExtentPad);

  vtkDataArray* origArray = transR->GetPointData()->GetArray(nameOfPointData.c_str());

  vtkDoubleArray* paddedArray = vtkDoubleArray::New();
  paddedArray->SetName(nameOfPointData.c_str());
  paddedArray->SetNumberOfComponents(origArray->GetNumberOfComponents());
  paddedArray->SetNumberOfTuples(output->GetNumberOfPoints());
  paddedArray->Fill(0.0);

  int* tmp = transR->GetDimensions();
  std::vector<int> origSize = std::vector<int>(tmp, tmp + 3);
  // std::cout << "origSize: [" << origSize[0] << ", " << origSize[1] << ", " << origSize[2] << "]"
  // << std::endl;

  tmp = output->GetDimensions();
  std::vector<int> paddedSize = std::vector<int>(tmp, tmp + 3);
  // std::cout << "paddedSize: [" << paddedSize[0] << ", " << paddedSize[1] << ", " << paddedSize[2]
  // << "]" << std::endl;

  for (vtkIdType i = 0; i < transR->GetNumberOfPoints(); i++)
  {
    std::vector<int> coord = getCoord(i, origSize);
    vtkIdType index = getArrayIndex(coord, paddedSize);
    paddedArray->SetTuple(index, origArray->GetTuple(i));
  }

  output->GetPointData()->AddArray(paddedArray);

  // vtkImageConstantPad* paddedField = vtkImageConstantPad::New();
  // paddedField->SetOutputWholeExtent(dataExtentPad);
  // paddedField->SetInputData(transR);
  // paddedField->SetConstant(0.0);
  // paddedField->Update();
  // vtkImageData* output = paddedField->GetOutput();

  // return translateToOrigin(output);

  // vtkXMLImageDataWriter* writer = vtkXMLImageDataWriter::New();
  // writer->SetInputData(output);
  // writer->SetFileName("/Users/ktsai/Documents/VTK_MomentInvariants/momentPatternDetetctionTest/output/paddedField.vti");
  // writer->Write();

  return output;
}

//--------------------------------------------------------------------------------------------
vtkImageData* vtkMomentsHelper::padKernel(vtkImageData* kernel, vtkImageData* paddedField)
{
  // Translate to the origin
  vtkImageData* trans = translateToOrigin(kernel);

  // int extent[6] = {0, 0, 0, 0, 0, 0};
  // for (size_t i = 0; i < this->Dimension; i++)
  // {
  //     extent[2*i] = paddedField->GetExtent()[2*i];
  //     extent[2*i+1] = paddedField->GetExtent()[2*i+1];
  // }
  // std::cout << "Kernel extent padded: [" << extent[0] << ", " << extent[1] << ", " << extent[2]
  // << ", " << extent[3] << ", " << extent[4] << ", " << extent[5] << "]" << std::endl;

  vtkImageData* output = vtkImageData::New();
  output->SetOrigin(0, 0, 0);
  output->SetSpacing(trans->GetSpacing());
  output->SetExtent(paddedField->GetExtent());

  vtkDataArray* scalars = trans->GetPointData()->GetScalars();

  vtkDoubleArray* scalarsPad = vtkDoubleArray::New();
  scalarsPad->SetName("kernel");
  scalarsPad->SetNumberOfComponents(1);
  scalarsPad->SetNumberOfTuples(output->GetNumberOfPoints());
  scalarsPad->Fill(0.0);

  int* tmp = trans->GetDimensions();
  std::vector<int> origSize = std::vector<int>(tmp, tmp + 3);
  // std::cout << "origSize: [" << origSize[0] << ", " << origSize[1] << ", " << origSize[2] << "]"
  // << std::endl;

  tmp = output->GetDimensions();
  std::vector<int> paddedSize = std::vector<int>(tmp, tmp + 3);
  // std::cout << "paddedSize: [" << paddedSize[0] << ", " << paddedSize[1] << ", " << paddedSize[2]
  // << "]" << std::endl;

  std::vector<int> cent(3);

  for (size_t i = 0; i < cent.size(); i++)
  {
    cent[i] = origSize[i] / 2;
  }

  // std::cout << "cent: [" << cent[0] << ", " << cent[1] << ", " << cent[2] << "]" << std::endl;

  for (vtkIdType i = 0; i < scalars->GetNumberOfTuples(); i++)
  {
    std::vector<int> coord = getCoord(i, origSize);
    // std::cout << "coord: [" << coord[0] << ", " << coord[1] << ", " << coord[2] << "]" <<
    // std::endl;
    std::vector<int> offset(coord.size());
    for (size_t j = 0; j < offset.size(); j++)
    {
      offset[j] = cent[j] - coord[j];
    }

    std::vector<int> newCoord(coord.size());
    for (size_t j = 0; j < newCoord.size(); j++)
    {
      newCoord[j] = coord[j] - cent[j];
      if (coord[j] < cent[j])
      {
        newCoord[j] = paddedSize[j] - offset[j];
        // std::cout << newCoord[j] << std::endl;
      }
    }
    // std::cout << "newCoord: [" << newCoord[0] << ", " << newCoord[1] << ", " << newCoord[2] <<
    // "]" << std::endl;

    vtkIdType index = getArrayIndex(newCoord, paddedSize);
    scalarsPad->SetTuple1(index, scalars->GetTuple1(i));
  }

  output->GetPointData()->SetScalars(scalarsPad);

  return output;
}
