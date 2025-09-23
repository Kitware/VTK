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
 * grid if the cell is a 1D-2D.
 *
 * To use this class first you need to call BuildHashLinks() with the
 * vtkUnstructuredGrid as argument. Then you can use the methods
 * 1) GetNumberOfFaces() to get the number of faces in the grid
 * 2) GetNumberOfHashes() to get the number of hashes
 * 4) GetNumberOfFacesInHash() to get the number of faces in a particular hash
 * 5) GetCellIdOfFacesInHash() to get the cell id of the faces in a particular hash
 * 6) GetFaceIdOfFacesInHash() to get the face id of the faces in a particular hash
 *
 * In general, this class tries to minimize the memory usage as much as possible.
 * For that reason while to get a face you need to cell id and the face id, so they could
 * be a struct, it's more efficient to store them in two different arrays to minimize
 * the memory usage.
 *
 * Template parameters:
 * 1) TInputIdType: the type of the id of the cells and points of the input grid.
 *    If Number of points and number of cells Cells < 2^31 then use vtkTypeInt32.
 *    Otherwise, use vtkTypeInt64.
 * 2) TFaceIdType: the type of the id of the faces of the input grid. Most of the times
 *    a cell can have less than 2^7 faces, so use vtkTypeInt8. Otherwise, use vtkTypeInt32
 *    when the input grid has polyhedron cells.
 *
 * @warning
 * This class handles only linear cells.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkStaticEdgeLocatorTemplate, vtkStaticCellLinksTemplate
 */
#ifndef vtkStaticFaceHashLinksTemplate_h
#define vtkStaticFaceHashLinksTemplate_h

#include "vtkABINamespace.h"
#include "vtkType.h"

#include <memory>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class vtkUnstructuredGrid;
VTK_ABI_NAMESPACE_END

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
  void BuildHashLinks(vtkUnstructuredGrid* input);

  /**
   * Reset the hash links and free the memory.
   */
  void Reset();

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
    return this->FaceOffsets.get()[hash + 1] - this->FaceOffsets.get()[hash];
  }

  /**
   * Get cell id of faces in a particular hash.
   */
  TInputIdType* GetCellIdOfFacesInHash(vtkIdType hash) const
  {
    return this->CellIdOfFaceLinks.get() + this->FaceOffsets.get()[hash];
  }

  /**
   * Get face id of faces in a particular hash.
   */
  TFaceIdType* GetFaceIdOfFacesInHash(vtkIdType hash) const
  {
    return this->FaceIdOfFaceLinks.get() + this->FaceOffsets.get()[hash];
  }

protected:
  vtkIdType NumberOfFaces = 0;
  vtkIdType NumberOfHashes = 0;
  std::shared_ptr<TInputIdType> CellIdOfFaceLinks;
  std::shared_ptr<TFaceIdType> FaceIdOfFaceLinks;
  std::shared_ptr<vtkIdType> FaceOffsets;

private:
  vtkStaticFaceHashLinksTemplate(const vtkStaticFaceHashLinksTemplate&) = delete;
  void operator=(const vtkStaticFaceHashLinksTemplate&) = delete;

  struct GeometryInformation;

  struct CountFaces;

  template <typename TCellOffSetIdType>
  struct CreateFacesInformation;

  template <typename TCellOffSetIdType>
  struct CountHashes;

  struct PrefixSum;

  template <typename TCellOffSetIdType>
  struct BuildFaceHashLinks;

  template <typename TCellOffSetIdType>
  void BuildHashLinksInternal(vtkUnstructuredGrid* input, GeometryInformation& geometryInformation);
};

VTK_ABI_NAMESPACE_END
#include "vtkStaticFaceHashLinksTemplate.txx"

#endif // vtkStaticFaceHashLinksTemplate_h
// VTK-HeaderTest-Exclude: vtkStaticFaceHashLinksTemplate.h
