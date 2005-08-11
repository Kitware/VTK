/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SimpleView3.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright 2004 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/

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


#ifndef SIMPLEVIEW_H
#define SIMPLEVIEW_H

#include "uiSimpleView3.h"

// Forward class declarations
class vtkCylinderSource;
class vtkPolyDataMapper;
class vtkExodusReader;
class vtkDataSetMapper;
class vtkActor;
class vtkRenderer;


class SimpleView : public uiSimpleView
{
    Q_OBJECT

public:

    // Constructor/Destructor
    SimpleView(QWidget* parent = 0);
    ~SimpleView() {};

public slots:

     virtual void fileOpen();
     virtual void fileExit();

protected:

protected slots:

private:
     vtkCylinderSource* source;
     vtkPolyDataMapper* mapper;
     vtkActor* actor;
     vtkRenderer* ren;
   
};

#endif // SIMPLEVIEW_H

