// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkSelectionNode
 * @brief a node in a vtkSelection the defines the selection criteria.
 *
 * vtkSelectionNode helps define the selection criteria in a vtkSelection.
 * vtkSelection can comprise of multiple such vtkSelectionNode instances that
 * help define the selection.
 *
 * vtkSelectionNode has two components: a list of properties (stored in a
 * vtkInformation) and a selection list (a vtkAbstractArray subclass). The
 * properties help indicate how to interpret the values specified in a
 * selection-list.
 *
 * The properties can be broadly classified into three categories: core,
 * qualifiers, and information. The core properties must be specified other wise the
 * vtkSelectionNode is not considered valid. These are `FIELD_TYPE` and
 * `CONTENT_TYPE`. `FIELD_TYPE` defines what kinds of entities are being selected.
 * Since selections are used to select items in a data-object, these correspond to
 * things like cells, points, nodes, edges, rows, etc. Supported FIELD_TYPE
 * values are defined in `vtkSelectionNode::SelectionField`. `CONTENT_TYPE`
 * defines the how the selection is described. Supported values are
 * `vtkSelectionNode::SelectionContent`. For example, if CONTENT_TYPE is
 * `INDICES`, then selection is described as ids for entities being selected.
 * The ids themselves will be specified using the selection list. Thus, the
 * CONTENT_TYPE dictates what the selection list comprises.
 *
 * Qualifiers are used to further qualify the selection criteria. All qualifiers
 * are optional. When present they restrict the selection. For example, when
 * selecting elements from a composite-dataset, the `COMPOSITE_INDEX` qualifier
 * can be specified of limit the selection to a specific dataset (or subtree).
 *
 * Information properties are simply for informative purposes and generally used
 * to provide information about the selection when the selection is created. For
 * example, `PIXEL_COUNT` is used to indicate how many screen pixels resulted in
 * creation of this selection node.
 *
 * @section SelectionTypes Selection Types
 *
 * `CONTENT_TYPE` property is one of the required properties that helps
 * describe how the selection is defined. To set the content type, one can
 * access the properties store using `vtkSelectionNode::GetProperties()` and
 * then set the `CONTENT_TYPE` using the `vtkSelectionNode::CONTENT_TYPE()` key
 * or simply use `vtkSelectionNode::SetContentType`.
 *
 * * `vtkSelectionNode::GLOBALIDS`: indicates that the selection is defined
 *   using global ids. In VTK data-object, global ids are specified an `vtkIdTypeArray`
 *   added to a appropriate `vtkDataSetAttributes` and marked as global-ids
 *   using vtkDataSetAttributes API. Since global ids are expected to be unique
 *   for that element type over the entire dataset, it's a convenient way of
 *   defining selections. For this content-type, the selection list must be
 *   a single-component, `vtkIdTypeArray` that lists all the globals ids for
 *   the selected elements.
 *
 * * `vtkSelectionNode::PEDIGREEIDS`: similar to `GLOBALIDS` except uses
 *   pedigree ids instead of global ids.
 *
 * * `vtkSelectionNode::VALUES`: this type is used to define a selection based
 *   on array values. The selection list specifies the values to be selected.
 *   All elements with array values in the selection list are treated as
 *   selected. The qualifier COMPONENT_NUMBER is used to indicate which
 *   component to use for the checks. Use `-1` for magnitude. Current
 *   implementation does not support checking multiple-components or non-exact
 *   matches although support for both is conceivable in future.
 *   The selection list array name is used to specify the name of the array from
 *   the dataset to use for the checks. Thus, for defining a selection for a
 *   dataset where all `TEMP` values match those specified in the selection
 *   list, ensure that the selection list array's name is set to `TEMP` as well.
 *
 * * `vtkSelectionNode::INDICES`: this is similar to global ids except in this
 *   case the selection list is simply the VTK element id which is 0-based
 *   index of that element in the particular dataset. Often with this type of
 *   selection, additional qualifiers such as `COMPOSITE_INDEX`,
 *   `BLOCK_SELECTORS`, `PROCESS_ID` etc. are needed to correctly identify the
 *   chosen element(s) in case of composite or distributed datasets.
 *
 * * `vtkSelectionNode::FRUSTUM`: this type is used to define a frustum in world
 *   coordinates that identifies the selected elements. In this case, the
 *   selection list is a vtkDoubleArray with 32 values specifying the 8 frustum
 *   corners in homogeneous world coordinates.
 *
 * * `vtkSelectionNode::LOCATIONS`: this is used to select points (or cells)
 *   near (or containing) specified locations. The selection list is a
 *   3-component vtkDoubleArray with coordinates for locations of interest.
 *
 * * `vtkSelectionNode::THRESHOLDS`: this type is used to define a selection based
 *   on array value ranges. This is akin to thresholding. All elements with values in
 *   the specified ranges are to be treated as selected. For this content-type,
 *   the selection-list is a 2-component `vtkDataArray`-subclass that specifies
 *   where each tuple defines the min and max values for a range. The selection
 *   list can have multiple tuples to define multiple ranges. Elements with
 *   values in any of the specified ranges are treated as selected. The
 *   selection list array name is used to specify the name of the array from
 *   the dataset to use for the checks. Thus, for defining a selection for a
 *   dataset where all `TEMP` values are within a range, ensure that the
 *   selection list array's name is set to `TEMP` as well.
 *
 * * `vtkSelectionNode::BLOCKS`: this type is used to select blocks in a
 *   composite dataset. The term blocks is used loosely here and can correspond
 *   to a block in a multiblock dataset or a partition in a partitioned-dataset.
 *   The selection list is an integral type vtkDataArray subclass that can be 1-
 *   or 2- component. If 1-component, it's interpreted as the composite-index
 *   (also called flat index) and can be applied to any composite-dataset to
 *   choose specific datasets. If 2-component, it's typically associated with
 *   vtkUniformGridAMR or vtkPartitionedDataSetCollection which support 2-tuple
 *   indexing to locate a dataset.
 *
 * * `vtkSelectionNode::BLOCK_SELECTORS`: this is similar to BLOCKS, however
 *   instead of using indices to select datasets, here, the selection list is a
 *   vtkStringArray which lists selector expressions to select blocks in the
 *   composite dataset. By default, the selector expressions are applied to a
 *   vtkDataAssembly generated from the composite dataset that represents its
 *   hierarchy (see `vtkDataAssembly::GenerateHierarchy`). However, in case of
 *   vtkPartitionedDataSetCollection, one can select any other data assembly
 *   associated with the vtkPartitionedDataSetCollection by naming the array
 *   with the name of the assembly.
 *
 *   @note, currently vtkPartitionedDataSetCollection only supports a single
 *   vtkDataAssembly but this may change in the future.
 *
 * * `vtkSelectionNode::QUERY`: this type is primarily added for ParaView where
 *   selection expression is specified as a query string. This is likely to
 *   change in the future and hence applications are discouraged from using this
 *   type.
 *
 * @section Properties Properties
 *
 * Following a properties that can be used to qualify the selection.
 *
 * * `vtkSelectionNode::EPSILON()`: this is a qualifier that can be used to
 *   indicate a fuzz-factor when comparing values for equality. Currently, this
 *   is only used with content-type LOCATIONS, however, it can be expanded to
 *   other selection types in the future.
 *
 * * `vtkSelectionNode::CONTAINING_CELLS()`: this qualifier is intended to be
 *   used with field-type `POINT`. When present, it indicates that while the
 *   selection criteria selects a collection of points the selection should be
 *   formed using cell containing the chosen points.
 *
 * * `vtkSelectionNode::CONNECTED_LAYERS()`: a qualifier used to expand the
 *   definition of selected elements to connected elements for the specified
 *   number of layers. Layers can only be positive to grow the selection.
 *
 * * `vtkSelectionNode::CONNECTED_LAYERS_REMOVE_SEED()`: this qualifier indicates
 *   that when using a number of CONNECTED_LAYERS >= 1, the initial selection will
 *   not be kept.
 *
 * * `vtkSelectionNode::CONNECTED_LAYERS_REMOVE_INTERMEDIATE_LAYERS()`: this qualifier
 *   indicates that when using a number of CONNECTED_LAYERS >= 2, the intermediate layers
 *   will not be kept.
 *
 * * `vtkSelectionNode::INVERSE()`: a qualifier that causes the selection to be
 *   inverted i.e. all elements not chosen by the criteria are to be treated
 *   as selected.
 *
 * * `vtkSelectionNode::COMPONENT_NUMBER()`: for VALUES and THRESHOLDS selection
 *   types, this qualifier identifies the array component of interest. -1
 *   indicates magnitude.
 *
 * * `vtkSelectionNode::PROCESS_ID()`: limits the selection to a particular
 *   rank in a distributed environment.
 *
 * * `vtkSelectionNode::COMPOSITE_INDEX()`: a qualifier used to limit the
 *   selection to a specific composite index for a composite-dataset.
 *
 * * `vtkSelectionNode::ASSEMBLY_NAME()`, `vtkSelectionNode::SELECTORS()`:
 *   similar to composite index, except uses data-assembly and selectors to
 *   limit the selection to a subset of nodes in a composite-dataset.
 *
 * * `vtkSelectionNode::HIERARCHICAL_LEVEL()`,
 *   `vtkSelectionNode::HIERARCHICAL_INDEX()`: similar to composite index, except
 *   uses level and index for an AMR dataset so limit the selection to a
 *   specific AMR level or dataset.
 *
 * Following for properties that are primarily intended to provide additional
 * information when the selection is created.
 *
 * * `vtkSelectionNode::ZBUFFER_VALUE()`: an information qualifier representing
 *   the z-depth for a particular selection when it was created.
 *
 * * `vtkSelectionNode::PIXEL_COUNT()`: a qualifier used to provide a count for
 *   the number of pixels that resulted in this selection.
 *
 * * `vtkSelectionNode::SOURCE()`, `vtkSelectionNode::SOURCE_ID()`: provides
 *   information about data producer or selection originator. The interpretation
 *   is very specific to the creator creating the selection and varies greatly
 *   with VTK.
 *
 * * `vtkSelectionNode::PROP(), `vtkSelectionNode::PROP_ID()`: similar to
 *   SOURCE/SOURCE_ID except is used to represent a rendering prop from which
 *   the selection was created.
 *
 * @warning
 * No SelectionList is created by default. It should be assigned.
 *
 * @section SelectionFieldMismatch vtkSelectionNode::SelectionField and
 * vtkDataSetAttribute::AttributeTypes
 *
 * Strictly speaking, vtkSelectionNode::SelectionField maps directly to
 * vtkDataSetAttribute::AttributeTypes. However, the two enum values are not
 * identical for historical reasons. Use
 * `vtkSelectionNode::ConvertSelectionFieldToAttributeType` and
 * `vtkSelectionNode::ConvertAttributeTypeToSelectionField` to convert between
 * the two.
 */

#ifndef vtkSelectionNode_h
#define vtkSelectionNode_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractArray;
class vtkDataSetAttributes;
class vtkInformation;
class vtkInformationDoubleKey;
class vtkInformationIntegerKey;
class vtkInformationObjectBaseKey;
class vtkInformationStringKey;
class vtkInformationStringVectorKey;
class vtkProp;
class vtkTable;

class VTKCOMMONDATAMODEL_EXPORT vtkSelectionNode : public vtkObject
{
public:
  vtkTypeMacro(vtkSelectionNode, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkSelectionNode* New();

  /**
   * Restore data object to initial state,
   */
  virtual void Initialize();

  ///@{
  /**
   * Get/Set the selection list. The selection list is the container
   * that stores values that indicate the selected items. What these values
   * correspond to depends on the `ContentType`. `ContentType` may also dictate
   * the type and form of the selection list array.
   */
  virtual void SetSelectionList(vtkAbstractArray*);
  virtual vtkAbstractArray* GetSelectionList();
  ///@}

  ///@{
  /**
   * Sets the selection table.
   */
  virtual void SetSelectionData(vtkDataSetAttributes* data);
  vtkGetObjectMacro(SelectionData, vtkDataSetAttributes);
  ///@}

  ///@{
  /**
   * Returns the property map.
   */
  vtkGetObjectMacro(Properties, vtkInformation);
  ///@}

  /**
   * Copy properties, selection list and children of the input.
   */
  virtual void DeepCopy(vtkSelectionNode* src);

  /**
   * Copy properties, selection list and children of the input.
   * This is a shallow copy: selection lists and pointers in the
   * properties are passed by reference.
   */
  virtual void ShallowCopy(vtkSelectionNode* src);

  /**
   * Return the MTime taking into account changes to the properties
   */
  vtkMTimeType GetMTime() override;

  // vtkSelectionNode specific keys follow:
  /**
   * Get the (primary) property that describes the content of a selection
   * node's data. This key takes on values from the SelectionContent enum.
   * GetContentType() returns -1 if the content type is not set.
   *
   * \sa vtkSelectionNode::SelectionContent
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* CONTENT_TYPE();

  /**
   * Indicate the means by which data is selected.
   * In some cases this implies the type of data selected.
   */
  enum SelectionContent
  {
    GLOBALIDS,       //!< Select entities called out by their globally-unique IDs.
    PEDIGREEIDS,     //!< Select entities that have some identifiable pedigree.
    VALUES,          //!< Select entities that take on specific array values.
    INDICES,         //!< Select entities by their offsets into the dataset.
    FRUSTUM,         //!< Select entities contained within a viewing frustum.
    LOCATIONS,       //!< Select entities near the supplied world coordinates.
    THRESHOLDS,      //!< Select entities whose array values fall within a given threshold.
    BLOCKS,          //!< Select blocks within a composite dataset by their flat index.
    BLOCK_SELECTORS, //!< Select datasets within a composite dataset using selector expressions.
    QUERY,           //!< Select entities with a text query.
    USER,            //!< Select entities with user-supplied, application-specific logic.
    NUM_CONTENT_TYPES
  };

  ///@{
  /**
   * Get or set the content type of the selection.
   * This is the same as setting the CONTENT_TYPE() key on the property.
   */
  virtual void SetContentType(int type);
  virtual int GetContentType();
  ///@}

  /**
   * Get the content type as a string.
   */
  static const char* GetContentTypeAsString(int type);

  /**
   * Controls whether cell, point, or field data determine what is inside and out.
   * The default is CELL.
   * Vertex and edge types are also available for graph classes.
   * GetFieldType() returns -1 if the field type is not set.
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* FIELD_TYPE();

  /// Indicate the types of entities to which the selection-data applies.
  enum SelectionField
  {
    CELL,   //!< The selection data provided is cell-data.
    POINT,  //!< The selection data provided is point-data.
    FIELD,  //!< The selection data provided is field-data.
    VERTEX, //!< The selection data provided is graph vertex-data.
    EDGE,   //!< The selection data provided is graph edge-data.
    ROW,    //!< The selection data provided is table row-data.
    NUM_FIELD_TYPES
  };

  ///@{
  /**
   * Get or set the field type of the selection.
   * This is the same as setting the FIELD_TYPE() key on the property.
   */
  virtual void SetFieldType(int type);
  virtual int GetFieldType();
  ///@}

  /**
   * Get the field type as a string.
   */
  static const char* GetFieldTypeAsString(int type);

  /**
   * Get field type from string. Returns NUM_FIELD_TYPES if not found.
   */
  static int GetFieldTypeFromString(const char* type);

  ///@{
  /**
   * Methods to convert vtkSelectionNode::SelectionField to
   * vtkDataSetAttribute::AttributeTypes and vice-versa.
   */
  static int ConvertSelectionFieldToAttributeType(int val);
  static int ConvertAttributeTypeToSelectionField(int val);
  ///@}

  ///@{
  /**
   * Set/Get the query expression string.
   */
  vtkSetStringMacro(QueryString);
  vtkGetStringMacro(QueryString);
  ///@}

  /**
   * For location selection of points, if distance is greater than this reject.
   * \ingroup InformationKeys
   */
  static vtkInformationDoubleKey* EPSILON();

  /**
   * If present, closest zbuffer value of this selection
   * \ingroup InformationKeys
   */
  static vtkInformationDoubleKey* ZBUFFER_VALUE();

  /**
   * This flag tells the extraction filter, when FIELD_TYPE==POINT, that
   * it should also extract the cells that contain any of the extracted points.
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* CONTAINING_CELLS();

  /**
   * When specified, this indicates how many layers of *connected* elements
   * in addition to those chosen explicitly are being selected. Currently,
   * this is only supported for cells and points.
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* CONNECTED_LAYERS();

  /**
   * When specified and also using CONNECTED_LAYERS(), this indicates
   * if the initial selection should be kept or not.
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* CONNECTED_LAYERS_REMOVE_SEED();

  /**
   * When specified and also using CONNECTED_LAYERS(), this indicates
   * if the intermediate layers should be kept or not.
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* CONNECTED_LAYERS_REMOVE_INTERMEDIATE_LAYERS();

  /**
   * When ContentType==THRESHOLDS  or ContentType==VALUES
   * i.e. threshold and value based selections, it is
   * possible pick the component number using this key. If none is specified,
   * the 0th component is used. If any number less than 0 is specified, then
   * the magnitude is used.
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* COMPONENT_NUMBER();

  /**
   * This flag tells the extraction filter to exclude the selection.
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* INVERSE();

  /**
   * A helper for visible cell selector, this is the number of pixels covered
   * by the actor whose cells are listed in the selection.
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* PIXEL_COUNT();

  /**
   * Pointer to the data or algorithm the selection belongs to.
   * \ingroup InformationKeys
   */
  static vtkInformationObjectBaseKey* SOURCE();

  /**
   * ID of the data or algorithm the selection belongs to. What
   * ID means is application specific.
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* SOURCE_ID();

  /**
   * Pointer to the prop the selection belongs to.
   * \ingroup InformationKeys
   */
  static vtkInformationObjectBaseKey* PROP();

  /**
   * ID of the prop the selection belongs to. What
   * ID means is application specific.
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* PROP_ID();

  /**
   * Process id the selection is on.
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* PROCESS_ID();

  ///@{
  /**
   * Keys for selector-based identification of
   * blocks to choose from a composite dataset. `ASSEMBLY_NAME` identifies the
   * name for the assembly to use and `SELECTORS` provide a list of node
   * selectors applied to the chosen assembly.
   *
   * Use `vtkDataAssemblyUtilities::HierarchyName` as the assembly name
   * to use the data hierarchy
   * \ingroup InformationKeys
   */
  static vtkInformationStringKey* ASSEMBLY_NAME();
  static vtkInformationStringVectorKey* SELECTORS();
  ///@}

  /**
   * Used to identify a node in composite datasets.
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* COMPOSITE_INDEX();

  ///@{
  /**
   * Used to identify a dataset in a hiererchical box dataset.
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* HIERARCHICAL_LEVEL();
  static vtkInformationIntegerKey* HIERARCHICAL_INDEX();
  ///@}

  ///@{
  /**
   * Used to identify a cell type and whether picked entity is a side in a vtkCellGrid.
   * 1. `CELLGRID_CELL_TYPE_INDEX` is an offset into vtkCellGrid::GetCellTypesArray()
   * 2. For cell spec, `CELLGRID_SOURCE_SPECIFICATION_INDEX` will be 0,
   * 3. For all other side specs, `CELLGRID_SOURCE_SPECIFICATION_INDEX` will take on values starting
   * from 1. NOTE: The cell/side spec correspond to `vtkDGCell::Source` objects found in
   * `vtkDGCell::GetCellSpec()` and `vtkDGCell::GetSideSpecs()`
   */
  static vtkInformationIntegerKey* CELLGRID_CELL_TYPE_INDEX();
  static vtkInformationIntegerKey* CELLGRID_SOURCE_SPECIFICATION_INDEX();
  ///@}

  /**
   * Merges the selection list between self and the other. Assumes that both has
   * identical properties.
   */
  void UnionSelectionList(vtkSelectionNode* other);

  /**
   * Subtracts the items in the selection list, other, from this selection list.
   * Assumes that both selections have identical properties (i.e., test with EqualProperties
   * before using).
   */
  void SubtractSelectionList(vtkSelectionNode* other);

  /**
   * Compares Properties of self and other to ensure that they are exactly same.
   */
  bool EqualProperties(vtkSelectionNode* other, bool fullcompare = true);

protected:
  vtkSelectionNode();
  ~vtkSelectionNode() override;

  vtkInformation* Properties;
  vtkDataSetAttributes* SelectionData;
  char* QueryString;

  // Map from content type to content type name
  static const char ContentTypeNames[SelectionContent::NUM_CONTENT_TYPES][16];

  // Map from integer field type to field type name
  static const char FieldTypeNames[SelectionField::NUM_FIELD_TYPES][8];

private:
  vtkSelectionNode(const vtkSelectionNode&) = delete;
  void operator=(const vtkSelectionNode&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
