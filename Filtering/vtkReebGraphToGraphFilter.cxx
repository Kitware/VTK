/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include  "vtkReebGraphToGraphFilter.h"

#include  "vtkInformation.h"
#include  "vtkInformationVector.h"

vtkCxxRevisionMacro(vtkReebGraphToGraphFilter, "$Revision$");
vtkStandardNewMacro(vtkReebGraphToGraphFilter);

//----------------------------------------------------------------------------
vtkReebGraphToGraphFilter::vtkReebGraphToGraphFilter()
{
  this->SetNumberOfInputPorts(1);
}

//----------------------------------------------------------------------------
vtkReebGraphToGraphFilter::~vtkReebGraphToGraphFilter()
{
}

//----------------------------------------------------------------------------
int vtkReebGraphToGraphFilter::FillInputPortInformation(
  int portNumber, vtkInformation *info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkReebGraph");
  return 1;
}

//----------------------------------------------------------------------------
int vtkReebGraphToGraphFilter::FillOutputPortInformation(
  int, vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMutableDirectedGraph");
  return 1;
}

//----------------------------------------------------------------------------
void vtkReebGraphToGraphFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
vtkMutableDirectedGraph* vtkReebGraphToGraphFilter::GetOutput()
{
  return vtkMutableDirectedGraph::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
int vtkReebGraphToGraphFilter::RequestDataObject(vtkInformation *request,
                                            vtkInformationVector **inputVector,
                                            vtkInformationVector *outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }
  vtkReebGraph *input = vtkReebGraph::SafeDownCast(
    inInfo->Get(vtkReebGraph::DATA_OBJECT()));

  if (input)
    {
    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    vtkMutableDirectedGraph *output = (vtkMutableDirectedGraph *)
      outInfo->Get(vtkMutableDirectedGraph::DATA_OBJECT());

    if(!output){
      output = input->GetVtkGraph();
      /*vtkMutableDirectedGraph *rG = input->GetVtkGraph();
      output = vtkMutableDirectedGraph::New();
      output->CopyStructure(rG);
      output->GetVertexData()->PassData(rG->GetVertexData());
      output->GetEdgeData()->PassData(rG->GetEdgeData());*/
      output->SetPipelineInformation(outInfo);

      cout << "vtkGraph (inside filter):  vD: " << output->GetVertexData()
        << ", eD: " << output->GetEdgeData()
        << ", vAnB: " << output->GetVertexData()->GetNumberOfArrays()
        << endl;
    }
    return 1;
    }
  return 0;
}
