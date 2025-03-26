// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef EnSightDataSet_h
#define EnSightDataSet_h

#include "EnSightFile.h"

#include "vtkSmartPointer.h"
#include "vtkTransform.h"

#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArraySelection;
class vtkDataSet;
class vtkFloatArray;
class vtkPartitionedDataSetCollection;
class vtkRectilinearGrid;
class vtkStructuredGrid;
class vtkUniformGrid;
class vtkUnstructuredGrid;
VTK_ABI_NAMESPACE_END

namespace ensight_gold
{
VTK_ABI_NAMESPACE_BEGIN

enum class GridType
{
  Unknown,
  Uniform,
  Rectilinear,
  Curvilinear,
  Unstructured
};

struct GridOptions
{
  GridType Type = GridType::Unknown;
  bool IBlanked = false;
  bool WithGhost = false;
  bool HasRange = false;
};

enum class ElementType
{
  Unknown,
  Point,
  Bar2,
  Bar3,
  Tria3,
  Tria6,
  Quad4,
  Quad8,
  Tetra4,
  Tetra10,
  Pyramid5,
  Pyramid13,
  Penta6,
  Penta15,
  Hexa8,
  Hexa20,
  NSided,
  NFaced,
  GPoint,
  GBar2,
  GBar3,
  GTria3,
  GTria6,
  GQuad4,
  GQuad8,
  GTetra4,
  GTetra10,
  GPyramid5,
  GPyramid13,
  GPenta6,
  GPenta15,
  GHexa8,
  GHexa20,
  GNSided,
  GNFaced
};

struct PartInfo
{
  std::string Name;
  int NumNodes = 0;
  // For structured grids only
  int NumElements = 0;
  std::vector<int> NumElementsPerType;

  PartInfo()
    : NumElementsPerType(static_cast<int>(ElementType::GNFaced) + 1, 0)
  {
  }
};

using PartInfoMapType = std::map<int, PartInfo>;

enum class VariableType
{
  Unknown,
  ConstantPerCase,
  ConstantPerCaseFile,
  ConstantPerPart,
  ScalarPerNode,
  ScalarPerMeasuredNode,
  VectorPerNode,
  VectorPerMeasuredNode,
  TensorSymmPerNode,
  TensorAsymPerNode,
  ComplexScalarPerNode,
  ComplexVectorPerNode,
  ScalarPerElement,
  VectorPerElement,
  TensorSymmPerElement,
  TensorAsymPerElement,
  ComplexScalarPerElement,
  ComplexVectorPerElement
};

struct VariableOptions
{
  VariableType Type;
  std::string Name;
  int Frequency; // only for complex variables
  EnSightFile File;
  EnSightFile ImaginaryFile; // only for complex variables
  std::vector<float> Constants;
};

/**
 * Handles reading a full EnSight Gold dataset. Uses the EnSightFile class for opening the
 * individual files that make up the ensight dataset.
 */
class EnSightDataSet
{
public:
  EnSightDataSet();
  ~EnSightDataSet() = default;

  /**
   * Parses through case file until version information is found.
   * Returns true if the file is an EnSight Gold file
   */
  bool CheckVersion(const char* casefilename);

  /**
   * Parses all sections of a case file to get information such as filenames.
   */
  bool ParseCaseFile(const char* casefilename);

  /**
   * returns a vector containing all time steps in the dataset
   */
  std::vector<double> GetTimeSteps();

  /**
   * Reads Geometry file, caching the data if not transient
   */
  bool ReadGeometry(vtkPartitionedDataSetCollection* output, vtkDataArraySelection* selection);

  /**
   * Reads Measured Geometry file
   */
  bool ReadMeasuredGeometry(
    vtkPartitionedDataSetCollection* output, vtkDataArraySelection* selection);

  /**
   * Read the rigid body file.
   */
  bool ReadRigidBodyGeometryFile();

