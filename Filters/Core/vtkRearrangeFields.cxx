/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRearrangeFields.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRearrangeFields.h"

#include "vtkCellData.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include <ctype.h>

vtkStandardNewMacro(vtkRearrangeFields);

typedef vtkRearrangeFields::Operation Operation;

// Used by AddOperation() and RemoveOperation() designed to be used
// from other language bindings.
char vtkRearrangeFields::OperationTypeNames[2][5]
= { "COPY",
    "MOVE" };
char vtkRearrangeFields::FieldLocationNames[3][12]
= { "DATA_OBJECT",
    "POINT_DATA",
    "CELL_DATA" };
char vtkRearrangeFields::AttributeNames[vtkDataSetAttributes::NUM_ATTRIBUTES][10]  = { {0} };



//--------------------------------------------------------------------------


vtkRearrangeFields::vtkRearrangeFields()
{
  this->Head = 0;
  this->Tail = 0;
  this->LastId = 0;
  //convert the attribute names to uppercase for local use
  if (vtkRearrangeFields::AttributeNames[0][0] == 0)
    {
    for (int i = 0; i < vtkDataSetAttributes::NUM_ATTRIBUTES; i++)
      {
      int l = static_cast<int>(
        strlen(vtkDataSetAttributes::GetAttributeTypeAsString(i)));
      for (int c = 0; c < l && c < 10; c++)
        {
        vtkRearrangeFields::AttributeNames[i][c] =
          toupper(vtkDataSetAttributes::GetAttributeTypeAsString(i)[c]);
        }
      }
    }
}

vtkRearrangeFields::~vtkRearrangeFields()
{
  this->DeleteAllOperations();
}

int vtkRearrangeFields::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // This has to be here because it initialized all field datas.
  output->CopyStructure( input );

  // Apply all operations.
  Operation* cur = this->GetFirst();
  if (cur)
    {
    Operation* before;
    do
      {
      before = cur;
      cur = cur->Next;
      this->ApplyOperation(before, input, output);
      }
    while (cur);
    }

  // Pass all.
  if ( output->GetFieldData() && input->GetFieldData() )
    {
    output->GetFieldData()->PassData( input->GetFieldData() );
    }
  output->GetPointData()->PassData( input->GetPointData() );
  output->GetCellData()->PassData( input->GetCellData() );

  return 1;
}

// Given location (DATA_OBJECT, CELL_DATA, POINT_DATA) return the
// pointer to the corresponding field data.
vtkFieldData* vtkRearrangeFields::GetFieldDataFromLocation(vtkDataSet* ds,
                                                           int fieldLoc)
{
  vtkFieldData* fd=0;

  switch (fieldLoc)
    {
    case vtkRearrangeFields::DATA_OBJECT:
      fd = ds->GetFieldData();
      break;
    case vtkRearrangeFields::POINT_DATA:
      fd = ds->GetPointData();
      break;
    case vtkRearrangeFields::CELL_DATA:
      fd = ds->GetCellData();
      break;
    }
  return fd;
}

