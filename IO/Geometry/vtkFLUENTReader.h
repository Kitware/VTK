// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkFLUENTReader
 * @brief   reads a dataset in Fluent file format
 *
 * vtkFLUENTReader creates an unstructured grid multiblock dataset.
 * It reads .cas (with associated .dat) and .msh files stored in FLUENT native format.
 * When multiple zone sections are defined in the file they are provided in separate blocks.
 * Each zone section can be unselected so that it won't be part of the outputted multiblock dataset.
 *
 * Keep in mind that all intermediate structures are cached by default to avoid re-parsing the file
 * when the zone selections change. If you wish to avoid caching to lower memory usage at the
 * expense of IO performances, you can set CacheData to false.
 *
 * Because of zone sections interdependency in the FLUENT format, some unselected zone sections may
 * still need to be read from the file, even if they are not part of the outputted multiblock. Here
 * is the general file parsing logic:
 * - If any cell zone is enabled, the whole file needs to be read
 * - Otherwise, only the necessary zones are read (nodes, faces, data arrays,...)
 * Therefore, unselecting a zone will not always improve the file's reading
 * time, but will lower the output' size.
 *
 * @par Thanks:
 * Thanks to Brian W. Dotson & Terry E. Jordan (Department of Energy, National
 * Energy Technology Laboratory) & Douglas McCorkle (Iowa State University)
 * who developed this class.
 * Please address all comments to Brian Dotson (brian.dotson@netl.doe.gov) &
 * Terry Jordan (terry.jordan@sa.netl.doe.gov)
 * & Doug McCorkle (mccdo@iastate.edu)
 *
 *
 * @sa
 * vtkGAMBITReader
 */

#ifndef vtkFLUENTReader_h
#define vtkFLUENTReader_h

#include "vtkDeprecation.h"      // For deprecation macro
#include "vtkIOGeometryModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkNew.h" // For vtkNew

#include <set>
#include <unordered_map>

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArraySelection;
class vtkPoints;
class vtkTriangle;
class vtkTetra;
class vtkQuad;
class vtkHexahedron;
class vtkPyramid;
class vtkWedge;
class vtkConvexPointSet;
class vtkUnstructuredGrid;

