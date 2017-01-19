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
/**
 * @class   vtkOpenFOAMReader
 * @brief   reads a dataset in OpenFOAM format
 *
 * vtkOpenFOAMReader creates a multiblock dataset. It reads mesh
 * information and time dependent data.  The polyMesh folders contain
 * mesh information. The time folders contain transient data for the
 * cells. Each folder can contain any number of data files.
 *
 * @par Thanks:
 * Thanks to Terry Jordan of SAIC at the National Energy
 * Technology Laboratory who developed this class.
 * Please address all comments to Terry Jordan (terry.jordan@sa.netl.doe.gov).
 * GUI Based selection of mesh regions and fields available in the case,
 * minor bug fixes, strict memory allocation checks,
 * minor performance enhancements by Philippose Rajan (sarith@rocketmail.com).
 *
 * Token-based FoamFile format lexer/parser,
 * performance/stability/compatibility enhancements, gzipped file
 * support, lagrangian field support, variable timestep support,
 * builtin cell-to-point filter, pointField support, polyhedron
 * decomposition support, OF 1.5 extended format support, multiregion
 * support, old mesh format support, parallelization support for
 * decomposed cases in conjunction with vtkPOpenFOAMReader, et. al. by
 * Takuya Oshima of Niigata University, Japan (oshima@eng.niigata-u.ac.jp).
 *
 * Misc cleanup, bugfixes, improvements
 * Mark Olesen (OpenCFD Ltd.)
*/

#ifndef vtkOpenFOAMReader_h
#define vtkOpenFOAMReader_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkCollection;
class vtkCharArray;
class vtkDataArraySelection;
class vtkDoubleArray;
class vtkStdString;
class vtkStringArray;

class vtkOpenFOAMReaderPrivate;