void vtkRearrangeFields::ApplyOperation(Operation* op, vtkDataSet* input,
                                        vtkDataSet* output)
{
  vtkDebugMacro("Applying operation: " << op->Id);

  // Get the field data corresponding to the operation
  // from input and output
  vtkFieldData* inputFD = this->GetFieldDataFromLocation(input,
                                                         op->FromFieldLoc);
  vtkFieldData* outputFD = this->GetFieldDataFromLocation(output,
                                                          op->ToFieldLoc);
  if ( !inputFD || !outputFD)
    {
    vtkWarningMacro("Can not apply operation " << op->Id
                    << ": Inappropriate input or output location"
                    << " specified for the operation.");
    return;
    }

  // If the source is specified by name
  if ( op->FieldType == vtkRearrangeFields::NAME )
    {
    vtkDebugMacro("Copy by name:" << op->FieldName);
    // Pass the array
    outputFD->AddArray(inputFD->GetArray(op->FieldName));
    // If moving the array, make sure that it is not copied
    // with PassData()
    if ( op->OperationType == vtkRearrangeFields::MOVE )
      {
      vtkFieldData* fd = this->GetFieldDataFromLocation(output,
                                                        op->FromFieldLoc);
      fd->CopyFieldOff(op->FieldName);
      }
    else if ( op->OperationType == vtkRearrangeFields::COPY )
      {
      }
    else
      {
      vtkWarningMacro("Can not apply operation " << op->Id
                      << ": Inappropriate operation type.");
      return;
      }
    }
  // If source is specified as attribute
  else if ( op->FieldType == vtkRearrangeFields::ATTRIBUTE )
    {
    vtkDebugMacro("Copy by attribute");
    // Get the attribute and pass it
    vtkDataSetAttributes* dsa = vtkDataSetAttributes::SafeDownCast(inputFD);
    if (!dsa)
      {
      vtkWarningMacro("Can not apply operation " << op->Id
                      << ": Input has to be vtkDataSetAttributes.");
      }
    outputFD->AddArray(dsa->GetAbstractAttribute(op->AttributeType));
    // If moving the array, make sure that it is not copied
    // with PassData()
    if ( op->OperationType == vtkRearrangeFields::MOVE )
      {
      vtkFieldData* fd = this->GetFieldDataFromLocation(output,
                                                        op->FromFieldLoc);
      vtkDataSetAttributes* dsa2 = vtkDataSetAttributes::SafeDownCast(fd);
      if (dsa2)
        {
        dsa2->SetCopyAttribute(op->AttributeType,0);
        }
      }
    else if ( op->OperationType == vtkRearrangeFields::COPY )
      {
      }
    else
      {
      vtkWarningMacro("Can not apply operation " << op->Id
                      << ": Inappropriate operation type.");
      return;
      }
    }
  else
    {
    vtkWarningMacro("Can not apply operation " << op->Id
                    << ": Inappropriate field type"
                    << " specified for the operation.");
    return;
    }
}

// Helper method used by the Tcl bindings. Allows the caller to
// specify arguments as strings instead of enums.Returns an operation id
// which can later be used to remove the operation.
int vtkRearrangeFields::AddOperation(const char* operationType,
                                     const char* name,
                                     const char* fromFieldLoc,
                                     const char* toFieldLoc)
{
  int numAttr = vtkDataSetAttributes::NUM_ATTRIBUTES;
  int numOpTypes = 2;
  int numFieldLocs = 3;
  int opType=-1, i;
  // Convert strings to ints and call the appropriate AddOperation()
  for(i=0; i<numOpTypes; i++)
    {
    if (!strcmp(operationType, OperationTypeNames[i]))
      {
      opType = i;
      break;
      }
    }
  if (opType == -1)
    {
    vtkErrorMacro("Syntax error in operation.");
    return -1;
    }
  int attributeType=-1;
  for(i=0; i<numAttr; i++)
    {
    if (!strcmp(name, AttributeNames[i]))
      {
      attributeType = i;
      break;
      }
    }

  int fromLoc=-1;
  for(i=0; i<numFieldLocs; i++)
    {
    if (!strcmp(fromFieldLoc, FieldLocationNames[i]))
      {
      fromLoc = i;
      break;
      }
    }
  if (fromLoc == -1)
    {
    vtkErrorMacro("Syntax error in operation.");
    return -1;
    }

  int toLoc=-1;
  for(i=0; i<numFieldLocs; i++)
    {
    if (!strcmp(toFieldLoc, FieldLocationNames[i]))
      {
      toLoc = i;
      break;
      }
    }
  if (toLoc == -1)
    {
    vtkErrorMacro("Syntax error in operation.");
    return -1;
    }

  if ( attributeType == -1 )
    {
    vtkDebugMacro("Adding operation with parameters: "
                  << opType << " " << name << " "
                  << fromLoc << " " << toLoc);
    return this->AddOperation(opType, name, fromLoc, toLoc);
    }
  else
    {
    vtkDebugMacro("Adding operation with parameters: "
                  << opType << " " << attributeType << " "
                  << fromLoc << " " << toLoc);
    return this->AddOperation(opType, attributeType, fromLoc, toLoc);
    }
}


