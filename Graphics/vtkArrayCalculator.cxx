/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayCalculator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkArrayCalculator.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkFunctionParser.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

vtkCxxRevisionMacro(vtkArrayCalculator, "1.32");
vtkStandardNewMacro(vtkArrayCalculator);

vtkArrayCalculator::vtkArrayCalculator()
{
  this->FunctionParser = vtkFunctionParser::New();
  
  this->Function = NULL;
  this->ResultArrayName = NULL;
  this->SetResultArrayName("resultArray");
  this->ScalarArrayNames = NULL;
  this->VectorArrayNames = NULL;
  this->ScalarVariableNames = NULL;
  this->VectorVariableNames = NULL;
  this->NumberOfScalarArrays = 0;
  this->NumberOfVectorArrays = 0;
  this->AttributeMode = VTK_ATTRIBUTE_MODE_DEFAULT;
  this->SelectedScalarComponents = NULL;
  this->SelectedVectorComponents = NULL;

  this->ReplaceInvalidValues = 0;
  this->ReplacementValue = 0.0;
}

vtkArrayCalculator::~vtkArrayCalculator()
{
  int i;
  
  this->FunctionParser->Delete();
  this->FunctionParser = NULL;
  
  if (this->Function)
    {
    delete [] this->Function;
    this->Function = NULL;
    }

  if (this->ResultArrayName)
    {
    delete [] this->ResultArrayName;
    this->ResultArrayName = NULL;
    }
  
  if (this->ScalarArrayNames)
    {
    for (i = 0; i < this->NumberOfScalarArrays; i++)
      {
      delete [] this->ScalarArrayNames[i];
      this->ScalarArrayNames[i] = NULL;
      }
    delete [] this->ScalarArrayNames;
    this->ScalarArrayNames = NULL;
    }

  if (this->VectorArrayNames)
    {
    for (i = 0; i < this->NumberOfVectorArrays; i++)
      {
      delete [] this->VectorArrayNames[i];
      this->VectorArrayNames[i] = NULL;
      }
    delete [] this->VectorArrayNames;
    this->VectorArrayNames = NULL;
    }

  if (this->ScalarVariableNames)
    {
    for (i = 0; i < this->NumberOfScalarArrays; i++)
      {
      delete [] this->ScalarVariableNames[i];
      this->ScalarVariableNames[i] = NULL;
      }
    delete [] this->ScalarVariableNames;
    this->ScalarVariableNames = NULL;
    }
  
  if (this->VectorVariableNames)
    {
    for (i = 0; i < this->NumberOfVectorArrays; i++)
      {
      delete [] this->VectorVariableNames[i];
      this->VectorVariableNames[i] = NULL;
      }
    delete [] this->VectorVariableNames;
    this->VectorVariableNames = NULL;
    }
  
  if (this->SelectedScalarComponents)
    {
    delete [] this->SelectedScalarComponents;
    this->SelectedScalarComponents = NULL;
    }
  
  if (this->SelectedVectorComponents)
    {
    for (i = 0; i < this->NumberOfVectorArrays; i++)
      {
      delete [] this->SelectedVectorComponents[i];
      this->SelectedVectorComponents[i] = NULL;
      }
    delete [] this->SelectedVectorComponents;
    this->SelectedVectorComponents = NULL;
    }
}

void vtkArrayCalculator::SetResultArrayName(const char* name)
{
  if (name == NULL || *name == '\0')
    {
    vtkErrorMacro("The result array must have a name.");
    return;
    }
  if (this->ResultArrayName != NULL && strcmp(this->ResultArrayName, name) == 0)
    {
    return;
    }
  this->Modified();
  if (this->ResultArrayName)
    {
    delete [] this->ResultArrayName;
    this->ResultArrayName = NULL;
    }
  this->ResultArrayName = new char [strlen(name)+1];
  strcpy(this->ResultArrayName, name);
}

