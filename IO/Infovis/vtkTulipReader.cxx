/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTulipReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkTulipReader.h"

#include "vtkAnnotation.h"
#include "vtkAnnotationLayers.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkVariantArray.h"

#include <cassert>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <stack>
#include <vector>
#include <cctype>

// I need a safe way to read a line of arbitrary length.  It exists on
// some platforms but not others so I'm afraid I have to write it
// myself.
// This function is also defined in Infovis/vtkDelimitedTextReader.cxx,
// so it would be nice to put this in a common file.
static int my_getline(std::istream& stream, vtkStdString &output, char delim='\n');

vtkStandardNewMacro(vtkTulipReader);

vtkTulipReader::vtkTulipReader()
{
  // Default values for the origin vertex
  this->FileName = 0;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(2);
}

vtkTulipReader::~vtkTulipReader()
{
  this->SetFileName(0);
}

void vtkTulipReader::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: "
     << (this->FileName ? this->FileName : "(none)") << endl;
}

int vtkTulipReader::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUndirectedGraph");
  }
  else if (port == 1)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkAnnotationLayers");
  }
  else
  {
    return 0;
  }
  return 1;
}

struct vtkTulipReaderCluster
{
  int clusterId;
  int parentId;
  static const int NO_PARENT = -1;
  vtkStdString name;
  vtkSmartPointer<vtkIdTypeArray> nodes;
};

struct vtkTulipReaderToken
{
  enum {
    OPEN_PAREN,
    CLOSE_PAREN,
    KEYWORD,
    INT,
    DOUBLE,
    TEXT,
    END_OF_FILE
  };
  int Type;
  vtkStdString StringValue;
  int IntValue;
  double DoubleValue;
};

static void vtkTulipReaderNextToken(std::istream& in, vtkTulipReaderToken& tok)
{
  char ch = in.peek();
  while (!in.eof() && (ch == ';' || isspace(ch)))
  {
    while (!in.eof() && ch == ';')
    {
      vtkStdString comment;
      my_getline(in, comment);
      ch = in.peek();
    }
    while (!in.eof() && isspace(ch))
    {
      in.get();
      ch = in.peek();
    }
  }

  if (in.eof())
  {
    tok.Type = vtkTulipReaderToken::END_OF_FILE;
    return;
  }
  if (ch == '(')
  {
    in.get();
    tok.Type = vtkTulipReaderToken::OPEN_PAREN;
  }
  else if (ch == ')')
  {
    in.get();
    tok.Type = vtkTulipReaderToken::CLOSE_PAREN;
  }
  else if (isdigit(ch) || ch == '.')
  {
    bool isDouble = false;
    std::stringstream ss;
    while (isdigit(ch) || ch == '.')
    {
      in.get();
      isDouble = isDouble || ch == '.';
      ss << ch;
      ch = in.peek();
    }
    if (isDouble)
    {
      ss >> tok.DoubleValue;
      tok.Type = vtkTulipReaderToken::DOUBLE;
    }
    else
    {
      ss >> tok.IntValue;
      tok.Type = vtkTulipReaderToken::INT;
    }
  }
  else if (ch == '"')
  {
    in.get();
    tok.StringValue = "";
    ch = in.get();
    while (ch != '"')
    {
      tok.StringValue += ch;
      ch = in.get();
    }
    tok.Type = vtkTulipReaderToken::TEXT;
  }
  else
  {
    in >> tok.StringValue;
    tok.Type = vtkTulipReaderToken::KEYWORD;
  }
}

int vtkTulipReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  if (this->FileName == NULL)
  {
    vtkErrorMacro("File name undefined");
    return 0;
  }

  std::ifstream fin(this->FileName);
  if(!fin.is_open())
  {
    vtkErrorMacro("Could not open file " << this->FileName << ".");
    return 0;
  }

  // Get the output graph
  vtkSmartPointer<vtkMutableUndirectedGraph> builder =
    vtkSmartPointer<vtkMutableUndirectedGraph>::New();

  // An array for vertex pedigree ids.
  vtkVariantArray* vertexPedigrees = vtkVariantArray::New();
  vertexPedigrees->SetName("id");
  builder->GetVertexData()->SetPedigreeIds(vertexPedigrees);
  vertexPedigrees->Delete();

  // An array for edge ids.
  vtkVariantArray* edgePedigrees = vtkVariantArray::New();
  edgePedigrees->SetName("id");

  // Structures to record cluster hierarchy - all vertices belong to cluster 0.
  std::vector<vtkTulipReaderCluster> clusters;
  vtkTulipReaderCluster root;
  root.clusterId = 0;
  root.parentId = vtkTulipReaderCluster::NO_PARENT;
  root.name = "<default>";
  root.nodes = vtkSmartPointer<vtkIdTypeArray>::New();

  std::stack<int> parentage;
  parentage.push(root.clusterId);
  clusters.push_back(root);

  std::map<int, vtkIdType> nodeIdMap;
  std::map<int, vtkIdType> edgeIdMap;
  vtkTulipReaderToken tok;
  vtkTulipReaderNextToken(fin, tok);
  while (tok.Type == vtkTulipReaderToken::OPEN_PAREN)
  {
    vtkTulipReaderNextToken(fin, tok);
    assert(tok.Type == vtkTulipReaderToken::KEYWORD);
    if (tok.StringValue == "nodes")
    {
      vtkTulipReaderNextToken(fin, tok);
      while (tok.Type != vtkTulipReaderToken::CLOSE_PAREN)
      {
        assert(tok.Type == vtkTulipReaderToken::INT);
        vtkIdType id = builder->AddVertex(tok.IntValue);
        nodeIdMap[tok.IntValue] = id;
        vtkTulipReaderNextToken(fin, tok);
      }
    }
    else if (tok.StringValue == "edge")
    {
      vtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == vtkTulipReaderToken::INT);
      int tulipId = tok.IntValue;
      vtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == vtkTulipReaderToken::INT);
      int source = tok.IntValue;
      vtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == vtkTulipReaderToken::INT);
      int target = tok.IntValue;

      vtkEdgeType e = builder->AddEdge(nodeIdMap[source], nodeIdMap[target]);
      edgeIdMap[tulipId] = e.Id;
      edgePedigrees->InsertValue(e.Id, tulipId);

      vtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == vtkTulipReaderToken::CLOSE_PAREN);
    }
    else if (tok.StringValue == "cluster")
    {
      // Cluster preamble
      vtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == vtkTulipReaderToken::INT);
      int clusterId = tok.IntValue;
      vtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == vtkTulipReaderToken::TEXT);
      vtkStdString clusterName = tok.StringValue;

      vtkTulipReaderCluster newCluster;
      newCluster.clusterId = clusterId;
      newCluster.parentId = parentage.top();
      newCluster.name = clusterName;
      newCluster.nodes = vtkSmartPointer<vtkIdTypeArray>::New();
      parentage.push(clusterId);

      // Cluster nodes
      vtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == vtkTulipReaderToken::OPEN_PAREN);

      vtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == vtkTulipReaderToken::KEYWORD);
      assert(tok.StringValue == "nodes");

      vtkTulipReaderNextToken(fin, tok);
      while (tok.Type != vtkTulipReaderToken::CLOSE_PAREN)
      {
        assert(tok.Type == vtkTulipReaderToken::INT);
        newCluster.nodes->InsertNextValue(nodeIdMap[tok.IntValue]);
        vtkTulipReaderNextToken(fin, tok);
      }

      // Cluster edges - currently ignoring these...
      vtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == vtkTulipReaderToken::OPEN_PAREN);

      vtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == vtkTulipReaderToken::KEYWORD);
      assert(tok.StringValue == "edges");

      vtkTulipReaderNextToken(fin, tok);
      while (tok.Type != vtkTulipReaderToken::CLOSE_PAREN)
      {
        assert(tok.Type == vtkTulipReaderToken::INT);
        vtkTulipReaderNextToken(fin, tok);
      }
      clusters.push_back(newCluster);

      // End of cluster(s)
      vtkTulipReaderNextToken(fin, tok);
      while (tok.Type == vtkTulipReaderToken::CLOSE_PAREN)
      {
        parentage.pop();
        vtkTulipReaderNextToken(fin, tok);
      }
      continue;
    }
    else if (tok.StringValue == "property")
    {
      vtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == vtkTulipReaderToken::INT);
      //int clusterId = tok.IntValue;
      // We only read properties for cluster 0 (the whole graph).

      vtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == vtkTulipReaderToken::KEYWORD);
      vtkStdString type = tok.StringValue;

      vtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == vtkTulipReaderToken::TEXT);
      vtkStdString name = tok.StringValue;

      // The existing types are the following
      // bool : This type is used to store boolean on elements.
      // color : This type is used to store the color of elements.
      //   The color is defined with a sequence of four integer from 0 to 255.
      //   "(red,green,blue,alpha)"
      // double : This is used to store 64 bits real on elements.
      // layout : This type is used to store 3D nodes position.
      //   The position of nodes is defined by a set of 3 doubles
      //   "(x_coord,y_coord,z_coord)".
      //   The position of edges is a list of 3D points.
      //   These points are the bends of edges.
      //   "((x_coord1,y_coord1,z_coord1)(x_coord2,y_coord2,z_coord2))"
      // int : This type is used to store integers on elements.
      // size : This type is used to store the size of elements.
      //   The size is defined with a sequence of three double.
      //   "(width,heigth,depth)"
      // string : This is used to store text on elements.

      if (type == "string")
      {
        vtkStringArray* vertArr = vtkStringArray::New();
        vertArr->SetName(name.c_str());

        vtkStringArray* edgeArr = vtkStringArray::New();
        edgeArr->SetName(name.c_str());

        vtkTulipReaderNextToken(fin, tok);
        while (tok.Type != vtkTulipReaderToken::CLOSE_PAREN)
        {
          assert(tok.Type == vtkTulipReaderToken::OPEN_PAREN);
          vtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == vtkTulipReaderToken::KEYWORD);
          vtkStdString key = tok.StringValue;
          vtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == vtkTulipReaderToken::TEXT || tok.Type == vtkTulipReaderToken::INT);
          int id = tok.IntValue;
          vtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == vtkTulipReaderToken::TEXT);
          vtkStdString value = tok.StringValue;
          vtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == vtkTulipReaderToken::CLOSE_PAREN);
          vtkTulipReaderNextToken(fin, tok);

          if (key == "node")
          {
            vertArr->InsertValue(nodeIdMap[id], value);
          }
          else if (key == "edge")
          {
            edgeArr->InsertValue(edgeIdMap[id], value);
          }
        }

        if (static_cast<size_t>(vertArr->GetNumberOfValues()) == nodeIdMap.size())
        {
          builder->GetVertexData()->AddArray(vertArr);
        }
        vertArr->Delete();
        if (static_cast<size_t>(edgeArr->GetNumberOfValues()) == edgeIdMap.size())
        {
          builder->GetEdgeData()->AddArray(edgeArr);
        }
        edgeArr->Delete();
      }
      else if (type == "int")
      {
        vtkIntArray* vertArr = vtkIntArray::New();
        vertArr->SetName(name.c_str());

        vtkIntArray* edgeArr = vtkIntArray::New();
        edgeArr->SetName(name.c_str());

        vtkTulipReaderNextToken(fin, tok);
        while (tok.Type != vtkTulipReaderToken::CLOSE_PAREN)
        {
          assert(tok.Type == vtkTulipReaderToken::OPEN_PAREN);
          vtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == vtkTulipReaderToken::KEYWORD);
          vtkStdString key = tok.StringValue;
          vtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == vtkTulipReaderToken::TEXT ||
            tok.Type == vtkTulipReaderToken::INT);
          int id = tok.IntValue;
          vtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == vtkTulipReaderToken::TEXT);
          std::stringstream ss;
          int value;
          ss << tok.StringValue;
          ss >> value;
          assert(!ss.fail());
          vtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == vtkTulipReaderToken::CLOSE_PAREN);
          vtkTulipReaderNextToken(fin, tok);

          if (key == "node")
          {
            vertArr->InsertValue(nodeIdMap[id], value);
          }
          else if (key == "edge")
          {
            edgeArr->InsertValue(edgeIdMap[id], value);
          }
        }

        if (static_cast<size_t>(vertArr->GetNumberOfTuples()) == nodeIdMap.size())
        {
          builder->GetVertexData()->AddArray(vertArr);
        }
        vertArr->Delete();
        if (static_cast<size_t>(edgeArr->GetNumberOfTuples()) == edgeIdMap.size())
        {
          builder->GetEdgeData()->AddArray(edgeArr);
        }
        edgeArr->Delete();
      }
      else if (type == "double")
      {
        vtkDoubleArray* vertArr = vtkDoubleArray::New();
        vertArr->SetName(name.c_str());

        vtkDoubleArray* edgeArr = vtkDoubleArray::New();
        edgeArr->SetName(name.c_str());

        vtkTulipReaderNextToken(fin, tok);
        while (tok.Type != vtkTulipReaderToken::CLOSE_PAREN)
        {
          assert(tok.Type == vtkTulipReaderToken::OPEN_PAREN);
          vtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == vtkTulipReaderToken::KEYWORD);
          vtkStdString key = tok.StringValue;
          vtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == vtkTulipReaderToken::TEXT ||
            tok.Type == vtkTulipReaderToken::INT);
          int id = tok.IntValue;
          vtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == vtkTulipReaderToken::TEXT);
          std::stringstream ss;
          double value;
          ss << tok.StringValue;
          ss >> value;
          assert(!ss.fail());
          vtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == vtkTulipReaderToken::CLOSE_PAREN);
          vtkTulipReaderNextToken(fin, tok);

          if (key == "node")
          {
            vertArr->InsertValue(nodeIdMap[id], value);
          }
          else if (key == "edge")
          {
            edgeArr->InsertValue(edgeIdMap[id], value);
          }
        }

        if (static_cast<size_t>(vertArr->GetNumberOfTuples()) == nodeIdMap.size())
        {
          builder->GetVertexData()->AddArray(vertArr);
        }
        vertArr->Delete();
        if (static_cast<size_t>(edgeArr->GetNumberOfTuples()) == edgeIdMap.size())
        {
          builder->GetEdgeData()->AddArray(edgeArr);
        }
        edgeArr->Delete();
      }
      else // Remaining properties are ignored.
      {
        vtkTulipReaderNextToken(fin, tok);
        while (tok.Type != vtkTulipReaderToken::CLOSE_PAREN)
        {
          assert(tok.Type == vtkTulipReaderToken::OPEN_PAREN);
          vtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == vtkTulipReaderToken::KEYWORD);
          vtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == vtkTulipReaderToken::TEXT || tok.Type == vtkTulipReaderToken::INT);
          vtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == vtkTulipReaderToken::TEXT);
          vtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == vtkTulipReaderToken::CLOSE_PAREN);
          vtkTulipReaderNextToken(fin, tok);
        }
      }
    }
    else if (tok.StringValue == "displaying")
    {
      vtkTulipReaderNextToken(fin, tok);
      while (tok.Type != vtkTulipReaderToken::CLOSE_PAREN)
      {
        assert(tok.Type == vtkTulipReaderToken::OPEN_PAREN);
        while (tok.Type != vtkTulipReaderToken::CLOSE_PAREN)
        {
          vtkTulipReaderNextToken(fin, tok);
        }
        vtkTulipReaderNextToken(fin, tok);
      }
    }

    vtkTulipReaderNextToken(fin, tok);
  }
  assert(parentage.size() == 1);

  // Clean up
  fin.close();

  builder->GetEdgeData()->SetPedigreeIds(edgePedigrees);
  edgePedigrees->Delete();

  // Move graph structure to output
  vtkGraph* output = vtkGraph::GetData(outputVector);
  if (!output->CheckedShallowCopy(builder))
  {
    vtkErrorMacro(<<"Invalid graph structure.");
    return 0;
  }

  // Create annotation layers output.
  vtkSmartPointer<vtkAnnotationLayers> annotationLayers =
    vtkSmartPointer<vtkAnnotationLayers>::New();

  // Determine list of unique cluster names.
  std::set<vtkStdString> uniqueLabels;
  for (size_t i = 0; i < clusters.size(); ++i)
  {
    uniqueLabels.insert(clusters.at(i).name);
  }

  // Create annotations.
  std::set<vtkStdString>::iterator labels = uniqueLabels.begin();
  for (; labels != uniqueLabels.end(); ++labels)
  {
    vtkSmartPointer<vtkAnnotation> annotation = vtkSmartPointer<vtkAnnotation>::New();
    annotation->GetInformation()->Set(vtkAnnotation::COLOR(), 0.0, 0.0, 1.0);
    annotation->GetInformation()->Set(vtkAnnotation::OPACITY(), 0.5);
    annotation->GetInformation()->Set(vtkAnnotation::LABEL(), labels->c_str());
    annotation->GetInformation()->Set(vtkAnnotation::ENABLE(), 1);

    vtkSmartPointer<vtkSelection> selection =
      vtkSmartPointer<vtkSelection>::New();
    for (size_t i = 0; i < clusters.size(); ++i)
    {
      if (clusters.at(i).name.compare(labels->c_str()) == 0)
      {
        vtkSelectionNode* selectionNode = vtkSelectionNode::New();
        selectionNode->SetFieldType(vtkSelectionNode::VERTEX);
        selectionNode->SetContentType(vtkSelectionNode::INDICES);
        selectionNode->SetSelectionList(clusters.at(i).nodes);
        selection->AddNode(selectionNode);
        selectionNode->Delete();
      }
    }
    annotation->SetSelection(selection);
    annotationLayers->AddAnnotation(annotation);
  }

  // Copy annotations to output port 1
  vtkInformation* info1 = outputVector->GetInformationObject(1);
  vtkAnnotationLayers* output1 = vtkAnnotationLayers::GetData(info1);
  output1->ShallowCopy(annotationLayers);

  return 1;
}

static int
my_getline(std::istream& in, vtkStdString &out, char delimiter)
{
  out = vtkStdString();
  unsigned int numCharactersRead = 0;
  int nextValue = 0;

  while ((nextValue = in.get()) != EOF &&
         numCharactersRead < out.max_size())
  {
    ++numCharactersRead;

    char downcast = static_cast<char>(nextValue);
    if (downcast != delimiter)
    {
      out += downcast;
    }
    else
    {
      return numCharactersRead;
    }
  }

  return numCharactersRead;
}


