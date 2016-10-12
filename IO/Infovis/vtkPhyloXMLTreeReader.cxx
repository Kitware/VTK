/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPhyloXMLTreeReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPhyloXMLTreeReader.h"

#include "vtkBitArray.h"
#include "vtkCharArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkInformationStringKey.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkNew.h"
#include "vtkTree.h"
#include "vtkTreeDFSIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkShortArray.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataParser.h"

vtkStandardNewMacro(vtkPhyloXMLTreeReader);

//----------------------------------------------------------------------------
vtkPhyloXMLTreeReader::vtkPhyloXMLTreeReader()
{
  vtkTree *output = vtkTree::New();
  this->SetOutput(output);
  // Releasing data for pipeline parallelism.
  // Filters will know it is empty.
  output->ReleaseData();
  output->Delete();

  this->NumberOfNodes = 0;
  this->HasBranchColor = false;
}

//----------------------------------------------------------------------------
vtkPhyloXMLTreeReader::~vtkPhyloXMLTreeReader()
{
}

//----------------------------------------------------------------------------
vtkTree* vtkPhyloXMLTreeReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkTree* vtkPhyloXMLTreeReader::GetOutput(int idx)
{
  return vtkTree::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
void vtkPhyloXMLTreeReader::SetOutput(vtkTree *output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//----------------------------------------------------------------------------
const char* vtkPhyloXMLTreeReader::GetDataSetName()
{
  return "phylogeny";
}

//----------------------------------------------------------------------------
void vtkPhyloXMLTreeReader::SetupEmptyOutput()
{
  this->GetOutput()->Initialize();
}

//----------------------------------------------------------------------------
void vtkPhyloXMLTreeReader::ReadXMLData()
{
  vtkXMLDataElement *rootElement = this->XMLParser->GetRootElement();
  this->CountNodes(rootElement);
  vtkNew<vtkMutableDirectedGraph> builder;

  // Initialize the edge weight array
  vtkNew<vtkDoubleArray> weights;
  weights->SetNumberOfComponents(1);
  weights->SetName("weight");
  //the number of edges = number of nodes -1 for a tree
  weights->SetNumberOfValues(this->NumberOfNodes - 1);
  weights->FillComponent(0, 0.0);
  builder->GetEdgeData()->AddArray(weights.GetPointer());

  // Initialize the names array
  vtkNew<vtkStringArray> names;
  names->SetNumberOfComponents(1);
  names->SetName("node name");
  names->SetNumberOfValues(this->NumberOfNodes);
  builder->GetVertexData()->AddArray(names.GetPointer());

  // parse the input file to create the tree
  this->ReadXMLElement(rootElement, builder.GetPointer(), -1);

  vtkTree *output = this->GetOutput();
  if (!output->CheckedDeepCopy(builder.GetPointer()))
  {
    vtkErrorMacro(<<"Edges do not create a valid tree.");
    return;
  }

  // assign branch color from parent to child where none was specified.
  this->PropagateBranchColor(output);

  // check if our input file contained edge weight information
  bool haveWeights = false;
  for (vtkIdType i = 0; i < weights->GetNumberOfTuples(); ++i)
  {
    if (weights->GetValue(i) != 0.0)
    {
      haveWeights = true;
      break;
    }
  }
  if (!haveWeights)
  {
    return;
  }

  vtkNew<vtkDoubleArray> nodeWeights;
  nodeWeights->SetNumberOfValues(output->GetNumberOfVertices());

  //set node weights
  vtkNew<vtkTreeDFSIterator> treeIterator;
  treeIterator->SetStartVertex(output->GetRoot());
  treeIterator->SetTree(output);
  while (treeIterator->HasNext())
  {
    vtkIdType vertex = treeIterator->Next();
    vtkIdType parent = output->GetParent(vertex);
    double weight = 0.0;
    if (parent >= 0)
    {
      weight = weights->GetValue(output->GetEdgeId(parent, vertex));
      weight += nodeWeights->GetValue(parent);
    }
    nodeWeights->SetValue(vertex, weight);
  }

  nodeWeights->SetName("node weight");
  output->GetVertexData()->AddArray(nodeWeights.GetPointer());
}

//----------------------------------------------------------------------------
void vtkPhyloXMLTreeReader::CountNodes(vtkXMLDataElement *element)
{
  if (strcmp(element->GetName(), "clade") == 0)
  {
    this->NumberOfNodes++;
  }

  int numNested = element->GetNumberOfNestedElements();
  for(int i = 0; i < numNested; ++i)
  {
    this->CountNodes(element->GetNestedElement(i));
  }
}

//----------------------------------------------------------------------------
void vtkPhyloXMLTreeReader::ReadXMLElement(vtkXMLDataElement *element,
  vtkMutableDirectedGraph *g, vtkIdType vertex)
{
  bool inspectNested = true;
  if (strcmp(element->GetName(), "clade") == 0)
  {
    vtkIdType child = this->ReadCladeElement(element, g, vertex);
    // update our current vertex to this newly created one.
    vertex = child;
  }

  else if (strcmp(element->GetName(), "name") == 0)
  {
    this->ReadNameElement(element, g, vertex);
  }
  else if (strcmp(element->GetName(), "description") == 0)
  {
    this->ReadDescriptionElement(element, g);
  }

  else if (strcmp(element->GetName(), "property") == 0)
  {
    this->ReadPropertyElement(element, g, vertex);
  }

  else if (strcmp(element->GetName(), "branch_length") == 0)
  {
    this->ReadBranchLengthElement(element, g, vertex);
  }

  else if (strcmp(element->GetName(), "confidence") == 0)
  {
    this->ReadConfidenceElement(element, g, vertex);
  }

  else if (strcmp(element->GetName(), "color") == 0)
  {
    this->ReadColorElement(element, g, vertex);
    inspectNested = false;
  }

  else if (strcmp(element->GetName(), "phyloxml") != 0 &&
           strcmp(element->GetName(), "phylogeny") != 0)
  {
    vtkWarningMacro(<< "Unsupported PhyloXML tag encountered: "
                    << element->GetName());
  }

  if (!inspectNested)
  {
    return;
  }

  int numNested = element->GetNumberOfNestedElements();
  for(int i = 0; i < numNested; ++i)
  {
    this->ReadXMLElement(element->GetNestedElement(i), g, vertex);
  }
}

//----------------------------------------------------------------------------
vtkIdType vtkPhyloXMLTreeReader::ReadCladeElement(vtkXMLDataElement *element,
  vtkMutableDirectedGraph *g, vtkIdType parent)
{
  // add a new vertex to the graph
  vtkIdType vertex = -1;
  if (parent == -1)
  {
    vertex = g->AddVertex();
  }
  else
  {
    vertex = g->AddChild(parent);
    // check for branch length attribute
    double weight = 0.0;
    element->GetScalarAttribute("branch_length", weight);
    g->GetEdgeData()->GetAbstractArray("weight")->SetVariantValue(
      g->GetEdgeId(parent, vertex), vtkVariant(weight));
  }

  // set a default (blank) name for this vertex here since
  // vtkStringArray does not support a default value.
  g->GetVertexData()->GetAbstractArray("node name")->SetVariantValue(
    vertex, vtkVariant(""));

  return vertex;
}

//----------------------------------------------------------------------------
void vtkPhyloXMLTreeReader::ReadNameElement(vtkXMLDataElement *element,
  vtkMutableDirectedGraph *g, vtkIdType vertex)
{
  std::string name = "";
  if (element->GetCharacterData() != NULL)
  {
    name = this->GetTrimmedString(element->GetCharacterData());
  }
  // support for phylogeny-level name (as opposed to clade-level name)
  if (vertex == -1)
  {
    vtkNew<vtkStringArray> treeName;
    treeName->SetNumberOfComponents(1);
    treeName->SetName("phylogeny.name");
    treeName->SetNumberOfValues(1);
    treeName->SetValue(0, name);
    g->GetVertexData()->AddArray(treeName.GetPointer());
  }
  else
  {
    g->GetVertexData()->GetAbstractArray("node name")->SetVariantValue(
      vertex, vtkVariant(name));
  }
}

//----------------------------------------------------------------------------
void vtkPhyloXMLTreeReader::ReadDescriptionElement(vtkXMLDataElement *element,
  vtkMutableDirectedGraph *g)
{
  std::string description = "";
  if (element->GetCharacterData() != NULL)
  {
    description = this->GetTrimmedString(element->GetCharacterData());
  }
  vtkNew<vtkStringArray> treeDescription;
  treeDescription->SetNumberOfComponents(1);
  treeDescription->SetName("phylogeny.description");
  treeDescription->SetNumberOfValues(1);
  treeDescription->SetValue(0, description);
  g->GetVertexData()->AddArray(treeDescription.GetPointer());
}

//----------------------------------------------------------------------------
void vtkPhyloXMLTreeReader::ReadPropertyElement(vtkXMLDataElement *element,
  vtkMutableDirectedGraph *g, vtkIdType vertex)
{
  const char *datatype = element->GetAttribute("datatype");
  if (!datatype)
  {
    vtkErrorMacro(<<"property element is missing the datatype attribute");
    return;
  }

  const char *ref = element->GetAttribute("ref");
  if (!ref)
  {
    vtkErrorMacro(<<"property element is missing the ref attribute");
    return;
  }

  const char *appliesTo = element->GetAttribute("applies_to");
  if (!appliesTo)
  {
    vtkErrorMacro(<<"property element is missing the applies_to attribute");
    return;
  }

  // get the name of this property from the ref tag.
  std::string propertyName = "property.";
  propertyName += this->GetStringAfterColon(ref);

  // get the authority for this property from the ref tag
  std::string authority = this->GetStringBeforeColon(ref);

  // get what type of data will be stored in this array.
  std::string typeOfData = this->GetStringAfterColon(datatype);

  // get the value for this property as a string
  std::string propertyValue =
    this->GetTrimmedString(element->GetCharacterData());

  // check if this property applies to a clade, or to the whole tree
  unsigned int numValues = this->NumberOfNodes;
  if (vertex == -1)
  {
    propertyName = "phylogeny." + propertyName;
    numValues = 1;
    vertex = 0;
  }

  if (typeOfData.compare("string") == 0 ||
      typeOfData.compare("duration") == 0 ||
      typeOfData.compare("dateTime") == 0 ||
      typeOfData.compare("time") == 0 ||
      typeOfData.compare("date") == 0 ||
      typeOfData.compare("gYearMonth") == 0 ||
      typeOfData.compare("gYear") == 0 ||
      typeOfData.compare("gMonthDay") == 0 ||
      typeOfData.compare("gDay") == 0 ||
      typeOfData.compare("gMonth") == 0 ||
      typeOfData.compare("anyURI") == 0 ||
      typeOfData.compare("normalizedString") == 0 ||
      typeOfData.compare("token") == 0 ||
      typeOfData.compare("hexBinary") == 0 ||
      typeOfData.compare("base64Binary") == 0)
  {
    if (!g->GetVertexData()->HasArray(propertyName.c_str()))
    {
      vtkNew<vtkStringArray> propertyArray;
      propertyArray->SetNumberOfComponents(1);
      propertyArray->SetNumberOfValues(numValues);
      propertyArray->SetName(propertyName.c_str());
      g->GetVertexData()->AddArray(propertyArray.GetPointer());
    }
    g->GetVertexData()->GetAbstractArray(propertyName.c_str())->SetVariantValue(
      vertex, vtkVariant(propertyValue));
  }

  else if (typeOfData.compare("boolean") == 0)
  {
    if (!g->GetVertexData()->HasArray(propertyName.c_str()))
    {
      vtkNew<vtkBitArray> propertyArray;
      propertyArray->SetNumberOfComponents(1);
      propertyArray->SetNumberOfValues(numValues);
      propertyArray->SetName(propertyName.c_str());
      g->GetVertexData()->AddArray(propertyArray.GetPointer());
    }
    int prop = 0;
    if (propertyValue.compare("true") == 0 ||
        propertyValue.compare("1") == 0)
    {
      prop = 1;
    }
    g->GetVertexData()->GetAbstractArray(propertyName.c_str())->SetVariantValue(
      vertex, vtkVariant(prop));
  }

  else if (typeOfData.compare("decimal") == 0 ||
           typeOfData.compare("float") == 0 ||
           typeOfData.compare("double") == 0)
  {
    if (!g->GetVertexData()->HasArray(propertyName.c_str()))
    {
      vtkNew<vtkDoubleArray> propertyArray;
      propertyArray->SetNumberOfComponents(1);
      propertyArray->SetNumberOfValues(numValues);
      propertyArray->SetName(propertyName.c_str());
      g->GetVertexData()->AddArray(propertyArray.GetPointer());
    }
    double prop = strtod(propertyValue.c_str(), NULL);
    g->GetVertexData()->GetAbstractArray(propertyName.c_str())->SetVariantValue(
      vertex, vtkVariant(prop));
  }

  else if (typeOfData.compare("int") == 0 ||
           typeOfData.compare("integer") == 0 ||
           typeOfData.compare("nonPositiveInteger") == 0 ||
           typeOfData.compare("negativeInteger") == 0)
  {
    if (!g->GetVertexData()->HasArray(propertyName.c_str()))
    {
      vtkNew<vtkIntArray> propertyArray;
      propertyArray->SetNumberOfComponents(1);
      propertyArray->SetNumberOfValues(numValues);
      propertyArray->SetName(propertyName.c_str());
      g->GetVertexData()->AddArray(propertyArray.GetPointer());
    }
    int prop = strtol(propertyValue.c_str(), NULL, 0);
    g->GetVertexData()->GetAbstractArray(propertyName.c_str())->SetVariantValue(
      vertex, vtkVariant(prop));
  }

  else if (typeOfData.compare("long") == 0)
  {
    if (!g->GetVertexData()->HasArray(propertyName.c_str()))
    {
      vtkNew<vtkLongArray> propertyArray;
      propertyArray->SetNumberOfComponents(1);
      propertyArray->SetNumberOfValues(numValues);
      propertyArray->SetName(propertyName.c_str());
      g->GetVertexData()->AddArray(propertyArray.GetPointer());
    }
    long prop = strtol(propertyValue.c_str(), NULL, 0);
    g->GetVertexData()->GetAbstractArray(propertyName.c_str())->SetVariantValue(
      vertex, vtkVariant(prop));
  }

  else if (typeOfData.compare("short") == 0)
  {
    if (!g->GetVertexData()->HasArray(propertyName.c_str()))
    {
      vtkNew<vtkShortArray> propertyArray;
      propertyArray->SetNumberOfComponents(1);
      propertyArray->SetNumberOfValues(numValues);
      propertyArray->SetName(propertyName.c_str());
      g->GetVertexData()->AddArray(propertyArray.GetPointer());
    }
    short prop = strtol(propertyValue.c_str(), NULL, 0);
    g->GetVertexData()->GetAbstractArray(propertyName.c_str())->SetVariantValue(
      vertex, vtkVariant(prop));
  }

  else if (typeOfData.compare("byte") == 0)
  {
    if (!g->GetVertexData()->HasArray(propertyName.c_str()))
    {
      vtkNew<vtkCharArray> propertyArray;
      propertyArray->SetNumberOfComponents(1);
      propertyArray->SetNumberOfValues(numValues);
      propertyArray->SetName(propertyName.c_str());
      g->GetVertexData()->AddArray(propertyArray.GetPointer());
    }
    char prop = strtol(propertyValue.c_str(), NULL, 0);
    g->GetVertexData()->GetAbstractArray(propertyName.c_str())->SetVariantValue(
      vertex, vtkVariant(prop));
  }

  else if (typeOfData.compare("nonNegativeInteger") == 0 ||
           typeOfData.compare("positiveInteger") == 0 ||
           typeOfData.compare("unsignedInt") == 0)
  {
    if (!g->GetVertexData()->HasArray(propertyName.c_str()))
    {
      vtkNew<vtkUnsignedIntArray> propertyArray;
      propertyArray->SetNumberOfComponents(1);
      propertyArray->SetNumberOfValues(numValues);
      propertyArray->SetName(propertyName.c_str());
      g->GetVertexData()->AddArray(propertyArray.GetPointer());
    }
    unsigned int prop = strtoul(propertyValue.c_str(), NULL, 0);
    g->GetVertexData()->GetAbstractArray(propertyName.c_str())->SetVariantValue(
      vertex, vtkVariant(prop));
  }
  else if (typeOfData.compare("unsignedLong") == 0)
  {
    if (!g->GetVertexData()->HasArray(propertyName.c_str()))
    {
      vtkNew<vtkUnsignedLongArray> propertyArray;
      propertyArray->SetNumberOfComponents(1);
      propertyArray->SetNumberOfValues(numValues);
      propertyArray->SetName(propertyName.c_str());
      g->GetVertexData()->AddArray(propertyArray.GetPointer());
    }
    unsigned long prop = strtoul(propertyValue.c_str(), NULL, 0);
    g->GetVertexData()->GetAbstractArray(propertyName.c_str())->SetVariantValue(
      vertex, vtkVariant(prop));
  }
  else if (typeOfData.compare("unsignedShort") == 0)
  {
    if (!g->GetVertexData()->HasArray(propertyName.c_str()))
    {
      vtkNew<vtkUnsignedShortArray> propertyArray;
      propertyArray->SetNumberOfComponents(1);
      propertyArray->SetNumberOfValues(numValues);
      propertyArray->SetName(propertyName.c_str());
      g->GetVertexData()->AddArray(propertyArray.GetPointer());
    }
    unsigned short prop = strtoul(propertyValue.c_str(), NULL, 0);
    g->GetVertexData()->GetAbstractArray(propertyName.c_str())->SetVariantValue(
      vertex, vtkVariant(prop));
  }
  else if (typeOfData.compare("unsignedByte") == 0)
  {
    if (!g->GetVertexData()->HasArray(propertyName.c_str()))
    {
      vtkNew<vtkUnsignedCharArray> propertyArray;
      propertyArray->SetNumberOfComponents(1);
      propertyArray->SetNumberOfValues(numValues);
      propertyArray->SetName(propertyName.c_str());
      g->GetVertexData()->AddArray(propertyArray.GetPointer());
    }
    unsigned char prop = strtoul(propertyValue.c_str(), NULL, 0);
    g->GetVertexData()->GetAbstractArray(propertyName.c_str())->SetVariantValue(
      vertex, vtkVariant(prop));
  }

  vtkAbstractArray *propertyArray =
    g->GetVertexData()->GetAbstractArray(propertyName.c_str());

  // add annotations to this array if it was just created.
  if (propertyArray->GetInformation()->GetNumberOfKeys() == 0)
  {
    // authority (required attribute)
    vtkInformationStringKey *authorityKey =
      vtkInformationStringKey::MakeKey("authority", "vtkPhyloXMLTreeReader");
    propertyArray->GetInformation()->Set(authorityKey, authority.c_str());

    // applies_to (required attribute)
    vtkInformationStringKey *appliesToKey =
      vtkInformationStringKey::MakeKey("applies_to", "vtkPhyloXMLTreeReader");
    propertyArray->GetInformation()->Set(appliesToKey, appliesTo);

    // unit (optional attribute)
    const char *unit = element->GetAttribute("unit");
    if (unit)
    {
      vtkInformationStringKey *unitKey =
        vtkInformationStringKey::MakeKey("unit", "vtkPhyloXMLTreeReader");
      propertyArray->GetInformation()->Set(unitKey, unit);
    }
  }
}

//----------------------------------------------------------------------------
void vtkPhyloXMLTreeReader::ReadBranchLengthElement(vtkXMLDataElement *element,
  vtkMutableDirectedGraph *g, vtkIdType vertex)
{
  std::string weightStr = this->GetTrimmedString(element->GetCharacterData());
  double weight = strtod(weightStr.c_str(), NULL);

  // This assumes that the vertex only has one incoming edge.
  // We have to use GetInEdge because g does not have a GetParent()
  // method.
  g->GetEdgeData()->GetAbstractArray("weight")->SetVariantValue(
    g->GetInEdge(vertex, 0).Id, vtkVariant(weight));
}

//----------------------------------------------------------------------------
void vtkPhyloXMLTreeReader::ReadConfidenceElement(vtkXMLDataElement *element,
  vtkMutableDirectedGraph *g, vtkIdType vertex)
{
  // get the confidence value
  double confidence = 0.0;
  if (element->GetCharacterData() != NULL)
  {
    std::string confidenceStr =
      this->GetTrimmedString(element->GetCharacterData());
    confidence = strtod(confidenceStr.c_str(), NULL);
  }

  // get the confidence type
  const char *type = element->GetAttribute("type");

  // support for phylogeny-level name (as opposed to clade-level name)
  if (vertex == -1)
  {
    vtkNew<vtkDoubleArray> treeConfidence;
    treeConfidence->SetNumberOfComponents(1);
    treeConfidence->SetName("phylogeny.confidence");
    treeConfidence->SetNumberOfValues(1);
    treeConfidence->SetValue(0, confidence);

    // add the confidence type as an Information type on this array
    vtkInformationStringKey *key =
      vtkInformationStringKey::MakeKey("type", "vtkPhyloXMLTreeReader");
    treeConfidence->GetInformation()->Set(key, type);

    g->GetVertexData()->AddArray(treeConfidence.GetPointer());
  }
  else
  {
    if (!g->GetVertexData()->HasArray("confidence"))
    {
      vtkNew<vtkDoubleArray> confidenceArray;
      confidenceArray->SetNumberOfComponents(1);
      confidenceArray->SetNumberOfValues(this->NumberOfNodes);
      confidenceArray->SetName("confidence");

      // add the confidence type as an Information type on this array
      vtkInformationStringKey *key =
        vtkInformationStringKey::MakeKey("type", "vtkPhyloXMLTreeReader");
      confidenceArray->GetInformation()->Set(key, type);

      g->GetVertexData()->AddArray(confidenceArray.GetPointer());
    }
    g->GetVertexData()->GetAbstractArray("confidence")->SetVariantValue(
      vertex, vtkVariant(confidence));
  }
}

//----------------------------------------------------------------------------
void vtkPhyloXMLTreeReader::ReadColorElement(vtkXMLDataElement *element,
  vtkMutableDirectedGraph *g, vtkIdType vertex)
{
  // get the color values
  unsigned char red = 0;
  unsigned char green = 0;
  unsigned char blue = 0;
  int numNested = element->GetNumberOfNestedElements();
  for(int i = 0; i < numNested; ++i)
  {
    vtkXMLDataElement *childElement = element->GetNestedElement(i);
    if (childElement->GetCharacterData() == NULL)
    {
      continue;
    }
    std::string childVal =
      this->GetTrimmedString(childElement->GetCharacterData());
    unsigned char val = static_cast<unsigned char>(strtod(childVal.c_str(), NULL));
    if (strcmp(childElement->GetName(), "red") == 0)
    {
      red = val;
    }
    else if (strcmp(childElement->GetName(), "green") == 0)
    {
      green = val;
    }
    else if (strcmp(childElement->GetName(), "blue") == 0)
    {
      blue = val;
    }
  }

  // initialize the color array if necessary
  if (!g->GetVertexData()->HasArray("color"))
  {
    vtkNew<vtkUnsignedCharArray> colorArray;
    colorArray->SetNumberOfComponents(3);
    colorArray->SetComponentName(0, "red");
    colorArray->SetComponentName(1, "green");
    colorArray->SetComponentName(2, "blue");
    colorArray->SetNumberOfTuples(this->NumberOfNodes);
    colorArray->SetName("color");
    colorArray->FillComponent(0, 0);
    colorArray->FillComponent(1, 0);
    colorArray->FillComponent(2, 0);
    g->GetVertexData()->AddArray(colorArray.GetPointer());
    this->HasBranchColor = true;

    // also set up an array so we can keep track of which vertices
    // have color.
    this->ColoredVertices = vtkSmartPointer<vtkBitArray>::New();
    this->ColoredVertices->SetNumberOfComponents(1);
    this->ColoredVertices->SetName("colored vertices");
    for (int i = 0; i < this->NumberOfNodes; ++i)
    {
      this->ColoredVertices->InsertNextValue(0);
    }
  }

  // store this color value in the array
  vtkUnsignedCharArray *colorArray = vtkArrayDownCast<vtkUnsignedCharArray>(
    g->GetVertexData()->GetAbstractArray("color"));
  colorArray->SetTuple3(vertex, red, green, blue);
  this->ColoredVertices->SetValue(vertex, 1);
}

//----------------------------------------------------------------------------
void vtkPhyloXMLTreeReader::PropagateBranchColor(vtkTree *tree)
{
  if (!this->HasBranchColor)
  {
    return;
  }

  vtkUnsignedCharArray *colorArray = vtkArrayDownCast<vtkUnsignedCharArray>(
    tree->GetVertexData()->GetAbstractArray("color"));
  if (!colorArray)
  {
    return;
  }

  for (vtkIdType vertex = 1; vertex < tree->GetNumberOfVertices(); ++vertex)
  {
    if (this->ColoredVertices->GetValue(vertex) == 0)
    {
      vtkIdType parent = tree->GetParent(vertex);
      double *color = colorArray->GetTuple3(parent);
      colorArray->SetTuple3(vertex, color[0], color[1], color[2]);
    }
  }
}

//----------------------------------------------------------------------------
std::string vtkPhyloXMLTreeReader::GetTrimmedString(const char *input)
{
  std::string trimmedString = "";
  std::string whitespace = " \t\r\n";
  std::string untrimmed = input;
  size_t strBegin = untrimmed.find_first_not_of(whitespace);
  if (strBegin != std::string::npos)
  {
    size_t strEnd = untrimmed.find_last_not_of(whitespace);
    trimmedString = untrimmed.substr(strBegin, strEnd - strBegin + 1);
  }
  return trimmedString;
}

//----------------------------------------------------------------------------
std::string vtkPhyloXMLTreeReader::GetStringBeforeColon(const char *input)
{
  std::string fullStr(input);
  size_t strEnd = fullStr.find(':');
  std::string retStr =
    fullStr.substr(0, strEnd);
  return retStr;
}

//----------------------------------------------------------------------------
std::string vtkPhyloXMLTreeReader::GetStringAfterColon(const char *input)
{
  std::string fullStr(input);
  size_t strBegin = fullStr.find(':') + 1;
  std::string retStr =
    fullStr.substr(strBegin, fullStr.size() - strBegin + 1);
  return retStr;
}

//----------------------------------------------------------------------------
int vtkPhyloXMLTreeReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTree");
  return 1;
}

//----------------------------------------------------------------------------
void vtkPhyloXMLTreeReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
