/*=========================================================================

  Program:   Visualization Toolkit
  Module:    GraphItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCommand.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkGraph.h"
#include "vtkGraphItem.h"
#include "vtkRandomGraphSource.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkVariant.h"

#include "vtkObjectFactory.h"

// NOTE: @hcwiley commented this out as its not being used, and causes builds
// to fail with examples on but testing off. Why is this included?
//#include "vtkRegressionTestImage.h"

class GraphAnimate : public vtkCommand
{
public:
  static GraphAnimate* New() { return new GraphAnimate(); }
  vtkTypeMacro(GraphAnimate, vtkCommand);
  void Execute(vtkObject*, unsigned long, void*) override
  {
    this->GraphItem->UpdatePositions();
    this->View->Render();
    this->View->GetRenderWindow()->GetInteractor()->CreateOneShotTimer(10);
  }
  vtkGraphItem* GraphItem;
  vtkContextView* View;
};

//----------------------------------------------------------------------------
int main(int, char*[])
{
  // Set up a 2D context view, context test object and add it to the scene
  vtkSmartPointer<vtkContextView> view = vtkSmartPointer<vtkContextView>::New();
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(800, 600);

  vtkSmartPointer<vtkRandomGraphSource> source = vtkSmartPointer<vtkRandomGraphSource>::New();
  source->SetNumberOfVertices(100);
  source->SetNumberOfEdges(0);
  source->StartWithTreeOn();
  source->Update();
  vtkSmartPointer<vtkGraphItem> item = vtkSmartPointer<vtkGraphItem>::New();
  item->SetGraph(source->GetOutput());
  view->GetScene()->AddItem(item);

  vtkSmartPointer<GraphAnimate> anim = vtkSmartPointer<GraphAnimate>::New();
  anim->View = view;
  anim->GraphItem = item;
  view->GetRenderWindow()->GetInteractor()->Initialize();
  view->GetRenderWindow()->GetInteractor()->CreateOneShotTimer(10);
  view->GetRenderWindow()->GetInteractor()->AddObserver(vtkCommand::TimerEvent, anim);

  view->GetRenderWindow()->GetInteractor()->Start();
}
