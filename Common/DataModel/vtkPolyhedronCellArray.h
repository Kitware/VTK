// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolyhedronCellArray
 * @brief   object to represent the faces of polyhedral cells
 *
 * vtkPolyhedronCellArray stores the topology of polyhedral cells, e.g., the
 * VTK_POLYHEDRON cells of a vtkUnstructuredGrid, as an explicit table listing
 * the faces that make up each cell, and the point ids that make up each face.
 *
 * Internally, the table is represented as two vtkCellArray: Faces and
 * FaceLocations.
 *
 * Faces is a vtkCellArray storing every face as a cell, i.e., the list of its
 * point ids.
 *
 * FaceLocations is a vtkCellArray storing every polyhedral cell as a cell,
 * i.e., the list of the ids of its faces in Faces. A cell that is not
 * polyhedral, e.g., the hexahedron of a vtkUnstructuredGrid that also has
 * polyhedra, has no face.
 *
 * Thus, for a set of cells consisting of a tetrahedron given by its faces, a
 * hexahedron, and a pyramid given by its faces, the internal arrays will appear
 * as follows:
 *
 * ```
 * Topology:
 * ---------
 * Cell 0: Polyhedron | faces: {0, 1, 2}, {0, 3, 1}, {1, 3, 2}, {2, 3, 0}
 * Cell 1: Hexahedron | no face
 * Cell 2: Polyhedron | faces: {4, 7, 6, 5}, {4, 5, 8}, {5, 6, 8}, {6, 7, 8}, {7, 4, 8}
 *
 * vtkPolyhedronCellArray:
 * -----------------------
 * Faces:
 *   Offsets:      {0, 3, 6, 9, 12, 16, 19, 22, 25, 28}
 *   Connectivity: {0, 1, 2, 0, 3, 1, 1, 3, 2, 2, 3, 0,
 *                  4, 7, 6, 5, 4, 5, 8, 5, 6, 8, 6, 7, 8, 7, 4, 8}
 * FaceLocations:
 *   Offsets:      {0, 4, 4, 9}
 *   Connectivity: {0, 1, 2, 3, 4, 5, 6, 7, 8}
 * ```
 *
 * The faces of a cell may also be given by a face stream, as the polyhedra of
 * vtkUnstructuredGrid::InsertNextCell(): the number of points of every face,
 * followed by its point ids, e.g., {3, 0, 1, 2, 3, 0, 3, 1, 3, 1, 3, 2, 3, 2, 3,
 * 0} for the four faces of cell 0 above.
 *
 * The query methods, e.g., GetCellNumberOfFaces(), GetFaceSize() and
 * GetFacePoints(), dispatch the storage type of the arrays for every call, as
 * vtkCellArray::GetCellAtId(). They are thread-safe if the arrays are not
 * modified, and if every thread gives its own vtkIdList to GetFacePoints().
 * Whenever many faces are traversed, e.g., in a threaded loop, it is preferable
 * to dispatch the arrays once with Dispatch(), and to read them directly:
 *
 * ```
 * struct CountFacePoints : public vtkPolyhedronCellArray::DispatchUtilities
 * {
 *   template <class FaceOffsetsT, class FaceConnectivityT, class LocationOffsetsT,
 *     class LocationConnectivityT>
 *   vtkIdType operator()(FaceOffsetsT* faceOffsets, FaceConnectivityT*,
 *     LocationOffsetsT* locationOffsets, LocationConnectivityT* locationConnectivity,
 *     vtkIdType cellId)
 *   {
 *     vtkIdType numPoints = 0;
 *     const vtkIdType numFaces = GetCellSize(locationOffsets, cellId);
 *     const auto faceIds = GetCellRange(locationOffsets, locationConnectivity, cellId);
 *     for (int face = 0; face < numFaces; ++face)
 *     {
 *       numPoints += GetCellSize(faceOffsets, faceIds[face]);
 *     }
 *     return numPoints;
 *   }
 * };
 *
 * vtkIdType numPoints = polyhedra->Dispatch(CountFacePoints{}, cellId);
 * ```
 *
 * The internal arrays of Faces and FaceLocations may store either 32- or 64-bit
 * values, and may be "fixed size", see vtkCellArray, e.g., the faces of
 * hexahedra converted to polyhedra have 4 points, and their cells 6 faces. The
 * storage type of a vtkPolyhedronCellArray is the storage type of Faces and
 * FaceLocations when they have the same one, otherwise Generic. Methods for
 * managing the storage type are:
 *
 * - `StorageTypes GetStorageType()`
 * - `bool IsStorageShareable() // Can pointers to the points of the faces be shared`
 * - `bool ConvertTo64BitStorage()`
 * - `bool ConvertTo32BitStorage()`
 * - `bool ConvertToDefaultStorage() // Depends on vtkIdType`
 * - `bool ConvertToSmallestStorage() // Depends on current values in arrays`
 *
 * @sa vtkCellArray vtkUnstructuredGrid vtkPolyhedron
 */

