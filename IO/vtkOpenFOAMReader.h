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
// vtkOpenFOAMReader creates a multiblock dataset. It reads mesh
// information and time dependent data.  The polyMesh folders contain
// mesh information. The time folders contain transient data for the
// cells. Each folder can contain any number of data files.

// .SECTION Thanks
// Thanks to Terry Jordan of SAIC at the National Energy
// Technology Laboratory who developed this class.
// Please address all comments to Terry Jordan (terry.jordan@sa.netl.doe.gov).
// GUI Based selection of mesh regions and fields available in the case,
// minor bug fixes, strict memory allocation checks,
// minor performance enhancements by Philippose Rajan (sarith@rocketmail.com).
// Token-based FoamFile format lexer/parser,
// performance/stability/compatibility enhancements, gzipped file
// support, lagrangian field support, variable timestep support,
// builtin cell-to-point filter, pointField support, polyhedron
// decomposition support, OF 1.5 extended format support, multiregion
// support, old mesh format support, parallelization support for
// decomposed cases in conjunction with vtkPOpenFOAMReader, et. al. by
// Takuya Oshima of Niigata University, Japan (oshima@eng.niigata-u.ac.jp).

#ifndef __vtkOpenFOAMReader_h
#define __vtkOpenFOAMReader_h

#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkCollection;
class vtkCharArray;
class vtkDataArraySelection;
class vtkDoubleArray;
class vtkStdString;
class vtkStringArray;

class vtkOpenFOAMReaderPrivate;

class VTK_IO_EXPORT vtkOpenFOAMReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkOpenFOAMReader *New();
  vtkTypeMacro(vtkOpenFOAMReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream &, vtkIndent);

  // Description:
  // Determine if the file can be readed with this reader.
  int CanReadFile(const char *);

  // Description:
  // Set/Get the filename.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Get the number of cell arrays available in the input.
  int GetNumberOfCellArrays(void)
  { return this->GetNumberOfSelectionArrays(this->CellDataArraySelection); }

  // Description:
  // Get/Set whether the cell array with the given name is to
  // be read.
  int GetCellArrayStatus(const char *name)
  { return this->GetSelectionArrayStatus(this->CellDataArraySelection, name); }
  void SetCellArrayStatus(const char *name, int status)
  { this->SetSelectionArrayStatus(this->CellDataArraySelection, name, status); }

  // Description:
  // Get the name of the  cell array with the given index in
  // the input.
  const char *GetCellArrayName(int index)
  { return this->GetSelectionArrayName(this->CellDataArraySelection, index); }

  // Description:
  // Turn on/off all cell arrays.
  void DisableAllCellArrays()
  { this->DisableAllSelectionArrays(this->CellDataArraySelection); }
  void EnableAllCellArrays()
  { this->EnableAllSelectionArrays(this->CellDataArraySelection); }

  // Description:
  // Get the number of point arrays available in the input.
  int GetNumberOfPointArrays(void)
  { return this->GetNumberOfSelectionArrays(this->PointDataArraySelection); }

  // Description:
  // Get/Set whether the point array with the given name is to
  // be read.
  int GetPointArrayStatus(const char *name)
  { return this->GetSelectionArrayStatus(this->PointDataArraySelection, name); }
  void SetPointArrayStatus(const char *name, int status)
  { this->SetSelectionArrayStatus(this->PointDataArraySelection,
    name, status); }

  // Description:
  // Get the name of the  point array with the given index in
  // the input.
  const char *GetPointArrayName(int index)
  { return this->GetSelectionArrayName(this->PointDataArraySelection, index); }

  // Description:
  // Turn on/off all point arrays.
  void DisableAllPointArrays()
  { this->DisableAllSelectionArrays(this->PointDataArraySelection); }
  void EnableAllPointArrays()
  { this->EnableAllSelectionArrays(this->PointDataArraySelection); }

  // Description:
  // Get the number of Lagrangian arrays available in the input.
  int GetNumberOfLagrangianArrays(void)
  { return this->GetNumberOfSelectionArrays(
    this->LagrangianDataArraySelection); }

  // Description:
  // Get/Set whether the Lagrangian array with the given name is to
  // be read.
  int GetLagrangianArrayStatus(const char *name)
  { return this->GetSelectionArrayStatus(this->LagrangianDataArraySelection,
    name); }
  void SetLagrangianArrayStatus(const char *name, int status)
  { this->SetSelectionArrayStatus(this->LagrangianDataArraySelection, name,
    status); }

  // Description:
  // Get the name of the  Lagrangian array with the given index in
  // the input.
  const char* GetLagrangianArrayName(int index)
  { return this->GetSelectionArrayName(this->LagrangianDataArraySelection,
    index); }

  // Description:
  // Turn on/off all Lagrangian arrays.
  void DisableAllLagrangianArrays()
  { this->DisableAllSelectionArrays(this->LagrangianDataArraySelection); }
  void EnableAllLagrangianArrays()
  { this->EnableAllSelectionArrays(this->LagrangianDataArraySelection); }

  // Description:
  // Get the number of Patches (inlcuding Internal Mesh) available in the input.
  int GetNumberOfPatchArrays(void)
  { return this->GetNumberOfSelectionArrays(this->PatchDataArraySelection); }

  // Description:
  // Get/Set whether the Patch with the given name is to
  // be read.
  int GetPatchArrayStatus(const char *name)
  { return this->GetSelectionArrayStatus(this->PatchDataArraySelection, name); }
  void SetPatchArrayStatus(const char *name, int status)
  { this->SetSelectionArrayStatus(this->PatchDataArraySelection, name,
    status); }

  // Description:
  // Get the name of the Patch with the given index in
  // the input.
  const char *GetPatchArrayName(int index)
  { return this->GetSelectionArrayName(this->PatchDataArraySelection, index); }

  // Description:
  // Turn on/off all Patches including the Internal Mesh.
  void DisableAllPatchArrays()
  { this->DisableAllSelectionArrays(this->PatchDataArraySelection); }
  void EnableAllPatchArrays()
  { this->EnableAllSelectionArrays(this->PatchDataArraySelection); }

  // Description:
  // Set/Get whether to create cell-to-point translated data for cell-type data
  vtkSetMacro(CreateCellToPoint,int);
  vtkGetMacro(CreateCellToPoint,int);
  vtkBooleanMacro(CreateCellToPoint, int);

  // Description:
  // Set/Get whether mesh is to be cached.
  vtkSetMacro(CacheMesh, int);
  vtkGetMacro(CacheMesh, int);
  vtkBooleanMacro(CacheMesh, int);

  // Description:
  // Set/Get whether polyhedra are to be decomposed.
  vtkSetMacro(DecomposePolyhedra, int);
  vtkGetMacro(DecomposePolyhedra, int);
  vtkBooleanMacro(DecomposePolyhedra, int);

  // Option for reading old binary lagrangian/positions format
  // Description:
  // Set/Get whether the lagrangian/positions is in OF 1.3 format
  vtkSetMacro(PositionsIsIn13Format, int);
  vtkGetMacro(PositionsIsIn13Format, int);
  vtkBooleanMacro(PositionsIsIn13Format, int);

  // Description:
  // Determine if time directories are to be listed according to controlDict
  vtkSetMacro(ListTimeStepsByControlDict, int);
  vtkGetMacro(ListTimeStepsByControlDict, int);
  vtkBooleanMacro(ListTimeStepsByControlDict, int);

  // Description:
  // Add dimensions to array names
  vtkSetMacro(AddDimensionsToArrayNames, int);
  vtkGetMacro(AddDimensionsToArrayNames, int);
  vtkBooleanMacro(AddDimensionsToArrayNames, int);

  // Description:
  // Set/Get whether zones will be read.
  vtkSetMacro(ReadZones, int);
  vtkGetMacro(ReadZones, int);
  vtkBooleanMacro(ReadZones, int);

  void SetRefresh() { this->Refresh = true; this->Modified(); }

  void SetParent(vtkOpenFOAMReader *parent) { this->Parent = parent; }
  int MakeInformationVector(vtkInformationVector *, const vtkStdString &);
  bool SetTimeValue(const double);
  vtkDoubleArray *GetTimeValues();
  int MakeMetaDataAtTimeStep(const bool);

  //BTX
  friend class vtkOpenFOAMReaderPrivate;
  //ETX

