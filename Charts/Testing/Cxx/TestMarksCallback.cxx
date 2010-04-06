/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMarksCallback.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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
#include "vtkCommand.h"
#include <cassert>

namespace
{
  vtkDataElement DataFunction(vtkMark* vtkNotUsed(m), vtkDataElement& d)
  {
    return d;
  }

  double LeftFunction(vtkMark* m, vtkDataElement& vtkNotUsed(d))
  {
    return m->GetIndex()* 20;
  }

  double HeightFunction(vtkMark* vtkNotUsed(m), vtkDataElement& d)
  {
    return d.GetValue().ToDouble()* 80;
  }
  
  vtkColor FillColorFunction(vtkMark *m, vtkDataElement& d)
  {
    vtkIdType idx=m->GetIndex();
    
    vtkIdType i=static_cast<vtkIdType>(m->GetUserVariable("i").GetConstant());
    
    if(idx==i)
      {
      return vtkColor(1.0,0.5,0.0); // orange
      }
    else
      {
      return vtkMarkUtil::DefaultSeriesColorFromParent(m,d); // blue
      }
  }
  
class MyBarCommand : public vtkCommand
{
public:
  static MyBarCommand *New()
    {
      return new MyBarCommand;
    }
  void Execute(vtkObject *caller, unsigned long vtkNotUsed(eventId), 
               void *callData)
    {
      assert("pre: callData_exists" && callData!=0);
      int barIndex=*static_cast<int *>(callData);
      
      cout << "callback from bar index="<< barIndex << endl;
      
      vtkBarMark *m=static_cast<vtkBarMark *>(caller);
      m->SetUserVariable("i",vtkValue<double>(barIndex));
      m->SetUserVariable("j",vtkValue<double>(barIndex));
      m->GetScene()->SetDirty(true);
    }
protected:
  MyBarCommand() {}
  ~MyBarCommand() {}
};

}

int TestMarksCallback(int argc, char* argv[])
{
  // Set up a 2D context view, context test object and add it to the scene
  vtkSmartPointer<vtkContextView> view = vtkSmartPointer<vtkContextView>::New();
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(400, 400);
  view->GetRenderWindow()->SetMultiSamples(0);
  
  vtkSmartPointer<vtkTable> t = vtkSmartPointer<vtkTable>::New();
  vtkSmartPointer<vtkDoubleArray> arr1 = vtkSmartPointer<vtkDoubleArray>::New();
  arr1->SetName("Array1");
  vtkSmartPointer<vtkDoubleArray> arr2 = vtkSmartPointer<vtkDoubleArray>::New();
  arr2->SetName("Array2");
  vtkSmartPointer<vtkDoubleArray> arr3 = vtkSmartPointer<vtkDoubleArray>::New();
  arr3->SetName("Array3");
  for (vtkIdType i = 0; i < 20; ++i)
    {
    arr1->InsertNextValue(sin(i/5.0) + 1);
    arr2->InsertNextValue(cos(i/5.0) + 1);
    arr3->InsertNextValue(i/10.0);
    }
  t->AddColumn(arr1);
  t->AddColumn(arr2);
  t->AddColumn(arr3);

  vtkDataElement data(t);
  data.SetDimension(1);

  vtkSmartPointer<vtkPanelMark> panel = vtkSmartPointer<vtkPanelMark>::New();
  view->GetScene()->AddItem(panel);
  panel->SetData(data);
  panel->SetLeft(2);
  panel->SetBottom(2);

  vtkMark* bar = panel->Add(vtkMark::BAR);
  bar->SetData(DataFunction);
  bar->SetLeft(LeftFunction);
  bar->SetBottom(vtkMarkUtil::StackBottom);
//  bar->SetBottom(0.0);
  bar->SetWidth(15);
  bar->SetHeight(HeightFunction);
  bar->SetUserVariable("i",vtkValue<double>(-1));
  bar->SetFillColor(FillColorFunction);
  
  MyBarCommand *myBarCommand=MyBarCommand::New();
  bar->AddObserver(vtkCommand::EnterEvent,myBarCommand);
  myBarCommand->Delete();
  
  vtkMark* line = panel->Add(vtkMark::LINE);
  line->SetLineColor(vtkMarkUtil::DefaultSeriesColorFromParent);
  line->SetLineWidth(2);
  line->SetBottom(bar->GetHeight());

  view->GetInteractor()->Initialize();

  int retVal = vtkRegressionTestImage(view->GetRenderWindow());
  if(retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    view->GetInteractor()->Start();
    }

  return !retVal;
}
