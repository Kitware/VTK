// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLinearTransformCellLocator.h"

#include "vtkArrayDispatch.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSMPTools.h"
#include "vtkTransform.h"

#include "vtk_eigen.h"
#include VTK_EIGEN(Geometry)

static constexpr vtkIdType VTK_MAX_SAMPLE_POINTS = 100;

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkLinearTransformCellLocator);

//------------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkLinearTransformCellLocator, CellLocator, vtkAbstractCellLocator);

//------------------------------------------------------------------------------
vtkLinearTransformCellLocator::vtkLinearTransformCellLocator()
{
  this->CellLocator = nullptr;
  this->Transform = vtkSmartPointer<vtkTransform>::New();
  this->InverseTransform = vtkSmartPointer<vtkTransform>::New();
}

//------------------------------------------------------------------------------
vtkLinearTransformCellLocator::~vtkLinearTransformCellLocator()
{
  this->SetCellLocator(nullptr);
}

//------------------------------------------------------------------------------
void vtkLinearTransformCellLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->CellLocator)
  {
    os << indent << "CellLocator: " << this->CellLocator << "\n";
  }
  else
  {
    os << indent << "CellLocator: (none)\n";
  }
  os << indent << "Transform: " << this->Transform << "\n";
  os << indent << "InverseTransform: " << this->InverseTransform << "\n";
  os << indent << "IsLinearTransformation: " << this->IsLinearTransformation << "\n";
  os << indent << "UseAllPoints: " << this->UseAllPoints << "\n";
}

//------------------------------------------------------------------------------
struct ComputeTransformationWorker
{
  Eigen::Matrix3d RotationMatrix;
  Eigen::Vector3d TranslationVector;

  template <typename PointsArray>
  void FastTransformComputation(
    PointsArray* points1, PointsArray* points2, bool& validTransformation)
  {
    using ValueType = typename PointsArray::ValueType;
    using MatrixX = Eigen::Matrix<ValueType, Eigen::Dynamic, Eigen::Dynamic>;
    using Matrix3 = Eigen::Matrix<ValueType, 3, 3>;
    using Vector3 = Eigen::Matrix<ValueType, 3, 1>;
    auto p1 = Eigen::Map<MatrixX>(points1->GetPointer(0), 3, (points1)->GetNumberOfTuples());
    auto p2 = Eigen::Map<MatrixX>(points2->GetPointer(0), 3, (points2)->GetNumberOfTuples());
    // find the rotation and translation matrix between 2 sets of points
    Vector3 p1BaryCenter = p1.rowwise().mean();
    Vector3 p2BaryCenter = p2.rowwise().mean();
    auto centeredP1 = p1.colwise() - p1BaryCenter;
    auto centeredP2 = p2.colwise() - p2BaryCenter;
    auto covarianceMatrix = centeredP2 * centeredP1.transpose();
    Eigen::JacobiSVD<MatrixX> svd(covarianceMatrix, Eigen::ComputeFullV | Eigen::ComputeFullU);
    // both matrices are 3x3
    auto matrixV = svd.matrixV();
    auto& matrixU = svd.matrixU();
    Matrix3 rotationMatrix = matrixV * matrixU.transpose();
    // there is reflection
    if (rotationMatrix.determinant() < 0)
    {
      matrixV.col(2) *= -1;
      rotationMatrix = matrixV * matrixU.transpose();
    }
    Vector3 translationVector = -rotationMatrix * p2BaryCenter + p1BaryCenter;
    // calculate the root mean squared error between the actual p1 and replicated p2
    auto rotatedP2 = (rotationMatrix * p2).colwise() + translationVector;
    auto errorMatrix = rotatedP2 - p1;
    double rootMeanSquaredError = std::sqrt(errorMatrix.array().square().sum() / p1.cols());
    // check if p2 is a linear transformation of p1
    if (rootMeanSquaredError <= 0.001)
    {
      validTransformation = true;
      this->RotationMatrix = rotationMatrix.template cast<double>();
      this->TranslationVector = translationVector.template cast<double>();
    }
    else
    {
      validTransformation = false;
      vtkGenericWarningMacro(
        "Points are not close enough to be considered a linear transformation. "
        << rootMeanSquaredError);
    }
  }

