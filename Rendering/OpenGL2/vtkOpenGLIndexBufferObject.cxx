// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkObjectFactory.h"

#include "vtkArrayDispatch.h"
#include "vtkBatch.h"
#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkDataArrayRange.h"
#include "vtkPoints.h"
#include "vtkSMPTools.h"
#include "vtkUnsignedCharArray.h"

#include "vtk_glad.h"

#include <set>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenGLIndexBufferObject);

vtkOpenGLIndexBufferObject::vtkOpenGLIndexBufferObject()
{
  this->IndexCount = 0;
  this->SetType(vtkOpenGLIndexBufferObject::ElementArrayBuffer);
}

vtkOpenGLIndexBufferObject::~vtkOpenGLIndexBufferObject() = default;

namespace
{
struct AppendTrianglesBatchData
{
  vtkIdType TrianglesOffset;

  AppendTrianglesBatchData()
    : TrianglesOffset(0)
  {
  }
  ~AppendTrianglesBatchData() = default;
  AppendTrianglesBatchData& operator+=(const AppendTrianglesBatchData& other)
  {
    this->TrianglesOffset += other.TrianglesOffset;
    return *this;
  }
  AppendTrianglesBatchData operator+(const AppendTrianglesBatchData& other) const
  {
    AppendTrianglesBatchData result = *this;
    result += other;
    return result;
  }
};
using AppendTrianglesBatch = vtkBatch<AppendTrianglesBatchData>;
using AppendTrianglesBatches = vtkBatches<AppendTrianglesBatchData>;

// A worker functor. The calculation is implemented in the function template
// for operator().
template <typename TPointsArray, typename TOffsets, typename TConnectivity>
struct AppendTrianglesFunctor
{
  TPointsArray* Points;
  TOffsets* Offsets;
  TConnectivity* Connectivity;
  std::vector<unsigned int>* IndexArray;
  std::vector<unsigned char>* EdgeArray;
  unsigned char* EdgeFlags;
  vtkIdType VOffset;

  AppendTrianglesBatches TriangleBatches;

  AppendTrianglesFunctor(TPointsArray* points, TOffsets* offsets, TConnectivity* connectivity,
    std::vector<unsigned int>* indexArray, std::vector<unsigned char>* edgeArray,
    unsigned char* edgeFlags, vtkIdType vOffset)
    : Points(points)
    , Offsets(offsets)
    , Connectivity(connectivity)
    , IndexArray(indexArray)
    , EdgeArray(edgeArray)
    , EdgeFlags(edgeFlags)
    , VOffset(vOffset)
  {
    // initialize batches
    this->TriangleBatches.Initialize(offsets->GetNumberOfValues() - 1, 1000);
  }

  void Initialize() {}

  void operator()(vtkIdType beginBatchId, vtkIdType endBatchId)
  {
    auto points = vtk::DataArrayTupleRange<3>(this->Points);
    auto offsets = vtk::DataArrayValueRange<1, vtkIdType>(this->Offsets);
    auto connectivity = vtk::DataArrayValueRange<1, vtkIdType>(this->Connectivity);

    for (vtkIdType batchId = beginBatchId; batchId < endBatchId; ++batchId)
    {
      AppendTrianglesBatch& batch = this->TriangleBatches[batchId];
      auto& batchNumberOfTriangles = batch.Data.TrianglesOffset;
      for (vtkIdType cellId = batch.BeginId; cellId < batch.EndId; ++cellId)
      {
        const auto cellSize = offsets[cellId + 1] - offsets[cellId];
        auto cell = connectivity.begin() + offsets[cellId];
        if (cellSize >= 3)
        {
          const auto& id1 = cell[0];
          for (int i = 1; i < cellSize - 1; i++)
          {
            const auto& id2 = cell[i];
            const auto& id3 = cell[i + 1];

            const auto& pt1 = points[id1];
            const auto& pt2 = points[id2];
            const auto& pt3 = points[id3];
            if (pt1 != pt2 && pt1 != pt3 && pt2 != pt3)
            {
              ++batchNumberOfTriangles;
            }
          }
        }
      }
    }
  }

