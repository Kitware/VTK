/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtView.h

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkQtView - Superclass for Qt widget-based views.
//
// .SECTION Description
// This abstract superclass provides common api to integrate a Qt widget 
// into the VTK view framework. Not much here yet, but in the future there
// could be methods around selection, event-handling, drag-and-drop, etc.
//


#ifndef __vtkQtView_h
#define __vtkQtView_h

#include "QVTKWin32Header.h"
#include "vtkView.h"

#include <QObject>

class QVTK_EXPORT vtkQtView : public QObject, public vtkView
{
Q_OBJECT
public:

  vtkTypeMacro(vtkQtView, vtkView);
  void PrintSelf(ostream& os, vtkIndent indent);
      
  // Description:
  // Get the main container of this view (a  QWidget).
  // The application typically places the view with a call
  // to GetWidget(): something like this
  // this->ui->box->layout()->addWidget(this->View->GetWidget());
  virtual QWidget* GetWidget()=0;

  // Description:
  // Calls QApplication::processEvents().  This is useful if you are using QWidgets
  // but have not called QApplication::exec because you don't want to give control
  // to the Qt event loop.  See also ProcessQtEventsNoUserEvents().
  virtual void ProcessQtEvents();

  // Description:
  // Calls QApplication::processEvents(QEventLoop::ExcludeUserInputEvents).
  // See also ProcessQtEvents().
  virtual void ProcessQtEventsNoUserInput();

  // Description:
  // Save an image.  Uses QPixmap::grab and QPixmap::save.  The image format will
  // be determined from the filename.  Qt's image format support may vary, usually
  // bmp, jpg, ppm, or png is a safe choice.  Returns false if there was a failure.
  virtual bool SaveImage(const char* fileName);
  
protected:
  vtkQtView();
  ~vtkQtView();

private:
  vtkQtView(const vtkQtView&);  // Not implemented.
  void operator=(const vtkQtView&);  // Not implemented.
  
};

#endif
