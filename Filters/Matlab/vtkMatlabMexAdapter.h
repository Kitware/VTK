
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMatlabMexAdapter.h

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
 * @class   vtkMatlabMexAdapter
 * @brief   This is a utility class to convert VTK array
 *  data to and from the Matlab mxArray format.  It is used with the Matlab
 *  Mex and Matlab Engine interfaces.
 *
 *
 *
 *  The default behavior of each function is to perform a deep copy of the
 *  data.  Set the ShallowCopy argument to true to use the block of memory
 *  that has already been allocated by either VTK or Matlab.  The result of
 *  a shallow copy will produce the transpose of the data in the new system,
 *  because VTK uses row major ordering and Matlab uses column major ordering
 *  for data.
 *
 *  VTK data structures created by this class from Matlab types are stored in
 *  array collections and freed when the class destructor is called.  Use the
 *  Register() method on a returned object to increase its reference count by one,
 *  in order keep the object around after this classes destructor has been called.
 *  The code calling Register() must eventually call Delete() on the object to free
 *  memory.
 *
 * @sa
 *  vtkMatlabEngineInterface vtkMatlabEngineFilter
 *
 * @par Thanks:
 *  Developed by Thomas Otahal at Sandia National Laboratories.
 *
*/

#ifndef vtkMatlabMexAdapter_h
#define vtkMatlabMexAdapter_h

#include "mex.h" // Needed for Matlab mex.h mxArray
#include "matrix.h" // Needed for Matlab matrix.h mxArray
#include "vtkObject.h"
#include "vtkFiltersMatlabModule.h" // For export macro

class vtkInformation;
class vtkInformationVector;
class vtkDataArray;
class vtkArray;
class vtkGraph;
class vtkDataArrayCollection;
class vtkArrayData;
class vtkDataObjectCollection;

class VTKFILTERSMATLAB_EXPORT vtkMatlabMexAdapter : public vtkObject
{

public:

  vtkTypeMacro(vtkMatlabMexAdapter, vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkMatlabMexAdapter *New();

  /**
   * Create a mxArray copy of a vtkDataArray (Allocates memory by default)
   */
  mxArray* vtkDataArrayToMxArray(vtkDataArray* aa, bool ShallowCopy = false);

  /**
   * Create a vtkDataArray copy of a Matlab mxArray (Allocates memory by default)
   */
  vtkDataArray* mxArrayTovtkDataArray(const mxArray* mxa, bool ShallowCopy = false);

  /**
   * Create a mxArray copy of a vtkArray (Allocates memory by default)
   */
  mxArray* vtkArrayToMxArray(vtkArray* va);

  /**
   * Create a vtkArray copy of a mxArray (Allocates memory by default)
   */
  vtkArray* mxArrayTovtkArray(mxArray* mxa);

  /**
   * Create a mxArray copy of a vtkGraph (Allocates memory by default)
   * The result is an n by n connectivity matrix, where n is the number
   * of nodes in the graph.
   */
  mxArray* vtkGraphToMxArray(vtkGraph* ga);

  /**
   * Create a vtkGraph copy of a mxArray (Allocates memory by default)
   * Input mxArray should be a n by n connectivity matrix, where n is
   * the number of nodes in the graph.
   */
  vtkGraph* mxArrayTovtkGraph(mxArray* mxa);

  /**
   * Match Matlab and VTK data types for conversion.
   */
  static mxClassID GetMatlabDataType(vtkDataArray* da);

  /**
   * Match Matlab and VTK data types for conversion.
   */
  static vtkDataArray* GetVTKDataType(mxClassID cid);

protected:

  vtkMatlabMexAdapter();
  ~vtkMatlabMexAdapter();

private:

  vtkMatlabMexAdapter(const vtkMatlabMexAdapter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMatlabMexAdapter&) VTK_DELETE_FUNCTION;

  template<typename T> vtkArray* CopymxArrayToVTKArray(mxArray* mxa, int ValueType);

  vtkDataArrayCollection* vdac;  // Collection of vtkDataArrays that have been converted from R.
  vtkArrayData* vad;  // Collection of vtkArrays that have been converted from R.
  vtkDataObjectCollection* vdoc; // Collection of vtkTables that have been converted from R.

};

#endif