  template <typename PointsArray1, typename PointsArray2>
  void operator()(PointsArray1* points1, PointsArray2* points2, bool& validTransformation)
  {
    auto p1Range = vtk::DataArrayTupleRange<3>(points1);
    auto p2Range = vtk::DataArrayTupleRange<3>(points2);
    Eigen::MatrixXd p1, p2;
    p1.resize(3, p1Range.size());
    p2.resize(3, p2Range.size());
    vtkSMPTools::For(0, p1Range.size(), [&](vtkIdType begin, vtkIdType end) {
      for (vtkIdType i = begin; i < end; i++)
      {
        p1(0, i) = p1Range[i][0];
        p1(1, i) = p1Range[i][1];
        p1(2, i) = p1Range[i][2];
        p2(0, i) = p2Range[i][0];
        p2(1, i) = p2Range[i][1];
        p2(2, i) = p2Range[i][2];
      }
    });

    // find the rotation and translation matrix between 2 sets of points
    Eigen::Vector3d p1BaryCenter = p1.rowwise().mean();
    Eigen::Vector3d p2BaryCenter = p2.rowwise().mean();
    auto centeredP1 = p1.colwise() - p1BaryCenter;
    auto centeredP2 = p2.colwise() - p2BaryCenter;
    auto covarianceMatrix = centeredP2 * centeredP1.transpose();
    Eigen::JacobiSVD<Eigen::MatrixXd> svd(
      covarianceMatrix, Eigen::ComputeFullV | Eigen::ComputeFullU);
    // both matrices are 3x3
    auto matrixV = svd.matrixV();
    auto& matrixU = svd.matrixU();
    Eigen::Matrix3d rotationMatrix = matrixV * matrixU.transpose();
    // there is reflection
    if (rotationMatrix.determinant() < 0)
    {
      matrixV.col(2) *= -1;
      rotationMatrix = matrixV * matrixU.transpose();
    }
    Eigen::Vector3d translationVector = -rotationMatrix * p2BaryCenter + p1BaryCenter;
    // calculate the root mean squared error between the actual p1 and replicated p2
    auto rotatedP2 = (rotationMatrix * p2).colwise() + translationVector;
    auto errorMatrix = rotatedP2 - p1;
    double rootMeanSquaredError = std::sqrt(errorMatrix.array().square().sum() / p1.cols());
    // check if p2 is a linear transformation of p1
    if (rootMeanSquaredError <= 0.001)
    {
      validTransformation = true;
      this->RotationMatrix = rotationMatrix;
      this->TranslationVector = translationVector;
    }
    else
    {
      validTransformation = false;
      vtkGenericWarningMacro(
        "Points are not close enough to be considered a linear transformation. "
        << rootMeanSquaredError);
    }
  }

  void DefineTransform(vtkTransform* transform, vtkTransform* inverseTransform)
  {
    auto transposeRotationMatrix = this->RotationMatrix.transpose();
    double matrix[4][4];
    vtkMatrix4x4::Identity(*matrix);
    for (uint8_t i = 0; i < 3; ++i)
    {
      for (uint8_t j = 0; j < 3; ++j)
      {
        matrix[i][j] = transposeRotationMatrix(i, j);
      }
    }
    transform->SetMatrix(*matrix);
    auto negativeTranslationVector = -this->TranslationVector;
    transform->Translate(
      negativeTranslationVector[0], negativeTranslationVector[1], negativeTranslationVector[2]);
    transform->Update();
    vtkMatrix4x4::Invert(transform->GetMatrix()->GetData(), *matrix);
    inverseTransform->SetMatrix(*matrix);
    inverseTransform->Update();
  }
};

