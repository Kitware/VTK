/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLineMark.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkLineMark - base class for items that are part of a vtkContextScene.
//
// .SECTION Description
// Derive from this class to create custom items that can be added to a
// vtkContextScene.

#ifndef __vtkLineMark_h
#define __vtkLineMark_h

#include "vtkMark.h"

class vtkBrush;
class vtkDataObject;
class vtkPen;

class VTK_CHARTS_EXPORT vtkLineMark : public vtkMark
{
public:
  vtkTypeMacro(vtkLineMark, vtkMark);
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  static vtkLineMark* New();

  // Description:
  // Paint event for the item, called whenever the item needs to be drawn,
  virtual bool Paint(vtkContext2D *painter);

//BTX
  // Description:
  // Return true if the supplied x, y coordinate is inside the item.
  virtual bool Hit(const vtkContextMouseEvent &mouse);
//ETX

  virtual int GetType() { return LINE; }

//BTX
protected:
  vtkLineMark();
  ~vtkLineMark();

private:
  vtkLineMark(const vtkLineMark &); // Not implemented.
  void operator=(const vtkLineMark &);   // Not implemented.
//ETX
};

#endif //__vtkLineMark_h