int vtkArrayCalculator::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int resultType; // 0 for scalar, 1 for vector
  int attributeDataType; // 0 for point data, 1 for cell data
  vtkIdType i;
  int j;
  
  vtkPointData* inPD = input->GetPointData();
  vtkCellData* inCD = input->GetCellData();
  vtkFieldData* inFD;
  vtkDataArray* currentArray;
  vtkDoubleArray* resultArray;
  vtkIdType numTuples;
  double scalarResult[1];

  this->FunctionParser->SetReplaceInvalidValues(this->ReplaceInvalidValues);
  this->FunctionParser->SetReplacementValue(this->ReplacementValue);

  if (this->AttributeMode == VTK_ATTRIBUTE_MODE_DEFAULT)
    {
    inFD = inPD;
    attributeDataType = 0;
    numTuples = input->GetNumberOfPoints();
    }
  else if (this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_POINT_DATA)
    {
    inFD = inPD;
    attributeDataType = 0;
    numTuples = input->GetNumberOfPoints();
    }
  else
    {
    inFD = inCD;
    attributeDataType = 1;
    numTuples = input->GetNumberOfCells();
    }
  if (numTuples < 1)
    {
    vtkDebugMacro("Empty data.");
    return 1;
    }
  
  for (i = 0; i < this->NumberOfScalarArrays; i++)
    {
    currentArray = inFD->GetArray(this->ScalarArrayNames[i]);
    if (currentArray)
      {
      if (currentArray->GetNumberOfComponents() >
          this->SelectedScalarComponents[i])
        {
        this->FunctionParser->
          SetScalarVariableValue(
            this->ScalarVariableNames[i],
            currentArray->GetComponent(0, this->SelectedScalarComponents[i]));
        }
      else
        {
        vtkErrorMacro("Array " << this->ScalarArrayNames[i]
                      << " does not contain the selected component.");
        return 1;
        }
      }
    else
      {
      vtkErrorMacro("Invalid array name: " << this->ScalarArrayNames[i]);
      return 1;
      }
    }

  for (i = 0; i < this->NumberOfVectorArrays; i++)
    {
    currentArray = inFD->GetArray(this->VectorArrayNames[i]);
    if (currentArray)
      {
      if ((currentArray->GetNumberOfComponents() >
           this->SelectedVectorComponents[i][0]) &&
          (currentArray->GetNumberOfComponents() >
           this->SelectedVectorComponents[i][1]) &&
          (currentArray->GetNumberOfComponents() >
           this->SelectedVectorComponents[i][2]))
        {
        this->FunctionParser->
          SetVectorVariableValue(
            this->VectorVariableNames[i],
            currentArray->GetComponent(0, this->SelectedVectorComponents[i][0]),
            currentArray->GetComponent(0, this->SelectedVectorComponents[i][1]),
            currentArray->GetComponent(0, this->SelectedVectorComponents[i][2]));
        }
      else
        {
        vtkErrorMacro("Array " << this->VectorArrayNames[i]
                      << " does not contain one of the selected components.");
        return 1;
        }
      }
    else
      {
      vtkErrorMacro("Invalid array name: " << this->VectorArrayNames[i]);
      return 1;
      }
    }

  if (this->FunctionParser->IsScalarResult())
    {
    resultType = 0;
    }
  else if (this->FunctionParser->IsVectorResult())
    {
    resultType = 1;
    }
  else
    {
    // Error occurred in vtkFunctionParser.
    return 1;
    }

  resultArray = vtkDoubleArray::New();
  if (resultType == 0)
    {
    resultArray->SetNumberOfComponents(1);
    resultArray->SetNumberOfTuples(numTuples);
    scalarResult[0] = this->FunctionParser->GetScalarResult();
    resultArray->SetTuple(0, scalarResult);
    }
  else
    {
    resultArray->Allocate(numTuples * 3);
    resultArray->SetNumberOfComponents(3);
    resultArray->SetNumberOfTuples(numTuples);
    resultArray->SetTuple(0, this->FunctionParser->GetVectorResult());
    }
  
  for (i = 1; i < numTuples; i++)
    {
    for (j = 0; j < this->NumberOfScalarArrays; j++)
      {
      currentArray = inFD->GetArray(this->ScalarArrayNames[j]);
      this->FunctionParser->
        SetScalarVariableValue(
          j, currentArray->GetComponent(i, this->SelectedScalarComponents[j]));
      }
    for (j = 0; j < this->NumberOfVectorArrays; j++)
      {
      currentArray = inFD->GetArray(this->VectorArrayNames[j]);
      this->FunctionParser->
        SetVectorVariableValue(
          j, currentArray->GetComponent(i, this->SelectedVectorComponents[j][0]),
          currentArray->GetComponent(
            i, this->SelectedVectorComponents[j][1]),
          currentArray->GetComponent(i, this->SelectedVectorComponents[j][2]));
      }
    if (resultType == 0)
      {
      scalarResult[0] = this->FunctionParser->GetScalarResult();
      resultArray->SetTuple(i, scalarResult);
      }
    else
      {
      resultArray->SetTuple(i, this->FunctionParser->GetVectorResult());
      }
    }
  
  output->CopyStructure(input);
  output->GetPointData()->PassData(inPD);
  output->GetCellData()->PassData(inCD);
  
  resultArray->SetName(this->ResultArrayName);
  if (attributeDataType == 0)
    {
    output->GetPointData()->AddArray(resultArray);
    if (resultType == 0)
      {
      output->GetPointData()->SetActiveScalars(this->ResultArrayName);
      }
    else
      {
      output->GetPointData()->SetActiveVectors(this->ResultArrayName);
      }
    }
  else
    {
    output->GetCellData()->AddArray(resultArray);
    if (resultType == 0)
      {
      output->GetCellData()->SetActiveScalars(this->ResultArrayName);
      }
    else
      {
      output->GetCellData()->SetActiveVectors(this->ResultArrayName);
      }
    }
  
  resultArray->Delete();

  return 1;
}