  void Reduce()
  {
    const auto globalSum = this->TriangleBatches.BuildOffsetsAndGetGlobalSum();
    vtkIdType totalNumTriangles = globalSum.TrianglesOffset;
    const auto indexArraySize = this->IndexArray->size();
    const auto edgeArraySize = this->EdgeArray ? this->EdgeArray->size() : 0;
    this->IndexArray->resize(this->IndexArray->size() + totalNumTriangles * 3);
    if (this->EdgeArray)
    {
      this->EdgeArray->resize(this->EdgeArray->size() + totalNumTriangles);
    }

    vtkSMPTools::For(0, this->TriangleBatches.GetNumberOfBatches(),
      [&](vtkIdType beginBatchId, vtkIdType endBatchId)
      {
        auto points = vtk::DataArrayTupleRange<3>(this->Points);
        auto offsets = vtk::DataArrayValueRange<1, vtkIdType>(this->Offsets);
        auto connectivity = vtk::DataArrayValueRange<1, vtkIdType>(this->Connectivity);

        for (vtkIdType batchId = beginBatchId; batchId < endBatchId; ++batchId)
        {
          AppendTrianglesBatch& batch = this->TriangleBatches[batchId];
          auto trianglesOffset = batch.Data.TrianglesOffset;
          auto indexArray = this->IndexArray->data() + indexArraySize + trianglesOffset * 3;
          auto edgeArray =
            this->EdgeArray ? this->EdgeArray->data() + edgeArraySize + trianglesOffset : nullptr;
          for (vtkIdType cellId = batch.BeginId; cellId < batch.EndId; ++cellId)
          {
            const auto cellSize = offsets[cellId + 1] - offsets[cellId];
            const auto cell = connectivity.begin() + offsets[cellId];
            if (cellSize >= 3)
            {
              const auto& id1 = cell[0];
              for (int i = 1; i < cellSize - 1; i++)
              {
                const auto& id2 = cell[i];
                const auto& id3 = cell[i + 1];

                const auto& pt1 = points[id1];
                const auto& pt2 = points[id2];
                const auto& pt3 = points[id3];
                if (pt1 != pt2 && pt1 != pt3 && pt2 != pt3)
                {
                  *indexArray++ = static_cast<unsigned int>(id1 + this->VOffset);
                  *indexArray++ = static_cast<unsigned int>(id2 + this->VOffset);
                  *indexArray++ = static_cast<unsigned int>(id3 + this->VOffset);
                  if (edgeArray)
                  {
                    // NOLINTNEXTLINE(readability-avoid-nested-conditional-operator)
                    int val = cellSize == 3 ? 7 : i == 1 ? 3 : i == cellSize - 2 ? 6 : 2;
                    if (this->EdgeFlags)
                    {
                      int mask = 0;
                      mask =
                        this->EdgeFlags[id1] + this->EdgeFlags[id2] * 2 + this->EdgeFlags[id3] * 4;
                      *edgeArray++ = val & mask;
                    }
                    else
                    {
                      *edgeArray++ = val;
                    }
                  }
                }
              }
            }
          }
        }
      });
  }
};

struct AppendTrianglesWorker
{

  template <typename TPointsArray, typename TOffsets, typename TConnectivity>
  void operator()(TPointsArray* points, TOffsets* offsets, TConnectivity* connectivity,
    std::vector<unsigned int>* indexArray, std::vector<unsigned char>* edgeArray,
    unsigned char* edgeFlags, vtkIdType vOffset)
  {
    AppendTrianglesFunctor<TPointsArray, TOffsets, TConnectivity> functor(
      points, offsets, connectivity, indexArray, edgeArray, edgeFlags, vOffset);
    vtkSMPTools::For(0, functor.TriangleBatches.GetNumberOfBatches(), functor);
  }
};
} // end anon namespace

// used to create an IBO for triangle primitives
void vtkOpenGLIndexBufferObject::AppendTriangleIndexBuffer(std::vector<unsigned int>& indexArray,
  vtkCellArray* cells, vtkPoints* points, vtkIdType vOffset, std::vector<unsigned char>* edgeArray,
  vtkDataArray* edgeFlags)
{
  unsigned char* ucef = nullptr;
  if (edgeFlags)
  {
    ucef = vtkArrayDownCast<vtkUnsignedCharArray>(edgeFlags)->GetPointer(0);
  }

  // Define our dispatcher
  using FloatArrays = vtkArrayDispatch::FilterArraysByValueType<vtkArrayDispatch::AllArrays,
    vtkArrayDispatch::Reals>::Result;
  using Dispatcher = vtkArrayDispatch::Dispatch3ByArray<FloatArrays, vtkCellArray::StorageArrayList,
    vtkCellArray::StorageArrayList>;
  AppendTrianglesWorker worker;
  // Execute the dispatcher:
  if (!Dispatcher::Execute(points->GetData(), cells->GetOffsetsArray(),
        cells->GetConnectivityArray(), worker, &indexArray, edgeArray, ucef, vOffset))
  {
    // Fallback to the generic implementation.
    worker(points->GetData(), cells->GetOffsetsArray(), cells->GetConnectivityArray(), &indexArray,
      edgeArray, ucef, vOffset);
  }
}

