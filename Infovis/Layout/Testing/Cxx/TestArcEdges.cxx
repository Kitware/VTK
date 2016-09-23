
#include "vtkActor.h"
#include "vtkArcParallelEdgeStrategy.h"
#include "vtkCircularLayoutStrategy.h"
#include "vtkEdgeLayout.h"
#include "vtkForceDirectedLayoutStrategy.h"
#include "vtkGraphLayout.h"
#include "vtkGraphToPolyData.h"
#include "vtkPassThroughEdgeStrategy.h"
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
  VTK_CREATE(vtkEdgeLayout, edgeLayout);
  VTK_CREATE(vtkArcParallelEdgeStrategy, edgeStrategy);
  VTK_CREATE(vtkGraphToPolyData, graphToPoly);
  VTK_CREATE(vtkPolyDataMapper, edgeMapper);
  VTK_CREATE(vtkActor, edgeActor);
  VTK_CREATE(vtkVertexGlyphFilter, vertGlyph);
  VTK_CREATE(vtkPolyDataMapper, vertMapper);
  VTK_CREATE(vtkActor, vertActor);
  VTK_CREATE(vtkRenderer, ren);
  VTK_CREATE(vtkRenderWindow, win);
  win->SetMultiSamples(0);

  VTK_CREATE(vtkRenderWindowInteractor, iren);

  source->SetNumberOfVertices(3);
  source->SetNumberOfEdges(50);
  source->AllowSelfLoopsOn();
  source->AllowParallelEdgesOn();
  source->StartWithTreeOff();
  source->DirectedOff();
  layout->SetInputConnection(source->GetOutputPort());
  layout->SetLayoutStrategy(strategy);
  edgeStrategy->SetNumberOfSubdivisions(50);
  edgeLayout->SetInputConnection(layout->GetOutputPort());
  edgeLayout->SetLayoutStrategy(edgeStrategy);

  // Pull the graph out of the pipeline so we can test the
  // edge points API.
  edgeLayout->Update();
  vtkGraph* g = edgeLayout->GetOutput();
  vtkIdType npts = g->GetNumberOfEdgePoints(0);
  double* pts = new double[3*npts];
  for (vtkIdType i = 0; i < npts; ++i)
  {
    double* pt = g->GetEdgePoint(0, i);
    pts[3*i + 0] = pt[0];
    pts[3*i + 1] = pt[1];
    pts[3*i + 2] = pt[2];
  }
  g->ClearEdgePoints(0);
  for (vtkIdType i = 0; i < npts; ++i)
  {
    double* pt = pts + 3*i;
    g->AddEdgePoint(0, pt);
    g->SetEdgePoint(0, i, pt);
    g->SetEdgePoint(0, i, pt[0], pt[1], pt[2]);
  }
  delete [] pts;

  graphToPoly->SetInputData(g);
  edgeMapper->SetInputConnection(graphToPoly->GetOutputPort());
  edgeActor->SetMapper(edgeMapper);
  ren->AddActor(edgeActor);

  vertGlyph->SetInputData(g);
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

