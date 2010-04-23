/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExodusModel.h

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

// .NAME vtkExodusModel - Exodus Model
//
// .SECTION Description
//
//   A vtkUnstructuredGrid output by vtkExodusReader or vtkPExodusReader
//   is missing a great deal of initialization and static model data 
//   that is in an Exodus II file.  (Global variables, properties,
//   node sets, side sets, and so on.)  This data can be stored in a 
//   vtkModelMetadata object, which can be initialized using
//   this vtkExodusModel class.
//
//   This class can be initialized with a file handle for an open Exodus 
//   file, and the vtkUnstructuredGrid derived from that file.  The methods 
//   used would be SetGlobalInformation, SetLocalInformation, 
//   AddUGridElementVariable and AddUGridNodeVariable.  The vtkExodusReader
//   does this.
//
//   It can also be initialized (using UnpackExodusModel) from a 
//   vtkUnstructuredGrid that has had metadata packed into it's field 
//   arrays with PackExodusModel.   The vtkExodusIIWriter does this.
//
//   If you plan to write out the Exodus file (with vtkExodusIIWriter), 
//   you should direct the Exodus reader to create a vtkExodusModel object.
//   This will be used by the Exodus writer to create a correct Exodus II 
//   file on output.  In addition, the vtkDistributedDataFilter is 
//   cognizant of the ExodusModel object and will unpack, extract, merge, 
//   and pack these objects associated with the grids it is partitioning.
//
// .SECTION See also
//   vtkExodusReader  vtkPExodusReader vtkExodusIIWriter vtkModelMetadata
//   vtkDistributedDataFilter

#ifndef __vtkExodusModel_h
#define __vtkExodusModel_h

#include "vtkObject.h"

#include "vtkModelMetadata.h"  // So those who include vtkExodusModel don't
                               // need to know anything about ModelMetadata

class vtkIdTypeArray;
class vtkUnstructuredGrid;

class VTK_HYBRID_EXPORT vtkExodusModel : public vtkObject
{ 
public:
  vtkTypeMacro(vtkExodusModel, vtkObject);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  static vtkExodusModel *New();

  // Description:
  //   In order to write a correct Exodus file from a
  //   vtkUnstructuredGrid, we need to know the global data
  //   which does not get represented in the UGrid.
  //   Initialize, with an open Exodus file, all the global 
  //   fields of the ExodusModel object.  fid is the file handle
  //   of the opened Exodus file.  compute_word_size is the
  //   size of floating point values exchanged with the
  //   the Exodus library.  (It's set in ex_open or ex_create.)
  //   The global fields are those which don't depend on
  //   which cells or field arrays are being read from the
  //   file.
 
  int SetGlobalInformation(int fid, int compute_word_size);

  // Description:
  //   In order to write Exodus files from vtkUnstructuredGrid
  //   objects that were read from Exodus files, we need to know
  //   the mapping from variable names in the UGrid to variable
  //   names in the Exodus file.  (The Exodus reader combines
  //   scalar variables with similar names into vectors in the
  //   UGrid.)  When building the UGrid to which this
  //   ExodusModel refers, add each element and node variable
  //   name with this call, including the name of original variable
  //   that yielded it's first component, and the number of components.
  //   If a variable is removed from the UGrid, remove it from
  //   the ExodusModel.  (If this information is missing or
  //   incomplete, the ExodusIIWriter can still do something
  //   sensible in creating names for variables.)

  int AddUGridElementVariable(char *ugridVarName, char *origName, int numComponents);
  int RemoveUGridElementVariable(char *ugridVarName);

  int AddUGridNodeVariable(char *ugridVarName, char *origName, int numComponents);
  int RemoveUGridNodeVariable(char *ugridVarName);

  void SetElementVariableInfo(int numOrigNames, char **origNames,
            int numNames, char **names,  int *numComp, int *map);
  void SetNodeVariableInfo(int numOrigNames, char **origNames,
            int numNames, char **names,  int *numComp, int *map);

