/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractVectorComponents.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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

  this->vtkProcessObject::SetInput(0, input);

  if ( input == NULL )
    {
    return;
    }

  if (this->NumberOfOutputs < 3)
    {
    this->SetOutput(0,input->MakeObject());
    this->Outputs[0]->Delete();
    this->SetOutput(1,input->MakeObject());
    this->Outputs[1]->Delete();
    this->SetOutput(2,input->MakeObject());
    this->Outputs[2]->Delete();
    return;
    }

  // since the input has changed we might need to create a new output
  if (strcmp(this->Outputs[0]->GetClassName(),input->GetClassName()))
    {
    this->SetOutput(0,input->MakeObject());
    this->Outputs[0]->Delete();
    this->SetOutput(1,input->MakeObject());
    this->Outputs[1]->Delete();
    this->SetOutput(2,input->MakeObject());
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



//----------------------------------------------------------------------------
// copy Update extent from output passed in to input.
int 
vtkExtractVectorComponents::ComputeInputUpdateExtents(vtkDataObject *output)
{
  this->GetInput()->CopyUpdateExtent(output);
  return 1;  
}


