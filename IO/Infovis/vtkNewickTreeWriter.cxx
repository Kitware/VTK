/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNewickTreeWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkNewickTreeWriter.h"

#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkTree.h"

vtkStandardNewMacro(vtkNewickTreeWriter);

//----------------------------------------------------------------------------
vtkNewickTreeWriter::vtkNewickTreeWriter()
{
  this->SetFileTypeToASCII();

  this->EdgeWeightArrayName = "weight";
  this->NodeNameArrayName = "node name";

  this->EdgeWeightArray = NULL;
  this->NodeNameArray = NULL;
}

//----------------------------------------------------------------------------
void vtkNewickTreeWriter::WriteData()
{
  vtkDebugMacro(<<"Writing vtk tree data...");

  vtkTree* const input = this->GetInput();

  this->EdgeWeightArray =
    input->GetEdgeData()->GetAbstractArray(this->EdgeWeightArrayName.c_str());

  this->NodeNameArray =
    input->GetVertexData()->GetAbstractArray(this->NodeNameArrayName.c_str());

  ostream *fp;
  if( !(fp=this->OpenVTKFile()) )
    {
    if(fp)
      {
      if(this->FileName)
        {
        vtkErrorMacro("Problem opening file: "
                      << this->FileName);
        this->CloseVTKFile(fp);
        }
      else
        {
        this->CloseVTKFile(fp);
        vtkErrorMacro("The FileName was not set correctly");
        }
      }
    return;
    }

  this->WriteVertex(fp, input, input->GetRoot());

  // the tree ends with a semi-colon
  *fp << ";";

  this->CloseVTKFile(fp);
}

//----------------------------------------------------------------------------
void vtkNewickTreeWriter::WriteVertex(ostream *fp, vtkTree* const input,
                                      vtkIdType vertex)
{
  vtkIdType numChildren = input->GetNumberOfChildren(vertex);
  if (numChildren > 0)
    {
    *fp << "(";
    for (vtkIdType child = 0; child < numChildren; ++child)
      {
      this->WriteVertex(fp, input, input->GetChild(vertex, child));
      if (child != numChildren - 1)
        {
        *fp << ",";
        }
      }
    *fp << ")";
    }

  if (this->NodeNameArray)
    {
    vtkStdString name = this->NodeNameArray->GetVariantValue(vertex).ToString();
    if (name != "")
      {
      *fp << name;
      }
    }

  if (this->EdgeWeightArray)
    {
    vtkIdType parent = input->GetParent(vertex);
    if (parent != -1)
      {
      vtkIdType edge = input->GetEdgeId(parent, vertex);
      if (edge != -1)
        {
        double weight = this->EdgeWeightArray->GetVariantValue(edge).ToDouble();
        *fp << ":" << weight;
        }
      }
    }
}

//----------------------------------------------------------------------------
int vtkNewickTreeWriter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTree");
  return 1;
}

//----------------------------------------------------------------------------
vtkTree* vtkNewickTreeWriter::GetInput()
{
  return vtkTree::SafeDownCast(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
vtkTree* vtkNewickTreeWriter::GetInput(int port)
{
  return vtkTree::SafeDownCast(this->Superclass::GetInput(port));
}

//----------------------------------------------------------------------------
void vtkNewickTreeWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "EdgeWeightArrayName: " << this->EdgeWeightArrayName << endl;
  os << indent << "NodeNameArrayName: " << this->NodeNameArrayName << endl;
}
