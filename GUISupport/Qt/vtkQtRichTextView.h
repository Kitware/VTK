/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtRichTextView.h

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
// .NAME vtkQtRichTextView - Superclass for QAbstractItemView-based views.
//
// .SECTION Description
// This superclass provides all the plumbing to integrate a QAbstractItemView
// into the VTK view framework, including reporting selection changes and
// detecting selection changes from linked views.
//
// .SECTION Thanks
// Thanks to Bob Kerr from Sandia National Laboratories for implementing
// this class

#ifndef __vtkQtRichTextView_h
#define __vtkQtRichTextView_h

#include "QVTKWin32Header.h"
#include "vtkQtView.h"

class vtkStdString;
class QUrl;

class QVTK_EXPORT vtkQtRichTextView : public vtkQtView
{
Q_OBJECT

public:
  static vtkQtRichTextView *New();
  vtkTypeMacro(vtkQtRichTextView, vtkQtView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the main container of this view (a  QWidget).
  // The application typically places the view with a call
  // to GetWidget(): something like this
  // this->ui->box->layout()->addWidget(this->View->GetWidget());
  virtual QWidget* GetWidget();

//BTX
  enum
    {
    FIELD_DATA = 0,
    POINT_DATA = 1,
    CELL_DATA = 2,
    VERTEX_DATA = 3,
    EDGE_DATA = 4,
    ROW_DATA = 5,
    };
//ETX

  // Description:
  // The field type to copy into the output table.
  // Should be one of FIELD_DATA, POINT_DATA, CELL_DATA, VERTEX_DATA, EDGE_DATA.
  void SetFieldType(int);
  int GetFieldType();

  // Description:
  // If a vtkTable is provided as input to the view, this sets the column
  // name to use for the content.  (Default: 'http').
  vtkSetStringMacro(ContentColumnName);
  vtkGetStringMacro(ContentColumnName);

  // Description:
  // If a vtkTable is provided as input to the view, this sets the column
  // name to use for the preview when multiple items in selection.
  vtkSetStringMacro(PreviewColumnName);
  vtkGetStringMacro(PreviewColumnName);

  // Description:
  // If a vtkTable is provided as input to the view, this sets the column
  // name to use for the title displayed in the title bar.
  vtkSetStringMacro(TitleColumnName);
  vtkGetStringMacro(TitleColumnName);

  // Description:
  // This field sets a URL for a HTTP proxy server.
  vtkSetStringMacro(ProxyURL);
  vtkGetStringMacro(ProxyURL);

  // Description:
  // This fields sets the port number for a HTTP proxy server.
  vtkSetMacro(ProxyPort, int);
  vtkGetMacro(ProxyPort, int);

  // Description:
  // Updates the view.
  virtual void Update();

protected slots:
  void onBack();
  void onForward();
  void onZoomIn();
  void onZoomReset();
  void onZoomOut();
  void onLoadProgress(int progress);
  void onLinkClicked(const QUrl &url);

protected:
  vtkQtRichTextView();
  ~vtkQtRichTextView();

private:
  vtkQtRichTextView(const vtkQtRichTextView&);  // Not implemented.
  void operator=(const vtkQtRichTextView&);  // Not implemented.

  char* ContentColumnName;
  char* PreviewColumnName;
  char* TitleColumnName;
  char* ProxyURL;
  int   ProxyPort;

  class Implementation;
  Implementation* Internal;
};

#endif
