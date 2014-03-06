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
#include "vtkInformationIterator.h"
#include "vtkInformationStringKey.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkTree.h"
#include "vtkUnsignedCharArray.h"
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

  // PhyloXML supports some optional elements for the entire tree.
  this->WriteTreeLevelElement(input, rootElement.GetPointer(), "name", "");
  this->WriteTreeLevelElement(input, rootElement.GetPointer(), "description",
                              "");
  this->WriteTreeLevelElement(input, rootElement.GetPointer(), "confidence",
                              "type");
  this->WriteTreeLevelProperties(input, rootElement.GetPointer());

  // Generate PhyloXML for the vertices of the input tree.
  this->WriteCladeElement(input, input->GetRoot(), rootElement.GetPointer());

  rootElement->PrintXML(*this->Stream, vtkIndent());
  this->EndFile();
  return 1;
}

//----------------------------------------------------------------------------
void vtkPhyloXMLTreeWriter::WriteTreeLevelElement(
  vtkTree *input, vtkXMLDataElement *rootElement, const char *elementName,
  const char *attributeName)
{
  std::string arrayName = "phylogeny.";
  arrayName += elementName;
  vtkAbstractArray *array =
    input->GetVertexData()->GetAbstractArray(arrayName.c_str());
  if (array)
    {
    vtkNew<vtkXMLDataElement> element;
    element->SetName(elementName);
    vtkStdString val = array->GetVariantValue(0).ToString();
    element->SetCharacterData(val, static_cast<int>(val.length()));

    // set the attribute for this element if one was requested.
    if (strcmp(attributeName, "") != 0)
      {
      const char* attributeValue =
        this->GetArrayAttribute(array, attributeName);
      if (strcmp(attributeValue, "") != 0)
        {
        element->SetAttribute(attributeName, attributeValue);
        }
      }

    rootElement->AddNestedElement(element.GetPointer());

    // add this array to the blacklist so we don't try to write it again later
    this->Blacklist->InsertNextValue(arrayName.c_str());
    }
}