void vtkArrayCalculator::SetFunction(const char* function)
{
  if (this->Function && function &&
      strcmp(this->Function, function) == 0)
    {
    return;
    }

  this->Modified();
  
  if (this->Function)
    {
    delete [] this->Function;
    this->Function = NULL;
    }
  
  if (function)
    {
    this->Function = new char[strlen(function)+1];
    strcpy(this->Function, function);
    this->FunctionParser->SetFunction(this->Function);
    }
}

void vtkArrayCalculator::AddScalarArrayName(const char* arrayName,
                                            int component)
{
  if (!arrayName)
    {
    return;
    }
  
  int i;
  char** arrayNames = new char *[this->NumberOfScalarArrays];
  char** varNames = new char *[this->NumberOfScalarArrays];
  int* tempComponents = new int[this->NumberOfScalarArrays];
  
  for (i = 0; i < this->NumberOfScalarArrays; i++)
    {
    arrayNames[i] = new char[strlen(this->ScalarArrayNames[i]) + 1];
    strcpy(arrayNames[i], this->ScalarArrayNames[i]);
    delete [] this->ScalarArrayNames[i];
    this->ScalarArrayNames[i] = NULL;
    varNames[i] = new char[strlen(this->ScalarArrayNames[i]) + 1];
    strcpy(varNames[i], this->ScalarVariableNames[i]);
    delete [] this->ScalarVariableNames[i];
    this->ScalarVariableNames[i] = NULL;
    tempComponents[i] = this->SelectedScalarComponents[i];
    }
  if (this->ScalarArrayNames)
    {
    delete [] this->ScalarArrayNames;
    this->ScalarArrayNames = NULL;
    }
  if (this->ScalarVariableNames)
    {
    delete [] this->ScalarVariableNames;
    this->ScalarVariableNames = NULL;
    }
  if (this->SelectedScalarComponents)
    {
    delete [] this->SelectedScalarComponents;
    this->SelectedScalarComponents = NULL;
    }
  
  this->ScalarArrayNames = new char *[this->NumberOfScalarArrays + 1];
  this->ScalarVariableNames = new char *[this->NumberOfScalarArrays + 1];
  this->SelectedScalarComponents = new int[this->NumberOfScalarArrays + 1];
  
  for (i = 0; i < this->NumberOfScalarArrays; i++)
    {
    this->ScalarArrayNames[i] = new char[strlen(arrayNames[i]) + 1];
    strcpy(this->ScalarArrayNames[i], arrayNames[i]);
    delete [] arrayNames[i];
    arrayNames[i] = NULL;
    this->ScalarVariableNames[i] = new char[strlen(varNames[i]) + 1];
    strcpy(this->ScalarVariableNames[i], varNames[i]);
    delete [] varNames[i];
    varNames[i] = NULL;
    this->SelectedScalarComponents[i] = tempComponents[i];
    }
  delete [] arrayNames;
  delete [] varNames;
  delete [] tempComponents;

  this->ScalarArrayNames[i] = new char[strlen(arrayName) + 1];
  strcpy(this->ScalarArrayNames[i], arrayName);
  this->ScalarVariableNames[i] = new char[strlen(arrayName) + 1];
  strcpy(this->ScalarVariableNames[i], arrayName);
  this->SelectedScalarComponents[i] = component;
  
  this->NumberOfScalarArrays++;
}

