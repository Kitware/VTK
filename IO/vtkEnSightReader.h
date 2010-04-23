/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEnSightReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkEnSightReader - superclass for EnSight file readers

#ifndef __vtkEnSightReader_h
#define __vtkEnSightReader_h

#include "vtkGenericEnSightReader.h"

class vtkDataSet;
class vtkDataSetCollection;
class vtkEnSightReaderCellIdsType;
class vtkIdList;
class vtkMultiBlockDataSet;

class VTK_IO_EXPORT vtkEnSightReader : public vtkGenericEnSightReader
{
public:
  vtkTypeMacro(vtkEnSightReader, vtkGenericEnSightReader);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  //BTX
  enum ElementTypesList
  {
    POINT     = 0,
    BAR2      = 1,
    BAR3      = 2,
    NSIDED    = 3,
    TRIA3     = 4,
    TRIA6     = 5,
    QUAD4     = 6,
    QUAD8     = 7,
    NFACED    = 8,
    TETRA4    = 9,
    TETRA10   = 10,
    PYRAMID5  = 11,
    PYRAMID13 = 12,
    HEXA8     = 13,
    HEXA20    = 14,
    PENTA6    = 15,
    PENTA15   = 16,
    NUMBER_OF_ELEMENT_TYPES  = 17
  };

  enum VariableTypesList
  {
    SCALAR_PER_NODE            = 0,
    VECTOR_PER_NODE            = 1,
    TENSOR_SYMM_PER_NODE       = 2,
    SCALAR_PER_ELEMENT         = 3,
    VECTOR_PER_ELEMENT         = 4,
    TENSOR_SYMM_PER_ELEMENT    = 5,
    SCALAR_PER_MEASURED_NODE   = 6,
    VECTOR_PER_MEASURED_NODE   = 7,
    COMPLEX_SCALAR_PER_NODE    = 8,
    COMPLEX_VECTOR_PER_NODE    = 9,
    COMPLEX_SCALAR_PER_ELEMENT = 10,
    COMPLEX_VECTOR_PER_ELEMENT = 11
  };

  enum SectionTypeList
  {
    COORDINATES = 0,
    BLOCK       = 1,
    ELEMENT     = 2
  };
  //ETX

  // Description:
  // Get the Measured file name. Made public to allow access from 
  // apps requiring detailed info about the Data contents
  vtkGetStringMacro(MeasuredFileName);

  // Description:
  // Get the Match file name. Made public to allow access from 
  // apps requiring detailed info about the Data contents
  vtkGetStringMacro(MatchFileName);

  // Description:
  // The MeasuredGeometryFile should list particle coordinates
  // from 0->N-1.
  // If a file is loaded where point Ids are listed from 1-N
  // the Id to points reference will be wrong and the data
  // will be generated incorrectly.
  // Setting ParticleCoordinatesByIndex to true will force
  // all Id's to increment from 0->N-1 (relative to their order
  // in the file) and regardless of the actual Id of of the point.
  // Warning, if the Points are listed in non sequential order
  // then setting this flag will reorder them.
  vtkSetMacro(ParticleCoordinatesByIndex, int);
  vtkGetMacro(ParticleCoordinatesByIndex, int);
  vtkBooleanMacro(ParticleCoordinatesByIndex, int);

protected:
  vtkEnSightReader();
  ~vtkEnSightReader();
  
  virtual int RequestInformation(vtkInformation*, 
                                 vtkInformationVector**, 
                                 vtkInformationVector*);
  virtual int RequestData(vtkInformation*, 
                          vtkInformationVector**, 
                          vtkInformationVector*);

  // Description:
  // Set the Measured file name.
  vtkSetStringMacro(MeasuredFileName);

  // Description:
  // Set the Match file name.
  vtkSetStringMacro(MatchFileName);

  // Description:
  // Read the case file.  If an error occurred, 0 is returned; otherwise 1.
  int ReadCaseFile();

