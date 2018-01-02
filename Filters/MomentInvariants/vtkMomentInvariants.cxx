/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMomentInvariants.cxx

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

#include "vtkMomentInvariants.h"
#include "vtkMomentsHelper.h"
#include "vtkMomentsTensor.h"

#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <Eigen/Dense>
#include <Eigen/Eigenvalues>

#include <list>
#include <vector>

namespace
{
/** helper function that generates a 2D rotation matrix.
 * @param angle: angle in radiant
 * @return2D rotation matrix around angle
 */
Eigen::MatrixXd getRotMat(double angle)
{
  Eigen::Matrix2d rotMat;
  rotMat(0, 0) = cos(angle);
  rotMat(0, 1) = -sin(angle);
  rotMat(1, 0) = sin(angle);
  rotMat(1, 1) = cos(angle);
  return rotMat;
}

/** this function generates the rotation matrix that rotates the first dominantContraction into the
 * x-axis and if applicable, the second one into the x-y-halfplane with positive z
 * @param dominantContractions: the vectors used for the normalization
 * @return 2D or 3D rotation matrix
 */
Eigen::MatrixXd getRotMat(std::vector<vtkMomentsTensor>& dominantContractions,
  unsigned int dimension)
{
  if (dimension == 2)
  {
    if (dominantContractions.at(0).getVector().norm() < 1e-3)
    {
      return Eigen::Matrix2d::Identity();
    }
    Eigen::Matrix2d rotMat =
      getRotMat(-atan2(dominantContractions.at(0).get(1), dominantContractions.at(0).get(0)));
    if (std::abs((rotMat * dominantContractions.at(0).getVector())[1]) > 1e-3)
    {
      vtkGenericWarningMacro(
        "rotMat=" << rotMat << "gedreht=" << rotMat * dominantContractions.at(0).getVector());
      vtkGenericWarningMacro("rotation not successful.");
    }
    return rotMat;
  }
  else
  {
    if (dominantContractions.at(0).getVector().norm() < 1e-3)
    {
      return Eigen::Matrix3d::Identity();
    }
    if (dominantContractions.at(0).size() == 1)
    {
      return Eigen::Matrix3d::Identity();
    }
    Eigen::Vector3d axis1 = dominantContractions.at(0).getVector();
    axis1.normalize();
    axis1 = axis1 + Eigen::Vector3d::UnitX();
    axis1.normalize();
    Eigen::Vector3d axis2 = dominantContractions.at(0).getVector();
    axis2.normalize();
    Eigen::Matrix3d rotMat1, rotMat2;
    rotMat1 = Eigen::AngleAxisd(M_PI, axis1);
    if (dominantContractions.size() == 1)
    {
      return rotMat1;
    }
    if (dominantContractions.at(1).getVector().norm() < 1e-3 ||
      (Eigen::Vector3d(dominantContractions.at(0).getVector())
          .cross(Eigen::Vector3d(dominantContractions.at(1).getVector())))
          .norm() < 1e-3)
    {
      return rotMat1;
    }
    rotMat2 = Eigen::AngleAxisd(-atan2((rotMat1 * dominantContractions.at(1).getVector())[2],
                                  (rotMat1 * dominantContractions.at(1).getVector())[1]),
      Eigen::Vector3d::UnitX());
    if (std::abs((rotMat1 * dominantContractions.at(0).getVector())[1]) > 1e-3 ||
      std::abs((rotMat1 * dominantContractions.at(0).getVector())[2]) > 1e-3 ||
      std::abs((rotMat2 * rotMat1 * dominantContractions.at(1).getVector())[2]) > 1e-3 ||
      (rotMat2 * rotMat1 * dominantContractions.at(1).getVector())[1] < -1e-3)
    {
      vtkGenericWarningMacro("Rotation not successful.");
      vtkGenericWarningMacro(
        "rotMat1=" << rotMat1 << "gedreht=" << rotMat1 * dominantContractions.at(0).getVector());
      vtkGenericWarningMacro(
        "rotMat1=" << rotMat1 << "gedreht=" << rotMat1 * dominantContractions.at(1).getVector());
      vtkGenericWarningMacro("rotMat2="
        << rotMat2 << "gedreht=" << rotMat2 * rotMat1 * dominantContractions.at(1).getVector());
      vtkGenericWarningMacro("rotation not successful.");
    }
    return rotMat2 * rotMat1;
  }
}
}

vtkStandardNewMacro(vtkMomentInvariants);

/**
 * constructior setting defaults
 */
vtkMomentInvariants::vtkMomentInvariants()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(4);

  this->Dimension = 0;
  this->FieldRank = 0;

  // default settings
  this->Order = 2;
  this->Radii = std::vector<double>(0);
  this->NumberOfIntegrationSteps = 5;
  this->AngleResolution = 100;
  this->Eps = 1e-2;
  this->NameOfPointData = "no name set by user";
  this->RadiusPattern = std::numeric_limits<double>::max();

  double tmp[3] = { 0.0, 0.0, 0.0 };
  this->CenterPattern = tmp;

  this->IsTranslation = 0;
  this->IsScaling = 0;
  this->IsRotation = 1;
  this->IsReflection = 0;
  this->TranslationFactor = nullptr;
}

vtkMomentInvariants::~vtkMomentInvariants()
{
  delete[] this->TranslationFactor;
}

/**
 * the agorithm has two input ports
 * port 0 is the pattern, which is a vtkImageData of scalar, vector, or matrix type
 * port 1 is the output of computeMoments, which is vtkImageData
 */
int vtkMomentInvariants::FillInputPortInformation(int port, vtkInformation* info)
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
 * the agorithm generates 4 outputs, all are vtkImageData
 * the first two have the topology of the momentData
 * a field storing the similarity to the pattern for all radii in a scalar field each
 * the normalized moments of the field
 * the latter two have extent 0, they only have 1 point in each field
 * the moments of the pattern
 * the first standard position of the normalized moments of the pattern
 */
int vtkMomentInvariants::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  return 1;
}

/** standard vtk print function
 * @param os: the way how to print
 * @param indent: how far to the right the text shall appear
 */
