/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkXGMLReader.cxx

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

#include "vtkXGMLReader.h"

#include "vtkCellData.h"
#include "vtkIntArray.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkAbstractArray.h"
#include "vtkIdTypeArray.h"


#include <cassert>
#include <fstream>
#include <sstream>
#include <map>

#if defined (__BORLANDC__)
#include <cctype> // for isspace, isdigit
#endif

// Copied from vtkTulipReader.cxx ..
static int my_getline(std::istream& stream, vtkStdString &output, char delim='\n');

vtkStandardNewMacro(vtkXGMLReader);

vtkXGMLReader::vtkXGMLReader()
{
  // Default values for the origin vertex
  this->FileName = 0;
  this->SetNumberOfInputPorts(0);
}

vtkXGMLReader::~vtkXGMLReader()
{
  this->SetFileName(0);
}

void vtkXGMLReader::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: "
     << (this->FileName ? this->FileName : "(none)") << endl;
}

#define MAX_NR_PROPERTIES 50

struct vtkXGMLProperty
{
  enum {
    NODE_PROP,
    EDGE_PROP
  };
  int Kind;  // :: NODE_PROP or EDGE_PROP
  vtkAbstractArray *Data;
};

struct vtkXGMLReaderToken
{
  enum {
    OPEN_GROUP,
    CLOSE_GROUP,
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



static void vtkXGMLReaderNextToken(std::istream& in, vtkXGMLReaderToken& tok)
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
    tok.Type = vtkXGMLReaderToken::END_OF_FILE;
    return;
  }
  if (ch == '[')
  {
    in.get();
    tok.Type = vtkXGMLReaderToken::OPEN_GROUP;
  }
  else if (ch == ']')
  {
    in.get();
    tok.Type = vtkXGMLReaderToken::CLOSE_GROUP;
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
      tok.Type = vtkXGMLReaderToken::DOUBLE;
    }
    else
    {
      ss >> tok.IntValue;
      tok.Type = vtkXGMLReaderToken::INT;
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
    tok.Type = vtkXGMLReaderToken::TEXT;
  }
  else
  {
    in >> tok.StringValue;
    tok.Type = vtkXGMLReaderToken::KEYWORD;
  }
}

int vtkXGMLReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkIdType nr_of_nodes = 0;       // as read from file
  vtkIdType nr_of_edges = 0;       // as read from file
  int nr_of_properties = 0;
  vtkXGMLProperty property_table[MAX_NR_PROPERTIES];
  vtkStdString name;
  int kind;
  int i;
  vtkIdType dst, id = 0, src = 0;
  double d = 0.;
  vtkIdTypeArray *edgeIds, *nodeIds;


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

  std::map<int, vtkIdType> nodeIdMap;
  std::map<int, vtkIdType> edgeIdMap;
  vtkXGMLReaderToken tok;

  // expect graph
  vtkXGMLReaderNextToken(fin, tok);
  assert(tok.Type == vtkXGMLReaderToken::KEYWORD && tok.StringValue == "graph");

  // expect [
  vtkXGMLReaderNextToken(fin, tok);
  assert(tok.Type == vtkXGMLReaderToken::OPEN_GROUP);

  vtkXGMLReaderNextToken(fin, tok);
  while (tok.Type == vtkXGMLReaderToken::KEYWORD && tok.StringValue != "node")
  {
    if (tok.StringValue == "node_count")
    {
      vtkXGMLReaderNextToken(fin, tok);
      assert(tok.Type == vtkXGMLReaderToken::INT);
      nr_of_nodes = tok.IntValue;
    }
    else if (tok.StringValue == "edge_count")
    {
      vtkXGMLReaderNextToken(fin, tok);
      assert(tok.Type == vtkXGMLReaderToken::INT);
      nr_of_edges = tok.IntValue;
    }
    else if (tok.StringValue == "node_data" || tok.StringValue == "edge_data")
    {
      if (nr_of_properties == MAX_NR_PROPERTIES)
      {
        vtkErrorMacro(<<"Too many properties in file.");
        return 0;
      }
      kind = (tok.StringValue == "node_data") ? vtkXGMLProperty::NODE_PROP
        : vtkXGMLProperty::EDGE_PROP;
      vtkXGMLReaderNextToken(fin, tok);
      assert(tok.Type == vtkXGMLReaderToken::KEYWORD);
      name = tok.StringValue;

      vtkXGMLReaderNextToken(fin, tok);
      assert(tok.Type == vtkXGMLReaderToken::KEYWORD);
      if (tok.StringValue == "float")
      {
        property_table[nr_of_properties].Data = vtkDoubleArray::New();
      }
      else if (tok.StringValue == "int")
      {
        property_table[nr_of_properties].Data = vtkIntArray::New();
      }
      else if (tok.StringValue == "string")
      {
        property_table[nr_of_properties].Data = vtkStringArray::New();
      }
      property_table[nr_of_properties].Kind = kind;
      property_table[nr_of_properties].Data->SetName(name);
      property_table[nr_of_properties].Data->SetNumberOfTuples(
        kind == vtkXGMLProperty::NODE_PROP ? nr_of_nodes : nr_of_edges);
      nr_of_properties++;
    }
    else
    {
      vtkErrorMacro(<<"Parse error (header): unexpected token ")
        return 0;
    }
    vtkXGMLReaderNextToken(fin, tok);
  }

  while (tok.Type == vtkXGMLReaderToken::KEYWORD && tok.StringValue == "node")
  {
    // Expect [
    vtkXGMLReaderNextToken(fin, tok);
    assert(tok.Type == vtkXGMLReaderToken::OPEN_GROUP);

    vtkXGMLReaderNextToken(fin, tok);
    while (tok.Type == vtkXGMLReaderToken::KEYWORD)
    {
      if (tok.StringValue == "id")
      {
        vtkXGMLReaderNextToken(fin, tok);
        assert(tok.Type == vtkXGMLReaderToken::INT);
        id = builder->AddVertex();
        nodeIdMap[tok.IntValue] = id;
      }
      else if (tok.StringValue == "degree")
      {
        vtkXGMLReaderNextToken(fin, tok);
        // Read degree and ignore
      }
      else
      {
        for (i = 0; i < nr_of_properties; i++)
        {
          if (property_table[i].Kind == vtkXGMLProperty::NODE_PROP &&
              property_table[i].Data->GetName() == tok.StringValue) { break; }
        }
        if (i == nr_of_properties)
        {
          vtkErrorMacro(<<"Undefined node property ");
          cout << tok.StringValue<<"\n";
          return 0;
        }
        vtkXGMLReaderNextToken(fin, tok);
        if (property_table[i].Data->GetDataType() == VTK_INT)
        {
          assert(tok.Type == vtkXGMLReaderToken::INT);
          vtkArrayDownCast<vtkIntArray>(property_table[i].Data)->SetValue(nodeIdMap[id], tok.IntValue);
        }
        else if (property_table[i].Data->GetDataType() == VTK_DOUBLE)
        {
          if (tok.Type == vtkXGMLReaderToken::DOUBLE)
            d = tok.DoubleValue;
          else if (tok.Type == vtkXGMLReaderToken::INT)
            d = (double)tok.IntValue;
          else
            vtkErrorMacro(<<"Expected double or int.\n");
          vtkArrayDownCast<vtkDoubleArray>(property_table[i].Data)->SetValue(nodeIdMap[id], d);
        }
        else
        {
          assert(tok.Type == vtkXGMLReaderToken::TEXT);
          vtkArrayDownCast<vtkStringArray>(property_table[i].Data)->SetValue(nodeIdMap[id], tok.StringValue);
        }
      }
      vtkXGMLReaderNextToken(fin, tok);
    }
    assert(tok.Type == vtkXGMLReaderToken::CLOSE_GROUP);
    vtkXGMLReaderNextToken(fin, tok);
  }

  while (tok.Type == vtkXGMLReaderToken::KEYWORD && tok.StringValue == "edge")
  {
    // Expect [
    vtkXGMLReaderNextToken(fin, tok);
    assert(tok.Type == vtkXGMLReaderToken::OPEN_GROUP);

    vtkXGMLReaderNextToken(fin, tok);
    while (tok.Type == vtkXGMLReaderToken::KEYWORD)
    {
      // Assume that all edge groups will list id, source, and dest fields
      // before any edge property.
      if (tok.StringValue == "id")
      {
        vtkXGMLReaderNextToken(fin, tok);
        assert(tok.Type == vtkXGMLReaderToken::INT);
        id = tok.IntValue;
      }
      else if (tok.StringValue == "source")
      {
        vtkXGMLReaderNextToken(fin, tok);
        assert(tok.Type == vtkXGMLReaderToken::INT);
        src = tok.IntValue;
      }
      else if (tok.StringValue == "target")
      {
        vtkXGMLReaderNextToken(fin, tok);
        assert(tok.Type == vtkXGMLReaderToken::INT);
        dst = tok.IntValue;
        vtkEdgeType e = builder->AddEdge(nodeIdMap[src], nodeIdMap[dst]);
        edgeIdMap[id] = e.Id;
      }
      else
      {
        for (i = 0; i < nr_of_properties; i++)
        {
          if (property_table[i].Kind == vtkXGMLProperty::EDGE_PROP &&
              property_table[i].Data->GetName() == tok.StringValue) { break; }
        }
        if (i == nr_of_properties)
        {
          vtkErrorMacro(<<"Undefined node property ");
          return 0;
        }
        vtkXGMLReaderNextToken(fin, tok);
        if (property_table[i].Data->GetDataType() == VTK_INT)
        {
          assert(tok.Type == vtkXGMLReaderToken::INT);
          vtkArrayDownCast<vtkIntArray>(property_table[i].Data)->SetValue(edgeIdMap[id], tok.IntValue);
        }
        else if (property_table[i].Data->GetDataType() == VTK_DOUBLE)
        {
          if (tok.Type == vtkXGMLReaderToken::DOUBLE)
            d = tok.DoubleValue;
          else if (tok.Type == vtkXGMLReaderToken::INT)
            d = (double)tok.IntValue;
          else
            vtkErrorMacro(<<"Expected double or int.\n");
          vtkArrayDownCast<vtkDoubleArray>(property_table[i].Data)->SetValue(nodeIdMap[id], d);
        }
        else
        {
          assert(tok.Type == vtkXGMLReaderToken::TEXT);
          vtkArrayDownCast<vtkStringArray>(property_table[i].Data)->SetValue(edgeIdMap[id], tok.StringValue);
        }
      }
      vtkXGMLReaderNextToken(fin, tok);
    }
    assert(tok.Type == vtkXGMLReaderToken::CLOSE_GROUP);
    vtkXGMLReaderNextToken(fin, tok);
  }

  // Should now recognise the end of graph group ..
  assert(tok.Type == vtkXGMLReaderToken::CLOSE_GROUP);

  // .. followed by end-of-file.
  vtkXGMLReaderNextToken(fin, tok);
  // do an extra read
  vtkXGMLReaderNextToken(fin, tok);
  assert(tok.Type == vtkXGMLReaderToken::END_OF_FILE);

  // Clean up
  fin.close();

  for (i = 0; i < nr_of_properties; i++)
  {
    if (property_table[i].Kind == vtkXGMLProperty::NODE_PROP)
    {
      builder->GetVertexData()->AddArray(property_table[i].Data);
      property_table[i].Data->Delete();
    }
    else
    {
      builder->GetEdgeData()->AddArray(property_table[i].Data);
      property_table[i].Data->Delete();
    }
  }
  vtkFloatArray *weights = vtkFloatArray::New();
  weights->SetName("edge weight");
  weights->SetNumberOfTuples(nr_of_edges);
  edgeIds = vtkIdTypeArray::New();
  edgeIds->SetName("edge id");
  edgeIds->SetNumberOfTuples(nr_of_edges);
  for (i = 0; i < nr_of_edges; i++)
  {
    weights->SetValue(i,1.0);
    edgeIds->SetValue(i,i);
  }

  nodeIds = vtkIdTypeArray::New();
  nodeIds->SetName("vertex id");
  nodeIds->SetNumberOfTuples(nr_of_nodes);
  for (i = 0; i < nr_of_nodes; i++)
  {
    nodeIds->SetValue(i,i);
  }
  builder->GetEdgeData()->AddArray(weights);
  builder->GetEdgeData()->SetPedigreeIds(edgeIds);
  builder->GetVertexData()->SetPedigreeIds(nodeIds);
  weights->Delete();
  nodeIds->Delete();
  edgeIds->Delete();
  // Move structure to output
  vtkGraph* output = vtkGraph::GetData(outputVector);
  if (!output->CheckedShallowCopy(builder))
  {
    vtkErrorMacro(<<"Invalid graph strucutre.");
    return 0;
  }

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
