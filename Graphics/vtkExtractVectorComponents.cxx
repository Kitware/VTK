/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractVectorComponents.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkExtractVectorComponents.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkExtractVectorComponents* vtkExtractVectorComponents::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkExtractVectorComponents");
  if(ret)
    {
    return (vtkExtractVectorComponents*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkExtractVectorComponents;
}




vtkExtractVectorComponents::vtkExtractVectorComponents()
{
}

vtkExtractVectorComponents::~vtkExtractVectorComponents()
{
}

// Get the output dataset containing the indicated component. The component is 
// specified by an index between (0,2) corresponding to the x, y, or z vector
// component. By default, the x component is extracted.
vtkDataSet *vtkExtractVectorComponents::GetOutput(int i)
{
  if ( this->NumberOfOutputs < 3 )
    {
    vtkErrorMacro(<<"Abstract filters require input to be set before output can be retrieved");
    return NULL;
    }
  
  if ( i < 0 || i > 2 )
    {
    vtkErrorMacro(<<"Vector component must be between (0,2)");
    if ( i < 0 )
      {
      return (vtkDataSet *)this->Outputs[0];
      }
    if ( i > 2 )
      {
      return (vtkDataSet *)this->Outputs[2];
      }
    }

  return (vtkDataSet *)this->Outputs[i];
}

// Get the output dataset representing velocity x-component. If output is NULL
// then input hasn't been set, which is necessary for abstract objects. (Note:
// this method returns the same information as the GetOutput() method with an
// index of 0.)
vtkDataSet *vtkExtractVectorComponents::GetVxComponent()
{
  if ( this->NumberOfOutputs < 1)
    {
    vtkErrorMacro(<<"Abstract filters require input to be set before VxComponent can be retrieved");
    }
  return (vtkDataSet *)this->Outputs[0];
}

// Get the output dataset representing velocity y-component. If output is NULL
// then input hasn't been set, which is necessary for abstract objects. (Note:
// this method returns the same information as the GetOutput() method with an
// index of 1.)
vtkDataSet *vtkExtractVectorComponents::GetVyComponent()
{
  if ( this->NumberOfOutputs < 2)
    {
    vtkErrorMacro(<<"Abstract filters require input to be set before VyComponent can be retrieved");
    }
  return (vtkDataSet *)this->Outputs[1];
}

// Get the output dataset representing velocity z-component. If output is NULL
// then input hasn't been set, which is necessary for abstract objects. (Note:
// this method returns the same information as the GetOutput() method with an
// index of 2.)
vtkDataSet *vtkExtractVectorComponents::GetVzComponent()
{
  if ( this->NumberOfOutputs < 3)
    {
    vtkErrorMacro(<<"Abstract filters require input to be set before VzComponent can be retrieved");
    }
  return (vtkDataSet *)this->Outputs[2];
}

// Specify the input data or filter.
void vtkExtractVectorComponents::SetInput(vtkDataSet *input)
{
  if (this->NumberOfInputs > 0 && this->Inputs[0] == input )
    {
    return;
    }

  this->vtkProcessObject::SetNthInput(0, input);

  if ( input == NULL )
    {
    return;
    }

  if (this->NumberOfOutputs < 3)
    {
    this->SetNthOutput(0,input->MakeObject());
    this->Outputs[0]->Delete();
    this->SetNthOutput(1,input->MakeObject());
    this->Outputs[1]->Delete();
    this->SetNthOutput(2,input->MakeObject());
    this->Outputs[2]->Delete();
    return;
    }

  // since the input has changed we might need to create a new output
  if (strcmp(this->Outputs[0]->GetClassName(),input->GetClassName()))
    {
    this->SetNthOutput(0,input->MakeObject());
    this->Outputs[0]->Delete();
    this->SetNthOutput(1,input->MakeObject());
    this->Outputs[1]->Delete();
    this->SetNthOutput(2,input->MakeObject());
    this->Outputs[2]->Delete();
    vtkWarningMacro(<<" a new output had to be created since the input type changed.");
    }
}

void vtkExtractVectorComponents::Execute()
{
  int i, numVectors = 0;
  float *v;
  vtkVectors *vectors;
  vtkScalars *vx, *vy, *vz;
  vtkPointData *pd, *outVx, *outVy, *outVz;

  vtkDebugMacro(<<"Extracting vector components...");

  // taken out of previous update method.
  this->GetOutput()->CopyStructure(this->GetInput());
  this->GetVyComponent()->CopyStructure(this->GetInput());
  this->GetVzComponent()->CopyStructure(this->GetInput());
  
  pd = this->GetInput()->GetPointData();
  outVx = this->GetOutput()->GetPointData();  
  outVy = this->GetVyComponent()->GetPointData();  
  outVz = this->GetVzComponent()->GetPointData();  

  if ( (vectors = pd->GetVectors()) == NULL ||
  (numVectors = vectors->GetNumberOfVectors()) < 1 )  
    {
    vtkErrorMacro(<<"No vector data to extract!");
    return;
    }

  vx = vtkScalars::New(); vx->SetNumberOfScalars(numVectors);
  vy = vtkScalars::New(); vy->SetNumberOfScalars(numVectors);
  vz = vtkScalars::New(); vz->SetNumberOfScalars(numVectors);

  for (i=0; i<numVectors; i++)
    {
    v = vectors->GetVector(i);
    vx->SetScalar(i,v[0]);
    vy->SetScalar(i,v[1]);
    vz->SetScalar(i,v[2]);
    }

  outVx->CopyScalarsOff();
  outVx->PassData(pd);
  outVx->SetScalars(vx);
  vx->Delete();

  outVy->CopyScalarsOff();
  outVy->PassData(pd);
  outVy->SetScalars(vy);
  vy->Delete();

  outVz->CopyScalarsOff();
  outVz->PassData(pd);
  outVz->SetScalars(vz);
  vz->Delete();
}


//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkDataSet *vtkExtractVectorComponents::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkDataSet *)(this->Inputs[0]);
}