void vtkMomentInvariants::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "vtkMomentInvariants::PrintSelf\n";
  os << indent << "Dimension =  " << this->Dimension << "\n";
  os << indent << "FieldRank =  " << this->FieldRank << "\n";
  os << indent << "Order =  " << this->Order << "\n";
  os << indent << "Radii =  ";
  for (size_t i = 0; i < this->Radii.size(); ++i)
  {
    os << std::scientific << setprecision(10) << this->Radii.at(i) << " ";
  }
  os << "\n";
  os << indent << "NumberOfIntegrationSteps =  " << this->NumberOfIntegrationSteps << "\n";
  os << indent << "NumberOfFields =  " << this->NumberOfFields << "\n";
  os << indent << "NumberOfBasisFunctions =  " << this->NumberOfBasisFunctions << "\n";
  os << indent << "IsTranslation =  " << this->IsTranslation << "\n";
  os << indent << "IsScaling =  " << this->IsScaling << "\n";
  os << indent << "IsRotation =  " << this->IsRotation << "\n";
  os << indent << "IsReflection =  " << this->IsReflection << "\n";
  os << indent << "AngleResolution =  " << this->AngleResolution << "\n";
  os << indent << "Eps =  " << this->Eps << "\n";
  os << indent << "RadiusPattern =  " << this->RadiusPattern << "\n";
  os << indent << "CenterPattern =  " << this->CenterPattern[0] << " " << this->CenterPattern[1]
     << " " << this->CenterPattern[2] << "\n";
  os << indent << "NumberOfOrientationsToCompare =  " << this->MomentsPatternNormal.size() << "\n";

  cout << "momentsPattern" << endl;
  for (size_t k = 0; k < this->MomentsPattern.size(); ++k)
  {
    this->MomentsPattern.at(k).print();
  }
  if (this->IsTranslation)
  {
    cout << "momentsPatternTNormal" << endl;
    for (size_t k = 0; k < this->MomentsPatternTNormal.size(); ++k)
    {
      this->MomentsPatternTNormal.at(k).print();
    }
  }
  if (this->IsScaling)
  {
    cout << "momentsPatternTSNormal" << endl;
    for (size_t k = 0; k < this->MomentsPatternTSNormal.size(); ++k)
    {
      this->MomentsPatternTSNormal.at(k).print();
    }
  }

  this->Superclass::PrintSelf(os, indent);
}

/**
 * normalization with respect to outer transaltion, i.e. the result will be invariant to adding a
 * constant
 * the translational factor is evaluated with the same stencil as the moments
 * @param moments: the moments at one point stored in a vector of tensors
 * @param radius: the integration radius over which the moments were computed
 * @param isTranslation: if normalization w.tr.t. translation is desired by the user
 * @param stencil: the points in this stencil are used to numerically approximate the integral
 * @return the translationally normalized moments
 */
std::vector<vtkMomentsTensor> vtkMomentInvariants::NormalizeT(
  std::vector<vtkMomentsTensor>& moments,
  double radius,
  bool isTranslation,
  vtkImageData* stencil)
{
  /** normalization of the pattern with respect to translation */
  std::vector<vtkMomentsTensor> momentsNormal = moments;
  if (isTranslation)
  {
    for (size_t k = 0; k < moments.size(); ++k)
    {
      for (size_t i = 0; i < moments.at(k).size(); ++i)
      {
        if (this->Dimension == 2)
        {
          momentsNormal.at(k).set(i,
            moments.at(k).get(i) -
              moments.at(0).get(moments.at(k).getFieldIndices(i)) /
                vtkMomentsHelper::translationFactor(radius, 0, 0, 0, stencil) *
                vtkMomentsHelper::translationFactor(radius,
                  moments.at(k).getOrders(i).at(0),
                  moments.at(k).getOrders(i).at(1),
                  0,
                  stencil));
        }
        else
        {
          momentsNormal.at(k).set(i,
            moments.at(k).get(i) -
              moments.at(0).get(moments.at(k).getFieldIndices(i)) /
                vtkMomentsHelper::translationFactor(radius, 0, 0, 0, stencil) *
                vtkMomentsHelper::translationFactor(radius,
                  moments.at(k).getOrders(i).at(0),
                  moments.at(k).getOrders(i).at(1),
                  moments.at(k).getOrders(i).at(2),
                  stencil));
        }
      }
    }
  }
  return momentsNormal;
}

/**
 * normalization with respect to outer transaltion, i.e. the result will be invariant to adding a
 * constant
 * the translational factor is evaluated with the same stencil as the moments
 * @param moments: the moments at one point stored in a vector of tensors
 * @param radius: the integration radius over which the moments were computed
 * @param isTranslation: if normalization w.tr.t. translation is desired by the user
 * @param stencil: the points in this stencil are used to numerically approximate the integral
 * @return the translationally normalized moments
 */
std::vector<vtkMomentsTensor> vtkMomentInvariants::NormalizeT(
  std::vector<vtkMomentsTensor>& moments,
  int radiusIndex,
  bool isTranslation)
{
  /** normalization of the moments with respect to translation */
  std::vector<vtkMomentsTensor> momentsNormal = moments;
  if (isTranslation)
  {
    for (size_t k = 0; k < moments.size(); ++k)
    {
      for (size_t i = 0; i < moments.at(k).size(); ++i)
      {
        if (this->Dimension == 2)
        {
          momentsNormal.at(k).set(i,
            moments.at(k).get(i) -
              moments.at(0).get(moments.at(k).getFieldIndices(i)) /
                this->GetTranslationFactor(radiusIndex, 0, 0, 0) *
                this->GetTranslationFactor(radiusIndex,
                  moments.at(k).getOrders(i).at(0),
                  moments.at(k).getOrders(i).at(1),
                  0));
        }
        else
        {
          momentsNormal.at(k).set(i,
            moments.at(k).get(i) -
              moments.at(0).get(moments.at(k).getFieldIndices(i)) /
                this->GetTranslationFactor(radiusIndex, 0, 0, 0) *
                this->GetTranslationFactor(radiusIndex,
                  moments.at(k).getOrders(i).at(0),
                  moments.at(k).getOrders(i).at(1),
                  moments.at(k).getOrders(i).at(2)));
        }
      }
    }
  }
  return momentsNormal;
}

/**
 * normalization with respect to outer transaltion, i.e. the result will be invariant to adding a
 * constant
 * the translational factor is evaluated from the analytic formula
 * @param moments: the moments at one point stored in a vector of tensors
 * @param radius: the integration radius over which the moments were computed
 * @param isTranslation: if normalization w.tr.t. translation is desired by the user
 * @return the translationally normalized moments
 */