void vtkArrayCalculator::AddVectorArrayName(const char* arrayName,
                                            int component0, int component1,
                                            int component2)
{
  if (!arrayName)
    {
    return;
    }
  
  int i;
  char** arrayNames = new char *[this->NumberOfVectorArrays];
  char** varNames = new char *[this->NumberOfVectorArrays];
  int** tempComponents = new int *[this->NumberOfVectorArrays];
  
  for (i = 0; i < this->NumberOfVectorArrays; i++)
    {
    arrayNames[i] = new char[strlen(this->VectorArrayNames[i]) + 1];
    strcpy(arrayNames[i], this->VectorArrayNames[i]);
    delete [] this->VectorArrayNames[i];
    this->VectorArrayNames[i] = NULL;
    varNames[i] = new char[strlen(this->VectorVariableNames[i]) + 1];
    strcpy(varNames[i], this->VectorVariableNames[i]);
    delete [] this->VectorVariableNames[i];
    this->VectorVariableNames[i] = NULL;
    tempComponents[i] = new int[3];
    tempComponents[i][0] = this->SelectedVectorComponents[i][0];
    tempComponents[i][1] = this->SelectedVectorComponents[i][1];
    tempComponents[i][2] = this->SelectedVectorComponents[i][2];
    delete [] this->SelectedVectorComponents[i];
    this->SelectedVectorComponents[i] = NULL;
    }
  
  if (this->VectorArrayNames)
    {
    delete [] this->VectorArrayNames;
    this->VectorArrayNames = NULL;
    }
  if (this->VectorVariableNames)
    {
    delete [] this->VectorVariableNames;
    this->VectorVariableNames = NULL;
    }
  if (this->SelectedVectorComponents)
    {
    delete [] this->SelectedVectorComponents;
    this->SelectedVectorComponents = NULL;
    }
  
  this->VectorArrayNames = new char *[this->NumberOfVectorArrays + 1];
  this->VectorVariableNames = new char *[this->NumberOfVectorArrays + 1];
  this->SelectedVectorComponents = new int *[this->NumberOfVectorArrays + 1];
  
  for (i = 0; i < this->NumberOfVectorArrays; i++)
    {
    this->VectorArrayNames[i] = new char[strlen(arrayNames[i]) + 1];
    strcpy(this->VectorArrayNames[i], arrayNames[i]);
    delete [] arrayNames[i];
    arrayNames[i] = NULL;
    this->VectorVariableNames[i] = new char[strlen(varNames[i]) + 1];
    strcpy(this->VectorVariableNames[i], varNames[i]);
    delete [] varNames[i];
    varNames[i] = NULL;
    this->SelectedVectorComponents[i] = new int[3];
    this->SelectedVectorComponents[i][0] = component0;
    this->SelectedVectorComponents[i][1] = component1;
    this->SelectedVectorComponents[i][2] = component2;
    delete [] tempComponents[i];
    tempComponents[i] = NULL;
    }
  delete [] arrayNames;
  delete [] varNames;
  delete [] tempComponents;

  this->VectorArrayNames[i] = new char[strlen(arrayName) + 1];
  strcpy(this->VectorArrayNames[i], arrayName);
  this->VectorVariableNames[i] = new char[strlen(arrayName) + 1];
  strcpy(this->VectorVariableNames[i], arrayName);
  this->SelectedVectorComponents[i] = new int[3];
  this->SelectedVectorComponents[i][0] = component0;
  this->SelectedVectorComponents[i][1] = component1;
  this->SelectedVectorComponents[i][2] = component2;
  
  this->NumberOfVectorArrays++;
}

