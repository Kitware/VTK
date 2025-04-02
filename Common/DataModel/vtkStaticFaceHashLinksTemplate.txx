// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkStaticFaceHashLinksTemplate.h"

#include "vtkBatch.h"
#include "vtkGenericCell.h"
#include "vtkHexagonalPrism.h"
#include "vtkHexahedron.h"
#include "vtkIdList.h"
#include "vtkPentagonalPrism.h"
#include "vtkPyramid.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkTetra.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVoxel.h"
#include "vtkWedge.h"

#ifndef vtkStaticFaceHashLinksTemplate_txx
#define vtkStaticFaceHashLinksTemplate_txx

VTK_ABI_NAMESPACE_BEGIN
//-----------------------------------------------------------------------------
template <typename TInputIdType, typename TFaceIdType>
struct vtkStaticFaceHashLinksTemplate<TInputIdType, TFaceIdType>::GeometryInformation
{
  //-----------------------------------------------------------------------------
  struct GeometryBatchData
  {
    // In CountFaces::operator() this is used as an accumulator
    // in CountFaces::Reduce() this is changed to an offset
    // This is done to reduce memory footprint.
    vtkIdType FacesOffset;

    GeometryBatchData()
      : FacesOffset(0)
    {
    }
    ~GeometryBatchData() = default;
    GeometryBatchData& operator+=(const GeometryBatchData& other)
    {
      this->FacesOffset += other.FacesOffset;
      return *this;
    }
    GeometryBatchData operator+(const GeometryBatchData& other) const
    {
      GeometryBatchData result = *this;
      result += other;
      return result;
    }
  };
  using GeometryBatch = vtkBatch<GeometryBatchData>;
  using GeometryBatches = vtkBatches<GeometryBatchData>;

  GeometryBatches Batches;
  vtkIdType TotalNumberOfFaces = 0;
};

/**
 * Functor to compute the number of faces and the memory size for all the faces per batch of cells.
 */
template <typename TInputIdType, typename TFaceIdType>
struct vtkStaticFaceHashLinksTemplate<TInputIdType, TFaceIdType>::CountFaces
{
  vtkUnstructuredGrid* Input;
  GeometryInformation& GeometryInfo;

  vtkSMPThreadLocalObject<vtkGenericCell> TLCell;

  CountFaces(vtkUnstructuredGrid* input, GeometryInformation& geometryInfo)
    : Input(input)
    , GeometryInfo(geometryInfo)
  {
  }

  void Initialize() {}

  void operator()(vtkIdType beginBatchId, vtkIdType endBatchId)
  {
    auto cell = this->TLCell.Local();
    for (vtkIdType batchId = beginBatchId; batchId < endBatchId; ++batchId)
    {
      auto& batch = this->GeometryInfo.Batches[batchId];
      auto& numberOfFaces = batch.Data.FacesOffset;
      int numberOfCellFaces;
      unsigned char cellType;
      for (vtkIdType cellId = batch.BeginId; cellId < batch.EndId; ++cellId)
      {
        numberOfCellFaces = this->Input->GetCellNumberOfFaces(cellId, cellType, cell);
        // we mark cells with no faces as having one face so that we can
        // parse them later.
        numberOfFaces += numberOfCellFaces > 0 ? numberOfCellFaces
          : cellType != VTK_EMPTY_CELL         ? 1
                                               : 0;
      }
    }
  }

  void Reduce()
  {
    const auto globalSum = this->GeometryInfo.Batches.BuildOffsetsAndGetGlobalSum();
    this->GeometryInfo.TotalNumberOfFaces = globalSum.FacesOffset;
  }
};

/**
 * Functor to save the hash value for each face and the cell offsets of the faces ids for each face.
 */
template <typename TInputIdType, typename TFaceIdType>
template <typename TCellOffSetIdType>
struct vtkStaticFaceHashLinksTemplate<TInputIdType, TFaceIdType>::CreateFacesInformation
{
  vtkUnstructuredGrid* Input;
  GeometryInformation& GeometryInfo;
  std::shared_ptr<TCellOffSetIdType> CellOffsets;
  std::shared_ptr<TInputIdType> FaceHashValues;
  const TInputIdType NumberOfPoints;

  vtkSMPThreadLocalObject<vtkGenericCell> TLCell;
  vtkSMPThreadLocalObject<vtkIdList> TLFaceIds;
  vtkSMPThreadLocalObject<vtkIdList> TLFacePointIds;

