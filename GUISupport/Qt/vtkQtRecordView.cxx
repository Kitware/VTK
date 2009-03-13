/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtRecordView.cxx

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

#include "vtkQtRecordView.h"
#include <QObject>
#include <QTextEdit>

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkCommand.h"
#include "vtkConvertSelection.h"
#include "vtkDataObjectToTable.h"
#include "vtkDataRepresentation.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkSelectionLink.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

vtkCxxRevisionMacro(vtkQtRecordView, "1.3");
vtkStandardNewMacro(vtkQtRecordView);

//----------------------------------------------------------------------------
vtkQtRecordView::vtkQtRecordView()
{
  this->TextWidget = new QTextEdit();
  this->DataObjectToTable = vtkSmartPointer<vtkDataObjectToTable>::New();
  this->FieldType = vtkQtRecordView::VERTEX_DATA;
  this->CurrentRow = 0;
  this->Text = NULL;

}

//----------------------------------------------------------------------------
vtkQtRecordView::~vtkQtRecordView()
{
  if(this->TextWidget)
    {
    delete this->TextWidget;
    }
}

//----------------------------------------------------------------------------
QWidget* vtkQtRecordView::GetWidget()
{
  return this->TextWidget;
}

//----------------------------------------------------------------------------
void vtkQtRecordView::SetFieldType(int type)
{
  this->DataObjectToTable->SetFieldType(type);
  this->Update();
}

//----------------------------------------------------------------------------
void vtkQtRecordView::AddInputConnection( int vtkNotUsed(port), int vtkNotUsed(index),
  vtkAlgorithmOutput* conn, vtkAlgorithmOutput* vtkNotUsed(selectionConn))
{
  // Get a handle to the input data object. Note: For now
  // we are enforcing that the input data is a Table.
  conn->GetProducer()->Update();
  vtkDataObject *d = conn->GetProducer()->GetOutputDataObject(0);
  if (!d)
    {
    vtkErrorMacro("vtkQtRecordView requires a vtkDataObject as input");
    return;
    }

  // Give the data object to the Qt Table Adapters
  this->DataObjectToTable->SetInput(d);
  this->DataObjectToTable->Update();
}

//----------------------------------------------------------------------------
void vtkQtRecordView::RemoveInputConnection(
  int vtkNotUsed(port), int vtkNotUsed(index),
  vtkAlgorithmOutput* vtkNotUsed(conn),
  vtkAlgorithmOutput* vtkNotUsed(selectionConn))
{
  this->DataObjectToTable->SetInputConnection(NULL);
  this->DataObjectToTable->Update();
}

//----------------------------------------------------------------------------
void vtkQtRecordView::Update()
{
  vtkDataRepresentation* rep = this->GetRepresentation();
  vtkStdString html;
  if (!rep)
    {
    this->TextWidget->setHtml(html.c_str());
    return;
    }

  this->DataObjectToTable->Update();
  vtkTable *table = this->DataObjectToTable->GetOutput();
  if (!table)
    {
    this->TextWidget->setHtml(html.c_str());
    return;
    }

  vtkSmartPointer<vtkSelection> cs;
  cs.TakeReference(vtkConvertSelection::ToSelectionType(rep->GetSelectionLink()->GetSelection(), 
      table, vtkSelectionNode::INDICES));
  vtkSelectionNode *node = cs->GetNode(0);
  if(node)
    {
    vtkAbstractArray *indexArr = node->GetSelectionList();
    if(indexArr->GetNumberOfTuples()>0)
      {
      vtkVariant v(0);
      switch (indexArr->GetDataType())
        {
        vtkExtraExtendedTemplateMacro(v = *static_cast<VTK_TT*>(indexArr->GetVoidPointer(0)));
        }
      this->CurrentRow = v.ToInt();
      }
    }

  const vtkIdType row_count = table->GetNumberOfRows();
  const vtkIdType column_count = table->GetNumberOfColumns();

  if(row_count && column_count && this->CurrentRow>=0)
    {
    vtkIdType row_index = this->CurrentRow % row_count;
    while(row_index < 0)
      row_index += row_count;

    for(vtkIdType i = 0; i != column_count; ++i)
      {
      html += "<b>";
      html += table->GetColumnName(i);
      html += ":</b> ";
      html += table->GetValue(row_index, i).ToString().c_str();
      html += "<br>\n";
      }
    }

  this->TextWidget->setHtml(html.c_str());
}

//----------------------------------------------------------------------------
void vtkQtRecordView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

