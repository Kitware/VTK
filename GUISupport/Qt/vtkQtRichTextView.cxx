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

vtkCxxRevisionMacro(vtkQtRichTextView, "1.4");
vtkStandardNewMacro(vtkQtRichTextView);

/////////////////////////////////////////////////////////////////////////////
// vtkQtRichTextView::Implementation

class vtkQtRichTextView::Implementation
{
public:
  ~Implementation()
  {
    delete this->Frame;
  }

  vtkSmartPointer<vtkDataObjectToTable> DataObjectToTable;

  vtkStdString Content;

  QPointer<QFrame> Frame;
  QPointer<QWebView> WebView;
};

/////////////////////////////////////////////////////////////////////////////
// vtkQtRichTextView

vtkQtRichTextView::vtkQtRichTextView() :
  Internal(new Implementation())
{
  this->Internal->DataObjectToTable = vtkSmartPointer<vtkDataObjectToTable>::New();
  this->Internal->DataObjectToTable->SetFieldType(ROW_DATA);

  this->Internal->Frame = new QFrame();
  this->Internal->Frame->show();
  this->Internal->WebView = new QWebView();
  this->Internal->WebView->setHtml("");
  this->Internal->WebView->show();

  QPushButton* const back_button = new QPushButton("Back");
  
  QVBoxLayout* const layout = new QVBoxLayout();
  layout->addWidget(back_button);
  layout->addWidget(this->Internal->WebView);
  this->Internal->Frame->setLayout(layout);
  
  QNetworkProxy proxy(QNetworkProxy::HttpCachingProxy,"wwwproxy.sandia.gov",80);
  QNetworkProxy::setApplicationProxy(proxy);

  QObject::connect(back_button, SIGNAL(clicked()), this, SLOT(onBack()));
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
  return this->Internal->Frame;
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
    this->Internal->WebView->setHtml("");
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
    this->Internal->WebView->setHtml("");
    return;
    }

  // Special-case: if the table is empty, we're done ...
  if(0 == table->GetNumberOfRows())
    {
    this->Internal->WebView->setHtml("");
    return;
    }

  // Figure-out which row of the table we're going to display ...
  const vtkIdType row = 0; /** \TODO: Base this on the current selection */

  this->Internal->Content = table->GetValueByName(row, "html").ToString();
cerr << this->Internal->Content << endl;

  this->Internal->WebView->setHtml(this->Internal->Content.c_str());
  this->Internal->Content = this->Internal->Content;
}

void vtkQtRichTextView::onBack()
{
  if(this->Internal->WebView->history()->canGoBack())
    {
    this->Internal->WebView->back();
    }
  else
    {
    this->Internal->WebView->setHtml(this->Internal->Content.c_str());
    }
}

