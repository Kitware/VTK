// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkArrayCalculator.h"

#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkExprTkFunctionParser.h"
#include "vtkFieldData.h"
#include "vtkFunctionParser.h"
#include "vtkGraph.h"
#include "vtkHyperTreeGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMolecule.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkTable.h"
#include "vtkUnstructuredGrid.h"

#include <array>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkArrayCalculator);

//------------------------------------------------------------------------------
vtkArrayCalculator::vtkArrayCalculator()
{
  this->FunctionParserType = FunctionParserTypes::ExprTkFunctionParser;
  this->Function = nullptr;
  this->ResultArrayName = nullptr;
  this->SetResultArrayName("resultArray");
  this->ScalarArrayNames.clear();
  this->VectorArrayNames.clear();
  this->ScalarVariableNames.clear();
  this->VectorVariableNames.clear();
  this->AttributeType = DEFAULT_ATTRIBUTE_TYPE;
  this->SelectedScalarComponents.clear();
  this->CoordinateScalarVariableNames.clear();
  this->CoordinateVectorVariableNames.clear();
  this->SelectedCoordinateScalarComponents.clear();
  this->CoordinateResults = 0;
  this->ResultNormals = false;
  this->ResultTCoords = false;
  this->ReplaceInvalidValues = 0;
  this->ReplacementValue = 0.0;
  this->IgnoreMissingArrays = false;
  this->ResultArrayType = VTK_DOUBLE;
}

//------------------------------------------------------------------------------
vtkArrayCalculator::~vtkArrayCalculator()
{
  delete[] this->Function;
  this->Function = nullptr;

  delete[] this->ResultArrayName;
  this->ResultArrayName = nullptr;

  this->ScalarArrayNames.clear();
  this->VectorArrayNames.clear();
  this->ScalarVariableNames.clear();
  this->VectorVariableNames.clear();
  this->SelectedScalarComponents.clear();
  this->CoordinateScalarVariableNames.clear();
  this->CoordinateVectorVariableNames.clear();
  this->SelectedCoordinateScalarComponents.clear();
}

//------------------------------------------------------------------------------
int vtkArrayCalculator::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}

//------------------------------------------------------------------------------
template <typename TFunctionParser, typename TResultArray>
class vtkArrayCalculator::vtkArrayCalculatorFunctor
{
private:
  vtkArrayCalculator* Calculator;
  vtkDataObject* Input;
  vtkDataSet* DsInput;
  vtkGraph* GraphInput;

  int AttributeType;
  vtkDataSetAttributes* InFD;

  char* Function;
  vtkTypeBool ReplaceInvalidValues;
  double ReplacementValue;
  bool IgnoreMissingArrays;
  std::vector<std::string> ScalarArrayNames;
  std::vector<std::string> VectorArrayNames;
  std::vector<std::string> ScalarVariableNames;
  std::vector<std::string> VectorVariableNames;
  std::vector<int> SelectedScalarComponents;
  std::vector<std::string> CoordinateScalarVariableNames;
  std::vector<std::string> CoordinateVectorVariableNames;
  std::vector<int> SelectedCoordinateScalarComponents;

  int ScalarArrayNamesSize;
  int VectorArrayNamesSize;
  int CoordinateScalarVariableNamesSize;
  int CoordinateVectorVariableNamesSize;

  std::vector<vtkDataArray*> ScalarArrays;
  std::vector<vtkDataArray*> VectorArrays;
  std::vector<int> ScalarArrayIndices;
  std::vector<int> VectorArrayIndices;

  TResultArray* ResultArray;
  ResultTypes ResultType;

