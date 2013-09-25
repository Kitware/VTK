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
// .NAME vtkMPASReader - Read an MPAS netCDF file
// .SECTION Description
// This program reads an MPAS netCDF data file to allow paraview to
// display a dual-grid sphere or latlon projection.  Also allows
// display of primal-grid sphere.
// The variables that have time dim are available to ParaView.
//
// Assume all variables are of interest if they have dims
// (Time, nCells|nVertices, nVertLevels, [nTracers])
// Assume no more than 100 vars each for cell and point data
// Does not deal with edge data.
//
// When using this reader, it is important that you remember to do the
//following:
//   1.  When changing a selected variable, remember to select it also
//       in the drop down box to "color by".  It doesn't color by that variable
//       automatically.
//   2.  When selecting multilayer sphere view, make layer thickness around
//       100,000.
//   3.  When selecting multilayer lat/lon view, make layer thickness around 10.
//   4.  Always click the -Z orientation after making a switch from lat/lon to
//       sphere, from single to multilayer or changing thickness.
//   5.  Be conservative on the number of changes you make before hitting Apply,
//       since there may be bugs in this reader.  Just make one change and then
//       hit Apply.

//
// Christine Ahrens (cahrens@lanl.gov)
// Version 1.3

#ifndef __vtkMPASReader_h
#define __vtkMPASReader_h

#define MAX_VARS 100
#define MAX_VAR_NAME 100

#include "vtkIONetCDFModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class vtkCallbackCommand;
class vtkDataArraySelection;
class vtkDoubleArray;
class vtkStdString;
class vtkStringArray;

class VTKIONETCDF_EXPORT vtkMPASReader : public vtkUnstructuredGridAlgorithm
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
  vtkGetMacro(MaximumCells, int);

  // Description:
  // Get the number of points
  vtkGetMacro(MaximumPoints, int);

  // Description:
  // Get the number of data variables at the cell centers and points
  vtkGetMacro(NumberOfCellVars, int);
  vtkGetMacro(NumberOfPointVars, int);

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

  void SetLayerThickness(int val);
  vtkGetVector2Macro(LayerThicknessRange, int);

  void SetCenterLon(int val);
  vtkGetVector2Macro(CenterLonRange, int);

  void SetProjectLatLon(bool val);
  vtkGetMacro(ProjectLatLon, bool);

  void SetIsAtmosphere(bool val);
  vtkGetMacro(IsAtmosphere, bool);

  void SetIsZeroCentered(bool val);
  vtkGetMacro(IsZeroCentered, bool);

  void SetShowMultilayerView(bool val);
  vtkGetMacro(ShowMultilayerView, bool);

  // Description:
  // Returns true if the given file can be read.
  static int CanReadFile(const char *filename);

 protected:
  vtkMPASReader();
  ~vtkMPASReader();
  void DestroyData();

  char *FileName;         // First field part file giving path
  /*
    int Rank;               // Number of this processor
    int TotalRank;          // Number of processors
  */

  //  int NumberOfPieces;         // Number of files in dataset
  // vtkIdType NumberOfTuples;        // Number of tuples in sub extent

  vtkStdString* VariableName;     // Names of each variable
  int* VariableType;          // Scalar, vector or tensor

  int NumberOfTimeSteps;      // Temporal domain
  double* TimeSteps;          // Times available for request
  double DTime;


  // Observer to modify this object when array selections are modified
  vtkCallbackCommand* SelectionObserver;

  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *);
  int RequestInformation(vtkInformation *, vtkInformationVector **,
                         vtkInformationVector *);


  static void SelectionCallback(vtkObject* caller, unsigned long eid,
                                void* clientdata, void* calldata);

  bool InfoRequested;
  bool DataRequested;

  // params

  // Selected field of interest
  vtkDataArraySelection* PointDataArraySelection;
  vtkDataArraySelection* CellDataArraySelection;

  vtkDoubleArray** CellVarDataArray;    // Actual data arrays
  vtkDoubleArray** PointVarDataArray;   // Actual data arrays

  int VerticalLevelSelected;
  int VerticalLevelRange[2];

  int LayerThickness;
  int LayerThicknessRange[2];

  int CenterLon;
  int CenterLonRange[2];

  bool ProjectLatLon;
  bool IsAtmosphere;
  bool IsZeroCentered;
  bool ShowMultilayerView;

  bool IncludeTopography;
  bool DoBugFix;
  double CenterRad;


  // geometry
  int MaximumNVertLevels;
  int NumberOfCells;
  int NumberOfPoints;
  int CellOffset;
  int PointOffset;
  int PointsPerCell;
  int CurrentExtraPoint;  // current extra point
  int CurrentExtraCell;   // current extra  cell
  double* PointX;      // x coord of point
  double* PointY;      // y coord of point
  double* PointZ;      // z coord of point
  int ModNumPoints;
  int ModNumCells;
  int* CellMask;
  int* OrigConnections;   // original connections
  int* ModConnections;    // modified connections
  int* CellMap;           // maps from added cell to original cell #
  int* PointMap;          // maps from added point to original point #
  int* MaximumLevelPoint;      //
  int MaximumCells;           // max cells
  int MaximumPoints;          // max points
  int VerticalIndex;      // for singleLayer, which vertical level

  // vars
  int NumberOfCellVars;
  int NumberOfPointVars;
  double* PointVarData;

  void SetDefaults();
  int GetNcDims();
  int CheckParams();
  int GetNcVars(const char* cellDimName, const char* pointDimName);
  int ReadAndOutputGrid(bool init);
  int ReadAndOutputVariableData();
  int BuildVarArrays();
  int AllocSphereGeometry();
  int AllocLatLonGeometry();
  void ShiftLonData();
  int AddMirrorPoint(int index, double dividerX);
  void FixPoints();
  int EliminateXWrap();
  void OutputPoints(bool init);
  void OutputCells(bool init);
  unsigned char GetCellType();
  void LoadGeometryData(int var, double dTime);
  int LoadPointVarData(int variable, double dTime);
  int LoadCellVarData(int variable, double dTime);
  int RegenerateGeometry();

 private:
  vtkMPASReader(const vtkMPASReader&);    // Not implemented.
  void operator=(const vtkMPASReader&); // Not implemented.
  class Internal;
  Internal *Internals;

};

#endif
