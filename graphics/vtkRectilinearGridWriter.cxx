/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGridWriter.cxx
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
#include "vtkRectilinearGridWriter.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkRectilinearGridWriter* vtkRectilinearGridWriter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkRectilinearGridWriter");
  if(ret)
    {
    return (vtkRectilinearGridWriter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkRectilinearGridWriter;
}




//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkRectilinearGridWriter::SetInput(vtkRectilinearGrid *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkRectilinearGrid *vtkRectilinearGridWriter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkRectilinearGrid *)(this->Inputs[0]);
}


void vtkRectilinearGridWriter::WriteData()
{
  ostream *fp;
  vtkRectilinearGrid *input = this->GetInput();
  int dim[3];

  vtkDebugMacro(<<"Writing vtk rectilinear grid...");

  if ( !(fp=this->OpenVTKFile()) || !this->WriteHeader(fp) )
    {
    return;
    }
  //
  // Write rectilinear grid specific stuff
  //
  *fp << "DATASET RECTILINEAR_GRID\n"; 

  // Write data owned by the dataset
  this->WriteDataSetData(fp, input);

  input->GetDimensions(dim);
  *fp << "DIMENSIONS " << dim[0] << " " << dim[1] << " " << dim[2] << "\n";

  this->WriteCoordinates(fp, input->GetXCoordinates(), 0);
  this->WriteCoordinates(fp, input->GetYCoordinates(), 1);
  this->WriteCoordinates(fp, input->GetZCoordinates(), 2);

  this->WriteCellData(fp, input);
  this->WritePointData(fp, input);

  this->CloseVTKFile(fp);
}

void vtkRectilinearGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataWriter::PrintSelf(os,indent);
}
