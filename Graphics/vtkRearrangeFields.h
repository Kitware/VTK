/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRearrangeFields.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRearrangeFields - Move/copy fields between field data, point data and cell data
// .SECTION Description
// vtkRearrangeFields is used to copy/move fields (vtkDataArrays) between
// data object's field data, point data and cell data. To specify which
// fields are copied/moved, the user adds operations. There are two types
// of operations: 1. the type which copies/moves an attribute's data
// (i.e. the field will be copied but will not be an attribute in the
// target), 2. the type which copies/moves fields by name. For example:
// @verbatim
// rf->AddOperation(vtkRearrangeFields::COPY, "foo", 
//                  vtkRearrangeFields::DATA_OBJECT, 
//                  vtkRearrangeFields::POINT_DATA);
// @endverbatim
// adds an operation which copies a field (data array) called foo from
// the data object's field data to point data.
// From Tcl, the same operation can be added as follows:
// @verbatim
// rf AddOperation COPY foo DATA_OBJECT POINT_DATA
// @endverbatim
// The same can be done using Python and Java bindings by passing
// strings as arguments.
// @verbatim
// Operation types: COPY, MOVE
// AttributeTypes: SCALARS, VECTORS, NORMALS, TCOORDS, TENSORS
// Field data locations: DATA_OBJECT, POINT_DATA, CELL_DATA
// @endverbatim

// .SECTION Caveats
// When using Tcl, Java, Python or Visual Basic bindings, the array name 
// can not be one of the  AttributeTypes when calling AddOperation() which 
// takes strings as arguments. The Tcl (Java etc.) command will
// always assume the string corresponds to an attribute type when
// the argument is one of the AttributeTypes. In this situation,
// use the AddOperation() which takes enums.

// .SECTION See Also
// vtkFieldData vtkDataSet vtkDataObjectToDataSetFilter
// vtkDataSetAttributes vtkDataArray vtkAssignAttribute
// vtkSplitField vtkMergeFields

#ifndef __vtkRearrangeFields_h
#define __vtkRearrangeFields_h

#include "vtkDataSetAlgorithm.h"

#include "vtkDataSetAttributes.h" // Needed for NUM_ATTRIBUTES

class vtkFieldData;

class VTK_GRAPHICS_EXPORT vtkRearrangeFields : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkRearrangeFields,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a new vtkRearrangeFields with an empty operation list.
  static vtkRearrangeFields *New();

//BTX
  enum OperationType
  {
    COPY=0,
    MOVE=1
  };
  enum FieldLocation
  {
    DATA_OBJECT=0,
    POINT_DATA=1,
    CELL_DATA=2
  };
//ETX

  // Description:
  // Add an operation which copies an attribute's field (data array) from
  // one field data to another. Returns an operation id which can later
  // be used to remove the operation.
  int AddOperation(int operationType, int attributeType, int fromFieldLoc,
                   int toFieldLoc);
  // Description:
  // Add an operation which copies a field (data array) from one field 
  // data to another. Returns an operation id which can later
  // be used to remove the operation.
  int AddOperation(int operationType, const char* name, int fromFieldLoc,
                   int toFieldLoc);
  // Description:
  // Helper method used by other language bindings. Allows the caller to
  // specify arguments as strings instead of enums.Returns an operation id 
  // which can later be used to remove the operation.
  int AddOperation(const char* operationType, const char* attributeType,
                   const char* fromFieldLoc,  const char* toFieldLoc);

  // Description:
  // Remove an operation with the given id.
  int RemoveOperation(int operationId);
  // Description:
  // Remove an operation with the given signature. See AddOperation
  // for details.
  int RemoveOperation(int operationType, int attributeType, int fromFieldLoc,
                      int toFieldLoc);
  // Description:
  // Remove an operation with the given signature. See AddOperation
  // for details.
  int RemoveOperation(int operationType, const char* name, int fromFieldLoc,
                      int toFieldLoc);
  // Description:
  // Remove an operation with the given signature. See AddOperation
  // for details.
  int RemoveOperation(const char* operationType, const char* attributeType,
                      const char* fromFieldLoc,  const char* toFieldLoc);

  // Description:
  // Remove all operations.
  void RemoveAllOperations() 
    { 
    this->Modified();
    this->LastId = 0; 
    this->DeleteAllOperations(); 
    }
  
//BTX
  enum FieldType
  {
    NAME,
    ATTRIBUTE
  };

  struct Operation
  {
    int OperationType; // COPY or MOVE
    int FieldType;     // NAME or ATTRIBUTE
    char* FieldName;   
    int AttributeType;
    int FromFieldLoc; // fd, pd or do
    int ToFieldLoc;   // fd, pd or do
    int Id;            // assigned during creation
    Operation* Next;   // linked list
    Operation() { FieldName = 0; }
    ~Operation() { delete[] FieldName; }
  };
//ETX

protected:

  vtkRearrangeFields();
  virtual ~vtkRearrangeFields();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);


  // Operations are stored as a linked list.
  Operation* Head;
  Operation* Tail;
  // This is incremented whenever a new operation is created.
  // It is not decremented when an operation is deleted.
  int LastId;

  // Methods to browse/modify the linked list.
  Operation* GetNextOperation(Operation* op)
    { return op->Next; }
  Operation* GetFirst()
    { return this->Head; }
  void AddOperation(Operation* op);
  void DeleteOperation(Operation* op, Operation* before);
  Operation* FindOperation(int id, Operation*& before);
  Operation* FindOperation(const char* name, Operation*& before);
  Operation* FindOperation(int operationType, const char* name, 
                           int fromFieldLoc, int toFieldLoc,
                           Operation*& before);
  Operation* FindOperation(int operationType, int attributeType, 
                           int fromFieldLoc, int toFieldLoc,
                           Operation*& before);
  // Used when finding/deleting an operation given a signature.
  int CompareOperationsByType(const Operation* op1, const Operation* op2);
  int CompareOperationsByName(const Operation* op1, const Operation* op2);

  void DeleteAllOperations();
  void ApplyOperation(Operation* op, vtkDataSet* input, vtkDataSet* output);
  // Given location (DATA_OBJECT, CELL_DATA, POINT_DATA) return the
  // pointer to the corresponding field data.
  vtkFieldData* GetFieldDataFromLocation(vtkDataSet* ds, int fieldLoc);

  // Used by AddOperation() and RemoveOperation() designed to be used 
  // from other language bindings.
  static char OperationTypeNames[2][5];
  static char FieldLocationNames[3][12];
  static char AttributeNames[vtkDataSetAttributes::NUM_ATTRIBUTES][10];

  void PrintAllOperations(ostream& os, vtkIndent indent);
  void PrintOperation(Operation* op, ostream& os, vtkIndent indent);
private:
  vtkRearrangeFields(const vtkRearrangeFields&);  // Not implemented.
  void operator=(const vtkRearrangeFields&);  // Not implemented.
};

#endif