class VTKIOGEOMETRY_EXPORT vtkFLUENTReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkFLUENTReader* New();
  vtkTypeMacro(vtkFLUENTReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the file name of the Fluent file to read.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Get the total number of cells. The number of cells is only valid after a
   * successful read of the data file is performed. Initial value is 0.
   */
  vtkGetMacro(NumberOfCells, vtkIdType);
  ///@}

  ///@{
  /**
   * Get/Set if the filter should cache the data (i.e. keep the intermediate structures in memory to
   * avoid re-parsing the file). Defaults is true
   */
  vtkGetMacro(CacheData, bool);
  vtkSetMacro(CacheData, bool);
  vtkBooleanMacro(CacheData, bool);
  ///@}

  /**
   * Get the number of cell arrays available in the input.
   */
  int GetNumberOfCellArrays();

  /**
   * Get the name of the cell array with the given index in
   * the input.
   */
  const char* GetCellArrayName(int index);

  ///@{
  /**
   * Get/Set whether the cell array with the given name is to
   * be read.
   */
  int GetCellArrayStatus(const char* name);
  void SetCellArrayStatus(const char* name, int status);
  ///@}

  ///@{
  /**
   * Turn on/off all cell arrays.
   */
  void DisableAllCellArrays();
  void EnableAllCellArrays();
  ///@}

  ///@{
  /**
   * Zone section selection, to determine which zone sections
   * are loaded.
   */
  vtkDataArraySelection* GetZoneSectionSelection();
  ///@}

  ///@{
  /**
   * These methods should be used instead of the SwapBytes methods.
   * They indicate the byte ordering of the file you are trying
   * to read in. These methods will then either swap or not swap
   * the bytes depending on the byte ordering of the machine it is
   * being run on. For example, reading in a BigEndian file on a
   * BigEndian machine will result in no swapping. Trying to read
   * the same file on a LittleEndian machine will result in swapping.
   * As a quick note most UNIX machines are BigEndian while PC's
   * and VAX tend to be LittleEndian. So if the file you are reading
   * in was generated on a VAX or PC, SetDataByteOrderToLittleEndian
   * otherwise SetDataByteOrderToBigEndian. Not used when reading
   * text files.
   */
  void SetDataByteOrderToBigEndian();
  void SetDataByteOrderToLittleEndian();
  int GetDataByteOrder();
  void SetDataByteOrder(int);
  const char* GetDataByteOrderAsString();
  //
  //  Structures
  //
  struct Cell;
  struct Face;
  struct Zone;
  struct ZoneSection;
  struct ScalarDataChunk;
  struct VectorDataChunk;
  struct SubSection;
  ///@}

  /**
   * Get the last modified time of this filter.
   * This time also depends on the the modified
   * time of the internal ZoneSectionSelection instance.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkFLUENTReader();
  ~vtkFLUENTReader() override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  ///@{
  /**
   * Set/Get the byte swapping to explicitly swap the bytes of a file.
   * Not used when reading text files.
   */
  vtkSetMacro(SwapBytes, vtkTypeBool);
  vtkTypeBool GetSwapBytes() { return this->SwapBytes; }
  vtkBooleanMacro(SwapBytes, vtkTypeBool);
  ///@}

  virtual bool OpenCaseFile(const char* filename);
  virtual bool OpenDataFile(const char* filename);
  virtual int GetCaseChunk();
  virtual int GetCaseIndex();
  virtual void LoadVariableNames();
  virtual int GetDataIndex();
  virtual int GetDataChunk();
  virtual void GetSpeciesVariableNames();

  virtual int GetDimension();
  virtual void GetLittleEndianFlag();
  virtual void GetNodesAscii();
  virtual void GetNodesSinglePrecision();
  virtual void GetNodesDoublePrecision();
  virtual void GetCellsAscii();
  virtual void GetCellsBinary();
  virtual bool GetFacesAscii();
  virtual void GetFacesBinary();
  virtual void GetPeriodicShadowFacesAscii();
  virtual void GetPeriodicShadowFacesBinary();
  virtual void GetCellTreeAscii();
  virtual void GetCellTreeBinary();
  virtual void GetFaceTreeAscii();
  virtual void GetFaceTreeBinary();
  virtual void GetInterfaceFaceParentsAscii();
  virtual void GetInterfaceFaceParentsBinary();
  virtual void GetNonconformalGridInterfaceFaceInformationAscii();
  virtual void GetNonconformalGridInterfaceFaceInformationBinary();
  virtual void GetPartitionInfo() {}
  virtual void CleanCells();
  virtual void PopulateCellNodes();
  virtual int GetCaseBufferInt(int ptr);
  virtual float GetCaseBufferFloat(int ptr);
  virtual double GetCaseBufferDouble(int ptr);
  virtual void PopulateTriangleCell(size_t cellIdx);
  virtual void PopulateTetraCell(size_t cellIdx);
  virtual void PopulateQuadCell(size_t cellIdx);
  virtual void PopulateHexahedronCell(size_t cellIdx);
  virtual void PopulatePyramidCell(size_t cellIdx);
  virtual void PopulateWedgeCell(size_t cellIdx);
  virtual void PopulatePolyhedronCell(size_t cellIdx);
  virtual int GetDataBufferInt(int ptr);
  virtual float GetDataBufferFloat(int ptr);
  virtual double GetDataBufferDouble(int ptr);
  virtual void GetData(int dataType);
  virtual bool ParallelCheckCell(int vtkNotUsed(i)) { return true; }

  VTK_DEPRECATED_IN_9_5_0(
    "ReadZone is deprecated. It was an internal method an should not be used.")
  virtual void ReadZone();
  VTK_DEPRECATED_IN_9_5_0(
    "ParseCaseFile is deprecated. It was an internal method an should not be used.")
  virtual bool ParseCaseFile();
  VTK_DEPRECATED_IN_9_5_0(
    "ParseDataFile is deprecated. It was an internal method an should not be used.")
  virtual void ParseDataFile();

private:
  /**
   * Check whether all cell zones are disabled.
   */
  bool AreCellsEnabled();
  /**
   * Fill CurrentCells/CurrentFaces with cells/faces from enabled zone sections.
   */
  void DisableCellsAndFaces(std::vector<unsigned int>& disabledZones);
  /**
   * Disable the zones that belong to disabled zone sections.
   */
  void DisableZones(std::vector<unsigned int>& disabledZones, bool& areAllZonesDisabled);
  /**
   * Fill output multiblock with cells
   */
  bool FillMultiblock(std::vector<unsigned int>& disabledZones,
    std::vector<size_t>& zoneIDToBlockIdx,
    std::vector<vtkSmartPointer<vtkUnstructuredGrid>>& blockUGs);
  /**
   * Fill output multiblock with data scalars and vectors
   */
  void FillMultiblockData(std::vector<unsigned int>& disabledZones,
    std::vector<size_t>& zoneIDToBlockIdx,
    std::vector<vtkSmartPointer<vtkUnstructuredGrid>>& blockUGs);
  /**
   * Get arrays from SubSections.
   */
  void GetArraysFromSubSections();
  /**
   * Create a block per zone section.
   */
  void InitOutputBlocks(vtkMultiBlockDataSet* output, std::vector<size_t>& zoneIDToBlockIdx,
    std::vector<vtkSmartPointer<vtkUnstructuredGrid>>& blockUGs);
  /**
   * Parse the data zone according to its index.
   */
  void ParseDataZone(int index);
  /**
   * Parse all the data zones in DataZones.
   * A data zone is not parsed if:
   * - It is already parsed.
   * - Its zone section is disabled and there are no cell zones enabled
   */
  void ParseDataZones(bool areCellsEnabled);
  /**
   * Parse the zone according to its index.
   */
  void ParseZone(int index);
  /**
   * Parse all the zones in Zones.
   * A zone is not parsed if:
   * - It is already parsed.
   * - Its zone section is disabled and there are no cell zones enabled
   */
  void ParseZones(bool areCellsEnabled);
  /**
   * Parse the data file but only save the zoneId, zoneSectionId, and position of each data zone
   * into DataZones to be fully parsed later.
   */
  bool PreParseDataFile();
  /**
   * Parse the fluent file but only save the zoneId, zoneSectionId, and position of each zone into
   * Zones to be fully parsed later. Zone sections (39 and 45) are read into ZoneSections, @see
   * vtkFLUENTReader::ReadZoneSection
   */
  bool PreParseFluentFile();
  /**
   * Read the zone section id of a data zone
   * zones format: (zoneId (subSectionId zoneSectionId ...
   */
  bool ReadDataZoneSectionId(unsigned int& zoneSectionId);
  /**
   * Read the zone section id of a zone (only nodes, cells, and faces zones have section ids).
   * zones format: (zoneId (zoneSectionId ...
   */
  bool ReadZoneSectionId(unsigned int& zoneSectionId);
  /**
   * Read the header of a zone section to create and save a ZoneSection, allowing its selection or
   * not when creating the output.
   */
  bool ReadZoneSection(int limit);

  /**
   * Add an array to ZoneSectionSelection for each zone section in ZoneSections
   */
  void UpdateZoneSectionSelection();

  /**
   * @brief Create an output multi block dataset using only the faces of the file
   *
   * This function is used to generate an output when reading a FLUENT Mesh file
   * that only contains faces without cells.
   * It supports lines, triangles and quads.
   *
   * @param blockUGs per-bloc unstructured grid objects
   * @param zoneIDToBlockIdx Lookup map used to convert zone ID to block index.
   * @param disabledZones List of disabled zone ids
   */
  void FillMultiBlockFromFaces(std::vector<vtkSmartPointer<vtkUnstructuredGrid>>& blockUGs,
    const std::vector<size_t>& zoneIDToBlockIdx, std::vector<unsigned int> disabledZones);

  vtkFLUENTReader(const vtkFLUENTReader&) = delete;
  void operator=(const vtkFLUENTReader&) = delete;

  //
  //  Variables
  //
  vtkNew<vtkDataArraySelection> ZoneSectionSelection;
  vtkNew<vtkDataArraySelection> CellDataArraySelection;
  char* FileName = nullptr;
  vtkIdType NumberOfCells = 0;
  bool CacheData = true;

  istream* FluentFile = nullptr;
  istream* FluentDataFile = nullptr;
  std::string FluentBuffer;
  std::string DataBuffer;

  // File data cache
  vtkNew<vtkPoints> Points;
  std::vector<Cell> Cells;
  std::vector<Face> Faces;
  std::vector<ZoneSection> ZoneSections;
  std::map<size_t, std::string> VariableNames;
  std::vector<ScalarDataChunk> ScalarDataChunks;
  std::vector<VectorDataChunk> VectorDataChunks;
  std::vector<SubSection> SubSections;

  std::vector<std::string> ScalarVariableNames;
  std::vector<int> ScalarSubSectionIds;
  std::vector<std::string> VectorVariableNames;
  std::vector<int> VectorSubSectionIds;

  vtkTypeBool SwapBytes;
  int GridDimension = 0;
  int NumberOfScalars = 0;
  int NumberOfVectors = 0;

  std::vector<Zone> Zones;
  std::vector<Zone> DataZones;
  std::vector<Cell> CurrentCells;
  std::vector<Face> CurrentFaces;
  std::vector<ZoneSection> CurrentZoneSections;

  bool IsFilePreParsed = false;
};

VTK_ABI_NAMESPACE_END
#endif
