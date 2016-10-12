
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRAdapter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

/**
 * @class   vtkRAdapter
 * @brief   This is a utility class to convert VTK array data and
 *  VTK tables to and from Gnu R S expression (SEXP) data structures.  It is used
 *  with the R .Call interface and the embedded R interpreter.
 *
 *
 *
 *  This class creates deep copies of input data.  Created R SEXP variables created
 *  by these functions can be freed by the R garbage collector by calling UNPROTECT(1).
 *  The conversions are performed for double and integer data types.
 *
 *  VTK data structures created by this class from R types are stored in array collections
 *  and freed when the class destructor is called.  Use the Register() method on a returned
 *  object to increase its reference count by one, in order keep the object around after this
 *  classes destructor has been called.  The code calling Register() must eventually call Delete()
 *  on the object to free memory.
 *
 * @sa
 *  vtkRinterface vtkRcalculatorFilter
 *
 * @par Thanks:
 *  Developed by Thomas Otahal at Sandia National Laboratories.
 *
*/

#ifndef vtkRAdapter_h
#define vtkRAdapter_h

#include "vtkFiltersStatisticsGnuRModule.h" // For export macro
#include "vtkObject.h"

#ifndef VTK_BUILDING_FILTERS_STATISTICSGNUR
#define R_NO_REMAP /* Don't pollute the namespace (some of which conflict). DO NOT REMOVE. */
#endif

#include "Rinternals.h" // Needed for Rinternals.h SEXP data structure

class vtkInformation;
class vtkInformationVector;
class vtkDataArray;
class vtkArray;
class vtkTable;
class vtkTree;
class vtkDataArrayCollection;
class vtkArrayData;
class vtkDataObjectCollection;

class VTKFILTERSSTATISTICSGNUR_EXPORT vtkRAdapter : public vtkObject
{

public:

  vtkTypeMacro(vtkRAdapter, vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkRAdapter *New();

  /**
   * Create a vtkDataArray copy of GNU R input matrix vaiable (deep copy, allocates memory)
   * Input is a R matrix or vector of doubles or integers
   */
  vtkDataArray* RToVTKDataArray(SEXP variable);

  /**
   * Create a vtkArray copy of the GNU R input variable multi-dimensional array (deep copy, allocates memory)
   * Input is a R multi-dimensional array of doubles or integers
   */
  vtkArray* RToVTKArray(SEXP variable);

  /**
   * Create a GNU R matrix copy of the input vtkDataArray da (deep copy, allocates memory)
   */
  SEXP VTKDataArrayToR(vtkDataArray* da);

  /**
   * Create a GNU R multi-dimensional array copy of the input vtkArray da (deep copy, allocates memory)
   */
  SEXP VTKArrayToR(vtkArray* da);

  /**
   * Create a GNU R matrix copy of the input vtkTable table (deep copy, allocates memory)
   */
  SEXP VTKTableToR(vtkTable* table);

  /**
   * Create a vtkTable copy of the GNU R input matrix variable (deep copy, allocates memory)
   * Input is R list of equal length vectors or a matrix.
   */
  vtkTable* RToVTKTable(SEXP variable);

  /**
   * Create a GNU R phylo tree copy of the input vtkTree tree (deep copy, allocates memory)
   */
  SEXP VTKTreeToR(vtkTree* tree);

  /**
   * Create a vtkTree copy of the GNU R input phylo tree variable (deep copy, allocates memory)
   */
  vtkTree* RToVTKTree(SEXP variable);

protected:
  vtkRAdapter();
  ~vtkRAdapter();

private:

  vtkRAdapter(const vtkRAdapter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRAdapter&) VTK_DELETE_FUNCTION;

  vtkDataArrayCollection* vdac;  // Collection of vtkDataArrays that have been converted from R.
  vtkArrayData* vad;  // Collection of vtkArrays that have been converted from R.
  vtkDataObjectCollection* vdoc; // Collection of vtkTables that have been converted from R.

};


#endif
