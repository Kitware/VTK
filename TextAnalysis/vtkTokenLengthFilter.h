/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTokenLengthFilter.h

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

#ifndef __vtkTokenLengthFilter_h
#define __vtkTokenLengthFilter_h

#include <vtkTableAlgorithm.h>

// .NAME vtkTokenLengthFilter - Filters tokens based on their length (number of characters).
//
// .SECTION Description
//
// Parameters:
//   Begin, End: Defines a half-open range of token lengths [Begin, End) that will be removed
//     from the output.  Tokens for which Begin <= token-length < End is true will be removed.
//
// Inputs:
//   Input port 0: (required) A vtkTable containing a column of text.
//
// Outputs:
//   Output port 0: The same table, with (potentally) fewer rows.
//
// Use SetInputArrayToProcess(0, ...) to specify the "text" array.
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

class VTK_TEXT_ANALYSIS_EXPORT vtkTokenLengthFilter :
  public vtkTableAlgorithm
{
public:
  static vtkTokenLengthFilter* New();
  vtkTypeMacro(vtkTokenLengthFilter, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Controls the half-open range of token lengths that will be removed.  The default of [0, 0)
  // will not remove any tokens, so you must change Begin and End to see any results from the filter!
  vtkSetMacro(Begin, int);
  vtkGetMacro(Begin, int);
  vtkSetMacro(End, int);
  vtkGetMacro(End, int);

//BTX
protected:
  vtkTokenLengthFilter();
  ~vtkTokenLengthFilter();

  virtual int RequestData(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  int Begin;
  int End;

private:
  vtkTokenLengthFilter(const vtkTokenLengthFilter &); // Not implemented.
  void operator=(const vtkTokenLengthFilter &); // Not implemented.
//ETX
};

#endif // __vtkTokenLengthFilter_h

