// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkEnSightReader
 * @brief   superclass for EnSight file readers
 */

#ifndef vtkEnSightReader_h
#define vtkEnSightReader_h

#include "vtkGenericEnSightReader.h"
#include "vtkIOEnSightModule.h" // For export macro
#include "vtkSmartPointer.h"    // for vtkSmartPointer

#include <map>    // for std::map
#include <vector> // for std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkDataSetCollection;
class vtkDoubleArray;
class vtkEnSightReaderCellIdsType;
class vtkIdList;
class vtkMultiBlockDataSet;
class vtkTransform;

class VTKIOENSIGHT_EXPORT vtkEnSightReader : public vtkGenericEnSightReader
{
public:
  vtkTypeMacro(vtkEnSightReader, vtkGenericEnSightReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum ElementTypesList
  {
    POINT = 0,
    BAR2 = 1,
    BAR3 = 2,
    NSIDED = 3,
    TRIA3 = 4,
    TRIA6 = 5,
    QUAD4 = 6,
    QUAD8 = 7,
    NFACED = 8,
    TETRA4 = 9,
    TETRA10 = 10,
    PYRAMID5 = 11,
    PYRAMID13 = 12,
    HEXA8 = 13,
    HEXA20 = 14,
    PENTA6 = 15,
    PENTA15 = 16,
    NUMBER_OF_ELEMENT_TYPES = 17
  };

  enum VariableTypesList
  {
    SCALAR_PER_NODE = 0,
    VECTOR_PER_NODE = 1,
    TENSOR_SYMM_PER_NODE = 2,
    SCALAR_PER_ELEMENT = 3,
    VECTOR_PER_ELEMENT = 4,
    TENSOR_SYMM_PER_ELEMENT = 5,
    SCALAR_PER_MEASURED_NODE = 6,
    VECTOR_PER_MEASURED_NODE = 7,
    COMPLEX_SCALAR_PER_NODE = 8,
    COMPLEX_VECTOR_PER_NODE = 9,
    COMPLEX_SCALAR_PER_ELEMENT = 10,
    COMPLEX_VECTOR_PER_ELEMENT = 11,
    TENSOR_ASYM_PER_NODE = 12,
    TENSOR_ASYM_PER_ELEMENT = 13
  };

  enum SectionTypeList
  {
    COORDINATES = 0,
    BLOCK = 1,
    ELEMENT = 2
  };

  ///@{
  /**
   * Get the Measured file name. Made public to allow access from
   * apps requiring detailed info about the Data contents
   */
  vtkGetFilePathMacro(MeasuredFileName);
  ///@}

  ///@{
  /**
   * Get the Match file name. Made public to allow access from
   * apps requiring detailed info about the Data contents
   */
  vtkGetFilePathMacro(MatchFileName);
  ///@}

  ///@{
  /**
   * Get the rigid body file name. Made public to allow access from
   * apps requiring detailed info about the Data contents
   */
  vtkGetFilePathMacro(RigidBodyFileName);
  ///@}

protected:
  vtkEnSightReader();
  ~vtkEnSightReader() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void ClearForNewCaseFileName() override;

  ///@{
  /**
   * Set the Measured file name.
   */
  vtkSetFilePathMacro(MeasuredFileName);
  ///@}

  ///@{
  /**
   * Set the Match file name.
   */
  vtkSetFilePathMacro(MatchFileName);
  ///@}

  ///@{
  /**
   * Set the rigid body file name.
   */
  vtkSetFilePathMacro(RigidBodyFileName);
  ///@}

  ///@{
  /**
   * Read the case file.  If an error occurred, 0 is returned; otherwise 1.
   */
  int ReadCaseFile();
  int ReadCaseFileGeometry(char* line);
  int ReadCaseFileVariable(char* line);
  int ReadCaseFileTime(char* line);
  int ReadCaseFileFile(char* line);
  int ReadCaseFileScripts(char* line);

  ///@}

  // set in UpdateInformation to value returned from ReadCaseFile
  int CaseFileRead;

  /**
   * Read the geometry file.  If an error occurred, 0 is returned; otherwise 1.
   */
  virtual int ReadGeometryFile(
    const char* fileName, int timeStep, vtkMultiBlockDataSet* output) = 0;

  /**
   * Read the measured geometry file.  If an error occurred, 0 is returned;
   * otherwise 1.
   */
  virtual int ReadMeasuredGeometryFile(
    const char* fileName, int timeStep, vtkMultiBlockDataSet* output) = 0;

  /**
   * Read the rigid body file.  If an error occurred, 0 is returned; otherwise 1.
   *
   * Note: only supported for EnSight Gold files
   */
  int ReadRigidBodyGeometryFile();

  /**
   * Read the euler parameter file for rigid body transformations.
   * If an error occurred, 0 is returned; otherwise 1.
   *
   * Note: only supported for EnSight Gold files
   */
  int ReadRigidBodyEulerParameterFile(const char* path);

  /**
   * Helper method for reading matrices specified in rigid body files
   */
  int ReadRigidBodyMatrixLines(char* line, vtkTransform* transform, bool& applyToVectors);

  /**
   * Apply rigid body transforms to the specified part, if there are any.
   *
   * Note: only supported for EnSight Gold files
   */
  int ApplyRigidBodyTransforms(int partId, const char* name, vtkDataSet* output);

  /**
   * Read the variable files. If an error occurred, 0 is returned; otherwise 1.
   */
  int ReadVariableFiles(vtkMultiBlockDataSet* output);

  /**
   * Read scalars per node for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  virtual int ReadScalarsPerNode(const char* fileName, const char* description, int timeStep,
    vtkMultiBlockDataSet* output, int measured = 0, int numberOfComponents = 1,
    int component = 0) = 0;

  /**
   * Read vectors per node for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  virtual int ReadVectorsPerNode(const char* fileName, const char* description, int timeStep,
    vtkMultiBlockDataSet* output, int measured = 0) = 0;

  /**
   * Read asymmetric tensors per node for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  virtual int ReadAsymmetricTensorsPerNode(
    const char* fileName, const char* description, int timeStep, vtkMultiBlockDataSet* output) = 0;

  /**
   * Read tensors per node for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  virtual int ReadTensorsPerNode(
    const char* fileName, const char* description, int timeStep, vtkMultiBlockDataSet* output) = 0;

  /**
   * Read scalars per element for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  virtual int ReadScalarsPerElement(const char* fileName, const char* description, int timeStep,
    vtkMultiBlockDataSet* output, int numberOfComponents = 1, int component = 0) = 0;

  /**
   * Read vectors per element for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  virtual int ReadVectorsPerElement(
    const char* fileName, const char* description, int timeStep, vtkMultiBlockDataSet* output) = 0;

  /**
   * Read asymmetric tensors per element for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  virtual int ReadAsymmetricTensorsPerElement(
    const char* fileName, const char* description, int timeStep, vtkMultiBlockDataSet* output) = 0;

  /**
   * Read tensors per element for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  virtual int ReadTensorsPerElement(
    const char* fileName, const char* description, int timeStep, vtkMultiBlockDataSet* output) = 0;

  /**
   * Read an unstructured part (partId) from the geometry file and create a
   * vtkUnstructuredGrid output.  Return 0 if EOF reached.
   */
  virtual int CreateUnstructuredGridOutput(
    int partId, char line[80], const char* name, vtkMultiBlockDataSet* output) = 0;

  /**
   * Read a structured part from the geometry file and create a
   * vtkStructuredGridOutput.  Return 0 if EOF reached.
   */
  virtual int CreateStructuredGridOutput(
    int partId, char line[80], const char* name, vtkMultiBlockDataSet* output) = 0;

  /**
   * Add another file name to the list for a particular variable type.
   */
  void AddVariableFileName(const char* fileName1, const char* fileName2 = nullptr);

  /**
   * Add another description to the list for a particular variable type.
   */
  void AddVariableDescription(const char* description);

  /**
   * Record the variable type for the variable line just read.
   */
  void AddVariableType();

  /**
   * Determine the element type from a line read a file.  Return -1 for
   * invalid element type.
   */
  int GetElementType(const char* line);

  /**
   * Determine the section type from a line read a file.  Return -1 for
   * invalid section type.
   */
  int GetSectionType(const char* line);

  /**
   * Remove leading blank spaces from a string.
   */
  void RemoveLeadingBlanks(char* line);

  // Get the vtkIdList for the given output index and cell type.
  vtkIdList* GetCellIds(int index, int cellType);

  /**
   * Convenience method use to convert the readers from VTK 5 multiblock API
   * to the current composite data infrastructure.
   */
  void AddToBlock(vtkMultiBlockDataSet* output, unsigned int blockNo, vtkDataSet* dataset);

  /**
   * Convenience method use to convert the readers from VTK 5 multiblock API
   * to the current composite data infrastructure.
   */
  vtkDataSet* GetDataSetFromBlock(vtkMultiBlockDataSet* output, unsigned int blockNo);

  /**
   * Set the name of a block.
   */
  void SetBlockName(vtkMultiBlockDataSet* output, unsigned int blockNo, const char* name);

  char* MeasuredFileName;
  char* MatchFileName; // may not actually be necessary to read this file
  char* RigidBodyFileName;

  // pointer to lists of vtkIdLists (cell ids per element type per part)
  vtkEnSightReaderCellIdsType* CellIds;

  // part ids of unstructured outputs
  vtkIdList* UnstructuredPartIds;

  int VariableMode;

  // pointers to lists of filenames
  char** VariableFileNames; // non-complex
  char** ComplexVariableFileNames;

  // array of time sets
  vtkIdList* VariableTimeSetIds;
  vtkIdList* ComplexVariableTimeSetIds;

  // array of file sets
  vtkIdList* VariableFileSetIds;
  vtkIdList* ComplexVariableFileSetIds;

  // collection of filename numbers per time set
  vtkIdListCollection* TimeSetFileNameNumbers;
  vtkIdList* TimeSetsWithFilenameNumbers;

  // collection of filename numbers per file set
  vtkIdListCollection* FileSetFileNameNumbers;
  vtkIdList* FileSetsWithFilenameNumbers;

  // collection of number of steps per file per file set
  vtkIdListCollection* FileSetNumberOfSteps;

  // ids of the time and file sets
  vtkIdList* TimeSetIds;
  vtkIdList* FileSets;

  int GeometryTimeSet;
  int GeometryFileSet;
  int MeasuredTimeSet;
  int MeasuredFileSet;

  double GeometryTimeValue;
  double MeasuredTimeValue;

  vtkTypeBool UseTimeSets;
  vtkSetMacro(UseTimeSets, vtkTypeBool);
  vtkGetMacro(UseTimeSets, vtkTypeBool);
  vtkBooleanMacro(UseTimeSets, vtkTypeBool);

  vtkTypeBool UseFileSets;
  vtkSetMacro(UseFileSets, vtkTypeBool);
  vtkGetMacro(UseFileSets, vtkTypeBool);
  vtkBooleanMacro(UseFileSets, vtkTypeBool);

  int NumberOfGeometryParts;

  // global list of points for measured geometry
  int NumberOfMeasuredPoints;

  int NumberOfNewOutputs;
  int InitialRead;

  int CheckOutputConsistency();

  double ActualTimeValue;

  // We support only version 2 of rigid body transform files for only ensight gold files,
  // but it's implemented here, so we don't need to duplicate implementation for ASCII
  // and binary readers (the erb and eet files are always in ASCII).
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
  vtkSmartPointer<vtkDoubleArray> EulerTimeSteps;

private:
  vtkEnSightReader(const vtkEnSightReader&) = delete;
  void operator=(const vtkEnSightReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