  // // thread local
  vtkSMPThreadLocal<vtkSmartPointer<TFunctionParser>> FunctionParser;
  vtkSMPThreadLocal<std::vector<double>> Tuple;
  int MaxTupleSize;

public:
  explicit vtkArrayCalculatorFunctor(vtkArrayCalculator* calculator, vtkDataObject* input,
    const std::vector<vtkDataArray*>& scalarArrays, const std::vector<vtkDataArray*>& vectorArrays,
    const std::vector<int>& scalarArrayIndices, const std::vector<int>& vectorArrayIndices,
    TResultArray* resultArray)
    : Calculator(calculator)
    , Input(input)
    , DsInput(vtkDataSet::SafeDownCast(input))
    , GraphInput(vtkGraph::SafeDownCast(input))
    , AttributeType(calculator->GetAttributeTypeFromInput(input))
    , InFD(input->GetAttributes(this->AttributeType))
    , Function(calculator->GetFunction())
    , ReplaceInvalidValues(calculator->GetReplaceInvalidValues())
    , ReplacementValue(calculator->GetReplacementValue())
    , IgnoreMissingArrays(calculator->GetIgnoreMissingArrays())
    , ScalarArrayNames(calculator->GetScalarArrayNames())
    , VectorArrayNames(calculator->GetVectorArrayNames())
    , ScalarVariableNames(calculator->GetScalarVariableNames())
    , VectorVariableNames(calculator->GetVectorVariableNames())
    , SelectedScalarComponents(calculator->GetSelectedScalarComponents())
    , CoordinateScalarVariableNames(calculator->GetCoordinateScalarVariableNames())
    , CoordinateVectorVariableNames(calculator->GetCoordinateVectorVariableNames())
    , SelectedCoordinateScalarComponents(calculator->GetSelectedCoordinateScalarComponents())
    , ScalarArrayNamesSize(static_cast<int>(this->ScalarArrayNames.size()))
    , VectorArrayNamesSize(static_cast<int>(this->VectorArrayNames.size()))
    , CoordinateScalarVariableNamesSize(
        static_cast<int>(this->CoordinateScalarVariableNames.size()))
    , CoordinateVectorVariableNamesSize(
        static_cast<int>(this->CoordinateVectorVariableNames.size()))
    , ScalarArrays(scalarArrays)
    , VectorArrays(vectorArrays)
    , ScalarArrayIndices(scalarArrayIndices)
    , VectorArrayIndices(vectorArrayIndices)
    , ResultArray(resultArray)
    , ResultType(resultArray->GetNumberOfComponents() == 1 ? SCALAR : VECTOR)
  {
    // find the maximum tuple size
    this->MaxTupleSize = 3;
    for (int i = 0; i < this->ScalarArrayNamesSize; i++)
    {
      if (auto scalarArray = this->InFD->GetAbstractArray(this->ScalarArrayNames[i].c_str()))
      {
        this->MaxTupleSize = std::max(this->MaxTupleSize, scalarArray->GetNumberOfComponents());
      }
    }
    for (int i = 0; i < this->VectorArrayNamesSize; i++)
    {
      if (auto vectorArray = this->InFD->GetAbstractArray(this->VectorArrayNames[i].c_str()))
      {
        this->MaxTupleSize = std::max(this->MaxTupleSize, vectorArray->GetNumberOfComponents());
      }
    }
  }

  /**
   * Initialize only what the thread-function-parser needs.
   */
  void Initialize()
  {
    auto& functionParser = this->FunctionParser.Local();
    this->Tuple.Local().resize(static_cast<size_t>(this->MaxTupleSize));
    functionParser =
      this->Calculator->template InitializeFunctionParser<TFunctionParser>(this->Input);
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    auto resultArrayItr = vtk::DataArrayTupleRange(this->ResultArray, begin, end).begin();
    auto& functionParser = this->FunctionParser.Local();
    auto tuple = this->Tuple.Local().data();
    vtkDataArray* currentArray;

    for (vtkIdType i = begin; i < end; i++, resultArrayItr++)
    {
      for (int j = 0; j < this->ScalarArrayNamesSize; j++)
      {
        if ((currentArray = this->ScalarArrays[j]))
        {
          functionParser->SetScalarVariableValue(this->ScalarArrayIndices[j],
            currentArray->GetComponent(i, this->SelectedScalarComponents[j]));
        }
      }
      for (int j = 0; j < this->VectorArrayNamesSize; j++)
      {
        if ((currentArray = this->VectorArrays[j]))
        {
          currentArray->GetTuple(i, tuple);
          functionParser->SetVectorVariableValue(
            this->VectorArrayIndices[j], tuple, currentArray->GetNumberOfComponents());
        }
      }
      if (this->AttributeType == vtkDataObject::POINT ||
        this->AttributeType == vtkDataObject::VERTEX)
      {
        double pt[3];
        if (this->DsInput)
        {
          this->DsInput->GetPoint(i, pt);
        }
        else
        {
          this->GraphInput->GetPoint(i, pt);
        }
        for (int j = 0; j < this->CoordinateScalarVariableNamesSize; j++)
        {
          functionParser->SetScalarVariableValue(
            j + this->ScalarArrayNamesSize, pt[this->SelectedCoordinateScalarComponents[j]]);
        }
        for (int j = 0; j < this->CoordinateVectorVariableNamesSize; j++)
        {
          functionParser->SetVectorVariableValue(j + this->VectorArrayNamesSize, pt, 3);
        }
      }
      if (this->ResultType == SCALAR)
      {
        (*resultArrayItr)[0] = functionParser->GetScalarResult();
      }
      else
      {
        auto result = functionParser->GetVectorResult();
        std::copy_n(result, functionParser->GetResultSize(), (*resultArrayItr).begin());
      }
    }
  }