void vtkArrayCalculator::AddScalarVariable(const char* variableName,
                                           const char* arrayName,
                                           int component)
{
  if (!arrayName)
    {
    return;
    }
  
  int i;
  char** arrayNames = new char *[this->NumberOfScalarArrays];
  char** varNames = new char *[this->NumberOfScalarArrays];
  int* tempComponents = new int[this->NumberOfScalarArrays];
  
  for (i = 0; i < this->NumberOfScalarArrays; i++)
    {
    arrayNames[i] = new char[strlen(this->ScalarArrayNames[i]) + 1];
    strcpy(arrayNames[i], this->ScalarArrayNames[i]);
    delete [] this->ScalarArrayNames[i];
    this->ScalarArrayNames[i] = NULL;
    varNames[i] = new char[strlen(this->ScalarVariableNames[i]) + 1];
    strcpy(varNames[i], this->ScalarVariableNames[i]);
    delete [] this->ScalarVariableNames[i];
    this->ScalarVariableNames[i] = NULL;
    tempComponents[i] = this->SelectedScalarComponents[i];
    }
  if (this->ScalarArrayNames)
    {
    delete [] this->ScalarArrayNames;
    this->ScalarArrayNames = NULL;
    }
  if (this->ScalarVariableNames)
    {
    delete [] this->ScalarVariableNames;
    this->ScalarVariableNames = NULL;
    }
  if (this->SelectedScalarComponents)
    {
    delete [] this->SelectedScalarComponents;
    this->SelectedScalarComponents = NULL;
    }
  
  this->ScalarArrayNames = new char *[this->NumberOfScalarArrays + 1];
  this->ScalarVariableNames = new char *[this->NumberOfScalarArrays + 1];
  this->SelectedScalarComponents = new int[this->NumberOfScalarArrays + 1];
  
  for (i = 0; i < this->NumberOfScalarArrays; i++)
    {
    this->ScalarArrayNames[i] = new char[strlen(arrayNames[i]) + 1];
    strcpy(this->ScalarArrayNames[i], arrayNames[i]);
    delete [] arrayNames[i];
    arrayNames[i] = NULL;
    this->ScalarVariableNames[i] = new char[strlen(varNames[i]) + 1];
    strcpy(this->ScalarVariableNames[i], varNames[i]);
    delete [] varNames[i];
    varNames[i] = NULL;
    this->SelectedScalarComponents[i] = tempComponents[i];
    }
  delete [] arrayNames;
  delete [] varNames;
  delete [] tempComponents;

  this->ScalarArrayNames[i] = new char[strlen(arrayName) + 1];
  strcpy(this->ScalarArrayNames[i], arrayName);
  this->ScalarVariableNames[i] = new char[strlen(variableName) + 1];
  strcpy(this->ScalarVariableNames[i], variableName);
  this->SelectedScalarComponents[i] = component;
  
  this->NumberOfScalarArrays++;
}