#ifndef vtkPolyhedronCellArray_h
#define vtkPolyhedronCellArray_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

#include "vtkCellArray.h"     // Needed for inline methods
#include "vtkIdList.h"        // Needed for inline methods
#include "vtkSmartPointer.h"  // For vtkSmartPointer
#include "vtkWrappingHints.h" // For VTK_EXPECTS, VTK_MARSHALMANUAL, VTK_WRAPEXCLUDE

#include <type_traits> // Needed for std::is_base_of_v
#include <utility>     // Needed for std::forward

VTK_ABI_NAMESPACE_BEGIN
class vtkIdTypeArray;

class VTKCOMMONDATAMODEL_EXPORT VTK_MARSHALMANUAL vtkPolyhedronCellArray : public vtkObject
{
public:
  using AOSArray32 = vtkCellArray::AOSArray32;
  using AOSArray64 = vtkCellArray::AOSArray64;
  using AffineArray32 = vtkCellArray::AffineArray32;
  using AffineArray64 = vtkCellArray::AffineArray64;
  using StorageTypes = vtkCellArray::StorageTypes;
  using DispatchUtilities = vtkCellArray::DispatchUtilities;

  ///@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  static vtkPolyhedronCellArray* New();
  vtkTypeMacro(vtkPolyhedronCellArray, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Pre-allocate memory to store numCells cells of at most maxFacesPerCell
   * faces of at most maxFacePoints points, as vtkCellArray::AllocateEstimate():
   * existing data is NOT preserved. Returns true if allocation succeeds.
   */
  bool AllocateEstimate(vtkIdType numCells, vtkIdType maxFacesPerCell, vtkIdType maxFacePoints);

  /**
   * Free any memory and reset to an empty state.
   */
  void Initialize();

  /**
   * Reuse the list without freeing memory.
   */
  void Reset();

  /**
   * Reclaim any extra memory while preserving data.
   */
  void Squeeze();

  /**
   * Return the size, in kibibytes (1024 bytes), of the memory used by this
   * object.
   */
  unsigned long GetActualMemorySize() const;

  /**
   * Perform a deep copy (no reference counting) of the given polyhedron cell
   * array.
   */
  void DeepCopy(vtkPolyhedronCellArray* other);

  /**
   * Shallow copy the given polyhedron cell array: its Faces and FaceLocations
   * are shared.
   */
  void ShallowCopy(vtkPolyhedronCellArray* other);

  /**
   * Set the internal data arrays to the supplied faces and face locations,
   * which are referenced, not copied. Both must be non-null.
   */
  void SetData(vtkCellArray* faces, vtkCellArray* faceLocations);

  ///@{
  /**
   * Return the vtkCellArray of the faces, whose cells are the point ids of the
   * faces, and the vtkCellArray of the face locations, whose cells are the ids
   * of the faces of the polyhedral cells.
   */
  vtkCellArray* GetFaces() const { return this->Faces; }
  vtkCellArray* GetFaceLocations() const { return this->FaceLocations; }
  ///@}

  /**
   * Return the modification time of this object and of its arrays.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Get the number of cells, and the number of faces of all the cells.
   */
  vtkIdType GetNumberOfCells() const { return this->FaceLocations->GetNumberOfCells(); }
  vtkIdType GetNumberOfFaces() const { return this->Faces->GetNumberOfCells(); }
  ///@}

  /**
   * Return the number of faces of the cell with the most faces.
   */
  int GetMaxCellNumberOfFaces() const { return this->FaceLocations->GetMaxCellSize(); }

  /**
   * Return the size of the largest face, i.e., its number of points.
   */
  int GetMaxFaceSize() const { return this->Faces->GetMaxCellSize(); }

  /**
   * Return the storage type of the arrays: the one of Faces and FaceLocations
   * if they have the same, otherwise Generic, see Dispatch().
   */
  StorageTypes GetStorageType() const noexcept
  {
    const StorageTypes storageType = this->Faces->GetStorageType();
    return storageType == this->FaceLocations->GetStorageType() ? storageType
                                                                : StorageTypes::Generic;
  }

  /**
   * Return true if the points of the faces are stored as vtkIdType, so that
   * GetFacePoints() returns a pointer to them instead of copying them.
   */
  bool IsStorageShareable() const { return this->Faces->IsStorageShareable(); }

  ///@{
  /**
   * Convert the storage of Faces and FaceLocations, see vtkCellArray. Returns
   * true if both are converted. ConvertToSmallestStorage() gives both the same
   * storage type: fixed size only if both can be, and 64-bit if either must be.
   */
  bool ConvertTo64BitStorage();
  bool ConvertTo32BitStorage();
  bool ConvertToDefaultStorage();
  bool ConvertToSmallestStorage();
  ///@}

  /**
   * Return the number of faces of the cell cellId.
   */
  int GetCellNumberOfFaces(vtkIdType cellId) const
    VTK_EXPECTS(0 <= cellId && cellId < GetNumberOfCells());

  /**
   * Return the id, in Faces, of the face `face` of the cell cellId.
   */
  vtkIdType GetFaceId(vtkIdType cellId, int face) const
    VTK_EXPECTS(0 <= cellId && cellId < GetNumberOfCells());

  /**
   * Return the number of points of the face `face` of the cell cellId.
   */
  vtkIdType GetFaceSize(vtkIdType cellId, int face) const
    VTK_EXPECTS(0 <= cellId && cellId < GetNumberOfCells());

  /**
   * Return the number of points of all the faces of the cell cellId, as
   * listed in its faces: a point of several faces is counted several times.
   */
  vtkIdType GetCellNumberOfFacePoints(vtkIdType cellId) const
    VTK_EXPECTS(0 <= cellId && cellId < GetNumberOfCells());

  /**
   * Return the npts point ids `pts` of the face `face` of the cell cellId, as
   * vtkCellArray::GetCellAtId(): `pts` points to the internal storage if it is
   * shareable, see IsStorageShareable(), otherwise the point ids are copied
   * into `tempIds`, which must be given. `pts` is only valid until the faces,
   * or `tempIds`, are modified.
   */
  void GetFacePoints(vtkIdType cellId, int face, vtkIdType& npts,
    vtkIdType const*& pts VTK_SIZEHINT(npts), vtkIdList* tempIds) const
    VTK_EXPECTS(0 <= cellId && cellId < GetNumberOfCells());

  /**
   * Copy the point ids of the face `face` of the cell cellId into `pts`.
   */
  void GetFacePoints(vtkIdType cellId, int face, vtkIdList* pts) const
    VTK_EXPECTS(0 <= cellId && cellId < GetNumberOfCells());

  /**
   * Copy the faces of the cell cellId into `faceStream`, as a face stream
   * preceded by the number of faces: (numFaces, numFace0Pts, id1, id2, ...,
   * numFace1Pts, id1, id2, ...). It is empty if the cell has no face.
   */
  void GetFaceStream(vtkIdType cellId, vtkIdList* faceStream) const
    VTK_EXPECTS(0 <= cellId && cellId < GetNumberOfCells());

  /**
   * Copy the faces of the cell cellId into `faces`, one cell per face.
   */
  void GetCellFaces(vtkIdType cellId, vtkCellArray* faces) const
    VTK_EXPECTS(0 <= cellId && cellId < GetNumberOfCells());

  /**
   * Insert a cell given by its numFaces faces, as a face stream: (numFace0Pts,
   * id1, id2, ..., numFace1Pts, id1, id2, ...). Returns the id of the cell.
   */
  VTK_WRAPEXCLUDE vtkIdType InsertNextCell(vtkIdType numFaces, const vtkIdType* faceStream);

  /**
   * Insert a cell given by its faces, one cell of `faces` per face. Returns the
   * id of the cell.
   */
  vtkIdType InsertNextCell(vtkCellArray* faces);

  /**
   * Insert numCells cells without face, e.g., the cells of a
   * vtkUnstructuredGrid that are not polyhedral. Returns the id of the last
   * one, or -1 if numCells is 0.
   */
  vtkIdType InsertEmptyCells(vtkIdType numCells);

  /**
   * Append the cells of `source` into this, as vtkCellArray::Append(): the
   * point ids of its faces are offset by pointOffset, and the ids of its faces
   * by the number of faces of this.
   */
  void Append(vtkPolyhedronCellArray* source, vtkIdType pointOffset = 0);

  /**
   * Fill @a faces and @a faceLocations with the legacy layout of the faces of
   * the cells, the two vtkIdTypeArray of vtkUnstructuredGrid before VTK 9.4:
   * @a faces concatenates the face streams of the cells with faces, e.g.
   *
   * ```
   * { f0, n0_0, p0_0_0, ..., n0_1, p0_1_0, ..., f2, n2_0, p2_0_0, ... }
   * ```
   *
   * where `f0` is the number of faces of cell 0, `n0_Y` the number of points
   * of its face Y, and `p0_Y_Z` the Z'th point of face Y, and @a faceLocations
   * has one value per cell, the index in @a faces of its face stream, or -1 for
   * a cell without face. For the cells of the class documentation, the face
   * stream of cell 0 has 17 values, and @a faceLocations is { 0, -1, 17 }.
   */
  void ExportLegacyFormat(vtkIdTypeArray* faces, vtkIdTypeArray* faceLocations);

  ///@{
  /**
   * Import arrays of data with the legacy layout of the faces of the cells,
   * see ExportLegacyFormat(): @a faces of @a facesLength values, and
   * @a faceLocations of one value per cell, @a numberOfCells values.
   *
   * Returns false if the data is corrupt (for example a face location or `f0`
   * being too large for the actual buffer), in which case this is left
   * unchanged. Returns true upon successful conversion.
   */
  bool ImportLegacyFormat(vtkIdTypeArray* faces, vtkIdTypeArray* faceLocations);
  bool ImportLegacyFormat(const vtkIdType* faces, vtkIdType facesLength,
    const vtkIdType* faceLocations, vtkIdType numberOfCells) VTK_SIZEHINT(faces, facesLength)
    VTK_SIZEHINT(faceLocations, numberOfCells);
  ///@}

  ///@{
  /**
   * Append arrays of data with the legacy layout of the faces of the cells,
   * see ExportLegacyFormat() and ImportLegacyFormat(). Point ids are offset by
   * @a ptOffset.
   *
   * Returns false if the data is corrupt (for example a face location or `f0`
   * being too large for the actual buffer), in which case this is left
   * unchanged. Returns true upon successful conversion.
   */
  bool AppendLegacyFormat(
    vtkIdTypeArray* faces, vtkIdTypeArray* faceLocations, vtkIdType ptOffset = 0);
  bool AppendLegacyFormat(const vtkIdType* faces, vtkIdType facesLength,
    const vtkIdType* faceLocations, vtkIdType numberOfCells, vtkIdType ptOffset = 0)
    VTK_SIZEHINT(faces, facesLength) VTK_SIZEHINT(faceLocations, numberOfCells);
  ///@}

  /**
   * @warning Advanced use only.
   *
   * The Dispatch method allows efficient traversal of the vtkPolyhedronCellArray
   * internal arrays by dispatching a functor with the actual arrays, as
   * vtkCellArray::Dispatch(): the offsets and the connectivity of Faces, then of
   * FaceLocations, as vtkCellArray::AOSArray32, vtkCellArray::AOSArray64, the
   * vtkCellArray::AffineArray32 and vtkCellArray::AffineArray64 offsets of fixed
   * size storage, or vtkDataArray for Generic storage, see GetStorageType().
   * The functor may also return a value, and receive additional arguments:
   *
   * ```
   * struct Worker : public vtkPolyhedronCellArray::DispatchUtilities
   * {
   *   template <class FaceOffsetsT, class FaceConnectivityT, class LocationOffsetsT,
   *     class LocationConnectivityT>
   *   vtkIdType operator()(FaceOffsetsT* faceOffsets, FaceConnectivityT* faceConnectivity,
   *     LocationOffsetsT* locationOffsets, LocationConnectivityT* locationConnectivity,
   *     vtkIdType cellId)
   *   {
   *     // Do work with the arrays
   *   }
   * };
   *
   * vtkIdType result = polyhedra->Dispatch(Worker{}, cellId);
   * ```
   *
   * The returned type is deduced, so every storage type instantiation of the
   * functor must return the same type.
   *
   * The second overload takes the storage type, which must be the one of
   * GetStorageType(), so that a caller that dispatches many times, e.g., once
   * per cell, gets it only once. Thus, the first additional argument of the
   * first overload can not be a StorageTypes.
   *
   * ```
   * const vtkPolyhedronCellArray::StorageTypes storageType = polyhedra->GetStorageType();
   * for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
   * {
   *   vtkIdType result = polyhedra->Dispatch(Worker{}, storageType, cellId);
   * }
   * ```
   */
  template <typename Functor, typename... Args>
  decltype(auto) Dispatch(Functor&& functor, Args&&... args) const
  {
    return this->Dispatch(
      std::forward<Functor>(functor), this->GetStorageType(), std::forward<Args>(args)...);
  }
  template <typename Functor, typename... Args>
  decltype(auto) Dispatch(Functor&& functor, StorageTypes storageType, Args&&... args) const
  {
    vtkCellArray* faces = this->Faces;
    vtkCellArray* locations = this->FaceLocations;
    switch (storageType)
    {
      case StorageTypes::Int32:
        return functor(static_cast<AOSArray32*>(faces->GetOffsetsArray()),
          static_cast<AOSArray32*>(faces->GetConnectivityArray()),
          static_cast<AOSArray32*>(locations->GetOffsetsArray()),
          static_cast<AOSArray32*>(locations->GetConnectivityArray()), std::forward<Args>(args)...);
      case StorageTypes::Int64:
        return functor(static_cast<AOSArray64*>(faces->GetOffsetsArray()),
          static_cast<AOSArray64*>(faces->GetConnectivityArray()),
          static_cast<AOSArray64*>(locations->GetOffsetsArray()),
          static_cast<AOSArray64*>(locations->GetConnectivityArray()), std::forward<Args>(args)...);
      case StorageTypes::FixedSizeInt32:
        return functor(static_cast<AffineArray32*>(faces->GetOffsetsArray()),
          static_cast<AOSArray32*>(faces->GetConnectivityArray()),
          static_cast<AffineArray32*>(locations->GetOffsetsArray()),
          static_cast<AOSArray32*>(locations->GetConnectivityArray()), std::forward<Args>(args)...);
      case StorageTypes::FixedSizeInt64:
        return functor(static_cast<AffineArray64*>(faces->GetOffsetsArray()),
          static_cast<AOSArray64*>(faces->GetConnectivityArray()),
          static_cast<AffineArray64*>(locations->GetOffsetsArray()),
          static_cast<AOSArray64*>(locations->GetConnectivityArray()), std::forward<Args>(args)...);
      case StorageTypes::Generic:
      default:
        return functor(faces->GetOffsetsArray(), faces->GetConnectivityArray(),
          locations->GetOffsetsArray(), locations->GetConnectivityArray(),
          std::forward<Args>(args)...);
    }
  }

protected:
  vtkPolyhedronCellArray();
  ~vtkPolyhedronCellArray() override;

private:
  vtkPolyhedronCellArray(const vtkPolyhedronCellArray&) = delete;
  void operator=(const vtkPolyhedronCellArray&) = delete;

  vtkSmartPointer<vtkCellArray> Faces;
  vtkSmartPointer<vtkCellArray> FaceLocations;
};
VTK_ABI_NAMESPACE_END

namespace vtkPolyhedronCellArray_detail
{
VTK_ABI_NAMESPACE_BEGIN
struct GetCellNumberOfFacesImpl : public vtkPolyhedronCellArray::DispatchUtilities
{
  template <class FaceOffsetsT, class FaceConnectivityT, class LocationOffsetsT,
    class LocationConnectivityT>
  int operator()(FaceOffsetsT*, FaceConnectivityT*, LocationOffsetsT* locationOffsets,
    LocationConnectivityT*, vtkIdType cellId) const
  {
    return static_cast<int>(
      GetEndOffset(locationOffsets, cellId) - GetBeginOffset(locationOffsets, cellId));
  }
};

// The id, in Faces, of the face `face` of the cell cellId.
template <class LocationOffsetsT, class LocationConnectivityT>
vtkIdType GetFaceId(LocationOffsetsT* locationOffsets, LocationConnectivityT* locationConnectivity,
  vtkIdType cellId, int face)
{
  using Utilities = vtkPolyhedronCellArray::DispatchUtilities;
  return static_cast<vtkIdType>(Utilities::GetRange(
    locationConnectivity)[Utilities::GetBeginOffset(locationOffsets, cellId) + face]);
}

struct GetFaceIdImpl
{
  template <class FaceOffsetsT, class FaceConnectivityT, class LocationOffsetsT,
    class LocationConnectivityT>
  vtkIdType operator()(FaceOffsetsT*, FaceConnectivityT*, LocationOffsetsT* locationOffsets,
    LocationConnectivityT* locationConnectivity, vtkIdType cellId, int face) const
  {
    return GetFaceId(locationOffsets, locationConnectivity, cellId, face);
  }
};

struct GetFaceSizeImpl : public vtkPolyhedronCellArray::DispatchUtilities
{
  template <class FaceOffsetsT, class FaceConnectivityT, class LocationOffsetsT,
    class LocationConnectivityT>
  vtkIdType operator()(FaceOffsetsT* faceOffsets, FaceConnectivityT*,
    LocationOffsetsT* locationOffsets, LocationConnectivityT* locationConnectivity,
    vtkIdType cellId, int face) const
  {
    const vtkIdType faceId = GetFaceId(locationOffsets, locationConnectivity, cellId, face);
    return GetEndOffset(faceOffsets, faceId) - GetBeginOffset(faceOffsets, faceId);
  }
};

struct GetFacePointsImpl : public vtkPolyhedronCellArray::DispatchUtilities
{
  // Points pts at the points of the face, copied into tempIds unless they are shared, and returns
  // their number.
  template <class FaceOffsetsT, class FaceConnectivityT, class LocationOffsetsT,
    class LocationConnectivityT>
  vtkIdType operator()(FaceOffsetsT* faceOffsets, FaceConnectivityT* faceConnectivity,
    LocationOffsetsT* locationOffsets, LocationConnectivityT* locationConnectivity,
    vtkIdType cellId, int face, vtkIdType const*& pts, vtkIdList* tempIds) const
  {
    const vtkIdType faceId = GetFaceId(locationOffsets, locationConnectivity, cellId, face);
    const vtkIdType begin = GetBeginOffset(faceOffsets, faceId);
    const vtkIdType npts = GetEndOffset(faceOffsets, faceId) - begin;
    if constexpr (std::is_base_of_v<vtkAOSDataArrayTemplate<vtkIdType>, FaceConnectivityT>)
    {
      // The points are shared, as by vtkCellArray::GetCellAtId().
      pts = faceConnectivity->GetPointer(begin);
    }
    else
    {
      tempIds->SetNumberOfIds(npts);
      vtkIdType* ids = tempIds->GetPointer(0);
      const auto facePoints = GetRange(faceConnectivity).begin() + begin;
      for (vtkIdType i = 0; i < npts; ++i)
      {
        ids[i] = static_cast<vtkIdType>(facePoints[i]);
      }
      pts = ids;
    }
    return npts;
  }
};
VTK_ABI_NAMESPACE_END
} // namespace vtkPolyhedronCellArray_detail

VTK_ABI_NAMESPACE_BEGIN

//----------------------------------------------------------------------------
inline int vtkPolyhedronCellArray::GetCellNumberOfFaces(vtkIdType cellId) const
{
  return this->Dispatch(vtkPolyhedronCellArray_detail::GetCellNumberOfFacesImpl{}, cellId);
}

//----------------------------------------------------------------------------
inline vtkIdType vtkPolyhedronCellArray::GetFaceId(vtkIdType cellId, int face) const
{
  return this->Dispatch(vtkPolyhedronCellArray_detail::GetFaceIdImpl{}, cellId, face);
}

//----------------------------------------------------------------------------
inline vtkIdType vtkPolyhedronCellArray::GetFaceSize(vtkIdType cellId, int face) const
{
  return this->Dispatch(vtkPolyhedronCellArray_detail::GetFaceSizeImpl{}, cellId, face);
}

//----------------------------------------------------------------------------
inline void vtkPolyhedronCellArray::GetFacePoints(
  vtkIdType cellId, int face, vtkIdType& npts, vtkIdType const*& pts, vtkIdList* tempIds) const
{
  npts =
    this->Dispatch(vtkPolyhedronCellArray_detail::GetFacePointsImpl{}, cellId, face, pts, tempIds);
}

VTK_ABI_NAMESPACE_END
#endif // vtkPolyhedronCellArray_h
