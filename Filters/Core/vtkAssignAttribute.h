// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAssignAttribute
 * @brief   Labels/marks a field as an attribute
 *
 * vtkAssignAttribute is used to label/mark a field (vtkDataArray) as an attribute.
 * A field name or an attribute to labeled can be specified. For example:
 * @verbatim
 * aa->Assign("foo", vtkDataSetAttributes::SCALARS,
 *            vtkAssignAttribute::POINT_DATA);
 * @endverbatim
 * tells vtkAssignAttribute to make the array in the point data called
 * "foo" the active scalars. On the other hand,
 * @verbatim
 * aa->Assign(vtkDataSetAttributes::VECTORS, vtkDataSetAttributes::SCALARS,
 *            vtkAssignAttribute::POINT_DATA);
 * @endverbatim
 * tells vtkAssignAttribute to make the active vectors also the active
 * scalars.
 *
 * @warning
 * When using Java, Python or Visual Basic bindings, the array name
 * can not be one of the  AttributeTypes when calling Assign() which takes
 * strings as arguments. The wrapped command will
 * always assume the string corresponds to an attribute type when
 * the argument is one of the AttributeTypes. In this situation,
 * use the Assign() which takes enums.
 *
 * @sa
 * vtkFieldData vtkDataSet vtkDataObjectToDataSetFilter
 * vtkDataSetAttributes vtkDataArray vtkRearrangeFields
 * vtkSplitField vtkMergeFields
 */

#ifndef vtkAssignAttribute_h
#define vtkAssignAttribute_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

#include "vtkDataSetAttributes.h" // Needed for NUM_ATTRIBUTES

VTK_ABI_NAMESPACE_BEGIN
class vtkFieldData;

class VTKFILTERSCORE_EXPORT vtkAssignAttribute : public vtkPassInputTypeAlgorithm
{
public:
  vtkTypeMacro(vtkAssignAttribute, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Create a new vtkAssignAttribute.
   */
  static vtkAssignAttribute* New();

  /**
   * Label an attribute as another attribute.
   */
  void Assign(int inputAttributeType, int attributeType, int attributeLoc);

  /**
   * Label an array as an attribute.
   */
  void Assign(const char* fieldName, int attributeType, int attributeLoc);

  /**
   * Helper method used by other language bindings. Allows the caller to
   * specify arguments as strings instead of enums.
   */
  void Assign(const char* name, const char* attributeType, const char* attributeLoc);

  // Always keep NUM_ATTRIBUTE_LOCS as the last entry
  enum AttributeLocation
  {
    POINT_DATA = 0,
    CELL_DATA = 1,
    VERTEX_DATA = 2,
    EDGE_DATA = 3,
    NUM_ATTRIBUTE_LOCS
  };

protected:
  enum FieldType
  {
    NAME,
    ATTRIBUTE
  };

  vtkAssignAttribute();
  ~vtkAssignAttribute() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

  char* FieldName;
  int FieldTypeAssignment;
  int AttributeType;
  int InputAttributeType;
  int AttributeLocationAssignment;

  static char AttributeLocationNames[vtkAssignAttribute::NUM_ATTRIBUTE_LOCS][12];
  static char AttributeNames[vtkDataSetAttributes::NUM_ATTRIBUTES][20];

private:
  vtkAssignAttribute(const vtkAssignAttribute&) = delete;
  void operator=(const vtkAssignAttribute&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