  // set in UpdateInformation to value returned from ReadCaseFile
  int CaseFileRead;
  
  // Description:
  // Read the geometry file.  If an error occurred, 0 is returned; otherwise 1.
  virtual int ReadGeometryFile(const char* fileName, int timeStep,
                               vtkMultiBlockDataSet *output) = 0;

  // Description:
  // Read the measured geometry file.  If an error occurred, 0 is returned;
  // otherwise 1.
  virtual int ReadMeasuredGeometryFile(const char* fileName, int timeStep,
                                       vtkMultiBlockDataSet *output) = 0;

  // Description:
  // Read the variable files. If an error occurred, 0 is returned; otherwise 1.
  int ReadVariableFiles(vtkMultiBlockDataSet *output);

  // Description:
  // Read scalars per node for this dataset.  If an error occurred, 0 is
  // returned; otherwise 1.
  virtual int ReadScalarsPerNode(const char* fileName, const char* description,
                                 int timeStep, vtkMultiBlockDataSet *output,
                                 int measured = 0, int numberOfComponents = 1,
                                 int component = 0) = 0;
  
  // Description:
  // Read vectors per node for this dataset.  If an error occurred, 0 is
  // returned; otherwise 1.
  virtual int ReadVectorsPerNode(const char* fileName, const char* description,
                                 int timeStep, vtkMultiBlockDataSet *output,
                                 int measured = 0) = 0;

  // Description:
  // Read tensors per node for this dataset.  If an error occurred, 0 is
  // returned; otherwise 1.
  virtual int ReadTensorsPerNode(const char* fileName, const char* description,
                                 int timeStep, vtkMultiBlockDataSet *output) = 0;

  // Description:
  // Read scalars per element for this dataset.  If an error occurred, 0 is
  // returned; otherwise 1.
  virtual int ReadScalarsPerElement(const char* fileName, const char* description,
                                    int timeStep, vtkMultiBlockDataSet *output,
                                    int numberOfComponents = 1,
                                    int component = 0) = 0;

  // Description:
  // Read vectors per element for this dataset.  If an error occurred, 0 is
  // returned; otherwise 1.
  virtual int ReadVectorsPerElement(const char* fileName, const char* description,
                                    int timeStep, vtkMultiBlockDataSet *output) = 0;

  // Description:
  // Read tensors per element for this dataset.  If an error occurred, 0 is
  // returned; otherwise 1.
  virtual int ReadTensorsPerElement(const char* fileName, const char* description,
                                    int timeStep, vtkMultiBlockDataSet *output) = 0;

  // Description:
  // Read an unstructured part (partId) from the geometry file and create a
  // vtkUnstructuredGrid output.  Return 0 if EOF reached.
  virtual int CreateUnstructuredGridOutput(int partId, 
                                           char line[80],
                                           const char* name,
                                           vtkMultiBlockDataSet *output) = 0;
  
  // Description:
  // Read a structured part from the geometry file and create a
  // vtkStructuredGridOutput.  Return 0 if EOF reached.
  virtual int CreateStructuredGridOutput(int partId, 
                                         char line[80],
                                         const char* name,
                                         vtkMultiBlockDataSet *output) = 0;
    
  // Description:
  // Add another file name to the list for a particular variable type.
  void AddVariableFileName(const char* fileName1, const char* fileName2 = NULL);
  
  // Description:
  // Add another description to the list for a particular variable type.
  void AddVariableDescription(const char* description);
  
  // Description:
  // Record the variable type for the variable line just read.
  void AddVariableType();

  // Description:
  // Determine the element type from a line read a file.  Return -1 for
  // invalid element type.
  int GetElementType(const char* line);

  // Description:
  // Determine the section type from a line read a file.  Return -1 for
  // invalid section type.
 int GetSectionType(const char *line);

  // Description:
  // Replace the *'s in the filename with the given filename number.
  void ReplaceWildcards(char* filename, int num);

