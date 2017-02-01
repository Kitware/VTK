/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExodusIIWriter.h

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

/**
 * @class   vtkPExodusIIWriter
 * @brief   Write Exodus II files
 *
 *     This is a vtkWriter that writes it's vtkUnstructuredGrid
 *     input out to an Exodus II file.  Go to http://endo.sandia.gov/SEACAS/
 *     for more information about the Exodus II format.
 *
 *     Exodus files contain much information that is not captured
 *     in a vtkUnstructuredGrid, such as time steps, information
 *     lines, node sets, and side sets.  This information can be
 *     stored in a vtkModelMetadata object.
 *
 *     The vtkExodusReader and vtkPExodusReader can create
 *     a vtkModelMetadata object and embed it in a vtkUnstructuredGrid
 *     in a series of field arrays.  This writer searches for these
 *     field arrays and will use the metadata contained in them
 *     when creating the new Exodus II file.
 *
 *     You can also explicitly give the vtkExodusIIWriter a
 *     vtkModelMetadata object to use when writing the file.
 *
 *     In the absence of the information provided by vtkModelMetadata,
 *     if this writer is not part of a parallel application, we will use
 *     reasonable defaults for all the values in the output Exodus file.
 *     If you don't provide a block ID element array, we'll create a
 *     block for each cell type that appears in the unstructured grid.
 *
 *     However if this writer is part of a parallel application (hence
 *     writing out a distributed Exodus file), then we need at the very
 *     least a list of all the block IDs that appear in the file.  And
 *     we need the element array of block IDs for the input unstructured grid.
 *
 *     In the absence of a vtkModelMetadata object, you can also provide
 *     time step information which we will include in the output Exodus
 *     file.
 *
 * @warning
 *     If the input floating point field arrays and point locations are all
 *     floats or all doubles, this class will operate more efficiently.
 *     Mixing floats and doubles will slow you down, because Exodus II
 *     requires that we write only floats or only doubles.
 *
 * @warning
 *     We use the terms "point" and "node" interchangeably.
 *     Also, we use the terms "element" and "cell" interchangeably.
*/

#ifndef vtkPExodusIIWriter_h
#define vtkPExodusIIWriter_h

#include "vtkIOParallelExodusModule.h" // For export macro
#include "vtkSmartPointer.h" // For vtkSmartPointer
#include "vtkExodusIIWriter.h"

#include <vector> // STL Header
#include <map>    // STL Header
#include <string> // STL Header

class vtkModelMetadata;
class vtkDoubleArray;
class vtkIntArray;
class vtkUnstructuredGrid;

class VTKIOPARALLELEXODUS_EXPORT vtkPExodusIIWriter : public vtkExodusIIWriter
{
public:
  static vtkPExodusIIWriter *New ();
  vtkTypeMacro(vtkPExodusIIWriter,vtkExodusIIWriter);
  void PrintSelf (ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkPExodusIIWriter ();
  ~vtkPExodusIIWriter ();
  virtual int CheckParameters () VTK_OVERRIDE;
  virtual void CheckBlockInfoMap() VTK_OVERRIDE;

  virtual int RequestUpdateExtent (vtkInformation* request,
                                   vtkInformationVector** inputVector,
                                   vtkInformationVector* outputVector) VTK_OVERRIDE;
  virtual int GlobalContinueExecuting(int localContinue) VTK_OVERRIDE;

private:
  vtkPExodusIIWriter (const vtkPExodusIIWriter&) VTK_DELETE_FUNCTION;
  void operator= (const vtkPExodusIIWriter&) VTK_DELETE_FUNCTION;
};

#endif