std::vector<vtkMomentsTensor> vtkMomentInvariants::NormalizeTAnalytic(
  std::vector<vtkMomentsTensor>& moments,
  double radius,
  bool isTranslation)
{
  /** normalization of the pattern with respect to translation */
  std::vector<vtkMomentsTensor> momentsNormal = moments;
  if (isTranslation)
  {
    for (size_t k = 0; k < moments.size(); ++k)
    {
      for (size_t i = 0; i < moments.at(k).size(); ++i)
      {
        if (this->Dimension == 2)
        {
          momentsNormal.at(k).set(i,
            moments.at(k).get(i) -
              moments.at(0).get(moments.at(k).getFieldIndices(i)) /
                vtkMomentsHelper::translationFactorAnalytic(radius, 2, 0, 0, 0) *
                vtkMomentsHelper::translationFactorAnalytic(radius,
                  2,
                  moments.at(k).getOrders(i).at(0),
                  moments.at(k).getOrders(i).at(1),
                  0));
        }
        else
        {
          momentsNormal.at(k).set(i,
            moments.at(k).get(i) -
              moments.at(0).get(moments.at(k).getFieldIndices(i)) /
                vtkMomentsHelper::translationFactorAnalytic(radius, 3, 0, 0, 0) *
                vtkMomentsHelper::translationFactorAnalytic(radius,
                  3,
                  moments.at(k).getOrders(i).at(0),
                  moments.at(k).getOrders(i).at(1),
                  moments.at(k).getOrders(i).at(2)));
        }
      }
    }
  }
  return momentsNormal;
}

/**
 * normalization with respect to outer scaling, i.e. the result will be invariant to multiplying a
 * constant
 * @param moments: the moments at one point stored in a vector of tensors
 * @param isScaling: if normalization w.tr.t. scalin is desired by the user
 * @return the scale normalized moments
 */
std::vector<vtkMomentsTensor> vtkMomentInvariants::NormalizeS(
  std::vector<vtkMomentsTensor>& moments,
  bool isScaling,
  double radius)
{
  /** normalization of the pattern with respect to scaling */
  std::vector<vtkMomentsTensor> momentsNormal = moments;
  if (isScaling)
  {
    for (size_t k = 0; k < momentsNormal.size(); ++k)
    {
      for (size_t i = 0; i < moments.at(k).size(); ++i)
      {
        momentsNormal.at(k).set(i, moments.at(k).get(i) / pow(radius, k + this->Dimension));
      }
    }
    double norm = 0;
    for (size_t k = 0; k < momentsNormal.size(); ++k)
    {
      norm += momentsNormal.at(k).norm();
    }
    if (norm > 1e-10)
    {
      for (size_t k = 0; k < momentsNormal.size(); ++k)
      {
        for (size_t i = 0; i < momentsNormal.at(k).size(); ++i)
        {
          momentsNormal.at(k).set(i, momentsNormal.at(k).get(i) / norm);
        }
      }
    }
    norm = 0;
    for (size_t k = 0; k < momentsNormal.size(); ++k)
    {
      norm += momentsNormal.at(k).norm();
    }
    if (std::abs(norm - 1) > 1e-10 && norm > 1e-10)
    {
      vtkErrorMacro("The norm is not one after normalization, but " << norm);
    }
  }
  return momentsNormal;
}

/**
 * this functions reads out the parameters from the pattern and checks if they assume reasonable
 * values
 * @param pattern: the pattern that we will look for
 */
void vtkMomentInvariants::InterpretPattern(vtkImageData* pattern)
{
  // dimension
  double bounds[6];
  pattern->GetBounds(bounds);
  if (bounds[5] - bounds[4] < 1e-10)
  {
    this->Dimension = 2;
  }
  else
  {
    this->Dimension = 3;
  }

  // radius
  for (size_t d = 0; d < static_cast<size_t>(this->Dimension); ++d)
  {
    this->RadiusPattern = std::min(this->RadiusPattern, 0.5 * (bounds[2 * d + 1] - bounds[2 * d]));
  }

  // center
  for (size_t d = 0; d < 3; ++d)
  {
    this->CenterPattern[d] = 0.5 * (bounds[2 * d + 1] + bounds[2 * d]);
  }

  if (pattern->GetPointData()->GetNumberOfArrays() == 0)
  {
    vtkErrorMacro("The pattern does not contain any pointdata.");
    return;
  }
  if (this->NameOfPointData == "no name set by user")
  {
    this->NameOfPointData = pattern->GetPointData()->GetArrayName(0);
  }
  if (pattern->GetPointData()->GetArray(this->NameOfPointData.c_str()) == NULL)
  {
    vtkErrorMacro("The pattern does not contain an array by the set name in NameOfPointData.");
    return;
  }

  // FieldRank, i.e. scalars, vectors, or matrices
  int numberOfComponents =
    pattern->GetPointData()->GetArray(this->NameOfPointData.c_str())->GetNumberOfComponents();
  if (numberOfComponents == 1)
  {
    this->FieldRank = 0;
  }
  else if (numberOfComponents == 2 || numberOfComponents == 3)
  {
    this->FieldRank = 1;
  }
  else if (numberOfComponents == 4 || numberOfComponents == 6 || numberOfComponents == 9)
  {
    this->FieldRank = 2;
  }
  else
  {
    vtkErrorMacro("pattern pointdata's number of components does not correspond to 2D or 3D "
                  "scalars, vectors, or matrices.");
    return;
  }

  // if numberOfIntegrationSteps is zero, a point needs to be in the center of the pattern
  if (this->NumberOfIntegrationSteps == 0)
  {
    double center[3];
    pattern->GetPoint(pattern->FindPoint(this->CenterPattern), center);
    for (size_t d = 0; d < static_cast<size_t>(this->Dimension); d++)
    {
      if (center[d] - this->RadiusPattern < bounds[2 * d] - 1e-10 ||
        center[d] + this->RadiusPattern > bounds[2 * d + 1] + 1e-10)
      {
        vtkErrorMacro("If numberOfIntegrationSteps is zero, a point needs to be in the center of "
                      "the pattern. Resample the pattern with an odd dimension. center["
          << d << "] is " << center[d]);
        return;
      }
    }
  }

  // numberOfBasisFunctions
  this->NumberOfBasisFunctions = 0;
  for (size_t k = 0; k < this->Order + 1; ++k)
  {
    this->NumberOfBasisFunctions += pow(this->Dimension, k + this->FieldRank);
  }
}

/**
 * this functions reads out the parameters from the momentData and checks if they assume reasonable
 * values and if they match the ones from the pattern
 * @param moments: the moment data
 */
