/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEnSightReader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkEnSightReader - superclass for EnSight file readers

#ifndef __vtkEnSightReader_h
#define __vtkEnSightReader_h

#include "vtkDataSetSource.h"
#include "vtkCollection.h"

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

class VTK_EXPORT vtkEnSightReader : public vtkDataSetSource
{
public:
  vtkTypeMacro(vtkEnSightReader, vtkDataSetSource);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get the Case file name.
  void SetCaseFileName(char* fileName);
  vtkGetStringMacro(CaseFileName);
  
  // Description:
  // Set/Get the path the the data files.  If specified, this reader will look
  // in this directory for all data files.
  vtkSetStringMacro(FilePath);
  vtkGetStringMacro(FilePath);
  
  // Description:
  // Get the number of variables listed in the case file.
  int GetNumberOfVariables() { return this->NumberOfVariables; }
  int GetNumberOfComplexVariables() { return this->NumberOfComplexVariables; }
  
  // Description:
  // Get the number of variables of a particular type.
  int GetNumberOfVariables(int type); // returns -1 if unknown type specified
  vtkGetMacro(NumberOfScalarsPerNode, int);
  vtkGetMacro(NumberOfVectorsPerNode, int);
  vtkGetMacro(NumberOfTensorsSymmPerNode, int);
  vtkGetMacro(NumberOfScalarsPerElement, int);
  vtkGetMacro(NumberOfVectorsPerElement, int);
  vtkGetMacro(NumberOfTensorsSymmPerElement, int);
  vtkGetMacro(NumberOfScalarsPerMeasuredNode, int);
  vtkGetMacro(NumberOfVectorsPerMeasuredNode, int);
  vtkGetMacro(NumberOfComplexScalarsPerNode, int);
  vtkGetMacro(NumberOfComplexVectorsPerNode, int);
  vtkGetMacro(NumberOfComplexScalarsPerElement, int);
  vtkGetMacro(NumberOfComplexVectorsPerElement, int);
  
  // Description:
  // Get the nth description for a non-complex variable.
  char* GetDescription(int n);
  
  // Description:
  // Get the nth description for a complex variable.
  char* GetComplexDescription(int n);
  
  // Description:
  // Get the nth description of a particular variable type.  Returns NULL if no
  // variable of this type exists in this data set.
  // VTK_SCALAR_PER_NODE = 0; VTK_VECTOR_PER_NODE = 1;
  // VTK_TENSOR_SYMM_PER_NODE = 2; VTK_SCALAR_PER_ELEMENT = 3;
  // VTK_VECTOR_PER_ELEMENT = 4; VTK_TENSOR_SYMM_PER_ELEMENT = 5;
  // VTK_SCALAR_PER_MEASURED_NODE = 6; VTK_VECTOR_PER_MEASURED_NODE = 7;
  // VTK_COMPLEX_SCALAR_PER_NODE = 8; VTK_COMPLEX_VECTOR_PER_NODE 9;
  // VTK_COMPLEX_SCALAR_PER_ELEMENT  = 10; VTK_COMPLEX_VECTOR_PER_ELEMENT = 11
  char* GetDescription(int n, int type);

  // Description:
  // Get the variable type of variable n.
  int GetVariableType(int n);
  int GetComplexVariableType(int n);
  
  void Update();

  // Description:
  // Set/Get the time value at which to get the value.
  vtkSetMacro(TimeValue, float);
  vtkGetMacro(TimeValue, float);
  
  // Description:
  // Get the minimum or maximum time value in this data set.
  vtkGetMacro(MinimumTimeValue, float);
  vtkGetMacro(MaximumTimeValue, float);
  
protected:
  vtkEnSightReader();
  ~vtkEnSightReader();
  vtkEnSightReader(const vtkEnSightReader&) {};
  void operator=(const vtkEnSightReader&) {};
  
  void Execute();

  // Description:
  // Read the case file.  If an error occurred, 0 is returned; otherwise 1.
  int ReadCaseFile();
  
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
  // Internal function to read in a line up to 256 characters.
  // Returns zero if there was an error.
  int ReadLine(char result[256]);

  // Internal function that skips blank lines and reads the 1st
  // non-blank line it finds (up to 256 characters).
  // Returns 0 is there was an error.
  int ReadNextDataLine(char result[256]);
  
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
  
  char* FilePath;
  
  char* CaseFileName;
  char* GeometryFileName;
  char* MeasuredFileName;
  char* MatchFileName; // may not actually be necessary to read this file

  // pointer to lists of vtkIdLists (cell ids per element type per part)
  vtkIdList*** CellIds;
  
  // part ids of unstructured outputs
  vtkIdList* UnstructuredPartIds;
  
  int VariableMode;
  int NumberOfVariables; // non-complex
  int NumberOfComplexVariables;
  
  // array of types (one entry per instance of variable type in case file)
  int* VariableTypes; // non-complex
  int* ComplexVariableTypes;
  
  // pointers to lists of filenames
  char** VariableFileNames; // non-complex
  char** ComplexVariableFileNames;
  
  // pointers to lists of descriptions
  char** VariableDescriptions; // non-complex
  char** ComplexVariableDescriptions;

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
  
  float TimeValue;
  float GeometryTimeValue;
  float MeasuredTimeValue;
  
  float MinimumTimeValue;
  float MaximumTimeValue;
  
  // number of file names / descriptions per type
  int NumberOfScalarsPerNode;
  int NumberOfVectorsPerNode;
  int NumberOfTensorsSymmPerNode;
  int NumberOfScalarsPerElement;
  int NumberOfVectorsPerElement;
  int NumberOfTensorsSymmPerElement;
  int NumberOfScalarsPerMeasuredNode;
  int NumberOfVectorsPerMeasuredNode;
  int NumberOfComplexScalarsPerNode;
  int NumberOfComplexVectorsPerNode;  
  int NumberOfComplexScalarsPerElement;
  int NumberOfComplexVectorsPerElement;
  
  istream* IS;

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
};

#endif
