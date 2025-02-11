// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkEnSightGoldBinaryReader
 * @brief   class to read binary EnSight Gold files
 *
 * vtkEnSightGoldBinaryReader is a class to read EnSight Gold files into vtk.
 * Because the different parts of the EnSight data can be of various data
 * types, this reader produces multiple outputs, one per part in the input
 * file.
 * All variable information is being stored in field data.  The descriptions
 * listed in the case file are used as the array names in the field data.
 * For complex vector variables, the description is appended with _r (for the
 * array of real values) and _i (for the array if imaginary values).  Complex
 * scalar variables are stored as a single array with 2 components, real and
 * imaginary, listed in that order.
 * @warning
 * You must manually call Update on this reader and then connect the rest
 * of the pipeline because (due to the nature of the file format) it is
 * not possible to know ahead of time how many outputs you will have or
 * what types they will be.
 * This reader can only handle static EnSight datasets (both static geometry
 * and variables).
 * @par Thanks:
 * Thanks to Yvan Fournier for providing the code to support nfaced elements.
 */

#ifndef vtkEnSightGoldBinaryReader_h
#define vtkEnSightGoldBinaryReader_h

#include "vtkEnSightReader.h"
#include "vtkIOEnSightModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiBlockDataSet;

class VTKIOENSIGHT_EXPORT vtkEnSightGoldBinaryReader : public vtkEnSightReader
{
public:
  static vtkEnSightGoldBinaryReader* New();
  vtkTypeMacro(vtkEnSightGoldBinaryReader, vtkEnSightReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkEnSightGoldBinaryReader();
  ~vtkEnSightGoldBinaryReader() override;

  // Returns 1 if successful.  Sets file size as a side action.
  int OpenFile(const char* filename);

  // Returns 1 if successful.  Handles constructing the filename, opening the file and checking
  // if it's binary
  int InitializeFile(const char* filename);

  /**
   * Read the geometry file.  If an error occurred, 0 is returned; otherwise 1.
   */
  int ReadGeometryFile(const char* fileName, int timeStep, vtkMultiBlockDataSet* output) override;

  /**
   * Read the measured geometry file.  If an error occurred, 0 is returned;
   * otherwise 1.
   */
  int ReadMeasuredGeometryFile(
    const char* fileName, int timeStep, vtkMultiBlockDataSet* output) override;

  /**
   * Read scalars per node for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.  If there will be more than one component in
   * the data array, it is assumed that 0 is the first component added.
   */
  int ReadScalarsPerNode(const char* fileName, const char* description, int timeStep,
    vtkMultiBlockDataSet* output, int measured = 0, int numberOfComponents = 1,
    int component = 0) override;

  /**
   * Read vectors per node for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  int ReadVectorsPerNode(const char* fileName, const char* description, int timeStep,
    vtkMultiBlockDataSet* output, int measured = 0) override;

  /**
   * Read asymmetric tensors per node for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  int ReadAsymmetricTensorsPerNode(const char* fileName, const char* description, int timeStep,
    vtkMultiBlockDataSet* output) override;

  /**
   * Read tensors per node for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  int ReadTensorsPerNode(const char* fileName, const char* description, int timeStep,
    vtkMultiBlockDataSet* output) override;

  /**
   * Read scalars per element for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.  If there will be more than one component in the
   * data array, it is assumed that 0 is the first component added.
   */
  int ReadScalarsPerElement(const char* fileName, const char* description, int timeStep,
    vtkMultiBlockDataSet* output, int numberOfComponents = 1, int component = 0) override;

  /**
   * Read vectors per element for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  int ReadVectorsPerElement(const char* fileName, const char* description, int timeStep,
    vtkMultiBlockDataSet* output) override;

  /**
   * Read asymmetric tensors per element for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  int ReadAsymmetricTensorsPerElement(const char* fileName, const char* description, int timeStep,
    vtkMultiBlockDataSet* output) override;

  /**
   * Read tensors per element for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  int ReadTensorsPerElement(const char* fileName, const char* description, int timeStep,
    vtkMultiBlockDataSet* output) override;

  /**
   * Read an unstructured part (partId) from the geometry file and create a
   * vtkUnstructuredGrid output.  Return 0 if EOF reached. Return -1 if
   * an error occurred.
   */
  int CreateUnstructuredGridOutput(
    int partId, char line[80], const char* name, vtkMultiBlockDataSet* output) override;

  /**
   * Read a structured part from the geometry file and create a
   * vtkStructuredGrid output.  Return 0 if EOF reached.
   */
  int CreateStructuredGridOutput(
    int partId, char line[80], const char* name, vtkMultiBlockDataSet* output) override;

  /**
   * Read a structured part from the geometry file and create a
   * vtkRectilinearGrid output.  Return 0 if EOF reached.
   */
  int CreateRectilinearGridOutput(
    int partId, char line[80], const char* name, vtkMultiBlockDataSet* output);

  /**
   * Read a structured part from the geometry file and create a
   * vtkImageData output.  Return 0 if EOF reached.
   */
  int CreateImageDataOutput(
    int partId, char line[80], const char* name, vtkMultiBlockDataSet* output);

  /**
   * Internal function to read in a line up to 80 characters.
   * Returns zero if there was an error.
   */
  int ReadLine(char result[80]);

  ///@{
  /**
   * Internal function to read in a single integer.
   * Returns zero if there was an error.
   */
  template <typename T>
  int ReadInt(T* result);
  int ReadPartId(int* result);
  ///@}

  /**
   * Internal function to read a single float.
   * Returns zero if there was an error.
   */
  int ReadFloat(float* result);

  /**
   * Internal function to read in an integer array.
   * Returns zero if there was an error.
   */
  int ReadIntArray(int* result, vtkIdType numInts);

  /**
   * Internal function to read in a single long.
   * Returns zero if there was an error.
   */
  int ReadLong(vtkTypeInt64* result);

  /**
   * Internal function to read in a float array.
   * Returns zero if there was an error.
   */
  int ReadFloatArray(float* result, vtkIdType numFloats);

  /**
   * Counts the number of timesteps in the geometry file
   * This function assumes the file is already open and returns the
   * number of timesteps remaining in the file
   * The file will be closed after calling this method
   */
  int CountTimeSteps();

  ///@{
  /**
   * Read to the next time step in the geometry file.
   */
  int SkipTimeStep();
  int SkipStructuredGrid(char line[256]);
  int SkipUnstructuredGrid(char line[256]);
  int SkipRectilinearGrid(char line[256]);
  int SkipImageData(char line[256]);
  ///@}

  /**
   * Seeks the IFile to the nearest time step that is <= the target time step
   */
  int SeekToCachedTimeStep(const char* fileName, int realTimeStep);

  /**
   * Add an entry the time step cache
   */
  void AddTimeStepToCache(const char* fileName, int realTimeStep, vtkTypeInt64 address);

  /**
   * Read the file index, if available, and add it to the time step cache
   */
  void AddFileIndexToCache(const char* fileName);

  int NodeIdsListed;
  int ElementIdsListed;
  int Fortran;
  int FortranSkipBytes; // Number of bytes to skip when seeking within a fortran-written file

  istream* GoldIFile;
  // The size of the file could be used to choose byte order.
  vtkTypeUInt64 FileSize;

  class FileOffsetMapInternal;
  FileOffsetMapInternal* FileOffsets;

private:
  int SizeOfInt;
  vtkEnSightGoldBinaryReader(const vtkEnSightGoldBinaryReader&) = delete;
  void operator=(const vtkEnSightGoldBinaryReader&) = delete;

  /**
   * Opens a variable file name. This will compute the full path and then open
   * it. `variableType` is simply used to report helpful error messages.
   */
  bool OpenVariableFile(const char* fname, const char* variableType);

  /**
   * Jump forward to a particular timestep in the variable file, if
   * applicable.
   */
  bool SkipToTimeStep(const char* fileName, int timeStep, vtkMultiBlockDataSet* compositeOutput,
    int attributeType, int numComponents, bool measured);

  /**
   * Reads measured data from a variable file.
   */
  bool ReadMeasureVariableArray(
    const char* description, vtkMultiBlockDataSet* compositeOutput, int numComponents);

  bool ReadVariableArray(const char* description, vtkMultiBlockDataSet* compositeOutput,
    int attributeType, int numComponents, int component = -1);

  class vtkUtilities;
  friend class vtkUtilities;
};

VTK_ABI_NAMESPACE_END
#endif