  CreateFacesInformation(vtkUnstructuredGrid* input, GeometryInformation& geometryInfo,
    std::shared_ptr<TCellOffSetIdType>& cellOffsets, std::shared_ptr<TInputIdType>& faceHashValues)
    : Input(input)
    , GeometryInfo(geometryInfo)
    , CellOffsets(cellOffsets)
    , FaceHashValues(faceHashValues)
    , NumberOfPoints(static_cast<TInputIdType>(input->GetNumberOfPoints()))
  {
  }

  void Initialize() {}

  struct FaceInformationOperator
  {
    template <typename CellStateT>
    void operator()(
      CellStateT& state, CreateFacesInformation* This, vtkIdType beginBatchId, vtkIdType endBatchId)
    {
      using ValueType = typename CellStateT::ValueType;
      const ValueType* connectivityPtr = state.GetConnectivity()->GetPointer(0);
      const ValueType* offsetsPtr = state.GetOffsets()->GetPointer(0);
      const unsigned char* cellTypes = This->Input->GetCellTypesArray()->GetPointer(0);

      auto cell = This->TLCell.Local();
      auto faceIdsList = This->TLFaceIds.Local();
      auto facePointIdsList = This->TLFacePointIds.Local();
      const vtkIdType *faceVerts, *faceIds, *facePointIds;
      vtkIdType faceId, numFaces, numFacePoints;
      static constexpr int MAX_FACE_POINTS = 32;
      vtkIdType ptIds[MAX_FACE_POINTS]; // cell face point ids
      static constexpr int pixelConvert[4] = { 0, 1, 3, 2 };

      auto cellOffsets = This->CellOffsets.get();
      auto faceHashValues = This->FaceHashValues.get();
      for (vtkIdType batchId = beginBatchId; batchId < endBatchId; ++batchId)
      {
        auto& batch = This->GeometryInfo.Batches[batchId];
        auto facesOffset = batch.Data.FacesOffset;
        for (vtkIdType cellId = batch.BeginId; cellId < batch.EndId; ++cellId)
        {
          const unsigned char& cellType = cellTypes[cellId];
          // get cell points by just accessing the connectivity/offsets array
          const ValueType* pts = connectivityPtr + offsetsPtr[cellId];

          // the hash value of a face from a 3d cell is the minimum point id
          // the hash value of a face from a 0-1-2d cell is this->NumberOfPoints
          switch (cellType)
          {
            case VTK_EMPTY_CELL:
              cellOffsets[cellId] = facesOffset;
              break;
            case VTK_VERTEX:
            case VTK_POLY_VERTEX:
            case VTK_LINE:
            case VTK_POLY_LINE:
            case VTK_TRIANGLE:
            case VTK_QUAD:
            case VTK_POLYGON:
            case VTK_TRIANGLE_STRIP:
            case VTK_PIXEL:
              cellOffsets[cellId] = facesOffset;
              faceHashValues[facesOffset++] = This->NumberOfPoints;
              break;
            case VTK_TETRA:
              cellOffsets[cellId] = facesOffset;
              for (faceId = 0; faceId < 4; faceId++)
              {
                faceVerts = vtkTetra::GetFaceArray(faceId);
                ptIds[0] = pts[faceVerts[0]];
                ptIds[1] = pts[faceVerts[1]];
                ptIds[2] = pts[faceVerts[2]];
                faceHashValues[facesOffset++] = *std::min_element(ptIds, ptIds + 3);
              }
              break;

            case VTK_VOXEL:
              cellOffsets[cellId] = facesOffset;
              for (faceId = 0; faceId < 6; faceId++)
              {
                faceVerts = vtkVoxel::GetFaceArray(faceId);
                ptIds[0] = pts[faceVerts[pixelConvert[0]]];
                ptIds[1] = pts[faceVerts[pixelConvert[1]]];
                ptIds[2] = pts[faceVerts[pixelConvert[2]]];
                ptIds[3] = pts[faceVerts[pixelConvert[3]]];
                faceHashValues[facesOffset++] = *std::min_element(ptIds, ptIds + 4);
              }
              break;

            case VTK_HEXAHEDRON:
              cellOffsets[cellId] = facesOffset;
              for (faceId = 0; faceId < 6; faceId++)
              {
                faceVerts = vtkHexahedron::GetFaceArray(faceId);
                ptIds[0] = pts[faceVerts[0]];
                ptIds[1] = pts[faceVerts[1]];
                ptIds[2] = pts[faceVerts[2]];
                ptIds[3] = pts[faceVerts[3]];
                faceHashValues[facesOffset++] = *std::min_element(ptIds, ptIds + 4);
              }
              break;

            case VTK_WEDGE:
              cellOffsets[cellId] = facesOffset;
              for (faceId = 0; faceId < 5; faceId++)
              {
                faceVerts = vtkWedge::GetFaceArray(faceId);
                ptIds[0] = pts[faceVerts[0]];
                ptIds[1] = pts[faceVerts[1]];
                ptIds[2] = pts[faceVerts[2]];
                if (faceVerts[3] < 0)
                {
                  faceHashValues[facesOffset++] = *std::min_element(ptIds, ptIds + 3);
                }
                else
                {
                  ptIds[3] = pts[faceVerts[3]];
                  faceHashValues[facesOffset++] = *std::min_element(ptIds, ptIds + 4);
                }
              }
              break;

            case VTK_PYRAMID:
              cellOffsets[cellId] = facesOffset;
              for (faceId = 0; faceId < 5; faceId++)
              {
                faceVerts = vtkPyramid::GetFaceArray(faceId);
                ptIds[0] = pts[faceVerts[0]];
                ptIds[1] = pts[faceVerts[1]];
                ptIds[2] = pts[faceVerts[2]];
                if (faceVerts[3] < 0)
                {
                  faceHashValues[facesOffset++] = *std::min_element(ptIds, ptIds + 3);
                }
                else
                {
                  ptIds[3] = pts[faceVerts[3]];
                  faceHashValues[facesOffset++] = *std::min_element(ptIds, ptIds + 4);
                }
              }
              break;

            case VTK_HEXAGONAL_PRISM:
              cellOffsets[cellId] = facesOffset;
              for (faceId = 0; faceId < 8; faceId++)
              {
                faceVerts = vtkHexagonalPrism::GetFaceArray(faceId);
                ptIds[0] = pts[faceVerts[0]];
                ptIds[1] = pts[faceVerts[1]];
                ptIds[2] = pts[faceVerts[2]];
                ptIds[3] = pts[faceVerts[3]];
                if (faceVerts[4] < 0)
                {
                  faceHashValues[facesOffset++] = *std::min_element(ptIds, ptIds + 4);
                }
                else
                {
                  ptIds[4] = pts[faceVerts[4]];
                  ptIds[5] = pts[faceVerts[5]];
                  faceHashValues[facesOffset++] = *std::min_element(ptIds, ptIds + 6);
                }
              }
              break;

            case VTK_PENTAGONAL_PRISM:
              cellOffsets[cellId] = facesOffset;
              for (faceId = 0; faceId < 7; faceId++)
              {
                faceVerts = vtkPentagonalPrism::GetFaceArray(faceId);
                ptIds[0] = pts[faceVerts[0]];
                ptIds[1] = pts[faceVerts[1]];
                ptIds[2] = pts[faceVerts[2]];
                ptIds[3] = pts[faceVerts[3]];
                if (faceVerts[4] < 0)
                {
                  faceHashValues[facesOffset++] = *std::min_element(ptIds, ptIds + 4);
                }
                else
                {
                  ptIds[4] = pts[faceVerts[4]];
                  faceHashValues[facesOffset++] = *std::min_element(ptIds, ptIds + 5);
                }
              }
              break;
            case VTK_POLYHEDRON:
              cellOffsets[cellId] = facesOffset;
              This->Input->GetPolyhedronFaceLocations()->GetCellAtId(
                cellId, numFaces, faceIds, faceIdsList);
              for (faceId = 0; faceId < numFaces; faceId++)
              {
                This->Input->GetPolyhedronFaces()->GetCellAtId(
                  faceIds[faceId], numFacePoints, facePointIds, facePointIdsList);
                faceHashValues[facesOffset++] =
                  *std::min_element(facePointIds, facePointIds + numFacePoints);
              }
              break;
            default:
              // Other types of 3D linear cells handled by vtkGeometryFilter. Exactly what
              // is a linear cell is defined by vtkCellTypes::IsLinear().
              This->Input->GetCell(cellId, cell);
              if (cell->GetCellDimension() == 3 && cell->IsLinear())
              {
                cellOffsets[cellId] = facesOffset;
                for (faceId = 0, numFaces = cell->GetNumberOfFaces(); faceId < numFaces; faceId++)
                {
                  vtkCell* faceCell = cell->GetFace(faceId);
                  faceHashValues[facesOffset++] =
                    *std::min_element(faceCell->PointIds->GetPointer(0),
                      faceCell->PointIds->GetPointer(faceCell->PointIds->GetNumberOfIds()));
                }
              }
          }
        }
      }
    }
  };

