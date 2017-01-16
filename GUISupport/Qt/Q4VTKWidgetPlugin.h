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

#ifndef QVTK_WIDGET_PLUGIN
#define QVTK_WIDGET_PLUGIN

// Disable warnings that Qt headers give.
#if defined(__GNUC__) && (__GNUC__>4) || (__GNUC__==4 && __GNUC_MINOR__>=6)
#pragma GCC diagnostic push
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include <QDesignerCustomWidgetInterface>
#include <QDesignerCustomWidgetCollectionInterface>
#include <QObject>
#include <QtPlugin>
#include <QWidget>

#include "vtkConfigure.h" //for VTK_OVERRIDE


// implement Designer Custom Widget interface
class QVTKWidgetPlugin : public QDesignerCustomWidgetInterface
{
  public:
    QVTKWidgetPlugin();
    ~QVTKWidgetPlugin() VTK_OVERRIDE;

    QString name() const VTK_OVERRIDE;
    QString domXml() const VTK_OVERRIDE;
    QWidget* createWidget(QWidget* parent = 0) VTK_OVERRIDE;
    QString group() const VTK_OVERRIDE;
    QIcon icon() const VTK_OVERRIDE;
    QString includeFile() const VTK_OVERRIDE;
    QString toolTip() const VTK_OVERRIDE;
    QString whatsThis() const VTK_OVERRIDE;
    bool isContainer() const VTK_OVERRIDE;
};

// implement designer widget collection interface
class QVTKPlugin : public QObject, public QDesignerCustomWidgetCollectionInterface
{
  Q_OBJECT
  #if QT_VERSION >= 0x050000
  Q_PLUGIN_METADATA(IID "org.vtk.qvtkplugin")
  #endif
  Q_INTERFACES(QDesignerCustomWidgetCollectionInterface)
  public:
  QVTKPlugin();
  ~QVTKPlugin() VTK_OVERRIDE;

  QList<QDesignerCustomWidgetInterface*> customWidgets() const VTK_OVERRIDE;
  private:
    QVTKWidgetPlugin* mQVTKWidgetPlugin;
};

// fake QVTKWidget class to satisfy the designer
class QVTKWidget : public QWidget
{
  Q_OBJECT
public:
  QVTKWidget(QWidget* p) : QWidget(p) {}
};

// Undo disabling of warning.
#if defined(__GNUC__) && (__GNUC__>4) || (__GNUC__==4 && __GNUC_MINOR__>=6)
#pragma GCC diagnostic pop
#endif

#endif //QVTK_WIDGET_PLUGIN
