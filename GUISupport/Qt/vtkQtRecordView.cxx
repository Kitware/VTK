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
#include "vtkAnnotationLink.h"
#include "vtkCommand.h"
#include "vtkConvertSelection.h"
#include "vtkDataObjectToTable.h"
#include "vtkDataRepresentation.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

vtkStandardNewMacro(vtkQtRecordView);

//----------------------------------------------------------------------------
vtkQtRecordView::vtkQtRecordView()
{
  this->TextWidget = new QTextEdit();
  this->DataObjectToTable = vtkSmartPointer<vtkDataObjectToTable>::New();
  this->DataObjectToTable->SetFieldType(vtkDataObjectToTable::VERTEX_DATA);
  this->FieldType = vtkQtRecordView::VERTEX_DATA;
  this->Text = NULL;
  this->CurrentSelectionMTime = 0;
  this->LastInputMTime = 0;
  this->LastMTime = 0;
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
  if(this->FieldType != type)
    {
    this->FieldType = type;
    this->Modified();
    }
}

void vtkQtRecordView::AddRepresentationInternal(vtkDataRepresentation* rep)
{    
  vtkAlgorithmOutput *conn;
  conn = rep->GetInputConnection();

  this->DataObjectToTable->SetInputConnection(0, conn);
}

void vtkQtRecordView::RemoveRepresentationInternal(vtkDataRepresentation* rep)
{   
  vtkAlgorithmOutput *conn;
  conn = rep->GetInputConnection();
  this->DataObjectToTable->RemoveInputConnection(0, conn);
}

//----------------------------------------------------------------------------
void vtkQtRecordView::Update()
{
  vtkDataRepresentation* rep = this->GetRepresentation();

  vtkAlgorithmOutput* conn = rep->GetInputConnection();
  vtkDataObject *d = conn->GetProducer()->GetOutputDataObject(0);
  vtkSelection* s = rep->GetAnnotationLink()->GetCurrentSelection();
  if (d->GetMTime() == this->LastInputMTime &&
      this->LastMTime == this->GetMTime() &&
      s->GetMTime() == this->CurrentSelectionMTime)
    {
    return;
    }

  this->LastInputMTime = d->GetMTime();
  this->LastMTime = this->GetMTime();
  this->CurrentSelectionMTime = s->GetMTime();

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
  cs.TakeReference(vtkConvertSelection::ToSelectionType(
    rep->GetAnnotationLink()->GetCurrentSelection(), 
    table, vtkSelectionNode::INDICES, 0, vtkSelectionNode::ROW));
  vtkSelectionNode *node = cs->GetNode(0);
  const vtkIdType column_count = table->GetNumberOfColumns();

  if(node)
    {
    vtkAbstractArray *indexArr = node->GetSelectionList();
    int numRecords = indexArr->GetNumberOfTuples() > 2 ? 2 : indexArr->GetNumberOfTuples();
    for(vtkIdType i=0; i<numRecords; ++i)
      {
      vtkVariant v(0);
      switch (indexArr->GetDataType())
        {
        vtkExtraExtendedTemplateMacro(v = *static_cast<VTK_TT*>(indexArr->GetVoidPointer(i)));
        }

      for(vtkIdType j = 0; j != column_count; ++j)
        {
        html += "<b>";
        html += table->GetColumnName(j);
        html += ":</b> ";
        html += table->GetValue(v.ToInt(), j).ToString().c_str();
        html += "<br>\n";
        }
      html += "<br>\n<br>\n<br>\n<br>\n<br>\n";
      }
    }

  this->TextWidget->setHtml(html.c_str());
}

//----------------------------------------------------------------------------
void vtkQtRecordView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

