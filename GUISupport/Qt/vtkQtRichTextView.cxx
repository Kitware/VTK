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

#include "vtkQtRichTextView.h"
#include <QObject>
#include <QTextEdit>
#include <QWebPage>
#include <QWebView>
#include <QWebFrame>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QPushButton>
#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QFrame>
#include <QHBoxLayout>

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
#include "vtkSelectionNode.h"
#include "vtkSelectionSource.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

vtkCxxRevisionMacro(vtkQtRichTextView, "1.1");
vtkStandardNewMacro(vtkQtRichTextView);

//----------------------------------------------------------------------------
vtkQtRichTextView::vtkQtRichTextView()
{
  int argc = 0;
  
  this->TextWidgetView = new QWebView();
  this->TextWidgetPage = new QWebPage();
  this->BackButton = new QPushButton("Back");
  this->TextWidgetFrame = new QFrame();
  
  //   QHBoxLayout *hlayout = new QHBoxLayout();
//   hlayout->addWidget(this->BackButton);
//   hlayout->addStretch();

//   QVBoxLayout *vlayout = new QVBoxLayout();
//   vlayout->addWidget(this->RichTextView->GetWidget());
//   vlayout->addWidget(hlayout);


//  QVBoxLayout* layout = new QVBoxLayout();
  this->TextLayout = new QVBoxLayout();
  
  TextLayout->addWidget(this->TextWidgetView);
  TextLayout->addWidget(this->BackButton);

  this->TextWidgetFrame->setLayout(TextLayout);
  
  //TextWidgetPage->setView(this->TextWidgetView);
  TextWidgetPage->setView(this->TextWidgetFrame);

  QNetworkProxy proxy(QNetworkProxy::HttpCachingProxy,"wwwproxy.sandia.gov",80);

  QNetworkProxy::setApplicationProxy(proxy);


  this->DataObjectToTable = vtkSmartPointer<vtkDataObjectToTable>::New();
  this->FieldType = vtkQtRichTextView::VERTEX_DATA;
  this->Text = NULL;
  

}

//----------------------------------------------------------------------------
vtkQtRichTextView::~vtkQtRichTextView()
{
  if(this->TextWidgetView)
    {
    delete this->TextWidgetView;
    }
  if(this->TextWidgetPage)
    {
    delete this->TextWidgetPage;
    }
  if(this->BackButton)
    {
    delete this->BackButton;
    }
}

//----------------------------------------------------------------------------
QWidget* vtkQtRichTextView::GetWidget()
{
  return this->TextWidgetView;
  //return this->TextWidgetFrame;
  
}

//----------------------------------------------------------------------------
void vtkQtRichTextView::SetFieldType(int type)
{
  this->DataObjectToTable->SetFieldType(type);
  this->Update();
}

//----------------------------------------------------------------------------
void vtkQtRichTextView::AddInputConnection( 
                                           vtkAlgorithmOutput* conn, 
                                           vtkAlgorithmOutput* vtkNotUsed(selectionConn))
{  
  this->DataObjectToTable->SetInputConnection(0, conn);
}

//----------------------------------------------------------------------------
void vtkQtRichTextView::RemoveInputConnection(
                                              vtkAlgorithmOutput* conn, 
                                              vtkAlgorithmOutput* vtkNotUsed(selectionConn))
{  
  this->DataObjectToTable->RemoveInputConnection(0, conn);
}

