/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenerateIndexArray.h

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
 * @class   vtkGenerateIndexArray
 *
 *
 * Generates a new vtkIdTypeArray containing zero-base indices.
 *
 * vtkGenerateIndexArray operates in one of two distinct "modes".
 * By default, it simply generates an index array containing
 * monotonically-increasing integers in the range [0, N), where N
 * is appropriately sized for the field type that will store the
 * results.  This mode is useful for generating a unique ID field
 * for datasets that have none.
 *
 * The second "mode" uses an existing array from the input data
 * object as a "reference".  Distinct values from the reference
 * array are sorted in ascending order, and an integer index in
 * the range [0, N) is assigned to each.  The resulting map is
 * used to populate the output index array, mapping each value
 * in the reference array to its corresponding index and storing
 * the result in the output array.  This mode is especially
 * useful when generating tensors, since it allows us to "map"
 * from an array with arbitrary contents to an index that can
 * be used as tensor coordinates.
*/

#ifndef vtkGenerateIndexArray_h
#define vtkGenerateIndexArray_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkDataObjectAlgorithm.h"

class VTKINFOVISCORE_EXPORT vtkGenerateIndexArray : public vtkDataObjectAlgorithm
{
public:
  static vtkGenerateIndexArray *New();

  vtkTypeMacro(vtkGenerateIndexArray, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Control the output index array name.  Default: "index".
   */
  vtkSetStringMacro(ArrayName);
  vtkGetStringMacro(ArrayName);
  //@}

  //@{
  /**
   * Control the location where the index array will be stored.
   */
  vtkSetMacro(FieldType, int);
  vtkGetMacro(FieldType, int);
  //@}

  //@{
  /**
   * Specifies an optional reference array for index-generation.
   */
  vtkSetStringMacro(ReferenceArrayName);
  vtkGetStringMacro(ReferenceArrayName);
  //@}

  //@{
  /**
   * Specifies whether the index array should be marked as
   * pedigree ids.  Default: false.
   */
  vtkSetMacro(PedigreeID, int);
  vtkGetMacro(PedigreeID, int);
  //@}

  enum
  {
    ROW_DATA = 0,
    POINT_DATA = 1,
    CELL_DATA = 2,
    VERTEX_DATA = 3,
    EDGE_DATA = 4
  };

protected:
  vtkGenerateIndexArray();
  ~vtkGenerateIndexArray() override;

  int ProcessRequest(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestDataObject(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) override;

  char* ArrayName;
  int FieldType;
  char* ReferenceArrayName;
  int PedigreeID;

private:
  vtkGenerateIndexArray(const vtkGenerateIndexArray&) = delete;
  void operator=(const vtkGenerateIndexArray&) = delete;
};

#endif

