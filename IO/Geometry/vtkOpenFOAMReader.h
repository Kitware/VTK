// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
 * Thanks to Terry Jordan (terry.jordan@sa.netl.doe.gov) of SAIC
 * at the National Energy Technology Laboratory who originally
 * developed this class.
 *
 * Takuya Oshima of Niigata University, Japan (oshima@eng.niigata-u.ac.jp)
 * provided the major bulk of improvements (rewrite) that made the reader
 * truly functional and included the following features:
 * Token-based FoamFile format lexer/parser,
 * performance/stability/compatibility enhancements, gzipped file
 * support, lagrangian field support, variable timestep support,
 * builtin cell-to-point filter, pointField support, polyhedron
 * decomposition support, multiregion support, parallelization support for
 * decomposed cases in conjunction with vtkPOpenFOAMReader etc.
 *
 * Philippose Rajan (sarith@rocketmail.com) added
 * GUI-based selection of mesh regions and fields available in the case,
 * minor bug fixes, strict memory allocation checks,
 *
 * Mark Olesen (OpenCFD Ltd.) www.openfoam.com
 * has provided various bugfixes, improvements, cleanup
 */

#ifndef vtkOpenFOAMReader_h
#define vtkOpenFOAMReader_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

#include <mutex> // For std::mutex

VTK_ABI_NAMESPACE_BEGIN
class vtkCollection;
class vtkCharArray;
class vtkDataArraySelection;
class vtkDoubleArray;
class vtkStringArray;
class vtkTable;
class vtkUnsignedCharArray;

class vtkOpenFOAMReaderPrivate;

#define VTK_OPENFOAM_TIME_PROFILING 0

