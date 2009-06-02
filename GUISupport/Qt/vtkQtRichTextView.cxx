/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkQtRichTextView.cxx

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

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkCommand.h"
#include "vtkConvertSelection.h"
#include "vtkDataObjectToTable.h"
#include "vtkDataRepresentation.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkQtRichTextView.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSelectionSource.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <ui_vtkQtRichTextView.h>

#include <QFrame>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QObject>
#include <QPointer>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWebFrame>
#include <QWebHistory>
#include <QWebView>

vtkCxxRevisionMacro(vtkQtRichTextView, "1.6");
vtkStandardNewMacro(vtkQtRichTextView);

/////////////////////////////////////////////////////////////////////////////
// vtkQtRichTextView::Implementation

class vtkQtRichTextView::Implementation
{
public:
  ~Implementation()
  {
    delete this->Widget;
  }

  // Handles conversion of our input data to a table for display
  vtkSmartPointer<vtkDataObjectToTable> DataObjectToTable;

  // Caches displayed content so we can navigate backwards to it
  vtkStdString Content;

  QPointer<QWidget> Widget;
  Ui::vtkQtRichTextView UI;
};

/////////////////////////////////////////////////////////////////////////////
// vtkQtRichTextView

vtkQtRichTextView::vtkQtRichTextView() :
  Internal(new Implementation())
{
  this->Internal->DataObjectToTable = vtkSmartPointer<vtkDataObjectToTable>::New();
  this->Internal->DataObjectToTable->SetFieldType(ROW_DATA);

  this->Internal->Widget = new QWidget();
  this->Internal->UI.setupUi(this->Internal->Widget);
  this->Internal->UI.WebView->setHtml("");

  QNetworkProxy proxy(QNetworkProxy::HttpCachingProxy,"wwwproxy.sandia.gov",80);
  QNetworkProxy::setApplicationProxy(proxy);

  QObject::connect(this->Internal->UI.BackButton, SIGNAL(clicked()), this, SLOT(onBack()));
}

vtkQtRichTextView::~vtkQtRichTextView()
{
  delete this->Internal;
}

void vtkQtRichTextView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

QWidget* vtkQtRichTextView::GetWidget()
{
  return this->Internal->Widget;
}

void vtkQtRichTextView::SetFieldType(int type)
{
  this->Internal->DataObjectToTable->SetFieldType(type);
  this->Update();
}

int vtkQtRichTextView::GetFieldType()
{
  return this->Internal->DataObjectToTable->GetFieldType();
}

void vtkQtRichTextView::Update()
{
  // Make sure the input connection is up to date ...
  vtkDataRepresentation* const representation = this->GetRepresentation();
  if(!representation)
    {
    this->Internal->UI.WebView->setHtml("");
    return;
    }
  representation->Update();

  if(this->Internal->DataObjectToTable->GetTotalNumberOfInputConnections() == 0
      || this->Internal->DataObjectToTable->GetInputConnection(0, 0) != representation->GetInternalOutputPort(0))
    {
    this->Internal->DataObjectToTable->SetInputConnection(0, representation->GetInternalOutputPort(0));
    }
  this->Internal->DataObjectToTable->Update();

  // Get our input table ...
  vtkTable* const table = this->Internal->DataObjectToTable->GetOutput();
  if(!table)
    {
    this->Internal->UI.WebView->setHtml("");
    return;
    }

  // Special-case: if the table is empty, we're done ...
  if(0 == table->GetNumberOfRows())
    {
    this->Internal->UI.WebView->setHtml("");
    return;
    }

  // Figure-out which row of the table we're going to display ...
  const vtkIdType row = 0; /** \TODO: Base this on the current selection */

  this->Internal->Content = table->GetValueByName(row, "html").ToString();
  this->Internal->UI.WebView->history()->clear(); // Workaround for a quirk in QWebHistory
  //cerr << this->Internal->Content << endl;

  this->Internal->UI.WebView->setHtml(this->Internal->Content.c_str());
}

void vtkQtRichTextView::onBack()
{
  // This logic is a workaround for a quirk in QWebHistory
  if(this->Internal->UI.WebView->history()->currentItemIndex() <= 1)
    {
    this->Internal->UI.WebView->setHtml(this->Internal->Content.c_str());
    this->Internal->UI.WebView->history()->clear();
    }
  else
    {
    this->Internal->UI.WebView->back();
    }
}