  void operator()(vtkIdType beginBatchId, vtkIdType endBatchId)
  {
    this->Input->GetCells()->Visit(FaceInformationOperator{}, this, beginBatchId, endBatchId);
  }

  void Reduce()
  {
    // set the last offset
    this->CellOffsets.get()[this->Input->GetNumberOfCells()] =
      static_cast<TCellOffSetIdType>(this->GeometryInfo.TotalNumberOfFaces);
  }
};

/**
 * Functor to count how many times a hash is used, a.k.a. how many faces have the same hash.
 */
template <typename TInputIdType, typename TFaceIdType>
template <typename TCellOffSetIdType>
struct vtkStaticFaceHashLinksTemplate<TInputIdType, TFaceIdType>::CountHashes
{
  std::shared_ptr<TCellOffSetIdType> CellOffsets;
  std::shared_ptr<TInputIdType> FaceHashValues;

  std::atomic<TInputIdType>* Counts;

  CountHashes(std::shared_ptr<TCellOffSetIdType> cellOffsets,
    std::shared_ptr<TInputIdType> faceHashValues, std::atomic<TInputIdType>* counts)
    : CellOffsets(cellOffsets)
    , FaceHashValues(faceHashValues)
    , Counts(counts)
  {
  }

  void operator()(vtkIdType beginCellId, vtkIdType endCellId)
  {
    auto cellOffsets = this->CellOffsets.get();
    auto faceHashValues = this->FaceHashValues.get();
    TCellOffSetIdType faceId;
    for (vtkIdType cellId = beginCellId; cellId < endCellId; ++cellId)
    {
      const auto& beginFaceId = cellOffsets[cellId];
      const auto& endFaceId = cellOffsets[cellId + 1];
      for (faceId = beginFaceId; faceId < endFaceId; ++faceId)
      {
        const auto& hashValue = faceHashValues[faceId];
        // memory_order_relaxed is safe here, since we're not using the atomics for
        // synchronization.
        this->Counts[hashValue].fetch_add(1, std::memory_order_relaxed);
      }
    }
  }
};