  void Reduce() {}
};

//------------------------------------------------------------------------------
template <typename TFunctionParser>
struct vtkArrayCalculator::vtkArrayCalculatorWorker
{
  template <typename TResultArray>
  void operator()(TResultArray* resultArray, vtkArrayCalculator* calculator, vtkDataObject* input,
    const std::vector<vtkDataArray*>& scalarArrays, const std::vector<vtkDataArray*>& vectorArrays,
    const std::vector<int>& scalarArrayIndices, const std::vector<int>& vectorArrayIndices,
    vtkIdType numTuples)
  {
    // Execute functor for all tuples
    vtkArrayCalculatorFunctor<TFunctionParser, TResultArray> arrayCalculatorFunctor(calculator,
      input, scalarArrays, vectorArrays, scalarArrayIndices, vectorArrayIndices, resultArray);

    vtkIdType grain = 0;
    if (resultArray->GetDataType() == VTK_BIT)
    {
      // The grain size needs to be defined to prevent false sharing
      // when writing to a vtkBitArray.
      grain = sizeof(vtkIdType) * 64;
    }
    vtkSMPTools::For(0, numTuples, grain, arrayCalculatorFunctor);
  }
};

//------------------------------------------------------------------------------
template <typename TFunctionParser>
vtkSmartPointer<TFunctionParser> vtkArrayCalculator::InitializeFunctionParser(
  vtkDataObject* input) const
{
  vtkDataSet* dsInput = vtkDataSet::SafeDownCast(input);
  vtkGraph* graphInput = vtkGraph::SafeDownCast(input);

  int attributeType = this->GetAttributeTypeFromInput(input);

  vtkDataSetAttributes* inFD = input->GetAttributes(attributeType);
  vtkIdType numTuples = input->GetNumberOfElements(attributeType);

  auto functionParser = vtkSmartPointer<TFunctionParser>::New();
  functionParser->SetReplaceInvalidValues(this->ReplaceInvalidValues);
  functionParser->SetReplacementValue(this->ReplacementValue);
  functionParser->SetFunction(this->Function);

  if (numTuples < 1)
  {
    vtkDebugMacro("Empty data.");
    return nullptr;
  }

  vtkDataArray* currentArray;

  // Tell the parser about scalar arrays
  for (size_t i = 0; i < this->ScalarArrayNames.size(); i++)
  {
    currentArray = inFD->GetArray(this->ScalarArrayNames[i].c_str());
    if (currentArray)
    {
      if (currentArray->GetNumberOfComponents() > this->SelectedScalarComponents[i])
      {
        functionParser->SetScalarVariableValue(this->ScalarVariableNames[i],
          currentArray->GetComponent(0, this->SelectedScalarComponents[i]));
      }
      else
      {
        vtkErrorMacro(
          "Array " << this->ScalarArrayNames[i] << " does not contain the selected component.");
        return nullptr;
      }
    }
    else if (this->IgnoreMissingArrays)
    {
      // Add a dummy value with the variable name. We'll skip it if the variable is
      // actually needed when collecting the arrays needed for evaluation later on.
      functionParser->SetScalarVariableValue(this->ScalarVariableNames[i], 0.0);
    }
    else if (inFD->GetAbstractArray(this->ScalarArrayNames[i].c_str()) ==
      nullptr) // We ignore string array
    {
      vtkErrorMacro("Invalid array name: " << this->ScalarArrayNames[i]);
      return nullptr;
    }
  }

  // Tell the parser about vector arrays
  for (size_t i = 0; i < this->VectorArrayNames.size(); i++)
  {
    currentArray = inFD->GetArray(this->VectorArrayNames[i].c_str());
    if (currentArray)
    {
      functionParser->SetVectorVariableValue(this->VectorVariableNames[i],
        currentArray->GetTuple(0), currentArray->GetNumberOfComponents());
    }
    else if (this->IgnoreMissingArrays)
    {
      // Add a dummy value with the variable name. We'll skip it if the variable is
      // actually needed when collecting the arrays needed for evaluation later on.
      functionParser->SetVectorVariableValue(this->VectorVariableNames[i], 0.0, 0.0, 0.0);
    }
    else if (inFD->GetAbstractArray(this->VectorArrayNames[i].c_str()) ==
      nullptr) // We ignore string array
    {
      vtkErrorMacro("Invalid array name: " << this->VectorArrayNames[i]);
      return nullptr;
    }
  }

  // Tell the parser about the coordinate arrays
  if (attributeType == vtkDataObject::POINT || attributeType == vtkDataObject::VERTEX)
  {
    double pt[3];
    for (size_t i = 0; i < this->CoordinateScalarVariableNames.size(); i++)
    {
      if (dsInput)
      {
        dsInput->GetPoint(0, pt);
      }
      else
      {
        graphInput->GetPoint(0, pt);
      }
      functionParser->SetScalarVariableValue(
        this->CoordinateScalarVariableNames[i], pt[this->SelectedCoordinateScalarComponents[i]]);
    }

    for (size_t i = 0; i < this->CoordinateVectorVariableNames.size(); i++)
    {
      if (dsInput)
      {
        dsInput->GetPoint(0, pt);
      }
      else
      {
        graphInput->GetPoint(0, pt);
      }
      functionParser->SetVectorVariableValue(this->CoordinateVectorVariableNames[i], pt, 3);
    }
  }
  return functionParser;
}

