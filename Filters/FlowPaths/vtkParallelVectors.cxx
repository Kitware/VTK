/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParallelVectors.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkParallelVectors.h"

#include "vtkArrayDispatch.h"
#include "vtkCell3D.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyLine.h"
#include "vtkPolygon.h"
#include "vtkSMPTools.h"
#include "vtkTriangle.h"

#include "vtk_eigen.h"
#include VTK_EIGEN(Eigenvalues)
#include VTK_EIGEN(Geometry)

#include <array>
#include <deque>

//------------------------------------------------------------------------------
namespace
{
// Given a triangle with two vector fields (v0, v1, v2) and (w0, w1, w2) defined
// at its points, determine if the two vector fields are parallel at any point on
// the triangle's surface. Return true if they are, and additionally return the
// parametrized coordinates for the point at which the two vector fields are
// parallel (st). This method assumes that the vector fields are linearly
// interpolated across the triangle face.
//
// This method is adapted from Peikert, Ronald, and Martin Roth. "The "parallel
// vectors" operator - A vector field visualization primitive." Proceedings
// Visualization'99 (Cat. No. 99CB37067). IEEE, 1999.
bool fieldAlignmentPointForTriangle(const double v0[3], const double v1[3], const double v2[3],
  const double w0[3], const double w1[3], const double w2[3], double* st)
{
  // If the first field is zero across the entire face, the notion of parallel
  // vector fields is not applicable.
  if (std::abs(v0[0]) < VTK_DBL_EPSILON && std::abs(v0[1]) < VTK_DBL_EPSILON &&
    std::abs(v0[2]) < VTK_DBL_EPSILON && std::abs(v1[0]) < VTK_DBL_EPSILON &&
    std::abs(v1[1]) < VTK_DBL_EPSILON && std::abs(v1[2]) < VTK_DBL_EPSILON &&
    std::abs(v2[0]) < VTK_DBL_EPSILON && std::abs(v2[1]) < VTK_DBL_EPSILON &&
    std::abs(v2[2]) < VTK_DBL_EPSILON)
  {
    return false;
  }

  // If the second field is zero across the entire face, the notion of parallel
  // vector fields is not applicable.
  if (std::abs(w0[0]) < VTK_DBL_EPSILON && std::abs(w0[1]) < VTK_DBL_EPSILON &&
    std::abs(w0[2]) < VTK_DBL_EPSILON && std::abs(w1[0]) < VTK_DBL_EPSILON &&
    std::abs(w1[1]) < VTK_DBL_EPSILON && std::abs(w1[2]) < VTK_DBL_EPSILON &&
    std::abs(w2[0]) < VTK_DBL_EPSILON && std::abs(w2[1]) < VTK_DBL_EPSILON &&
    std::abs(w2[2]) < VTK_DBL_EPSILON)
  {
    return false;
  }

  // A parametrized description of vector field v on the surface of a triangle
  // can be expressed as
  //
  // \vec{v} = \mathbf{V} \left[ s \; t \; 1 \right]^{^\intercal}
  //
  // where $\mathbf{V}$ is a matrix composed of the vector field values at the
  // triangle's vertices and  $s, t \in [0,1]$ are parametrized scalars
  // describing the point's relative position between $(p0, p1)$ and $(p0, p2)$,
  // respectively.
  Eigen::Matrix<double, 3, 3> V, W;
  for (int i = 0; i < 3; ++i)
  {
    V(i, 0) = v1[i] - v0[i];
    V(i, 1) = v2[i] - v0[i];
    V(i, 2) = v0[i];

    W(i, 0) = w1[i] - w0[i];
    W(i, 1) = w2[i] - w0[i];
    W(i, 2) = w0[i];
  }

  // The two vector fields are parallel when
  //
  // \mathbf(V) \left[ s \; t \; 1 \right]^{^\intercal} =
  // \lambda \mathbf(W) \left[ s \; t \; 1 \right]^{^\intercal}
  //
  // whose solution can be found by computing the eigenvectors of
  //
  // \mathbf{M} = \mathbf{W}^{-1} \mathbf{V}
  //
  // or, by symmetry arguments, $\mathbf{M} = \mathbf{V}^{-1} \mathbf{W}$.
  Eigen::Matrix<double, 3, 3> M;
  if (std::abs(V.determinant()) > VTK_DBL_EPSILON)
  {
    M = V.inverse() * W;
  }
  else if (std::abs(W.determinant()) > VTK_DBL_EPSILON)
  {
    M = W.inverse() * V;
  }
  else
  {
    return false;
  }

  Eigen::EigenSolver<Eigen::Matrix<double, 3, 3>> eigensolver(M);
  Eigen::Matrix<std::complex<double>, 3, 3> eigenvectors = eigensolver.eigenvectors();

  for (int i = 0; i < 3; ++i)
  {
    const auto col = eigenvectors.col(i);

    // We are only interested in real solutions to the above equation.
    if (std::abs(col[0].imag()) > VTK_DBL_EPSILON || std::abs(col[1].imag()) > VTK_DBL_EPSILON ||
      std::abs(col[2].imag()) > VTK_DBL_EPSILON)
    {
      continue;
    }

    // Additionally, we require that our degenerate degree of freedom be nonzero
    // so we can rescale the eigenvectors to set it to unity.
    if (std::abs(col[2].real()) < VTK_DBL_EPSILON)
    {
      continue;
    }

    std::array<double, 3> eigenvector = { { col[0].real(), col[1].real(), col[2].real() } };

    for (double& component : eigenvector)
    {
      component /= eigenvector[2];
    }

    // Finally, we require that the computed point lie on the surface of the
    // triangle.
    if (eigenvector[0] < -VTK_DBL_EPSILON || eigenvector[1] < -VTK_DBL_EPSILON ||
      eigenvector[0] + eigenvector[1] > 1. + VTK_DBL_EPSILON)
    {
      continue;
    }

    for (int j = 0; j < 2; j++)
    {
      st[j] = eigenvector[j];
    }
    return true;
  }

  return false;
}

// A Link is simply a pair of vertex ids.
struct Link : public std::pair<vtkIdType, vtkIdType>
{
  Link(const vtkIdType& handle0, const vtkIdType& handle1)
    : std::pair<vtkIdType, vtkIdType>(handle0, handle1)
  {
  }
};

// A Chain is a list of Links, allowing for O[1] prepending, appending and
// joining.
using Chain = std::deque<Link>;

// An PolyLineBuilder is a list of Chains that supports the addition of links and
// the merging of chains.
struct PolyLineBuilder
{
  PolyLineBuilder()
    : MergeLimit(std::numeric_limits<std::size_t>::max())
  {
  }

