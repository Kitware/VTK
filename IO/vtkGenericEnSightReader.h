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

#include "vtkEnSightReader.h"

#define VTK_ENSIGHT_6    0
#define VTK_ENSIGHT_6_BINARY 1
#define VTK_ENSIGHT_GOLD 2
#define VTK_ENSIGHT_GOLD_BINARY 3

class VTK_IO_EXPORT vtkGenericEnSightReader : public vtkDataSetSource
{
public:
  static vtkGenericEnSightReader *New();
  vtkTypeRevisionMacro(vtkGenericEnSightReader, vtkDataSetSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the Case file name.
  void SetCaseFileName(char* fileName);
  vtkGetStringMacro(CaseFileName);

  // Description:
  // Set/Get the file path.
  vtkSetStringMacro(FilePath);
  vtkGetStringMacro(FilePath);
  
  void Update();
  void UpdateInformation();
  
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
  vtkGetObjectMacro(TimeSetTimeValuesCollection, vtkCollection);
  
protected:
  vtkGenericEnSightReader();
  ~vtkGenericEnSightReader();

  void Execute();
  
  // Description:
  // Reads the FORMAT part of the case file to determine whether this is an
  // EnSight6 or EnSightGold data set.  Returns 0 if the format is EnSight6,
  // 1 if it is EnSightGold, and -1 otherwise (meaning an error occurred).
  int DetermineEnSightVersion();

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
  vtkEnSightReader *Reader;
  
  char* CaseFileName;
  char* GeometryFileName;
  char* FilePath;

  int* VariableTypes;
  int* ComplexVariableTypes;
  
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
  
  vtkCollection *TimeSetTimeValuesCollection;
  virtual void SetTimeSetTimeValuesCollection(vtkCollection*);
  
private:
  vtkGenericEnSightReader(const vtkGenericEnSightReader&);  // Not implemented.
  void operator=(const vtkGenericEnSightReader&);  // Not implemented.
};

#endif