  // Description:
  //  Set the local information in the ExodusModel.  This is
  //  information which depends on which blocks were read in,
  //  and which time step was read in.
  //  (Example - count of cells in each block, values of global
  //  variables, node IDs for nodes in each node set.)
  //  Provide the ugrid, the time step (the first time step is 0), 
  //  the handle of an open Exodus file, and the
  //  size of floating point values exchanged with the Exodus library.
  //  Also indicate with a 1 if the geometry has changed (new blocks
  //  or blocks removed) since the last call.  (When in doubt set to 1.)
  //  Please call SetGlobalInformation once before calling
  //  SetLocalInformation.  SetLocalInformation may be called many
  //  times if different subsets of an Exodus file are read.  Each
  //  call replaces the previous local values.

  int SetLocalInformation(vtkUnstructuredGrid *ugrid,
        int fid, int timeStep, int newGeometry, int compute_word_size);

  // Description:
  //   Static function that returns 1 if the vtkUnstructuredGrid
  //   has metadata packed into it's field arrays, 0 otherwise.

  static int HasMetadata(vtkUnstructuredGrid *grid);

  // Description:
  //   Set or get the underlying vtkModelMetadata object.

  vtkModelMetadata *GetModelMetadata();
  void SetModelMetadata(vtkModelMetadata *emData);

  // Description:
  //    One way to initialize an ExodusModel object is to use
  //    SetGlobalInformation, SetLocalInformation, and the Add/Remove
  //    Variable calls to initialize it from an open Exodus file.
  //
  //    Another way is to initialize it with the ExodusModel which
  //    has been packed into field arrays of a vtkUnstructuredGrid.
  //    Set the second argument to 1 if you would like the packed
  //    field arrays to be deleted after this ExodusModel is
  //    initialized.
  //    Returns 1 if there is no ExodusModel object associated with
  //    the grid, 0 otherwise.

  int UnpackExodusModel(vtkUnstructuredGrid *grid, int deleteIt);

  // Description:
  //   Merge the supplied vtkExodusModel object into this one.  It is
  //   assumed the two objects represent portions of the same distributed
  //   data set.  (So the list of block IDs is the same, and so on.)

  int MergeExodusModel(vtkExodusModel *em);

  // Description:
  //    Create a new vtkExodusModel object representing a subset of the
  //    cells of this vtkExodusModel object.  We need a list of the
  //    global IDs of the cells to be extracted, the grid which
  //    generated the Exodus Model (so we can find the points associated
  //    with each cell), and the name of the grid's global cell ID array,
  //    and the name of the grid's global node ID array.

  vtkExodusModel *ExtractExodusModel(vtkIdTypeArray *globalCellIdList, 
                                     vtkUnstructuredGrid *grid);

  // Description:
  //    The metadata encapsulated in a vtkExodusModel object can be
  //    written to field arrays which are then stored in the
  //    vtkUnstructuredGrid itself.  PackExodusModel creates these
  //    field arrays and attaches them to the supplied grid.  

  void PackExodusModel(vtkUnstructuredGrid *grid);
  
  // Description::
  //    Reset all fields to their initial value.

  void Reset();

protected:

  vtkExodusModel();
  ~vtkExodusModel();

private:

  vtkModelMetadata *CheckSetModelMetadata();

  static void CopyDoubleToFloat(float *f, double *d, int len);

  int SetLocalBlockInformation(
          int fid, int use_floats, int *blockIds , int *cellIds, int ncells);
  int SetLocalNodeSetInformation(
                       int fid, int use_floats, int *pointIds, int npoints);
  int SetLocalSideSetInformation(
                        int fid, int use_floats, int *cellIds, int ncells);

  void RemoveBeginningAndTrailingSpaces(char **names, int len);

  vtkModelMetadata *ModelMetadata;

  int GeometryCount;

  vtkExodusModel(const vtkExodusModel&); // Not implemented
  void operator=(const vtkExodusModel&); // Not implemented
};
#endif