//------------------------------------------------------------------------------
template <typename TFunctionParser>
int vtkArrayCalculator::ProcessDataObject(vtkDataObject* input, vtkDataObject* output)
{
  if (!this->Function || strlen(this->Function) == 0)
  {
    output->ShallowCopy(input);
    return 1;
  }

  auto functionParser = this->InitializeFunctionParser<TFunctionParser>(input);
  if (!functionParser)
  {
    return 0;
  }

  int attributeType = this->GetAttributeTypeFromInput(input);
  vtkDataSetAttributes* inFD = input->GetAttributes(attributeType);
  vtkIdType numTuples = input->GetNumberOfElements(attributeType);

  vtkDataSetAttributes* outFD = output->GetAttributes(attributeType);
  vtkPointSet* psOutput = vtkPointSet::SafeDownCast(output);

  ResultTypes resultType;
  if (functionParser->IsScalarResult())
  {
    resultType = SCALAR;
  }
  else if (functionParser->IsVectorResult())
  {
    resultType = VECTOR;
  }
  else
  {
    output->ShallowCopy(input);
    // Error occurred in FunctionParser.
    vtkWarningMacro(
      "An error occurred when parsing the calculator's function.  See previous errors.");
    return 0;
  }

  if (resultType == SCALAR && this->ResultNormals)
  {
    vtkWarningMacro("ResultNormals specified but output is scalar");
  }

  vtkMolecule* moleculeInput = vtkMolecule::SafeDownCast(input);
  if (moleculeInput && attributeType == vtkDataObject::VERTEX &&
    strcmp(this->ResultArrayName, moleculeInput->GetAtomicNumberArrayName()) == 0)
  {
    vtkErrorMacro("Cannot override atomic numbers array");
    return 0;
  }

  if (moleculeInput && attributeType == vtkDataObject::EDGE &&
    strcmp(this->ResultArrayName, moleculeInput->GetBondOrdersArrayName()) == 0)
  {
    vtkErrorMacro("Cannot override bond orders array");
    return 0;
  }

  auto resultArray = vtk::TakeSmartPointer(vtkDataArray::CreateDataArray(this->ResultArrayType));
  vtkSmartPointer<vtkPoints> resultPoints;
  if (resultType == VECTOR && this->CoordinateResults != 0 &&
    (psOutput || vtkGraph::SafeDownCast(output)))
  {
    resultPoints = vtkSmartPointer<vtkPoints>::New();
    resultPoints->SetData(resultArray);
  }
  else if (this->CoordinateResults != 0)
  {
    if (resultType != VECTOR)
    {
      vtkErrorMacro("Coordinate output specified, "
                    "but there are no vector results");
    }
    else if (!psOutput)
    {
      vtkErrorMacro("Coordinate output specified, "
                    "but output is not polydata or unstructured grid");
    }
    return 1;
  }
  if (resultType == SCALAR)
  {
    resultArray->SetNumberOfComponents(1);
    resultArray->SetNumberOfTuples(numTuples);
    resultArray->SetComponent(0, 0, functionParser->GetScalarResult());
  }
  else
  {
    resultArray->SetNumberOfComponents(functionParser->GetResultSize());
    resultArray->SetNumberOfTuples(numTuples);
    resultArray->SetTuple(0, functionParser->GetVectorResult());
  }

  // Save array pointers to avoid looking them up for each tuple.
  std::vector<vtkDataArray*> scalarArrays(this->ScalarArrayNames.size());
  std::vector<vtkDataArray*> vectorArrays(this->VectorArrayNames.size());
  std::vector<int> scalarArrayIndices(this->ScalarArrayNames.size());
  std::vector<int> vectorArrayIndices(this->VectorArrayNames.size());

  for (size_t cc = 0; cc < this->ScalarArrayNames.size(); cc++)
  {
    int idx = functionParser->GetScalarVariableIndex(this->ScalarVariableNames[cc]);
    if (idx >= 0)
    {
      bool needed = functionParser->GetScalarVariableNeeded(idx);
      auto array = inFD->GetArray(this->ScalarArrayNames[cc].c_str());
      if (needed && array)
      {
        scalarArrays[cc] = array;
        scalarArrayIndices[cc] = idx;
      }
      else if (needed)
      {
        // Skip this dataset altogether. This is an array specifically requested to be available
        // as a variable by the user of this class that does not exist on this dataset.
        return 1;
      }
    }
  }

  for (size_t cc = 0; cc < this->VectorArrayNames.size(); cc++)
  {
    int idx = functionParser->GetVectorVariableIndex(this->VectorVariableNames[cc]);
    if (idx >= 0)
    {
      bool needed = functionParser->GetVectorVariableNeeded(idx);
      auto array = inFD->GetArray(this->VectorArrayNames[cc].c_str());
      if (needed && array)
      {
        vectorArrays[cc] = array;
        vectorArrayIndices[cc] = idx;
      }
      else if (needed)
      {
        // Skip this dataset altogether. This is an array specifically requested to be available
        // as a variable by the user of this class that does not exist on this dataset.
        return 1;
      }
    }
  }

  vtkArrayCalculatorWorker<TFunctionParser> arrayCalculatorWorker;
  if (!vtkArrayDispatch::Dispatch::Execute(resultArray.Get(), arrayCalculatorWorker, this, input,
        scalarArrays, vectorArrays, scalarArrayIndices, vectorArrayIndices, numTuples))
  {
    arrayCalculatorWorker(resultArray.Get(), this, input, scalarArrays, vectorArrays,
      scalarArrayIndices, vectorArrayIndices, numTuples);
  }

  output->ShallowCopy(input);
  if (resultPoints)
  {
    if (psOutput)
    {
      if (attributeType == vtkDataObject::CELL)
      {
        vtkPolyData* pd = vtkPolyData::SafeDownCast(psOutput);
        vtkUnstructuredGrid* ug = vtkUnstructuredGrid::SafeDownCast(psOutput);
        if (pd)
        {
          pd->Reset();
          vtkNew<vtkIdTypeArray> ids;
          ids->SetNumberOfValues(numTuples);
          std::iota(ids->Begin(), ids->End(), 0);
          vtkNew<vtkCellArray> cellArray;
          cellArray->SetData(1, ids);
          pd->SetVerts(cellArray);
        }
        else if (ug)
        {
          ug->Reset();
          vtkNew<vtkIdTypeArray> ids;
          ids->SetNumberOfValues(numTuples);
          std::iota(ids->Begin(), ids->End(), 0);
          vtkNew<vtkCellArray> cellArray;
          cellArray->SetData(1, ids);
          ug->SetCells(VTK_VERTEX, cellArray);
        }
      }
      psOutput->SetPoints(resultPoints);
    }
  }

  if (this->ResultTCoords || this->ResultNormals || !this->CoordinateResults)
  {
    resultArray->SetName(this->ResultArrayName);
    outFD->AddArray(resultArray);
    if (resultType == SCALAR)
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
      if (this->ResultTCoords || this->ResultNormals)
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
      else if (functionParser->GetResultSize() == 3)
      {
        outFD->SetActiveVectors(this->ResultArrayName);
      }
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkArrayCalculator::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());

  vtkCompositeDataSet* inputCD = vtkCompositeDataSet::GetData(inputVector[0], 0);
  vtkCompositeDataSet* outputCD = vtkCompositeDataSet::SafeDownCast(output);
  if (inputCD && outputCD)
  {
    int success = 1;

    // Copy the output structure
    outputCD->CopyStructure(inputCD);

    vtkSmartPointer<vtkCompositeDataIterator> cdIter;
    cdIter.TakeReference(inputCD->NewIterator());
    cdIter->SkipEmptyNodesOn();
    for (cdIter->InitTraversal(); !cdIter->IsDoneWithTraversal(); cdIter->GoToNextItem())
    {
      vtkDataObject* inputDataObject = cdIter->GetCurrentDataObject();
      vtkDataObject* outputDataObject = inputDataObject->NewInstance();
      outputDataObject->ShallowCopy(inputDataObject);
      outputCD->SetDataSet(cdIter, outputDataObject);
      outputDataObject->FastDelete();

      if (this->FunctionParserType == FunctionParser)
      {
        success *= this->ProcessDataObject<vtkFunctionParser>(inputDataObject, outputDataObject);
      }
      else if (this->FunctionParserType == ExprTkFunctionParser)
      {
        success *=
          this->ProcessDataObject<vtkExprTkFunctionParser>(inputDataObject, outputDataObject);
      }
      else
      {
        vtkErrorMacro("FunctionParserType is not supported");
        return 1;
      }
    }

    return success;
  }

  // Not a composite data set.
  if (this->FunctionParserType == FunctionParser)
  {
    return this->ProcessDataObject<vtkFunctionParser>(input, output);
  }
  else if (this->FunctionParserType == ExprTkFunctionParser)
  {
    return this->ProcessDataObject<vtkExprTkFunctionParser>(input, output);
  }
  else
  {
    vtkErrorMacro("FunctionParserType is not supported");
    return 1;
  }
}