//----------------------------------------------------------------------------
void vtkPhyloXMLTreeWriter::WriteTreeLevelProperties(
  vtkTree *input, vtkXMLDataElement *element)
{
  std::string prefix = "phylogeny.property.";
  for (int i = 0; i < input->GetVertexData()->GetNumberOfArrays(); ++i)
    {
    vtkAbstractArray *arr = input->GetVertexData()->GetAbstractArray(i);
    std::string arrName = arr->GetName();
    if (arrName.compare(0, prefix.length(), prefix) == 0)
      {
      this->WritePropertyElement(arr, -1, element);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPhyloXMLTreeWriter::WriteCladeElement(vtkTree* const input,
                                              vtkIdType vertex,
                                              vtkXMLDataElement *parentElement)
{
  // create new clade element for this vertex
  vtkNew<vtkXMLDataElement> cladeElement;
  cladeElement->SetName("clade");

  // write out clade-level elements
  this->WriteBranchLengthAttribute(input, vertex, cladeElement.GetPointer());
  this->WriteNameElement(vertex, cladeElement.GetPointer());
  this->WriteConfidenceElement(input, vertex, cladeElement.GetPointer());
  this->WriteColorElement(input, vertex, cladeElement.GetPointer());

  // represent any other non-blacklisted VertexData arrays as PhyloXML
  // property elements.
  for (int i = 0; i < input->GetVertexData()->GetNumberOfArrays(); ++i)
    {
    vtkAbstractArray *array = input->GetVertexData()->GetAbstractArray(i);
    if (array == this->NodeNameArray || array == this->EdgeWeightArray)
      {
      continue;
      }

    if (this->Blacklist->LookupValue(array->GetName()) != -1)
      {
      continue;
      }

    this->WritePropertyElement(array, vertex, cladeElement.GetPointer());
    }

  // create clade elements for any children of this vertex.
  vtkIdType numChildren = input->GetNumberOfChildren(vertex);
  if (numChildren > 0)
    {
    for (vtkIdType child = 0; child < numChildren; ++child)
      {
      this->WriteCladeElement(input, input->GetChild(vertex, child),
                              cladeElement.GetPointer());
      }
    }

  parentElement->AddNestedElement(cladeElement.GetPointer());
}

//----------------------------------------------------------------------------
void vtkPhyloXMLTreeWriter::WriteBranchLengthAttribute(
  vtkTree* const input, vtkIdType vertex, vtkXMLDataElement *element)
{
  if (!this->EdgeWeightArray)
    {
    return;
    }

  vtkIdType parent = input->GetParent(vertex);
  if (parent != -1)
    {
    vtkIdType edge = input->GetEdgeId(parent, vertex);
    if (edge != -1)
      {
      double weight = this->EdgeWeightArray->GetVariantValue(edge).ToDouble();
      element->SetDoubleAttribute("branch_length", weight);
      }
    }

  if (this->Blacklist->LookupValue(this->EdgeWeightArray->GetName()) == -1)
    {
    this->IgnoreArray(this->EdgeWeightArray->GetName());
    }
}

//----------------------------------------------------------------------------
void vtkPhyloXMLTreeWriter::WriteNameElement(vtkIdType vertex,
                                             vtkXMLDataElement *element)
{
  if (!this->NodeNameArray)
    {
    return;
    }

  vtkStdString name = this->NodeNameArray->GetVariantValue(vertex).ToString();
  if (name.compare("") != 0)
    {
    vtkNew<vtkXMLDataElement> nameElement;
    nameElement->SetName("name");
    nameElement->SetCharacterData(name, static_cast<int>(name.length()));
    element->AddNestedElement(nameElement.GetPointer());
    }

  if (this->Blacklist->LookupValue(this->NodeNameArray->GetName()) == -1)
    {
    this->IgnoreArray(this->NodeNameArray->GetName());
    }
}

//----------------------------------------------------------------------------
void vtkPhyloXMLTreeWriter::WriteConfidenceElement(vtkTree* const input,
                                                   vtkIdType vertex,
                                                   vtkXMLDataElement *element)
{
  vtkAbstractArray *confidenceArray =
    input->GetVertexData()->GetAbstractArray("confidence");
  if (!confidenceArray)
    {
    return;
    }

  vtkStdString confidence = confidenceArray->GetVariantValue(vertex).ToString();
  if (confidence.compare("") != 0)
    {
    vtkNew<vtkXMLDataElement> confidenceElement;
    confidenceElement->SetName("confidence");

    // set the type attribute for this element if possible.
    const char* type = this->GetArrayAttribute(confidenceArray, "type");
    if (strcmp(type, "") != 0)
      {
      confidenceElement->SetAttribute("type", type);
      }

    confidenceElement->SetCharacterData(confidence, static_cast<int>(confidence.length()));
    element->AddNestedElement(confidenceElement.GetPointer());
    }

  if (this->Blacklist->LookupValue("confidence") == -1)
    {
    this->IgnoreArray("confidence");
    }
}

//----------------------------------------------------------------------------
void vtkPhyloXMLTreeWriter::WriteColorElement(vtkTree* const input,
                                              vtkIdType vertex,
                                              vtkXMLDataElement *element)
{
  vtkUnsignedCharArray *colorArray = vtkUnsignedCharArray::SafeDownCast(
    input->GetVertexData()->GetAbstractArray("color"));
  if (!colorArray)
    {
    return;
    }

  vtkNew<vtkXMLDataElement> colorElement;
  colorElement->SetName("color");

  vtkNew<vtkXMLDataElement> redElement;
  redElement->SetName("red");
  std::string r = vtkVariant(colorArray->GetComponent(vertex, 0)).ToString();
  redElement->SetCharacterData(r.c_str(), static_cast<int>(r.length()));

  vtkNew<vtkXMLDataElement> greenElement;
  greenElement->SetName("green");
  std::string g = vtkVariant(colorArray->GetComponent(vertex, 1)).ToString();
  greenElement->SetCharacterData(g.c_str(), static_cast<int>(g.length()));

  vtkNew<vtkXMLDataElement> blueElement;
  blueElement->SetName("blue");
  std::string b = vtkVariant(colorArray->GetComponent(vertex, 2)).ToString();
  blueElement->SetCharacterData(b.c_str(), static_cast<int>(b.length()));

  colorElement->AddNestedElement(redElement.GetPointer());
  colorElement->AddNestedElement(greenElement.GetPointer());
  colorElement->AddNestedElement(blueElement.GetPointer());

  element->AddNestedElement(colorElement.GetPointer());

  if (this->Blacklist->LookupValue("color") == -1)
    {
    this->IgnoreArray("color");
    }
}

//----------------------------------------------------------------------------
void vtkPhyloXMLTreeWriter::WritePropertyElement(vtkAbstractArray *array,
                                                 vtkIdType vertex,
                                                 vtkXMLDataElement *element)
{
  // Search for attribute on this array.
  std::string authority = "";
  std::string appliesTo = "";
  std::string unit = "";

  vtkInformation *info = array->GetInformation();
  vtkNew<vtkInformationIterator> infoItr;
  infoItr->SetInformation(info);
  for (infoItr->InitTraversal(); !infoItr->IsDoneWithTraversal();
    infoItr->GoToNextItem())
    {
    vtkInformationStringKey* key =
      vtkInformationStringKey::SafeDownCast(infoItr->GetCurrentKey());
    if (strcmp(key->GetName(), "authority") == 0)
      {
      authority = info->Get(key);
      }
    else if (strcmp(key->GetName(), "applies_to") == 0)
      {
      appliesTo = info->Get(key);
      }
    else if (strcmp(key->GetName(), "unit") == 0)
      {
      unit = info->Get(key);
      }
    }

  // authority is a required attribute.  Use "VTK:" if one wasn't specified
  // on the array.
  if (authority.compare("") == 0)
    {
    authority = "VTK";
    }

  // applies_to is also required.  Use "clade" if one was not specified.
  if (appliesTo.compare("") == 0)
    {
    appliesTo = "clade";
    }

  // construct the value for the "ref" attribute.
  std::string arrayName = array->GetName();
  std::string prefix = "property.";
  size_t strBegin = arrayName.find(prefix);
  if (strBegin == std::string::npos)
    {
    strBegin = 0;
    }
  else
    {
    strBegin += prefix.length();
    }
  std::string propertyName = arrayName.substr(strBegin, arrayName.size() - strBegin + 1);
  std::string ref = authority;
  ref += ":";
  ref += propertyName;

  // if vertex is -1, this means that this is a tree-level property.
  if (vertex == -1)
    {
    vertex = 0;
    this->IgnoreArray(arrayName.c_str());
    }

  // get the value for the "datatype" attribute.
  // This requiring converting the type as reported by VTK variant
  // to an XML-compliant type.
  std::string variantType = array->GetVariantValue(vertex).GetTypeAsString();
  std::string datatype = "xsd:string";
  if (variantType.compare("short") == 0 ||
      variantType.compare("long") == 0 ||
      variantType.compare("float") == 0 ||
      variantType.compare("double") == 0)
    {
    datatype = "xsd:";
    datatype += variantType;
    }
  if (variantType.compare("int") == 0)
    {
    datatype = "xsd:integer";
    }
  else if (variantType.compare("bit") == 0)
    {
    datatype = "xsd:boolean";
    }
  else if (variantType.compare("char") == 0  ||
           variantType.compare("signed char") == 0)
    {
    datatype = "xsd:byte";
    }
  else if (variantType.compare("unsigned char") == 0)
    {
    datatype = "xsd:unsignedByte";
    }
  else if (variantType.compare("unsigned short") == 0)
    {
    datatype = "xsd:unsignedShort";
    }
  else if (variantType.compare("unsigned int") == 0)
    {
    datatype = "xsd:unsignedInt";
    }
  else if (variantType.compare("unsigned long") == 0 ||
           variantType.compare("unsigned __int64") == 0 ||
           variantType.compare("idtype") == 0)
    {
    datatype = "xsd:unsignedLong";
    }
  else if (variantType.compare("__int64") == 0)
    {
    datatype = "xsd:long";
    }

  // get the value for this property
  vtkStdString val = array->GetVariantValue(vertex).ToString();

  // create the new property element and add it to our document.
  vtkNew<vtkXMLDataElement> propertyElement;
  propertyElement->SetName("property");
  propertyElement->SetAttribute("datatype", datatype.c_str());
  propertyElement->SetAttribute("ref", ref.c_str());
  propertyElement->SetAttribute("applies_to", appliesTo.c_str());
  if (unit.compare("") != 0)
    {
    propertyElement->SetAttribute("unit", unit.c_str());
    }
  propertyElement->SetCharacterData(val, static_cast<int>(val.length()));

  element->AddNestedElement(propertyElement.GetPointer());
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
const char* vtkPhyloXMLTreeWriter::GetArrayAttribute(vtkAbstractArray *array,
                                                     const char *attributeName)
{
  vtkInformation *info = array->GetInformation();
  vtkNew<vtkInformationIterator> infoItr;
  infoItr->SetInformation(info);
  for (infoItr->InitTraversal(); !infoItr->IsDoneWithTraversal();
    infoItr->GoToNextItem())
    {
    if (strcmp(infoItr->GetCurrentKey()->GetName(), attributeName) == 0)
      {
      vtkInformationStringKey* key =
        vtkInformationStringKey::SafeDownCast(infoItr->GetCurrentKey());
      if (key)
        {
        return info->Get(key);
        }
      }
    }
  return "";
}

//----------------------------------------------------------------------------
void vtkPhyloXMLTreeWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "EdgeWeightArrayName: " << this->EdgeWeightArrayName << endl;
  os << indent << "NodeNameArrayName: " << this->NodeNameArrayName << endl;
}
