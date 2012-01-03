/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQtXMLProcessor.cxx
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkIdTypeArray.h>
//#include <vtkQtInitialization.h>
#include <vtkQtXMLProcessor.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkUnicodeString.h>

#include <QApplication>

#include <vtksys/ios/iostream>
#include <vtksys/ios/sstream>
#include <stdexcept>

#define test_expression(expression) \
{ \
  if(!(expression)) \
    { \
    std::ostringstream buffer; \
    buffer << "Expression failed at line " << __LINE__ << ": " << #expression; \
    throw std::runtime_error(buffer.str()); \
    } \
}

int TestQtXMLProcessor(int argc, char* argv[])
{
  try
    {
    QApplication app(argc, argv);

    vtkSmartPointer<vtkTable> table = vtkSmartPointer<vtkTable>::New();

    vtkIdTypeArray* const document_id = vtkIdTypeArray::New();
    document_id->SetName("document_id");
    document_id->InsertNextValue(0);
    document_id->InsertNextValue(1);
    document_id->InsertNextValue(2);
    table->AddColumn(document_id);
    document_id->Delete();

    vtkStringArray* const uri = vtkStringArray::New();
    uri->SetName("document_uri");
    uri->InsertNextValue("file:///home/bob/1234 32.txt");
    uri->InsertNextValue("file:///home/bob/A75 67.txt");
    uri->InsertNextValue("file:///home/bob/stuff/9944100.txt");
    table->AddColumn(uri);
    uri->Delete();

    vtkSmartPointer<vtkQtXMLProcessor> document_label = vtkSmartPointer<vtkQtXMLProcessor>::New();
    document_label->SetInputConnection(0, table->GetProducerPort());
    document_label->SetFieldType(vtkDataObject::ROW);
    document_label->MapArrayName("document_id", "document");
    document_label->SetInputDomain(vtkQtXMLProcessor::ROW_DOMAIN);
    document_label->SetQueryType(vtkQtXMLProcessor::XQUERY);
    document_label->SetQuery(
      "concat('Document ', data(/table/rows/row/document))"
      );
    document_label->SetOutputArray("document_label");

    vtkSmartPointer<vtkQtXMLProcessor> file_label = vtkSmartPointer<vtkQtXMLProcessor>::New();
    file_label->SetInputConnection(0, document_label->GetOutputPort());
    file_label->SetFieldType(vtkDataObject::ROW);
    file_label->MapArrayName("document_uri", "uri");
    file_label->SetInputDomain(vtkQtXMLProcessor::ROW_DOMAIN);
    file_label->SetQueryType(vtkQtXMLProcessor::XQUERY);
    file_label->SetQuery(
      "replace(data(/table/rows/row/uri),'file:.*/','')"
      );
    file_label->SetOutputArray("file_label");

    vtkSmartPointer<vtkQtXMLProcessor> bibliography = vtkSmartPointer<vtkQtXMLProcessor>::New();
    bibliography->SetInputConnection(0, table->GetProducerPort());
    bibliography->SetFieldType(vtkDataObject::ROW);
    bibliography->MapArrayName("document_uri", "file");
    bibliography->SetInputDomain(vtkQtXMLProcessor::DATA_OBJECT_DOMAIN);
    bibliography->SetQueryType(vtkQtXMLProcessor::XQUERY);
    bibliography->SetQuery(
      "<html>\n"
      "<body>\n"
      "<ul>\n"
      "{\n"
      "for $x in /table/rows/row/file\n"
      "return <li>{(data($x))}</li>\n"
      "}\n"
      "</ul>"
      "</body>"
      "</html>"
      );
    bibliography->SetOutputArray("bibliography");

    vtkSmartPointer<vtkQtXMLProcessor> tree = vtkSmartPointer<vtkQtXMLProcessor>::New();
    tree->SetInputConnection(0, table->GetProducerPort());
    tree->SetFieldType(vtkDataObject::ROW);
    tree->SetInputDomain(vtkQtXMLProcessor::DATA_OBJECT_DOMAIN);
    tree->SetQueryType(vtkQtXMLProcessor::XQUERY);
    tree->SetQuery(
      "<html>\n"
      "<body>\n"
      "<ul>\n"
      "{\n"
      "for $x in //*\n"
      "return <li>{(node-name($x))}</li>\n"
      "}\n"
      "</ul>"
      "</body>"
      "</html>"
      );
    tree->SetOutputArray("tree");


    cout << "Source data: \n";
    table->Dump(30);
    cout << "\n\n";

    cout << "Extra labels: \n";
    file_label->Update();
    vtkTable::SafeDownCast(file_label->GetOutput(0))->Dump(30);
    cout << "\n\n";

    test_expression(vtkTable::SafeDownCast(file_label->GetOutput(0))->GetValue(0, 2).ToString() == "Document 0");
    test_expression(vtkTable::SafeDownCast(file_label->GetOutput(0))->GetValue(1, 2).ToString() == "Document 1");
    test_expression(vtkTable::SafeDownCast(file_label->GetOutput(0))->GetValue(0, 3).ToString() == "1234 32.txt");
    test_expression(vtkTable::SafeDownCast(file_label->GetOutput(0))->GetValue(1, 3).ToString() == "A75 67.txt");

    cout << "Bibliography: \n";
    bibliography->Update();
    vtkTable::SafeDownCast(bibliography->GetOutput(1))->Dump(110);
    cout << "\n\n";

    test_expression(vtkTable::SafeDownCast(bibliography->GetOutput(1))->GetValue(0, 0).ToString() == "<html><body><ul><li>file:///home/bob/1234 32.txt</li><li>file:///home/bob/A75 67.txt</li><li>file:///home/bob/stuff/9944100.txt</li></ul></body></html>");

    cout << "Tree: \n";
    tree->Update();
    cout << vtkTable::SafeDownCast(tree->GetOutput(1))->GetValue(0, 0).ToString() << "\n\n";

    return 0;
    }
  catch(std::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

