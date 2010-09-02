/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTokenValueFilter.h

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

// .NAME vtkTokenValueFilter - Filters tokens based on their value.
//
// .SECTION Description
// vtkTokenValueFilter removes tokens from the pipeline based on whether they match a list
// of token values.  Typically, this is useful for handling lists of "stop words" that should
// be removed from the token stream before further analysis.
//
// Inputs:
//   Input port 0: (required) A vtkTable containing a column of text.
//
// Outputs:
//   Output port 0: The same table, with (potentally) fewer rows.
//
// Use SetInputArrayToProcess(0, ...) to specify the "text" array.
//
// Use AddValue() to append a new token value to the list of values to be discarded.
// By default, the list of values to be discarded is empty, so you must call AddValue()
// before using vtkTokenValueFilter to see any changes in output!
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkTokenValueFilter_h
#define __vtkTokenValueFilter_h

#include <vtkTableAlgorithm.h>

class vtkUnicodeString;

class VTK_TEXT_ANALYSIS_EXPORT vtkTokenValueFilter :
  public vtkTableAlgorithm
{
public:
  static vtkTokenValueFilter* New();
  vtkTypeMacro(vtkTokenValueFilter, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Convenience function that adds a list of typical stop-words to the list of token
  // values that will be discarded.
  void AddStopWordValues();
  // Description:
  // Adds a new value to the list of token values that will be discarded.
  void AddValue(const vtkUnicodeString& value);
  // Description:
  // Clears the list of discardable token values.
  void ClearValues();

//BTX
protected:
  vtkTokenValueFilter();
  ~vtkTokenValueFilter();

  virtual int RequestData(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);


private:
  vtkTokenValueFilter(const vtkTokenValueFilter &); // Not implemented.
  void operator=(const vtkTokenValueFilter &); // Not implemented.

  class Internals;
  Internals* const Implementation;
//ETX
};

#endif // __vtkTokenValueFilter_h

