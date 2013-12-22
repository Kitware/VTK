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
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

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
  this->CoordinateScalarVariableNames = NULL;
  this->CoordinateVectorVariableNames = NULL;
  this->NumberOfCoordinateScalarArrays = 0;
  this->NumberOfCoordinateVectorArrays = 0;
  this->SelectedCoordinateScalarComponents = NULL;
  this->SelectedCoordinateVectorComponents = NULL;
  this->CoordinateResults = 0;
  this->ResultNormals = false;
  this->ResultTCoords = false;
  this->ReplaceInvalidValues = 0;
  this->ReplacementValue = 0.0;

  this->ResultArrayType=VTK_DOUBLE;
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

  if (this->CoordinateScalarVariableNames)
    {
    for (i = 0; i < this->NumberOfCoordinateScalarArrays; i++)
      {
      delete [] this->CoordinateScalarVariableNames[i];
      this->CoordinateScalarVariableNames[i] = NULL;
      }
    delete [] this->CoordinateScalarVariableNames;
    this->CoordinateScalarVariableNames = NULL;
    }

  if (this->CoordinateVectorVariableNames)
    {
    for (i = 0; i < this->NumberOfCoordinateVectorArrays; i++)
      {
      delete [] this->CoordinateVectorVariableNames[i];
      this->CoordinateVectorVariableNames[i] = NULL;
      }
    delete [] this->CoordinateVectorVariableNames;
    this->CoordinateVectorVariableNames = NULL;
    }

  if (this->SelectedCoordinateScalarComponents)
    {
    delete [] this->SelectedCoordinateScalarComponents;
    this->SelectedCoordinateScalarComponents = NULL;
    }

  if (this->SelectedCoordinateVectorComponents)
    {
    for (i = 0; i < this->NumberOfCoordinateVectorArrays; i++)
      {
      delete [] this->SelectedCoordinateVectorComponents[i];
      this->SelectedCoordinateVectorComponents[i] = NULL;
      }
    delete [] this->SelectedCoordinateVectorComponents;
    this->SelectedCoordinateVectorComponents = NULL;
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

void CopyDataSetOrGraph(vtkDataSet* dsInput, vtkDataSet* dsOutput,
                        vtkGraph* graphInput, vtkGraph* graphOutput)
{
  if (dsInput)
    {
    dsOutput->CopyStructure(dsInput);
    dsOutput->CopyAttributes(dsInput);
    }
  else
    {
    graphOutput->ShallowCopy(graphInput);
    }
}

int vtkArrayCalculator::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject *output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  enum ResultType
  {
    SCALAR_RESULT,
    VECTOR_RESULT
  } resultType = SCALAR_RESULT;
  enum DataType
  {
    POINT_DATA,
    CELL_DATA
  } attributeDataType = POINT_DATA;
  vtkIdType i;
  int j;

  vtkDataSetAttributes* inFD = 0;
  vtkDataSetAttributes* outFD = 0;
  vtkDataArray* currentArray;
  vtkIdType numTuples = 0;
  double scalarResult[1];
  vtkDataArray* resultArray = 0;
  vtkPoints* resultPoints = 0;

  this->FunctionParser->SetReplaceInvalidValues(this->ReplaceInvalidValues);
  this->FunctionParser->SetReplacementValue(this->ReplacementValue);

  vtkDataSet *dsInput = vtkDataSet::SafeDownCast(input);
  vtkDataSet *dsOutput = vtkDataSet::SafeDownCast(output);
  vtkGraph *graphInput = vtkGraph::SafeDownCast(input);
  vtkGraph *graphOutput = vtkGraph::SafeDownCast(output);
  vtkPointSet* psInput = vtkPointSet::SafeDownCast(input);
  vtkPointSet* psOutput = vtkPointSet::SafeDownCast(output);
  if (dsInput)
    {
    if (this->AttributeMode == VTK_ATTRIBUTE_MODE_DEFAULT ||
        this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_POINT_DATA)
      {
      inFD = dsInput->GetPointData();
      outFD = dsOutput->GetPointData();
      attributeDataType = POINT_DATA;
      numTuples = dsInput->GetNumberOfPoints();
      }
    else
      {
      inFD = dsInput->GetCellData();
      outFD = dsOutput->GetCellData();
      attributeDataType = CELL_DATA;
      numTuples = dsInput->GetNumberOfCells();
      }
    }
  else if (graphInput)
    {
    if (this->AttributeMode == VTK_ATTRIBUTE_MODE_DEFAULT ||
        this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_VERTEX_DATA)
      {
      inFD = graphInput->GetVertexData();
      outFD = graphOutput->GetVertexData();
      attributeDataType = POINT_DATA;
      numTuples = graphInput->GetNumberOfVertices();
      }
    else
      {
      inFD = graphInput->GetEdgeData();
      outFD = graphOutput->GetEdgeData();
      attributeDataType = CELL_DATA;
      numTuples = graphInput->GetNumberOfEdges();
      }
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
    else if(inFD->GetAbstractArray(this->ScalarArrayNames[i]) == NULL) // We ignore string array
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

  if(attributeDataType == POINT_DATA)
    {
    for (i = 0; i < this->NumberOfCoordinateScalarArrays; i++)
      {
      double* pt = 0;
      if (dsInput)
        {
        pt = dsInput->GetPoint(0);
        }
      else
        {
        pt = graphInput->GetPoint(0);
        }
      this->FunctionParser->
        SetScalarVariableValue(
          this->CoordinateScalarVariableNames[i],
          pt[this->SelectedCoordinateScalarComponents[i]]);
      }

    for (i = 0; i < this->NumberOfCoordinateVectorArrays; i++)
      {
      double* pt = 0;
      if (dsInput)
        {
        pt = dsInput->GetPoint(0);
        }
      else
        {
        pt = graphInput->GetPoint(0);
        }
      this->FunctionParser->
        SetVectorVariableValue(
          this->CoordinateVectorVariableNames[i],
          pt[this->SelectedCoordinateVectorComponents[i][0]],
          pt[this->SelectedCoordinateVectorComponents[i][1]],
          pt[this->SelectedCoordinateVectorComponents[i][2]]);
      }
    }

  if ( !this->Function || strlen(this->Function) == 0)
    {
    CopyDataSetOrGraph(dsInput, dsOutput, graphInput, graphOutput);
    return 1;
    }
  else if (this->FunctionParser->IsScalarResult())
    {
    resultType = SCALAR_RESULT;
    }
  else if (this->FunctionParser->IsVectorResult())
    {
    resultType = VECTOR_RESULT;
    }
  else
    {
    CopyDataSetOrGraph(dsInput, dsOutput, graphInput, graphOutput);
    // Error occurred in vtkFunctionParser.
    vtkWarningMacro("An error occurred when parsing the calculator's function.  See previous errors.");
    return 1;
    }

  if(resultType == SCALAR_RESULT && this->ResultNormals)
    {
    vtkWarningMacro("ResultNormals specified but output is scalar");
    }

  if(resultType == VECTOR_RESULT &&
     CoordinateResults != 0 && (psOutput || graphOutput))
    {
    resultPoints = vtkPoints::New();
    resultPoints->SetNumberOfPoints(numTuples);
    resultArray = resultPoints->GetData();
    }
  else if(CoordinateResults != 0)
    {
    if(resultType != VECTOR_RESULT)
      {
      vtkErrorMacro("Coordinate output specified, "
                    "but there are no vector results");
      }
    else if(!psOutput)
      {
      vtkErrorMacro("Coordinate output specified, "
                    "but output is not polydata or unstructured grid");
      }
    return 1;
    }
  else
    {
      resultArray=
        vtkDataArray::SafeDownCast(vtkAbstractArray::CreateArray(this->ResultArrayType));
    }

  if (resultType == SCALAR_RESULT)
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
      if(currentArray)
        {
        this->FunctionParser->
          SetScalarVariableValue(
            j, currentArray->GetComponent(i, this->SelectedScalarComponents[j]));
        }
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
    if(attributeDataType == POINT_DATA)
      {
      double* pt = 0;
      if (dsInput)
        {
        pt = dsInput->GetPoint(i);
        }
      else
        {
        pt = graphInput->GetPoint(i);
        }
      for (j = 0; j < this->NumberOfCoordinateScalarArrays; j++)
        {
        this->FunctionParser->
          SetScalarVariableValue(
            j+this->NumberOfScalarArrays, pt[this->SelectedCoordinateScalarComponents[j]]);
        }
      for (j = 0; j < this->NumberOfCoordinateVectorArrays; j++)
        {
        this->FunctionParser->
          SetVectorVariableValue(
            j+this->NumberOfVectorArrays,
            pt[this->SelectedCoordinateVectorComponents[j][0]],
            pt[this->SelectedCoordinateVectorComponents[j][1]],
            pt[this->SelectedCoordinateVectorComponents[j][2]]);
        }
      }
    if (resultType == SCALAR_RESULT)
      {
      scalarResult[0] = this->FunctionParser->GetScalarResult();
      resultArray->SetTuple(i, scalarResult);
      }
    else
      {
      resultArray->SetTuple(i, this->FunctionParser->GetVectorResult());
      }
    }

  CopyDataSetOrGraph (dsInput, dsOutput, graphInput, graphOutput);
  if(resultPoints)
    {
    if(psInput)
      {
      if(attributeDataType == CELL_DATA)
        {
        vtkPolyData* pd = vtkPolyData::SafeDownCast(psOutput);
        vtkUnstructuredGrid* ug = vtkUnstructuredGrid::SafeDownCast(psOutput);
        if(pd)
          {
          pd->Reset();
          pd->Allocate(numTuples);
          for (i = 1; i < numTuples; i++)
            {
            pd->InsertNextCell(VTK_VERTEX, 1, &i);
            }
          }
        else if(ug)
          {
          ug->Reset();
          ug->Allocate(numTuples);
          for (i = 1; i < numTuples; i++)
            {
            ug->InsertNextCell(VTK_VERTEX, 1, &i);
            }
          }
        }
      psOutput->SetPoints(resultPoints);
      }
    resultPoints->Delete();
    }

  if(this->ResultTCoords || this->ResultNormals || ! this->CoordinateResults)
    {
    resultArray->SetName(this->ResultArrayName);
    outFD->AddArray(resultArray);
    if(resultType == SCALAR_RESULT)
      {
      if (this->ResultTCoords)
        {
        outFD->SetActiveTCoords(this->ResultArrayName);
        }
      else
        {
        outFD->SetActiveScalars(this->ResultArrayName);
        }
      }
    else
      {
      if (this->ResultTCoords || this ->ResultNormals)
        {
        if (this->ResultTCoords)
          {
          outFD->SetActiveTCoords(this->ResultArrayName);
          }
        if (this->ResultNormals)
          {
          outFD->SetActiveNormals(this->ResultArrayName);
          }
        }
      else
        {
        outFD->SetActiveVectors(this->ResultArrayName);
        }
      }
    if (! resultPoints)
      {
      resultArray->Delete();
      }
    }
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

void vtkArrayCalculator::AddCoordinateScalarVariable(const char* variableName,
                                                     int component)
{
  int i;
  char** varNames = new char *[this->NumberOfCoordinateScalarArrays];
  int* tempComponents = new int[this->NumberOfCoordinateScalarArrays];

  for (i = 0; i < this->NumberOfCoordinateScalarArrays; i++)
    {
    varNames[i] = new char[strlen(this->CoordinateScalarVariableNames[i]) + 1];
    strcpy(varNames[i], this->CoordinateScalarVariableNames[i]);
    delete [] this->CoordinateScalarVariableNames[i];
    this->CoordinateScalarVariableNames[i] = NULL;
    tempComponents[i] = this->SelectedCoordinateScalarComponents[i];
    }
  if (this->CoordinateScalarVariableNames)
    {
    delete [] this->CoordinateScalarVariableNames;
    this->CoordinateScalarVariableNames = NULL;
    }
  if (this->SelectedCoordinateScalarComponents)
    {
    delete [] this->SelectedCoordinateScalarComponents;
    this->SelectedCoordinateScalarComponents = NULL;
    }

  this->CoordinateScalarVariableNames =
    new char *[this->NumberOfCoordinateScalarArrays + 1];
  this->SelectedCoordinateScalarComponents =
    new int[this->NumberOfCoordinateScalarArrays + 1];

  for (i = 0; i < this->NumberOfCoordinateScalarArrays; i++)
    {
    this->CoordinateScalarVariableNames[i] = new char[strlen(varNames[i]) + 1];
    strcpy(this->CoordinateScalarVariableNames[i], varNames[i]);
    delete [] varNames[i];
    varNames[i] = NULL;
    this->SelectedCoordinateScalarComponents[i] = tempComponents[i];
    }
  delete [] varNames;
  delete [] tempComponents;

  this->CoordinateScalarVariableNames[i] = new char[strlen(variableName) + 1];
  strcpy(this->CoordinateScalarVariableNames[i], variableName);
  this->SelectedCoordinateScalarComponents[i] = component;

  this->NumberOfCoordinateScalarArrays++;
}

void vtkArrayCalculator::AddCoordinateVectorVariable(const char* variableName,
                                                     int component0,
                                                     int component1,
                                                     int component2)
{
  int i;
  char** varNames = new char *[this->NumberOfCoordinateVectorArrays];
  int** tempComponents = new int *[this->NumberOfCoordinateVectorArrays];

  for (i = 0; i < this->NumberOfCoordinateVectorArrays; i++)
    {
    varNames[i] = new char[strlen(this->CoordinateVectorVariableNames[i])+1];
    strcpy(varNames[i], this->CoordinateVectorVariableNames[i]);
    delete [] this->CoordinateVectorVariableNames[i];
    this->CoordinateVectorVariableNames[i] = NULL;
    tempComponents[i] = new int[3];
    tempComponents[i][0] = this->SelectedCoordinateVectorComponents[i][0];
    tempComponents[i][1] = this->SelectedCoordinateVectorComponents[i][1];
    tempComponents[i][2] = this->SelectedCoordinateVectorComponents[i][2];
    delete [] this->SelectedCoordinateVectorComponents[i];
    this->SelectedCoordinateVectorComponents[i] = NULL;
    }

  if (this->CoordinateVectorVariableNames)
    {
    delete [] this->CoordinateVectorVariableNames;
    this->CoordinateVectorVariableNames = NULL;
    }
  if (this->SelectedCoordinateVectorComponents)
    {
    delete [] this->SelectedCoordinateVectorComponents;
    this->SelectedCoordinateVectorComponents = NULL;
    }

  this->CoordinateVectorVariableNames =
    new char *[this->NumberOfCoordinateVectorArrays + 1];
  this->SelectedCoordinateVectorComponents =
    new int *[this->NumberOfCoordinateVectorArrays + 1];

  for (i = 0; i < this->NumberOfCoordinateVectorArrays; i++)
    {
    this->CoordinateVectorVariableNames[i] = new char[strlen(varNames[i]) + 1];
    strcpy(this->CoordinateVectorVariableNames[i], varNames[i]);
    delete [] varNames[i];
    varNames[i] = NULL;
    this->SelectedCoordinateVectorComponents[i] = new int[3];
    this->SelectedCoordinateVectorComponents[i][0] = tempComponents[i][0];
    this->SelectedCoordinateVectorComponents[i][1] = tempComponents[i][1];
    this->SelectedCoordinateVectorComponents[i][2] = tempComponents[i][2];
    delete [] tempComponents[i];
    tempComponents[i] = NULL;
    }
  delete [] varNames;
  delete [] tempComponents;

  this->CoordinateVectorVariableNames[i] = new char[strlen(variableName) + 1];
  strcpy(this->CoordinateVectorVariableNames[i], variableName);

  this->SelectedCoordinateVectorComponents[i] = new int[3];
  this->SelectedCoordinateVectorComponents[i][0] = component0;
  this->SelectedCoordinateVectorComponents[i][1] = component1;
  this->SelectedCoordinateVectorComponents[i][2] = component2;

  this->NumberOfCoordinateVectorArrays++;
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
  else if ( this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_CELL_DATA )
    {
    return "UseCellData";
    }
  else if ( this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_VERTEX_DATA )
    {
    return "UseVertexData";
    }
  else
    {
    return "UseEdgeData";
    }
}

void vtkArrayCalculator::RemoveScalarVariables()
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

  this->FunctionParser->RemoveScalarVariables();
}

