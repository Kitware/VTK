// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGridCopyQuery
 * @brief   Copy the cell metadata and attribute(s) of one cell-grid into another.
 *
 * Note that this query is run by vtkCellGrid::ShallowCopy(), vtkCellGrid::DeepCOpy(),
 * and vtkCellGrid::CopyStructure().
 *
 * In general, there are five types of information in cell grids that may
 * be transferred from the source cell-grid to the target. Here are
 * the types of information and the options which control how that
 * information is copied. Exactly how these flags on the query are used is
 * up to each responder.
 *
 * + **Cell metadata records.** These records are always copied.
 *   In the future, there may be an option to omit cells of specific types.
 *
 * + **Individual cells.** If subclasses of vtkCellMetadata contain
 *   further information, you may use SetCopyCells() to control
 *   whether that is copied or whether the new vtkCellMetadata
 *   instance is left uninitialized.
 *   When GetCopyCells() is enabled, the cell topology should be copied
 *   (though not necessarily the shape attribute's arrays);
 *   CopyCells overrides the copying of topological arrays even if
 *   CopyArrays is turned off.
 *   This way, if CopyCells is on, you should expect the source and
 *   target to report the same number of cells.
 *
 * + **Cell attributes.** You may request that only the shape attribute
 *   is copied from the source to the target with CopyOnlyShapeOn()
 *   or control which attributes are copied by calling
 *   AddSourceCellAttributeId() with the ID of each source attribute
 *   you wish copied.
 *
 * + **Cell-attribute arrays.** For each cell-attribute that is copied,
 *   zero or more arrays may be associated the attribute. You can
 *   control how the arrays are copied like so:
 *
 *   + SetCopyArrays() controls whether arrays should be created or not.
 *     How the arrays are copied depends on whether CopyArrayValues and
 *     DeepCopyArrays are enabled. Note that this setting should be
 *     ignored when copying cell topology (as opposed to attribute) arrays
 *     as CopyCells should control whether cells are present in the output.
 *     If cell-topology arrays are referenced by a cell attribute, be aware
 *     that disabling CopyArrays may still produce some entries for topology
 *     arrays.
 *   + SetCopyArrayValues() controls whether arrays should be (a) created
 *     but left empty or (b) created and populated with the source-array's
 *     values. This is useful for creating an empty copy that has all
 *     the necessary arrays prepared but no tuples so that further
 *     processing can insert new cells and attribute data.
 *   + SetDeepCopyArrays() controls whether to create deep copies of
 *     arrays or shallow copies, but only when GetCopyArrayValues()
 *     returns true.
 *
 * + **Schema and content version.** A cell-grid may advertise that its
 *   data adheres to a formal specification (which is indicated by a
 *   name and version number). If you wish to copy this information,
 *   ensure CopySchemaOn() has been called.
 *   If GetCopySchema() is true and the source has a content version
 *   number, the target cell-grid will have its content version
 *   incremented past the source's content version.
 *   Incrementing the content version even when the grids are otherwise
 *   identical improves track-ability, since the version number informs
 *   which grid preceded the other.
 *
 * ## For Callers
 *
 * You **must** execute this query on the source cell-grid, not the target.
 * Only the source is guaranteed to have cells of the proper types present;
 * the query iterates over each cell-type, so they must be present.
 *
 * Executing this query will overwrite the target cell-grid with the source,
 * erasing all of its cell metadata.
 * In the future, this class may offer more control over which types of
 * cells to copy from the source to the target.
 *
 * ## For Responders
 *
 * Responders to this query may call the helper methods provided
 * to copy a cell-attribute's arrays and create/update a cell-attribute.
 * These calls update maps from source to target arrays and attributes,
 * which you can inspect by calling GetArrayMap() and GetAttributeMap(),
 * respectively.
 * The latter is important since distinct attributes may have identical
 * names (though this is not advised).
 */

#ifndef vtkCellGridCopyQuery_h
#define vtkCellGridCopyQuery_h

#include "vtkCellGridQuery.h"
#include "vtkStringToken.h" // For API.

#include <map> // For ArrayMap ivar.
#include <set> // For CellAttributeIds ivar.

VTK_ABI_NAMESPACE_BEGIN

class vtkAbstractArray;
class vtkCellAttribute;
class vtkCellGrid;
class vtkIdList;

class VTKCOMMONDATAMODEL_EXPORT vtkCellGridCopyQuery : public vtkCellGridQuery
{
public:
  static vtkCellGridCopyQuery* New();
  vtkTypeMacro(vtkCellGridCopyQuery, vtkCellGridQuery);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@name QueryExecution Query Execution.
  ///@{
  /// This clears the ArrayMap ivar before the algorithm starts.
  bool Initialize() override;

  /// This clears the ArrayMap ivar after the algorithm completes (to save space).
  bool Finalize() override;
  ///@}

  ///@name SourceAndTarget Source and Target Cell-Grids.
  ///@{

  /// Set/get the source cell-grid to copy into the cell-grid on which this query is run.
  virtual void SetSource(vtkCellGrid* source);
  vtkGetObjectMacro(Source, vtkCellGrid);

  /// Set/get the target cell-grid into which the source should be copied.
  virtual void SetTarget(vtkCellGrid* target);
  vtkGetObjectMacro(Target, vtkCellGrid);

  ///@}

  ///@name CopySetup Copy Style Setup.
  ///@{

  /// Set/get whether to copy cell metadata instances
  /// or leave target grid void of all cell types.
  ///
  /// The default (true) is to copy cell types.
  vtkGetMacro(CopyCellTypes, int);
  vtkSetMacro(CopyCellTypes, int);
  vtkBooleanMacro(CopyCellTypes, int);

  /// Set/get whether to copy cell topology or leave each output instance
  /// of cell metadata uninitialized.
  ///
  /// The default (true) is to copy cells.
  vtkGetMacro(CopyCells, int);
  vtkSetMacro(CopyCells, int);
  vtkBooleanMacro(CopyCells, int);

  /// Set/get whether **only** the shape attribute should be copied.
  ///
  /// This option is provided so vtkCellGridCopyQuery can behave similar to
  /// vtkDataSet::CopyStructure().
  ///
  /// If true (the default), no vtkCellAttribute instances other than
  /// vtkCellGrid::GetShapeAttribute() will be copied from the source to
  /// the target, regardless of the values returned by
  /// vtkCellGridCopyQuery::GetCellAttributeIds().
  ///
  /// If false, then you **must** explicitly add the source's shape attribute
  /// to the schedule of attributes to be copied by calling
  /// vtkCellGridCopyQuery::AddSourceCellAttributeId() or the target will have
  /// no geometric shape (which is disallowed).
  /// (The intential omission of the shape attribute when CopyOnlyShape is
  /// false allows you to omit the shape during copying if you will be
  /// synthesizing a new shape for the same output cell topology.)
  vtkGetMacro(CopyOnlyShape, int);
  vtkSetMacro(CopyOnlyShape, int);
  vtkBooleanMacro(CopyOnlyShape, int);

  /// Set/get whether data arrays should be copied or omitted.
  ///
  /// The default (true) is for arrays associated with each cell-attribute to
  /// be copied. If false, then the requested cell-attribute(s) will be copied
  /// but will have no arrays set for any cell type.
  vtkGetMacro(CopyArrays, int);
  vtkSetMacro(CopyArrays, int);
  vtkBooleanMacro(CopyArrays, int);

  /// Set/get whether values in data arrays should be copied or not.
  ///
  /// The default is true (i.e., the source and target array contents
  /// should be equal (when DeepCopyArrays is false) or equivalent (when
  /// DeepCopyArrays is true)).
  vtkGetMacro(CopyArrayValues, int);
  vtkSetMacro(CopyArrayValues, int);
  vtkBooleanMacro(CopyArrayValues, int);

  /// Set/get whether data arrays should be copied by value (deep) or
  /// by reference (shallow). The default is false (i.e., shallow copy).
  ///
  /// This setting is ignored when GetCopyArrays() and/or GetCopyArrayValues() are false.
  /// If GetCopyArrays() returns false, no arrays will be copied from the source.
  /// If GetCopyArrays() returns true but GetCopyArrayValues() returns false,
  /// then new, empty arrays of the same type, name, and number of components
  /// as the source arrays will be created but no tuples will be inserted in them.
  vtkGetMacro(DeepCopyArrays, int);
  vtkSetMacro(DeepCopyArrays, int);
  vtkBooleanMacro(DeepCopyArrays, int);

  /// Set/get whether the source cell-grid's schema information should be copied.
  ///
  /// If true (the default), the query's Finalize() method will copy the schema to the target.
  vtkGetMacro(CopySchema, int);
  vtkSetMacro(CopySchema, int);
  vtkBooleanMacro(CopySchema, int);

  ///@}

  ///@name AttCopySetup Attribute Copying Setup.
  ///
  /// Note that even when GetCellAttributeIds() returns an empty set,
  /// the shape attribute may be copied. (Having a geometric shape
  /// is a mandatory part of a cell-grid.)

  ///@{
  /// Add \a attributeId to the list of cell-attributes to be copied from the source.
  virtual bool AddSourceCellAttributeId(int attributeId);

  /// Remove \a attributeId from the list of cell-attributes to be copied from the source.
  virtual bool RemoveSourceCellAttributeId(int attributeId);

  /// Add all of the source cell-grid's attributes to the list of IDs to be copied.
  ///
  /// Obviously, you must have called SetSource() prior to calling this method.
  virtual bool AddAllSourceCellAttributeIds();

  /// Return the set of attribute IDs scheduled to be copied when the query is run.
  const std::set<int>& GetCellAttributeIds() const { return this->CellAttributeIds; }

  /// Populate \a ids with the attribute IDs scheduled to be copied when the query is run.
  void GetCellAttributeIds(vtkIdList* ids) const;

  /// Reset the query so that no cell-attributes will be copied from the source.
  virtual void ResetCellAttributeIds();

  ///@}

  ///@name AttCopyState Attribute Copying State.
  ///
  ///@{

  /// Return the map from source to target arrays.
  ///
  /// This map is cleared by both Initialize() and Finalize()
  /// to minimize the time it holds data.
  const std::map<vtkAbstractArray*, vtkAbstractArray*>& GetArrayMap() const
  {
    return this->ArrayMap;
  }
  std::map<vtkAbstractArray*, vtkAbstractArray*>& GetArrayMap() { return this->ArrayMap; }

  /// Return the map from source to target cell-attributes.
  ///
  /// This map is cleared by both Initialize() and Finalize()
  /// to minimize the time it holds data.
  const std::map<vtkCellAttribute*, vtkCellAttribute*>& GetAttributeMap() const
  {
    return this->AttributeMap;
  }
  std::map<vtkCellAttribute*, vtkCellAttribute*>& GetAttributeMap() { return this->AttributeMap; }

  ///@}

  ///@name ResponderHelpers
  /// Responder Helpers.
  ///
  /// These methods are available for query-responders to use inside
  /// their implementations of Query().
  ///@{

  /// Copy the arrays for a single attribute from the source to the target.
  ///
  /// This method **does not** check CopyOnlyShape and CellAttributeIds to determine
  /// whether the attribute should be copied. Your responder must verify the
  /// attribute should be copied.
  ///
  /// This method **does** check the CopyArrays, CopyArrays, and CopyArrayValues
  /// flags to determine whether to reference source arrays or create (and potentially
  /// copy values) into new arrays.
  void CopyAttributeArrays(vtkCellAttribute* srcAtt, vtkStringToken cellType);

  /// Copy a cell-attribute (\a srcAtt) from the source into the target.
  ///
  /// This method uses the ArrayMap to set the arrays for each cell type
  /// (if arrays are being copied and not being copied by reference).
  ///
  /// This calls vtkCellAttribute::DeepCopy() on \a srcAtt and
  /// then copies any cached range data from the source to the target cell-grid.
  ///
  /// Your responder **must** call CopyAttributeArrays() for all the output
  /// cell types **before** calling this method if GetCopyArrays() returns true
  /// so that the ArrayMap will be properly populated.
  vtkCellAttribute* CopyOrUpdateAttributeRecord(vtkCellAttribute* srcAtt, vtkStringToken cellType);

  ///@}

protected:
  vtkCellGridCopyQuery() = default;
  ~vtkCellGridCopyQuery() override;

  vtkCellGrid* Source{ nullptr };
  vtkCellGrid* Target{ nullptr };
  std::set<int> CellAttributeIds;
  std::map<vtkAbstractArray*, vtkAbstractArray*> ArrayMap;
  std::map<vtkCellAttribute*, vtkCellAttribute*> AttributeMap;
  int CopyCellTypes{ 1 };
  int CopyCells{ 1 };
  int CopyOnlyShape{ 1 };
  int CopyArrays{ 1 };
  int CopyArrayValues{ 1 };
  int DeepCopyArrays{ 0 };
  int CopySchema{ 1 };

private:
  vtkCellGridCopyQuery(const vtkCellGridCopyQuery&) = delete;
  void operator=(const vtkCellGridCopyQuery&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridCopyQuery_h