/**
 * Functor to compute the prefix sum of the counts, a.k.a. the offsets of the faces.
 */
template <typename TInputIdType, typename TFaceIdType>
struct vtkStaticFaceHashLinksTemplate<TInputIdType, TFaceIdType>::PrefixSum
{
  std::atomic<TInputIdType>* Counts;
  std::shared_ptr<vtkIdType> FaceOffsets;
  const vtkIdType NumberOfFaces;
  const vtkIdType NumberOfHashes;
  const vtkIdType NumberOfThreads;

  std::vector<vtkIdType> ThreadSum;

  PrefixSum(std::atomic<TInputIdType>* counts, std::shared_ptr<vtkIdType> faceOffsets,
    vtkIdType numberOfFaces, vtkIdType numberOfHashes, vtkIdType numberOfThreads)
    : Counts(counts)
    , FaceOffsets(faceOffsets)
    , NumberOfFaces(numberOfFaces)
    , NumberOfHashes(numberOfHashes)
    , NumberOfThreads(numberOfThreads)
  {
    this->ThreadSum.resize(static_cast<size_t>(this->NumberOfThreads));
    // assign first offset
    this->FaceOffsets.get()[0] = 0;
    // assign last offset
    this->FaceOffsets.get()[this->NumberOfHashes] = this->NumberOfFaces;
  }

  void Initialize() {}

  void operator()(vtkIdType beginThread, vtkIdType endThread)
  {
    const vtkIdType lastThreadId = this->NumberOfThreads - 1;
    auto faceOffsets = this->FaceOffsets.get();
    for (vtkIdType threadId = beginThread; threadId < endThread; ++threadId)
    {
      const vtkIdType begin = threadId * this->NumberOfHashes / this->NumberOfThreads;
      const vtkIdType end = lastThreadId != threadId
        ? (threadId + 1) * this->NumberOfHashes / this->NumberOfThreads
        : this->NumberOfHashes;
      vtkIdType sum = 0;
      for (vtkIdType pointId = begin; pointId < end; ++pointId)
      {
        sum += this->Counts[pointId].load(std::memory_order_relaxed);
        faceOffsets[pointId + 1] = sum;
      }
      this->ThreadSum[threadId] = sum;
    }
  }

