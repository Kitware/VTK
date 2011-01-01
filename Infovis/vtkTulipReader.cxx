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

#include "vtkCellData.h"
#include "vtkIntArray.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"

#include <cassert>
#include <vtksys/ios/fstream>
#include <vtksys/ios/sstream>
#include <vtksys/stl/map>

#if defined (__BORLANDC__) && (__BORLANDC__ >= 0x0560)
#include <ctype.h> // for isspace, isdigit
#endif

// I need a safe way to read a line of arbitrary length.  It exists on
// some platforms but not others so I'm afraid I have to write it
// myself.
// This function is also defined in Infovis/vtkDelimitedTextReader.cxx,
// so it would be nice to put this in a common file.
static int my_getline(vtksys_ios::istream& stream, vtkStdString &output, char delim='\n');

vtkStandardNewMacro(vtkTulipReader);

vtkTulipReader::vtkTulipReader()
{
  // Default values for the origin vertex
  this->FileName = 0;
  this->SetNumberOfInputPorts(0);
}

vtkTulipReader::~vtkTulipReader()
{
  this->SetFileName(0);
}

void vtkTulipReader::PrintSelf(vtksys_ios::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "FileName: " 
     << (this->FileName ? this->FileName : "(none)") << endl;
}

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

void vtkTulipReaderNextToken(vtksys_ios::istream& in, vtkTulipReaderToken& tok)
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
    vtksys_ios::stringstream ss;
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

  vtksys_ios::ifstream fin(this->FileName);
  if(!fin.is_open())
    {
    vtkErrorMacro("Could not open file " << this->FileName << ".");
    return 0;
    }

  // Get the output graph
  vtkSmartPointer<vtkMutableUndirectedGraph> builder =
    vtkSmartPointer<vtkMutableUndirectedGraph>::New();

  vtksys_stl::map<int, vtkIdType> nodeIdMap;
  vtksys_stl::map<int, vtkIdType> edgeIdMap;
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
        vtkIdType id = builder->AddVertex();
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

      vtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == vtkTulipReaderToken::CLOSE_PAREN);
      }
    else if (tok.StringValue == "property")
      {
      vtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == vtkTulipReaderToken::INT);
      //int clusterId = tok.IntValue;

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
        vertArr->SetNumberOfValues(nodeIdMap.size());
        builder->GetVertexData()->AddArray(vertArr);
        vertArr->Delete();

        vtkStringArray* edgeArr = vtkStringArray::New();
        edgeArr->SetName(name.c_str());
        edgeArr->SetNumberOfValues(edgeIdMap.size());
        builder->GetEdgeData()->AddArray(edgeArr);
        edgeArr->Delete();

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
            vertArr->SetValue(nodeIdMap[id], value);
            }
          else if (key == "edge")
            {
            edgeArr->SetValue(edgeIdMap[id], value);
            }
          }
        }
      else if (type == "int")
        {
        vtkIntArray* vertArr = vtkIntArray::New();
        vertArr->SetName(name.c_str());
        vertArr->SetNumberOfValues(nodeIdMap.size());
        builder->GetVertexData()->AddArray(vertArr);
        vertArr->Delete();

        vtkIntArray* edgeArr = vtkIntArray::New();
        edgeArr->SetName(name.c_str());
        edgeArr->SetNumberOfValues(edgeIdMap.size());
        builder->GetEdgeData()->AddArray(edgeArr);
        edgeArr->Delete();

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
          vtksys_ios::stringstream ss;
          int value;
          ss << tok.StringValue;
          ss >> value;
          assert(!ss.fail());
          vtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == vtkTulipReaderToken::CLOSE_PAREN);
          vtkTulipReaderNextToken(fin, tok);

          if (key == "node")
            {
            vertArr->SetValue(nodeIdMap[id], value);
            }
          else if (key == "edge")
            {
            edgeArr->SetValue(edgeIdMap[id], value);
            }
          }
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

  // Clean up
  fin.close();

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
my_getline(vtksys_ios::istream& in, vtkStdString &out, char delimiter)
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


