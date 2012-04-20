
#include "vtkDataSetAttributes.h"
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
  vtkSmartPointer<vtkIntArray> time = vtkSmartPointer<vtkIntArray>::New();
  time->SetName("time");
  time->SetNumberOfTuples(1);
  vtkSmartPointer<vtkTable> table = vtkSmartPointer<vtkTable>::New();
  table->AddColumn(src);
  table->AddColumn(tgt);
  table->AddColumn(time);
  vtkSmartPointer<vtkTableToGraph> t2g = vtkSmartPointer<vtkTableToGraph>::New();
  t2g->SetInputData(table);
  t2g->AddLinkVertex("source");
  t2g->AddLinkVertex("target");
  t2g->AddLinkEdge("source", "target");
  t2g->SetDirected(true);
  vtkSmartPointer<vtkStreamGraph> stream = vtkSmartPointer<vtkStreamGraph>::New();
  stream->SetInputConnection(t2g->GetOutputPort());
  stream->UseEdgeWindowOn();
  stream->SetEdgeWindow(5);
  stream->SetEdgeWindowArrayName("time");
  for (int i = 0; i < 10; ++i)
    {
    src->SetValue(0, i);
    tgt->SetValue(0, i+1);
    time->SetValue(0, i);
    t2g->Modified();
    stream->Update();
    stream->GetOutput()->Dump();
    vtkSmartPointer<vtkTable> edgeTable = vtkSmartPointer<vtkTable>::New();
    edgeTable->SetRowData(stream->GetOutput()->GetEdgeData());
    edgeTable->Dump();
    }

  vtkGraph* output = stream->GetOutput();
  if (output->GetNumberOfVertices() != 11 || output->GetNumberOfEdges() != 6)
    {
    cerr << "ERROR: Incorrect number of vertices/edges." << endl;
    return 1;
    }
  vtkDataArray* outputTime = output->GetEdgeData()->GetArray("time");
  double timeRange[2];
  outputTime->GetRange(timeRange);
  if (timeRange[0] != 4 || timeRange[1] != 9)
    {
    cerr << "ERROR: Incorrect time range." << endl;
    return 1;
    }

  return 0;
}