//------------------------------------------------------------------------------
bool vtkLinearTransformCellLocator::ComputeTransformation()
{
  if (this->DataSet == nullptr || this->CellLocator->GetDataSet() == nullptr)
  {
    vtkErrorMacro("DataSet or CellLocator's DataSet is not set.");
    return false;
  }
  vtkIdType initialNumberOfPoints = this->CellLocator->GetDataSet()->GetNumberOfPoints();
  vtkIdType newNumberOfPoints = this->DataSet->GetNumberOfPoints();
  if (newNumberOfPoints != initialNumberOfPoints)
  {
    vtkErrorMacro("Number of points in the dataset and the cell locator's dataset do not match.");
    return false;
  }
  if (initialNumberOfPoints < 2)
  {
    vtkErrorMacro("Number of points in the dataset is less than 2.");
    return false;
  }
  auto initialPoints = this->CellLocator->GetDataSet()->GetPoints();
  auto newPoints = this->DataSet->GetPoints();
  vtkSmartPointer<vtkPoints> initialPointsSample, newPointsSample;
  vtkDataArray* initialPointsSampleData;
  vtkDataArray* newPointsSampleData;
  if (VTK_MAX_SAMPLE_POINTS >= initialNumberOfPoints || this->UseAllPoints)
  {
    initialPointsSampleData = initialPoints->GetData();
    newPointsSampleData = newPoints->GetData();
  }
  else
  {
    // sample points with a stride
    vtkIdType stride = initialNumberOfPoints / VTK_MAX_SAMPLE_POINTS;
    vtkIdType samplePoints = initialNumberOfPoints / stride;
    initialPointsSample = vtkSmartPointer<vtkPoints>::New();
    newPointsSample = vtkSmartPointer<vtkPoints>::New();
    initialPointsSample->SetDataType(initialPoints->GetDataType());
    newPointsSample->SetDataType(newPoints->GetDataType());
    initialPointsSample->SetNumberOfPoints(samplePoints);
    newPointsSample->SetNumberOfPoints(samplePoints);
    double point[3], point2[3];
    for (vtkIdType i = 0; i < samplePoints; ++i)
    {
      initialPoints->GetPoint(i * stride, point);
      initialPointsSample->SetPoint(i, point);
      newPoints->GetPoint(i * stride, point2);
      newPointsSample->SetPoint(i, point2);
    }
    initialPointsSampleData = initialPointsSample->GetData();
    newPointsSampleData = newPointsSample->GetData();
  }
  bool validTransformation = false;
  ComputeTransformationWorker worker;
  // first try to use the fast version which does not copy the points
  if (vtkDoubleArray::SafeDownCast(initialPointsSampleData) &&
    vtkDoubleArray::SafeDownCast(newPointsSampleData))
  {
    auto initialPointsDataDouble = vtkDoubleArray::SafeDownCast(initialPointsSampleData);
    auto newPointsDataDouble = vtkDoubleArray::SafeDownCast(newPointsSampleData);
    worker.FastTransformComputation<vtkDoubleArray>(
      initialPointsDataDouble, newPointsDataDouble, validTransformation);
  }
  else if (vtkFloatArray::SafeDownCast(initialPointsSampleData) &&
    vtkFloatArray::SafeDownCast(newPointsSampleData))
  {
    auto initialPointsDataFloat = vtkFloatArray::SafeDownCast(initialPointsSampleData);
    auto newPointsDataFloat = vtkFloatArray::SafeDownCast(newPointsSampleData);
    worker.FastTransformComputation<vtkFloatArray>(
      initialPointsDataFloat, newPointsDataFloat, validTransformation);
  }
  else
  {
    using Dispatcher = vtkArrayDispatch::Dispatch2BySameValueType<vtkArrayDispatch::Reals>;
    if (!Dispatcher::Execute(
          initialPointsSampleData, newPointsSampleData, worker, validTransformation))
    {
      worker(initialPointsSampleData, newPointsSampleData, validTransformation);
    }
  }
  if (validTransformation)
  {
    worker.DefineTransform(this->Transform, this->InverseTransform);
  }
  return validTransformation;
}

