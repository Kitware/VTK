/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAttributeDataToFieldDataFilter.cxx
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
#include "vtkAttributeDataToFieldDataFilter.h"

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
  vtkTensors *tensors;
  vtkNormals *normals;
  vtkTCoords *tcoords;
  vtkFieldData *fieldData;
  int arrayNum=0;
  
  vtkDebugMacro(<<"Generating field data from attribute data");

  if ( inPD->GetScalars() || inPD->GetVectors() || inPD->GetTensors() ||
       inPD->GetNormals() || inPD->GetTCoords() || inPD->GetFieldData() )
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
       inCD->GetNormals() || inCD->GetTCoords() || inCD->GetFieldData() )
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