  // When a link is inserted, we check to see if it can be prepended or
  // appended to any extant chains. If it can, we add it to the appropriate
  // chain in the correct orientation. Otherwise, it seeds a new Chain. If
  // the number of Chains exceeds the user-defined Merge Limit, the Chains
  // are merged.
  void insert_link(Link&& l)
  {
    if (this->Chains.size() >= this->MergeLimit)
    {
      this->merge_chains();
      this->MergeLimit *= 2;
    }

    // link (a,b)
    for (auto& c : this->Chains)
    {
      if (l.second == c.front().first)
      {
        // (a,b) -> (b,...)
        if (l.first != c.front().second)
        {
          c.emplace_front(l);
        }
        return;
      }
      else if (l.second == c.back().second)
      {
        // (...,b)<- ~(a,b)
        if (l.first != c.back().first)
        {
          c.emplace_back(Link(l.second, l.first));
        }
        return;
      }
      else if (l.first == c.back().second)
      {
        // (...,a) <- (a,b)
        if (l.second != c.back().first)
        {
          c.emplace_back(l);
        }
        return;
      }
      else if (l.first == c.front().first)
      {
        // ~(a,b) -> (a,...)
        if (l.second != c.front().second)
        {
          c.emplace_front(Link(l.second, l.first));
        }
        return;
      }
    }
    Chain c(1, l);
    this->Chains.push_back(c);
  }

