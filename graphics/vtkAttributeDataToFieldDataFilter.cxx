/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAttributeDataToFieldDataFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkAttributeDataToFieldDataFilter.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkAttributeDataToFieldDataFilter* vtkAttributeDataToFieldDataFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkAttributeDataToFieldDataFilter");
  if(ret)
    {
    return (vtkAttributeDataToFieldDataFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkAttributeDataToFieldDataFilter;
}




// Instantiate object.
vtkAttributeDataToFieldDataFilter::vtkAttributeDataToFieldDataFilter()
{
  this->PassAttributeData = 1;
}

void vtkAttributeDataToFieldDataFilter::Execute()
{
  vtkDataSet *input = (vtkDataSet *)this->GetInput();
  vtkDataSet *output = (vtkDataSet *)this->GetOutput();
  vtkPointData *inPD=input->GetPointData(), *outPD=output->GetPointData();
  vtkCellData *inCD=input->GetCellData(), *outCD=output->GetCellData();
  vtkScalars *scalars;
  vtkVectors *vectors;
  vtkGhostLevels *ghostLevels;
  vtkTensors *tensors;
  vtkNormals *normals;
  vtkTCoords *tcoords;
  vtkFieldData *fieldData;
  int arrayNum=0;
  
  vtkDebugMacro(<<"Generating field data from attribute data");

  // Start by copying input to output
  output->CopyStructure( input );

  if ( inPD->GetScalars() || inPD->GetVectors() || inPD->GetTensors() ||
       inPD->GetNormals() || inPD->GetTCoords() || inPD->GetFieldData() ||
       inPD->GetGhostLevels() )
    {
    vtkFieldData *fd=vtkFieldData::New();

    if ( scalars=inPD->GetScalars() )
      {
      fd->SetArray(arrayNum, scalars->GetData());
      fd->SetArrayName(arrayNum++, "PointScalars");
      }

    if ( vectors=inPD->GetVectors() )
      {
      fd->SetArray(arrayNum, vectors->GetData());
      fd->SetArrayName(arrayNum++, "PointVectors");
      }

    if ( ghostLevels=inPD->GetGhostLevels() )
      {
      fd->SetArray(arrayNum, ghostLevels->GetData());
      fd->SetArrayName(arrayNum++, "PointGhostLevels");
      }
    
    if ( tensors=inPD->GetTensors() )
      {
      fd->SetArray(arrayNum, tensors->GetData());
      fd->SetArrayName(arrayNum++, "PointTensors");
      }

    if ( normals=inPD->GetNormals() )
      {
      fd->SetArray(arrayNum, normals->GetData());
      fd->SetArrayName(arrayNum++, "PointNormals");
      }

    if ( tcoords=inPD->GetTCoords() )
      {
      fd->SetArray(arrayNum, tcoords->GetData());
      fd->SetArrayName(arrayNum++, "PointTCoords");
      }

    if ( fieldData=inPD->GetFieldData() )
      {
      vtkDataArray *da;
      for (int i=0; i<fieldData->GetNumberOfArrays(); i++)
        {
        da = fieldData->GetArray(i);
        fd->SetArray(arrayNum, da);
        fd->SetArrayName(arrayNum++, fieldData->GetArrayName(i));
        }
      }

    vtkDebugMacro(<<"Created point field data with " << fd->GetNumberOfArrays()
                  <<"arrays");

    outPD->SetFieldData(fd);
    fd->Delete();
    }
  
  if ( inCD->GetScalars() || inCD->GetVectors() || inCD->GetTensors() ||
       inCD->GetNormals() || inCD->GetTCoords() || inCD->GetFieldData() ||
       inCD->GetGhostLevels() )
    {
    vtkFieldData *fd=vtkFieldData::New();
    arrayNum = 0;

    if ( scalars=inCD->GetScalars() )
      {
      fd->SetArray(arrayNum, scalars->GetData());
      fd->SetArrayName(arrayNum++, "CellScalars");
      }

    if ( vectors=inCD->GetVectors() )
      {
      fd->SetArray(arrayNum, vectors->GetData());
      fd->SetArrayName(arrayNum++, "CellVectors");
      }

    if ( ghostLevels=inCD->GetGhostLevels() )
      {
      fd->SetArray(arrayNum, ghostLevels->GetData());
      fd->SetArrayName(arrayNum++, "CellGhostLevels");
      }
    
    if ( tensors=inCD->GetTensors() )
      {
      fd->SetArray(arrayNum, tensors->GetData());
      fd->SetArrayName(arrayNum++, "CellTensors");
      }

    if ( normals=inCD->GetNormals() )
      {
      fd->SetArray(arrayNum, normals->GetData());
      fd->SetArrayName(arrayNum++, "CellNormals");
      }

    if ( tcoords=inCD->GetTCoords() )
      {
      fd->SetArray(arrayNum, tcoords->GetData());
      fd->SetArrayName(arrayNum++, "CellTCoords");
      }

    if ( fieldData=inCD->GetFieldData() )
      {
      vtkDataArray *da;
      for (int i=0; i<fieldData->GetNumberOfArrays(); i++)
        {
        da = fieldData->GetArray(i);
        fd->SetArray(arrayNum, da);
        fd->SetArrayName(arrayNum++, fieldData->GetArrayName(i));
        }
      }

    vtkDebugMacro(<<"Created cell field data with " << fd->GetNumberOfArrays()
                  <<"arrays");

    outCD->SetFieldData(fd);
    fd->Delete();
    }

  if ( this->PassAttributeData )
    {
    outPD->PassNoReplaceData(inPD);
    outCD->PassNoReplaceData(inCD);
    }
}

void vtkAttributeDataToFieldDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Pass Attribute Data: " << (this->PassAttributeData ? "On\n" : "Off\n");
}

