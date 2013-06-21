/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtView.cxx

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkQtView.h"

#include <QApplication>
#include <QPixmap>
#include <QWidget>
#include "vtkObjectFactory.h"


//----------------------------------------------------------------------------
vtkQtView::vtkQtView()
{

}

//----------------------------------------------------------------------------
vtkQtView::~vtkQtView()
{

}

//----------------------------------------------------------------------------
void vtkQtView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkQtView::ProcessQtEvents()
{
  QApplication::processEvents();
}

//----------------------------------------------------------------------------
void vtkQtView::ProcessQtEventsNoUserInput()
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

//----------------------------------------------------------------------------
bool vtkQtView::SaveImage(const char* filename)
{
#if QT_VERSION >= 0x050000
  return this->GetWidget() != 0 ? this->GetWidget()->grab().save(filename) : false;
#else
  // This is ok even if this->GetWidget() returns null.
  return QPixmap::grabWidget(this->GetWidget()).save(filename);
#endif
}
