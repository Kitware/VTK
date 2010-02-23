
#include "vtkIntArray.h"
#include "vtkSmartPointer.h"
#include "vtkStreamGraph.h"
#include "vtkTable.h"
#include "vtkTableToGraph.h"

int TestStreamGraph(int, char*[])
{
  vtkSmartPointer<vtkIntArray> src = vtkSmartPointer<vtkIntArray>::New();
  src->SetName("source");
  src->SetNumberOfTuples(1);
  vtkSmartPointer<vtkIntArray> tgt = vtkSmartPointer<vtkIntArray>::New();
  tgt->SetName("target");
  tgt->SetNumberOfTuples(1);
  vtkSmartPointer<vtkTable> table = vtkSmartPointer<vtkTable>::New();
  table->AddColumn(src);
  table->AddColumn(tgt);
  vtkSmartPointer<vtkTableToGraph> t2g = vtkSmartPointer<vtkTableToGraph>::New();
  t2g->SetInput(table);
  t2g->AddLinkVertex("source");
  t2g->AddLinkVertex("target");
  t2g->AddLinkEdge("source", "target");
  t2g->SetDirected(true);
  vtkSmartPointer<vtkStreamGraph> stream = vtkSmartPointer<vtkStreamGraph>::New();
  stream->SetInputConnection(t2g->GetOutputPort());
  for (int i = 0; i < 10; ++i)
    {
    src->SetValue(0, i);
    tgt->SetValue(0, i+1);
    t2g->Modified();
    stream->Update();
    stream->GetOutput()->Dump();
    }

  vtkGraph* output = stream->GetOutput();
  if (output->GetNumberOfVertices() != 11 || output->GetNumberOfEdges() != 10)
    {
    cerr << "ERROR: Incorrect number of vertices/edges." << endl;
    return 1;
    }

  return 0;
}
