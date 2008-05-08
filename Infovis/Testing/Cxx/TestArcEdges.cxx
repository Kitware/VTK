
#include "vtkActor.h"
#include "vtkCircularLayoutStrategy.h"
#include "vtkForceDirectedLayoutStrategy.h"
#include "vtkGraphLayout.h"
#include "vtkGraphToPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRandomGraphSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkVertexGlyphFilter.h"

#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestArcEdges(int argc, char* argv[])
{
  VTK_CREATE(vtkRandomGraphSource, source);
  VTK_CREATE(vtkGraphLayout, layout);
  VTK_CREATE(vtkCircularLayoutStrategy, strategy);
  VTK_CREATE(vtkGraphToPolyData, graphToPoly);
  VTK_CREATE(vtkPolyDataMapper, edgeMapper);
  VTK_CREATE(vtkActor, edgeActor);
  VTK_CREATE(vtkGraphToPolyData, graphToPoints);
  VTK_CREATE(vtkVertexGlyphFilter, vertGlyph);
  VTK_CREATE(vtkPolyDataMapper, vertMapper);
  VTK_CREATE(vtkActor, vertActor);
  VTK_CREATE(vtkRenderer, ren);
  VTK_CREATE(vtkRenderWindow, win);
  VTK_CREATE(vtkRenderWindowInteractor, iren);

  source->SetNumberOfVertices(3);
  source->SetNumberOfEdges(50);
  source->AllowSelfLoopsOn();
  source->AllowParallelEdgesOn();
  source->StartWithTreeOff();
  source->DirectedOff();
  layout->SetInputConnection(source->GetOutputPort());
  layout->SetLayoutStrategy(strategy);
  graphToPoly->SetInputConnection(layout->GetOutputPort());
  graphToPoly->ArcEdgesOn();
  graphToPoly->SetNumberOfArcSubdivisions(50);
  edgeMapper->SetInputConnection(graphToPoly->GetOutputPort());
  edgeActor->SetMapper(edgeMapper);
  ren->AddActor(edgeActor);


  graphToPoints->SetInputConnection(layout->GetOutputPort());
  vertGlyph->SetInputConnection(graphToPoints->GetOutputPort());
  vertMapper->SetInputConnection(vertGlyph->GetOutputPort());
  vertActor->SetMapper(vertMapper);
  vertActor->GetProperty()->SetPointSize(1);
  ren->AddActor(vertActor);

  win->AddRenderer(ren);
  win->SetInteractor(iren);
  win->Render();

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Initialize();
    iren->Start();

    retVal = vtkRegressionTester::PASSED;
    }

  return !retVal;
}

