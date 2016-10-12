/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractArray.h

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkExtractArray
 * @brief   Given a vtkArrayData object containing one-or-more
 * vtkArray instances, produces a vtkArrayData containing just one vtkArray,
 * indentified by index.
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
*/

#ifndef vtkExtractArray_h
#define vtkExtractArray_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkArrayDataAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkExtractArray : public vtkArrayDataAlgorithm
{
public:
  static vtkExtractArray* New();
  vtkTypeMacro(vtkExtractArray, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Controls which array will be extracted.
   */
  vtkGetMacro(Index, vtkIdType);
  vtkSetMacro(Index, vtkIdType);
  //@}

protected:
  vtkExtractArray();
  ~vtkExtractArray() VTK_OVERRIDE;

  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) VTK_OVERRIDE;

private:
  vtkExtractArray(const vtkExtractArray&) VTK_DELETE_FUNCTION;
  void operator=(const vtkExtractArray&) VTK_DELETE_FUNCTION;

  vtkIdType Index;
};

#endif

