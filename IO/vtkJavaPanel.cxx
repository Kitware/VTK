/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJavaPanel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSystemIncludes.h"
#include <X11/Intrinsic.h>
#include "vtkJavaUtil.h"


// #define VTKJAVADEBUG

// super hack 9000
// include the header file as a C code fragment 
// include the JAVA C stubs file as a C code fragment 
extern "C" {
#include "java/vtk_vtkPanel.h"
#include "java/vtk_vtkPanel.c"

typedef struct Classsun_awt_motif_MCanvasPeer {
    struct Hjava_awt_Component *target;
    long pData;
} Classsun_awt_motif_MCanvasPeer;
HandleTo(sun_awt_motif_MCanvasPeer);

struct ComponentData {
    Widget  widget;
    int   repaintPending;
    int   x1, y1, x2, y2;
};

struct CanvasData {
    struct ComponentData  comp;
    Widget      shell;
    int       flags;
};

#define PEER_PDATA(T, T2, x) ((struct T *)(unhand((struct T2 *)unhand(x)->peer)->pData))
}

#include "vtkRenderWindow.h"

void vtk_vtkPanel_setWindow(struct Hvtk_vtkPanel *me,
                            struct Hvtk_vtkRenderWindow *id0)
{
  void *temp;
  vtkRenderWindow *op;

  op = (vtkRenderWindow *)vtkJavaGetPointerFromObject(id0,"vtkRenderWindow");

  struct CanvasData *wdata = 
    PEER_PDATA(CanvasData, Hsun_awt_motif_MCanvasPeer, me);
  Widget wig = wdata->comp.widget;

  // cerr << XtDisplay(wig) << " Display\n";
  // cerr << XtWindowOfObject(wig) << " Window\n";
  
  op->SetDisplayId((void *)XtDisplay(wig));
  op->SetWindowId((void *)XtWindowOfObject(wig));
}

