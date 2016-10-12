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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   vtkStringToNumeric
 * @brief   Converts string arrays to numeric arrays
 *
 *
 * vtkStringToNumeric is a filter for converting a string array
 * into a numeric arrays.
*/

#ifndef vtkStringToNumeric_h
#define vtkStringToNumeric_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkDataObjectAlgorithm.h"

class VTKINFOVISCORE_EXPORT vtkStringToNumeric : public vtkDataObjectAlgorithm
{
public:
  static vtkStringToNumeric* New();
  vtkTypeMacro(vtkStringToNumeric,vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Convert all numeric columns to vtkDoubleArray, even if they
   * contain only integer values. Default is off.
   */
  vtkSetMacro(ForceDouble, bool);
  vtkGetMacro(ForceDouble, bool);
  vtkBooleanMacro(ForceDouble, bool);
  //@}

  //@{
  /**
   * Set the default integer value assigned to arrays.  Default is 0.
   */
  vtkSetMacro(DefaultIntegerValue, int);
  vtkGetMacro(DefaultIntegerValue, int);
  //@}

  //@{
  /**
   * Set the default double value assigned to arrays.  Default is 0.0
   */
  vtkSetMacro(DefaultDoubleValue, double);
  vtkGetMacro(DefaultDoubleValue, double);
  //@}

  //@{
  /**
   * Whether to trim whitespace from strings prior to conversion to a numeric.
   * Default is false to preserve backward compatibility.

   * vtkVariant handles whitespace inconsistently, so trim it before we try to
   * convert it.  For example:

   * vtkVariant("  2.0").ToDouble() == 2.0 <-- leading whitespace is not a problem
   * vtkVariant("  2.0  ").ToDouble() == NaN <-- trailing whitespace is a problem
   * vtkVariant("  infinity  ").ToDouble() == NaN <-- any whitespace is a problem

   * In these cases, trimming the whitespace gives us the result we expect:
   * 2.0 and INF respectively.
   */
  vtkSetMacro(TrimWhitespacePriorToNumericConversion, bool);
  vtkGetMacro(TrimWhitespacePriorToNumericConversion, bool);
  vtkBooleanMacro(TrimWhitespacePriorToNumericConversion, bool);
  //@}

  //@{
  /**
   * Whether to detect and convert field data arrays.  Default is on.
   */
  vtkSetMacro(ConvertFieldData, bool);
  vtkGetMacro(ConvertFieldData, bool);
  vtkBooleanMacro(ConvertFieldData, bool);
  //@}

  //@{
  /**
   * Whether to detect and convert cell data arrays.  Default is on.
   */
  vtkSetMacro(ConvertPointData, bool);
  vtkGetMacro(ConvertPointData, bool);
  vtkBooleanMacro(ConvertPointData, bool);
  //@}

  //@{
  /**
   * Whether to detect and convert point data arrays.  Default is on.
   */
  vtkSetMacro(ConvertCellData, bool);
  vtkGetMacro(ConvertCellData, bool);
  vtkBooleanMacro(ConvertCellData, bool);
  //@}

  /**
   * Whether to detect and convert vertex data arrays.  Default is on.
   */
  virtual void SetConvertVertexData(bool b)
    { this->SetConvertPointData(b); }
  virtual bool GetConvertVertexData()
    { return this->GetConvertPointData(); }
  vtkBooleanMacro(ConvertVertexData, bool);

  /**
   * Whether to detect and convert edge data arrays.  Default is on.
   */
  virtual void SetConvertEdgeData(bool b)
    { this->SetConvertCellData(b); }
  virtual bool GetConvertEdgeData()
    { return this->GetConvertCellData(); }
  vtkBooleanMacro(ConvertEdgeData, bool);

  /**
   * Whether to detect and convert row data arrays.  Default is on.
   */
  virtual void SetConvertRowData(bool b)
    { this->SetConvertPointData(b); }
  virtual bool GetConvertRowData()
    { return this->GetConvertPointData(); }
  vtkBooleanMacro(ConvertRowData, bool);

  /**
   * This is required to capture REQUEST_DATA_OBJECT requests.
   */
  virtual int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inputVector,
                             vtkInformationVector* outputVector);

protected:
  vtkStringToNumeric();
  ~vtkStringToNumeric();

  /**
   * Creates the same output type as the input type.
   */
  virtual int RequestDataObject(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector);

  /**
   * Tries to convert string arrays to integer or double arrays.
   */
  void ConvertArrays(vtkFieldData* fieldData);

  bool ConvertFieldData;
  bool ConvertPointData;
  bool ConvertCellData;
  bool ForceDouble;
  int DefaultIntegerValue;
  double DefaultDoubleValue;
  bool TrimWhitespacePriorToNumericConversion;

  /**
   * Count the total number of items (array components) that will need
   * to be converted in the given vtkFieldData.  This lets us emit
   * ProgressEvent.
   */
  int CountItemsToConvert(vtkFieldData *fieldData);

  // These keep track of our progress
  int ItemsToConvert;
  int ItemsConverted;

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

private:
  vtkStringToNumeric(const vtkStringToNumeric&) VTK_DELETE_FUNCTION;
  void operator=(const vtkStringToNumeric&) VTK_DELETE_FUNCTION;
};

#endif

