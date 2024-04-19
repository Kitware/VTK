// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGenericEnSightReader
 * @brief   class to read any type of EnSight files
 *
 * The class vtkGenericEnSightReader allows the user to read an EnSight data
 * set without a priori knowledge of what type of EnSight data set it is.
 */

#ifndef vtkGenericEnSightReader_h
#define vtkGenericEnSightReader_h

#include "vtkIOEnSightModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

#include <string>

VTK_ABI_NAMESPACE_BEGIN
class vtkCallbackCommand;
class vtkDataArrayCollection;
class vtkDataArraySelection;
class vtkIdListCollection;

class TranslationTableType;

// Cell/Point Ids store mode:
// Sparse Mode is supposed to be for a large number of distributed processes (Unstructured)
// Non Sparse Mode is supposed to be for a small number of distributed processes (Unstructured)
// Implicit Mode is for Structured Data
enum EnsightReaderCellIdMode
{
  SINGLE_PROCESS_MODE,
  SPARSE_MODE,
  NON_SPARSE_MODE,
  IMPLICIT_STRUCTURED_MODE
};

class VTKIOENSIGHT_EXPORT vtkGenericEnSightReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkGenericEnSightReader* New();
  vtkTypeMacro(vtkGenericEnSightReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the Case file name.
   */
  void SetCaseFileName(VTK_FILEPATH const char* fileName);
  vtkGetFilePathMacro(CaseFileName);
  ///@}

  ///@{
  /**
   * Set/Get the file path.
   */
  vtkSetFilePathMacro(FilePath);
  vtkGetFilePathMacro(FilePath);
  ///@}

  ///@{
  /**
   * Get the EnSight file version being read.
   */
  vtkGetMacro(EnSightVersion, int);
  ///@}

  ///@{
  /**
   * Get the number of variables listed in the case file.
   */
  vtkGetMacro(NumberOfVariables, int);
  vtkGetMacro(NumberOfComplexVariables, int);
  ///@}

  ///@{
  /**
   * Get the number of variables of a particular type.
   */
  int GetNumberOfVariables(int type); // returns -1 if unknown type specified
  vtkGetMacro(NumberOfScalarsPerNode, int);
  vtkGetMacro(NumberOfVectorsPerNode, int);
  vtkGetMacro(NumberOfTensorsAsymPerNode, int);
  vtkGetMacro(NumberOfTensorsSymmPerNode, int);
  vtkGetMacro(NumberOfScalarsPerElement, int);
  vtkGetMacro(NumberOfVectorsPerElement, int);
  vtkGetMacro(NumberOfTensorsAsymPerElement, int);
  vtkGetMacro(NumberOfTensorsSymmPerElement, int);
  vtkGetMacro(NumberOfScalarsPerMeasuredNode, int);
  vtkGetMacro(NumberOfVectorsPerMeasuredNode, int);
  vtkGetMacro(NumberOfComplexScalarsPerNode, int);
  vtkGetMacro(NumberOfComplexVectorsPerNode, int);
  vtkGetMacro(NumberOfComplexScalarsPerElement, int);
  vtkGetMacro(NumberOfComplexVectorsPerElement, int);
  ///@}

  /**
   * Get the nth description for a non-complex variable.
   */
  const char* GetDescription(int n);

  /**
   * Get the nth description for a complex variable.
   */
  const char* GetComplexDescription(int n);

  /**
   * Get the nth description of a particular variable type.  Returns nullptr if no
   * variable of this type exists in this data set.
   * SCALAR_PER_NODE = 0; VECTOR_PER_NODE = 1;
   * TENSOR_SYMM_PER_NODE = 2; SCALAR_PER_ELEMENT = 3;
   * VECTOR_PER_ELEMENT = 4; TENSOR_SYMM_PER_ELEMENT = 5;
   * SCALAR_PER_MEASURED_NODE = 6; VECTOR_PER_MEASURED_NODE = 7;
   * COMPLEX_SCALAR_PER_NODE = 8; COMPLEX_VECTOR_PER_NODE 9;
   * COMPLEX_SCALAR_PER_ELEMENT = 10; COMPLEX_VECTOR_PER_ELEMENT = 11;
   * TENSOR_ASYM_PER_NODE = 12; TENSOR_ASYM_PER_ELEMENT = 13;
   */
  const char* GetDescription(int n, int type);

  ///@{
  /**
   * Get the variable type of variable n.
   */
  int GetVariableType(int n);
  int GetComplexVariableType(int n);
  ///@}

  ///@{
  /**
   * Set/Get the time value at which to get the value.
   */
  virtual void SetTimeValue(double value);
  vtkGetMacro(TimeValue, double);
  ///@}

  ///@{
  /**
   * Get the minimum or maximum time value for this data set.
   */
  vtkGetMacro(MinimumTimeValue, double);
  vtkGetMacro(MaximumTimeValue, double);
  ///@}

  ///@{
  /**
   * Get the time values per time set
   */
  vtkGetObjectMacro(TimeSets, vtkDataArrayCollection);
  ///@}

  /**
   * Reads the FORMAT part of the case file to determine whether this is an
   * EnSight6 or EnSightGold data set.  Returns an identifier listed in
   * the FileTypes enum or -1 if an error occurred or the file could not
   * be identified as any EnSight type.
   */
  int DetermineEnSightVersion(int quiet = 0);

  ///@{
  /**
   * Set/get the flag for whether to read all the variables
   */
  vtkBooleanMacro(ReadAllVariables, vtkTypeBool);
  vtkSetMacro(ReadAllVariables, vtkTypeBool);
  vtkGetMacro(ReadAllVariables, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Get the data array selection tables used to configure which data
   * arrays are loaded by the reader.
   */
  vtkGetObjectMacro(PointDataArraySelection, vtkDataArraySelection);
  vtkGetObjectMacro(CellDataArraySelection, vtkDataArraySelection);
  ///@}

  ///@{
  /**
   * Get the number of point or cell arrays available in the input.
   */
  int GetNumberOfPointArrays();
  int GetNumberOfCellArrays();
  ///@}

  ///@{
  /**
   * Get the name of the point or cell array with the given index in
   * the input.
   */
  const char* GetPointArrayName(int index);
  const char* GetCellArrayName(int index);
  ///@}

  ///@{
  /**
   * Get/Set whether the point or cell array with the given name is to
   * be read.
   */
  int GetPointArrayStatus(const char* name);
  int GetCellArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);
  void SetCellArrayStatus(const char* name, int status);
  ///@}

  enum FileTypes
  {
    ENSIGHT_6 = 0,
    ENSIGHT_6_BINARY = 1,
    ENSIGHT_GOLD = 2,
    ENSIGHT_GOLD_BINARY = 3,
    ENSIGHT_MASTER_SERVER = 4
  };

  ///@{
  /**
   * Set the byte order of the file (remember, more Unix workstations
   * write big endian whereas PCs write little endian). Default is
   * big endian (since most older PLOT3D files were written by
   * workstations).
   */
  void SetByteOrderToBigEndian();
  void SetByteOrderToLittleEndian();
  vtkSetMacro(ByteOrder, int);
  vtkGetMacro(ByteOrder, int);
  const char* GetByteOrderAsString();
  ///@}

  enum
  {
    FILE_BIG_ENDIAN = 0,
    FILE_LITTLE_ENDIAN = 1,
    FILE_UNKNOWN_ENDIAN = 2
  };

  ///@{
  /**
   * Get the Geometry file name. Made public to allow access from
   * apps requiring detailed info about the Data contents
   */
  vtkGetFilePathMacro(GeometryFileName);
  ///@}

  ///@{
  /**
   * The MeasuredGeometryFile should list particle coordinates
   * from 0->N-1.
   * If a file is loaded where point Ids are listed from 1-N
   * the Id to points reference will be wrong and the data
   * will be generated incorrectly.
   * Setting ParticleCoordinatesByIndex to true will force
   * all Id's to increment from 0->N-1 (relative to their order
   * in the file) and regardless of the actual Id of the point.
   * Warning, if the Points are listed in non sequential order
   * then setting this flag will reorder them.
   */
  vtkSetMacro(ParticleCoordinatesByIndex, vtkTypeBool);
  vtkGetMacro(ParticleCoordinatesByIndex, vtkTypeBool);
  vtkBooleanMacro(ParticleCoordinatesByIndex, vtkTypeBool);
  ///@}

  /**
   * Returns true if the file pointed to by casefilename appears to be a
   * valid EnSight case file.
   */
  static bool IsEnSightFile(VTK_FILEPATH const char* casefilename);

  /**
   * Returns IsEnSightFile() by default, but can be overridden
   */
  virtual int CanReadFile(VTK_FILEPATH const char* casefilename);

  // THIB
  vtkGenericEnSightReader* GetReader() { return this->Reader; }

  ///@{
  /**
   * Get/set to ApplyTetrahedralize.
   * It's used to apply a Tetrahedralize filter to prevent potential non manifold triangles
   * produced by the ensight solver.
   */
  vtkGetMacro(ApplyTetrahedralize, bool);
  vtkSetMacro(ApplyTetrahedralize, bool);
  ///@}

protected:
  vtkGenericEnSightReader();
  ~vtkGenericEnSightReader() override;

  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Clear data structures such that setting a new case file name works.
   * WARNING: Derived classes should call the base version after they clear
   * their own structures.
   */
  virtual void ClearForNewCaseFileName();

  /**
   * Sanitizes filename, removing quotations and removing trailing whitespace.
   */
  void SanitizeFileName(std::string& filename);

  /**
   * Internal function to read in a line up to 256 characters.
   * Returns zero if there was an error.
   */
  int ReadLine(char result[256]);

  /**
   * Internal function to read up to 80 characters from a binary file.
   * Returns zero if there was an error.
   */
  int ReadBinaryLine(char result[80]);

  /**
   * Skip certain non-comment lines that are not needed.
   */
  bool SkipDataLine(char line[256]);

  // Internal function that skips blank lines and reads the 1st
  // non-blank line it finds (up to 256 characters).
  // Returns 0 is there was an error.
  int ReadNextDataLine(char result[256]);

  ///@{
  /**
   * Set the geometry file name.
   */
  vtkSetFilePathMacro(GeometryFileName);
  ///@}

  ///@{
  /**
   * Add a variable description to the appropriate array.
   */
  void AddVariableDescription(const char* description);
  void AddComplexVariableDescription(const char* description);
  ///@}

  ///@{
  /**
   * Add a variable type to the appropriate array.
   */
  void AddVariableType(int variableType);
  void AddComplexVariableType(int variableType);
  ///@}

  /**
   * Replace the wildcards in the geometry file name with appropriate filename
   * numbers as specified in the time set or file set.
   */
  int ReplaceWildcards(char* fileName, int timeSet, int fileSet);

  /**
   * Replace the *'s in the filename with the given filename number.
   */
  static void ReplaceWildcardsHelper(char* fileName, int num);

  // Callback registered with the SelectionObserver.
  static void SelectionModifiedCallback(
    vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  void SelectionModified();

  // Utility to create argument for vtkDataArraySelection::SetArrays.
  char** CreateStringArray(int numStrings);
  void DestroyStringArray(int numStrings, char** strings);

  // Fill the vtkDataArraySelection objects with the current set of
  // EnSight variables.
  void SetDataArraySelectionSetsFromVariables();

  // Fill the vtkDataArraySelection objects with the current set of
  // arrays in the internal EnSight reader.
  void SetDataArraySelectionSetsFromReader();

  // Fill the internal EnSight reader's vtkDataArraySelection objects
  // from those in this object.
  void SetReaderDataArraySelectionSetsFromSelf();

  istream* IS;
  FILE* IFile;
  vtkGenericEnSightReader* Reader;

  char* CaseFileName;
  char* GeometryFileName;
  char* FilePath;

  // array of types (one entry per instance of variable type in case file)
  int* VariableTypes;
  int* ComplexVariableTypes;

  // pointers to lists of descriptions
  char** VariableDescriptions;
  char** ComplexVariableDescriptions;

  int NumberOfVariables;
  int NumberOfComplexVariables;

  // number of file names / descriptions per type
  int NumberOfScalarsPerNode;
  int NumberOfVectorsPerNode;
  int NumberOfTensorsAsymPerNode;
  int NumberOfTensorsSymmPerNode;
  int NumberOfScalarsPerElement;
  int NumberOfVectorsPerElement;
  int NumberOfTensorsAsymPerElement;
  int NumberOfTensorsSymmPerElement;
  int NumberOfScalarsPerMeasuredNode;
  int NumberOfVectorsPerMeasuredNode;
  int NumberOfComplexScalarsPerNode;
  int NumberOfComplexVectorsPerNode;
  int NumberOfComplexScalarsPerElement;
  int NumberOfComplexVectorsPerElement;

  double TimeValue;
  double MinimumTimeValue;
  double MaximumTimeValue;

  // Flag for whether TimeValue has been set.
  int TimeValueInitialized;

  vtkDataArrayCollection* TimeSets;
  virtual void SetTimeSets(vtkDataArrayCollection*);

  vtkTypeBool ReadAllVariables;

  int ByteOrder;
  vtkTypeBool ParticleCoordinatesByIndex;

  // The EnSight file version being read.  Valid after
  // UpdateInformation.  Value is -1 for unknown version.
  int EnSightVersion;

  // The array selections.  These map over the variables and complex
  // variables to hide the details of EnSight behind VTK terminology.
  vtkDataArraySelection* PointDataArraySelection;
  vtkDataArraySelection* CellDataArraySelection;

  // The observer to modify this object when the array selections are
  // modified.
  vtkCallbackCommand* SelectionObserver;

  // Whether the SelectionModified callback should invoke Modified.
  // This is used when we are copying to/from the internal reader.
  int SelectionModifiedDoNotCallModified;

  // Insert a partId and return the 'realId' that should be used.
  int InsertNewPartId(int partId);

  // Wrapper around an stl map
  TranslationTableType* TranslationTable;

  bool ApplyTetrahedralize = false;

private:
  vtkGenericEnSightReader(const vtkGenericEnSightReader&) = delete;
  void operator=(const vtkGenericEnSightReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