  // merge_chains consists of two loops over our Chains. For each Chain c1,
  // we cycle through the subsequent Chains in the list to see if they can be
  // appended or prepended to c1. Once all possible connections have been made
  // to c1, we move to the next chain. If all Links are present, the outer
  // loop will execute exactly one iteration. Otherwise, Chain fragments are
  // merged, ensuring the fewest possible number of Chains remain.
  void merge_chains()
  {
    for (auto c1 = this->Chains.begin(); c1 != Chains.end();)
    {
      if (c1->empty())
      {
        ++c1;
        continue;
      }

      const std::size_t c1_size = c1->size();
      auto c2 = c1;
      for (++c2; c2 != Chains.end(); ++c2)
      {
        if (c2->empty())
        {
          continue;
        }

        // chain c1 looks like (a,...,b)
        if (c1->front().first == c2->back().second)
        {
          // (...,a) -> (a,...,b)
          c1->insert(
            c1->begin(), std::make_move_iterator(c2->begin()), std::make_move_iterator(c2->end()));
          c2->clear();
        }
        else if (c2->front().first == c1->back().second)
        {
          // (a,...,b) <- (b,...)
          c1->insert(
            c1->end(), std::make_move_iterator(c2->begin()), std::make_move_iterator(c2->end()));
          c2->clear();
        }
        else if (c1->front().first == c2->front().first)
        {
          // (a,...,b) <- (a,...)
          for (auto linkIt = c2->begin(); linkIt != c2->end(); ++linkIt)
          {
            c1->emplace_front(Link(linkIt->second, linkIt->first));
          }
          c2->clear();
        }
        else if (c1->back().second == c2->back().second)
        {
          // (...,a) <- (...,a)
          for (auto linkIt = c2->rbegin(); linkIt != c2->rend(); ++linkIt)
          {
            c1->emplace_back(Link(linkIt->second, linkIt->first));
          }
          c2->clear();
        }
      }
      if (c1->size() == c1_size)
      {
        ++c1;
      }
    }

    // Erase the empty chains.
    for (auto c1 = this->Chains.begin(); c1 != Chains.end();)
    {
      if (c1->empty())
      {
        c1 = this->Chains.erase(c1);
      }
      else
      {
        ++c1;
      }
    }
  }

  std::deque<Chain> Chains;
  std::size_t MergeLimit;
};

bool surfaceTessellationForCell(vtkCell3D* cell, std::vector<std::array<vtkIdType, 3>>& triangles,
  vtkSmartPointer<vtkPolygon>& polygon, vtkSmartPointer<vtkIdList>& outTris)
{
  const vtkIdType* localPointIds;

  // compute the number of triangles in the surface tessellation
  {
    std::size_t nTriangles = 0;
    for (int face = 0; face < cell->GetNumberOfFaces(); ++face)
    {
      nTriangles += cell->GetFacePoints(face, localPointIds) - 2;
    }
    triangles.resize(nTriangles);
  }

  std::size_t t = 0;

  for (int face = 0; face < cell->GetNumberOfFaces(); ++face)
  {
    int nPoints = cell->GetFacePoints(face, localPointIds);

    switch (nPoints)
    {
      case 0:
      case 1:
      case 2:
        return false;
      case 3:
      {
        triangles[t++] = { { cell->GetPointIds()->GetId(localPointIds[0]),
          cell->GetPointIds()->GetId(localPointIds[1]),
          cell->GetPointIds()->GetId(localPointIds[2]) } };
        break;
      }
      case 4:
      {
        std::array<vtkIdType, 4> perimeter = { { cell->GetPointIds()->GetId(localPointIds[0]),
          cell->GetPointIds()->GetId(localPointIds[1]),
          cell->GetPointIds()->GetId(localPointIds[2]),
          cell->GetPointIds()->GetId(localPointIds[3]) } };

        std::rotate(
          perimeter.begin(), std::min_element(perimeter.begin(), perimeter.end()), perimeter.end());

        // This ordering ensures that the same two triangles are recovered if
        // the order of the perimeter points are reversed.
        triangles[t++] = { { perimeter[0], perimeter[1], perimeter[2] } };
        triangles[t++] = { { perimeter[0], perimeter[3], perimeter[2] } };
        break;
      }
      default:
      {
        polygon->GetPoints()->SetNumberOfPoints(nPoints);
        polygon->GetPointIds()->SetNumberOfIds(nPoints);

        for (vtkIdType i = 0; i < nPoints; ++i)
        {
          polygon->GetPoints()->SetPoint(i, cell->GetPoints()->GetPoint(localPointIds[i]));
          polygon->GetPointIds()->SetId(i, i);
        }

        polygon->Triangulate(outTris);

        for (vtkIdType i = 0; i < nPoints - 2; ++i)
        {
          triangles[t++] = { { cell->GetPointIds()->GetId(localPointIds[outTris->GetId(3 * i)]),
            cell->GetPointIds()->GetId(localPointIds[outTris->GetId(3 * i + 1)]),
            cell->GetPointIds()->GetId(localPointIds[outTris->GetId(3 * i + 2)]) } };
        }
      }
    }
  }
  return true;
}
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkParallelVectors);

