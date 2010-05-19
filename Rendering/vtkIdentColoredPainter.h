/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIdentColoredPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkIdentColoredPainter - DEPRECATED A vtkPolyDataPainter that colors each polygon
// with a color coded integer.
//
// .SECTION Description
// DEPRECATED. Refer to vtkHardwareSelectionPolyDataPainter instead.
// This painter will color each polygon in a color that encodes an integer.
// Doing so allows us to determine what polygon is behind each pixel on the 
// screen.

// Two different modes exist. The first mode colors every polygon the
// same (SetToColorByConstant). By setting the constant with a processor rank
// this lets us find out which processor rendered each pixel after parallel 
// depth compositing. Alternatively, by changing the constant in between 
// actors this allows us to differentiate visible actors. 

// The second mode is to render each polygon in the actor with its own color 
// (SetToColorByIncreasingIdent). Because color depth is limited to 24 bits
// while visualization data is often larger than 2^24 cells, the index which
// is increasing is implemented as a 72 bit number. You can customize this
// mode to color the pixels by three different 24 bit fields of the number.
// Rendering in three separate passes can then differentiate between 2^72 
// different cells.
//
// .SECTION See Also
// vtkVisibleCellSelection
//

#ifndef __vtkIdentColoredPainter_h
#define __vtkIdentColoredPainter_h

#include "vtkPolyDataPainter.h"

class vtkCellArray;
class vtkIdTypeArray;
class vtkProp;

class VTK_RENDERING_EXPORT vtkIdentColoredPainter : public vtkPolyDataPainter
{
public:
  vtkTypeMacro(vtkIdentColoredPainter, vtkPolyDataPainter);
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  static vtkIdentColoredPainter *New();

  //Description:
  //resets the current id to "first".
  void ResetCurrentId();

  //Description:
  //Use to color each cell by processor rank or by actor id.
  void ColorByConstant(unsigned int constant);
  
  //Description:
  //Use to color each cell with a different index.
  //plane 0 = bits 23..0 of ident.
  //plane 1 = bits 47..24 of ident.
  //plane 2 = bits 71..48 of ident.
  void ColorByIncreasingIdent(unsigned int plane);

  //Description:
  //Use the actor lookup table to lookup a color to render with.
  void ColorByActorId(vtkProp *ActorId);

  //Description:
  //Use to color each vertex of each cell by its number.
  void ColorByVertex();

  //Description:
  //Allows you to specify a mapping for selected actor ids.
  void MakeActorLookupTable(vtkProp **Props, vtkIdTypeArray *IdsForProps);

  //Description:
  //Allows you to get the id for selected actor.
  vtkProp* GetActorFromId(vtkIdType id);
  
protected:
  vtkIdentColoredPainter();
  ~vtkIdentColoredPainter();
  
  virtual void RenderInternal(vtkRenderer* renderer, vtkActor* actor,
                              unsigned long typeflags,bool forceCompileOnly);

  void DrawCells(int mode, vtkCellArray *connectivity,
    vtkIdType startCellId, vtkRenderer *renderer);

  vtkIdType TotalCells;

//BTX
  enum {COLORBYIDENT=0, COLORBYCONST, COLORBYVERTEX};
//ETX
  int ColorMode;

  //three 24 bit-fields of the 72 bit increment counter.
  unsigned int Plane;
  unsigned int CurrentIdPlane0;
  unsigned int CurrentIdPlane1;
  unsigned int CurrentIdPlane2;

  void IncrementCurrentId();
  void GetCurrentColor(unsigned char *RGB);

  vtkIdTypeArray *ActorIds;
  vtkProp **PropAddrs;

private:
  vtkIdentColoredPainter(const vtkIdentColoredPainter&); // Not implemented.
  void operator=(const vtkIdentColoredPainter&); // Not implemented.
};

#endif //__vtkIdentColoredPainter_h