//------------------------------------------------------------------------------
int vtkArrayCalculator::GetAttributeTypeFromInput(vtkDataObject* input) const
{
  vtkDataSet* dsInput = vtkDataSet::SafeDownCast(input);
  vtkHyperTreeGrid* htgInput = vtkHyperTreeGrid::SafeDownCast(input);
  vtkGraph* graphInput = vtkGraph::SafeDownCast(input);
  vtkTable* tableInput = vtkTable::SafeDownCast(input);

  int attribute = this->AttributeType;
  if (attribute == DEFAULT_ATTRIBUTE_TYPE)
  {
    if (dsInput)
    {
      attribute = vtkDataObject::POINT;
    }
    else if (htgInput)
    {
      attribute = vtkDataObject::CELL;
    }
    else if (graphInput)
    {
      attribute = vtkDataObject::VERTEX;
    }
    else if (tableInput)
    {
      attribute = vtkDataObject::ROW;
    }
    else
    {
      vtkErrorMacro("Unsupported input type");
      return 1;
    }
  }

  return attribute;
}

//------------------------------------------------------------------------------
std::string vtkArrayCalculator::CheckValidVariableName(const char* variableName)
{
  // check if it's sanitized or enclosed in quotes
  if (vtkExprTkFunctionParser::SanitizeName(variableName) == variableName)
  {
    return std::string(variableName);
  }
  else if (variableName[0] == '\"' && variableName[strlen(variableName) - 1] == '\"')
  {
    return std::string(variableName);
  }
  else // enclose it in quotes
  {
    return std::string('\"' + std::string(variableName) + '\"');
  }
}