//------------------------------------------------------------------------------
vtkParallelVectors::vtkParallelVectors()
{
  this->FirstVectorFieldName = nullptr;
  this->SecondVectorFieldName = nullptr;
}

//------------------------------------------------------------------------------
vtkParallelVectors::~vtkParallelVectors()
{
  this->SetFirstVectorFieldName(nullptr);
  this->SetSecondVectorFieldName(nullptr);
}

//------------------------------------------------------------------------------
bool vtkParallelVectors::AcceptSurfaceTriangle(const vtkIdType*)
{
  return true;
}

//------------------------------------------------------------------------------
bool vtkParallelVectors::ComputeAdditionalCriteria(
  const vtkIdType*, double, double, std::vector<double>&)
{
  return true;
}

//------------------------------------------------------------------------------
void vtkParallelVectors::Postfilter(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkPolyData* output = vtkPolyData::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));

  for (size_t i = 0; i < this->CriteriaArrays.size(); ++i)
  {
    output->GetPointData()->AddArray(this->CriteriaArrays[i]);
  }
}

//------------------------------------------------------------------------------
namespace detail
{
/**
 * Struct to store the coordinates and the additional criteria of a surface triangle point
 */
struct SurfaceTrianglePoint
{
  std::array<vtkIdType, 3> TrianglePointIds;
  std::array<double, 3> Coordinates;
  std::array<double, 3> InterpolationWeights;
  std::vector<double> Criteria;

  SurfaceTrianglePoint(const std::array<vtkIdType, 3>& trianglePointIds,
    const std::array<double, 3>& point, const std::array<double, 3>& interpolationWeights,
    const std::vector<double>& criteria)
    : TrianglePointIds(trianglePointIds)
    , Coordinates(point)
    , InterpolationWeights(interpolationWeights)
    , Criteria(criteria)
  {
  }
};

/**
 * Functor to collect the valid surface triangle points of each cell.
 */
template <typename VArrayType, typename WArrayType>
class CollectValidCellSurfacePointsFunctor
{
  const vtk::detail::TupleRange<VArrayType, 3> VRange;
  const vtk::detail::TupleRange<WArrayType, 3> WRange;
  vtkDataSet* Input;

