/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetWriter.cxx
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
#include "vtkDataSetWriter.h"
#include "vtkPolyDataWriter.h"
#include "vtkStructuredPointsWriter.h"
#include "vtkStructuredGridWriter.h"
#include "vtkUnstructuredGridWriter.h"
#include "vtkRectilinearGridWriter.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkDataSetWriter* vtkDataSetWriter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkDataSetWriter");
  if(ret)
    {
    return (vtkDataSetWriter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkDataSetWriter;
}




//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkDataSetWriter::SetInput(vtkDataSet *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkDataSet *vtkDataSetWriter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkDataSet *)(this->Inputs[0]);
}

void vtkDataSetWriter::WriteData()
{
  int type;
  vtkDataWriter *writer = NULL;
  vtkDataSet *input = this->GetInput();
  
  vtkDebugMacro(<<"Writing vtk dataset...");

  type = input->GetDataObjectType();
  if ( type == VTK_POLY_DATA )
    {
    vtkPolyDataWriter *pwriter = vtkPolyDataWriter::New();
    pwriter->SetInput((vtkPolyData *)input);
    writer = (vtkDataWriter *)pwriter;
    }

  else if ( type == VTK_STRUCTURED_POINTS || type == VTK_IMAGE_DATA)
    {
    vtkStructuredPointsWriter *spwriter = vtkStructuredPointsWriter::New();
    spwriter->SetInput((vtkImageData *)input);
    writer = (vtkDataWriter *)spwriter;
    }

  else if ( type == VTK_STRUCTURED_GRID )
    {
    vtkStructuredGridWriter *sgwriter = vtkStructuredGridWriter::New();
    sgwriter->SetInput((vtkStructuredGrid *)input);
    writer = (vtkDataWriter *)sgwriter;
    }

  else if ( type == VTK_UNSTRUCTURED_GRID )
    {
    vtkUnstructuredGridWriter *ugwriter = vtkUnstructuredGridWriter::New();
    ugwriter->SetInput((vtkUnstructuredGrid *)input);
    writer = (vtkDataWriter *)ugwriter;
    }

  else if ( type == VTK_RECTILINEAR_GRID )
    {
    vtkRectilinearGridWriter *rgwriter = vtkRectilinearGridWriter::New();
    rgwriter->SetInput((vtkRectilinearGrid *)input);
    writer = (vtkDataWriter *)rgwriter;
    }

  else
    {
    vtkErrorMacro(<< "Cannot write dataset type: " << type);
    return;
    }

  writer->SetFileName(this->FileName);
  writer->SetScalarsName(this->ScalarsName);
  writer->SetVectorsName(this->VectorsName);
  writer->SetNormalsName(this->NormalsName);
  writer->SetTensorsName(this->TensorsName);
  writer->SetTCoordsName(this->TCoordsName);
  writer->SetHeader(this->Header);
  writer->SetLookupTableName(this->LookupTableName);
  writer->SetFieldDataName(this->FieldDataName);
  writer->SetFileType(this->FileType);
  writer->SetDebug(this->Debug);
  writer->SetWriteToOutputString(this->WriteToOutputString);
  writer->Write();
  if (this->WriteToOutputString)
    {
    if (this->OutputString)
      {
      delete [] this->OutputString;
      }
    this->OutputStringLength = writer->GetOutputStringLength();
    // should fill something here.
    this->OutputStringAllocatedLength = this->OutputStringLength;
    this->OutputString = writer->RegisterAndGetOutputString();
    }
  writer->Delete();
}

void vtkDataSetWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataWriter::PrintSelf(os,indent);
}
