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

class vtkCollection;

// element types
#define VTK_ENSIGHT_POINT               0
#define VTK_ENSIGHT_BAR2                1
#define VTK_ENSIGHT_BAR3                2
#define VTK_ENSIGHT_NSIDED              3
#define VTK_ENSIGHT_TRIA3               4
#define VTK_ENSIGHT_TRIA6               5
#define VTK_ENSIGHT_QUAD4               6
#define VTK_ENSIGHT_QUAD8               7
#define VTK_ENSIGHT_TETRA4              8
#define VTK_ENSIGHT_TETRA10             9
#define VTK_ENSIGHT_PYRAMID5           10
#define VTK_ENSIGHT_PYRAMID13          11
#define VTK_ENSIGHT_HEXA8              12
#define VTK_ENSIGHT_HEXA20             13
#define VTK_ENSIGHT_PENTA6             14
#define VTK_ENSIGHT_PENTA15            15

// variable types
#define VTK_SCALAR_PER_NODE             0
#define VTK_VECTOR_PER_NODE             1
#define VTK_TENSOR_SYMM_PER_NODE        2
#define VTK_SCALAR_PER_ELEMENT          3
#define VTK_VECTOR_PER_ELEMENT          4
#define VTK_TENSOR_SYMM_PER_ELEMENT     5
#define VTK_SCALAR_PER_MEASURED_NODE    6
#define VTK_VECTOR_PER_MEASURED_NODE    7
#define VTK_COMPLEX_SCALAR_PER_NODE     8
#define VTK_COMPLEX_VECTOR_PER_NODE     9
#define VTK_COMPLEX_SCALAR_PER_ELEMENT 10
#define VTK_COMPLEX_VECTOR_PER_ELEMENT 11

class VTK_IO_EXPORT vtkEnSightReader : public vtkGenericEnSightReader
{
public:
  vtkTypeRevisionMacro(vtkEnSightReader, vtkGenericEnSightReader);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  void Update();
  void UpdateInformation();
  
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
  vtkIdList *VariableTimeSets;
  vtkIdList *ComplexVariableTimeSets;
  
  // array of file sets
  vtkIdList *VariableFileSets;
  vtkIdList *ComplexVariableFileSets;
  
  // collection of filename numbers per time set
  vtkCollection *TimeSetFilenameNumbersCollection;
  vtkIdList *TimeSetsWithFilenameNumbers;
  
  // collection of time values per time set
  vtkCollection *TimeSetTimeValuesCollection;
  
  // collection of filename numbers per file set
  vtkCollection *FileSetFilenameNumbersCollection;
  vtkIdList *FileSetsWithFilenameNumbers;
  
  // collection of number of steps per file per file set
  vtkCollection *FileSetNumberOfStepsCollection;
  
  // ids of the time and file sets
  vtkIdList *TimeSets;
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
  vtkIdList *MeasuredNodeIds;
private:
  vtkEnSightReader(const vtkEnSightReader&);  // Not implemented.
  void operator=(const vtkEnSightReader&);  // Not implemented.
};

#endif
