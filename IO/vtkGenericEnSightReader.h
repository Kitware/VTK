/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericEnSightReader.h
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
// .NAME vtkGenericEnSightReader - class to read any type of EnSight files
// .SECTION Description
// The class vtkGenericEnSightReader allows the user to read an EnSight data
// set without a priori knowledge of what type of EnSight data set it is.

#ifndef __vtkGenericEnSightReader_h
#define __vtkGenericEnSightReader_h

#include "vtkDataSetSource.h"

class vtkDataArrayCollection;
class vtkIdListCollection;


class VTK_IO_EXPORT vtkGenericEnSightReader : public vtkDataSetSource
{
public:
  static vtkGenericEnSightReader *New();
  vtkTypeRevisionMacro(vtkGenericEnSightReader, vtkDataSetSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the Case file name.
  void SetCaseFileName(const char* fileName);
  vtkGetStringMacro(CaseFileName);

  // Description:
  // Set/Get the file path.
  vtkSetStringMacro(FilePath);
  vtkGetStringMacro(FilePath);
  
  virtual void Update();
  virtual void ExecuteInformation();
  
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
  // SCALAR_PER_NODE = 0; VECTOR_PER_NODE = 1;
  // TENSOR_SYMM_PER_NODE = 2; SCALAR_PER_ELEMENT = 3;
  // VECTOR_PER_ELEMENT = 4; TENSOR_SYMM_PER_ELEMENT = 5;
  // SCALAR_PER_MEASURED_NODE = 6; VECTOR_PER_MEASURED_NODE = 7;
  // COMPLEX_SCALAR_PER_NODE = 8; COMPLEX_VECTOR_PER_NODE 9;
  // COMPLEX_SCALAR_PER_ELEMENT  = 10; COMPLEX_VECTOR_PER_ELEMENT = 11
  char* GetDescription(int n, int type);
  
  // Description:
  // Get the variable type of variable n.
  int GetVariableType(int n);
  int GetComplexVariableType(int n);
  
  // Description:
  // Set/Get the time value at which to get the value.
  vtkSetMacro(TimeValue, float);
  vtkGetMacro(TimeValue, float);

  // Description:
  // Get the minimum or maximum time value for this data set.
  vtkGetMacro(MinimumTimeValue, float);
  vtkGetMacro(MaximumTimeValue, float);
  
  // Description:
  // Get the time values per time set
  vtkGetObjectMacro(TimeSets, vtkDataArrayCollection);

  // Description:
  // Reads the FORMAT part of the case file to determine whether this is an
  // EnSight6 or EnSightGold data set.  Returns 0 if the format is EnSight6,
  // 1 if it is EnSightGold, and -1 otherwise (meaning an error occurred).
  int DetermineEnSightVersion();

  // Description:
  // Set/get the flag for whether to read all the variables
  vtkBooleanMacro(ReadAllVariables, int);
  vtkSetMacro(ReadAllVariables, int);
  vtkGetMacro(ReadAllVariables, int);
  
  // Description:
  // Specify which variables to read.
  // attributeType specifies whether this variable is point (=0)
  // or cell (=1) data.
  void AddVariableName(char* variableName, int attributeType);
  void AddPointVariableName(char* variableName);
  void AddCellVariableName(char* variableName);
  
  // Description:
  // Remove all the requested variable names.
  void RemoveAllVariableNames();
  
  // Description:
  // Remove all requested point/cell variable names.
  void RemoveAllPointVariableNames();
  void RemoveAllCellVariableNames();

  // Description:
  // Will this variable be loaded?
  // attributeType specifies whether this variable is point (=0)
  // or cell (=1) data.
  int IsRequestedVariable(char *variableName, int attributeType);
  
  // Description:
  // Get the number of point/cell arrays in this data set.
  int GetNumberOfPointArrays();
  int GetNumberOfCellArrays();
  
  //BTX
  enum FileTypes
  {
    ENSIGHT_6             = 0,
    ENSIGHT_6_BINARY      = 1,
    ENSIGHT_GOLD          = 2,
    ENSIGHT_GOLD_BINARY   = 3,
    ENSIGHT_MASTER_SERVER = 4
  };
  //ETX

  // Description:
  // Set the byte order of the file (remember, more Unix workstations
  // write big endian whereas PCs write little endian). Default is
  // big endian (since most older PLOT3D files were written by
  // workstations).
  void SetByteOrderToBigEndian();
  void SetByteOrderToLittleEndian();
  vtkSetMacro(ByteOrder, int);
  vtkGetMacro(ByteOrder, int);
  const char *GetByteOrderAsString();

//BTX
  enum 
  {
    FILE_BIG_ENDIAN=0,
    FILE_LITTLE_ENDIAN=1
  };
//ETX

protected:
  vtkGenericEnSightReader();
  ~vtkGenericEnSightReader();

  void Execute();
  
  // Description:
  // Internal function to read in a line up to 256 characters.
  // Returns zero if there was an error.
  int ReadLine(char result[256]);

  // Description:
  // Internal function to read up to 80 characters from a binary file.
  // Returns zero if there was an error.
  int ReadBinaryLine(char result[80]);
  
  // Internal function that skips blank lines and reads the 1st
  // non-blank line it finds (up to 256 characters).
  // Returns 0 is there was an error.
  int ReadNextDataLine(char result[256]);

  // Description:
  // Set/Get the geometry file name.
  vtkSetStringMacro(GeometryFileName);
  vtkGetStringMacro(GeometryFileName);
  
  // Description:
  // Add a variable description to the appropriate array.
  void AddVariableDescription(char* description);
  void AddComplexVariableDescription(char* description);

  // Description:
  // Add a variable type to the appropriate array.
  void AddVariableType(int variableType);
  void AddComplexVariableType(int variableType);

  // Description:
  // Replace the wildcards in the geometry file name with appropriate filename
  // numbers as specified in the time set or file set.
  void ReplaceWildcards(char* fileName, int timeSet, int fileSet);
  void ReplaceWildcardsHelper(char* fileName, int num);
  
  istream* IS;
  FILE *IFile;
  vtkGenericEnSightReader *Reader;
  
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
  
  float TimeValue;
  float MinimumTimeValue;
  float MaximumTimeValue;
  
  vtkDataArrayCollection *TimeSets;
  virtual void SetTimeSets(vtkDataArrayCollection*);

  int ReadAllVariables;
  int NumberOfRequestedPointVariables;
  int NumberOfRequestedCellVariables;
  char** RequestedPointVariables;
  char** RequestedCellVariables;

  int ByteOrder;
  
private:
  vtkGenericEnSightReader(const vtkGenericEnSightReader&);  // Not implemented.
  void operator=(const vtkGenericEnSightReader&);  // Not implemented.
};

#endif