void vtkArrayCalculator::RemoveVectorVariables()
{
  int i;

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

  this->FunctionParser->RemoveVectorVariables();
}

void vtkArrayCalculator::RemoveCoordinateScalarVariables()
{
  int i;

  for (i = 0; i < this->NumberOfCoordinateScalarArrays; i++)
    {
    delete [] this->CoordinateScalarVariableNames[i];
    this->CoordinateScalarVariableNames[i] = NULL;
    }
  if (this->NumberOfCoordinateScalarArrays > 0)
    {
    delete [] this->CoordinateScalarVariableNames;
    this->CoordinateScalarVariableNames = NULL;
    delete [] this->SelectedCoordinateScalarComponents;
    this->SelectedCoordinateScalarComponents = NULL;
    }
  this->NumberOfCoordinateScalarArrays = 0;

  this->FunctionParser->RemoveScalarVariables();
}

void vtkArrayCalculator::RemoveCoordinateVectorVariables()
{
  int i;

  for (i = 0; i < this->NumberOfCoordinateVectorArrays; i++)
    {
    delete [] this->CoordinateVectorVariableNames[i];
    this->CoordinateVectorVariableNames[i] = NULL;
    delete [] this->SelectedCoordinateVectorComponents[i];
    this->SelectedCoordinateVectorComponents[i] = NULL;
    }
  if (this->NumberOfVectorArrays > 0)
    {
    delete [] this->CoordinateVectorVariableNames;
    this->CoordinateVectorVariableNames = NULL;
    delete [] this->SelectedCoordinateVectorComponents;
    this->SelectedCoordinateVectorComponents = NULL;
    }
  this->NumberOfCoordinateVectorArrays = 0;

  this->FunctionParser->RemoveVectorVariables();
}

void vtkArrayCalculator::RemoveAllVariables()
{
  this->RemoveScalarVariables();
  this->RemoveVectorVariables();
  this->RemoveCoordinateScalarVariables();
  this->RemoveCoordinateVectorVariables();
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

  os << indent << "Result Array Type: " << vtkImageScalarTypeNameMacro(this->ResultArrayType) << endl;

  os << indent << "Coordinate Results: " << this->CoordinateResults << endl;
  os << indent << "Attribute Mode: " << this->GetAttributeModeAsString() << endl;
  os << indent << "Number Of Scalar Arrays: " << this->NumberOfScalarArrays
     << endl;
  os << indent << "Number Of Vector Arrays: " << this->NumberOfVectorArrays
     << endl;
  os << indent << "Number Of Coordinate Scalar Arrays: "
     << this->NumberOfCoordinateScalarArrays << endl;
  os << indent << "Number Of Coordinate Vector Arrays: "
     << this->NumberOfCoordinateVectorArrays << endl;
  os << indent << "Replace Invalid Values: "
     << (this->ReplaceInvalidValues ? "On" : "Off") << endl;
  os << indent << "Replacement Value: " << this->ReplacementValue << endl;
}
