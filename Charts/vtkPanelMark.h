/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPanelMark.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkPanelMark - base class for items that are part of a vtkContextScene.
//
// .SECTION Description
// Derive from this class to create custom items that can be added to a
// vtkContextScene.

#ifndef __vtkPanelMark_h
#define __vtkPanelMark_h

#include "vtkMark.h"

#include <vector> // STL required

class vtkBrush;
class vtkDataObject;
class vtkPen;
class vtkContextBufferId;

class VTK_CHARTS_EXPORT vtkPanelMark : public vtkMark
{
public:
  vtkTypeRevisionMacro(vtkPanelMark, vtkMark);
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  static vtkPanelMark* New();

  //virtual void Add(vtkMark* m);
  virtual vtkMark* Add(int type);
  
  // Description:
  // Return the index of the mark template `m' in the list of marks of the
  // panel. Return -1 if not found.
  // \pre m_exists: m!=0
  // \post valid_result: result>=-1 && result<this->Marks.size()
  virtual vtkIdType FindIndex(vtkMark *m);
  
  virtual bool Paint(vtkContext2D*);

  virtual void Update();

  virtual vtkMark* GetMarkInstance(int markIndex, int dataIndex)
    {
    vtkDataElement data = this->Data.GetData(this);
    vtkIdType numChildren = data.GetNumberOfChildren();
    return this->MarkInstances[markIndex*numChildren + dataIndex];
    }
  
  
//BTX
  // Description:
  // Paint the marks in a special mode to build a cache for picking.
  // Use internally.
  void PaintIds();
  
  // Description:
  // Make sure the buffer id for the children items is up-to-date.
  void UpdateBufferId();
  
  // Description:
  // Return the item under mouse cursor at x,y.
  vtkIdType GetPickedItem(int x, int y);
  
  // Description:
  // Mouse enter event. As Panel is container, it propagates the event to
  // its children.
  // Return true if the item holds the event, false if the event can be
  // propagated to other items.
  virtual bool MouseEnterEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse move event. As Panel is container, it propagates the event to
  // its children.
  // Return true if the item holds the event, false if the event can be
  // propagated to other items.
  virtual bool MouseMoveEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse leave event. As Panel is container, it propagates the event to
  // its children.
  // Return true if the item holds the event, false if the event can be
  // propagated to other items.
  virtual bool MouseLeaveEvent(const vtkContextMouseEvent &mouse);
  
  // Description:
  // Return true if the supplied x, y coordinate is inside the item.
  // As Panel is container, it delegates the call to its children.
  virtual bool Hit(const vtkContextMouseEvent &mouse);
  
  // Descrition:
  // PaintId mode for the submark template `m'.
  // Called by `m'.
  // \pre m_exists: m!=0
  virtual void PaintIdsOfMark(vtkMark *m);
  
//ETX
  
//BTX
protected:
  vtkPanelMark();
  ~vtkPanelMark();

  vtkstd::vector<vtkSmartPointer<vtkMark> > Marks;
  vtkstd::vector<vtkSmartPointer<vtkMark> > MarkInstances;

  bool MouseOver; // tell if the mouse cursor entered the panel
  
  vtkContextBufferId *BufferId;
  
  int ActiveItem;
  
private:
  vtkPanelMark(const vtkPanelMark &); // Not implemented.
  void operator=(const vtkPanelMark &);   // Not implemented.
//ETX
};

#endif //__vtkPanelMark_h
