/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEnSightReader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
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

class VTK_IO_EXPORT vtkEnSightReader : public vtkGenericEnSightReader
{
public:
  vtkTypeRevisionMacro(vtkEnSightReader, vtkGenericEnSightReader);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  void Update();
  void ExecuteInformation();
  
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
    TETRA4    = 8,
    TETRA10   = 9,
    PYRAMID5  = 10,
    PYRAMID13 = 11,
    HEXA8     = 12,
    HEXA20    = 13,
    PENTA6    = 14,
    PENTA15   = 15
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
  //ETX

  // Description:
  // This method sets/replaces one of the outputs of the
  // reader without changing it's modification time.
  // Make sure that you pass the right type of data object.
  void ReplaceNthOutput(int n, vtkDataObject* output);
  
protected:
  vtkEnSightReader();
  ~vtkEnSightReader();
  
  void Execute();

  // Description:
  // Read the case file.  If an error occurred, 0 is returned; otherwise 1.
  int ReadCaseFile();

  // set in UpdateInformation to value returned from ReadCaseFile
  int CaseFileRead;
  
  // Description:
  // Read the geometry file.  If an error occurred, 0 is returned; otherwise 1.
  virtual int ReadGeometryFile(char* fileName, int timeStep) = 0;

  // Description:
  // Read the measured geometry file.  If an error occurred, 0 is returned;
  // otherwise 1.
  virtual int ReadMeasuredGeometryFile(char* fileName, int timeStep) = 0;

  // Description:
  // Read the variable files. If an error occurred, 0 is returned; otherwise 1.
  int ReadVariableFiles();

  // Description:
  // Read scalars per node for this dataset.  If an error occurred, 0 is
  // returned; otherwise 1.
  virtual int ReadScalarsPerNode(char* fileName, char* description,
                                 int timeStep, int measured = 0,
                                 int numberOfComponents = 1,
                                 int component = 0) = 0;
  
  // Description:
  // Read vectors per node for this dataset.  If an error occurred, 0 is
  // returned; otherwise 1.
  virtual int ReadVectorsPerNode(char* fileName, char* description,
                                 int timeStep, int measured = 0) = 0;

  // Description:
  // Read tensors per node for this dataset.  If an error occurred, 0 is
  // returned; otherwise 1.
  virtual int ReadTensorsPerNode(char* fileName, char* description,
                                 int timeStep) = 0;

  // Description:
  // Read scalars per element for this dataset.  If an error occurred, 0 is
  // returned; otherwise 1.
  virtual int ReadScalarsPerElement(char* fileName, char* description,
                                    int timeStep, int numberOfComponents = 1,
                                    int component = 0) = 0;

  // Description:
  // Read vectors per element for this dataset.  If an error occurred, 0 is
  // returned; otherwise 1.
  virtual int ReadVectorsPerElement(char* fileName, char* description,
                                    int timeStep) = 0;

  // Description:
  // Read tensors per element for this dataset.  If an error occurred, 0 is
  // returned; otherwise 1.
  virtual int ReadTensorsPerElement(char* fileName, char* description,
                                    int timeStep) = 0;

  // Description:
  // Read an unstructured part (partId) from the geometry file and create a
  // vtkUnstructuredGrid output.  Return 0 if EOF reached.
  virtual int CreateUnstructuredGridOutput(int partId, char line[256]) = 0;
  
  // Description:
  // Read a structured part from the geometry file and create a
  // vtkStructuredGridOutput.  Return 0 if EOF reached.
  virtual int CreateStructuredGridOutput(int partId, char line[256]) = 0;
  
  // Description:
  // Set/Get the Model file name.
  vtkSetStringMacro(GeometryFileName);
  vtkGetStringMacro(GeometryFileName);

  // Description:
  // Set/Get the Measured file name.
  vtkSetStringMacro(MeasuredFileName);
  vtkGetStringMacro(MeasuredFileName);

  // Description:
  // Set/Get the Match file name.
  vtkSetStringMacro(MatchFileName);
  vtkGetStringMacro(MatchFileName);
  
  // Description:
  // Add another file name to the list for a particular variable type.
  void AddVariableFileName(char* fileName1, char* fileName2 = NULL);
  
  // Description:
  // Add another description to the list for a particular variable type.
  void AddVariableDescription(char* description);
  
  // Description:
  // Record the variable type for the variable line just read.
  void AddVariableType();

  // Description:
  // Determine the element type from a line read a file.  Return -1 for
  // invalid element type.
  int GetElementType(char* line);

  // Description:
  // Replace the *'s in the filename with the given filename number.
  void ReplaceWildcards(char* filename, int num);
  
  char* MeasuredFileName;
  char* MatchFileName; // may not actually be necessary to read this file

  // pointer to lists of vtkIdLists (cell ids per element type per part)
  vtkIdList*** CellIds;
  
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
  
  void SetNumberOfOutputsInternal(int num);

  // global list of points for measured geometry
  int NumberOfMeasuredPoints;
  vtkIdList *MeasuredNodeIds;
private:
  vtkEnSightReader(const vtkEnSightReader&);  // Not implemented.
  void operator=(const vtkEnSightReader&);  // Not implemented.
};

#endif
