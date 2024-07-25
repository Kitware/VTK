// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGridIOQuery
 * @brief   Serialize/deserialize vtkCellMetadata records.
 *
 * vtkCellGridIOQuery is a concrete subclass of vtkCellGridQuery that helps
 * serialize/deserialize vtkCellGrid objects to/from JSON.
 * Specifically, it reads/writes data specific to subclasses of vtkCellMetadata.
 *
 * @sa
 * vtkCellGridQuery
 */

#ifndef vtkCellGridIOQuery_h
#define vtkCellGridIOQuery_h

#include "vtkCellAttribute.h" // For vtkCellAttribute::CellTypeInfo
#include "vtkCellGridQuery.h"
#include "vtkIOCellGridModule.h" // For export macro

#include <vtk_nlohmannjson.h>        // For API.
#include VTK_NLOHMANN_JSON(json.hpp) // For API.

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArraySelection;
class vtkFieldData;
class vtkImageData;
class vtkStringArray;

class VTKIOCELLGRID_EXPORT vtkCellGridIOQuery : public vtkCellGridQuery
{
public:
  static vtkCellGridIOQuery* New();
  vtkTypeMacro(vtkCellGridIOQuery, vtkCellGridQuery);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@name Deserialization Deserialization
  ///@{
  /**
   * Prepare to deserialize cell metadata from the given \a sourceData JSON object.
   *
   * Note that \a sourceData, \a attributeData, and \a attributeList must be kept
   * alive for the duration of the query's execution; the query only references
   * them (rather than copying them).
   */
  void PrepareToDeserialize(const nlohmann::json& sourceData, const nlohmann::json& attributeData,
    std::vector<vtkCellAttribute*>& attributeList);
  ///@}

  ///@name Serialization Serialization
  ///@{
  /**
   * Prepare to serialize cell metadata to the given \a parent JSON object.
   */
  void PrepareToSerialize(nlohmann::json& destination, nlohmann::json& attributeData,
    const std::unordered_map<vtkAbstractArray*, vtkStringToken>& arrayLocations);
  ///@}

  ///@name ResponderUtilities Responder utilities
  ///@{
  /// Return the JSON object that is our source or target.
  nlohmann::json* GetData() { return this->Data; }

  /// Return the JSON object that holds attribute data.
  nlohmann::json* GetAttributeData() { return this->AttributeData; }

  /// Return the map of array pointers to the names of groups which contain them.
  const std::unordered_map<vtkAbstractArray*, vtkStringToken>& GetArrayLocations() const
  {
    return this->ArrayLocations;
  }
  std::unordered_map<vtkAbstractArray*, vtkStringToken>& GetArrayLocations()
  {
    return this->ArrayLocations;
  }

  /// Return a vector of cell-attributes that matches AttributeData.
  const std::vector<vtkCellAttribute*>& GetAttributeList() const { return *this->AttributeList; }

  /// Return true if the query is serializing cells (as opposed to deserializing).
  bool IsSerializing() const { return this->Serializing; }

  /// Extract JSON array information into vtkCellAttribute::CellTypeInfo.
  ///
  /// This is a helper intended to be called by responders.
  /// It extracts data from \a jsonInfo specific to \a cellType
  /// into the \a cellTypeInfo you pass in.
  ///
  /// The \a jsonInfo object point to data for one cell-attribute's "cell-info" key.
  bool ExtractCellTypeAttributeInfo(vtkCellGrid* grid, vtkCellAttribute::CellTypeInfo& cellTypeInfo,
    const nlohmann::json& jsonInfo, vtkStringToken cellTypeName);

  /// Insert vtkCellAttribute::CellTypeInfo into the given \a jsonInfo object
  /// for the given \a cellTypeName.
  ///
  /// This is a helper intended to be called by responders to insert
  /// the base CellTypeInfo data into \a jsonInfo. If subclasses of
  /// vtkCellMetadata ever insert subclasses of CellTypeInfo (not currently
  /// supported), then they will also need to add information specific to
  /// the subclass.
  bool InsertCellTypeAttributeInfo(vtkCellGrid* grid,
    const vtkCellAttribute::CellTypeInfo& cellTypeInfo, nlohmann::json& jsonInfo,
    vtkStringToken cellTypeName);

  /// Add a metadata entry for \a cellTypeName to the output.
  nlohmann::json& AddMetadataEntry(vtkStringToken cellTypeName);
  ///@}

protected:
  vtkCellGridIOQuery() = default;
  ~vtkCellGridIOQuery() override = default;

  nlohmann::json* Data{ nullptr };
  nlohmann::json* AttributeData{ nullptr };
  std::vector<vtkCellAttribute*>* AttributeList;
  std::unordered_map<vtkAbstractArray*, vtkStringToken> ArrayLocations;
  bool Serializing{ true };

private:
  vtkCellGridIOQuery(const vtkCellGridIOQuery&) = delete;
  void operator=(const vtkCellGridIOQuery&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