  vtkParallelVectors* ParallelVectors;
  std::vector<std::vector<SurfaceTrianglePoint>>& CellSurfaceTrianglePoints;
  vtkSMPThreadLocal<vtkSmartPointer<vtkGenericCell>> Cell;
  vtkSMPThreadLocal<vtkSmartPointer<vtkPolygon>> Polygon;
  vtkSMPThreadLocal<vtkSmartPointer<vtkIdList>> OutTris;
  vtkSMPThreadLocal<std::vector<double>> CriterionArrayValues;
  vtkSMPThreadLocal<std::array<double, 3>> Weights;

public:
  CollectValidCellSurfacePointsFunctor(VArrayType* vField, WArrayType* wField, vtkDataSet* input,
    vtkParallelVectors* parallelVectors,
    std::vector<std::vector<SurfaceTrianglePoint>>& cellSurfaceTrianglePoints)
    : VRange(vtk::DataArrayTupleRange<3>(vField))
    , WRange(vtk::DataArrayTupleRange<3>(wField))
    , Input(input)
    , ParallelVectors(parallelVectors)
    , CellSurfaceTrianglePoints(cellSurfaceTrianglePoints)
  {
    this->CellSurfaceTrianglePoints.resize(input->GetNumberOfCells());
    // instantiate any data-structure that needs to be cached for parallel execution.
    vtkNew<vtkGenericCell> cell;
    this->Input->GetCell(0, cell);
  }

  void Initialize()
  {
    auto& tlCell = this->Cell.Local();
    tlCell = vtkSmartPointer<vtkGenericCell>::New();
    auto& tlPolygon = this->Polygon.Local();
    tlPolygon = vtkSmartPointer<vtkPolygon>::New();
    auto& tlOutTris = this->OutTris.Local();
    tlOutTris = vtkSmartPointer<vtkIdList>::New();
    auto& tlCriterionArrayValues = this->CriterionArrayValues.Local();
    tlCriterionArrayValues.resize(this->ParallelVectors->CriteriaArrays.size());
    this->Weights.Local();
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    auto& tlCell = this->Cell.Local();
    auto& tlPolygon = this->Polygon.Local();
    auto& tlOutTris = this->OutTris.Local();
    auto& tlCriterionArrayValues = this->CriterionArrayValues.Local();
    auto& tlWeights = this->Weights.Local();
    std::vector<std::array<vtkIdType, 3>> surfaceTriangles;

    for (vtkIdType cellId = begin; cellId < end; ++cellId)
    {
      // We only parse 3D cells
      this->Input->GetCell(cellId, tlCell);
      vtkCell3D* cell = vtkCell3D::SafeDownCast(tlCell->GetRepresentativeCell());
      if (cell == nullptr)
      {
        continue;
      }

      // Compute the surface tessellation for the cell
      if (!surfaceTessellationForCell(cell, surfaceTriangles, tlPolygon, tlOutTris))
      {
        vtkLogF(ERROR, "3D cell surface cannot be acquired");
        continue;
      }

      double v[3][3];
      double w[3][3];
      int counter = 0;

      // For each triangle comprising the cell's surface...
      for (const std::array<vtkIdType, 3>& trianglePointIds : surfaceTriangles)
      {
        if (!this->ParallelVectors->AcceptSurfaceTriangle(trianglePointIds.data()))
        {
          continue;
        }

        // ...access the vector values at the vertices
        int trianglePointId;
        for (int i = 0; i < 3; i++)
        {
          trianglePointId = trianglePointIds[i];
          for (int j = 0; j < 3; ++j)
          {
            v[i][j] = static_cast<double>(this->VRange[trianglePointId][j]);
            w[i][j] = static_cast<double>(this->WRange[trianglePointId][j]);
          }
        }

        // Compute the parametric location on the triangle where the vectors are
        // parallel, and if they are in fact parallel
        double st[2];
        if (!fieldAlignmentPointForTriangle(v[0], v[1], v[2], w[0], w[1], w[2], st))
        {
          continue;
        }

        const double& s = st[0];
        const double& t = st[1];

        if (!this->ParallelVectors->ComputeAdditionalCriteria(
              trianglePointIds.data(), s, t, tlCriterionArrayValues))
        {
          continue;
        }

        double pCoords[3] = { s, t, 0.0 };
        vtkTriangle::InterpolationFunctions(pCoords, tlWeights.data());

        // Convert the parametric location to an absolute location
        double p[3][3];
        for (int i = 0; i < 3; i++)
        {
          this->Input->GetPoint(trianglePointIds[i], p[i]);
        }

        std::array<double, 3> pOut;
        for (int i = 0; i < 3; i++)
        {
          pOut[i] = (1. - s - t) * p[0][i] + s * p[1][i] + t * p[2][i];
        }

        this->CellSurfaceTrianglePoints[cellId].push_back(
          SurfaceTrianglePoint(trianglePointIds, pOut, tlWeights, tlCriterionArrayValues));

        if (counter == 2)
        {
          // If we are here, then we have found at least three faces that
          // contain unique points on which the vector field is parallel.
          // This can happen if the vector fields are constant across all
          // corners of the tetrahedron, but then the concept of computing
          // parallel vector lines becomes moot.
          ++counter;
          break;
        }

        // We have identified either our first or second point.
        ++counter;
      }
    }
  }