class VTKIOGEOMETRY_EXPORT vtkOpenFOAMReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkOpenFOAMReader *New();
  vtkTypeMacro(vtkOpenFOAMReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream &, vtkIndent) VTK_OVERRIDE;

  /**
   * Determine if the file can be read with this reader.
   */
  int CanReadFile(const char *);

  //@{
  /**
   * Set/Get the filename.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  /**
   * Get the number of cell arrays available in the input.
   */
  int GetNumberOfCellArrays(void)
  { return this->GetNumberOfSelectionArrays(this->CellDataArraySelection); }

  /**
   * Get/Set whether the cell array with the given name is to
   * be read.
   */
  int GetCellArrayStatus(const char *name)
  { return this->GetSelectionArrayStatus(this->CellDataArraySelection, name); }
  void SetCellArrayStatus(const char *name, int status)
  { this->SetSelectionArrayStatus(this->CellDataArraySelection, name, status); }

  /**
   * Get the name of the  cell array with the given index in
   * the input.
   */
  const char *GetCellArrayName(int index)
  { return this->GetSelectionArrayName(this->CellDataArraySelection, index); }

  /**
   * Turn on/off all cell arrays.
   */
  void DisableAllCellArrays()
  { this->DisableAllSelectionArrays(this->CellDataArraySelection); }
  void EnableAllCellArrays()
  { this->EnableAllSelectionArrays(this->CellDataArraySelection); }

  /**
   * Get the number of point arrays available in the input.
   */
  int GetNumberOfPointArrays(void)
  { return this->GetNumberOfSelectionArrays(this->PointDataArraySelection); }

  /**
   * Get/Set whether the point array with the given name is to
   * be read.
   */
  int GetPointArrayStatus(const char *name)
  { return this->GetSelectionArrayStatus(this->PointDataArraySelection, name); }
  void SetPointArrayStatus(const char *name, int status)
  { this->SetSelectionArrayStatus(this->PointDataArraySelection,
    name, status); }

  /**
   * Get the name of the  point array with the given index in
   * the input.
   */
  const char *GetPointArrayName(int index)
  { return this->GetSelectionArrayName(this->PointDataArraySelection, index); }

  /**
   * Turn on/off all point arrays.
   */
  void DisableAllPointArrays()
  { this->DisableAllSelectionArrays(this->PointDataArraySelection); }
  void EnableAllPointArrays()
  { this->EnableAllSelectionArrays(this->PointDataArraySelection); }

  /**
   * Get the number of Lagrangian arrays available in the input.
   */
  int GetNumberOfLagrangianArrays(void)
  { return this->GetNumberOfSelectionArrays(
    this->LagrangianDataArraySelection); }

  /**
   * Get/Set whether the Lagrangian array with the given name is to
   * be read.
   */
  int GetLagrangianArrayStatus(const char *name)
  { return this->GetSelectionArrayStatus(this->LagrangianDataArraySelection,
    name); }
  void SetLagrangianArrayStatus(const char *name, int status)
  { this->SetSelectionArrayStatus(this->LagrangianDataArraySelection, name,
    status); }

  /**
   * Get the name of the  Lagrangian array with the given index in
   * the input.
   */
  const char* GetLagrangianArrayName(int index)
  { return this->GetSelectionArrayName(this->LagrangianDataArraySelection,
    index); }

  /**
   * Turn on/off all Lagrangian arrays.
   */
  void DisableAllLagrangianArrays()
  { this->DisableAllSelectionArrays(this->LagrangianDataArraySelection); }
  void EnableAllLagrangianArrays()
  { this->EnableAllSelectionArrays(this->LagrangianDataArraySelection); }

  /**
   * Get the number of Patches (including Internal Mesh) available in the input.
   */
  int GetNumberOfPatchArrays(void)
  { return this->GetNumberOfSelectionArrays(this->PatchDataArraySelection); }

  /**
   * Get/Set whether the Patch with the given name is to
   * be read.
   */
  int GetPatchArrayStatus(const char *name)
  { return this->GetSelectionArrayStatus(this->PatchDataArraySelection, name); }
  void SetPatchArrayStatus(const char *name, int status)
  { this->SetSelectionArrayStatus(this->PatchDataArraySelection, name,
    status); }

  /**
   * Get the name of the Patch with the given index in
   * the input.
   */
  const char *GetPatchArrayName(int index)
  { return this->GetSelectionArrayName(this->PatchDataArraySelection, index); }

  /**
   * Turn on/off all Patches including the Internal Mesh.
   */
  void DisableAllPatchArrays()
  { this->DisableAllSelectionArrays(this->PatchDataArraySelection); }
  void EnableAllPatchArrays()
  { this->EnableAllSelectionArrays(this->PatchDataArraySelection); }

  //@{
  /**
   * Set/Get whether to create cell-to-point translated data for cell-type data
   */
  vtkSetMacro(CreateCellToPoint, int);
  vtkGetMacro(CreateCellToPoint, int);
  vtkBooleanMacro(CreateCellToPoint, int);
  //@}

  //@{
  /**
   * Set/Get whether mesh is to be cached.
   */
  vtkSetMacro(CacheMesh, int);
  vtkGetMacro(CacheMesh, int);
  vtkBooleanMacro(CacheMesh, int);
  //@}

  //@{
  /**
   * Set/Get whether polyhedra are to be decomposed.
   */
  vtkSetMacro(DecomposePolyhedra, int);
  vtkGetMacro(DecomposePolyhedra, int);
  vtkBooleanMacro(DecomposePolyhedra, int);
  //@}

  // Option for reading old binary lagrangian/positions format
  //@{
  /**
   * Set/Get whether the lagrangian/positions have additional data or not.
   * For historical reasons, PositionsIsIn13Format is used to denote that
   * the positions only have x,y,z value and the cell of the enclosing cell.
   * In OpenFOAM 1.4-2.4, positions included facei and stepFraction information.
   */
  vtkSetMacro(PositionsIsIn13Format, int);
  vtkGetMacro(PositionsIsIn13Format, int);
  vtkBooleanMacro(PositionsIsIn13Format, int);
  //@}

  //@{
  /**
   * Ignore 0/ time directory, which is normally missing Lagrangian fields
   * and may have many dictionary functionality that we cannot easily handle.
   */
  vtkSetMacro(SkipZeroTime, bool);
  vtkGetMacro(SkipZeroTime, bool);
  vtkBooleanMacro(SkipZeroTime, bool);
  //@}

  //@{
  /**
   * Determine if time directories are to be listed according to controlDict
   */
  vtkSetMacro(ListTimeStepsByControlDict, int);
  vtkGetMacro(ListTimeStepsByControlDict, int);
  vtkBooleanMacro(ListTimeStepsByControlDict, int);
  //@}

  //@{
  /**
   * Add dimensions to array names
   */
  vtkSetMacro(AddDimensionsToArrayNames, int);
  vtkGetMacro(AddDimensionsToArrayNames, int);
  vtkBooleanMacro(AddDimensionsToArrayNames, int);
  //@}

  //@{
  /**
   * Set/Get whether zones will be read.
   */
  vtkSetMacro(ReadZones, int);
  vtkGetMacro(ReadZones, int);
  vtkBooleanMacro(ReadZones, int);
  //@}

  //@{
  /**
   * If true, labels are expected to be 64-bit, rather than 32.
   */
  virtual void SetUse64BitLabels(bool val);
  vtkGetMacro(Use64BitLabels, bool)
  vtkBooleanMacro(Use64BitLabels, bool)
  //@}

  //@{
  /**
   * If true, floats are expected to be 64-bit, rather than 32. Note that
   * vtkFloatArrays may still be used in the output if this is true. This flag
   * is only used to ensure that binary data is correctly parsed.
   */
  virtual void SetUse64BitFloats(bool val);
  vtkGetMacro(Use64BitFloats, bool)
  vtkBooleanMacro(Use64BitFloats, bool)
  //@}

  void SetRefresh() { this->Refresh = true; this->Modified(); }

  void SetParent(vtkOpenFOAMReader *parent) { this->Parent = parent; }
  int MakeInformationVector(vtkInformationVector *, const vtkStdString &);
  bool SetTimeValue(const double);
  vtkDoubleArray *GetTimeValues();
  int MakeMetaDataAtTimeStep(const bool);

  friend class vtkOpenFOAMReaderPrivate;