void vtkArrayCalculator::AddVectorVariable(const char* variableName,
                                           const char* arrayName,
                                           int component0, int component1,
                                           int component2)
{
  if (!arrayName)
    {
    return;
    }
  
  int i;
  char** arrayNames = new char *[this->NumberOfVectorArrays];
  char** varNames = new char *[this->NumberOfVectorArrays];
  int** tempComponents = new int *[this->NumberOfVectorArrays];
  
  for (i = 0; i < this->NumberOfVectorArrays; i++)
    {
    arrayNames[i] = new char[strlen(this->VectorArrayNames[i]) + 1];
    strcpy(arrayNames[i], this->VectorArrayNames[i]);
    delete [] this->VectorArrayNames[i];
    this->VectorArrayNames[i] = NULL;
    varNames[i] = new char[strlen(this->VectorVariableNames[i]) + 1];
    strcpy(varNames[i], this->VectorVariableNames[i]);
    delete [] this->VectorVariableNames[i];
    this->VectorVariableNames[i] = NULL;
    tempComponents[i] = new int[3];
    tempComponents[i][0] = this->SelectedVectorComponents[i][0];
    tempComponents[i][1] = this->SelectedVectorComponents[i][1];
    tempComponents[i][2] = this->SelectedVectorComponents[i][2];
    delete [] this->SelectedVectorComponents[i];
    this->SelectedVectorComponents[i] = NULL;
    }
  
  if (this->VectorArrayNames)
    {
    delete [] this->VectorArrayNames;
    this->VectorArrayNames = NULL;
    }
  if (this->VectorVariableNames)
    {
    delete [] this->VectorVariableNames;
    this->VectorVariableNames = NULL;
    }
  if (this->SelectedVectorComponents)
    {
    delete [] this->SelectedVectorComponents;
    this->SelectedVectorComponents = NULL;
    }
  
  this->VectorArrayNames = new char *[this->NumberOfVectorArrays + 1];
  this->VectorVariableNames = new char *[this->NumberOfVectorArrays + 1];
  this->SelectedVectorComponents = new int *[this->NumberOfVectorArrays + 1];
  
  for (i = 0; i < this->NumberOfVectorArrays; i++)
    {
    this->VectorArrayNames[i] = new char[strlen(arrayNames[i]) + 1];
    strcpy(this->VectorArrayNames[i], arrayNames[i]);
    delete [] arrayNames[i];
    arrayNames[i] = NULL;
    this->VectorVariableNames[i] = new char[strlen(varNames[i]) + 1];
    strcpy(this->VectorVariableNames[i], varNames[i]);
    delete [] varNames[i];
    varNames[i] = NULL;
    this->SelectedVectorComponents[i] = new int[3];
    this->SelectedVectorComponents[i][0] = tempComponents[i][0];
    this->SelectedVectorComponents[i][1] = tempComponents[i][1];
    this->SelectedVectorComponents[i][2] = tempComponents[i][2];
    delete [] tempComponents[i];
    tempComponents[i] = NULL;
    }
  delete [] arrayNames;
  delete [] varNames;
  delete [] tempComponents;

  this->VectorArrayNames[i] = new char[strlen(arrayName) + 1];
  strcpy(this->VectorArrayNames[i], arrayName);
  this->VectorVariableNames[i] = new char[strlen(variableName) + 1];
  strcpy(this->VectorVariableNames[i], variableName);
  this->SelectedVectorComponents[i] = new int[3];
  this->SelectedVectorComponents[i][0] = component0;
  this->SelectedVectorComponents[i][1] = component1;
  this->SelectedVectorComponents[i][2] = component2;
  
  this->NumberOfVectorArrays++;
}

