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
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkAppendSelection);

//----------------------------------------------------------------------------
vtkAppendSelection::vtkAppendSelection()
{
  this->UserManagedInputs = 0;
  this->AppendByUnion = 1;
}

//----------------------------------------------------------------------------
vtkAppendSelection::~vtkAppendSelection()
{
}

//----------------------------------------------------------------------------
// Add a dataset to the list of data to append.
void vtkAppendSelection::AddInput(vtkSelection *ds)
{
  if (this->UserManagedInputs)
    {
    vtkErrorMacro(<<
      "AddInput is not supported if UserManagedInputs is true");
    return;
    }
  this->Superclass::AddInputConnection(ds->GetProducerPort());
}

//----------------------------------------------------------------------------
// Remove a dataset from the list of data to append.
void vtkAppendSelection::RemoveInput(vtkSelection *ds)
{
  if (this->UserManagedInputs)
    {
    vtkErrorMacro(<<
      "RemoveInput is not supported if UserManagedInputs is true");
    return;
    }

  vtkAlgorithmOutput *algOutput = 0;
  if (ds)
    {
    algOutput = ds->GetProducerPort();
    }

  this->RemoveInputConnection(0, algOutput);
}

//----------------------------------------------------------------------------
// make ProcessObject function visible
// should only be used when UserManagedInputs is true.
void vtkAppendSelection::SetNumberOfInputs(int num)
{
  if (!this->UserManagedInputs)
    {
    vtkErrorMacro(<<
      "SetNumberOfInputs is not supported if UserManagedInputs is false");
    return;
    }

  // Ask the superclass to set the number of connections.
  this->SetNumberOfInputConnections(0, num);
}

//----------------------------------------------------------------------------
// Set Nth input, should only be used when UserManagedInputs is true.
void vtkAppendSelection::SetInputByNumber(int num, vtkSelection *input)
{
  if (!this->UserManagedInputs)
    {
    vtkErrorMacro(<<
      "SetInputByNumber is not supported if UserManagedInputs is false");
    return;
    }

  // Ask the superclass to connect the input.
  this->SetNthInputConnection(0, num, input? input->GetProducerPort() : 0);
}

//----------------------------------------------------------------------------
int vtkAppendSelection::RequestData(vtkInformation *vtkNotUsed(request),
                                   vtkInformationVector **inputVector,
                                   vtkInformationVector *outputVector)
{
  // Get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // Get the output
  vtkSelection *output = vtkSelection::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  output->Initialize();
  
  // If there are no inputs, we are done.
  int numInputs = this->GetNumberOfInputConnections(0);
  if (numInputs == 0)
    {
    return 1;
    }

  if (!this->AppendByUnion)
    {
    for (int idx=0; idx < numInputs; ++idx)
      {
      vtkInformation *inInfo = inputVector[0]->GetInformationObject(idx);
      vtkSelection *sel = vtkSelection::GetData(inInfo);
      if (sel != NULL)
        {
        for (unsigned int j = 0; j < sel->GetNumberOfNodes(); ++j)
          {
          vtkSelectionNode* inputNode = sel->GetNode(j);
          vtkSmartPointer<vtkSelectionNode> outputNode =
            vtkSmartPointer<vtkSelectionNode>::New();
          outputNode->ShallowCopy(inputNode);
          output->AddNode(outputNode);
          }
        } 
      } // for each input
    return 1;
    }
  
  // The first non-null selection determines the required content type of all selections.
  int idx = 0;
  vtkSelection *first = NULL;
  while (first == NULL && idx < numInputs)
    {
    vtkInformation *inInfo = inputVector[0]->GetInformationObject(idx);
    first = vtkSelection::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    idx++;
    }
  
  // If they are all null, return.
  if (first == NULL)
    {
    return 1;
    }
  
  output->ShallowCopy(first);
  
  // Take the union of all non-null selections
  for (; idx < numInputs; ++idx)
    {
    vtkInformation *inInfo = inputVector[0]->GetInformationObject(idx);
    vtkSelection *s = vtkSelection::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    if (s != NULL)
      {
      output->Union(s);
      } // for a non NULL input
    } // for each input

  return 1;
}

//----------------------------------------------------------------------------
vtkSelection *vtkAppendSelection::GetInput(int idx)
{
  return vtkSelection::SafeDownCast(
    this->GetExecutive()->GetInputData(0, idx));
}

//----------------------------------------------------------------------------
int vtkAppendSelection::FillInputPortInformation(int port, vtkInformation *info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkAppendSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << "UserManagedInputs: " << (this->UserManagedInputs?"On":"Off") << endl;
  os << "AppendByUnion: " << (this->AppendByUnion? "On": "Off") << endl;
}