  void Reduce() {}
};

struct CollectValidCellSurfacePointsWorker
{
  template <typename VArrayType, typename WArrayType>
  void operator()(VArrayType* vArray, WArrayType* wArray, vtkDataSet* input,
    vtkParallelVectors* parallelVectors,
    std::vector<std::vector<SurfaceTrianglePoint>>& cellSurfaceTrianglePoints)
  {
    CollectValidCellSurfacePointsFunctor<VArrayType, WArrayType> functor(
      vArray, wArray, input, parallelVectors, cellSurfaceTrianglePoints);
    vtkSMPTools::For(0, input->GetNumberOfCells(), functor);
  }
};
}

//------------------------------------------------------------------------------
int vtkParallelVectors::RequestData(
  vtkInformation* info, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  this->Prefilter(info, inputVector, outputVector);

  // Check that the input names for the two vector fields have been set
  {
    bool fail = false;
    if (this->FirstVectorFieldName == nullptr)
    {
      vtkErrorMacro(<< "First vector field has not been set");
      fail = true;
    }

    if (this->SecondVectorFieldName == nullptr)
    {
      vtkErrorMacro(<< "Second vector field has not been set");
      fail = true;
    }

    if (fail)
    {
      return 0;
    }
  }

  // Access the two vector fields
  vtkDataSetAttributes* inDA =
    vtkDataSetAttributes::SafeDownCast(input->GetAttributesAsFieldData(vtkDataObject::POINT));
  vtkDataSetAttributes* outDA =
    vtkDataSetAttributes::SafeDownCast(output->GetAttributesAsFieldData(vtkDataObject::POINT));
  outDA->InterpolateAllocate(inDA);

  vtkDataArray* vField =
    vtkDataArray::SafeDownCast(inDA->GetAbstractArray(this->FirstVectorFieldName));
  vtkDataArray* wField =
    vtkDataArray::SafeDownCast(inDA->GetAbstractArray(this->SecondVectorFieldName));

  // Check that the two fields are, in fact, vector fields
  {
    bool fail = false;
    if (vField == nullptr)
    {
      vtkErrorMacro(<< "Could not access first vector field \"" << this->FirstVectorFieldName
                    << "\"");
      fail = true;
    }
    else if (vField->GetNumberOfComponents() != 3)
    {
      vtkErrorMacro(<< "First field \"" << this->FirstVectorFieldName
                    << "\" is not a vector field");
      fail = true;
    }

    if (wField == nullptr)
    {
      vtkErrorMacro(<< "Could not access second vector field \"" << this->SecondVectorFieldName
                    << "\"");
      fail = true;
    }
    else if (wField->GetNumberOfComponents() != 3)
    {
      vtkErrorMacro(<< "Second field \"" << this->SecondVectorFieldName
                    << "\" is not a vector field");
      fail = true;
    }

    if (fail)
    {
      return 0;
    }
  }

  // Compute polylines that correspond to locations where  two vector point
  // fields are parallel.

  // collection of valid surface triangle points of each cell
  std::vector<std::vector<detail::SurfaceTrianglePoint>> cellSurfaceTrianglePoints;

  detail::CollectValidCellSurfacePointsWorker worker;
  using Dispatcher =
    vtkArrayDispatch::Dispatch2ByValueType<vtkArrayDispatch::Reals, vtkArrayDispatch::Reals>;
  if (!Dispatcher::Execute(vField, wField, worker, input, this, cellSurfaceTrianglePoints))
  {
    worker(vField, wField, input, this, cellSurfaceTrianglePoints);
  }

  // Initialize the output points (collected using a point locator)
  vtkNew<vtkPoints> outputPoints;
  vtkNew<vtkMergePoints> locator;
  {
    double bounds[6];
    input->GetBounds(bounds);
    locator->InitPointInsertion(outputPoints, bounds);
  }

  // Initialize the output lines (collected using a PolyLineBuilder)
  vtkNew<vtkCellArray> outputLines;
  PolyLineBuilder polyLineBuilder;

  // For large lists of cells, have the PolyLineBuilder collapse its chain
  // fragments periodically during insertion.
  if (input->GetNumberOfCells() > 100)
  {
    polyLineBuilder.MergeLimit = static_cast<std::size_t>(std::cbrt(input->GetNumberOfCells()));
  }

  vtkNew<vtkIdList> trianglePointIds;
  trianglePointIds->SetNumberOfIds(3);

  for (auto& points : cellSurfaceTrianglePoints)
  {
    vtkIdType pIndex[2] = { -1, -1 };
    int counter = 0;
    // For each surface triangle point comprising the cell's surface...
    for (auto& point : points)
    {
      if (counter == 2)
      {
        // If we are here, then we have found at least three faces that
        // contain unique points on which the vector field is parallel.
        // This can happen if the vector fields are constant across all
        // corners of the tetrahedron, but then the concept of computing
        // parallel vector lines becomes moot.
        ++counter;
        break;
      }

      vtkIdType pIdx;
      locator->InsertUniquePoint(point.Coordinates.data(), pIdx);

      // interpolate output points based on input points
      for (int i = 0; i < 3; ++i)
      {
        trianglePointIds->SetId(i, point.TrianglePointIds[i]);
      }
      outDA->InterpolatePoint(inDA, pIdx, trianglePointIds, point.InterpolationWeights.data());

      // Add criteria values to their arrays
      for (size_t i = 0; i < this->CriteriaArrays.size(); ++i)
      {
        this->CriteriaArrays[i]->InsertTypedTuple(pIdx, (&(point.Criteria[i])));
      }

      // We have identified either our first or second point. Record it
      // and continue searching.
      pIndex[counter] = pIdx;
      ++counter;
    }

    // If our counter is less than 2, then we likely have found a point that
    // is one of a pair for another tetrahedron in the grid. If our counter is
    // greater than 2, we have reached a degenerate condition and cannot
    // represent the parallel vectors using a line. In either case, we move on
    // to the next tetrahedron.
    if (counter != 2)
    {
      continue;
    }

    // Register our line segment with the polyLineBuilder.
    polyLineBuilder.insert_link(Link(pIndex[0], pIndex[1]));
  }

  // Concatenate the computed chains prior to polyline extraction.
  polyLineBuilder.merge_chains();

  // For each contiguous chain, construct a polyline.
  for (auto& chain : polyLineBuilder.Chains)
  {
    vtkNew<vtkPolyLine> polyLine;
    polyLine->GetPointIds()->SetNumberOfIds(static_cast<vtkIdType>(chain.size() + 1));
    vtkIdType counter = 0;
    for (auto& link : chain)
    {
      polyLine->GetPointIds()->SetId(counter++, link.first);
    }
    polyLine->GetPointIds()->SetId(counter, static_cast<vtkIdType>(chain.back().second));
    outputLines->InsertNextCell(polyLine);
  }

  // Populate our output polydata.
  output->SetPoints(outputPoints);
  output->SetLines(outputLines);
  outDA->Squeeze();

  this->Postfilter(info, inputVector, outputVector);

  return 1;
}

//------------------------------------------------------------------------------
int vtkParallelVectors::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
void vtkParallelVectors::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FirstVectorFieldName:"
     << (this->FirstVectorFieldName ? this->FirstVectorFieldName : "(undefined)") << endl;
  os << indent << "SecondVectorFieldName:"
     << (this->SecondVectorFieldName ? this->SecondVectorFieldName : "(undefined)") << endl;
}