class VTKIOGEOMETRY_EXPORT vtkOpenFOAMReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  // Access for implementation class
  friend class vtkOpenFOAMReaderPrivate;

  static vtkOpenFOAMReader* New();
  vtkTypeMacro(vtkOpenFOAMReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Determine if the file can be read with this reader.
   */
  int CanReadFile(VTK_FILEPATH const char*);

  ///@{
  /**
   * Set/Get the filename.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Set/Get If sequential (instead of multithreaded) processing is utilized for reading each case
   * files.
   *
   * Defaults to on. Off is usually better for reading data on local drives. Enable to
   * potentially improve performance reading files from high-latency network drives.
   */
  vtkSetMacro(SequentialProcessing, bool);
  vtkGetMacro(SequentialProcessing, bool);
  vtkBooleanMacro(SequentialProcessing, bool);
  ///@}

  /**
   * Get the CellDataArraySelection object.
   */
  vtkGetObjectMacro(CellDataArraySelection, vtkDataArraySelection);

  /**
   * Get the number of cell arrays available in the input.
   */
  int GetNumberOfCellArrays()
  {
    return this->GetNumberOfSelectionArrays(this->CellDataArraySelection);
  }

  /**
   * Get/Set whether the cell array with the given name is to
   * be read.
   */
  int GetCellArrayStatus(const char* name)
  {
    return this->GetSelectionArrayStatus(this->CellDataArraySelection, name);
  }
  void SetCellArrayStatus(const char* name, int status)
  {
    this->SetSelectionArrayStatus(this->CellDataArraySelection, name, status);
  }

  /**
   * Get the name of the cell array with the given index in
   * the input.
   */
  const char* GetCellArrayName(int index)
  {
    return this->GetSelectionArrayName(this->CellDataArraySelection, index);
  }

  /**
   * Turn on/off all cell arrays.
   */
  void DisableAllCellArrays() { this->DisableAllSelectionArrays(this->CellDataArraySelection); }
  void EnableAllCellArrays() { this->EnableAllSelectionArrays(this->CellDataArraySelection); }

  /**
   * Get the PointDataArraySelection object.
   */
  vtkGetObjectMacro(PointDataArraySelection, vtkDataArraySelection);

  /**
   * Get the number of point arrays available in the input.
   */
  int GetNumberOfPointArrays()
  {
    return this->GetNumberOfSelectionArrays(this->PointDataArraySelection);
  }

  /**
   * Get/Set whether the point array with the given name is to
   * be read.
   */
  int GetPointArrayStatus(const char* name)
  {
    return this->GetSelectionArrayStatus(this->PointDataArraySelection, name);
  }
  void SetPointArrayStatus(const char* name, int status)
  {
    this->SetSelectionArrayStatus(this->PointDataArraySelection, name, status);
  }

  /**
   * Get the name of the point array with the given index in
   * the input.
   */
  const char* GetPointArrayName(int index)
  {
    return this->GetSelectionArrayName(this->PointDataArraySelection, index);
  }

  /**
   * Turn on/off all point arrays.
   */
  void DisableAllPointArrays() { this->DisableAllSelectionArrays(this->PointDataArraySelection); }
  void EnableAllPointArrays() { this->EnableAllSelectionArrays(this->PointDataArraySelection); }

  /**
   * Get the PointDataArraySelection object.
   */
  vtkGetObjectMacro(LagrangianDataArraySelection, vtkDataArraySelection);

  /**
   * Get the number of Lagrangian arrays available in the input.
   */
  int GetNumberOfLagrangianArrays()
  {
    return this->GetNumberOfSelectionArrays(this->LagrangianDataArraySelection);
  }

  /**
   * Get/Set whether the Lagrangian array with the given name is to
   * be read.
   */
  int GetLagrangianArrayStatus(const char* name)
  {
    return this->GetSelectionArrayStatus(this->LagrangianDataArraySelection, name);
  }
  void SetLagrangianArrayStatus(const char* name, int status)
  {
    this->SetSelectionArrayStatus(this->LagrangianDataArraySelection, name, status);
  }

  /**
   * Get the name of the Lagrangian array with the given index in
   * the input.
   */
  const char* GetLagrangianArrayName(int index)
  {
    return this->GetSelectionArrayName(this->LagrangianDataArraySelection, index);
  }

  /**
   * Turn on/off all Lagrangian arrays.
   */
  void DisableAllLagrangianArrays()
  {
    this->DisableAllSelectionArrays(this->LagrangianDataArraySelection);
  }
  void EnableAllLagrangianArrays()
  {
    this->EnableAllSelectionArrays(this->LagrangianDataArraySelection);
  }

  /**
   * Get the PatchDataArraySelection object.
   */
  vtkGetObjectMacro(PatchDataArraySelection, vtkDataArraySelection);

  /**
   * Get the number of Patches (including Internal Mesh) available in the input.
   */
  int GetNumberOfPatchArrays()
  {
    return this->GetNumberOfSelectionArrays(this->PatchDataArraySelection);
  }

  /**
   * Get/Set whether the Patch with the given name is to
   * be read.
   */
  int GetPatchArrayStatus(const char* name)
  {
    return this->GetSelectionArrayStatus(this->PatchDataArraySelection, name);
  }
  void SetPatchArrayStatus(const char* name, int status)
  {
    this->SetSelectionArrayStatus(this->PatchDataArraySelection, name, status);
  }

  /**
   * Get the name of the Patch with the given index in
   * the input.
   */
  const char* GetPatchArrayName(int index)
  {
    return this->GetSelectionArrayName(this->PatchDataArraySelection, index);
  }

  /**
   * Turn on/off all Patches including the Internal Mesh.
   */
  void DisableAllPatchArrays() { this->DisableAllSelectionArrays(this->PatchDataArraySelection); }
  void EnableAllPatchArrays() { this->EnableAllSelectionArrays(this->PatchDataArraySelection); }

  ///@{
  /**
   * Set/Get whether to create cell-to-point translated data for cell-type data
   */
  vtkSetMacro(CreateCellToPoint, vtkTypeBool);
  vtkGetMacro(CreateCellToPoint, vtkTypeBool);
  vtkBooleanMacro(CreateCellToPoint, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get whether to weigh cell to point averaging by size of cells (only meaningful when
   * CreateCellToPoint is true)
   *
   * @sa
   * CreateCellToPoint
   */
  vtkSetMacro(SizeAverageCellToPoint, vtkTypeBool);
  vtkGetMacro(SizeAverageCellToPoint, vtkTypeBool);
  vtkBooleanMacro(SizeAverageCellToPoint, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get whether mesh is to be cached.
   */
  vtkSetMacro(CacheMesh, vtkTypeBool);
  vtkGetMacro(CacheMesh, vtkTypeBool);
  vtkBooleanMacro(CacheMesh, vtkTypeBool);
  ///@}

  // Option for reading old binary lagrangian/positions format
  ///@{
  /**
   * Set/Get whether the lagrangian/positions have additional data or not.
   * For historical reasons, PositionsIsIn13Format is used to denote that
   * the positions only have x,y,z value and the cell of the enclosing cell.
   * In OpenFOAM 1.4-2.4, positions included facei and stepFraction information.
   */
  vtkSetMacro(PositionsIsIn13Format, vtkTypeBool);
  vtkGetMacro(PositionsIsIn13Format, vtkTypeBool);
  vtkBooleanMacro(PositionsIsIn13Format, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Ignore 0/ time directory, which is normally missing Lagrangian fields
   * and may have many dictionary functionality that we cannot easily handle.
   */
  vtkSetMacro(SkipZeroTime, bool);
  vtkGetMacro(SkipZeroTime, bool);
  vtkBooleanMacro(SkipZeroTime, bool);
  ///@}

  ///@{
  /**
   * Determine if time directories are to be listed according to controlDict
   */
  vtkSetMacro(ListTimeStepsByControlDict, vtkTypeBool);
  vtkGetMacro(ListTimeStepsByControlDict, vtkTypeBool);
  vtkBooleanMacro(ListTimeStepsByControlDict, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Add dimensions to array names
   */
  vtkSetMacro(AddDimensionsToArrayNames, vtkTypeBool);
  vtkGetMacro(AddDimensionsToArrayNames, vtkTypeBool);
  vtkBooleanMacro(AddDimensionsToArrayNames, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get whether zones will be read.
   */
  vtkSetMacro(ReadZones, vtkTypeBool);
  vtkGetMacro(ReadZones, vtkTypeBool);
  vtkBooleanMacro(ReadZones, vtkTypeBool);
  ///@}

  ///@{
  /**
   * If true, labels are expected to be 64-bit, rather than 32.
   */
  virtual void SetUse64BitLabels(bool val);
  vtkGetMacro(Use64BitLabels, bool);
  vtkBooleanMacro(Use64BitLabels, bool);
  ///@}

  ///@{
  /**
   * If true, data of the internal mesh are copied to the cell zones.
   * Default is false.
   */
  vtkGetMacro(CopyDataToCellZones, bool);
  vtkSetMacro(CopyDataToCellZones, bool);
  vtkBooleanMacro(CopyDataToCellZones, bool);
  ///@}

  ///@{
  /**
   * If true, floats are expected to be 64-bit, rather than 32. Note that
   * vtkFloatArrays may still be used in the output if this is true. This flag
   * is only used to ensure that binary data is correctly parsed.
   */
  virtual void SetUse64BitFloats(bool val);
  vtkGetMacro(Use64BitFloats, bool);
  vtkBooleanMacro(Use64BitFloats, bool);
  ///@}

  ///@{
  /**
   * Set/Get whether the restart files (with name ending by "_0")
   * should be ignored.
   * Default is true.
   */
  vtkGetMacro(IgnoreRestartFiles, bool);
  vtkSetMacro(IgnoreRestartFiles, bool);
  vtkBooleanMacro(IgnoreRestartFiles, bool);
  ///@}

  void SetRefresh()
  {
    this->Refresh = true;
    this->Modified();
  }

  void SetParent(vtkOpenFOAMReader* parent) { this->Parent = parent; }

#ifndef __VTK_WRAP__
  int MakeInformationVector(vtkInformationVector*, const vtkStdString& procDirName,
    vtkStringArray* timeNames = nullptr, vtkDoubleArray* timeValues = nullptr,
    const std::vector<vtkSmartPointer<vtkUnsignedCharArray>>&
      populateMeshIndicesFileChecksPerPrivateReader = {});
#endif

  double GetTimeValue() const;
  bool SetTimeValue(double);
  vtkStringArray* GetTimeNames();
  vtkDoubleArray* GetTimeValues();

#ifndef __VTK_WRAP__
  std::vector<vtkSmartPointer<vtkUnsignedCharArray>> GetPopulateMeshIndicesFileChecksPerReader();
  std::vector<vtkSmartPointer<vtkTable>> GetMarshalledMetadataPerReader();
  void SetMarshalledMetadataPerReader(const std::vector<vtkSmartPointer<vtkTable>>&);
#endif

  int MakeMetaDataAtTimeStep(bool listNextTimeStep, bool skipComputingMetaData = false);

  /**
   * Compute the progress of the reader.
   */
  virtual double ComputeProgress();

#if VTK_OPENFOAM_TIME_PROFILING
  long long GetRequestInformationTimeInMicroseconds() const;
  long long GetRequestDataTimeInMicroseconds() const;
  size_t GetRequestInformationBytes() const;
  size_t GetRequestDataBytes() const;
  virtual void InitializeRequestInformation();
  virtual void InitializeRequestData();
  virtual void PrintRequestInformation();
  virtual void PrintRequestData();
#endif

protected:
  // refresh flag
  bool Refresh;

  bool SequentialProcessing;

  // for creating cell-to-point translated data
  vtkTypeBool CreateCellToPoint;

  // for running size average for cell to point calculation
  vtkTypeBool SizeAverageCellToPoint = false;

  // for caching mesh
  vtkTypeBool CacheMesh;

  // for decomposing polyhedra on-the-fly
  vtkTypeBool DecomposePolyhedra;
  vtkGetMacro(DecomposePolyhedra, vtkTypeBool);

  // for lagrangian/positions without extra data (OF 1.4 - 2.4)
  vtkTypeBool PositionsIsIn13Format;

  // for reading point/face/cell-Zones
  vtkTypeBool ReadZones;

  // Ignore 0/ directory
  bool SkipZeroTime;

  // determine if time directories are listed according to controlDict
  vtkTypeBool ListTimeStepsByControlDict;

  // add dimensions to array names
  vtkTypeBool AddDimensionsToArrayNames;

  // Expect label size to be 64-bit integers instead of 32-bit.
  bool Use64BitLabels;

  // Expect float data to be 64-bit floats instead of 32-bit.
  // Note that vtkFloatArrays may still be used -- this just tells the reader how to
  // parse the binary data.
  bool Use64BitFloats;

  // The data of internal mesh are copied to cell zones
  bool CopyDataToCellZones;

  char* FileName;
  vtkCharArray* CasePath;
  std::vector<vtkSmartPointer<vtkObject>> Readers;

  // DataArraySelection for Patch / Region Data
  vtkDataArraySelection* PatchDataArraySelection;
  vtkDataArraySelection* CellDataArraySelection;
  vtkDataArraySelection* PointDataArraySelection;
  vtkDataArraySelection* LagrangianDataArraySelection;

  // old selection status
  vtkMTimeType PatchSelectionMTimeOld;
  vtkGetMacro(PatchSelectionMTimeOld, vtkMTimeType);
  vtkMTimeType CellSelectionMTimeOld;
  vtkGetMacro(CellSelectionMTimeOld, vtkMTimeType);
  vtkMTimeType PointSelectionMTimeOld;
  vtkGetMacro(PointSelectionMTimeOld, vtkMTimeType);
  vtkMTimeType LagrangianSelectionMTimeOld;
  vtkGetMacro(LagrangianSelectionMTimeOld, vtkMTimeType);

  // preserved old information
  std::string FileNameOld;
  vtkGetMacro(FileNameOld, std::string);
  bool SkipZeroTimeOld;
  vtkGetMacro(SkipZeroTimeOld, bool);
  int ListTimeStepsByControlDictOld;
  vtkGetMacro(ListTimeStepsByControlDictOld, int);
  int CreateCellToPointOld;
  vtkGetMacro(CreateCellToPointOld, int);
  int DecomposePolyhedraOld;
  vtkGetMacro(DecomposePolyhedraOld, int);
  int PositionsIsIn13FormatOld;
  vtkGetMacro(PositionsIsIn13FormatOld, int);
  int AddDimensionsToArrayNamesOld;
  vtkGetMacro(AddDimensionsToArrayNamesOld, int);
  int ReadZonesOld;
  vtkGetMacro(ReadZonesOld, int);
  bool Use64BitLabelsOld;
  vtkGetMacro(Use64BitLabelsOld, bool);
  bool Use64BitFloatsOld;
  vtkGetMacro(Use64BitFloatsOld, bool);

  vtkOpenFOAMReader();
  ~vtkOpenFOAMReader() override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void CreateCasePath(vtkStdString&, vtkStdString&);
  void SetTimeInformation(vtkInformationVector*, vtkDoubleArray*);
  void CreateCharArrayFromString(vtkCharArray*, const char*, vtkStdString&);
  void UpdateStatus();
  void UpdateProgress(vtkOpenFOAMReaderPrivate* reader, double progress);

private:
  vtkOpenFOAMReader* Parent;

  vtkOpenFOAMReader(const vtkOpenFOAMReader&) = delete;
  void operator=(const vtkOpenFOAMReader&) = delete;

  int GetNumberOfSelectionArrays(vtkDataArraySelection*);
  int GetSelectionArrayStatus(vtkDataArraySelection*, const char*);
  void SetSelectionArrayStatus(vtkDataArraySelection*, const char*, int);
  const char* GetSelectionArrayName(vtkDataArraySelection*, int);
  void DisableAllSelectionArrays(vtkDataArraySelection*);
  void EnableAllSelectionArrays(vtkDataArraySelection*);

  void AddSelectionNames(vtkDataArraySelection*, vtkStringArray*);

  // Print some time information (names, current time-step)
  void PrintTimes(std::ostream& os, vtkIndent indent = vtkIndent(), bool full = false) const;

  std::mutex ArraySelectionMutex;
  std::mutex ProgressMutex;

  // Ignore files with name ending by "_0".
  bool IgnoreRestartFiles = true;

#if VTK_OPENFOAM_TIME_PROFILING
  long long RequestInformationTimeInMicroseconds = 0;
  size_t RequestDataBytes = 0;
  long long RequestDataTimeInMicroseconds = 0;
  size_t RequestInformationBytes = 0;
#endif
};

VTK_ABI_NAMESPACE_END
#endif