const char* vtkArrayCalculator::GetAttributeModeAsString()
{
  if ( this->AttributeMode == VTK_ATTRIBUTE_MODE_DEFAULT )
    {
    return "Default";
    }
  else if ( this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_POINT_DATA )
    {
    return "UsePointData";
    }
  else 
    {
    return "UseCellData";
    }
}

void vtkArrayCalculator::RemoveAllVariables()
{
  int i;
  
  for (i = 0; i < this->NumberOfScalarArrays; i++)
    {
    delete [] this->ScalarArrayNames[i];
    this->ScalarArrayNames[i] = NULL;
    delete [] this->ScalarVariableNames[i];
    this->ScalarVariableNames[i] = NULL;
    }
  if (this->NumberOfScalarArrays > 0)
    {
    delete [] this->ScalarArrayNames;
    this->ScalarArrayNames = NULL;
    delete [] this->ScalarVariableNames;
    this->ScalarVariableNames = NULL;
    delete [] this->SelectedScalarComponents;
    this->SelectedScalarComponents = NULL;
    }
  this->NumberOfScalarArrays = 0;
  
  for (i = 0; i < this->NumberOfVectorArrays; i++)
    {
    delete [] this->VectorArrayNames[i];
    this->VectorArrayNames[i] = NULL;
    delete [] this->VectorVariableNames[i];
    this->VectorVariableNames[i] = NULL;
    delete [] this->SelectedVectorComponents[i];
    this->SelectedVectorComponents[i] = NULL;
    }
  if (this->NumberOfVectorArrays > 0)
    {
    delete [] this->VectorArrayNames;
    this->VectorArrayNames = NULL;
    delete [] this->VectorVariableNames;
    this->VectorVariableNames = NULL;
    delete [] this->SelectedVectorComponents;
    this->SelectedVectorComponents = NULL;
    }
  this->NumberOfVectorArrays = 0;

  this->FunctionParser->RemoveAllVariables();
}

char* vtkArrayCalculator::GetScalarArrayName(int i)
{
  if (i < this->NumberOfScalarArrays)
    {
    return this->ScalarArrayNames[i];
    }
  return NULL;
}

char* vtkArrayCalculator::GetVectorArrayName(int i)
{
  if (i < this->NumberOfVectorArrays)
    {
    return this->VectorArrayNames[i];
    }
  return NULL;
}

char* vtkArrayCalculator::GetScalarVariableName(int i)
{
  if (i < this->NumberOfScalarArrays)
    {
    return this->ScalarVariableNames[i];
    }
  return NULL;
}

char* vtkArrayCalculator::GetVectorVariableName(int i)
{
  if (i < this->NumberOfVectorArrays)
    {
    return this->VectorVariableNames[i];
    }
  return NULL;
}

int vtkArrayCalculator::GetSelectedScalarComponent(int i)
{
  if (i < this->NumberOfScalarArrays)
    {
    return this->SelectedScalarComponents[i];
    }
  return -1;
}

int* vtkArrayCalculator::GetSelectedVectorComponents(int i)
{
  if (i < this->NumberOfVectorArrays)
    {
    return this->SelectedVectorComponents[i];
    }
  return NULL;
}

void vtkArrayCalculator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Function: " 
     << (this->Function ? this->Function : "(none)") << endl;
  os << indent << "Result Array Name: "
     << (this->ResultArrayName ? this->ResultArrayName : "(none)") << endl;
  os << indent << "Attribute Mode: " << this->GetAttributeModeAsString() << endl;
  os << indent << "Number Of Scalar Arrays: " << this->NumberOfScalarArrays
     << endl;
  os << indent << "Number Of Vector Arrays: " << this->NumberOfVectorArrays
     << endl;
  os << indent << "Replace Invalid Values: " 
     << (this->ReplaceInvalidValues ? "On" : "Off") << endl;
  os << indent << "Replacement Value: " << this->ReplacementValue << endl;
}
