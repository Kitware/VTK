// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkUnstructuredGridFieldAnnotations
 * @brief   Fetch/add field annotations from/to an unstructured grid's field data.
 *
 * Takes partitioned dataset collection (or a single unstructured grid) as input and
 * parses (or adds) field-data records as annotations that can be used to improve
 * conversion to (or from) a vtkCellGrid.
 *
 * This field data is currently added by vtkIOSSReader.
 * The vtkIOSSCellGridReader and vtkDGTranscribeUnstructuredCells both use
 * this class.
 */
#ifndef vtkUnstructuredGridFieldAnnotations_h
#define vtkUnstructuredGridFieldAnnotations_h

#include "vtkCellGrid.h"              // for API + ivars
#include "vtkFiltersCellGridModule.h" // For export macro
#include "vtkNew.h"                   // for ivar
#include "vtkObject.h"
#include "vtkStringToken.h" // for API + ivars

#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

class vtkDataArray;
class vtkDataAssembly;
class vtkDataSetAttributes;
class vtkUnstructuredGrid;
class vtkPartitionedDataSetCollection;

class VTKFILTERSCELLGRID_EXPORT vtkUnstructuredGridFieldAnnotations : public vtkObject
{
public:
  static vtkUnstructuredGridFieldAnnotations* New();
  vtkTypeMacro(vtkUnstructuredGridFieldAnnotations, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  virtual void FetchAnnotations(vtkFieldData* fieldData, vtkDataAssembly* assembly);
  virtual void AddAnnotations(vtkFieldData* fieldData, vtkDataAssembly* assembly);
  virtual void Reset();

  /// A key for indexing annotations.
  struct BlockAttributesKey
  {
    vtkStringToken DOFSharing;
    vtkStringToken FunctionSpace;
    bool operator<(const BlockAttributesKey& other) const
    {
      return this->DOFSharing.GetId() < other.DOFSharing.GetId() ||
        (this->DOFSharing == other.DOFSharing &&
          (this->FunctionSpace.GetId() < other.FunctionSpace.GetId()));
    }
  };

  /// Gloms of multiple field names that represent vectors or tensors.
  struct FieldGlom
  {
    /// An ordered list of single-component IOSS arrays that should
    /// be interleaved into a single, multi-component array.
    std::vector<vtkStringToken> Members;
  };

  /// Configuration hints for a partitioned dataset collection entry.
  struct BlockAttributesValue
  {
    vtkStringToken BasisSource;      // Currently always "Intrepid2"
    vtkStringToken FunctionSpace;    // "HDIV", "HGRAD", "HCURL"
    vtkStringToken Shape;            // "HEX", "QUAD", etc.
    vtkStringToken QuadratureScheme; // "I1", "C2", etc.
    vtkStringToken Formulation;      // Currently always "FEM"
    std::set<unsigned int> NodeIds;  // Nodes in the vtkDataAssembly which reference this block.
    mutable std::unordered_set<vtkStringToken> FieldNames; // Special fields for this block.
    mutable std::unordered_map<vtkStringToken, FieldGlom>
      FieldGloms; // Special fields that are glommed.
  };

  /// A map from dataset IDs (i.e., FlatIndex values in the collection) to
  /// a map keyed on function space and DOF sharing; and whose ultimate values
  /// are a numerical basis set of cell-attribute names of that type.
  ///
  /// Each responder is expected to use the request's FlatIndex to find matching
  /// entries in the outer map and iterate the inner map to match arrays to
  /// cell attributes. Any arrays with no match should be considered "traditional"
  /// point- or cell-data.
  std::unordered_map<unsigned int, std::map<BlockAttributesKey, BlockAttributesValue>> Data;

protected:
  vtkUnstructuredGridFieldAnnotations();
  ~vtkUnstructuredGridFieldAnnotations() override = default;

private:
  vtkUnstructuredGridFieldAnnotations(const vtkUnstructuredGridFieldAnnotations&) = delete;
  void operator=(const vtkUnstructuredGridFieldAnnotations&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif // vtkUnstructuredGridFieldAnnotations_h
// VTK-HeaderTest-Exclude: vtkUnstructuredGridFieldAnnotations.h