  void Reduce()
  {
    for (vtkIdType threadId = 1; threadId < this->NumberOfThreads; ++threadId)
    {
      this->ThreadSum[threadId] += this->ThreadSum[threadId - 1];
    }
    vtkSMPTools::For(1, this->NumberOfThreads,
      [&](vtkIdType beginThread, vtkIdType endThread)
      {
        const vtkIdType lastThreadId = this->NumberOfThreads - 1;
        auto faceOffsets = this->FaceOffsets.get();
        for (vtkIdType threadId = beginThread; threadId < endThread; ++threadId)
        {
          const vtkIdType begin = threadId * this->NumberOfHashes / this->NumberOfThreads;
          const vtkIdType end = lastThreadId != threadId
            ? (threadId + 1) * this->NumberOfHashes / this->NumberOfThreads
            : this->NumberOfHashes;
          const auto& threadLocalSum = this->ThreadSum[threadId - 1];
          for (vtkIdType pointId = begin + 1; pointId <= end; ++pointId)
          {
            faceOffsets[pointId] += threadLocalSum;
          }
        }
      });
  }
};

/**
 * Functor to assign the cell id and the face id links of each face that belong in the same hash.
 */
template <typename TInputIdType, typename TFaceIdType>
template <typename TCellOffSetIdType>
struct vtkStaticFaceHashLinksTemplate<TInputIdType, TFaceIdType>::BuildFaceHashLinks
{
  std::shared_ptr<TCellOffSetIdType> CellOffsets;
  std::shared_ptr<TInputIdType> FaceHashValues;

  std::atomic<TInputIdType>* Counts;
  std::shared_ptr<vtkIdType> FaceOffsets;

  std::shared_ptr<TInputIdType> CellIdOfFaceLinks;
  std::shared_ptr<TFaceIdType> FaceIdOfFaceLinks;

  BuildFaceHashLinks(std::shared_ptr<TCellOffSetIdType> cellOffsets,
    std::shared_ptr<TInputIdType> faceHashValues, std::atomic<TInputIdType>* counts,
    std::shared_ptr<vtkIdType> faceOffsets, std::shared_ptr<TInputIdType> cellIdOfFaceLinks,
    std::shared_ptr<TFaceIdType> faceIdOfFaceLinks)
    : CellOffsets(cellOffsets)
    , FaceHashValues(faceHashValues)
    , Counts(counts)
    , FaceOffsets(faceOffsets)
    , CellIdOfFaceLinks(cellIdOfFaceLinks)
    , FaceIdOfFaceLinks(faceIdOfFaceLinks)
  {
  }

  void operator()(vtkIdType beginCellId, vtkIdType endCellId)
  {
    auto cellOffsets = this->CellOffsets.get();
    auto faceHashValues = this->FaceHashValues.get();
    auto faceOffsets = this->FaceOffsets.get();
    auto cellIdOfFaceLinks = this->CellIdOfFaceLinks.get();
    auto faceIdOfFaceLinks = this->FaceIdOfFaceLinks.get();
    // Now build the hash links. The summation from the prefix sum indicates where
    // the cells are to be inserted. Each time a cell is inserted, the offset
    // is decremented. In the end, the offset array is also constructed as it
    // points to the beginning of each cell run.
    vtkIdType offset;
    TFaceIdType localFaceId;
    TCellOffSetIdType faceId;
    for (vtkIdType cellId = beginCellId; cellId < endCellId; ++cellId)
    {
      const auto& beginFaceId = cellOffsets[cellId];
      const auto& endFaceId = cellOffsets[cellId + 1];
      for (faceId = beginFaceId, localFaceId = 0; faceId < endFaceId; ++faceId, ++localFaceId)
      {
        const auto& hashValue = faceHashValues[faceId];
        // memory_order_relaxed is safe here, since we're not using the atomics for
        // synchronization.
        offset = faceOffsets[hashValue] +
          this->Counts[hashValue].fetch_sub(1, std::memory_order_relaxed) - 1;
        cellIdOfFaceLinks[offset] = cellId;
        faceIdOfFaceLinks[offset] = localFaceId;
      }
    }
  }
};

/**
 * Calls CreateFacesInformation, CountHashes, PrefixSum and BuildFaceHashLinks.
 * This is needed since we need to template the cell offsets type.
 */
