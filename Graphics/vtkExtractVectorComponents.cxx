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
  this->ExtractToFieldData = 0;
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
    return 0;
    }
  return static_cast<vtkDataSet *>(this->Outputs[0]);
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
    return 0;
    }
  return static_cast<vtkDataSet *>(this->Outputs[1]);
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
    return 0;
    }
  return static_cast<vtkDataSet *>(this->Outputs[2]);
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

template <class T>
static void ExtractComponents(int numVectors, T* vectors, T* vx, T* vy, T* vz)
{
  for (int i=0; i<numVectors; i++)
    {
    vx[i] = vectors[3*i];
    vy[i] = vectors[3*i+1];
    vz[i] = vectors[3*i+2];
    }
}

void vtkExtractVectorComponents::Execute()
{
  int numVectors = 0, numVectorsc = 0;
  vtkDataArray *vectors, *vectorsc;
  vtkDataArray *vx, *vy, *vz;
  vtkDataArray *vxc, *vyc, *vzc;
  vtkPointData *pd, *outVx, *outVy=0, *outVz=0;
  vtkCellData *cd, *outVxc, *outVyc=0, *outVzc=0;

  vtkDebugMacro(<<"Extracting vector components...");

  // taken out of previous update method.
  this->GetOutput()->CopyStructure(this->GetInput());
  if (!this->ExtractToFieldData)
    {
    this->GetVyComponent()->CopyStructure(this->GetInput());
    this->GetVzComponent()->CopyStructure(this->GetInput());
    }
  
  pd = this->GetInput()->GetPointData();
  cd = this->GetInput()->GetCellData();
  outVx = this->GetOutput()->GetPointData();  
  outVxc = this->GetOutput()->GetCellData();  
  if (!this->ExtractToFieldData)
    {
    outVy = this->GetVyComponent()->GetPointData();  
    outVz = this->GetVzComponent()->GetPointData();  
    outVyc = this->GetVyComponent()->GetCellData();  
    outVzc = this->GetVzComponent()->GetCellData();  
    }

  vectors = pd->GetActiveVectors();
  vectorsc = cd->GetActiveVectors();
  if ( (vectors == NULL ||
	((numVectors = vectors->GetNumberOfTuples()) < 1) ) && 
       (vectorsc == NULL ||
	((numVectorsc = vectorsc->GetNumberOfTuples()) < 1)))  
    {
    vtkErrorMacro(<<"No vector data to extract!");
    return;
    }

  const char* name = vectors->GetName();
  char* newName = new char[strlen(name)+10];

  if (vectors)
    {
    vx = vtkDataArray::CreateDataArray(vectors->GetDataType());
    vx->SetNumberOfTuples(numVectors);
    sprintf(newName, "%s-x", name);
    vx->SetName(newName);
    vy = vtkDataArray::CreateDataArray(vectors->GetDataType());
    vy->SetNumberOfTuples(numVectors);
    sprintf(newName, "%s-y", name);
    vy->SetName(newName);
    vz = vtkDataArray::CreateDataArray(vectors->GetDataType());
    vz->SetNumberOfTuples(numVectors);
    sprintf(newName, "%s-z", name);
    vz->SetName(newName);

    switch (vectors->GetDataType())
      {
      vtkTemplateMacro5(ExtractComponents, numVectors,
			(VTK_TT *)vectors->GetVoidPointer(0),
			(VTK_TT *)vx->GetVoidPointer(0),
			(VTK_TT *)vy->GetVoidPointer(0),
			(VTK_TT *)vz->GetVoidPointer(0));
      }

    outVx->CopyScalarsOff();
    outVx->PassData(pd);
    outVx->SetScalars(vx);
    vx->Delete();
    
    if (this->ExtractToFieldData)
      {
      outVx->AddArray(vy);
      outVx->AddArray(vz);
      }
    else
      {
      outVy->CopyScalarsOff();
      outVy->PassData(pd);
      outVy->SetScalars(vy);
      
      outVz->CopyScalarsOff();
      outVz->PassData(pd);
      outVz->SetScalars(vz);
      }
    vy->Delete();
    vz->Delete();
    }

  if (vectorsc)
    {
    vxc = vtkDataArray::CreateDataArray(vectorsc->GetDataType());
    vxc->SetNumberOfComponents(3);
    vxc->SetNumberOfTuples(numVectorsc);
    sprintf(newName, "%s-x", name);
    vxc->SetName(newName);
    vyc = vtkDataArray::CreateDataArray(vectorsc->GetDataType());
    vyc->SetNumberOfComponents(3);
    vyc->SetNumberOfTuples(numVectorsc);
    sprintf(newName, "%s-y", name);
    vyc->SetName(newName);
    vzc = vtkDataArray::CreateDataArray(vectorsc->GetDataType());
    vzc->SetNumberOfComponents(3);
    vzc->SetNumberOfTuples(numVectors);
    sprintf(newName, "%s-z", name);
    vzc->SetName(newName);

    switch (vectorsc->GetDataType())
      {
      vtkTemplateMacro5(ExtractComponents, numVectorsc,
			(VTK_TT *)vectorsc->GetVoidPointer(0),
			(VTK_TT *)vxc->GetVoidPointer(0),
			(VTK_TT *)vyc->GetVoidPointer(0),
			(VTK_TT *)vzc->GetVoidPointer(0));
      }

    outVxc->CopyScalarsOff();
    outVxc->PassData(cd);
    outVxc->SetScalars(vxc);
    vxc->Delete();
    
    if (this->ExtractToFieldData)
      {
      outVxc->AddArray(vyc);
      outVxc->AddArray(vzc);
      }
    else
      {
      outVyc->CopyScalarsOff();
      outVyc->PassData(cd);
      outVyc->SetScalars(vyc);
      
      outVzc->CopyScalarsOff();
      outVzc->PassData(cd);
      outVzc->SetScalars(vzc);
      }
    vyc->Delete();
    vzc->Delete();
    }
  delete[] newName;

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

void vtkExtractVectorComponents::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkSource::PrintSelf(os,indent);
  
  os << indent << "ExtractToFieldData: " << this->ExtractToFieldData << endl;
}
