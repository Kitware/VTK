/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPassArrays.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkPassArrays - Passes a subset of arrays to the output
//
// .SECTION Description
// This filter preserves all the topology of the input, but only a subset of
// arrays are passed to the output. Add an array to be passed to the output
// data object with AddArray(). If RemoveArrays is on, the specified arrays will
// be the ones that are removed instead of the ones that are kept.
//
// Arrays with special attributes (scalars, pedigree ids, etc.) will retain those
// attributes in the output.
//
// By default, only those field types with at least one array specified through
// AddArray will be processed. If instead UseFieldTypes
// is turned on, you explicitly set which field types to process with AddFieldType.
//
// Example 1:
//
// <pre>
// passArray->AddArray(vtkDataObject::POINT, "velocity");
// </pre>
//
// The output will have only that one array "velocity" in the
// point data, but cell and field data will be untouched.
//
// Example 2:
//
// <pre>
// passArray->AddArray(vtkDataObject::POINT, "velocity");
// passArray->UseFieldTypesOn();
// passArray->AddFieldType(vtkDataObject::POINT);
// passArray->AddFieldType(vtkDataObject::CELL);
// </pre>
//
// The point data would still contain the single array, but the cell data
// would be cleared since you did not specify any arrays to pass. Field data would
// still be untouched.

#ifndef __vtkPassArrays_h
#define __vtkPassArrays_h

#include "vtkDataObjectAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkPassArrays : public vtkDataObjectAlgorithm
{
public:
  static vtkPassArrays* New();
  vtkTypeMacro(vtkPassArrays,vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Adds an array to pass through.
  // fieldType where the array that should be passed (point data, cell data, etc.).
  // It should be one of the constants defined in the vtkDataObject::AttributeTypes
  // enumeration.
  virtual void AddArray(int fieldType, const char* name);
  
  // Description:
  // Clear all arrays to pass through.
  virtual void ClearArrays();
  
  // Description:
  // Instead of passing only the specified arrays, remove the specified arrays
  // and keep all other arrays. Default is off.
  vtkSetMacro(RemoveArrays, bool);
  vtkGetMacro(RemoveArrays, bool);
  vtkBooleanMacro(RemoveArrays, bool);

  // Description:
  // Process only those field types explicitly specified with AddFieldType.
  // Otherwise, processes field types associated with at least one specified
  // array. Default is off.
  vtkSetMacro(UseFieldTypes, bool);
  vtkGetMacro(UseFieldTypes, bool);
  vtkBooleanMacro(UseFieldTypes, bool);

  // Description:
  // Add a field type to process.
  // fieldType where the array that should be passed (point data, cell data, etc.).
  // It should be one of the constants defined in the vtkDataObject::AttributeTypes
  // enumeration.
  // NOTE: These are only used if UseFieldType is turned on.
  virtual void AddFieldType(int fieldType);

  // Description:
  // Clear all field types to process.
  virtual void ClearFieldTypes();

  // Description:
  // This is required to capture REQUEST_DATA_OBJECT requests.
  virtual int ProcessRequest(vtkInformation* request, 
                             vtkInformationVector** inputVector,
                             vtkInformationVector* outputVector);

protected:
  vtkPassArrays();
  ~vtkPassArrays();

  // Description:
  // Creates the same output type as the input type.
  virtual int RequestDataObject(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector);
  
  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

  bool RemoveArrays;
  bool UseFieldTypes;

  //BTX
  class Internals;
  Internals* Implementation;
  //ETX
    
private:
  vtkPassArrays(const vtkPassArrays&); // Not implemented
  void operator=(const vtkPassArrays&);   // Not implemented
};

#endif

