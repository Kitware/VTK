/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendSelection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "vtkAppendSelection.h"

#include "vtkAlgorithmOutput.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <algorithm>
#include <regex>
#include <sstream>
#include <vector>

namespace
{
//------------------------------------------------------------------------------
void ReplaceStringUsingRegex(
  std::string& source, const std::regex& regex, const std::string& replace, const std::string& with)
{
  for (auto match = std::sregex_iterator(source.begin(), source.end(), regex);
       match != std::sregex_iterator(); ++match)
  {
    if (match->str() == replace)
    {
      source.replace(match->position(), match->length(), with);
    }
  }
}
}

//------------------------------------------------------------------------------
class vtkAppendSelection::vtkInternals
{
public:
  std::vector<std::string> Names;
  const std::regex RegExNodeId;
  const std::regex RegExNodeIdInExpression;

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
  else if (!std::regex_match(safeInputSelectionName, this->Internals->RegExNodeId))
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
}