  /**
   * Only grabs Part (block) information from the Geometry file to be used
   * in a vtkDataArraySelection to enable user to choose which parts to load
   */
  bool GetPartInfo(vtkDataArraySelection* partSelection, vtkDataArraySelection* pointArraySelection,
    vtkDataArraySelection* cellArraySelection, vtkDataArraySelection* fieldArraySelection);

  /**
   * Reads Variable file(s)
   */
  bool ReadVariables(vtkPartitionedDataSetCollection* output, vtkDataArraySelection* partSelection,
    vtkDataArraySelection* pointArraySelection, vtkDataArraySelection* cellArraySelection,
    vtkDataArraySelection* fieldArraySelection);

  /**
   * Returns true if a rigid body file is specified in the case file
   */
  bool HasRigidBodyFile();

  /**
   * Returns true if the time steps specified in the rigid body files should be used
   */
  bool UseRigidBodyTimeSteps();

  /**
   * Get the array of time steps from the rigid body files
   */
  std::vector<double> GetEulerTimeSteps();

  void SetActualTimeValue(double time);

private:
  bool ParseFormatSection();
  void ParseGeometrySection();
  void ParseVariableSection();
  void ParseTimeSection();
  void ParseFileSection();

  std::string GetFullPath(const std::string& fname);
  void SetVariableFileFormat();
  bool IsSectionHeader(std::string line);

  void CreateUniformGridOutput(const GridOptions& opts, vtkUniformGrid* output);
  void CreateRectilinearGridOutput(const GridOptions& opts, vtkRectilinearGrid* output);
  void CreateStructuredGridOutput(const GridOptions& opts, vtkStructuredGrid* output);
  void CreateUnstructuredGridOutput(const GridOptions& opts, vtkUnstructuredGrid* output);

  void PassThroughUniformGrid(const GridOptions& opts, int partId);
  void PassThroughRectilinearGrid(const GridOptions& opts, int partId);
  void PassThroughStructuredGrid(const GridOptions& opts, int partId);
  void PassThroughUnstructuredGrid(const GridOptions& opts, int partId);
  void PassThroughOptionalSections(const GridOptions& opts, int numPts, int numCells);

  int ReadPartId(EnSightFile& file);
  void ReadDimensions(bool hasRange, int dimensions[3], int& numPts, int& numCells);
  void ReadRange(int range[6]);
  void ReadOptionalValues(int numVals, int* data, std::string sectionName = "");
  void CheckForOptionalHeader(const std::string& sectionName);

  void ReadCell(
    ElementType eType, vtkUnstructuredGrid* output, bool padBegin = false, bool padEnd = false);
  void ReadCell(int cellType, int numNodes, vtkUnstructuredGrid* output, bool padBegin = false,
    bool padEnd = false);
  void ReadNSidedSection(int& numElements, vtkUnstructuredGrid* output);
  void ReadNFacedSection(int& numElements, vtkUnstructuredGrid* output);

  void ReadVariableNodes(EnSightFile& file, const std::string& arrayName, int numComponents,
    vtkPartitionedDataSetCollection* output, vtkDataArraySelection* selection,
    bool isComplex = false, bool isReal = true);
  void ReadVariableMeasuredNodes(EnSightFile& file, const std::string& arrayName, int numComponents,
    vtkPartitionedDataSetCollection* output, vtkDataArraySelection* selection);
  void ReadVariableElements(EnSightFile& file, const std::string& arrayName, int numComponents,
    vtkPartitionedDataSetCollection* output, vtkDataArraySelection* selection,
    bool isComplex = false, bool isReal = true);
  vtkSmartPointer<vtkFloatArray> ReadVariableArray(
    EnSightFile& file, const std::string& sectionHeader, vtkIdType numElements, int numComponents);
  void ReadVariableConstantCase(VariableOptions& var, vtkPartitionedDataSetCollection* output);

  void ProcessNodeIds(int numPts, vtkDataSet* output);
  void ProcessElementIds(int numCells, vtkDataSet* output);

  void ProcessGhostCells(int numCells, vtkDataSet* output);

  bool CurrentGeometryFileContainsConnectivity();

