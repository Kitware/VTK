/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTimePointToString.h

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
 * @class   vtkTimePointToString
 * @brief   Converts a timestamp array to a string array
 *
 *
 *
 * vtkTimePointToString is a filter for converting a timestamp array
 * into string array using one of the formats defined in vtkTimePointUtility.h.
 *
 * Use SetInputArrayToProcess to indicate the array to process.
 * This array must be an unsigned 64-bit integer array for
 * DATETIME formats, and may be either an unsigned 32-bit or
 * unsigned 64-bit array for DATE and TIME formats.
 *
 * If the new array name is not specified, the array name will be
 * the old name appended by " [to string]".
*/

#ifndef vtkTimePointToString_h
#define vtkTimePointToString_h

#include "vtkDataObjectAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkTimePointToString : public vtkDataObjectAlgorithm
{
public:
  static vtkTimePointToString* New();
  vtkTypeMacro(vtkTimePointToString,vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * The format to use when converting the timestamp to a string.
   */
  vtkSetMacro(ISO8601Format, int);
  vtkGetMacro(ISO8601Format, int);
  //@}

  //@{
  /**
   * The name of the output array.
   * If this is not specified, the name will be the input array name with
   * " [to string]" appended to it.
   */
  vtkSetStringMacro(OutputArrayName);
  vtkGetStringMacro(OutputArrayName);
  //@}

  /**
   * This is required to capture REQUEST_DATA_OBJECT requests.
   */
  virtual int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inputVector,
                             vtkInformationVector* outputVector);

protected:
  vtkTimePointToString();
  ~vtkTimePointToString();

  /**
   * Creates the same output type as the input type.
   */
  virtual int RequestDataObject(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector);

  int ISO8601Format;
  char* OutputArrayName;

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

private:
  vtkTimePointToString(const vtkTimePointToString&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTimePointToString&) VTK_DELETE_FUNCTION;
};

#endif