//------------------------------------------------------------------------------
void vtkArrayCalculator::AddScalarArrayName(const char* arrayName, int component)
{
  if (!arrayName)
  {
    return;
  }

  std::string validVariableName = vtkArrayCalculator::CheckValidVariableName(arrayName);

  for (size_t i = 0; i < this->ScalarArrayNames.size(); i++)
  {
    if (this->ScalarVariableNames[i] == arrayName &&
      this->ScalarArrayNames[i] == validVariableName &&
      this->SelectedScalarComponents[i] == component)
    {
      // Already have this variable/array/components so return.
      return;
    }
  }

  this->ScalarArrayNames.emplace_back(arrayName);
  this->ScalarVariableNames.push_back(validVariableName);
  this->SelectedScalarComponents.push_back(component);
}

//------------------------------------------------------------------------------
void vtkArrayCalculator::AddVectorArrayName(const char* arrayName)
{
  if (!arrayName)
  {
    return;
  }

  std::string validVariableName = vtkArrayCalculator::CheckValidVariableName(arrayName);

  for (size_t i = 0; i < this->VectorArrayNames.size(); i++)
  {
    if (this->VectorVariableNames[i] == arrayName && this->VectorArrayNames[i] == validVariableName)
    {
      // Already have this variable/array so return.
      return;
    }
  }

  this->VectorArrayNames.emplace_back(arrayName);
  this->VectorVariableNames.push_back(validVariableName);
}