int vtkRearrangeFields::AddOperation(int operationType, const char* name,
                                     int fromFieldLoc, int toFieldLoc)
{
  if (!name)
    { return -1; }

  // Syntax and sanity checks.
  if ( (operationType != vtkRearrangeFields::COPY) &&
       (operationType != vtkRearrangeFields::MOVE) )
    {
    vtkErrorMacro("Wrong operation type.");
    return -1;
    }
  if ( (fromFieldLoc !=  vtkRearrangeFields::DATA_OBJECT) &&
       (fromFieldLoc !=  vtkRearrangeFields::POINT_DATA) &&
       (fromFieldLoc !=  vtkRearrangeFields::CELL_DATA) )
    {
    vtkErrorMacro("The source for the field is wrong.");
    return -1;
    }
  if ( (toFieldLoc !=  vtkRearrangeFields::DATA_OBJECT) &&
       (toFieldLoc !=  vtkRearrangeFields::POINT_DATA) &&
       (toFieldLoc !=  vtkRearrangeFields::CELL_DATA) )
    {
    vtkErrorMacro("The source for the field is wrong.");
    return -1;
    }

  // Create an operation with the specified parameters.
  Operation* op = new Operation;
  op->OperationType = operationType;
  op->FieldName = new char [strlen(name)+1];
  strcpy(op->FieldName, name);
  op->FromFieldLoc = fromFieldLoc;
  op->ToFieldLoc = toFieldLoc;
  op->FieldType = vtkRearrangeFields::NAME;
  op->Id = this->LastId++;
  // assign this anyway
  op->AttributeType = 0;

  this->AddOperation(op);
  this->Modified();

  return op->Id;
}

int vtkRearrangeFields::AddOperation(int operationType, int attributeType,
                                     int fromFieldLoc, int toFieldLoc)
{

  // Syntax and sanity checks.
  if ( (operationType != vtkRearrangeFields::COPY) &&
       (operationType != vtkRearrangeFields::MOVE) )
    {
    vtkErrorMacro("Wrong operation type.");
    return -1;
    }
  if ( (fromFieldLoc !=  vtkRearrangeFields::DATA_OBJECT) &&
       (fromFieldLoc !=  vtkRearrangeFields::POINT_DATA) &&
       (fromFieldLoc !=  vtkRearrangeFields::CELL_DATA) )
    {
    vtkErrorMacro("The source for the field is wrong.");
    return -1;
    }
  if ( (attributeType < 0) ||
       (attributeType > vtkDataSetAttributes::NUM_ATTRIBUTES) )
    {
    vtkErrorMacro("Wrong attribute type.");
    return -1;
    }
  if ( (toFieldLoc !=  vtkRearrangeFields::DATA_OBJECT) &&
       (toFieldLoc !=  vtkRearrangeFields::POINT_DATA) &&
       (toFieldLoc !=  vtkRearrangeFields::CELL_DATA) )
    {
    vtkErrorMacro("The source for the field is wrong.");
    return -1;
    }

  // Create an operation with the specified parameters.
  Operation* op = new Operation;
  op->OperationType = operationType;
  op->AttributeType = attributeType;
  op->FromFieldLoc = fromFieldLoc;
  op->ToFieldLoc = toFieldLoc;
  op->FieldType = vtkRearrangeFields::ATTRIBUTE;
  op->Id = this->LastId++;

  this->AddOperation(op);
  this->Modified();

  return op->Id;
}

