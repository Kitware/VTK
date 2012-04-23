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

#include <QDesignerCustomWidgetInterface>
#include <QDesignerCustomWidgetCollectionInterface>
#include <QtPlugin>
#include <QObject>
#include <QWidget>


// implement Designer Custom Widget interface
class QVTKWidgetPlugin : public QDesignerCustomWidgetInterface
{
  public:
    QVTKWidgetPlugin();
    ~QVTKWidgetPlugin();

    QString name() const;
    QString domXml() const;
    QWidget* createWidget(QWidget* parent = 0);
    QString group() const;
    QIcon icon() const;
    QString includeFile() const;
    QString toolTip() const;
    QString whatsThis() const;
    bool isContainer() const;
};

// implement designer widget collection interface
class QVTKPlugin : public QObject, public QDesignerCustomWidgetCollectionInterface
{
  Q_OBJECT
  Q_INTERFACES(QDesignerCustomWidgetCollectionInterface)
  public:
  QVTKPlugin();
  ~QVTKPlugin();

  virtual QList<QDesignerCustomWidgetInterface*> customWidgets() const;
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


#endif //QVTK_WIDGET_PLUGIN
