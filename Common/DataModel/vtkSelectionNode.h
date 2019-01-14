/*=========================================================================

  Program:   ParaView
  Module:    vtkSelectionNode.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSelectionNode
 * @brief   A node in a selection tree. Used to store selection results.
 *
 *
 * vtkSelectionNode stores selection parameters for a selection
 * (or part of a selection). It stores a list of properties (in a vtkInformation)
 * and a list of selection values (in a vtkAbstractArray). The properties
 * provide information about what the selection values mean. For example the
 * CONTENT_TYPE property gives information about what is stored by the node.
 * If the CONTENT_TYPE is GLOBALIDS,
 * the SelectionList array should contain a list of cell or point ids, which
 * identify the particular cells or points that have matching values in the
 * GLOBALID vtkDataSetAttribute array. If the CONTENT_TYPE is PEDIGREEIDS, the
 * SelectionList array should contain a list of cell or point ids, which identify
 * the particular cells or points that have matching values in the PEDIGREEID
 * vtkDataSetAttribute array. The FIELD_TYPE property designates whether the
 * selection refers to cells or points.
 *
 * Usually, each node under the root is a selection from
 * one data object. SOURCE or SOURCE_ID properties point to this object. If
 * the selection was performed on a renderer, PROP or PROP_ID point to the
 * prop the selection was made on. Selection nodes corresponding to
 * composite datasets may contain child nodes. Each child node of a
 * composite dataset should have COMPOSITE_INDEX set. This is the flat-index to
 * identify a node with in the composite dataset to which the selection applies.
 *
 * @warning
 * No SelectionList is created by default. It should be assigned.
*/

#ifndef vtkSelectionNode_h
#define vtkSelectionNode_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

class vtkAbstractArray;
class vtkDataSetAttributes;
class vtkInformation;
class vtkInformationDoubleKey;
class vtkInformationIntegerKey;
class vtkInformationObjectBaseKey;
class vtkProp;
class vtkTable;