template <typename TInputIdType, typename TFaceIdType>
template <typename TCellOffSetIdType>
void vtkStaticFaceHashLinksTemplate<TInputIdType, TFaceIdType>::BuildHashLinksInternal(
  vtkUnstructuredGrid* input, GeometryInformation& geometryInfo)
{
  const vtkIdType numberOfCells = input->GetNumberOfCells();
  this->NumberOfHashes = input->GetNumberOfPoints() + 1 /* for the 0D-1D-2D faces */;
  // allocate memory for the cell offsets and face hash values
  std::shared_ptr<TCellOffSetIdType> cellOffsets(
    new TCellOffSetIdType[numberOfCells + 1], std::default_delete<TCellOffSetIdType[]>());
  std::shared_ptr<TInputIdType> faceHashValues(
    new TInputIdType[this->NumberOfFaces], std::default_delete<TInputIdType[]>());

  // create the faces information
  CreateFacesInformation<TCellOffSetIdType> facesInformationCreator(
    input, geometryInfo, cellOffsets, faceHashValues);
  vtkSMPTools::For(0, geometryInfo.Batches.GetNumberOfBatches(), facesInformationCreator);

  // count the number of faces per hash
  std::atomic<TInputIdType>* counts = new std::atomic<TInputIdType>[this->NumberOfHashes]();
  CountHashes<TCellOffSetIdType> countHash(cellOffsets, faceHashValues, counts);
  vtkSMPTools::For(0, numberOfCells, countHash);

  // Perform prefix sum to determine offsets
  this->FaceOffsets = std::shared_ptr<vtkIdType>(
    new vtkIdType[this->NumberOfHashes + 1], std::default_delete<vtkIdType[]>());
  const auto numberOfThreads = static_cast<vtkIdType>(vtkSMPTools::GetEstimatedNumberOfThreads());
  PrefixSum prefixSum(
    counts, this->FaceOffsets, this->NumberOfFaces, this->NumberOfHashes, numberOfThreads);
  vtkSMPTools::For(0, numberOfThreads, prefixSum);

  // Build face hash links
  this->CellIdOfFaceLinks = std::shared_ptr<TInputIdType>(
    new TInputIdType[this->NumberOfFaces], std::default_delete<TInputIdType[]>());
  this->FaceIdOfFaceLinks = std::shared_ptr<TFaceIdType>(
    new TFaceIdType[this->NumberOfFaces], std::default_delete<TFaceIdType[]>());
  BuildFaceHashLinks<TCellOffSetIdType> buildFaceHashLinks(cellOffsets, faceHashValues, counts,
    this->FaceOffsets, this->CellIdOfFaceLinks, this->FaceIdOfFaceLinks);
  vtkSMPTools::For(0, numberOfCells, buildFaceHashLinks);

  // Clean up
  cellOffsets.reset();
  faceHashValues.reset();
  delete[] counts;
}

//----------------------------------------------------------------------------
template <typename TInputIdType, typename TFaceIdType>
void vtkStaticFaceHashLinksTemplate<TInputIdType, TFaceIdType>::BuildHashLinks(
  vtkUnstructuredGrid* input)
{
  const vtkIdType numberOfCells = input->GetNumberOfCells();
  GeometryInformation geometryInfo;
  geometryInfo.Batches.Initialize(numberOfCells);

  // first we count the number of faces
  CountFaces faceCount(input, geometryInfo);
  vtkSMPTools::For(0, geometryInfo.Batches.GetNumberOfBatches(), faceCount);
  this->NumberOfFaces = geometryInfo.TotalNumberOfFaces;

#ifdef VTK_USE_64BIT_IDS
  const bool use64BitIdsOffsets = this->NumberOfFaces > VTK_INT_MAX;
  if (use64BitIdsOffsets)
  {
    using TCellOffSetIdType = vtkTypeInt64;
    this->BuildHashLinksInternal<TCellOffSetIdType>(input, geometryInfo);
  }
  else
#endif
  {
    using TCellOffSetIdType = vtkTypeInt32;
    this->BuildHashLinksInternal<TCellOffSetIdType>(input, geometryInfo);
  }
}

//----------------------------------------------------------------------------
template <typename TInputIdType, typename TFaceIdType>
void vtkStaticFaceHashLinksTemplate<TInputIdType, TFaceIdType>::Reset()
{
  this->NumberOfFaces = 0;
  this->NumberOfHashes = 0;
  this->CellIdOfFaceLinks.reset();
  this->FaceIdOfFaceLinks.reset();
  this->FaceOffsets.reset();
}
VTK_ABI_NAMESPACE_END
#endif // vtkStaticFaceHashLinksTemplate_txx
