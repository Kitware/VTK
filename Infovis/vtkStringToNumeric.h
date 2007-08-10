/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStringToNumeric.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkStringToNumeric - Converts string arrays to numeric arrays
//
// .SECTION Description
// vtkStringToNumeric is a filter for converting a string array
// into a numeric arrays.

#ifndef __vtkStringToNumeric_h
#define __vtkStringToNumeric_h

#include "vtkDataObjectAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkStringToNumeric : public vtkDataObjectAlgorithm
{
public:
  static vtkStringToNumeric* New();
  vtkTypeRevisionMacro(vtkStringToNumeric,vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Whether to detect and convert field data arrays.  Default is on.
  vtkSetMacro(ConvertFieldData, bool);
  vtkGetMacro(ConvertFieldData, bool);
  vtkBooleanMacro(ConvertFieldData, bool);
  
  // Description:
  // Whether to detect and convert cell data arrays.  Default is on.
  vtkSetMacro(ConvertPointData, bool);
  vtkGetMacro(ConvertPointData, bool);
  vtkBooleanMacro(ConvertPointData, bool);
  
  // Description:
  // Whether to detect and convert point data arrays.  Default is on.
  vtkSetMacro(ConvertCellData, bool);
  vtkGetMacro(ConvertCellData, bool);
  vtkBooleanMacro(ConvertCellData, bool);

  // Description:
  // This is required to capture REQUEST_DATA_OBJECT requests.
  virtual int ProcessRequest(vtkInformation* request, 
                             vtkInformationVector** inputVector,
                             vtkInformationVector* outputVector);

protected:
  vtkStringToNumeric();
  ~vtkStringToNumeric();

  // Description:
  // Creates the same output type as the input type.
  virtual int RequestDataObject(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector);
  
  // Description:
  // Tries to convert string arrays to integer or double arrays.
  void ConvertArrays(vtkFieldData* fieldData);
  
  bool ConvertFieldData;
  bool ConvertPointData;
  bool ConvertCellData;

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);
    
private:
  vtkStringToNumeric(const vtkStringToNumeric&); // Not implemented
  void operator=(const vtkStringToNumeric&);   // Not implemented
};

#endif

