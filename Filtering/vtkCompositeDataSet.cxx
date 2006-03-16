/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeDataSet.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkInformationDataObjectKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkTrivialProducer.h"

vtkCxxRevisionMacro(vtkCompositeDataSet, "1.8");

vtkInformationKeyMacro(vtkCompositeDataSet,INDEX,Integer);
vtkInformationKeyMacro(vtkCompositeDataSet,COMPOSITE_DATA_SET,DataObject);

//----------------------------------------------------------------------------
vtkCompositeDataSet::vtkCompositeDataSet()
{
}

//----------------------------------------------------------------------------
vtkCompositeDataSet::~vtkCompositeDataSet()
{
}

//----------------------------------------------------------------------------
void vtkCompositeDataSet::Initialize()
{
  this->Superclass::Initialize();
}

//----------------------------------------------------------------------------
void vtkCompositeDataSet::SetPipelineInformation(vtkInformation* newInfo)
{
  vtkInformation* oldInfo = this->PipelineInformation;
  if(newInfo != oldInfo)
    {
    if(newInfo)
      {
      // Reference the new information.
      newInfo->Register(this);

      // Detach the output that used to be held by the new information.
      if(vtkDataObject* oldData = 
         newInfo->Get(COMPOSITE_DATA_SET()))
        {
        oldData->Register(this);
        oldData->SetPipelineInformation(0);
        oldData->UnRegister(this);
        }

      // Tell the new information about this object.
      newInfo->Set(COMPOSITE_DATA_SET(), this);
      }

    // Save the pointer to the new information.
    this->PipelineInformation = newInfo;

    if(oldInfo)
      {
      // Remove the old information's reference to us.
      oldInfo->Set(COMPOSITE_DATA_SET(), 0);

      // Remove our reference to the old information.
      oldInfo->UnRegister(this);
      }
    }
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkCompositeDataSet::GetProducerPort()
{  
  // Make sure there is an executive.
  if(!this->GetExecutive())
    {
    vtkTrivialProducer* tp = vtkTrivialProducer::New();
    vtkCompositeDataPipeline* exec = vtkCompositeDataPipeline::New();
    tp->SetExecutive(exec);
    vtkInformation* portInfo = 
      tp->GetOutputPortInformation(0);
    portInfo->Set(vtkCompositeDataPipeline::COMPOSITE_DATA_TYPE_NAME(), 
                  this->GetClassName());
    exec->Delete();
    tp->SetOutput(this);
    tp->Delete();
    }

  // Get the port from the executive.
  return this->GetExecutive()->GetProducerPort(this);
}

//----------------------------------------------------------------------------
vtkCompositeDataSet* vtkCompositeDataSet::GetData(vtkInformation* info)
{
  return info? vtkCompositeDataSet::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkCompositeDataSet* vtkCompositeDataSet::GetData(vtkInformationVector* v,
                                                  int i)
{
  return vtkCompositeDataSet::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
void vtkCompositeDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  // this->UpdateExtent
  this->Superclass::PrintSelf(os,indent);
}

