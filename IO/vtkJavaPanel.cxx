/*=========================================================================

  Program:   Java Wrapper for VTK
  Module:    vtkJavaPanel.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file's contents may be copied, reproduced or altered in any way 
without the express written consent of the author.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

