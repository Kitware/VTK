/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMark.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkMark - base class for items that are part of a vtkContextScene.
//
// .SECTION Description
// Derive from this class to create custom items that can be added to a
// vtkContextScene.

#ifndef __vtkMark_h
#define __vtkMark_h

#include "vtkContextItem.h"
#include "vtkDataElement.h"
#include "vtkDataValue.h"
#include "vtkSmartPointer.h"
#include "vtkVariant.h"
#include "vtkValueHolder.h"

class vtkPanelMark;

class vtkColor
{
public:
  vtkColor() :
    Red(0.0), Green(0.0), Blue(0.0), Alpha(1.0) { }
  vtkColor(double r, double g, double b) :
    Red(r), Green(g), Blue(b), Alpha(1.0) { }
  vtkColor(double r, double g, double b, double a) :
    Red(r), Green(g), Blue(b), Alpha(a) { }
  double Red;
  double Green;
  double Blue;
  double Alpha;
};

class VTK_CHARTS_EXPORT vtkMark : public vtkContextItem
{
public:
  vtkTypeRevisionMacro(vtkMark, vtkContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  static vtkMark* New();

  enum {
    BAR,
    LINE
    };

  static vtkMark* CreateMark(int type);

  virtual void Extend(vtkMark* m);

  virtual bool Paint(vtkContext2D* vtkNotUsed(painter)) { return true; }

  virtual void Update();

  void SetData(vtkDataValue data);
  vtkDataValue GetData();

  void SetLeft(vtkValue<double> v);
  vtkValue<double>& GetLeft();

  void SetRight(vtkValue<double> v);
  vtkValue<double>& GetRight();

  void SetTop(vtkValue<double> v);
  vtkValue<double>& GetTop();

  void SetBottom(vtkValue<double> v);
  vtkValue<double>& GetBottom();

  void SetTitle(vtkValue<std::string> v);
  vtkValue<std::string>& GetTitle();

  void SetLineColor(vtkValue<vtkColor> v);
  vtkValue<vtkColor>& GetLineColor();

  void SetFillColor(vtkValue<vtkColor> v);
  vtkValue<vtkColor>& GetFillColor();

  void SetLineWidth(vtkValue<double> v);
  vtkValue<double>& GetLineWidth();

  void SetWidth(vtkValue<double> v);
  vtkValue<double>& GetWidth();

  void SetHeight(vtkValue<double> v);
  vtkValue<double>& GetHeight();

  void SetParent(vtkPanelMark* p);
  vtkPanelMark* GetParent();

  vtkSetMacro(ParentMarkIndex, int);
  vtkGetMacro(ParentMarkIndex, int);

  vtkSetMacro(ParentDataIndex, int);
  vtkGetMacro(ParentDataIndex, int);

  void SetIndex(vtkIdType i);
  vtkIdType GetIndex();

  double GetCousinLeft();
  double GetCousinRight();
  double GetCousinTop();
  double GetCousinBottom();
  double GetCousinWidth();
  double GetCousinHeight();

  virtual void DataChanged();
  virtual int GetType();

//BTX
protected:
  vtkMark();
  ~vtkMark();

  bool PaintBar(vtkContext2D *painter);
  bool PaintLine(vtkContext2D *painter);

  vtkDataValue Data;
  vtkValueHolder<double> Left;
  vtkValueHolder<double> Right;
  vtkValueHolder<double> Top;
  vtkValueHolder<double> Bottom;
  vtkValueHolder<std::string> Title;
  vtkValueHolder<vtkColor> FillColor;
  vtkValueHolder<vtkColor> LineColor;
  vtkValueHolder<double> LineWidth;
  vtkValueHolder<double> Width;
  vtkValueHolder<double> Height;

  vtkPanelMark* Parent;
  vtkIdType ParentMarkIndex;
  vtkIdType ParentDataIndex;
  vtkIdType Index;

private:
  vtkMark(const vtkMark &); // Not implemented.
  void operator=(const vtkMark &);   // Not implemented.
//ETX
};

#endif //__vtkMark_h
