// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStaticFaceHashLinksTemplate
 * @brief   templated hashLinks for extracting faces from a vtkUnstructuredGrid
 *
 * vtkStaticFaceHashLinksTemplate is a templated class for grouping faces
 * of an unstructured grid. The faces are grouped by their hash value.
 * Their hash value is the minimum of the ids of the points defining the
 * face of a 3D cells, or the the number of points of the unstructured
 * grid if the cell is a 0D-1D-2D.
 *
 * To use this class first you need to call BuildHashLinks() with the
 * vtkUnstructuredGrid as argument. Then you can use the methods
 * 1) GetNumberOfFaces() to get the number of faces in the grid
 * 2) GetNumberOfHashes() to get the number of hashes
 * 4) GetNumberOfFacesInHash() to get the number of faces in a particular hash
 * 5) GetCellIdOfFacesInHash() to get the cell id of the faces in a particular hash
 * 6) GetFaceIdOfFacesInHash() to get the face id of the faces in a particular hash
 *
 * The hash links are the keys of vtkUnstructuredGridSides::BuildScatteredSides(): the faces of the
 * 3D cells, in the order of their cells, then, in the last hash, the 0D-1D-2D cells, in the order
 * of their ids, with a face id of -1.
 *
 * Template parameters:
 * 1) TInputIdType: the type of the id of the cells and points of the input grid.
 *    If Number of points and number of cells Cells < 2^31 then use vtkTypeInt32.
 *    Otherwise, use vtkTypeInt64.
 * 2) TFaceIdType: the type of the id of the faces of the input grid. Most of the times
 *    a cell can have less than 2^7 faces, so use vtkTypeInt8. Otherwise, use vtkTypeInt32
 *    when the input grid has polyhedron cells.
 * Only these 4 combinations, those of vtkUnstructuredGridSides, are supported.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkUnstructuredGridSides, vtkStaticElementLocator, vtkStaticCellLinksTemplate
 */

#ifndef vtkStaticFaceHashLinksTemplate_h
#define vtkStaticFaceHashLinksTemplate_h

#include "vtkABINamespace.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer
#include "vtkType.h"
#include "vtkUnstructuredGrid.h"      // For GetNumberOfPoints()
#include "vtkUnstructuredGridSides.h" // For the hash links

#include <memory>      // For std::shared_ptr
#include <type_traits> // For std::is_same_v

VTK_ABI_NAMESPACE_BEGIN
/**
 * Templated on types of ids defining an edge.
 */
template <typename TInputIdType, typename TFaceIdType>
class vtkStaticFaceHashLinksTemplate
{
public:
  vtkStaticFaceHashLinksTemplate() = default;

  /**
   * Build the hash links.
   */
  void BuildHashLinks(vtkUnstructuredGrid* input)
  {
    static_assert(
      std::is_same_v<TInputIdType, vtkTypeInt32> || std::is_same_v<TInputIdType, vtkTypeInt64>,
      "TInputIdType must be vtkTypeInt32 or vtkTypeInt64, see vtkUnstructuredGridSides");
    static_assert(
      std::is_same_v<TFaceIdType, vtkTypeInt8> || std::is_same_v<TFaceIdType, vtkTypeInt32>,
      "TFaceIdType must be vtkTypeInt8 or vtkTypeInt32, see vtkUnstructuredGridSides");
    // The hashes: the faces of the 3D cells, scattered by their smallest point id, and the 0D-1D-2D
    // cells, in the last key id.
    using SidesType = vtkUnstructuredGridSides<TInputIdType, TFaceIdType>;
    this->Sides = vtkSmartPointer<SidesType>::New();
    this->Sides->SetDataSet(input);
    this->Sides->SetOutputDimensionControl(SidesType::SideFlags::SurfacesOfVolumes);
    this->Sides->SetPreserveRenderableInputs(true);
    // The duplicate ghost cells are ordinary cells, so that the 0D-1D-2D ones are preserved: the
    // ghost cells are masked by the user of the hash links, e.g., vtkGeometryFilter.
    this->Sides->SetRemoveGhostInterfaces(false);
    this->Sides->BuildScatteredSides();
    this->NumberOfFaces = this->Sides->GetNumberOfSides();
    this->NumberOfHashes = input->GetNumberOfPoints() + 1 /* for the 0D-1D-2D faces */;
  }

  /**
   * Reset the hash links and free the memory.
   */
  void Reset()
  {
    this->NumberOfFaces = 0;
    this->NumberOfHashes = 0;
    this->Sides = nullptr;
  }

  /**
   * Get number of faces.
   */
  vtkIdType GetNumberOfFaces() const { return this->NumberOfFaces; }

  /**
   * Get the number of hashes.
   */
  vtkIdType GetNumberOfHashes() const { return this->NumberOfHashes; }

  /**
   * Get the number of faces in a particular hash.
   */
  vtkIdType GetNumberOfFacesInHash(vtkIdType hash) const
  {
    const TInputIdType keyId = this->GetKeyId(hash);
    return keyId >= 0 ? this->Sides->GetNumberOfSidesOfKey(keyId) : 0;
  }

  /**
   * Get cell id of faces in a particular hash.
   */
  TInputIdType* GetCellIdOfFacesInHash(vtkIdType hash) const
  {
    const TInputIdType keyId = this->GetKeyId(hash);
    return keyId >= 0 ? const_cast<TInputIdType*>(this->Sides->GetCellIdsOfKey(keyId)) : nullptr;
  }

  /**
   * Get face id of faces in a particular hash.
   */
  TFaceIdType* GetFaceIdOfFacesInHash(vtkIdType hash) const
  {
    const TInputIdType keyId = this->GetKeyId(hash);
    return keyId >= 0 ? const_cast<TFaceIdType*>(this->Sides->GetSideIdsOfKey(keyId)) : nullptr;
  }

protected:
  vtkIdType NumberOfFaces = 0;
  vtkIdType NumberOfHashes = 0;
  // Not used anymore: the hash links are the keys of vtkUnstructuredGridSides.
  std::shared_ptr<TInputIdType> CellIdOfFaceLinks;
  std::shared_ptr<TFaceIdType> FaceIdOfFaceLinks;
  std::shared_ptr<vtkIdType> FaceOffsets;

private:
  vtkStaticFaceHashLinksTemplate(const vtkStaticFaceHashLinksTemplate&) = delete;
  void operator=(const vtkStaticFaceHashLinksTemplate&) = delete;

  // The key id of the faces of a hash: the key of its point, the last key id for the last hash, or
  // -1 for a point beyond the keys, which has no face.
  TInputIdType GetKeyId(vtkIdType hash) const
  {
    const TInputIdType lastKeyId = this->Sides->GetNumberOfKeys() - 1;
    if (hash == this->NumberOfHashes - 1)
    {
      return lastKeyId;
    }
    return hash < lastKeyId ? static_cast<TInputIdType>(hash) : -1;
  }

  vtkSmartPointer<vtkUnstructuredGridSides<TInputIdType, TFaceIdType>> Sides;
};
VTK_ABI_NAMESPACE_END

#endif // vtkStaticFaceHashLinksTemplate_h
// VTK-HeaderTest-Exclude: vtkStaticFaceHashLinksTemplate.h
