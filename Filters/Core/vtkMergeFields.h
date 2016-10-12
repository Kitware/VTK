/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeFields.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMergeFields
 * @brief   Merge multiple fields into one.
 *
 * vtkMergeFields is used to merge mutliple field into one.
 * The new field is put in the same field data as the original field.
 * For example
 * @verbatim
 * mf->SetOutputField("foo", vtkMergeFields::POINT_DATA);
 * mf->SetNumberOfComponents(2);
 * mf->Merge(0, "array1", 1);
 * mf->Merge(1, "array2", 0);
 * @endverbatim
 * will tell vtkMergeFields to use the 2nd component of array1 and
 * the 1st component of array2 to create a 2 component field called foo.
 * The same can be done using Tcl:
 * @verbatim
 * mf SetOutputField foo POINT_DATA
 * mf Merge 0 array1 1
 * mf Merge 1 array2 0
 *
 * Field locations: DATA_OBJECT, POINT_DATA, CELL_DATA
 * @endverbatim
 *
 * @sa
 * vtkFieldData vtkDataSet vtkDataObjectToDataSetFilter
 * vtkDataSetAttributes vtkDataArray vtkRearrangeFields
 * vtkSplitField vtkAssignAttribute
*/

#ifndef vtkMergeFields_h
#define vtkMergeFields_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class vtkDataArray;
class vtkFieldData;

class VTKFILTERSCORE_EXPORT vtkMergeFields : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkMergeFields,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Create a new vtkMergeFields.
   */
  static vtkMergeFields *New();

  /**
   * The output field will have the given name and it will be in
   * fieldLoc (the input fields also have to be in fieldLoc).
   */
  void SetOutputField(const char* name, int fieldLoc);

  /**
   * Helper method used by the other language bindings. Allows the caller to
   * specify arguments as strings instead of enums.Returns an operation id
   * which can later be used to remove the operation.
   */
  void SetOutputField(const char* name, const char* fieldLoc);

  /**
   * Add a component (arrayName,sourceComp) to the output field.
   */
  void Merge(int component, const char* arrayName, int sourceComp);

  //@{
  /**
   * Set the number of the components in the output field.
   * This has to be set before execution. Default value is 0.
   */
  vtkSetMacro(NumberOfComponents, int);
  vtkGetMacro(NumberOfComponents, int);
  //@}

  enum FieldLocations
  {
    DATA_OBJECT=0,
    POINT_DATA=1,
    CELL_DATA=2
  };

  struct Component
  {
    int Index;
    int SourceIndex;
    char* FieldName;
    Component* Next;   // linked list
    void SetName(const char* name)
    {
        delete[] this->FieldName;
        this->FieldName = 0;
        if (name)
        {
          size_t len = strlen(name)+1;
          this->FieldName = new char[len];
          strncpy(this->FieldName, name, len);
        }
    }
    Component() { FieldName = 0; }
    ~Component() { delete[] FieldName; }
  };

protected:

  enum FieldType
  {
    NAME,
    ATTRIBUTE
  };

  vtkMergeFields();
  ~vtkMergeFields() VTK_OVERRIDE;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

  char* FieldName;
  int FieldLocation;
  int NumberOfComponents;
  int OutputDataType;

  static char FieldLocationNames[3][12];


  int MergeArray(vtkDataArray* in, vtkDataArray* out, int inComp, int outComp);

  // Components are stored as a linked list.
  Component* Head;
  Component* Tail;

  // Methods to browse/modify the linked list.
  Component* GetNextComponent(Component* op)
    { return op->Next; }
  Component* GetFirst()
    { return this->Head; }
  void AddComponent(Component* op);
  Component* FindComponent(int index);
  void DeleteAllComponents();

  void PrintComponent(Component* op, ostream& os, vtkIndent indent);
  void PrintAllComponents(ostream& os, vtkIndent indent);
private:
  vtkMergeFields(const vtkMergeFields&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMergeFields&) VTK_DELETE_FUNCTION;
};

#endif