// used to create an IBO for triangle primitives
size_t vtkOpenGLIndexBufferObject::CreateTriangleIndexBuffer(vtkCellArray* cells, vtkPoints* points,
  std::vector<unsigned char>* edgeValues, vtkDataArray* edgeFlags)
{
  if (!cells->GetNumberOfCells())
  {
    this->IndexCount = 0;
    return 0;
  }

  const bool hasOnlyTriangles =
    cells->GetNumberOfConnectivityIds() == cells->GetNumberOfCells() * 3;
  if (cells->IsStorage32Bit() && hasOnlyTriangles)
  {
    // If connectivity ids are 32-bits and we only have triangles, upload them as-is.
    vtkCellArray::ArrayType32* array = cells->GetConnectivityArray32();
    this->Upload(array->GetPointer(0), array->GetNumberOfValues(),
      vtkOpenGLIndexBufferObject::ElementArrayBuffer);
    this->IndexCount = array->GetNumberOfValues();
  }
  else
  {
    std::vector<unsigned int> indexArray;
    AppendTriangleIndexBuffer(indexArray, cells, points, 0, edgeValues, edgeFlags);
    this->Upload(indexArray, vtkOpenGLIndexBufferObject::ElementArrayBuffer);
    this->IndexCount = indexArray.size();
  }

  return this->IndexCount;
}

// used to create an IBO for point primitives
void vtkOpenGLIndexBufferObject::AppendPointIndexBuffer(
  std::vector<unsigned int>& indexArray, vtkCellArray* cells, vtkIdType vOffset)
{
  const vtkIdType* indices(nullptr);
  vtkIdType npts(0);
  size_t targetSize = indexArray.size() + cells->GetNumberOfConnectivityIds();
  if (targetSize > indexArray.capacity())
  {
    targetSize = std::max<double>(targetSize, indexArray.capacity() * 1.5);
    indexArray.reserve(targetSize);
  }

  for (cells->InitTraversal(); cells->GetNextCell(npts, indices);)
  {
    for (int i = 0; i < npts; ++i)
    {
      indexArray.push_back(static_cast<unsigned int>(*(indices++) + vOffset));
    }
  }
}

// used to create an IBO for triangle primitives
size_t vtkOpenGLIndexBufferObject::CreatePointIndexBuffer(vtkCellArray* cells)
{
  if (!cells->GetNumberOfCells())
  {
    this->IndexCount = 0;
    return 0;
  }
  std::vector<unsigned int> indexArray;
  AppendPointIndexBuffer(indexArray, cells, 0);
  this->Upload(indexArray, vtkOpenGLIndexBufferObject::ElementArrayBuffer);
  this->IndexCount = indexArray.size();
  return indexArray.size();
}

// used to create an IBO for primitives as lines.  This method treats each line segment
// as independent.  So for a triangle mesh you would get 6 verts per triangle
// 3 edges * 2 verts each.  With a line loop you only get 3 verts so half the storage.
// but... line loops are slower than line segments.
void vtkOpenGLIndexBufferObject::AppendTriangleLineIndexBuffer(
  std::vector<unsigned int>& indexArray, vtkCellArray* cells, vtkIdType vOffset)
{
  const vtkIdType* indices(nullptr);
  vtkIdType npts(0);
  size_t targetSize = indexArray.size() + 2 * cells->GetNumberOfConnectivityIds();
  if (targetSize > indexArray.capacity())
  {
    targetSize = std::max<double>(targetSize, indexArray.capacity() * 1.5);
    indexArray.reserve(targetSize);
  }

  for (cells->InitTraversal(); cells->GetNextCell(npts, indices);)
  {
    for (int i = 0; i < npts; ++i)
    {
      indexArray.push_back(static_cast<unsigned int>(indices[i] + vOffset));
      indexArray.push_back(static_cast<unsigned int>(indices[i < npts - 1 ? i + 1 : 0] + vOffset));
    }
  }
}