//------------------------------------------------------------------------------
void vtkArrayCalculator::AddScalarVariable(
  const char* variableName, const char* arrayName, int component)
{
  if (!variableName || !arrayName)
  {
    return;
  }

  // if variable name is not valid, ignore this addition
  if (vtkArrayCalculator::CheckValidVariableName(variableName) != variableName)
  {
    vtkWarningMacro("Variable name is not valid!");
    return;
  }

  for (size_t i = 0; i < this->ScalarArrayNames.size(); i++)
  {
    if (this->ScalarVariableNames[i] == variableName && this->ScalarArrayNames[i] == arrayName &&
      this->SelectedScalarComponents[i] == component)
    {
      // Already have this variable/array/components so return.
      return;
    }
  }

  this->ScalarArrayNames.emplace_back(arrayName);
  this->ScalarVariableNames.emplace_back(variableName);
  this->SelectedScalarComponents.push_back(component);
}

//------------------------------------------------------------------------------
void vtkArrayCalculator::AddVectorVariable(const char* variableName, const char* arrayName)
{
  if (!variableName || !arrayName)
  {
    return;
  }

  // if variable name is not valid, ignore this addition
  if (vtkArrayCalculator::CheckValidVariableName(variableName) != variableName)
  {
    vtkWarningMacro("Variable name is not valid!");
    return;
  }

  for (size_t i = 0; i < this->VectorArrayNames.size(); i++)
  {
    if (this->VectorVariableNames[i] == variableName && this->VectorArrayNames[i] == arrayName)
    {
      // Already have this variable/array so return.
      return;
    }
  }

  this->VectorArrayNames.emplace_back(arrayName);
  this->VectorVariableNames.emplace_back(variableName);
}

//------------------------------------------------------------------------------
void vtkArrayCalculator::AddCoordinateScalarVariable(const char* variableName, int component)
{
  if (!variableName)
  {
    return;
  }

  // if variable name is not valid, ignore this addition
  if (vtkArrayCalculator::CheckValidVariableName(variableName) != variableName)
  {
    vtkWarningMacro("Variable name is not valid!");
    return;
  }

  this->CoordinateScalarVariableNames.emplace_back(variableName);
  this->SelectedCoordinateScalarComponents.push_back(component);
}

//------------------------------------------------------------------------------
void vtkArrayCalculator::AddCoordinateVectorVariable(const char* variableName)
{
  if (!variableName)
  {
    return;
  }

  // if variable name is not valid, ignore this addition
  if (vtkArrayCalculator::CheckValidVariableName(variableName) != variableName)
  {
    vtkWarningMacro("Variable name is not valid!");
    return;
  }

  this->CoordinateVectorVariableNames.emplace_back(variableName);
}

//------------------------------------------------------------------------------
const char* vtkArrayCalculator::GetAttributeTypeAsString()
{
  switch (this->AttributeType)
  {
    default:
    case DEFAULT_ATTRIBUTE_TYPE:
      return "Default";
    case vtkDataObject::POINT:
      return "UsePointData";
    case vtkDataObject::CELL:
      return "UseCellData";
    case vtkDataObject::VERTEX:
      return "UseVertexData";
    case vtkDataObject::EDGE:
      return "UseEdgeData";
    case vtkDataObject::ROW:
      return "UseRowData";
  }
}

//------------------------------------------------------------------------------
void vtkArrayCalculator::RemoveScalarVariables()
{
  this->ScalarArrayNames.clear();
  this->ScalarVariableNames.clear();
  this->SelectedScalarComponents.clear();
}