void vtkMomentInvariants::InterpretField(vtkImageData* moments)
{
  // numer of fields
  this->NumberOfFields = moments->GetPointData()->GetNumberOfArrays();
  // std::cout << "this->NumberOfFields =" << this->NumberOfFields << "\n";

  // fieldrank
  std::string name = moments->GetPointData()->GetArrayName(0);
  int rank = name.length() - 1 - name.find("x");
  if (rank < 0 || rank > 2)
  {
    vtkErrorMacro("the field rank of the moments must be 0, 1, or 2.");
  }
  if ((rank == 0 && this->FieldRank != 0) || (rank == 1 && this->FieldRank != 1) ||
    (rank == 2 && this->FieldRank != 2))
  {
    vtkErrorMacro("field rank of pattern and field must match.");
  }

  // order
  name = moments->GetPointData()->GetArrayName(this->NumberOfFields - 1);
  char buffer = name.back();
  // std::cout<<"buffer="<<buffer <<"end \n";
  if (!(buffer == '2' || buffer == '1' || buffer == 'x'))
  {
    vtkErrorMacro("index of the last moment field must end with 1 or 2.");
  }
  if ((buffer == '2' && this->Dimension == 2) || (buffer == '1' && this->Dimension == 3))
  {
    vtkErrorMacro("the dimensions of the pattern and the field must match.");
  }
  name = moments->GetPointData()->GetArrayName(this->NumberOfFields - 1);
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
    radius = double(int(radius * 1e6)) / 1e6;
    //    std::cout<<"name="<<name <<"end \n";
    //    std::cout<<"buffer2="<<buffer2 <<"end \n";
    //    std::cout<<std::scientific<< setprecision(10)<<"radius="<<radius <<"\n";
    if (this->Radii.size() == 0 || radius != this->Radii.back())
    {
      this->Radii.push_back(radius);
    }
  }
  // std::cout << "this->Radii.size()=" << this->Radii.size() << "\n";

  // number of basis functions
  if (this->NumberOfFields % this->Radii.size() == 0)
  {
    if (!(this->NumberOfBasisFunctions == this->NumberOfFields / this->Radii.size()))
    {
      vtkErrorMacro("the number of fields in moments has to be the product of the number of radii "
                    "and the numberOfBasisFunctions.");
    }
  }
  else
  {
    vtkErrorMacro("the number of fields in moments has to be a multiple of the number of radii.");
  }
}

/** calculation of the dominant contraction
 * there can be multiple dominant contractions due to EV or 3D
 * dominantContractions.at( i ) contains 1 vector in 2D and 2 in 3D
 * dominantContractions.size() = 1 if no EV, 2 if 1 EV, 4 if 2EV are chosen
 * if no contraction was found dominantContractions.size() = 0
 * if only one contraction was found in 3D dominantContractions.at(i).size() = 1 instead of 2
 * @param momentsPattern: the moments of the pattern
 * @return the dominant contractions, i.e. the biggest vectors that can be used for the normalizaion
 * w.r.t. rotation
 */
std::vector<std::vector<vtkMomentsTensor> > vtkMomentInvariants::CalculateDominantContractions(
  std::vector<vtkMomentsTensor>& momentsPatterLocal)
{
  /** calculation of the products */
  std::list<vtkMomentsTensor> contractions;
  for (size_t k = 0; k < momentsPatterLocal.size(); ++k)
  {
    contractions.push_back(momentsPatterLocal.at(k));
  }
  for (std::list<vtkMomentsTensor>::iterator it = contractions.begin(); it != contractions.end();
       ++it)
  {
    if (it->norm() > 1e-3 && it->getRank() > 0) // prevents infinite powers of zero order
    {
      for (size_t k = 0; k < momentsPatterLocal.size(); ++k)
      {
        // cout<<"it->getRank()="<<it->getRank()<<" k="<<k<<endl;
        if (momentsPatterLocal.at(k).norm() > 1e-3 &&
          it->getRank() <= momentsPatterLocal.at(k).getRank() &&
          it->getRank() + momentsPatterLocal.at(k).getRank() < momentsPatterLocal.back().getRank())
        {
          contractions.push_back(vtkMomentsTensor::tensorProduct(*it, momentsPatterLocal.at(k)));
        }
      }
    }
  }

  /** calculation of the contractions */
  for (std::list<vtkMomentsTensor>::iterator it = contractions.begin(); it != contractions.end();
       ++it)
  {
    if (it->getRank() > 2)
    {
      std::vector<vtkMomentsTensor> contractionsTemp = it->contractAll();
      for (size_t i = 0; i < contractionsTemp.size(); ++i)
      {
        contractions.push_back(contractionsTemp.at(i));
      }
    }
  }

  /** calculation of the eigenvectors */
  for (std::list<vtkMomentsTensor>::iterator it = contractions.begin(); it != contractions.end();
       ++it)
  {
    if (it->getRank() == 2 && it->norm() > this->Eps)
    {
      std::vector<vtkMomentsTensor> contractionsTemp = it->eigenVectors();
      for (size_t i = 0; i < contractionsTemp.size(); ++i)
      {
        contractions.push_back(contractionsTemp.at(i));
      }
    }
  }

  /** calculation of the dominant contraction */
  std::vector<std::vector<vtkMomentsTensor> > dominantContractions(1);
  dominantContractions.at(0).push_back(vtkMomentsTensor(
    this->Dimension, momentsPatterLocal.at(0).getRank(), momentsPatterLocal.at(0).getFieldRank()));
  for (std::list<vtkMomentsTensor>::iterator it = contractions.begin(); it != contractions.end();
       ++it)
  {
    if (it->getRank() == 1)
    {
      // cout << "vectors ";
      // it->print();

      if (dominantContractions.at(0).at(0).norm() < it->norm())
      {
        dominantContractions.at(0).at(0) = *it;
      }
    }
  }
  // zero check
  if (dominantContractions.at(0).at(0).norm() < this->Eps)
  {
    // cout << "all contractions up to this rank are zero.";
    return std::vector<std::vector<vtkMomentsTensor> >(0);
    // vtkGenericWarningMacro( "all contractions up to this rank are zero." );
  }

  if (dominantContractions.at(0).at(0).getContractionInfo().size() % 2 == 1)
  {
    dominantContractions.push_back(dominantContractions.at(0));
    dominantContractions.back().at(0).otherEV();
  }

  if (this->Dimension == 3)
  {
    for (size_t i = 0; i < dominantContractions.size(); ++i)
    {
      dominantContractions.at(i).push_back(
        vtkMomentsTensor(this->Dimension, 1, momentsPatterLocal.at(0).getFieldRank()));
      for (std::list<vtkMomentsTensor>::iterator it = contractions.begin();
           it != contractions.end();
           ++it)
      {
        if (it->getRank() == 1)
        {
          if (Eigen::Vector3d(dominantContractions.at(i).at(0).getVector())
                .cross(Eigen::Vector3d(dominantContractions.at(i).at(1).getVector()))
                .norm() <
            Eigen::Vector3d(Eigen::Vector3d(dominantContractions.at(i).at(0).getVector()))
              .cross(Eigen::Vector3d(it->getVector()))
              .norm())
          {
            dominantContractions.at(i).at(1) = *it;
          }
        }
      }

      // zero or dependence check
      if (Eigen::Vector3d(dominantContractions.at(i).at(0).getVector())
            .cross(Eigen::Vector3d(dominantContractions.at(i).at(1).getVector()))
            .norm() < 1e-2 * dominantContractions.at(i).at(0).getVector().norm())
      {
        // cout << "all contractions up to this rank are linearly dependent." << endl;
        for (size_t j = 0; j < dominantContractions.size(); ++j)
        {
          dominantContractions.at(j).resize(1);
        }
        return dominantContractions;
      }
    }

    size_t size = dominantContractions.size();
    for (size_t i = 0; i < size; ++i)
    {
      if (dominantContractions.at(i).at(1).getContractionInfo().size() % 2 == 1)
      {
        dominantContractions.push_back(dominantContractions.at(i));
        dominantContractions.back().at(1).otherEV();
      }
    }
  }

  // check if reproduction was successsful
  for (size_t i = 0; i < dominantContractions.size(); ++i)
  {
    std::vector<vtkMomentsTensor> reproducedContractions =
      ReproduceContractions(dominantContractions.at(i), momentsPatterLocal);
    for (size_t j = 0; j < dominantContractions.at(i).size(); ++j)
    {
      if ((dominantContractions.at(i).at(j).getVector() - reproducedContractions.at(j).getVector())
            .norm() > 1e-3)
      {
        reproducedContractions.at(j)
          .rotate(getRotMat(reproducedContractions, this->Dimension))
          .print();
        vtkGenericWarningMacro("reproduction fails.");
      }
    }
  }
  return dominantContractions;
}