// used to create an IBO for primitives as lines.  This method treats each line segment
// as independent.  So for a triangle mesh you would get 6 verts per triangle
// 3 edges * 2 verts each.  With a line loop you only get 3 verts so half the storage.
// but... line loops are slower than line segments.
size_t vtkOpenGLIndexBufferObject::CreateTriangleLineIndexBuffer(vtkCellArray* cells)
{
  if (!cells->GetNumberOfCells())
  {
    this->IndexCount = 0;
    return 0;
  }
  std::vector<unsigned int> indexArray;
  AppendTriangleLineIndexBuffer(indexArray, cells, 0);
  this->Upload(indexArray, vtkOpenGLIndexBufferObject::ElementArrayBuffer);
  this->IndexCount = indexArray.size();
  return indexArray.size();
}

// used to create an IBO for primitives as lines.  This method treats each
// line segment as independent.  So for a line strip you would get multiple
// line segments out
void vtkOpenGLIndexBufferObject::AppendLineIndexBuffer(
  std::vector<unsigned int>& indexArray, vtkCellArray* cells, vtkIdType vOffset)
{
  const vtkIdType* indices(nullptr);
  vtkIdType npts(0);

  // possibly adjust size
  if (cells->GetNumberOfConnectivityIds() > 2 * cells->GetNumberOfCells())
  {
    size_t targetSize =
      indexArray.size() + 2 * (cells->GetNumberOfConnectivityIds() - cells->GetNumberOfCells());
    if (targetSize > indexArray.capacity())
    {
      targetSize = std::max<double>(targetSize, indexArray.capacity() * 1.5);
      indexArray.reserve(targetSize);
    }
  }
  for (cells->InitTraversal(); cells->GetNextCell(npts, indices);)
  {
    for (int i = 0; i < npts - 1; ++i)
    {
      indexArray.push_back(static_cast<unsigned int>(indices[i] + vOffset));
      indexArray.push_back(static_cast<unsigned int>(indices[i + 1] + vOffset));
    }
  }
}

// used to create an IBO for primitives as lines.  This method treats each
// line segment as independent.  So for a line strip you would get multiple
// line segments out
size_t vtkOpenGLIndexBufferObject::CreateLineIndexBuffer(vtkCellArray* cells)
{
  if (!cells->GetNumberOfCells())
  {
    this->IndexCount = 0;
    return 0;
  }
  std::vector<unsigned int> indexArray;
  AppendLineIndexBuffer(indexArray, cells, 0);
  this->Upload(indexArray, vtkOpenGLIndexBufferObject::ElementArrayBuffer);
  this->IndexCount = indexArray.size();
  return indexArray.size();
}

// used to create an IBO for triangle strips
size_t vtkOpenGLIndexBufferObject::CreateStripIndexBuffer(
  vtkCellArray* cells, bool wireframeTriStrips)
{
  if (!cells->GetNumberOfCells())
  {
    this->IndexCount = 0;
    return 0;
  }
  std::vector<unsigned int> indexArray;
  AppendStripIndexBuffer(indexArray, cells, 0, wireframeTriStrips);
  this->Upload(indexArray, vtkOpenGLIndexBufferObject::ElementArrayBuffer);
  this->IndexCount = indexArray.size();
  return indexArray.size();
}

void vtkOpenGLIndexBufferObject::AppendStripIndexBuffer(std::vector<unsigned int>& indexArray,
  vtkCellArray* cells, vtkIdType vOffset, bool wireframeTriStrips)
{
  const vtkIdType* pts = nullptr;
  vtkIdType npts = 0;

  size_t triCount = cells->GetNumberOfConnectivityIds() - 2 * cells->GetNumberOfCells();
  size_t targetSize = wireframeTriStrips ? 2 * (triCount * 2 + 1) : triCount * 3;
  indexArray.reserve(targetSize);

  if (wireframeTriStrips)
  {
    for (cells->InitTraversal(); cells->GetNextCell(npts, pts);)
    {
      indexArray.push_back(static_cast<unsigned int>(pts[0] + vOffset));
      indexArray.push_back(static_cast<unsigned int>(pts[1] + vOffset));
      for (int j = 0; j < npts - 2; ++j)
      {
        indexArray.push_back(static_cast<unsigned int>(pts[j] + vOffset));
        indexArray.push_back(static_cast<unsigned int>(pts[j + 2] + vOffset));
        indexArray.push_back(static_cast<unsigned int>(pts[j + 1] + vOffset));
        indexArray.push_back(static_cast<unsigned int>(pts[j + 2] + vOffset));
      }
    }
  }
  else
  {
    for (cells->InitTraversal(); cells->GetNextCell(npts, pts);)
    {
      for (int j = 0; j < npts - 2; ++j)
      {
        indexArray.push_back(static_cast<unsigned int>(pts[j] + vOffset));
        indexArray.push_back(static_cast<unsigned int>(pts[j + 1 + j % 2] + vOffset));
        indexArray.push_back(static_cast<unsigned int>(pts[j + 1 + (j + 1) % 2] + vOffset));
      }
    }
  }
}

