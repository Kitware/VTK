/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEnSightGoldReader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkEnSightGoldReader - class to read EnSight Gold files
// .SECTION Description
// vtkEnSightGoldReader is a class to read EnSight Gold files into vtk.
// Because the different parts of the EnSight data can be of various data
// types, this reader produces multiple outputs, one per part in the input
// file.
// All variable information is being stored in field data.  The descriptions
// listed in the case file are used as the array names in the field data.
// For complex variables, the description is appended with _r (for the array
// of real values) and _i (for the array if imaginary values)
// .SECTION Caveats
// You must manually call Update on this reader and then connect the rest
// of the pipeline because (due to the nature of the file format) it is
// not possible to know ahead of time how many outputs you will have or
// what types they will be.
// This reader can only handle static EnSight datasets (both static geometry
// and variables).

#ifndef __vtkEnSightGoldReader_h
#define __vtkEnSightGoldReader_h

#include "vtkDataSetSource.h"

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

class VTK_EXPORT vtkEnSightGoldReader : public vtkDataSetSource
{
public:
  static vtkEnSightGoldReader *New();
  vtkTypeMacro(vtkEnSightGoldReader, vtkDataSetSource);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get the Case file name.
  vtkSetStringMacro(CaseFileName);
  vtkGetStringMacro(CaseFileName);
  
  // Description:
  // Set/Get the path the the data files.  If specified, this reader will look
  // in this directory for all data files.
  vtkSetStringMacro(FilePath);
  vtkGetStringMacro(FilePath);
  
  // Description:
  // Get the number of variables listed in the case file.
  int GetNumberOfVariables() { return this->NumberOfVariables +
                                 this->NumberOfComplexVariables; }
  
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
  // Get the nth description of a particular variable type.
  char* GetDescription(int n, int type);
  
  void Update();
  
protected:
  vtkEnSightGoldReader();
  ~vtkEnSightGoldReader();
  vtkEnSightGoldReader(const vtkEnSightGoldReader&) {};
  void operator=(const vtkEnSightGoldReader&) {};
  
  void Execute();

  // Description:
  // Read the case file.  If an error occurred, 0 is returned; otherwise 1.
  int ReadCaseFile();
  
  // Description:
  // Read the geometry file.  If an error occurred, 0 is returned; otherwise 1.
  int ReadGeometryFile();

  // Description:
  // Read the variable files. If an error occurred, 0 is returned; otherwise 1.
  int ReadVariableFiles();

  // Description:
  // Read scalars per node for this dataset.  If an error occurred, 0 is
  // returned; otherwise 1.
  int ReadScalarsPerNode(char* fileName, char* description);
  
  // Description:
  // Read vectors per node for this dataset.  If an error occurred, 0 is
  // returned; otherwise 1.
  int ReadVectorsPerNode(char* fileName, char* description);

  // Description:
  // Read tensors per node for this dataset.  If an error occurred, 0 is
  // returned; otherwise 1.
  int ReadTensorsPerNode(char* fileName, char* description);

  // Description:
  // Read scalars per element for this dataset.  If an error occurred, 0 is
  // returned; otherwise 1.
  int ReadScalarsPerElement(char* fileName, char* description);

  // Description:
  // Read vectors per element for this dataset.  If an error occurred, 0 is
  // returned; otherwise 1.
  int ReadVectorsPerElement(char* fileName, char* description);

  // Description:
  // Read tensors per element for this dataset.  If an error occurred, 0 is
  // returned; otherwise 1.
  int ReadTensorsPerElement(char* fileName, char* description);

  // Description:
  // Read an unstructured part (partId) from the geometry file and create a
  // vtkUnstructuredGrid output.  Return 0 if EOF reached.
  int CreateUnstructuredGridOutput(int partId, char line[256]);
  
  // Description:
  // Read a structured part from the geometry file and create a
  // vtkStructuredGridOutput.  Return 0 if EOF reached.
  int CreateStructuredGridOutput(int partId, char line[256]);
  
  // Description:
  // Read a structured part from the geometry file and create a
  // vtkRectilinearGridOutput.  Return 0 if EOF reached.
  int CreateRectilinearGridOutput(int partId, char line[256]);
  
  // Description:
  // Read a structured part from the geometry file and create a
  // vtkStructuredPointsOutput.  Return 0 if EOF reached.
  int CreateStructuredPointsOutput(int partId, char line[256]);
  
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
};

#endif