  /**
   * Read the euler parameter file for rigid body transformations.
   * If an error occurred, 0 is returned; otherwise 1.
   *
   * Note: only supported for EnSight Gold files
   */
  bool ReadRigidBodyEulerParameterFile(const std::string& path);

  /**
   * Helper method for reading matrices specified in rigid body files
   */
  bool ReadRigidBodyMatrixLines(
    std::string& line, const std::string& transType, vtkTransform* transform, bool& applyToVectors);

  /**
   * Apply rigid body transforms to the specified part, if there are any.
   */
  bool ApplyRigidBodyTransforms(int partId, std::string partName, vtkDataSet* output);

  EnSightFile CaseFile;

  std::string GeometryFileName;
  EnSightFile GeometryFile;
  // set true when at least some part of the geometry needs to be cached
  // use in conjunction with GeometryChangeCoordsOnly
  bool CacheGeometry;
  bool GeometryCached;

  // indicates that changing geometry is only coordinates, not connectivity
  bool GeometryChangeCoordsOnly;

  // zero based time step that contains the connectivity.
  // only used when GeometryChangeCoordsOnly == true
  int GeometryCStep;

  vtkSmartPointer<vtkPartitionedDataSetCollection> Cache;

  std::string MeasuredFileName;
  EnSightFile MeasuredFile;
  int MeasuredPartitionId;

  std::vector<std::string> FilePath;

  bool NodeIdsListed;
  bool ElementIdsListed;

  PartInfoMapType PartInfoMap;
  TimeSetInfoMapType TimeSetInfoMap;
  FileSetInfoMapType FileSetInfoMap;
  std::vector<double> AllTimeSteps;

  std::vector<VariableOptions> Variables;
  double ActualTimeValue;

  std::string RigidBodyFileName;
  EnSightFile RigidBodyFile;
  EnSightFile EETFile;
  // We support only version 2 of rigid body transform files for only ensight gold files.
  // For rigid body transforms, we need to track per part:
  // 1. transforms to be applied before the Euler transformation
  // 2. Information about which data to use in the Euler Transform file (eet file)
  // 3. transforms to be applied after the Euler transformation
  struct PartTransforms
  {
    // Pre and post transforms do not change over time
    // We have to track each transform separately, because some transforms need to be
    // applied to geometry and vectors, while others should only be applied to the geometry
    std::vector<vtkSmartPointer<vtkTransform>> PreTransforms;
    std::vector<bool> PreTransformsApplyToVectors;
    std::vector<vtkSmartPointer<vtkTransform>> PostTransforms;
    std::vector<bool> PostTransformsApplyToVectors;

    // EnSight format requires specifying the eet file per part, but according to the user manual
    // use of different eet files for the same dataset is not actually allowed
    std::string EETFilename;

    // title is related to, but not necessarily a part name. for instance, if you have 4 wheel parts
    // there may only be a single "wheel" title that all wheel parts use, applying the same Euler
    // rotation to all wheels
    std::string EETTransTitle;
  };

  // rigid body files allows for using either part names or part Ids to specify
  // transforms for parts;
  bool UsePartNamesRB;

  // keeps track of all transforms for each part
  // if UsePartNamesRB == true, the key is the part name
  // otherwise, the key name is the partId converted to a string
  std::map<std::string, PartTransforms> RigidBodyTransforms;

  // map time step to the Euler transform for a part
  using TimeToEulerTransMapType = std::map<double, vtkSmartPointer<vtkTransform>>;

  // map a title to all of its Euler transforms
  using TitleToTimeStepMapType = std::map<std::string, TimeToEulerTransMapType>;

  TitleToTimeStepMapType EulerTransformsMap;

  // It's possible for an EnSight dataset to not contain transient data, except for the
  // Euler transforms. In this case, we will populate EulerTimeSteps so we can use it for
  // time information, instead of the usual time set
  bool UseEulerTimeSteps;
  std::vector<double> EulerTimeSteps;
};

VTK_ABI_NAMESPACE_END
} // end namespace ensight_gold

#endif // EnSightDataSet_h