protected:
  // refresh flag
  bool Refresh;

  // for creating cell-to-point translated data
  int CreateCellToPoint;

  // for caching mesh
  int CacheMesh;

  // for decomposing polyhedra on-the-fly
  int DecomposePolyhedra;

  // for lagrangian/positions without extra data (OF 1.4 - 2.4)
  int PositionsIsIn13Format;

  // for reading point/face/cell-Zones
  int ReadZones;

  // Ignore 0/ directory
  bool SkipZeroTime;

  // determine if time directories are listed according to controlDict
  int ListTimeStepsByControlDict;

  // add dimensions to array names
  int AddDimensionsToArrayNames;

  // Expect label size to be 64-bit integers instead of 32-bit.
  bool Use64BitLabels;

  // Expect float data to be 64-bit floats instead of 32-bit.
  // Note that vtkFloatArrays may still be used -- this just tells the reader how to
  // parse the binary data.
  bool Use64BitFloats;

  char *FileName;
  vtkCharArray *CasePath;
  vtkCollection *Readers;

  // DataArraySelection for Patch / Region Data
  vtkDataArraySelection *PatchDataArraySelection;
  vtkDataArraySelection *CellDataArraySelection;
  vtkDataArraySelection *PointDataArraySelection;
  vtkDataArraySelection *LagrangianDataArraySelection;

  // old selection status
  vtkMTimeType PatchSelectionMTimeOld;
  vtkMTimeType CellSelectionMTimeOld;
  vtkMTimeType PointSelectionMTimeOld;
  vtkMTimeType LagrangianSelectionMTimeOld;

  // preserved old information
  vtkStdString *FileNameOld;
  bool SkipZeroTimeOld;
  int ListTimeStepsByControlDictOld;
  int CreateCellToPointOld;
  int DecomposePolyhedraOld;
  int PositionsIsIn13FormatOld;
  int AddDimensionsToArrayNamesOld;
  int ReadZonesOld;
  bool Use64BitLabelsOld;
  bool Use64BitFloatsOld;

  // paths to Lagrangians
  vtkStringArray *LagrangianPaths;

  // number of reader instances
  int NumberOfReaders;
  // index of the active reader
  int CurrentReaderIndex;

  vtkOpenFOAMReader();
  ~vtkOpenFOAMReader() VTK_OVERRIDE;
  int RequestInformation(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *) VTK_OVERRIDE;
  int RequestData(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *) VTK_OVERRIDE;

  void CreateCasePath(vtkStdString &, vtkStdString &);
  void SetTimeInformation(vtkInformationVector *, vtkDoubleArray *);
  void CreateCharArrayFromString(vtkCharArray *, const char *, vtkStdString &);
  void UpdateStatus();
  void UpdateProgress(double);

private:
  vtkOpenFOAMReader *Parent;

  vtkOpenFOAMReader(const vtkOpenFOAMReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenFOAMReader&) VTK_DELETE_FUNCTION;

  int GetNumberOfSelectionArrays(vtkDataArraySelection *);
  int GetSelectionArrayStatus(vtkDataArraySelection *, const char *);
  void SetSelectionArrayStatus(vtkDataArraySelection *, const char *, int);
  const char *GetSelectionArrayName(vtkDataArraySelection *, int);
  void DisableAllSelectionArrays(vtkDataArraySelection *);
  void EnableAllSelectionArrays(vtkDataArraySelection *);

  void AddSelectionNames(vtkDataArraySelection *, vtkStringArray *);
};

#endif