/** the dominant contractions are stored as a vector of integers that encode which tensors were
 * multiplied and contracted to form them. This function applies these excat instructions to the
 * moments in the field. That way, these can be normalized in the same way as the pattern was, which
 * is crucial for the comparison.
 * @param dominantContractions: the vectors that can be used for the normalization of this
 * particular pattern, i.e. the ones that are nt zero or linearly dependent
 * @param moments: the moments at one point
 */
std::vector<vtkMomentsTensor> vtkMomentInvariants::ReproduceContractions(
  std::vector<vtkMomentsTensor>& dominantContractions,
  std::vector<vtkMomentsTensor>& moments)
{
  std::vector<vtkMomentsTensor> reproducedTensors(dominantContractions.size());
  for (size_t i = 0; i < dominantContractions.size(); ++i)
  {
    vtkMomentsTensor reproducedTensor =
      moments.at(dominantContractions.at(i).getProductInfo().at(0) - moments.at(0).getFieldRank());

    for (size_t j = 1; j < dominantContractions.at(i).getProductInfo().size(); ++j)
    {
      reproducedTensor = vtkMomentsTensor::tensorProduct(reproducedTensor,
        moments.at(
          dominantContractions.at(i).getProductInfo().at(j) - moments.at(0).getFieldRank()));
    }
    reproducedTensors.at(i) =
      reproducedTensor.contract(dominantContractions.at(i).getContractionInfo());
  }
  return reproducedTensors;
}

/** normalization of the pattern with respect to rotation and reflection
 * @param dominantContractions: the vectors used for the normalization
 * @param isRotation: if the user wants normalization w.r.t rotation
 * @param isReflection: if the user wants normalization w.r.t reflection
 * @param moments: the moments at a given point
 */
std::vector<vtkMomentsTensor> vtkMomentInvariants::NormalizeR(
  std::vector<vtkMomentsTensor>& dominantContractions,
  bool isRotation,
  bool isReflection,
  std::vector<vtkMomentsTensor>& moments)
{
  if (isRotation || isReflection)
  {
    std::vector<vtkMomentsTensor> momentsNormal = moments;
    // determine rotation matrix to move dominantContraction to positive real axis
    std::vector<vtkMomentsTensor> reproducedContraction =
      this->ReproduceContractions(dominantContractions, moments);
    Eigen::MatrixXd rotMat = getRotMat(reproducedContraction, this->Dimension);
    // rotate the tensors
    for (size_t k = 0; k < moments.size(); ++k)
    {
      momentsNormal.at(k) = moments.at(k).rotate(rotMat);
    }

    return momentsNormal;
  }
  else
  {
    return moments;
  }
}

/** if no dominant contractions could be found to be non-zero, the algorithm defaults back to looking
 * for all possible orientations of the given template the parameter AngleResolution determines what
 * "everywhere" means in 2D, we divide phi=[0,...,2Pi] into that many equidistant steps in 3D, we
 * divide phi=[0,...,2Pi] into that many equidistant steps and theta=[0,...,Pi] in half that many
 * steps to determine the rotation axis. Then, we use anther AngleResolution different rotation
 * angles in [0,...,2Pi] to cover all positions
 * @param momentsPatterNormal: this contains all orientations of the moments of the pattern. during
 * the detection later, we will compare the moments of the field to all these version of the pattern
 * @param momentsPatternTranslationalNormal: this contains the moments that are not invariant to
 * orientation yet
 */
void vtkMomentInvariants::LookEverywhere(
  std::vector<std::vector<vtkMomentsTensor> >& momentsPatternNormal,
  std::vector<vtkMomentsTensor>& momentsPatternTranslationalNormal)
{
  std::vector<vtkMomentsTensor> rotatedMoments = momentsPatternTranslationalNormal;
  //    cout << "rotatedMoments" << endl;
  //    for( size_t k = 0; k < rotatedMoments.size(); ++k )
  //    {
  //        rotatedMoments.at( k ).print();
  //    }
  if (this->Dimension == 2)
  {
    for (size_t i = 0; i < static_cast<size_t>(this->AngleResolution); ++i)
    {
      Eigen::Matrix2d rotMat = getRotMat(2 * M_PI / this->AngleResolution * i);
      for (size_t k = 0; k < momentsPatternTranslationalNormal.size(); ++k)
      {
        rotatedMoments.at(k) = momentsPatternTranslationalNormal.at(k).rotate(rotMat);
      }
      momentsPatternNormal.push_back(rotatedMoments);
    }
  }
  else
  {
    Eigen::Matrix3d rotMat;
    for (size_t i = 0; i < static_cast<size_t>(this->AngleResolution); ++i)
    {
      double phi = 2 * M_PI / this->AngleResolution * i;
      for (size_t j = 0; j < static_cast<size_t>(this->AngleResolution) / 2; ++j)
      {
        double theta = M_PI / this->AngleResolution * j;
        Eigen::Vector3d axis(cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi));
        for (size_t l = 0; l < static_cast<size_t>(this->AngleResolution); ++l)
        {
          rotMat = Eigen::AngleAxisd(2 * M_PI / this->AngleResolution * l, axis);
          for (size_t k = 0; k < momentsPatternTranslationalNormal.size(); ++k)
          {
            rotatedMoments.at(k) = momentsPatternTranslationalNormal.at(k).rotate(rotMat);
          }
          momentsPatternNormal.push_back(rotatedMoments);
        }
      }
    }
  }
}

