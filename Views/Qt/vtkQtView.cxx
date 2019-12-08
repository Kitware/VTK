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

#include "vtkObjectFactory.h"
#include <QApplication>
#include <QPixmap>
#include <QWidget>

//----------------------------------------------------------------------------
vtkQtView::vtkQtView() {}

//----------------------------------------------------------------------------
vtkQtView::~vtkQtView() {}

//----------------------------------------------------------------------------
void vtkQtView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
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
  return this->GetWidget() != nullptr ? this->GetWidget()->grab().save(filename) : false;
}
