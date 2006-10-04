/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenFOAMReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenFOAMReader - reads a dataset in OpenFOAM format
// .SECTION Description
// vtkOpenFOAMReader creates an multiblock dataset. It reads a controlDict
// file, mesh information, and time dependent data.  The controlDict file
// contains timestep information. The polyMesh folders contain mesh information
// The time folders contain transient data for the cells  Each folder can
// contain any number of data files.

// .SECTION Thanks
// Thanks to Terry Jordan of SAIC at the National Energy
// Technology Laboratory who developed this class.
// Please address all comments to Terry Jordan (terry.jordan@sa.netl.doe.gov)

#ifndef __vtkOpenFOAMReader_h
#define __vtkOpenFOAMReader_h

#include "vtkMultiBlockDataSetAlgorithm.h"

typedef struct
{
  int faceIndex;
  bool neighborFace;
} face;

class vtkUnstructuredGrid;
class vtkPoints;
class vtkIntArray;
class vtkFloatArray;
class vtkDoubleArray;
class vtkDataArraySelection;
struct stdString;
struct stringVector;
struct intVector;
struct intVectorVector;
struct faceVectorVector;

class VTK_IO_EXPORT vtkOpenFOAMReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkOpenFOAMReader *New();
  vtkTypeRevisionMacro(vtkOpenFOAMReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the filename.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Returns the number of timesteps.
  vtkGetMacro(NumberOfTimeSteps, int);

  // Description:
  // Set/Get the current timestep.
  vtkSetMacro(TimeStep, int);
  vtkGetMacro(TimeStep, int);

  // Description:
  // Get the timesteprange. Filled during RequestInformation.
  vtkGetVector2Macro(TimeStepRange, int);

  // Description:
  // Get the number of cell arrays available in the input.
  int GetNumberOfCellArrays(void);

  // Description:
  // Get/Set whether the cell array with the given name is to
  // be read.
  int GetCellArrayStatus(const char* name);
  void SetCellArrayStatus(const char* name, int status);

  // Description:
  // Get the name of the  cell array with the given index in
  // the input.
  const char* GetCellArrayName(int index);

  // Description:
  // Turn on/off all cell arrays.
  void DisableAllCellArrays();
  void EnableAllCellArrays();


protected:
  vtkOpenFOAMReader();
  ~vtkOpenFOAMReader();
  int RequestData(vtkInformation *,
    vtkInformationVector **, vtkInformationVector *);
  int RequestInformation(vtkInformation *,
    vtkInformationVector **, vtkInformationVector *);

private:
  vtkOpenFOAMReader(const vtkOpenFOAMReader&);  // Not implemented.
  void operator=(const vtkOpenFOAMReader&);  // Not implemented.

  vtkSetVector2Macro(TimeStepRange, int);

  char * FileName;
  int NumberOfTimeSteps;
  int TimeStep;
  int TimeStepRange[2];
  double * Steps;
  bool RequestInformationFlag;
  int StartFace;

  stdString * Path;
  stdString * PathPrefix;
  stringVector * TimeStepData;
  vtkDataArraySelection * CellDataArraySelection;
  intVectorVector * FacePoints;
  intVectorVector * FacesOwnerCell;
  intVectorVector * FacesNeighborCell;
  faceVectorVector * FacesOfCell;
  vtkPoints * Points;
  vtkIdType NumCells;
  vtkIdType NumFaces;
  vtkIntArray * FaceOwner;
  //vtkIntArray * FaceNeighbor;
  stringVector * PolyMeshPointsDir;
  stringVector * PolyMeshFacesDir;
  vtkIdType NumPoints;
  intVector * SizeOfBoundary;
  stringVector * BoundaryNames;
  stringVector * PointZoneNames;
  stringVector * FaceZoneNames;
  stringVector * CellZoneNames;
  int NumBlocks;
  void CombineOwnerNeigbor();
  vtkUnstructuredGrid * MakeInternalMesh();
  double ControlDictDataParser(const char *);
  void ReadControlDict ();
  void GetPoints (int);
  void ReadFacesFile (const char *);
  void ReadOwnerFile(const char *);
  void ReadNeighborFile(const char *);
  void PopulatePolyMeshDirArrays();
  const char * GetDataType(const char *, const char *);
  vtkDoubleArray * GetInternalVariableAtTimestep(const char *, int);
  vtkDoubleArray * GetBoundaryVariableAtTimestep(int, const char *, int,
                                                 vtkUnstructuredGrid *);
  stringVector *GatherBlocks(const char *, int);
  vtkUnstructuredGrid * GetBoundaryMesh(int, int);
  vtkUnstructuredGrid * GetPointZoneMesh(int, int);
  vtkUnstructuredGrid * GetFaceZoneMesh(int, int);
  vtkUnstructuredGrid * GetCellZoneMesh(int, int);
  void CreateDataSet(vtkMultiBlockDataSet *);
};

#endif
