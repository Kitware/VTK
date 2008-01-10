
#include "vtkActor.h"
#include "vtkIntArray.h"
#include "vtkInteractorStyle.h"
#include "vtkInteractorStyleTreeMapHover.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkSquarifyLayoutStrategy.h"
#include "vtkStdString.h"
#include "vtkTree.h"
#include "vtkTreeFieldAggregator.h"
#include "vtkTreeMapLayout.h"
#include "vtkTreeMapToPolyData.h"

#include <vtksys/stl/vector>

using vtksys_stl::vector;

#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

// Main testing proc
void TestStyle(vtkInteractorStyle *style)
{
  VTK_CREATE(vtkMath, m);
  m->RandomSeed(1);
  
  vector<vtkStdString> buttons;
  buttons.push_back("Left");
  buttons.push_back("Middle");
  buttons.push_back("Right");
  
  int useTimers = style->GetUseTimers();
  style->UseTimersOff();
  style->AutoAdjustCameraClippingRangeOn();

  cerr << "Testing: " << style->GetClassName() << endl;

  vtkRenderWindowInteractor *iren = style->GetInteractor();
  vtkRenderWindow *renwin = iren->GetRenderWindow();

  renwin->Render();

  // Get renwin size and center
  int *win_size = renwin->GetSize();
  double win_center_x = win_size[0] / 2.0;
  double win_center_y = win_size[1] / 2.0;

  double start_x = win_center_x;
  double start_y = win_center_y;

  double radius = 20.0;

  for (int ctrl = 0; ctrl < 2; ++ctrl)
    {
    for (int shift = 0; shift < 2; ++shift)
      {
      for (int button = 0; button < 3; ++button)
        {
        cerr << " " << buttons[button];

        // Start by pressing the button
        iren->SetEventInformationFlipY(
                static_cast<int>(start_x), static_cast<int>(start_y), ctrl, shift, 0, 0, 0);
        vtkStdString pressEvent = buttons[button] + "ButtonPressEvent";
        iren->InvokeEvent(pressEvent.c_str());

        // Now move around (alternating left and right around
        // the window center in order to compensate somehow).

        int sign = 1;
        int x = 0;
        int y = 0;
        for (int i = 0; i < 5; ++i)
          {
          sign = sign * -1;
          x = static_cast<int>(m->Random(
            win_center_x + radius * 2 * sign,
            win_center_x + radius * sign));
          y = static_cast<int>(m->Random(
            win_center_y + radius * 2 * sign,
            win_center_y + radius * sign));
          iren->SetEventInformationFlipY(x, y, ctrl, shift, 0, 0, 0);
          iren->InvokeEvent("MouseMoveEvent");

          // If this style use timers, run OnTimer multiple times
          if (useTimers) 
            {
            for (int j = 0; j < 10; ++j)
              {
              iren->InvokeEvent("TimerEvent");
              }
            }
          renwin->Render();
          }

        // End by releasing the button
        iren->SetEventInformationFlipY(x, y, ctrl, shift, 0, 0, 0);
        vtkStdString releaseEvent = buttons[button] + "ButtonReleaseEvent";
        iren->InvokeEvent(releaseEvent.c_str());
        }
      cerr << "." << endl;
      }
    }
  
  style->SetUseTimers(useTimers);
  renwin->Render();
}


int TestInteractorStyleTreeMapHover(int argc, char* argv[])
{
  // Create input
  VTK_CREATE(vtkTree, tree);
  VTK_CREATE(vtkIntArray, sizeArr);
  sizeArr->SetName("size");
  tree->GetVertexData()->AddArray(sizeArr);
  tree->AddRoot();
  sizeArr->InsertNextValue(0);
  tree->AddChild(0);
  sizeArr->InsertNextValue(15);
  tree->AddChild(0);
  sizeArr->InsertNextValue(50);
  tree->AddChild(0);
  sizeArr->InsertNextValue(0);
  tree->AddChild(3);
  sizeArr->InsertNextValue(2);
  tree->AddChild(3);
  sizeArr->InsertNextValue(12);
  tree->AddChild(3);
  sizeArr->InsertNextValue(10);
  tree->AddChild(3);
  sizeArr->InsertNextValue(8);
  tree->AddChild(3);
  sizeArr->InsertNextValue(6);
  tree->AddChild(3);
  sizeArr->InsertNextValue(4);
  
  VTK_CREATE(vtkTreeFieldAggregator, agg);
  agg->SetInput(tree);
  agg->SetField("size");
  agg->SetLeafVertexUnitSize(false);

  VTK_CREATE(vtkTreeMapLayout, layout);
  VTK_CREATE(vtkSquarifyLayoutStrategy, box);
  box->SetBorderPercentage(0.1);
  layout->SetInputConnection(agg->GetOutputPort());
  layout->SetLayoutStrategy(box);

  VTK_CREATE(vtkTreeMapToPolyData, poly);
  poly->SetInputConnection(layout->GetOutputPort());

  VTK_CREATE(vtkPolyDataMapper, mapper);
  mapper->SetInputConnection(poly->GetOutputPort());
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SelectColorArray("size");
  mapper->SetScalarRange(0, 100);

  VTK_CREATE(vtkActor, actor);
  actor->SetMapper(mapper);

  VTK_CREATE(vtkRenderWindow, win);
  VTK_CREATE(vtkRenderer, ren);
  ren->AddActor(actor);
  win->AddRenderer(ren);
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  VTK_CREATE(vtkInteractorStyleTreeMapHover, hover);
  hover->SetLabelField("size");
  hover->SetTreeMapToPolyData(poly);
  hover->SetLayout(layout);

  win->SetInteractor(iren);
  iren->SetInteractorStyle(hover);

  TestStyle(hover);

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    win->Render();
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
    }

  return !retVal;
}