/** if only one dominant contraction could be found to be non-zero, but no second one to be linearly
 * independent from the first one, the algorithm, will rotate the first contraction to the x-axis
 * and the look for all possible orientations of the given template around this axis. In principal,
 * it reduces the 3D problem to a 2D problem. the parameter AngleResolution determines what
 * "everywhere" means we divide phi=[0,...,2Pi] into that many equidistant steps
 * @param dominantContractions: the vectors used for the normalization
 * @param momentsPatternNormal: this contains all orientations of the moments of the pattern. during
 * the detection later, we will compare the moments of the field to all these version of the pattern
 */
void vtkMomentInvariants::LookEverywhere(
  std::vector<std::vector<vtkMomentsTensor> >& dominantContractions,
  std::vector<std::vector<vtkMomentsTensor> >& momentsPatternNormal)
{
  std::vector<vtkMomentsTensor> rotatedMoments = momentsPatternNormal.at(0);
  Eigen::Matrix3d rotMat;
  for (size_t i = 0; i < dominantContractions.size(); ++i)
  {
    Eigen::Vector3d axis = dominantContractions.at(i).at(0).getVector();
    for (size_t j = 1; j < static_cast<size_t>(this->AngleResolution); ++j)
    {
      rotMat = Eigen::AngleAxisd(2 * M_PI / this->AngleResolution * j, axis);
      for (size_t k = 0; k < momentsPatternNormal.at(0).size(); ++k)
      {
        rotatedMoments.at(k) = momentsPatternNormal.at(i).at(k).rotate(rotMat);
      }
      momentsPatternNormal.push_back(rotatedMoments);
    }
  }
}

/** calculation of the moments of the pattern and its invariants.
 * we choose, which contractions (dominantContractions) can be used for the normalization of this
 * particular pattern, i.e. the ones that are nt zero or linearly dependent. They will later be
 * used for the normalization of the field moments, too.
 * @param dominantContractions: the vectors that can be used for the normalization of this
 * particular pattern, i.e. the ones that are nt zero or linearly dependent
 * @param momentsPatternNormal: the moment invariants of the pattern
 * @param pattern: the pattern
 * @param originalMomentsPattern: the moments of the pattern
 * @param normalizedMomentsPattern: the normalized moments of the pattern. It
 * visualizes how the standard position of this particular pattern looks like
 */
void vtkMomentInvariants::HandlePattern(
  std::vector<std::vector<vtkMomentsTensor> >& dominantContractions,
  vtkImageData* pattern,
  vtkImageData* originalMomentsPattern,
  vtkImageData* normalizedMomentsPattern)
{
  // calculation of the moments of the pattern
  if (this->NumberOfIntegrationSteps == 0)
  {
    this->MomentsPattern = vtkMomentsHelper::allMomentsOrigResImageData(this->Dimension,
      this->Order,
      this->FieldRank,
      this->RadiusPattern,
      pattern->FindPoint(this->CenterPattern),
      pattern,
      this->NameOfPointData);
    // normalize the moments of the pattern w.r.t translation
    this->MomentsPatternTNormal =
      this->NormalizeT(this->MomentsPattern, this->RadiusPattern, this->IsTranslation, pattern);
  }
  else
  {
    vtkImageData* stencil = vtkImageData::New();
    vtkMomentsHelper::BuildStencil(stencil,
      this->RadiusPattern,
      this->NumberOfIntegrationSteps,
      this->Dimension,
      pattern,
      this->NameOfPointData);
    vtkMomentsHelper::CenterStencil(
      this->CenterPattern, pattern, stencil, this->NumberOfIntegrationSteps, this->NameOfPointData);
    this->MomentsPattern = vtkMomentsHelper::allMoments(this->Dimension,
      this->Order,
      this->FieldRank,
      this->RadiusPattern,
      this->CenterPattern,
      stencil,
      this->NameOfPointData);
    // normalize the moments of the pattern w.r.t translation
    this->MomentsPatternTNormal =
      this->NormalizeT(this->MomentsPattern, this->RadiusPattern, this->IsTranslation, stencil);
    stencil->Delete();
  }

  this->MomentsPatternTSNormal =
    this->NormalizeS(this->MomentsPatternTNormal, this->IsScaling, this->RadiusPattern);

  if (this->IsRotation || this->IsReflection)
  {
    // calculation of the dominant contraction
    dominantContractions = this->CalculateDominantContractions(this->MomentsPatternTSNormal);
    // no dominant contraction could be found?
    if (dominantContractions.size() == 0)
    {
      this->LookEverywhere(this->MomentsPatternNormal, this->MomentsPatternTSNormal);
    }
    else
    {
      for (size_t i = 0; i < dominantContractions.size(); ++i)
      {
        std::vector<vtkMomentsTensor> reproducedContractions =
          this->ReproduceContractions(dominantContractions.at(i), this->MomentsPatternTSNormal);
        for (size_t j = 0; j < dominantContractions.at(i).size(); ++j)
        {
          reproducedContractions.at(j).rotate(getRotMat(reproducedContractions, this->Dimension));
        }
      }

      // normalization of the pattern
      for (size_t i = 0; i < dominantContractions.size(); ++i)
      {
        this->MomentsPatternNormal.push_back(this->NormalizeR(dominantContractions.at(i),
          this->IsRotation,
          this->IsReflection,
          this->MomentsPatternTSNormal));
      }
      // 3D and only one dominant contraction could be found
      if (this->Dimension == 3 && dominantContractions.at(0).size() == 1)
      {
        this->LookEverywhere(dominantContractions, this->MomentsPatternNormal);
      }
      for (size_t i = 0; i < dominantContractions.size(); ++i)
      {
        if (!this->IsRotation)
        {
          for (size_t k = 0; k < this->MomentsPatternNormal.back().size(); ++k)
          {
            for (size_t j = 0; j < this->MomentsPatternNormal.back().at(k).size(); ++j)
            {
              this->MomentsPatternNormal.back().at(k).set(j,
                pow(-1,
                  this->MomentsPatternNormal.back().at(k).getIndexSum(j).at(this->Dimension - 1)) *
                  this->MomentsPatternNormal.back().at(k).get(j));
            }
          }
        }
        if (this->IsRotation && this->IsReflection)
        {
          this->MomentsPatternNormal.push_back(this->MomentsPatternNormal.back());
          for (size_t k = 0; k < MomentsPatternNormal.back().size(); ++k)
          {
            for (size_t j = 0; j < MomentsPatternNormal.back().at(k).size(); ++j)
            {
              this->MomentsPatternNormal.back().at(k).set(j,
                pow(-1,
                  this->MomentsPatternNormal.back().at(k).getIndexSum(j).at(this->Dimension - 1)) *
                  this->MomentsPatternNormal.back().at(k).get(j));
            }
          }
        }
      }
    }
  }
  else
  {
    this->MomentsPatternNormal.push_back(this->MomentsPatternTSNormal);
  }

  // store moments as output
  originalMomentsPattern->SetOrigin(this->CenterPattern);
  originalMomentsPattern->SetExtent(0, 0, 0, 0, 0, 0);
  for (size_t i = 0; i < this->NumberOfBasisFunctions; ++i)
  {
    vtkDoubleArray* array = vtkDoubleArray::New();
    std::string fieldName = "radius" + std::to_string(this->RadiusPattern) + "index" +
      vtkMomentsHelper::getTensorIndicesFromFieldIndexAsString(
        i, this->Dimension, this->Order, this->FieldRank)
        .c_str();
    array->SetName(fieldName.c_str());
    array->SetNumberOfTuples(1);
    originalMomentsPattern->GetPointData()->AddArray(array);
    array->Delete();
  }
  for (unsigned int k = 0; k < this->Order + 1; ++k)
  {
    for (size_t i = 0; i < this->MomentsPattern.at(k).size(); ++i)
    {
      originalMomentsPattern->GetPointData()
        ->GetArray(vtkMomentsHelper::getFieldIndexFromTensorIndices(0,
          this->MomentsPattern.at(k).getIndices(i),
          this->Dimension,
          this->FieldRank,
          this->NumberOfBasisFunctions))
        ->SetTuple1(0, this->MomentsPattern.at(k).get(i));
    }
  }
  normalizedMomentsPattern->SetOrigin(this->CenterPattern);
  normalizedMomentsPattern->SetExtent(0, 0, 0, 0, 0, 0);
  for (size_t i = 0; i < this->NumberOfBasisFunctions; ++i)
  {
    vtkDoubleArray* array = vtkDoubleArray::New();
    std::string fieldName = "radius" + std::to_string(this->RadiusPattern) + "index" +
      vtkMomentsHelper::getTensorIndicesFromFieldIndexAsString(
        i, this->Dimension, this->Order, this->FieldRank)
        .c_str();
    array->SetName(fieldName.c_str());
    array->SetNumberOfTuples(1);
    normalizedMomentsPattern->GetPointData()->AddArray(array);
    array->Delete();
  }

  for (unsigned int k = 0; k < this->Order + 1; ++k)
  {
    for (size_t i = 0; i < this->MomentsPatternNormal.at(0).at(k).size(); ++i)
    {
      normalizedMomentsPattern->GetPointData()
        ->GetArray(vtkMomentsHelper::getFieldIndexFromTensorIndices(0,
          this->MomentsPatternNormal.at(0).at(k).getIndices(i),
          this->Dimension,
          this->FieldRank,
          this->NumberOfBasisFunctions))
        ->SetTuple1(0, this->MomentsPatternNormal.at(0).at(k).get(i));
    }
  }
}

