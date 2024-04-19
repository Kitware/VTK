// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkQtRecordView.h"
#include <QObject>
#include <QTextEdit>

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkAnnotationLink.h"
#include "vtkAttributeDataToTableFilter.h"
#include "vtkCommand.h"
#include "vtkConvertSelection.h"
#include "vtkDataRepresentation.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

namespace
{
const std::map<int, int> FIELD_ASSOCIATION_MAP = { { vtkQtRecordView::FIELD_DATA,
                                                     vtkDataObject::FIELD_ASSOCIATION_NONE },
  { vtkQtRecordView::POINT_DATA, vtkDataObject::FIELD_ASSOCIATION_POINTS },
  { vtkQtRecordView::CELL_DATA, vtkDataObject::FIELD_ASSOCIATION_CELLS },
  { vtkQtRecordView::VERTEX_DATA, vtkDataObject::FIELD_ASSOCIATION_VERTICES },
  { vtkQtRecordView::EDGE_DATA, vtkDataObject::FIELD_ASSOCIATION_EDGES },
  { vtkQtRecordView::ROW_DATA, vtkDataObject::FIELD_ASSOCIATION_ROWS } };
}

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkQtRecordView);

//------------------------------------------------------------------------------
vtkQtRecordView::vtkQtRecordView()
{
  this->TextWidget = new QTextEdit();
  this->DataObjectToTable = vtkSmartPointer<vtkAttributeDataToTableFilter>::New();
  this->DataObjectToTable->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_VERTICES);
  this->FieldType = vtkQtRecordView::VERTEX_DATA;
  this->Text = nullptr;
  this->CurrentSelectionMTime = 0;
  this->LastInputMTime = 0;
  this->LastMTime = 0;
}

//------------------------------------------------------------------------------
vtkQtRecordView::~vtkQtRecordView()
{
  delete this->TextWidget;
}

//------------------------------------------------------------------------------
QWidget* vtkQtRecordView::GetWidget()
{
  return this->TextWidget;
}

//------------------------------------------------------------------------------
void vtkQtRecordView::SetFieldType(int type)
{
  this->DataObjectToTable->SetFieldAssociation(::FIELD_ASSOCIATION_MAP.at(type));
  if (this->FieldType != type)
  {
    this->FieldType = type;
    this->Modified();
  }
}

void vtkQtRecordView::AddRepresentationInternal(vtkDataRepresentation* rep)
{
  vtkAlgorithmOutput* conn;
  conn = rep->GetInputConnection();

  this->DataObjectToTable->SetInputConnection(0, conn);
}

void vtkQtRecordView::RemoveRepresentationInternal(vtkDataRepresentation* rep)
{
  vtkAlgorithmOutput* conn;
  conn = rep->GetInputConnection();
  this->DataObjectToTable->RemoveInputConnection(0, conn);
}

//------------------------------------------------------------------------------
void vtkQtRecordView::Update()
{
  vtkDataRepresentation* rep = this->GetRepresentation();

  vtkAlgorithmOutput* conn = rep->GetInputConnection();
  vtkDataObject* d = conn->GetProducer()->GetOutputDataObject(0);
  vtkSelection* s = rep->GetAnnotationLink()->GetCurrentSelection();
  if (d->GetMTime() == this->LastInputMTime && this->LastMTime == this->GetMTime() &&
    s->GetMTime() == this->CurrentSelectionMTime)
  {
    return;
  }

  this->LastInputMTime = d->GetMTime();
  this->LastMTime = this->GetMTime();
  this->CurrentSelectionMTime = s->GetMTime();

  std::string html;
  if (!rep)
  {
    this->TextWidget->setHtml(html.c_str());
    return;
  }

  this->DataObjectToTable->Update();
  vtkTable* table = this->DataObjectToTable->GetOutput();
  if (!table)
  {
    this->TextWidget->setHtml(html.c_str());
    return;
  }

  vtkSmartPointer<vtkSelection> cs;
  cs.TakeReference(
    vtkConvertSelection::ToSelectionType(rep->GetAnnotationLink()->GetCurrentSelection(), table,
      vtkSelectionNode::INDICES, nullptr, vtkSelectionNode::ROW));
  vtkSelectionNode* node = cs->GetNode(0);
  const vtkIdType column_count = table->GetNumberOfColumns();

  if (node)
  {
    vtkAbstractArray* indexArr = node->GetSelectionList();
    int numRecords = indexArr->GetNumberOfTuples() > 2 ? 2 : indexArr->GetNumberOfTuples();
    for (vtkIdType i = 0; i < numRecords; ++i)
    {
      vtkVariant v(0);
      switch (indexArr->GetDataType())
      {
        vtkExtraExtendedTemplateMacro(v = *static_cast<VTK_TT*>(indexArr->GetVoidPointer(i)));
      }

      for (vtkIdType j = 0; j != column_count; ++j)
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

//------------------------------------------------------------------------------
void vtkQtRecordView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
