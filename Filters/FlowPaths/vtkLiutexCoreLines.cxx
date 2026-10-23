// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLiutexCoreLines.h"

#include "vtkDataSet.h"
#include "vtkGradientFilter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLiutex.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkParallelVectors.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
constexpr const char* LiutexVectorName = "Liutex";
constexpr const char* LiutexMagnitudeName = "LiutexMagnitude";
constexpr const char* MagnitudeGradientName = "vtkLiutexCoreLinesMagnitudeGradient";
}

VTK_ABI_NAMESPACE_BEGIN
class vtkParallelVectorsForLiutexCoreLines : public vtkParallelVectors
{
public:
  static vtkParallelVectorsForLiutexCoreLines* New();
  vtkTypeMacro(vtkParallelVectorsForLiutexCoreLines, vtkParallelVectors);

  void SetLiutexMagnitude(vtkDataArray* magnitude, double minimum)
  {
    this->Magnitude = magnitude;
    this->Minimum = minimum;
  }

protected:
  vtkParallelVectorsForLiutexCoreLines() = default;
  ~vtkParallelVectorsForLiutexCoreLines() override = default;

  bool AcceptSurfaceTriangle(const vtkIdType surfaceSimplexIndices[3]) override
  {
    for (int i = 0; i < 3; ++i)
    {
      if (!(this->Magnitude->GetComponent(surfaceSimplexIndices[i], 0) > this->Minimum))
      {
        return false;
      }
    }
    return true;
  }

  bool ComputeAdditionalCriteria(const vtkIdType surfaceSimplexIndices[3], double s, double t,
    std::vector<double>& vtkNotUsed(criterionArrayValues)) override
  {
    const double magnitude =
      (1. - s - t) * this->Magnitude->GetComponent(surfaceSimplexIndices[0], 0) +
      s * this->Magnitude->GetComponent(surfaceSimplexIndices[1], 0) +
      t * this->Magnitude->GetComponent(surfaceSimplexIndices[2], 0);
    return magnitude > this->Minimum;
  }

private:
  vtkParallelVectorsForLiutexCoreLines(const vtkParallelVectorsForLiutexCoreLines&) = delete;
  void operator=(const vtkParallelVectorsForLiutexCoreLines&) = delete;

  vtkSmartPointer<vtkDataArray> Magnitude;
  double Minimum = 0.;
};
vtkStandardNewMacro(vtkParallelVectorsForLiutexCoreLines);

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkLiutexCoreLines);

//------------------------------------------------------------------------------
vtkLiutexCoreLines::vtkLiutexCoreLines()
{
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::VECTORS);
}

//------------------------------------------------------------------------------
vtkLiutexCoreLines::~vtkLiutexCoreLines() = default;

//------------------------------------------------------------------------------
int vtkLiutexCoreLines::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkLiutexCoreLines::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0], 0);
  vtkPolyData* output = vtkPolyData::GetData(outputVector, 0);

  vtkNew<vtkLiutex> liutex;
  liutex->SetInputData(input);
  liutex->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));
  liutex->SetLiutexVectorArrayName(LiutexVectorName);
  liutex->SetLiutexMagnitudeArrayName(LiutexMagnitudeName);
  liutex->SetFasterApproximation(this->FasterApproximation);
  liutex->SetContainerAlgorithm(this);
  liutex->Update();
  vtkDataSet* withLiutex = vtkDataSet::SafeDownCast(liutex->GetOutputDataObject(0));
  if (!withLiutex || !withLiutex->GetPointData()->GetArray(LiutexMagnitudeName))
  {
    vtkErrorMacro("Could not compute Liutex. The input array must be point data.");
    return 0;
  }
  if (this->CheckAbort())
  {
    return 1;
  }

  vtkNew<vtkGradientFilter> gradient;
  gradient->SetInputData(withLiutex);
  gradient->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, LiutexMagnitudeName);
  gradient->SetResultArrayName(MagnitudeGradientName);
  gradient->SetFasterApproximation(this->FasterApproximation);
  gradient->SetContainerAlgorithm(this);
  gradient->Update();
  if (!gradient->GetOutput()->GetPointData()->GetArray(MagnitudeGradientName))
  {
    vtkErrorMacro("Could not compute the gradient of the Liutex magnitude.");
    return 0;
  }
  if (this->CheckAbort())
  {
    return 1;
  }

  vtkDataSet* work = gradient->GetOutput();

  vtkNew<vtkParallelVectorsForLiutexCoreLines> parallelVectors;
  parallelVectors->SetInputData(work);
  parallelVectors->SetFirstVectorFieldName(MagnitudeGradientName);
  parallelVectors->SetSecondVectorFieldName(LiutexVectorName);
  parallelVectors->SetLiutexMagnitude(
    work->GetPointData()->GetArray(LiutexMagnitudeName), this->MinimumLiutexMagnitude);
  parallelVectors->SetContainerAlgorithm(this);
  parallelVectors->Update();

  output->ShallowCopy(parallelVectors->GetOutput());
  output->GetPointData()->RemoveArray(MagnitudeGradientName);
  return 1;
}

//------------------------------------------------------------------------------
void vtkLiutexCoreLines::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MinimumLiutexMagnitude: " << this->MinimumLiutexMagnitude << endl;
  os << indent << "FasterApproximation: " << this->FasterApproximation << endl;
}
VTK_ABI_NAMESPACE_END
