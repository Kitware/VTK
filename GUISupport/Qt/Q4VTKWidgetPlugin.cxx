/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

/*========================================================================
 For general information about using VTK and Qt, see:
 http://www.trolltech.com/products/3rdparty/vtksupport.html
=========================================================================*/

/*========================================================================
 !!! WARNING for those who want to contribute code to this file.
 !!! If you use a commercial edition of Qt, you can modify this code.
 !!! If you use an open source version of Qt, you are free to modify
 !!! and use this code within the guidelines of the GPL license.
 !!! Unfortunately, you cannot contribute the changes back into this
 !!! file.  Doing so creates a conflict between the GPL and BSD-like VTK
 !!! license.
=========================================================================*/

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#pragma warning(disable:4512)
#endif

#if !defined(_DEBUG)
# if !defined(QT_NO_DEBUG)
#  define QT_NO_DEBUG
# endif
#endif

#include "Q4VTKWidgetPlugin.h"

#include "qobject.h"
#include "QVTKWidget.h"
#include "QVTKWidget.xpm"

#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkDataSetMapper.h"
#include "vtkPolyData.h"
#include "vtkElevationFilter.h"
#include "vtkActor.h"

// macro for debug printing
#define qDebug(a)
//#define qDebug(a) printf(a)

QVTKWidgetPlugin::QVTKWidgetPlugin()
{
  qDebug("QVTKWidgetPlugin instantiated\n");
}

QVTKWidgetPlugin::~QVTKWidgetPlugin()
{
  qDebug("QVTKWidgetPlugin destructed\n");
}
    
//! return the name of this widget
QString QVTKWidgetPlugin::name() const
{
  qDebug("QVTKWidgetPlugin::name\n");
  return "QVTKWidget";
}

QString QVTKWidgetPlugin::domXml() const
{
  return QLatin1String("<widget class=\"QVTKWidget\" name=\"qvtkWidget\">\n"
                       " <property name=\"geometry\">\n"
                       "  <rect>\n"
                       "   <x>0</x>\n"
                       "   <y>0</y>\n"
                       "   <width>100</width>\n"
                       "   <height>100</height>\n"
                       "  </rect>\n"
                       " </property>\n"
                       "</widget>\n");
}

QWidget* QVTKWidgetPlugin::createWidget(QWidget* parent)
{
  qDebug("QVTKWidgetPlugin::createWidget\n");
  QVTKWidget* widget = new QVTKWidget(parent);
  // gotta make a renderer so we get a nice black background in the designer
  vtkRenderer* ren = vtkRenderer::New();
  widget->GetRenderWindow()->AddRenderer(ren);

  // also for fun, let's make a cylinder and put it in the window
  // this REALLY lets the user know that a QVTKWidget works in the designer
  vtkSphereSource* cyl = vtkSphereSource::New();
  vtkElevationFilter* ele = vtkElevationFilter::New();
  ele->SetLowPoint(0.0, -0.5, 0.0);
  ele->SetHighPoint(0.0, 0.5, 0.0);
  ele->SetInput(cyl->GetOutput());
  vtkDataSetMapper* mapper = vtkDataSetMapper::New();
  mapper->SetInput(ele->GetOutput());
  ele->Delete();
  cyl->Delete();
  vtkActor* actor = vtkActor::New();
  actor->SetMapper(mapper);
  mapper->Delete();
#if (VTK_MAJOR_VERSION > 4) || (VTK_MAJOR_VERSION == 4 && VTK_MINOR_VERSION >=5)
  ren->AddViewProp(actor);
#else
  ren->AddProp(actor);
#endif
  actor->Delete();
  ren->Delete();

  // return the widget
  return widget;
}

QString QVTKWidgetPlugin::group() const
{
  qDebug("QVTKWidgetPlugin::group\n");
  return "QVTK";
}

QIcon QVTKWidgetPlugin::icon() const
{
  qDebug("QVTKWidgetPlugin::icon\n");
  return QIcon( QPixmap( QVTKWidget_image ) );
}

//! the name of the include file for building an app with a widget
QString QVTKWidgetPlugin::includeFile() const
{
  qDebug("QVTKWidgetPlugin::includeFile\n");
  return "QVTKWidget.h";
}

//! tool tip text
QString QVTKWidgetPlugin::toolTip() const
{
  qDebug("QVTKWidgetPlugin::toolTip\n");
  return "Qt VTK Widget";
}

//! what's this text
QString QVTKWidgetPlugin::whatsThis() const
{
  qDebug("QVTKWidgetPlugin::whatsThis\n");
  return "A Qt/VTK Graphics Window";
}

//! returns whether widget is a container
bool QVTKWidgetPlugin::isContainer() const
{
  qDebug("QVTKWidgetPlugin::isContainer\n");
  return false;
}

QVTKPlugin::QVTKPlugin()
{
  mQVTKWidgetPlugin = new QVTKWidgetPlugin;
}
QVTKPlugin::~QVTKPlugin()
{
  delete mQVTKWidgetPlugin;
}

QList<QDesignerCustomWidgetInterface*> QVTKPlugin::customWidgets() const
{
  QList<QDesignerCustomWidgetInterface*> plugins;
  plugins.append(mQVTKWidgetPlugin);
  return plugins;
}

Q_EXPORT_PLUGIN(QVTKPlugin)