class VTKCOMMONDATAMODEL_EXPORT vtkSelectionNode : public vtkObject
{
public:
  vtkTypeMacro(vtkSelectionNode,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkSelectionNode* New();

  /**
   * Restore data object to initial state,
   */
  virtual void Initialize();

  //@{
  /**
   * Sets the selection list.
   */
  virtual void SetSelectionList(vtkAbstractArray*);
  virtual vtkAbstractArray* GetSelectionList();
  //@}

  //@{
  /**
   * Sets the selection table.
   */
  virtual void SetSelectionData(vtkDataSetAttributes* data);
  vtkGetObjectMacro(SelectionData, vtkDataSetAttributes);
  //@}

  //@{
  /**
   * Returns the property map.
   */
  vtkGetObjectMacro(Properties, vtkInformation);
  //@}

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
   */
  static vtkInformationIntegerKey* CONTENT_TYPE();

  /**
   * Indicate the means by which data is selected.
   * In some cases this implies the type of data selected.
   */
  enum SelectionContent
  {
    SELECTIONS,   //!< Deprecated.
    GLOBALIDS,    //!< Select entities called out by their globally-unique IDs.
    PEDIGREEIDS,  //!< Select entities that have some identifiable pedigree.
    VALUES,       //!< Select entities that take on specific array values.
    INDICES,      //!< Select entities by their offsets into the dataset.
    FRUSTUM,      //!< Select entities contained within a viewing frustum.
    LOCATIONS,    //!< Select entities near the supplied world coordinates.
    THRESHOLDS,   //!< Select entities whose array values fall within a given threshold.
    BLOCKS,       //!< Select blocks within a composite dataset by their flat index.
    QUERY,        //!< Select entities with a text query.
    USER          //!< Select entities with user-supplied, application-specific logic.
  };

  //@{
  /**
   * Get or set the content type of the selection.
   * This is the same as setting the CONTENT_TYPE() key on the property.
   */
  virtual void SetContentType(int type);
  virtual int GetContentType();
  //@}

  /**
   * Controls whether cell, point, or field data determine what is inside and out.
   * The default is CELL.
   * Vertex and edge types are also available for graph classes.
   * GetFieldType() returns -1 if the field type is not set.
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
    ROW     //!< The selection data provided is table row-data.
  };

  //@{
  /**
   * Get or set the field type of the selection.
   * This is the same as setting the FIELD_TYPE() key on the property.
   */
  virtual void SetFieldType(int type);
  virtual int GetFieldType();
  //@}

  //@{
  /**
   * Methods to convert vtkSelectionNode::SelectionField to
   * vtkDataSetAttribute::AttributeTypes and vice-versa.
   */
  static int ConvertSelectionFieldToAttributeType(int val);
  static int ConvertAttributeTypeToSelectionField(int val);
  //@}

  //@{
  /**
   * Set/Get the query expression string.
   */
  vtkSetStringMacro(QueryString);
  vtkGetStringMacro(QueryString);
  //@}

  /**
   * For location selection of points, if distance is greater than this reject.
   */
  static vtkInformationDoubleKey* EPSILON();

  /**
   * If present, closest zbuffer value of this selection
   */
  static vtkInformationDoubleKey* ZBUFFER_VALUE();

  /**
   * This flag tells the extraction filter, when FIELD_TYPE==POINT, that
   * it should also extract the cells that contain any of the extracted points.
   */
  static vtkInformationIntegerKey* CONTAINING_CELLS();

  /**
   * When ContentType==THRESHOLDS  or ContentType==VALUES
   * i.e. threshold and value based selections, it is
   * possible pick the component number using this key. If none is specified,
   * the 0th component is used. If any number less than 0 is specified, then
   * the magnitude is used.
   */
  static vtkInformationIntegerKey* COMPONENT_NUMBER();

  /**
   * This flag tells the extraction filter to exclude the selection.
   */
  static vtkInformationIntegerKey* INVERSE();

  /**
   * A helper for visible cell selector, this is the number of pixels covered
   * by the actor whose cells are listed in the selection.
   */
  static vtkInformationIntegerKey* PIXEL_COUNT();

  /**
   * Pointer to the data or algorithm the selection belongs to.
   */
  static vtkInformationObjectBaseKey* SOURCE();

  /**
   * ID of the data or algorithm the selection belongs to. What
   * ID means is application specific.
   */
  static vtkInformationIntegerKey* SOURCE_ID();

  /**
   * Pointer to the prop the selection belongs to.
   */
  static vtkInformationObjectBaseKey* PROP();

  /**
   * ID of the prop the selection belongs to. What
   * ID means is application specific.
   */
  static vtkInformationIntegerKey* PROP_ID();

  /**
   * Process id the selection is on.
   */
  static vtkInformationIntegerKey* PROCESS_ID();

  /**
   * Used to identify a node in composite datasets.
   */
  static vtkInformationIntegerKey* COMPOSITE_INDEX();

  //@{
  /**
   * Used to identify a dataset in a hiererchical box dataset.
   */
  static vtkInformationIntegerKey* HIERARCHICAL_LEVEL();
  static vtkInformationIntegerKey* HIERARCHICAL_INDEX();
  //@}

  /**
   * This key is used when making visible vertex selection. It means
   * that the cell ID selection has data about which vertices for each
   * cell are visible.
   */
  static vtkInformationIntegerKey* INDEXED_VERTICES();

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
  bool EqualProperties(vtkSelectionNode* other, bool fullcompare=true);

protected:
  vtkSelectionNode();
  ~vtkSelectionNode() override;

  vtkInformation* Properties;
  vtkDataSetAttributes* SelectionData;
  char* QueryString;

private:
  vtkSelectionNode(const vtkSelectionNode&) = delete;
  void operator=(const vtkSelectionNode&) = delete;

};

#endif
