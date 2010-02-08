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
#include "vtkSmartPointer.h"

#include <vector>

class vtkBrush;
class vtkDataObject;
class vtkPen;

class VTK_CHARTS_EXPORT vtkPanelMark : public vtkMark
{
public:
  vtkTypeRevisionMacro(vtkPanelMark, vtkMark);
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  static vtkPanelMark* New();

  //virtual void Add(vtkMark* m);
  virtual vtkMark* Add(int type);

  virtual bool Paint(vtkContext2D*);

  virtual void Update();

  virtual vtkMark* GetMarkInstance(int markIndex, int dataIndex)
    {
    vtkDataElement data = this->Data.GetData(this);
    vtkIdType numChildren = data.GetNumberOfChildren();
    return this->MarkInstances[markIndex*numChildren + dataIndex];
    }

//BTX
protected:
  vtkPanelMark();
  ~vtkPanelMark();

  std::vector<vtkSmartPointer<vtkMark> > Marks;
  std::vector<vtkSmartPointer<vtkMark> > MarkInstances;

private:
  vtkPanelMark(const vtkPanelMark &); // Not implemented.
  void operator=(const vtkPanelMark &);   // Not implemented.
//ETX
};

#endif //__vtkPanelMark_h
