/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEnSightGoldReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkEnSightGoldReader
 * @brief   class to read EnSight Gold files
 *
 * vtkEnSightGoldReader is a class to read EnSight Gold files into vtk.
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

#ifndef vtkEnSightGoldReader_h
#define vtkEnSightGoldReader_h

#include "vtkIOEnSightModule.h" // For export macro
#include "vtkEnSightReader.h"


class vtkMultiBlockDataSet;

class VTKIOENSIGHT_EXPORT vtkEnSightGoldReader : public vtkEnSightReader
{
public:
  static vtkEnSightGoldReader *New();
  vtkTypeMacro(vtkEnSightGoldReader, vtkEnSightReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkEnSightGoldReader();
  ~vtkEnSightGoldReader() VTK_OVERRIDE;

  /**
   * Read the geometry file.  If an error occurred, 0 is returned; otherwise 1.
   */
  int ReadGeometryFile(const char* fileName, int timeStep,
    vtkMultiBlockDataSet *output) VTK_OVERRIDE;

  /**
   * Read the measured geometry file.  If an error occurred, 0 is returned;
   * otherwise 1.
   */
  int ReadMeasuredGeometryFile(const char* fileName, int timeStep,
    vtkMultiBlockDataSet *output) VTK_OVERRIDE;

  /**
   * Read scalars per node for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.  If there will be more than one component in
   * the data array, it is assumed that 0 is the first component added.
   */
  int ReadScalarsPerNode(const char* fileName, const char* description,
    int timeStep, vtkMultiBlockDataSet *output,
    int measured = 0,
    int numberOfComponents = 1,
    int component = 0) VTK_OVERRIDE;

  /**
   * Read vectors per node for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  int ReadVectorsPerNode(const char* fileName, const char* description,
    int timeStep, vtkMultiBlockDataSet *output,
    int measured = 0) VTK_OVERRIDE;

  /**
   * Read tensors per node for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  int ReadTensorsPerNode(const char* fileName, const char* description,
    int timeStep, vtkMultiBlockDataSet *output) VTK_OVERRIDE;

  /**
   * Read scalars per element for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.  If there will be more than one componenet in the
   * data array, it is assumed that 0 is the first component added.
   */
  int ReadScalarsPerElement(const char* fileName, const char* description,
    int timeStep, vtkMultiBlockDataSet *output,
    int numberOfComponents = 1,
    int component = 0) VTK_OVERRIDE;

  /**
   * Read vectors per element for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  int ReadVectorsPerElement(const char* fileName, const char* description,
    int timeStep, vtkMultiBlockDataSet *output) VTK_OVERRIDE;

  /**
   * Read tensors per element for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  int ReadTensorsPerElement(const char* fileName, const char* description,
    int timeStep, vtkMultiBlockDataSet *output) VTK_OVERRIDE;

  /**
   * Read an unstructured part (partId) from the geometry file and create a
   * vtkUnstructuredGrid output.  Return 0 if EOF reached. Return -1 if
   * an error occurred.
   */
  int CreateUnstructuredGridOutput(int partId,
    char line[256],
    const char* name,
    vtkMultiBlockDataSet *output) VTK_OVERRIDE;

  /**
   * Read a structured part from the geometry file and create a
   * vtkStructuredGrid output.  Return 0 if EOF reached.
   */
  int CreateStructuredGridOutput(int partId,
    char line[256],
    const char* name,
    vtkMultiBlockDataSet *output) VTK_OVERRIDE;

  /**
   * Read a structured part from the geometry file and create a
   * vtkRectilinearGrid output.  Return 0 if EOF reached.
   */
  int CreateRectilinearGridOutput(int partId, char line[256], const char* name,
    vtkMultiBlockDataSet *output);

  /**
   * Read a structured part from the geometry file and create a
   * vtkImageData output.  Return 0 if EOF reached.
   */
  int CreateImageDataOutput(int partId, char line[256], const char* name,
    vtkMultiBlockDataSet *output);

  /**
   * Skip next line in file if the 'undef' or 'partial' keyword was
   * specified after a sectional keyword
   */
  int CheckForUndefOrPartial(const char *line);

  int NodeIdsListed;
  int ElementIdsListed;

  class UndefPartialInternal;
  /**
   * Handle the undef / partial support for EnSight gold
   */
  UndefPartialInternal* UndefPartial;

  class FileOffsetMapInternal;
  FileOffsetMapInternal *FileOffsets;

private:
  vtkEnSightGoldReader(const vtkEnSightGoldReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkEnSightGoldReader&) VTK_DELETE_FUNCTION;
};

#endif