/** main part of the pattern detection
 * the moments of the field at each point are normalized and compared to the moments of the pattern
 * @param dominantContractions: the dominant contractions, i.e. vectors for the normalization w.r.t.
 * rotation
 * @param moments: the moments of the field
 * @param normalizedMoments: the moment invariants of the field
 * @param pattern: the pattern
 * @param similarityFields: the output of this algorithm. it has the topology of moments and will
 * have a number of scalar fields euqal to NumberOfRadii. each point contains the similarity of its
 * surrounding (of size radius) to the pattern
 */
void vtkMomentInvariants::HandleField(
  std::vector<std::vector<vtkMomentsTensor> >& dominantContractions,
  vtkImageData* moments,
  vtkImageData* normalizedMoments,
  vtkImageData* pattern,
  vtkImageData* similarityFields)
{
  normalizedMoments->CopyStructure(moments);
  similarityFields->CopyStructure(moments);

  // vector of arrays for the moments. the name is the tensor indices
  for (size_t r = 0; r < this->Radii.size(); ++r)
  {
    vtkDoubleArray* similarity = vtkDoubleArray::New();
    similarity->SetName(std::to_string(this->Radii.at(r)).c_str());
    similarity->SetNumberOfTuples(moments->GetNumberOfPoints());
    similarityFields->GetPointData()->AddArray(similarity);
    similarity->Delete();

    for (size_t i = 0; i < this->NumberOfBasisFunctions; ++i)
    {
      vtkDoubleArray* array = vtkDoubleArray::New();
      std::string fieldName = "radius" + std::to_string(this->Radii.at(r)) + "index" +
        vtkMomentsHelper::getTensorIndicesFromFieldIndexAsString(
          i, this->Dimension, this->Order, this->FieldRank)
          .c_str();
      array->SetName(fieldName.c_str());
      array->SetNumberOfTuples(moments->GetNumberOfPoints());
      normalizedMoments->GetPointData()->AddArray(array);
      array->Delete();
    }
  }

  // prepare the translational factors. They will be reused for all points
  this->BuildTranslationalFactorArray(pattern);

  // read the tensor vector, normalize it, compute similarity, and store it in the output
  for (size_t r = 0; r < this->Radii.size(); ++r)
  {
    for (int j = 0; j < moments->GetNumberOfPoints(); ++j)
    {
      // read the moment vector
      std::vector<vtkMomentsTensor> tensorVector(this->Order + 1);
      for (unsigned int k = 0; k < this->Order + 1; ++k)
      {
        tensorVector.at(k) =
          vtkMomentsTensor(this->Dimension, k + this->FieldRank, this->FieldRank);
        for (size_t i = 0; i < tensorVector.at(k).size(); ++i)
        {
          tensorVector.at(k).set(i,
            moments->GetPointData()
              ->GetArray(vtkMomentsHelper::getFieldIndexFromTensorIndices(r,
                tensorVector.at(k).getIndices(i),
                this->Dimension,
                this->FieldRank,
                this->NumberOfBasisFunctions))
              ->GetTuple(j)[0]);
        }
      }
      // normalize the moment vector
      std::vector<vtkMomentsTensor> tensorVectorTNormal =
        this->NormalizeT(tensorVector, r, this->IsTranslation);

      std::vector<vtkMomentsTensor> tensorVectorTSNormal =
        this->NormalizeS(tensorVectorTNormal, this->IsScaling, this->Radii.at(r));
      std::vector<vtkMomentsTensor> tensorVectorNormal = tensorVectorTSNormal;
      if (dominantContractions.size() > 0)
      {
        tensorVectorNormal = this->NormalizeR(
          dominantContractions.at(0), this->IsRotation, this->IsReflection, tensorVectorTSNormal);
      }
      // compute similarity to pattern
      if (vtkMomentsHelper::isCloseToEdge(this->Dimension, j, this->Radii.at(r), moments))
      {
        similarityFields->GetPointData()
          ->GetArray(std::to_string(this->Radii.at(r)).c_str())
          ->SetTuple1(j, 0);
      }
      else
      {
        double distance = std::numeric_limits<double>::max();
        for (size_t i = 0; i < this->MomentsPatternNormal.size(); ++i)
        {
          double distanceTemp = 0;
          for (unsigned int k = 0; k < this->Order + 1; ++k)
          {
            distanceTemp += vtkMomentsTensor::tensorDistance(
              this->MomentsPatternNormal.at(i).at(k), tensorVectorNormal.at(k));
          }
          distance = std::min(distance, distanceTemp);
        }
        similarityFields->GetPointData()
          ->GetArray(std::to_string(this->Radii.at(r)).c_str())
          ->SetTuple1(j, 1. / distance);
      }
      // store normalized moments in the output
      for (unsigned int k = 0; k < this->Order + 1; ++k)
      {
        for (size_t i = 0; i < tensorVectorNormal.at(k).size(); ++i)
        {
          normalizedMoments->GetPointData()
            ->GetArray(vtkMomentsHelper::getFieldIndexFromTensorIndices(r,
              tensorVectorNormal.at(k).getIndices(i),
              this->Dimension,
              this->FieldRank,
              this->NumberOfBasisFunctions))
            ->SetTuple1(j, tensorVectorNormal.at(k).get(i));
        }
      }
    }
  }
}

