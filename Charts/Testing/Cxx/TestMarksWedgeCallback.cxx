/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMarksWedgeCallback.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkWedgeMark.h"
#include "vtkBarMark.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkDoubleArray.h"
#include "vtkLineMark.h"
#include "vtkMarkUtil.h"
#include "vtkPanelMark.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTable.h"
#include <functional>
#include <cassert>
#include "vtkCommand.h"
#include "vtkMath.h"

#include "vtkValueHolder.txx"
// explicit instantiation
template class vtkValueHolder<double>;

namespace
{

// Normalized the data in [0,1].
vtkDataElement DataFunction(vtkMark* vtkNotUsed(m), vtkDataElement& d)
{  
  vtkSmartPointer<vtkDoubleArray> a=vtkSmartPointer<vtkDoubleArray>::New();
  double s=0.0;
  int c=d.GetNumberOfChildren();
  int i=0;
  a->SetNumberOfTuples(c);
  while(i<c)
    {
    s+=d.GetValue(i).ToDouble();
    ++i;
    }
  i=0;
  while(i<c)
    {
    a->SetValue(i,d.GetValue(i).ToDouble()/s);
    ++i;
    }
  vtkDataElement result(a);
  return result;
}
  
  // Convert incoming data [0,1] to angles in degrees.
  double AngleFunction(vtkMark* vtkNotUsed(m), vtkDataElement& d)
  {
    return d.GetValue().ToDouble()*360.0;
  }
  
  double LeftFunction(vtkMark *m, vtkDataElement& d)
  {
    vtkIdType i=m->GetIndex();
    
    // this line assume that angle is independant of leftfunction.
    // otherwise, they will be an infinite recursion.
    double *angle=m->GetAngleHolder().GetArray(m);
    
     // Compute start angle. Not efficient but the protovis sibling()
    // has not to been translated into VTK Marks.
    double a0=0.0;
    double a1=0.0;
    vtkIdType j=0;
    while(j<=i)
      {
      a0=a1;
      a1=angle[j]+a0;
      ++j;
      }
    
    vtkIdType o=static_cast<vtkIdType>(m->GetUserVariable("o").GetConstant());
    return 175.0+cos(vtkMath::RadiansFromDegrees(a0+angle[i]/2))*(o==i ? 10 : 0);
  }
  double BottomFunction(vtkMark *m, vtkDataElement& d)
  {
    vtkIdType i=m->GetIndex();
    
    // this line assume that angle is independant of leftfunction.
    // otherwise, they will be an infinite recursion.
    double *angle=m->GetAngleHolder().GetArray(m);
    
    // Compute start angle. Not efficient but the protovis sibling()
    // has not to been translated into VTK Marks.
    double a0=0.0;
    double a1=0.0;
    vtkIdType j=0;
    while(j<=i)
      {
      a0=a1;
      a1=angle[j]+a0;
      ++j;
      }
    
    vtkIdType o=static_cast<vtkIdType>(m->GetUserVariable("o").GetConstant());
    
    // protovis uses clockwise and radians.
    // VTK uses counterclockwise and degrees.
    return 175.0+sin(vtkMath::RadiansFromDegrees(a0+angle[i]/2))*(o==i ? 10 : 0);
  }
  
}

class MyCommand : public vtkCommand
{
public:
  static MyCommand *New()
    {
      return new MyCommand;
    }
  void Execute(vtkObject *caller, unsigned long eventId, 
               void *callData)
    {
      assert("pre: callData_exists" && callData!=0);
      int sectorIndex=*static_cast<int *>(callData);
      vtkWedgeMark *m=static_cast<vtkWedgeMark *>(caller);
      m->SetUserVariable("o",vtkValue<double>(sectorIndex));
      m->GetScene()->SetDirty(true);
    }
protected:
  MyCommand() {}
  ~MyCommand() {}
};

int TestMarksWedgeCallback(int argc, char* argv[])
{
  // Set up a 2D context view, context test object and add it to the scene
  vtkSmartPointer<vtkContextView> view =
    vtkSmartPointer<vtkContextView>::New();
  
  view->GetRenderer()->SetBackground(0.5, 0.0, 0.2);
  view->GetRenderWindow()->SetSize(400, 400);
  view->GetRenderWindow()->SetMultiSamples(0);
  vtkSmartPointer<vtkTable> t = vtkSmartPointer<vtkTable>::New();
  vtkSmartPointer<vtkDoubleArray> arr1 =
    vtkSmartPointer<vtkDoubleArray>::New();
  arr1->SetName("Array1");
  
  double input[]={1, 1.2, 1.7, 1.5, .7};
  
  for (vtkIdType i = 0; i < 5; ++i)
    {
    arr1->InsertNextValue(input[i]);
    }
  
  t->AddColumn(arr1);
  vtkDataElement data(t);
  data.SetDimension(1); // ??

  vtkSmartPointer<vtkPanelMark> panel = vtkSmartPointer<vtkPanelMark>::New();
  view->GetScene()->AddItem(panel);
  panel->SetData(data);
  panel->SetLeft(2);
  panel->SetBottom(2);
  panel->SetWidth(150);
  panel->SetHeight(150);

  vtkMark *wedge=panel->Add(vtkMark::WEDGE);
  wedge->SetData(DataFunction);
  wedge->SetLeft(LeftFunction); // 175.0
  wedge->SetBottom(BottomFunction); // 175.0
  wedge->SetLineWidth(0.0);
  wedge->SetInnerRadius(100.0);
  wedge->SetOuterRadius(140.0);
  
  wedge->SetAngle(AngleFunction);
  wedge->SetLineColor(vtkColor(1.0,1.0,1.0));
  wedge->SetUserVariable("o",vtkValue<double>(-1));
  
  MyCommand *myCommand=MyCommand::New();
  wedge->AddObserver(vtkCommand::EnterEvent,myCommand);
  myCommand->Delete();
  
  view->GetInteractor()->Initialize();

  int retVal = vtkRegressionTestImage(view->GetRenderWindow());
  if(retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    view->GetInteractor()->Start();
    }

  return !retVal;
}