  // Description:
  // Remove leading blank spaces from a string.
  void RemoveLeadingBlanks(char *line);

  // Get the vtkIdList for the given output index and cell type.
  vtkIdList* GetCellIds(int index, int cellType);

  // Description:
  // Convenience method use to convert the readers from VTK 5 multiblock API 
  // to the current composite data infrastructure.
  void AddToBlock(vtkMultiBlockDataSet* output, 
                  unsigned int blockNo, 
                  vtkDataSet* dataset);

  // Description:
  // Convenience method use to convert the readers from VTK 5 multiblock API 
  // to the current composite data infrastructure.
  vtkDataSet* GetDataSetFromBlock(vtkMultiBlockDataSet* output,
                                  unsigned int blockNo);

  // Description:
  // Set the name of a block.
  void SetBlockName(vtkMultiBlockDataSet* output, unsigned int blockNo, 
    const char* name);
  
  char* MeasuredFileName;
  char* MatchFileName; // may not actually be necessary to read this file

  // pointer to lists of vtkIdLists (cell ids per element type per part)
  vtkEnSightReaderCellIdsType* CellIds;
  
  // part ids of unstructured outputs
  vtkIdList* UnstructuredPartIds;
  
  int VariableMode;
  
  // pointers to lists of filenames
  char** VariableFileNames; // non-complex
  char** ComplexVariableFileNames;
  
  // array of time sets
  vtkIdList *VariableTimeSetIds;
  vtkIdList *ComplexVariableTimeSetIds;
  
  // array of file sets
  vtkIdList *VariableFileSetIds;
  vtkIdList *ComplexVariableFileSetIds;
  
  // collection of filename numbers per time set
  vtkIdListCollection *TimeSetFileNameNumbers;
  vtkIdList *TimeSetsWithFilenameNumbers;
  
  // collection of filename numbers per file set
  vtkIdListCollection *FileSetFileNameNumbers;
  vtkIdList *FileSetsWithFilenameNumbers;
  
  // collection of number of steps per file per file set
  vtkIdListCollection *FileSetNumberOfSteps;
  
  // ids of the time and file sets
  vtkIdList *TimeSetIds;
  vtkIdList *FileSets;
  
  int GeometryTimeSet;
  int GeometryFileSet;
  int MeasuredTimeSet;
  int MeasuredFileSet;
  
  float GeometryTimeValue;
  float MeasuredTimeValue;
  
  int UseTimeSets;
  vtkSetMacro(UseTimeSets, int);
  vtkGetMacro(UseTimeSets, int);
  vtkBooleanMacro(UseTimeSets, int);
  
  int UseFileSets;
  vtkSetMacro(UseFileSets, int);
  vtkGetMacro(UseFileSets, int);
  vtkBooleanMacro(UseFileSets, int);
  
  int NumberOfGeometryParts;
  
  // global list of points for measured geometry
  int NumberOfMeasuredPoints;
  
  int NumberOfNewOutputs;
  int InitialRead;
  
  int CheckOutputConsistency();

  int ParticleCoordinatesByIndex;
  
  double ActualTimeValue;
  
  //BTX
  enum { FORWARD_TIME_STEP_SHIFT_NON = 0, 
         FORWARD_TIME_STEP_SHIFT_YES = 1, 
         FORWARD_TIME_STEP_SHIFT_END = 2 
       };
  //ETX
  
  // Whether (non-zero) or not (zero) the next time step is a successor 
  // (unnecessarily immediate) of the previous one in the same geometry
  // file. Specifically a value of 2 means that the file just ends with
  // the the next time step.
  int       PreviousTimeStepInFile;
  int       ForwardTimeStepShiftMode;
  istream * ForwardTimeStepShiftIS;

private:
  vtkEnSightReader(const vtkEnSightReader&);  // Not implemented.
  void operator=(const vtkEnSightReader&);  // Not implemented.
};

#endif