// Helper method used by the Tcl bindings. Allows the caller to
// specify arguments as strings instead of enums.Returns an operation id
// which can later be used to remove the operation.
int vtkRearrangeFields::RemoveOperation(const char* operationType,
                                        const char* name,
                                        const char* fromFieldLoc,
                                        const char* toFieldLoc)
{
  if (!operationType || !name || !fromFieldLoc || !toFieldLoc)
    {
    return 0;
    }

  int numAttr = vtkDataSetAttributes::NUM_ATTRIBUTES;
  int numOpTypes = 2;
  int numFieldLocs = 3;
  int opType=-1, i;
  // Convert strings to ints and call the appropriate AddOperation()
  for(i=0; i<numOpTypes; i++)
    {
    if (!strcmp(operationType, OperationTypeNames[i]))
      {
      opType = i;
      }
    }
  if (opType == -1)
    {
    vtkErrorMacro("Syntax error in operation.");
    return 0;
    }
  int attributeType=-1;
  for(i=0; i<numAttr; i++)
    {
    if (!strcmp(name, AttributeNames[i]))
      {
      attributeType = i;
      }
    }

  int fromLoc=-1;
  for(i=0; i<numFieldLocs; i++)
    {
    if (!strcmp(fromFieldLoc, FieldLocationNames[i]))
      {
      fromLoc = i;
      }
    }
  if (fromLoc == -1)
    {
    vtkErrorMacro("Syntax error in operation.");
    return 0;
    }

  int toLoc=-1;
  for(i=0; i<numFieldLocs; i++)
    {
    if (!strcmp(toFieldLoc, FieldLocationNames[i]))
      {
      toLoc = i;
      }
    }
  if (toLoc == -1)
    {
    vtkErrorMacro("Syntax error in operation.");
    return 0;
    }

  if ( attributeType == -1 )
    {
    vtkDebugMacro("Removing operation with parameters: "
                  << opType << " " << name << " "
                  << fromLoc << " " << toLoc);
    return this->RemoveOperation(opType, name, fromLoc, toLoc);
    }
  else
    {
    vtkDebugMacro("Removing operation with parameters: "
                  << opType << " " << attributeType << " "
                  << fromLoc << " " << toLoc);
    return this->RemoveOperation(opType, attributeType, fromLoc, toLoc);
    }
}

int vtkRearrangeFields::RemoveOperation(int operationId)
{
  Operation* before;
  Operation* op;

  op = this->FindOperation(operationId, before);
  if (!op) {return 0;}
  this->DeleteOperation(op, before);
  return 1;
}

int vtkRearrangeFields::RemoveOperation(int operationType, const char* name,
                                        int fromFieldLoc, int toFieldLoc)
{
  Operation* before;
  Operation* op;
  op = this->FindOperation(operationType, name, fromFieldLoc, toFieldLoc,
                           before);
  if (!op) { return 0;}
  this->Modified();
  this->DeleteOperation(op, before);
  return 1;
}

int vtkRearrangeFields::RemoveOperation(int operationType, int attributeType,
                                        int fromFieldLoc, int toFieldLoc)
{
  Operation* before;
  Operation* op;
  op = this->FindOperation(operationType, attributeType, fromFieldLoc,
                           toFieldLoc,  before);
  if (!op) { return 0;}
  this->Modified();
  this->DeleteOperation(op, before);
  return 1;
}

void vtkRearrangeFields::AddOperation(Operation* op)
{
  op->Next = 0;

  if (!this->Head)
    {
    this->Head = op;
    this->Tail = op;
    return;
    }
  this->Tail->Next = op;
  this->Tail = op;
}

void vtkRearrangeFields::DeleteOperation(Operation* op, Operation* before)
{
  if (!op) { return; }
  if (!before)
    {
    this->Head = op->Next;
    }
  else
    {
    before->Next = op->Next;
    if (!before->Next)
      {
      this->Tail=before;
      }
    }
  delete op;
}

Operation* vtkRearrangeFields::FindOperation(int id, Operation*& before)
{
  Operation* cur = this->GetFirst();
  if (!cur) { return 0; }

  before = 0;
  if (cur->Id == id) { return cur; }
  while (cur->Next)
    {
    before = cur;
    if (cur->Next->Id == id)
      {
      return cur->Next;
      }
    cur = cur->Next;
    }
  return 0;
}

Operation* vtkRearrangeFields::FindOperation(int operationType,
                                             const char* name,
                                             int fromFieldLoc,
                                             int toFieldLoc,
                                             Operation*& before)
{
  if (!name) {return 0;}

  Operation op;
  op.OperationType = operationType;
  op.FieldName = new char [strlen(name)+1];
  strcpy(op.FieldName, name);
  op.FromFieldLoc = fromFieldLoc;
  op.ToFieldLoc = toFieldLoc;

  Operation* cur = this->GetFirst();
  before = 0;
  if ( (cur->FieldType == vtkRearrangeFields::NAME) &&
       this->CompareOperationsByName(cur, &op)) { return cur; }
  while (cur->Next)
    {
    before = cur;
    if ( (cur->Next->FieldType == vtkRearrangeFields::NAME) &&
         this->CompareOperationsByName(cur->Next, &op))
      {
      return cur->Next;
      }
    cur = cur->Next;
    }
  return 0;
}

