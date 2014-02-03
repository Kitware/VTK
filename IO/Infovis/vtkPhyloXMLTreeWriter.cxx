/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPhyloXMLTreeWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPhyloXMLTreeWriter.h"

#include "vtkDataSetAttributes.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkTree.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkPhyloXMLTreeWriter);

//----------------------------------------------------------------------------
vtkPhyloXMLTreeWriter::vtkPhyloXMLTreeWriter()
{
  this->EdgeWeightArrayName = "weight";
  this->NodeNameArrayName = "node name";

  this->EdgeWeightArray = NULL;
  this->NodeNameArray = NULL;
  this->Blacklist = vtkSmartPointer<vtkStringArray>::New();
}

//----------------------------------------------------------------------------
int vtkPhyloXMLTreeWriter::StartFile()
{
  ostream& os = *(this->Stream);
  os.imbue(std::locale::classic());

  // Open the document-level element.  This will contain the rest of
  // the elements.
  os << "<phyloxml xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
     << " xmlns=\"http://www.phyloxml.org\" xsi:schemaLocation=\""
     << "http://www.phyloxml.org http://www.phyloxml.org/1.10/phyloxml.xsd\">"
     << endl;

  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPhyloXMLTreeWriter::EndFile()
{
  ostream& os = *(this->Stream);

  // Close the document-level element.
  os << "</phyloxml>\n";

  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPhyloXMLTreeWriter::WriteData()
{
  vtkDebugMacro(<<"Writing vtk tree data as PhyloXML...");

  vtkTree* const input = this->GetInput();

  this->EdgeWeightArray =
    input->GetEdgeData()->GetAbstractArray(this->EdgeWeightArrayName.c_str());

  this->NodeNameArray =
    input->GetVertexData()->GetAbstractArray(this->NodeNameArrayName.c_str());

  if(this->StartFile() == 0)
    {
    return 0;
    }

  vtkNew<vtkXMLDataElement> rootElement;
  rootElement->SetName("phylogeny");
  rootElement->SetAttribute("rooted", "true");
  // PhyloXML also supports name & description for the entire tree.
  // I don't think we have any equivalent data field in vtkTree currently...

  this->ConvertVertexToXML(input, input->GetRoot(), rootElement.GetPointer());

  rootElement->PrintXML(*this->Stream, vtkIndent());
  this->EndFile();
  return 1;
}

//----------------------------------------------------------------------------
void vtkPhyloXMLTreeWriter::ConvertVertexToXML(vtkTree* const input,
                                               vtkIdType vertex,
                                               vtkXMLDataElement *parentElement)
{
  vtkNew<vtkXMLDataElement> cladeElement;
  cladeElement->SetName("clade");

  if (this->EdgeWeightArray)
    {
    vtkIdType parent = input->GetParent(vertex);
    if (parent != -1)
      {
      vtkIdType edge = input->GetEdgeId(parent, vertex);
      if (edge != -1)
        {
        double weight = this->EdgeWeightArray->GetVariantValue(edge).ToDouble();
        cladeElement->SetDoubleAttribute("branch_length", weight);
        }
      }
    }

  if (this->NodeNameArray)
    {
    vtkStdString name = this->NodeNameArray->GetVariantValue(vertex).ToString();
    if (name != "")
      {
      vtkNew<vtkXMLDataElement> nameElement;
      nameElement->SetName("name");
      nameElement->SetCharacterData(name, name.length());
      cladeElement->AddNestedElement(nameElement.GetPointer());
      }
    }

  // support for other VertexData.
  for (int i = 0; i < input->GetVertexData()->GetNumberOfArrays(); ++i)
    {
    vtkAbstractArray *arr = input->GetVertexData()->GetAbstractArray(i);
    if (arr == this->NodeNameArray || arr == this->EdgeWeightArray)
      {
      continue;
      }

    std::string arrName = arr->GetName();
    if (this->Blacklist->LookupValue(arrName) != -1)
      {
      continue;
      }

    vtkStdString val = arr->GetVariantValue(vertex).ToString();
    std::string type = "xsd:";
    type += arr->GetVariantValue(vertex).GetTypeAsString();

    vtkNew<vtkXMLDataElement> propertyElement;
    propertyElement->SetName("property");
    propertyElement->SetAttribute("datatype", type.c_str());
    propertyElement->SetAttribute("ref", arrName.c_str());
    propertyElement->SetAttribute("applies_to", "clade");
    propertyElement->SetCharacterData(val, val.length());

    cladeElement->AddNestedElement(propertyElement.GetPointer());
    }

  vtkIdType numChildren = input->GetNumberOfChildren(vertex);
  if (numChildren > 0)
    {
    for (vtkIdType child = 0; child < numChildren; ++child)
      {
      this->ConvertVertexToXML(input, input->GetChild(vertex, child),
                               cladeElement.GetPointer());
      }
    }

  parentElement->AddNestedElement(cladeElement.GetPointer());
}

//----------------------------------------------------------------------------
int vtkPhyloXMLTreeWriter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTree");
  return 1;
}

//----------------------------------------------------------------------------
vtkTree* vtkPhyloXMLTreeWriter::GetInput()
{
  return vtkTree::SafeDownCast(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
vtkTree* vtkPhyloXMLTreeWriter::GetInput(int port)
{
  return vtkTree::SafeDownCast(this->Superclass::GetInput(port));
}

//----------------------------------------------------------------------------
const char* vtkPhyloXMLTreeWriter::GetDefaultFileExtension()
{
  return "xml";
}

//----------------------------------------------------------------------------
const char* vtkPhyloXMLTreeWriter::GetDataSetName()
{
  if (!this->InputInformation)
    {
    return "vtkTree";
    }
  vtkDataObject *hdInput = vtkDataObject::SafeDownCast(
    this->InputInformation->Get(vtkDataObject::DATA_OBJECT()));
  if (!hdInput)
    {
    return 0;
    }
  return hdInput->GetClassName();
}

//----------------------------------------------------------------------------
void vtkPhyloXMLTreeWriter::IgnoreArray(const char * arrayName)
{
  this->Blacklist->InsertNextValue(arrayName);
}

//----------------------------------------------------------------------------
void vtkPhyloXMLTreeWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "EdgeWeightArrayName: " << this->EdgeWeightArrayName << endl;
  os << indent << "NodeNameArrayName: " << this->NodeNameArrayName << endl;
}
