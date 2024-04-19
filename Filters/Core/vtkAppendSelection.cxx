// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAppendSelection.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"

#include <vtksys/RegularExpression.hxx>

#include <algorithm>
#include <array>
#include <sstream>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
namespace
{
//------------------------------------------------------------------------------
void ReplaceStringUsingRegex(std::string& source, vtksys::RegularExpression& regex,
  const std::string& replace, const std::string& with)
{
  // find all matches
  std::vector<std::string> matches;
  for (size_t next = 0; regex.find(source.substr(next)); next += regex.end())
  {
    const auto mathWord = source.substr(next + regex.start(), regex.end() - regex.start());
    if (mathWord == replace)
    {
      matches.push_back(mathWord);
    }
  }
  // replace all matches
  for (const auto& match : matches)
  {
    const auto pos = source.find(match);
    if (pos != std::string::npos)
    {
      source.replace(pos, match.length(), with);
    }
  }
}
}

//------------------------------------------------------------------------------
class vtkAppendSelection::vtkInternals
{
public:
  std::vector<std::string> Names;
  std::vector<std::array<double, 3>> Colors;
  vtksys::RegularExpression RegExNodeId;
  vtksys::RegularExpression RegExNodeIdInExpression;

  vtkInternals()
    : RegExNodeId("^[a-zA-Z0-9]+$")
    , RegExNodeIdInExpression("[a-zA-Z0-9]+")
  {
  }
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkAppendSelection);

//------------------------------------------------------------------------------
vtkAppendSelection::vtkAppendSelection()
  : UserManagedInputs(false)
  , AppendByUnion(true)
  , Inverse(false)
  , Internals(new vtkAppendSelection::vtkInternals())
{
}

//------------------------------------------------------------------------------
vtkAppendSelection::~vtkAppendSelection() = default;

//------------------------------------------------------------------------------
void vtkAppendSelection::SetInputName(int index, const char* name)
{
  if (index < 0)
  {
    vtkErrorMacro("Invalid index specified '" << index << "'.");
    return;
  }
  const std::string safeInputSelectionName = name ? name : "";
  if (safeInputSelectionName.empty())
  {
    vtkErrorMacro("Empty input selection name");
    return;
  }
  else if (!this->Internals->RegExNodeId.find(safeInputSelectionName))
  {
    vtkErrorMacro("`" << safeInputSelectionName << "` is not in the expected form.");
    return;
  }
  try
  {
    auto& currentName = this->Internals->Names.at(index);
    if (currentName != safeInputSelectionName)
    {
      if (std::find(this->Internals->Names.begin(), this->Internals->Names.end(),
            safeInputSelectionName) != this->Internals->Names.end())
      {
        vtkErrorMacro("Input selection name already exists: " << safeInputSelectionName);
      }
      else
      {
        currentName = safeInputSelectionName;
        this->Modified();
      }
    }
  }
  catch (std::out_of_range&)
  {
    this->Internals->Names.resize(index + 1);
    if (std::find(this->Internals->Names.begin(), this->Internals->Names.end(),
          safeInputSelectionName) != this->Internals->Names.end())
    {
      vtkErrorMacro("Input selection name already exists: " << safeInputSelectionName);
    }
    else
    {
      this->Internals->Names[index] = safeInputSelectionName;
      this->Modified();
    }
  }
}

//------------------------------------------------------------------------------
const char* vtkAppendSelection::GetInputName(int index) const
{
  if (index < 0 || static_cast<size_t>(index) >= this->Internals->Names.size())
  {
    vtkErrorMacro("Invalid index: " << index);
    return nullptr;
  }
  return this->Internals->Names[index].c_str();
}

