/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPCosmoReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkPCosmoReader.h

Copyright (c) 2010 Los Alamos National Security, LLC

All rights reserved.

Copyright 2010. Los Alamos National Security, LLC.
This software was produced under U.S. Government contract DE-AC52-06NA25396
for Los Alamos National Laboratory (LANL), which is operated by
Los Alamos National Security, LLC for the U.S. Department of Energy.
The U.S. Government has rights to use, reproduce, and distribute this software.
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.
If software is modified to produce derivative works, such modified software
should be clearly marked, so as not to confuse it with the version available
from LANL.

Additionally, redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following conditions
are met:
-   Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

class vtkInternals;

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

  vtkInternals *Internals;

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

};

#endif