Operation* vtkRearrangeFields::FindOperation(int operationType,
                                             int attributeType,
                                             int fromFieldLoc,
                                             int toFieldLoc,
                                             Operation*& before)
{
  Operation op;
  op.OperationType = operationType;
  op.AttributeType = attributeType;
  op.FromFieldLoc = fromFieldLoc;
  op.ToFieldLoc = toFieldLoc;

  Operation* cur = this->GetFirst();
  before = 0;
  if ( (cur->FieldType == vtkRearrangeFields::ATTRIBUTE) &&
       this->CompareOperationsByType(cur, &op)) { return cur; }
  while (cur->Next)
    {
    before = cur;
    if ( (cur->Next->FieldType == vtkRearrangeFields::ATTRIBUTE) &&
         this->CompareOperationsByType(cur->Next, &op))
      {
      return cur->Next;
      }
    cur = cur->Next;
    }
  return 0;
}

void vtkRearrangeFields::DeleteAllOperations()
{
  Operation* cur = this->GetFirst();
  if (!cur) {return;}
  Operation* before;
  do
    {
    before = cur;
    cur = cur->Next;
    delete before;
    }
  while (cur);
  this->Head = 0;
  this->Tail = 0;
}

int vtkRearrangeFields::CompareOperationsByName(const Operation* op1,
                                                const Operation* op2)
{
  if ( op1->OperationType != op2->OperationType )
    { return 0; }
  if ( !op1->FieldName || !op2->FieldName || strcmp(op1->FieldName,
                                                    op2->FieldName) )
    { return 0; }
  if ( op1->FromFieldLoc != op2->FromFieldLoc )
    { return 0; }
  if ( op1->ToFieldLoc != op2->ToFieldLoc )
    { return 0; }
  return 1;
}

int vtkRearrangeFields::CompareOperationsByType(const Operation* op1,
                                                const Operation* op2)
{
  if ( op1->OperationType != op2->OperationType )
    { return 0; }
  if ( op1->AttributeType != op2->AttributeType )
    { return 0; }
  if ( op1->FromFieldLoc != op2->FromFieldLoc )
    { return 0; }
  if ( op1->ToFieldLoc != op2->ToFieldLoc )
    { return 0; }
  return 1;
}


void vtkRearrangeFields::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Linked list head: " << this->Head << endl;
  os << indent << "Linked list tail: " << this->Tail << endl;
  os << indent << "Last id: " << this->LastId << endl;
  os << indent << "Operations: " << endl;
  this->PrintAllOperations(os, indent.GetNextIndent());
}

void vtkRearrangeFields::PrintAllOperations(ostream& os, vtkIndent indent)
{
  Operation* cur = this->GetFirst();
  if (!cur) { return; }
  Operation* before;
  do
    {
    before = cur;
    cur = cur->Next;
    os << endl;
    this->PrintOperation(before, os, indent);
    }
  while (cur);
}

void vtkRearrangeFields::PrintOperation(Operation* op, ostream& os, vtkIndent indent)
{
  os << indent << "Id: " << op->Id << endl;
  os << indent << "Type: " << op->OperationType << endl;
  os << indent << "Field type: " << op->FieldType << endl;
  if ( op->FieldName)
    {
    os << indent << "Field name: " << op->FieldName << endl;
    }
  else
    {
    os << indent << "Field name: (none)" << endl;
    }
  os << indent << "Attribute type: " << op->AttributeType << endl;
  os << indent << "Source field location: " << op->FromFieldLoc << endl;
  os << indent << "Target field location: " << op->ToFieldLoc << endl;
  os << indent << "Next operation: " << op->Next << endl;
  os << endl;
}