//------------------------------------------------------------------------------
void vtkAppendSelection::RemoveAllInputNames()
{
  if (!this->Internals->Names.empty())
  {
    this->Internals->Names.clear();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkAppendSelection::SetInputColor(int index, double r, double g, double b)
{
  if (index < 0)
  {
    vtkErrorMacro("Invalid index specified '" << index << "'.");
    return;
  }

  if (this->Internals->Colors.size() <= static_cast<size_t>(index))
  {
    this->Internals->Colors.resize(index + 1);
  }

  this->Internals->Colors[index][0] = r;
  this->Internals->Colors[index][1] = g;
  this->Internals->Colors[index][2] = b;
  this->Modified();
}

//------------------------------------------------------------------------------
double* vtkAppendSelection::GetInputColor(int index) const
{
  if (index < 0 || static_cast<size_t>(index) >= this->Internals->Colors.size())
  {
    vtkErrorMacro("Invalid index: " << index);
    return nullptr;
  }
  return this->Internals->Colors[index].data();
}

//------------------------------------------------------------------------------
void vtkAppendSelection::RemoveAllInputColors()
{
  if (!this->Internals->Colors.empty())
  {
    this->Internals->Colors.clear();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
// Add a dataset to the list of data to append.
void vtkAppendSelection::AddInputData(vtkSelection* ds)
{
  if (this->UserManagedInputs)
  {
    vtkErrorMacro(<< "AddInput is not supported if UserManagedInputs is true");
    return;
  }
  this->AddInputDataInternal(0, ds);
}

//------------------------------------------------------------------------------
// Remove a dataset from the list of data to append.
void vtkAppendSelection::RemoveInputData(vtkSelection* ds)
{
  if (this->UserManagedInputs)
  {
    vtkErrorMacro(<< "RemoveInput is not supported if UserManagedInputs is true");
    return;
  }

  if (!ds)
  {
    return;
  }
  int numCons = this->GetNumberOfInputConnections(0);
  for (int i = 0; i < numCons; i++)
  {
    if (this->GetInput(i) == ds)
    {
      this->RemoveInputConnection(0, this->GetInputConnection(0, i));
    }
  }
}

//------------------------------------------------------------------------------
// make ProcessObject function visible
// should only be used when UserManagedInputs is true.
void vtkAppendSelection::SetNumberOfInputs(int num)
{
  if (!this->UserManagedInputs)
  {
    vtkErrorMacro(<< "SetNumberOfInputs is not supported if UserManagedInputs is false");
    return;
  }

  // Ask the superclass to set the number of connections.
  this->SetNumberOfInputConnections(0, num);
}

//------------------------------------------------------------------------------
// Set Nth input, should only be used when UserManagedInputs is true.
void vtkAppendSelection::SetInputConnectionByNumber(int num, vtkAlgorithmOutput* input)
{
  if (!this->UserManagedInputs)
  {
    vtkErrorMacro(<< "SetInputByNumber is not supported if UserManagedInputs is false");
    return;
  }

  // Ask the superclass to connect the input.
  this->SetNthInputConnection(0, num, input);
}

//------------------------------------------------------------------------------
void vtkAppendSelection::SetColorArray(vtkSelectionNode* node, double* color)
{
  if (!node || !color)
  {
    return;
  }

  const char* colorArrayName = vtkAppendSelection::GetColorArrayName();

  auto* selectionData = node->GetSelectionData();
  auto* arr = selectionData->GetArray(colorArrayName);
  if (arr)
  {
    return;
  }

  vtkNew<vtkUnsignedCharArray> colorArray;
  colorArray->SetName(colorArrayName);
  colorArray->SetNumberOfComponents(3);

  int length = selectionData->GetNumberOfTuples();
  colorArray->SetNumberOfTuples(length);
  for (int i = 0; i < length; i++)
  {
    colorArray->SetTuple3(i, color[0] * 255, color[1] * 255, color[2] * 255);
  }

  if (length > 0)
  {
    colorArray->CreateDefaultLookupTable();
    selectionData->AddArray(colorArray);

    selectionData->SetAttribute(colorArray, vtkDataSetAttributes::SCALARS);
    selectionData->SetActiveAttribute(colorArrayName, vtkDataSetAttributes::SCALARS);

    selectionData->SetScalars(colorArray);

    selectionData->CopyScalarsOn();
    selectionData->Modified();
    selectionData->Update();
  }
}

//------------------------------------------------------------------------------
int vtkAppendSelection::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the output
  vtkSelection* output = vtkSelection::GetData(outputVector->GetInformationObject(0));
  output->Initialize();

  // If there are no inputs, we are done.
  int numInputs = this->GetNumberOfInputConnections(0);
  if (numInputs == 0)
  {
    return 1;
  }

  int checkAbortInterval = std::min(numInputs / 10 + 1, 1000);
  if (!this->AppendByUnion)
  {
    // Expression is not set, so the vtkSelection automatically merges the nodes
    // using the `|` boolean operator.
    if (this->Expression.empty())
    {
      bool addSeperator = false;
      std::ostringstream stream;
      // iterate over all the selection inputs
      for (int inputId = 0; inputId < numInputs; ++inputId)
      {
        if (inputId % checkAbortInterval == 0 && this->CheckAbort())
        {
          break;
        }
        vtkSelection* sel = vtkSelection::GetData(inputVector[0]->GetInformationObject(inputId));
        if (sel == nullptr)
        {
          continue;
        }
        // set the selection name
        const std::string subSelectionName = "S" + std::to_string(inputId);
        for (unsigned int subNodeId = 0; subNodeId < sel->GetNumberOfNodes(); ++subNodeId)
        {
          vtkSelectionNode* inputNode = sel->GetNode(subNodeId);
          vtkNew<vtkSelectionNode> outputNode;
          outputNode->ShallowCopy(inputNode);
          bool canAddColorArray = this->Internals->Colors.size() == static_cast<size_t>(numInputs);
          if (canAddColorArray)
          {
            this->SetColorArray(outputNode, this->Internals->Colors[inputId].data());
          }
          const std::string subNodeName = sel->GetNodeNameAtIndex(subNodeId);
          const std::string combinedNodeName = subSelectionName + subNodeName;
          output->SetNode(combinedNodeName, outputNode);
          stream << (addSeperator ? "|" : "") << combinedNodeName;
          addSeperator = true;
        }
      } // for each input
      // set the combined expression
      if (output->GetNumberOfNodes() > 0)
      {
        const auto combinedExpression = this->Inverse ? "!(" + stream.str() + ")" : stream.str();
        output->SetExpression(combinedExpression);
      }
      return 1;
    }
    // Expression is set, so we need to define the combined selection expression using the selection
    // names and expressions of the individual sub-expressions
    else
    {
      // check if the number of input selection names != the number of input selections
      if (this->Internals->Names.size() != static_cast<size_t>(numInputs))
      {
        vtkErrorMacro("Number of input selection names does not match number of input selections.");
        return 0;
      }
      std::string combinedExpression = this->Expression;
      // iterate over all the selection inputs
      for (int inputId = 0; inputId < numInputs; ++inputId)
      {
        if (inputId % checkAbortInterval == 0 && this->CheckAbort())
        {
          break;
        }
        vtkSelection* sel = vtkSelection::GetData(inputVector[0]->GetInformationObject(inputId));
        if (sel == nullptr || sel->GetNumberOfNodes() == 0)
        {
          continue;
        }
        // get the selection name
        const std::string subSelectionName = this->Internals->Names[inputId];
        // get the sub-expression
        std::string subExpression = sel->GetExpression();
        // if the subExpression is empty, we need to use the boolean operator '|' to combine
        // the selection nodes.
        if (subExpression.empty())
        {
          bool addSeperator = false;
          std::ostringstream stream;
          for (unsigned int subNodeId = 0; subNodeId < sel->GetNumberOfNodes(); ++subNodeId)
          {
            // add node from sub-selection to combined-selection
            vtkSelectionNode* inputNode = sel->GetNode(subNodeId);
            vtkNew<vtkSelectionNode> outputNode;
            outputNode->ShallowCopy(inputNode);

            bool canAddColorArray =
              this->Internals->Colors.size() == static_cast<size_t>(numInputs);
            if (canAddColorArray)
            {
              this->SetColorArray(outputNode, this->Internals->Colors[inputId].data());
            }

            const std::string subNodeName = sel->GetNodeNameAtIndex(subNodeId);
            const std::string combinedNodeName = subSelectionName + subNodeName;
            output->SetNode(combinedNodeName, outputNode);
            // define the sub-expression
            stream << (addSeperator ? "|" : "") << combinedNodeName;
            addSeperator = true;
          }
          subExpression = "(" + stream.str() + ")";
        }
        // if the subExpression is not empty, we need to replace the node names in the
        // subExpression with the node names that will be added to the combined expression
        else
        {
          for (unsigned int subNodeId = 0; subNodeId < sel->GetNumberOfNodes(); ++subNodeId)
          {
            // add node from sub-selection to combined-selection
            vtkSelectionNode* inputNode = sel->GetNode(subNodeId);
            vtkNew<vtkSelectionNode> outputNode;
            outputNode->ShallowCopy(inputNode);

            bool canAddColorArray =
              this->Internals->Colors.size() == static_cast<size_t>(numInputs);
            if (canAddColorArray)
            {
              this->SetColorArray(outputNode, this->Internals->Colors[inputId].data());
            }

            const std::string subNodeName = sel->GetNodeNameAtIndex(subNodeId);
            const std::string combinedNodeName = subSelectionName + subNodeName;
            output->SetNode(combinedNodeName, outputNode);
            // replace the node name of the sub-expression with the node name of the
            // combined-expression
            ReplaceStringUsingRegex(subExpression, this->Internals->RegExNodeIdInExpression,
              subNodeName, combinedNodeName);
          }
          subExpression = "(" + subExpression + ")";
        }
        // replace the selection name in the combined-expression with the sub-expression which
        // includes the (new) node names of the sub-selection
        ReplaceStringUsingRegex(combinedExpression, this->Internals->RegExNodeIdInExpression,
          subSelectionName, subExpression);
      } // for each input

      // set the combined expression
      if (output->GetNumberOfNodes() > 0)
      {
        combinedExpression = this->Inverse ? "!(" + combinedExpression + ")" : combinedExpression;
        output->SetExpression(combinedExpression);
      }
      return 1;
    }
  }
  else
  {
    // The first non-null selection determines the required content type of all selections.
    int idx = 0;
    vtkSelection* first = nullptr;
    while (first == nullptr && idx < numInputs)
    {
      if (idx % checkAbortInterval == 0 && this->CheckAbort())
      {
        break;
      }
      first = vtkSelection::GetData(inputVector[0]->GetInformationObject(idx));
      idx++;
    }

    // If they are all null, return.
    if (first == nullptr)
    {
      return 1;
    }

    output->ShallowCopy(first);

    // Take the union of all non-null selections
    for (; idx < numInputs; ++idx)
    {
      if (idx % checkAbortInterval == 0 && this->CheckAbort())
      {
        break;
      }
      vtkSelection* s = vtkSelection::GetData(inputVector[0]->GetInformationObject(idx));
      if (s != nullptr)
      {
        output->Union(s);
      } // for a non nullptr input
    }   // for each input

    return 1;
  }
}

//------------------------------------------------------------------------------
vtkSelection* vtkAppendSelection::GetInput(int idx)
{
  return vtkSelection::SafeDownCast(this->GetExecutive()->GetInputData(0, idx));
}

//------------------------------------------------------------------------------
int vtkAppendSelection::FillInputPortInformation(int port, vtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}

//------------------------------------------------------------------------------
void vtkAppendSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "UserManagedInputs: " << (this->UserManagedInputs ? "On" : "Off") << endl;
  os << "AppendByUnion: " << (this->AppendByUnion ? "On" : "Off") << endl;
  os << "Expression: " << this->Expression << endl;
  os << "Inverse: " << (this->Inverse ? "On" : "Off") << endl;
  for (size_t i = 0; i < this->Internals->Names.size(); ++i)
  {
    os << "InputName " << i << ": " << this->Internals->Names[i] << endl;
  }

  for (size_t i = 0; i < this->Internals->Colors.size(); ++i)
  {
    os << "InputColor " << i << ": {" << this->Internals->Colors[i][0] << ","
       << this->Internals->Colors[i][1] << "," << this->Internals->Colors[i][2] << "}" << endl;
  }
}
VTK_ABI_NAMESPACE_END
