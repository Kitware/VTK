/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyDataAlgorithm.h"

#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkTrivialProducer.h"

vtkCxxRevisionMacro(vtkPolyDataAlgorithm, "1.1.2.1");
vtkStandardNewMacro(vtkPolyDataAlgorithm);

//----------------------------------------------------------------------------
vtkPolyDataAlgorithm::vtkPolyDataAlgorithm()
{
}

//----------------------------------------------------------------------------
vtkPolyDataAlgorithm::~vtkPolyDataAlgorithm()
{
}

//----------------------------------------------------------------------------
void vtkPolyDataAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkPolyData* vtkPolyDataAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkPolyData* vtkPolyDataAlgorithm::GetOutput(int port)
{
  return vtkPolyData::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
void vtkPolyDataAlgorithm::SetInput(vtkPolyData* input)
{
  this->SetInput(0, input);
}

//----------------------------------------------------------------------------
void vtkPolyDataAlgorithm::SetInput(int index, vtkPolyData* input)
{
  if(input)
    {
    if(vtkAlgorithmOutput* producerPort = input->GetProducerPort())
      {
      this->SetInputConnection(index, producerPort);
      }
    else
      {
      // The data object has no producer.  Give it a trivial producer.
      vtkTrivialProducer* producer = vtkTrivialProducer::New();
      producer->SetOutput(input);
      this->SetInputConnection(index, producer->GetOutputPort(0));
      producer->Delete();
      }
    }
  else
    {
    // Setting a NULL input removes the connection.
    this->SetInputConnection(index, 0);
    }
}

//----------------------------------------------------------------------------
void vtkPolyDataAlgorithm::AddInput(vtkPolyData* input)
{
  this->AddInput(0, input);
}

//----------------------------------------------------------------------------
void vtkPolyDataAlgorithm::AddInput(int index, vtkPolyData* input)
{
  if(input)
    {
    if(vtkAlgorithmOutput* producerPort = input->GetProducerPort())
      {
      this->AddInputConnection(index, producerPort);
      }
    else
      {
      // The data object has no producer.  Give it a trivial producer.
      vtkTrivialProducer* producer = vtkTrivialProducer::New();
      producer->SetOutput(input);
      this->AddInputConnection(index, producer->GetOutputPort(0));
      producer->Delete();
      }
    }
}
