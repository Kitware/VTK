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

namespace
{
  double PanelLeftFunction(vtkMark* m, vtkDataElement& vtkNotUsed(d))
  {
    return 20 + m->GetIndex()*15;
  }

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
}

int TestMarks(int argc, char* argv[])
{
  // Set up a 2D context view, context test object and add it to the scene
  vtkSmartPointer<vtkContextView> view = vtkSmartPointer<vtkContextView>::New();
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(400, 400);

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
  //panel->SetLeft(PanelLeftFunction);
  panel->SetLeft(2);
  panel->SetBottom(2);

  vtkMark* bar = panel->Add(vtkMark::BAR);
  bar->SetData(DataFunction);
  bar->SetLeft(LeftFunction);
  bar->SetBottom(vtkMarkUtil::StackBottom);
  //bar->SetBottom(0.0);
  bar->SetWidth(15);
  bar->SetHeight(HeightFunction);

  vtkMark* line = panel->Add(vtkMark::LINE);
  line->SetLineColor(vtkMarkUtil::DefaultSeriesColor);
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