//------------------------------------------------------------------------------
void vtkLinearTransformCellLocator::GenerateRepresentation(int level, vtkPolyData* pd)
{
  this->BuildLocator();
  if (this->CellLocator)
  {
    this->CellLocator->GenerateRepresentation(level, pd);
    this->Transform->TransformPoints(pd->GetPoints(), pd->GetPoints());
    pd->GetPoints()->Modified();
    pd->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkLinearTransformCellLocator::FreeSearchStructure() {}

//------------------------------------------------------------------------------
void vtkLinearTransformCellLocator::BuildLocator()
{
  // don't rebuild if build time is newer than modified and dataset modified time
  if (this->IsLinearTransformation && this->BuildTime > this->MTime &&
    this->BuildTime > this->DataSet->GetMTime())
  {
    return;
  }
  this->BuildLocatorInternal();
}

//------------------------------------------------------------------------------
void vtkLinearTransformCellLocator::ForceBuildLocator()
{
  this->BuildLocatorInternal();
}

//------------------------------------------------------------------------------
void vtkLinearTransformCellLocator::BuildLocatorInternal()
{
  if (!this->CellLocator)
  {
    vtkErrorMacro("Cell Locator not set");
    return;
  }
  this->IsLinearTransformation = this->ComputeTransformation();
  this->BuildTime.Modified();
}

//------------------------------------------------------------------------------
void vtkLinearTransformCellLocator::ShallowCopy(vtkAbstractCellLocator* locator)
{
  auto cellLocator = vtkLinearTransformCellLocator::SafeDownCast(locator);
  if (!cellLocator)
  {
    vtkErrorMacro("Cannot cast " << locator->GetClassName() << " to " << this->GetClassName());
  }
  // we only copy what's actually used by vtkLinearTransformCellLocator
  this->SetCellLocator(cellLocator->GetCellLocator());
  this->Transform = cellLocator->Transform;
  this->InverseTransform = cellLocator->InverseTransform;
  this->IsLinearTransformation = cellLocator->IsLinearTransformation;
  this->UseAllPoints = cellLocator->UseAllPoints;
  this->BuildTime.Modified();
}

//------------------------------------------------------------------------------
int vtkLinearTransformCellLocator::IntersectWithLine(const double p1[3], const double p2[3],
  double tol, double& t, double x[3], double pcoords[3], int& subId, vtkIdType& cellId,
  vtkGenericCell* cell)
{
  if (!this->CellLocator)
  {
    return 0;
  }
  this->BuildLocator();
  double p1Transform[3], p2Transform[3];
  this->InverseTransform->InternalTransformPoint(p1, p1Transform);
  this->InverseTransform->InternalTransformPoint(p2, p2Transform);
  int result = this->CellLocator->IntersectWithLine(
    p1Transform, p2Transform, tol, t, x, pcoords, subId, cellId, cell);
  if (cellId != -1)
  {
    double point[3];
    for (vtkIdType i = 0, max = cell->GetNumberOfPoints(); i < max; ++i)
    {
      auto pointId = cell->PointIds->GetId(i);
      this->DataSet->GetPoint(pointId, point);
      cell->Points->SetPoint(i, point);
    }
    this->Transform->InternalTransformPoint(x, x);
  }
  return result;
}

//------------------------------------------------------------------------------
int vtkLinearTransformCellLocator::IntersectWithLine(const double p1[3], const double p2[3],
  double tol, vtkPoints* points, vtkIdList* cellIds, vtkGenericCell* cell)
{
  if (!this->CellLocator)
  {
    return 0;
  }
  this->BuildLocator();
  double p1Transform[3], p2Transform[3];
  this->InverseTransform->InternalTransformPoint(p1, p1Transform);
  this->InverseTransform->InternalTransformPoint(p2, p2Transform);
  int result =
    this->CellLocator->IntersectWithLine(p1Transform, p2Transform, tol, points, cellIds, cell);
  if (points)
  {
    double point[3];
    for (vtkIdType i = 0, max = points->GetNumberOfPoints(); i < max; ++i)
    {
      points->GetPoint(i, point);
      this->Transform->InternalTransformPoint(point, point);
      points->SetPoint(i, point);
    }
  }
  return result;
}

//------------------------------------------------------------------------------
vtkIdType vtkLinearTransformCellLocator::FindClosestPointWithinRadius(double x[3], double radius,
  double closestPoint[3], vtkGenericCell* cell, vtkIdType& cellId, int& subId, double& dist2,
  int& inside)
{
  if (!this->CellLocator)
  {
    return -1;
  }
  this->BuildLocator();
  double xTransform[3];
  this->InverseTransform->InternalTransformPoint(x, xTransform);
  vtkIdType result = this->CellLocator->FindClosestPointWithinRadius(
    xTransform, radius, closestPoint, cell, cellId, subId, dist2, inside);
  if (result != -1)
  {
    double point[3];
    for (vtkIdType i = 0, max = cell->GetNumberOfPoints(); i < max; ++i)
    {
      auto pointId = cell->PointIds->GetId(i);
      this->DataSet->GetPoint(pointId, point);
      cell->Points->SetPoint(i, point);
    }
    this->Transform->InternalTransformPoint(closestPoint, closestPoint);
  }
  return result;
}

//------------------------------------------------------------------------------
void vtkLinearTransformCellLocator::FindCellsWithinBounds(double*, vtkIdList*)
{
  vtkErrorMacro("FindCellsWithinBounds is not supported");
}

//------------------------------------------------------------------------------
void vtkLinearTransformCellLocator::FindCellsAlongPlane(
  const double o[3], const double n[3], double tolerance, vtkIdList* cells)
{
  if (!this->CellLocator)
  {
    return;
  }
  this->BuildLocator();
  double oTransform[3], nTransform[3];
  this->InverseTransform->InternalTransformPoint(o, oTransform);
  this->InverseTransform->InternalTransformNormal(n, nTransform);
  this->CellLocator->FindCellsAlongPlane(oTransform, nTransform, tolerance, cells);
}

//------------------------------------------------------------------------------
vtkIdType vtkLinearTransformCellLocator::FindCell(
  double x[3], double tol2, vtkGenericCell* cell, int& subId, double pcoords[3], double* weights)
{
  if (!this->CellLocator)
  {
    return -1;
  }
  this->BuildLocator();
  double xTransform[3];
  this->InverseTransform->InternalTransformPoint(x, xTransform);
  vtkIdType cellId = this->CellLocator->FindCell(xTransform, tol2, cell, subId, pcoords, weights);
  if (cellId != -1)
  {
    double point[3];
    for (vtkIdType i = 0, max = cell->GetNumberOfPoints(); i < max; ++i)
    {
      auto pointId = cell->PointIds->GetId(i);
      this->DataSet->GetPoint(pointId, point);
      cell->Points->SetPoint(i, point);
    }
  }
  return cellId;
}

//------------------------------------------------------------------------------
bool vtkLinearTransformCellLocator::InsideCellBounds(double x[3], vtkIdType cellId)
{
  if (!this->CellLocator)
  {
    return false;
  }
  this->BuildLocator();
  double xTransform[3];
  this->InverseTransform->InternalTransformPoint(x, xTransform);
  return this->CellLocator->InsideCellBounds(xTransform, cellId);
}
VTK_ABI_NAMESPACE_END
