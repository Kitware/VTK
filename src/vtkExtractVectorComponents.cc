/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractVectorComponents.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkExtractVectorComponents.hh"

vtkExtractVectorComponents::vtkExtractVectorComponents()
{
  this->VyComponent = NULL;
  this->VzComponent = NULL;
}

// Description:
// Get the output dataset containing the indicated component. The component is 
// specified by an index between (0,2) corresponding to the x, y, or z vector
// component. By default, the x component is extracted.
vtkDataSet *vtkExtractVectorComponents::GetOutput(int i)
{
  if ( i < 0 || i > 2 )
    {
    vtkErrorMacro(<<"Vector component must be between (0,2)");
    if ( i < 0 ) return this->Output;
    if ( i > 2 ) return this->VzComponent;
    }

  if ( this->Output == NULL )
    {
    vtkErrorMacro(<<"Abstract filters require input to be set before output can be retrieved");
    }

  if ( i == 0 ) return this->Output;
  else if ( i == 1 ) return this->VyComponent;
  else return this->VzComponent;
}

// Description:
// Get the output dataset representing velocity x-component. If output is NULL
// then input hasn't been set, which is necessary for abstract objects. (Note:
// this method returns the same information as the GetOutput() method with an
// index of 0.)
vtkDataSet *vtkExtractVectorComponents::GetVxComponent()
{
  if ( this->Output == NULL )
    {
    vtkErrorMacro(<<"Abstract filters require input to be set before VxComponent can be retrieved");
    }
  return this->Output;
}

// Description:
// Get the output dataset representing velocity y-component. If output is NULL
// then input hasn't been set, which is necessary for abstract objects. (Note:
// this method returns the same information as the GetOutput() method with an
// index of 1.)
vtkDataSet *vtkExtractVectorComponents::GetVyComponent()
{
  if ( this->VyComponent == NULL )
    {
    vtkErrorMacro(<<"Abstract filters require input to be set before VyComponent can be retrieved");
    }
  return this->VyComponent;
}

// Description:
// Get the output dataset representing velocity z-component. If output is NULL
// then input hasn't been set, which is necessary for abstract objects. (Note:
// this method returns the same information as the GetOutput() method with an
// index of 2.)
vtkDataSet *vtkExtractVectorComponents::GetVzComponent()
{
  if ( this->VzComponent == NULL )
    {
    vtkErrorMacro(<<"Abstract filters require input to be set before VzComponent can be retrieved");
    }
  return this->VzComponent;
}

// Description:
// Specify the input data or filter.
void vtkExtractVectorComponents::SetInput(vtkDataSet *input)
{
  if ( this->Input != input )
    {
    vtkDebugMacro(<<" setting Input to " << (void *)input);
    this->Input = input;
    this->Modified();

    if ( this->Input == NULL ) return;

    if ( ! this->Output )
      {
      this->Output = this->Input->MakeObject();
      this->Output->SetSource(this);
      this->VyComponent = this->Input->MakeObject();
      this->VyComponent->SetSource(this);
      this->VzComponent = this->Input->MakeObject();
      this->VzComponent->SetSource(this);
      return;
      }

    // since the input has changed we might need to create a new output
    if (!strcmp(this->Output->GetClassName(),this->Input->GetClassName()))
      {
      this->Output->Delete();
      this->VyComponent->Delete();
      this->VzComponent->Delete();

      this->Output = this->Input->MakeObject();
      this->Output->SetSource(this);
      this->VyComponent = this->Input->MakeObject();
      this->VyComponent->SetSource(this);
      this->VzComponent = this->Input->MakeObject();
      this->VzComponent->SetSource(this);

      vtkWarningMacro(<<" a new output had to be created since the input type changed.");
      }
    }
}

// Description:
// Update input to this filter and the filter itself. Note that we are 
// overloading this method because the output is an abstract dataset type.
// This requires special treatment.
void vtkExtractVectorComponents::Update()
{
  // make sure output has been created
  if ( !this->Output )
    {
    vtkErrorMacro(<< "No output has been created...need to set input");
    return;
    }

  // make sure input is available
  if ( !this->Input )
    {
    vtkErrorMacro(<< "No input...can't execute!");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  this->Input->Update();
  this->Updating = 0;

  if ( this->Input->GetMTime() > this->ExecuteTime ||
  this->GetMTime() > this->ExecuteTime )
    {
    if ( this->Input->GetDataReleased() )
      {
      this->Input->ForceUpdate();
      }

    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    // clear just point data output because structure is copied from input
    this->Output->CopyStructure(this->Input);
    this->VyComponent->CopyStructure(this->Input);
    this->VzComponent->CopyStructure(this->Input);
    this->Execute();
    this->ExecuteTime.Modified();
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }

  if ( this->Input->ShouldIReleaseData() ) this->Input->ReleaseData();
}

void vtkExtractVectorComponents::Execute()
{
  int i, numVectors = 0;
  float *v;
  vtkVectors *vectors;
  vtkFloatScalars *vx, *vy, *vz;
  vtkPointData *pd, *outVx, *outVy, *outVz;

  vtkDebugMacro(<<"Extracting vector components...");

  pd = this->Input->GetPointData();
  outVx = this->Output->GetPointData();  
  outVy = this->VyComponent->GetPointData();  
  outVz = this->VzComponent->GetPointData();  

  if ( (vectors = pd->GetVectors()) == NULL ||
  (numVectors = vectors->GetNumberOfVectors()) < 1 )  
    {
    vtkErrorMacro(<<"No vector data to extract!");
    return;
    }

  vx = new vtkFloatScalars(numVectors);  
  vy = new vtkFloatScalars(numVectors);  
  vz = new vtkFloatScalars(numVectors);  

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