// used to create an IBO for polys in wireframe with edge flags
void vtkOpenGLIndexBufferObject::AppendEdgeFlagIndexBuffer(
  std::vector<unsigned int>& indexArray, vtkCellArray* cells, vtkIdType vOffset, vtkDataArray* ef)
{
  const vtkIdType* pts(nullptr);
  vtkIdType npts(0);

  unsigned char* ucef = vtkArrayDownCast<vtkUnsignedCharArray>(ef)->GetPointer(0);

  // possibly adjust size
  if (cells->GetNumberOfConnectivityIds() > 2 * cells->GetNumberOfCells())
  {
    size_t targetSize =
      indexArray.size() + 2 * (cells->GetNumberOfConnectivityIds() - cells->GetNumberOfCells());
    if (targetSize > indexArray.capacity())
    {
      targetSize = std::max<double>(targetSize, indexArray.capacity() * 1.5);
      indexArray.reserve(targetSize);
    }
  }
  for (cells->InitTraversal(); cells->GetNextCell(npts, pts);)
  {
    for (int j = 0; j < npts; ++j)
    {
      if (ucef[pts[j]] && npts > 1) // draw this edge and poly is not degenerate
      {
        // determine the ending vertex
        vtkIdType nextVert = (j == npts - 1) ? pts[0] : pts[j + 1];
        indexArray.push_back(static_cast<unsigned int>(pts[j] + vOffset));
        indexArray.push_back(static_cast<unsigned int>(nextVert + vOffset));
      }
    }
  }
}

// used to create an IBO for polys in wireframe with edge flags
size_t vtkOpenGLIndexBufferObject::CreateEdgeFlagIndexBuffer(vtkCellArray* cells, vtkDataArray* ef)
{
  if (!cells->GetNumberOfCells())
  {
    this->IndexCount = 0;
    return 0;
  }
  std::vector<unsigned int> indexArray;
  AppendEdgeFlagIndexBuffer(indexArray, cells, 0, ef);
  this->Upload(indexArray, vtkOpenGLIndexBufferObject::ElementArrayBuffer);
  this->IndexCount = indexArray.size();
  return indexArray.size();
}

// used to create an IBO for point primitives
void vtkOpenGLIndexBufferObject::AppendVertexIndexBuffer(
  std::vector<unsigned int>& indexArray, vtkCellArray** cells, vtkIdType vOffset)
{
  const vtkIdType* indices(nullptr);
  vtkIdType npts(0);

  // we use a set to make them unique
  std::set<vtkIdType> vertsUsed;
  for (int j = 0; j < 4; j++)
  {
    for (cells[j]->InitTraversal(); cells[j]->GetNextCell(npts, indices);)
    {
      for (int i = 0; i < npts; ++i)
      {
        vertsUsed.insert(static_cast<unsigned int>(*(indices++) + vOffset));
      }
    }
  }

  // now put them into the vector
  size_t targetSize = indexArray.size() + vertsUsed.size();
  if (targetSize > indexArray.capacity())
  {
    targetSize = std::max<double>(targetSize, indexArray.capacity() * 1.5);
    indexArray.reserve(targetSize);
  }

  for (std::set<vtkIdType>::const_iterator i = vertsUsed.begin(); i != vertsUsed.end(); ++i)
  {
    indexArray.push_back(*i);
  }
}

// used to create an IBO for triangle primitives
size_t vtkOpenGLIndexBufferObject::CreateVertexIndexBuffer(vtkCellArray** cells)
{
  unsigned long totalCells = 0;
  for (int i = 0; i < 4; i++)
  {
    totalCells += cells[i]->GetNumberOfCells();
  }

  if (!totalCells)
  {
    this->IndexCount = 0;
    return 0;
  }
  std::vector<unsigned int> indexArray;
  AppendVertexIndexBuffer(indexArray, cells, 0);
  this->Upload(indexArray, vtkOpenGLIndexBufferObject::ElementArrayBuffer);
  this->IndexCount = indexArray.size();
  return indexArray.size();
}

//------------------------------------------------------------------------------
void vtkOpenGLIndexBufferObject::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
