/*=========================================================================

  Program:   ParaView
  Module:    vtkExodusIIReaderParser.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExodusIIReaderParser.h"

#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkDataSetAttributes.h"

#include <cassert>

vtkStandardNewMacro(vtkExodusIIReaderParser);
vtkCxxRevisionMacro(vtkExodusIIReaderParser, "1.2");
//-----------------------------------------------------------------------------
vtkExodusIIReaderParser::vtkExodusIIReaderParser()
{
  this->SIL = vtkMutableDirectedGraph::New();
  this->InBlocks = false;
  this->InMaterialAssignments = false;
}

//-----------------------------------------------------------------------------
vtkExodusIIReaderParser::~vtkExodusIIReaderParser()
{
  this->SIL->Delete();
  this->SIL = 0;
}

//-----------------------------------------------------------------------------
void vtkExodusIIReaderParser::StartElement( const char* tagName, const char** attrs)
{
  const char* name = strrchr( tagName, ':' );

  // If tag name has xml namespace separator, get rid of namespace:
  name = name ? name + 1 : tagName; 
  vtkstd::string tName(name);

  if (tName == "solid-model")
    {
    // Move down to the Assemblies branch.
    this->CurrentVertex.push_back(this->AssembliesVertex);
    }
  else if (tName == "assembly")
    {
    // Starting a new "assembly" node. Get paratmeters for this assembly.
    const char* assemblyNumber = this->GetValue("number", attrs);
    const char* assemblyDescription = this->GetValue("description", attrs);

    // Setup the name for this node.
    vtkstd::string node_name = vtkstd::string("Assembly: ") +
      assemblyDescription + vtkstd::string(" (") +
      assemblyNumber + vtkstd::string(")");

    // Now add a vertex in the SIL for this assembly node.
    vtkIdType vertexID = this->AddVertexToSIL(node_name.c_str());
    this->AddChildEdgeToSIL(this->CurrentVertex.back(), vertexID);
    this->CurrentVertex.push_back(vertexID);
    }
  else if (tName == "part")
    {
    const char* instance = this->GetValue("instance", attrs);
    vtkstd::string instanceString = instance? instance : "";
    const char* partNumber =this->GetValue("number",attrs);
    vtkstd::string partNumberString;
    if (partNumber)
      {
      partNumberString = vtkstd::string(partNumber) +
        vtkstd::string(" Instance: ") + instanceString;
      }

    const char* partDescString=this->GetValue("description",attrs);

    // This will create a new vertex if none already present.
    vtkIdType partVertex = this->GetPartVertex(partNumberString.c_str());

    // Now fix the part vertex name.
    vtkstd::string result = vtkstd::string("Part: ") +
      partDescString + vtkstd::string(" (") +
      partNumber + vtkstd::string(")") + vtkstd::string(" Instance: ") +
      instanceString;
    this->NamesArray->InsertValue(partVertex, result.c_str());

    // Insert the part vertex info the assemblies hierarchy.
    this->AddChildEdgeToSIL(this->CurrentVertex.back(), partVertex);
    // The cross link between the part at the blocks it refers is added when the
    // <blocks/> are parsed.

    // Save the description for this part, this description is used later to
    // name the block appropriately.
    this->PartVertexID_To_Descriptions[partVertex] = partDescString? partDescString : "";

    // Add a "part" vertex in the "Assemblies" hierarchy.    
    this->CurrentVertex.push_back(partVertex);
    }
  else if (tName == "material-specification")
    {
    // The <part /> element may contain material-specification for each part.
    // These are used only if <material-assignments/> are not present.
    vtkIdType partVertex = this->CurrentVertex.back();

    const char * materialDescriptionString = this->GetValue("description", attrs);
    vtkstd::string material = materialDescriptionString ? materialDescriptionString : "";
    material += " : ";

    const char * materialSpecificationString =
      this->GetValue("specification", attrs);
    material += materialSpecificationString? materialSpecificationString : "";

    this->MaterialSpecifications[partVertex] = material;
    }
  else if (tName == "mesh")
    {
    assert(this->CurrentVertex.size() == 0);
    this->CurrentVertex.push_back(this->BlocksVertex);
    }
  else if (tName == "blocks")
    {
    const char* instance = this->GetValue("part-instance",attrs);
    vtkstd::string instanceString = instance ? instance : "";
    const char* partNumber =this->GetValue("part-number",attrs);
    vtkstd::string partNumberString;
    if (partNumber)
      {
      partNumberString = vtkstd::string(partNumber) +
        vtkstd::string(" Instance: ") + instanceString;
      }

    // Will will create a new vertex if none already exists.
    vtkIdType partVertex = this->GetPartVertex(partNumberString.c_str());
    this->CurrentVertex.push_back(partVertex);
    this->InBlocks = true;
    }
  else if (tName == "block")
    {
    const char* blockString=this->GetValue("id",attrs);
    int id=-1;
    if (blockString)
      {
      id = atoi(blockString);
      }

    if (id >= 0)
      {
      if (this->InBlocks)
        {
        // the name for the block is re-generated at the end.
        vtkIdType blockVertex = this->AddVertexToSIL(blockString);
        this->AddChildEdgeToSIL(this->BlocksVertex, blockVertex);

        // This <block /> element was encountered while reading the <mesh />.
        this->BlockID_To_VertexID[id] = blockVertex;
       
        // Add cross edge linking the assembly part to the block.
        vtkIdType partVertex = this->CurrentVertex.back();
        this->AddCrossEdgeToSIL(partVertex, blockVertex);

        this->BlockID_To_PartVertexID[id] = partVertex;
        }
      else if (this->InMaterialAssignments)
        {
        // This <block /> element was encountered while reading the
        // <material-assignments />
        const char* tmaterialName=this->GetValue("material-name",attrs);
        if (tmaterialName)
          {
          // Save the material information for later since we may not have
          // seen the <blocks /> yet, consequently we have no mapping from
          // vertex to block id.
          this->BlockID_To_MaterialName[id] = tmaterialName;
          }
        }
      }
    }
  else if (tName == "material-assignments")
    {
    this->CurrentVertex.push_back(this->MaterialsVertex);
    this->InMaterialAssignments = true;
    }
  else if ( tName == "material" )
    {
    const char* material = this->GetValue("name",attrs);
    const char* spec = this->GetValue("specification",attrs);
    const char* desc = this->GetValue("description",attrs);
    vtkstd::string node_name;
    if (material && desc)
      {
      node_name = desc;
      }
    else
      {
      node_name = material;
      }
    if (material && spec)
      {
      node_name += " : ";
      node_name += spec;
      }

    vtkIdType vertex = this->AddVertexToSIL(node_name.c_str());
    this->AddChildEdgeToSIL(this->MaterialsVertex, vertex);
    this->MaterialName_To_VertexID[material] = vertex;
    }
}

//-----------------------------------------------------------------------------
void vtkExodusIIReaderParser::EndElement(const char* tagName)
{
  const char* name = strrchr( tagName, ':' );
  // If tag name has xml namespace separator, get rid of namespace:
  name = name ? name + 1 : tagName; 
  vtkstd::string tName(name);
  if (tName == "solid-model")
    {
    this->CurrentVertex.pop_back();
    }
  else if (tName == "assembly")
    {
    this->CurrentVertex.pop_back();
    }
  else if (tName == "part")
    {
    this->CurrentVertex.pop_back();
    }
  else if (tName == "mesh")
    {
    this->CurrentVertex.pop_back();
    }
  else if (tName == "blocks")
    {
    this->InBlocks = false;
    this->CurrentVertex.pop_back();
    }
  else if (tName == "material-assignments")
    {
    this->InMaterialAssignments = false;
    this->CurrentVertex.pop_back();
    }
}

//-----------------------------------------------------------------------------
void vtkExodusIIReaderParser::FinishedParsing()
{
  // * Assign correct names for all the "block" vertices.
  vtkstd::map<int, vtkIdType>::iterator iter;
  for (iter = this->BlockID_To_VertexID.begin();
    iter != this->BlockID_To_VertexID.end(); ++iter)
    {
    // To locate the part description for this block, first locate the part to
    // which this block belongs.
    if (this->BlockID_To_PartVertexID.find(iter->first) ==
      this->BlockID_To_PartVertexID.end())
      {
      vtkWarningMacro("Invalid XML.");
      continue;
      }

    vtkIdType partVertex = this->BlockID_To_PartVertexID[iter->first];

    vtksys_ios::ostringstream stream;
    stream << "Block: " << iter->first 
      << " (" << this->PartVertexID_To_Descriptions[partVertex].c_str() << ") "
      << this->PartVertexID_To_PartNumberInstanceString[partVertex].c_str(); 
    this->NamesArray->SetValue(iter->second, stream.str().c_str());
    }

  //// * If <material-assignments /> are not present use
  //// <material-specification /> to construct material assignemnts.
  if (this->BlockID_To_MaterialName.size() == 0)
    {
    vtkstd::map<int, vtkIdType>::iterator iterII;
    for (iterII = this->BlockID_To_PartVertexID.begin();
      iterII != this->BlockID_To_PartVertexID.end();
      iterII++)
      {
      int blockID = iterII->first;
      vtkIdType partVertex = iterII->second;
      
      vtkstd::string node_name = this->MaterialSpecifications[partVertex];
      vtkIdType materialVertex;
      if (this->MaterialName_To_VertexID.find(node_name) ==
        this->MaterialName_To_VertexID.end())
        {
        materialVertex = this->AddVertexToSIL(node_name.c_str());
        this->AddChildEdgeToSIL(this->MaterialsVertex, materialVertex);
        this->MaterialName_To_VertexID[node_name] = materialVertex;
        }
      else
        {
        materialVertex = this->MaterialName_To_VertexID[node_name];
        }
      this->BlockID_To_MaterialName[blockID] = node_name;
      }
    }

  //// * Add cross-links between "block" vertices and "material" vertices.
  vtkstd::map<int, vtkstd::string>::iterator iter2;
  for (iter2 = this->BlockID_To_MaterialName.begin();
    iter2 != this->BlockID_To_MaterialName.end(); ++iter2)
    {
    vtkIdType blockVertex = this->BlockID_To_VertexID[iter2->first];
    if (this->MaterialName_To_VertexID.find(iter2->second) !=
      this->MaterialName_To_VertexID.end())
      {
      vtkIdType materialVertex = this->MaterialName_To_VertexID[iter2->second];
      this->AddCrossEdgeToSIL(materialVertex, blockVertex);
      }
    }
}

//-----------------------------------------------------------------------------
vtkIdType vtkExodusIIReaderParser::AddVertexToSIL(const char* name)
{
  vtkIdType vertex = this->SIL->AddVertex();
  this->NamesArray->InsertValue(vertex, name);
  return vertex;
}

//-----------------------------------------------------------------------------
vtkIdType vtkExodusIIReaderParser::AddChildEdgeToSIL(vtkIdType src, vtkIdType dst)
{
  vtkIdType id = this->SIL->AddEdge(src, dst).Id;
  this->CrossEdgesArray->InsertValue(id, 0);
  return id;
}

//-----------------------------------------------------------------------------
vtkIdType vtkExodusIIReaderParser::AddCrossEdgeToSIL(vtkIdType src, vtkIdType dst)
{
  vtkIdType id = this->SIL->AddEdge(src, dst).Id;
  this->CrossEdgesArray->InsertValue(id, 1);
  return id;
}

//-----------------------------------------------------------------------------
vtkIdType vtkExodusIIReaderParser::GetPartVertex(const char* part_number_instance_string)
{
  vtkstd::map<vtkstd::string, vtkIdType>::iterator iter = 
    this->Part_To_VertexID.find(part_number_instance_string);
  if (iter != this->Part_To_VertexID.end())
    {
    return iter->second;
    }

  // The name here is temporary. The full name for a the "part" nodes is
  // determined when the assembly is parsed.
  vtkIdType vertex = this->AddVertexToSIL(part_number_instance_string);
  // Save the vertex for later use.
  this->Part_To_VertexID[part_number_instance_string] = vertex;
  this->PartVertexID_To_PartNumberInstanceString[vertex] = part_number_instance_string;
  return vertex;
}

//-----------------------------------------------------------------------------
void vtkExodusIIReaderParser::Go(const char* filename)
{
  this->SIL->Initialize();
  this->CurrentVertex.clear();
  this->BlockID_To_VertexID.clear();
  this->BlockID_To_MaterialName.clear();
  this->MaterialName_To_VertexID.clear();
  this->PartVertexID_To_Descriptions.clear();
  this->Part_To_VertexID.clear();
  this->PartVertexID_To_PartNumberInstanceString.clear();
  this->MaterialSpecifications.clear();
  this->BlockID_To_PartVertexID.clear();
  this->InBlocks = false;
  this->InMaterialAssignments = false;

  this->NamesArray = vtkSmartPointer<vtkStringArray>::New();
  this->NamesArray->SetName("Names");
  this->CrossEdgesArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
  this->CrossEdgesArray->SetName("CrossEdges");
  this->SIL->GetVertexData()->AddArray(this->NamesArray);
  this->SIL->GetEdgeData()->AddArray(this->CrossEdgesArray);

  this->RootVertex = this->AddVertexToSIL("SIL");
  this->BlocksVertex = this->AddVertexToSIL("Blocks");
  this->AssembliesVertex = this->AddVertexToSIL("Assemblies");
  this->MaterialsVertex = this->AddVertexToSIL("Materials");
  this->AddChildEdgeToSIL(this->RootVertex, this->BlocksVertex);
  this->AddChildEdgeToSIL(this->RootVertex, this->AssembliesVertex);
  this->AddChildEdgeToSIL(this->RootVertex, this->MaterialsVertex);

  this->SetFileName(filename);
  this->Parse();
  this->FinishedParsing();
}

//-----------------------------------------------------------------------------
vtkstd::string vtkExodusIIReaderParser::GetBlockName(int id)
{
  if (this->BlockID_To_VertexID.find(id) != this->BlockID_To_VertexID.end())
    {
    vtkIdType vertex = this->BlockID_To_VertexID[id];
    return this->NamesArray->GetValue(vertex);
    }
  return "";
}

//-----------------------------------------------------------------------------
void vtkExodusIIReaderParser::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SIL: " << this->SIL << endl;
}