protected:
  // refresh flag
  bool Refresh;

  // for creating cell-to-point translated data
  int CreateCellToPoint;

  // for caching mesh
  int CacheMesh;

  // for decomposing polyhedra on-the-fly
  int DecomposePolyhedra;

  // for reading old binary lagrangian/positions format
  int PositionsIsIn13Format;

  // for reading point/face/cell-Zones
  int ReadZones;

  // determine if time directories are listed according to controlDict
  int ListTimeStepsByControlDict;

  // add dimensions to array names
  int AddDimensionsToArrayNames;

  char *FileName;
  vtkCharArray *CasePath;
  vtkCollection *Readers;

  // DataArraySelection for Patch / Region Data
  vtkDataArraySelection *PatchDataArraySelection;
  vtkDataArraySelection *CellDataArraySelection;
  vtkDataArraySelection *PointDataArraySelection;
  vtkDataArraySelection *LagrangianDataArraySelection;

  // old selection status
  unsigned long int PatchSelectionMTimeOld;
  unsigned long int CellSelectionMTimeOld;
  unsigned long int PointSelectionMTimeOld;
  unsigned long int LagrangianSelectionMTimeOld;

  // preserved old information
  vtkStdString *FileNameOld;
  int ListTimeStepsByControlDictOld;
  int CreateCellToPointOld;
  int DecomposePolyhedraOld;
  int PositionsIsIn13FormatOld;
  int AddDimensionsToArrayNamesOld;
  int ReadZonesOld;

  // paths to Lagrangians
  vtkStringArray *LagrangianPaths;

  // number of reader instances
  int NumberOfReaders;
  // index of the active reader
  int CurrentReaderIndex;

  vtkOpenFOAMReader();
  ~vtkOpenFOAMReader();
  int RequestInformation(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *);
  int RequestData(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *);

  void CreateCasePath(vtkStdString &, vtkStdString &);
  void SetTimeInformation(vtkInformationVector *, vtkDoubleArray *);
  void CreateCharArrayFromString(vtkCharArray *, const char *, vtkStdString &);
  void UpdateStatus();
  void UpdateProgress(double);

private:
  vtkOpenFOAMReader *Parent;

  vtkOpenFOAMReader(const vtkOpenFOAMReader&);  // Not implemented.
  void operator=(const vtkOpenFOAMReader&);  // Not implemented.

  int GetNumberOfSelectionArrays(vtkDataArraySelection *);
  int GetSelectionArrayStatus(vtkDataArraySelection *, const char *);
  void SetSelectionArrayStatus(vtkDataArraySelection *, const char *, int);
  const char *GetSelectionArrayName(vtkDataArraySelection *, int);
  void DisableAllSelectionArrays(vtkDataArraySelection *);
  void EnableAllSelectionArrays(vtkDataArraySelection *);

  void AddSelectionNames(vtkDataArraySelection *, vtkStringArray *);
};

#endif