/**
 * Make sure that the user has not entered weird values.
 * @param pattern: the pattern that we will look for
 */
void vtkMomentInvariants::CheckValidity(vtkImageData* pattern)
{
  if (!pattern)
  {
    vtkErrorMacro("A pattern needs to be provided through SetInputData().");
    return;
  }
  if (this->NumberOfIntegrationSteps < 0)
  {
    vtkErrorMacro("The number of integration steps must be >= 0.");
    return;
  }
}

/**
 * this computes the translational factors necessary for normalization w.r.t. translation
 * we have radius and then p,q,r
 * @param stencil: the points in this stencil are used to numerically approximate the integral
 */
void vtkMomentInvariants::BuildTranslationalFactorArray(vtkImageData* pattern)
{
  if (this->IsTranslation)
  {
    delete[] this->TranslationFactor;
    this->TranslationFactor = new double[this->Radii.size() *
      int(pow((this->Order + 1) + this->FieldRank, this->Dimension))];
    for (size_t radiusIndex = 0; radiusIndex < this->Radii.size(); ++radiusIndex)
    {
      // prepare stencil
      vtkImageData* stencil = vtkImageData::New();
      if (this->NumberOfIntegrationSteps == 0)
      {
        stencil->CopyStructure(pattern);
        if (this->Dimension == 2)
        {
          stencil->SetSpacing(
            pattern->GetSpacing()[0] / this->RadiusPattern * this->Radii.at(radiusIndex),
            pattern->GetSpacing()[1] / this->RadiusPattern * this->Radii.at(radiusIndex),
            1);
        }
        else
        {
          stencil->SetSpacing(
            pattern->GetSpacing()[0] / this->RadiusPattern * this->Radii.at(radiusIndex),
            pattern->GetSpacing()[1] / this->RadiusPattern * this->Radii.at(radiusIndex),
            pattern->GetSpacing()[2] / this->RadiusPattern * this->Radii.at(radiusIndex));
        }
      }
      else
      {
        vtkMomentsHelper::BuildStencil(stencil,
          this->Radii.at(radiusIndex),
          this->NumberOfIntegrationSteps,
          this->Dimension,
          pattern,
          this->NameOfPointData);
      }
      // compute factor
      for (size_t p = 0; p < (this->Order + 1); ++p)
      {
        for (size_t q = 0; q < (this->Order + 1) - p; ++q)
        {
          if (this->Dimension == 2)
          {
            this->SetTranslationFactor(radiusIndex,
              p,
              q,
              0,
              vtkMomentsHelper::translationFactor(this->Radii.at(radiusIndex), p, q, 0, stencil));
          }
          else
          {
            for (size_t r = 0; r < (this->Order + 1) - p - q; ++r)
            {
              this->SetTranslationFactor(radiusIndex,
                p,
                q,
                r,
                vtkMomentsHelper::translationFactor(this->Radii.at(radiusIndex), p, q, r, stencil));
            }
          }
        }
      }
      stencil->Delete();
    }
  }
}

int vtkMomentInvariants::RequestUpdateExtent(vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector*)
{
  // We need to ask for the whole extent from this input.
  vtkInformation* momentInfo = inputVector[1]->GetInformationObject(0);

  if (momentInfo)
  {
    momentInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

    momentInfo->Remove(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
    if (momentInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
    {
      momentInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        momentInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
        6);
    }

    momentInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
    momentInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
    momentInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  }
  return 1;
}

/** main executive of the program, reads the input, calles the functions, and produces the utput.
 * @param request: ?
 * @param inputVector: the input information
 * @param outputVector: the output information
 */
int vtkMomentInvariants::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* patternInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* momentsInfo = inputVector[1]->GetInformationObject(0);
  // similarity fields
  vtkInformation* outInfo0 = outputVector->GetInformationObject(0);
  // normalized moments of the field
  vtkInformation* outInfo1 = outputVector->GetInformationObject(1);
  // moments of the pattern
  vtkInformation* outInfo2 = outputVector->GetInformationObject(2);
  // normalized moments of the pattern
  vtkInformation* outInfo3 = outputVector->GetInformationObject(3);

  // get the input and output
  vtkImageData* pattern =
    vtkImageData::SafeDownCast(patternInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData* momentData;
  if (momentsInfo)
  {
    momentData = vtkImageData::SafeDownCast(momentsInfo->Get(vtkDataObject::DATA_OBJECT()));
  }
  vtkImageData* similarityFields =
    vtkImageData::SafeDownCast(outInfo0->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData* normalizedMomentsField =
    vtkImageData::SafeDownCast(outInfo1->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData* originalMomentsPattern =
    vtkImageData::SafeDownCast(outInfo2->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData* normalizedMomentsPattern =
    vtkImageData::SafeDownCast(outInfo3->Get(vtkDataObject::DATA_OBJECT()));

  CheckValidity(pattern);
  this->InterpretPattern(pattern);

  if (momentsInfo)
  {
    this->InterpretField(momentData);
  }

  std::vector<std::vector<vtkMomentsTensor> > dominantContractions;

  this->HandlePattern(
    dominantContractions, pattern, originalMomentsPattern, normalizedMomentsPattern);

  if (momentsInfo)
  {
    this->HandleField(
      dominantContractions, momentData, normalizedMomentsField, pattern, similarityFields);
  }

  return 1;
}
