// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSphericalHarmonics.h"

#include "vtkArrayDispatch.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"
#include "vtkTable.h"

#include <array>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkSphericalHarmonics);

namespace
{

struct ComputeSH
{
  vtkIdType Width;
  vtkIdType Height;
  vtkFloatArray* Harmonics;
  vtkSphericalHarmonics* Filter;

  ComputeSH(
    vtkIdType width, vtkIdType height, vtkFloatArray* outArray, vtkSphericalHarmonics* filter)
    : Width(width)
    , Height(height)
    , Harmonics(outArray)
    , Filter(filter)
  {
  }

  template <typename ArrayType>
  struct Impl // SMP functor
  {
    ArrayType* Image;
    vtkIdType Width;
    vtkIdType Height;
    std::array<std::array<double, 9>, 3> Harmonics = {};

    vtkSMPThreadLocal<double> LocalWeight;
    vtkSMPThreadLocal<std::array<std::array<double, 9>, 3>> LocalHarmonics;
    vtkSphericalHarmonics* Filter;

    Impl(vtkIdType w, vtkIdType h, ArrayType* image, vtkSphericalHarmonics* filter)
      : Image(image)
      , Width(w)
      , Height(h)
      , Filter(filter)
    {
    }

    void Initialize()
    {
      this->LocalHarmonics.Local() = {};
      this->LocalWeight.Local() = 0.0;
    }

    void operator()(vtkIdType ybegin, vtkIdType yend)
    {
      using T = typename ArrayType::ValueType;

      // The solid angle for each pixel is equal to (2 * pi / dimX) * (pi / dimY)
      const double solidAngle = 2.0 * vtkMath::Pi() * vtkMath::Pi() / (this->Width * this->Height);
      double& localWeight = this->LocalWeight.Local();
      auto& localHarmonics = this->LocalHarmonics.Local();
      bool isFirst = vtkSMPTools::GetSingleThread();

      for (vtkIdType i = ybegin; i < yend; i++)
      {
        if (isFirst)
        {
          this->Filter->CheckAbort();
        }
        if (this->Filter->GetAbortOutput())
        {
          break;
        }
        double theta = ((i + 0.5) / static_cast<double>(this->Height)) * vtkMath::Pi();
        double ct = std::cos(theta);
        double st = std::sin(theta);
        double weight = solidAngle * st;

        for (vtkIdType j = 0; j < this->Width; j++)
        {
          double phi = (((j + 0.5) / static_cast<double>(this->Width)) * 2.0 - 1.0) * vtkMath::Pi();
          double cp = std::cos(phi);
          double sp = std::sin(phi);

          // conversion to cartesian coordinates
          // note that we are using VTK/OpenGL coordinates (Y up) so we need to rotate
          double n[3] = { st * cp, -ct, st * sp };

          double basis[9] = { 0.282095, -0.488603 * n[1], 0.488603 * n[2], -0.488603 * n[0],
            1.092548 * n[0] * n[1], -1.092548 * n[1] * n[2], 0.315392 * (3.0 * n[2] * n[2] - 1.0),
            -1.092548 * n[0] * n[2], 0.546274 * (n[0] * n[0] - n[1] * n[1]) };

          localWeight += weight;

          // in case we have an alpha channel, we ignore it
          for (int k = 0; k < 3; k++)
          {
            double v = static_cast<double>(this->Image->GetTypedComponent(this->Width * i + j, k));

            // integral types must be normalized
            if (std::is_integral<T>::value)
            {
              v /= static_cast<double>(std::numeric_limits<T>::max());

              // 8-bits images are usually encoded in sRGB color space
              // conversion to linear color space is required
              if (sizeof(T) == 1)
              {
                v = pow(v, 2.2);
              }
            }

            auto& compSH = localHarmonics[k];

            for (int y = 0; y < 9; y++)
            {
              compSH[y] += weight * v * basis[y];
            }
          }
        }
      }
    }

    void Reduce()
    {
      double weightSum = 0.0;
      for (double w : this->LocalWeight)
      {
        weightSum += w;
      }

      // the surface of the sphere is equal to 4 * pi
      // the weight sum should also be equal to 4 * pi but we normalize just in case.
      double normalizeFactor = 4.0 * vtkMath::Pi() / weightSum;
      for (const auto& l : this->LocalHarmonics)
      {
        for (size_t i = 0; i < 3; i++)
        {
          for (size_t j = 0; j < 9; j++)
          {
            this->Harmonics[i][j] += normalizeFactor * l[i][j];
          }
        }
      }
    }
  };

  template <typename ArrayType>
  void operator()(ArrayType* image)
  {
    Impl<ArrayType> functor(this->Width, this->Height, image, this->Filter);
    vtkSMPTools::For(0, this->Height, functor);

    for (vtkIdType i = 0; i < 3; i++)
    {
      for (int j = 0; j < 9; j++)
      {
        this->Harmonics->SetTypedComponent(i, j, functor.Harmonics[i][j]);
      }
    }
  }
};
} // end anon namespace

//----------------------------------------------------------------------------
int vtkSphericalHarmonics::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkImageData* input = vtkImageData::GetData(inputVector[0]);
  vtkTable* output = vtkTable::GetData(outputVector);

  vtkIdType dimensions[3];
  input->GetDimensions(dimensions);

  int nbComp = input->GetNumberOfScalarComponents();

  if ((nbComp != 3 && nbComp != 4) || dimensions[2] > 1)
  {
    vtkErrorMacro("Only 2D images with RGB or RGBA attributes are supported.");
    return 0;
  }

  vtkNew<vtkFloatArray> harmonics;
  harmonics->SetName("SphericalHarmonics");
  harmonics->SetNumberOfComponents(9);
  harmonics->SetNumberOfTuples(3);

  ComputeSH worker(dimensions[0], dimensions[1], harmonics, this);

  vtkDataArray* scalars = input->GetPointData()->GetScalars();

  if (!scalars)
  {
    vtkErrorMacro("No scalars found in image point data.");
    return 0;
  }

  if (!vtkArrayDispatch::Dispatch::Execute(scalars, worker))
  {
    vtkErrorMacro("Computation of spherical harmonics failed.");
    return 0;
  }

  output->AddColumn(harmonics);

  return 1;
}

//----------------------------------------------------------------------------
void vtkSphericalHarmonics::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkSphericalHarmonics::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");
  return 1;
}
VTK_ABI_NAMESPACE_END