//------------------------------------------------------------------------------
void vtkArrayCalculator::RemoveVectorVariables()
{
  this->VectorArrayNames.clear();
  this->VectorVariableNames.clear();
}

//------------------------------------------------------------------------------
void vtkArrayCalculator::RemoveCoordinateScalarVariables()
{
  this->CoordinateScalarVariableNames.clear();
  this->SelectedCoordinateScalarComponents.clear();
}

//------------------------------------------------------------------------------
void vtkArrayCalculator::RemoveCoordinateVectorVariables()
{
  this->CoordinateVectorVariableNames.clear();
}

//------------------------------------------------------------------------------
void vtkArrayCalculator::RemoveAllVariables()
{
  this->RemoveScalarVariables();
  this->RemoveVectorVariables();
  this->RemoveCoordinateScalarVariables();
  this->RemoveCoordinateVectorVariables();
}

//------------------------------------------------------------------------------
std::string vtkArrayCalculator::GetScalarArrayName(int i)
{
  if (i < static_cast<int>(this->ScalarArrayNames.size()))
  {
    return this->ScalarArrayNames[i];
  }
  return std::string();
}

//------------------------------------------------------------------------------
std::string vtkArrayCalculator::GetVectorArrayName(int i)
{
  if (i < static_cast<int>(this->VectorArrayNames.size()))
  {
    return this->VectorArrayNames[i];
  }
  return std::string();
}

//------------------------------------------------------------------------------
std::string vtkArrayCalculator::GetScalarVariableName(int i)
{
  if (i < static_cast<int>(this->ScalarVariableNames.size()))
  {
    return this->ScalarVariableNames[i];
  }
  return std::string();
}

//------------------------------------------------------------------------------
std::string vtkArrayCalculator::GetVectorVariableName(int i)
{
  if (i < static_cast<int>(this->VectorVariableNames.size()))
  {
    return this->VectorVariableNames[i];
  }
  return std::string();
}

//------------------------------------------------------------------------------
int vtkArrayCalculator::GetSelectedScalarComponent(int i)
{
  if (i < static_cast<int>(this->ScalarArrayNames.size()))
  {
    return this->SelectedScalarComponents[i];
  }
  return -1;
}

static std::vector<vtkTuple<int, 3>> vectorComponents;

//------------------------------------------------------------------------------
const std::vector<vtkTuple<int, 3>>& vtkArrayCalculator::GetSelectedVectorComponents()
{
  return vectorComponents;
}

//------------------------------------------------------------------------------
std::string vtkArrayCalculator::GetCoordinateScalarVariableName(int i)
{
  if (i < static_cast<int>(this->CoordinateScalarVariableNames.size()))
  {
    return this->CoordinateScalarVariableNames[i];
  }
  return std::string();
}

//------------------------------------------------------------------------------
std::string vtkArrayCalculator::GetCoordinateVectorVariableName(int i)
{
  if (i < static_cast<int>(this->CoordinateVectorVariableNames.size()))
  {
    return this->CoordinateVectorVariableNames[i];
  }
  return std::string();
}

//------------------------------------------------------------------------------
int vtkArrayCalculator::GetSelectedCoordinateScalarComponent(int i)
{
  if (i < static_cast<int>(this->CoordinateScalarVariableNames.size()))
  {
    return this->SelectedCoordinateScalarComponents[i];
  }
  return -1;
}

//------------------------------------------------------------------------------
vtkDataSet* vtkArrayCalculator::GetDataSetOutput()
{
  return vtkDataSet::SafeDownCast(this->GetOutput());
}

//------------------------------------------------------------------------------
void vtkArrayCalculator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Function: " << (this->Function ? this->Function : "(none)") << endl;
  os << indent << "FunctionParserType: " << this->FunctionParserType << endl;
  os << indent
     << "Result Array Name: " << (this->ResultArrayName ? this->ResultArrayName : "(none)") << endl;

  os << indent << "Result Array Type: " << vtkImageScalarTypeNameMacro(this->ResultArrayType)
     << endl;

  os << indent << "Coordinate Results: " << this->CoordinateResults << endl;
  os << indent << "Attribute Type: " << this->GetAttributeTypeAsString() << endl;
  os << indent << "Replace Invalid Values: " << (this->ReplaceInvalidValues ? "On" : "Off") << endl;
  os << indent << "Replacement Value: " << this->ReplacementValue << endl;
}
VTK_ABI_NAMESPACE_END