//----------------------------------------------------------------------------
void vtkQtRichTextView::Update()
{
  
  vtkDataRepresentation* rep = this->GetRepresentation();
 
  vtkStdString html, new_html;
  if (!rep)
    {
    this->TextWidgetView->setHtml(html.c_str());
    return;
    }

  this->DataObjectToTable->Update();
  vtkTable *table = this->DataObjectToTable->GetOutput();
  if (!table)
    {
    this->TextWidgetView->setHtml(html.c_str());
    return;
    }
  
  vtkSmartPointer<vtkSelection> cs = vtkSmartPointer<vtkSelection>::New();
  //cs.TakeReference(vtkConvertSelection::ToSelectionType(rep->GetSelectionLink()->GetSelection(), 
  //                                                    table, vtkSelectionNode::INDICES, 0, vtkSelectionNode::ROW));

  vtkSmartPointer<vtkSelectionSource> source = vtkSmartPointer<vtkSelectionSource>::New();
  source->SetContentType(vtkSelectionNode::INDICES);
  source->SetFieldType(3);
  source->AddID(-1,0);
  source->AddID(-1,1);
  source->AddID(-1,2);
  source->AddID(-1,3);
  source->AddID(-1,4);
  
  source->Update();
  

  cs->ShallowCopy(source->GetOutput());
  
  
  vtkSelectionNode *node = cs->GetNode(0);
  const vtkIdType column_count = table->GetNumberOfColumns();

  if(node)
    {
    vtkAbstractArray *indexArr = node->GetSelectionList();
    int numRecords = indexArr->GetNumberOfTuples();
    
    //Okay, we're assuming that we're getting a table, with a
    //selection, (or a table of selections) that have columns labeled
    //according to what they've done. <header> <body> <footer>
    //<htmlized_body>

    //We want to glue these together into an html file, with an
    //appropriate html header and footer

    QString originalString(new_html.c_str());
    QString htmlTextString;
//    newString.reserve(originalString.size() *2);
    htmlTextString+="<!--  ************************************************** -->";
    htmlTextString+="<html>";
    htmlTextString+="<head>";
    htmlTextString+="<style type=\"text/css\" media=\"all\">";
    htmlTextString+="p";
    htmlTextString+="  {";
    htmlTextString+="  font: 83%/150% georgia, palatino, serif;";
    htmlTextString+="  }";
    htmlTextString+="body, p, h3";
    htmlTextString+="  {";
    htmlTextString+="  font-family: arial \'Bitstream Vera Sans Mono\' monospace;";
    htmlTextString+="  }";
    htmlTextString+="h3";
    htmlTextString+="  {";
    htmlTextString+="  letter-spacing: 3px;";
    htmlTextString+="  }";
    htmlTextString+="a ";
    htmlTextString+="  { ";
    htmlTextString+="  text-decoration: none; ";
    htmlTextString+="  border-bottom: 1px dashed; ";
    htmlTextString+="  }";
    htmlTextString+="a:hover ";
    htmlTextString+="  { ";
    htmlTextString+="  border: 1px dashed; ";
    htmlTextString+="  }";
    htmlTextString+="#nav-menu ul";
    htmlTextString+="  {";
    htmlTextString+="  list-style: none;";
    htmlTextString+="  padding: 0;";
    htmlTextString+="  margin: 0;";
    htmlTextString+="  } ";
    htmlTextString+="#nav-menu a ";
    htmlTextString+="  { ";
    htmlTextString+="  text-decoration: none;";
    htmlTextString+="  border-bottom: none;";
    htmlTextString+="  font-size: 85%;";
    htmlTextString+="  }";
    htmlTextString+="#nav-menu a:hover ";
    htmlTextString+="  { ";
    htmlTextString+="  border: none; ";
    htmlTextString+="  border-bottom: 1px solid; ";
    htmlTextString+="  }";
    htmlTextString+="#M3Header p";
    htmlTextString+="  {";
    htmlTextString+="  font-size: 65%;";
    htmlTextString+="  }";
    htmlTextString+="#M3Text p";
    htmlTextString+="  {";
    htmlTextString+="  font-size: 75%;";
    htmlTextString+="  }";
    htmlTextString+="#M3Footer p ";
    htmlTextString+="  {";
    htmlTextString+="  font-size: 65%; ";
    htmlTextString+="  }";
    htmlTextString+="#M3NavigationLink p ";
    htmlTextString+="  {";
    htmlTextString+="  font-size: 35%; ";
    htmlTextString+="  }";
    htmlTextString+="a.PERSON { color: blue; }";
    htmlTextString+="a.LOCATION { color: green; }";
    htmlTextString+="a.ORGANIZATION { color: red; }";
    htmlTextString+="a.MISC { color: orange; }";
  
    htmlTextString+="<!-- #p111{background: yellow} -->";
    htmlTextString+="<!-- #p112{background: tan} -->";
    htmlTextString+="<!-- #p113{background: lightblue} -->";
    htmlTextString+="<!-- #p114{background: black} -->";

    htmlTextString+="</style></head><body>";
    vtkIdType ii;

    htmlTextString+="<a name=\"GlobalNav\"></a>";
    htmlTextString+="<div id=\"nav-menu\">";
    htmlTextString+="<H3>Global Navigation</H3>";
    htmlTextString+="<ul>";
  
    for(ii = 0; ii<numRecords; ++ii)
      {
      if((table->GetValueByName(ii, "htmlized_body").ToString()).size() > 0)
        {
        vtkStdString docID = table->GetValueByName(ii, "document").ToString();
        htmlTextString += "<li><a href=\"#Navigation";
        htmlTextString += docID;
        htmlTextString += "\">";
        htmlTextString += " Document ";
        htmlTextString += docID;
        htmlTextString += "</a></li>";
        }
      }
    htmlTextString += "</ul>";
    htmlTextString += "</div>";
    
    for(vtkIdType ii=0; ii<numRecords; ++ii)
      {
      vtkStdString currentHtmlText = table->GetValueByName(ii, "htmlized_body").ToString();
      if(currentHtmlText.size() > 0)
        {
        htmlTextString+= table->GetValueByName(ii, "htmlized_body").ToString();
        htmlTextString += "<div id=\"nav-menu\"><a href=\"#GlobalNav\">Back to Global Navigation</a></div>";
        htmlTextString += "<br /><br /><br /><br />";
        }
      }
    htmlTextString += "</body></html>";

    cout<<htmlTextString.toStdString();
    
    this->TextWidgetView->setHtml(htmlTextString);
    cout.flush();
    }
  this->TextWidgetView->show();
  
  this->TextWidgetView->update();
 
}

//----------------------------------------------------------------------------
void vtkQtRichTextView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

int vtkQtRichTextView::find_string(QString &myString, QString &searchString, int prev_loc)
{

  int my_loc = myString.indexOf(searchString, prev_loc, Qt::CaseInsensitive);
  return my_loc;
  //int QString::indexOf ( const QString & str, int from = 0, Qt::CaseSensitivity cs = Qt::CaseSensitive ) const

}

int vtkQtRichTextView::insert_string(QString &myString, QString &htmlString, int location)
{
  cout<<"MyString size = "<<myString.size()<<endl;
  myString.insert(location, htmlString);
  cout<<"MyString size = "<<myString.size()<<endl;
    
  location += htmlString.size();
  return location;
  
//  QString & QString::insert ( int position, const QString & str )
}
