/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPASReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

Copyright (c) 2002-2005 Los Alamos National Laboratory

This software and ancillary information known as vtk_ext (and herein
called "SOFTWARE") is made available under the terms described below.
The SOFTWARE has been approved for release with associated LA_CC
Number 99-44, granted by Los Alamos National Laboratory in July 1999.

Unless otherwise indicated, this SOFTWARE has been authored by an
employee or employees of the University of California, operator of the
Los Alamos National Laboratory under Contract No. W-7405-ENG-36 with
the United States Department of Energy.

The United States Government has rights to use, reproduce, and
distribute this SOFTWARE.  The public may copy, distribute, prepare
derivative works and publicly display this SOFTWARE without charge,
provided that this Notice and any statement of authorship are
reproduced on all copies.

Neither the U. S. Government, the University of California, nor the
Advanced Computing Laboratory makes any warranty, either express or
implied, nor assumes any liability or responsibility for the use of
this SOFTWARE.

If SOFTWARE is modified to produce derivative works, such modified
SOFTWARE should be clearly marked, so as not to confuse it with the
version available from Los Alamos National Laboratory.

=========================================================================*/
// .NAME vtkMPASReader - Read an MPAS netCFD file
// .SECTION Description
// This program reads an MPAS netCDF data file to allow paraview to
// display a dual-grid sphere.
// The variables that have time dim are available to ParaView.
//
// Assume all variables are of interest if they have dims
// (Time, nCells|nVertices, nVertLevels, [nTracers])
// Converts variable data type from double to float.
// Assume no more than 100 vars each for cell and point data
// Displays tracer vars as tracer1, tracer2, etc.
// Does not deal with edge data.
//
// Christine Ahrens
// 8/9/2010
// Version 1.2

#ifndef __vtkMPASReader_h
#define __vtkMPASReader_h

#define MAX_VARS 100
#define MAX_VAR_NAME 100

#include "vtkUnstructuredGridAlgorithm.h"

class vtkCallbackCommand;
class vtkDataArraySelection;
class vtkFloatArray;
class vtkStdString;
class vtkStringArray;

class VTK_IO_EXPORT vtkMPASReader : public vtkUnstructuredGridAlgorithm
{
 public:
  static vtkMPASReader *New();
  vtkTypeMacro(vtkMPASReader,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify file name of MPAS data file to read.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Get the number of data cells
  vtkGetMacro(NumberOfDualCells, int);

  // Description:
  // Get the number of points
  vtkGetMacro(NumberOfDualPoints, int);

  // Description:
  // Get the number of data variables at the cell centers and points
  vtkGetMacro(NumberOfVariables, int);

  // Description:
  // Get the reader's output
  vtkUnstructuredGrid *GetOutput();
  vtkUnstructuredGrid *GetOutput(int index);

  // Description:
  // The following methods allow selective reading of solutions fields.
  // By default, ALL data fields on the nodes are read, but this can
  // be modified.
  int GetNumberOfPointArrays();
  const char* GetPointArrayName(int index);
  int GetPointArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);
  void DisableAllPointArrays();
  void EnableAllPointArrays();

  int GetNumberOfCellArrays();
  const char* GetCellArrayName(int index);
  int GetCellArrayStatus(const char* name);
  void SetCellArrayStatus(const char* name, int status);
  void DisableAllCellArrays();
  void EnableAllCellArrays();
  void SetVerticalLevel(int level);
  vtkGetVector2Macro(VerticalLevelRange, int);

  // Description:
  // Returns true if the given file can be read.
  static int CanReadFile(const char *filename);

 protected:
  vtkMPASReader();
  ~vtkMPASReader();

  char *FileName;         // First field part file giving path
  /*
    int Rank;               // Number of this processor
    int TotalRank;          // Number of processors
  */

  //  int NumberOfPieces;         // Number of files in dataset
  vtkIdType NumberOfDualPoints;       // Number of points in grid
  vtkIdType NumberOfDualCells;        // Number of cells in grid
  // vtkIdType NumberOfTuples;        // Number of tuples in sub extent

  int NumberOfVariables;      // Number of variables to display
  vtkStdString* VariableName;     // Names of each variable
  int* VariableType;          // Scalar, vector or tensor

  int NumberOfTimeSteps;      // Temporal domain
  double* TimeSteps;          // Times available for request
  double dTime;

  vtkFloatArray** dualCellVarData;    // Actual data arrays
  vtkFloatArray** dualPointVarData;   // Actual data arrays

  // Selected field of interest
  vtkDataArraySelection* PointDataArraySelection;
  vtkDataArraySelection* CellDataArraySelection;

  int VerticalLevelSelected;
  int VerticalLevelRange[2];

  // Observer to modify this object when array selections are modified
  vtkCallbackCommand* SelectionObserver;

  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *);
  int RequestInformation(vtkInformation *, vtkInformationVector **,
                         vtkInformationVector *);

  void LoadGeometryData(int var, double dTime);
  void LoadPointData(int var);
  void LoadCellData(int var);

  static void SelectionCallback(vtkObject* caller, unsigned long eid,
                                void* clientdata, void* calldata);

  bool infoRequested;
  bool dataRequested;

  char tracerNames[MAX_VAR_NAME][MAX_VARS];
  int numDualCellVars;
  int numDualPointVars;
  double* primalPointVarData;
  double* primalCellVarData;
  int ReadAndOutputDualGrid();
  int ReadAndOutputVariableData();
  int LoadPointVarData(int variable, double dTime);
  int LoadCellVarData(int variable, double dTime);
  int BuildVarArrays();

 private:
  vtkMPASReader(const vtkMPASReader&);    // Not implemented.
  void operator=(const vtkMPASReader&); // Not implemented.

  class Internal;
  Internal *Internals;

};

#endif
