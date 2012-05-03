
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkEdgeListIterator.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkPassThrough.h"
#include "vtkSmartPointer.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

bool CompareData(vtkGraph* Output, vtkGraph* Input)
{
  bool inputDirected = (vtkDirectedGraph::SafeDownCast(Input) != 0);
  bool outputDirected = (vtkDirectedGraph::SafeDownCast(Output) != 0);
  if(inputDirected != outputDirected)
    {
    cerr << "Directedness not the same" << endl;
    return false;
    }

  if(Input->GetNumberOfVertices() != Output->GetNumberOfVertices())
    {
    cerr << "GetNumberOfVertices not the same" << endl;
    return false;
    }

  if(Input->GetNumberOfEdges() != Output->GetNumberOfEdges())
    {
    cerr << "GetNumberOfEdges not the same" << endl;
    return false;
    }

  if(Input->GetVertexData()->GetNumberOfArrays() != Output->GetVertexData()->GetNumberOfArrays())
    {
    cerr << "GetVertexData()->GetNumberOfArrays() not the same" << endl;
    return false;
    }

  if(Input->GetEdgeData()->GetNumberOfArrays() != Output->GetEdgeData()->GetNumberOfArrays())
    {
    cerr << "GetEdgeData()->GetNumberOfArrays() not the same" << endl;
    return false;
    }

  vtkEdgeListIterator *inputEdges = vtkEdgeListIterator::New();
  vtkEdgeListIterator *outputEdges = vtkEdgeListIterator::New();
  while(inputEdges->HasNext())
    {
    vtkEdgeType inputEdge = inputEdges->Next();
    vtkEdgeType outputEdge = outputEdges->Next();
    if(inputEdge.Source != outputEdge.Source)
      {
      cerr << "Input source != output source" << endl;
      return false;
      }

    if(inputEdge.Target != outputEdge.Target)
      {
      cerr << "Input target != output target" << endl;
      return false;
      }
    }
  inputEdges->Delete();
  outputEdges->Delete();

  return true;
}

int TestPassThrough(int , char* [])
{
  cerr << "Generating graph ..." << endl;
  VTK_CREATE(vtkMutableDirectedGraph, g);
  VTK_CREATE(vtkDoubleArray, x);
  x->SetName("x");
  VTK_CREATE(vtkDoubleArray, y);
  y->SetName("y");
  VTK_CREATE(vtkDoubleArray, z);
  z->SetName("z");
  for (vtkIdType i = 0; i < 10; ++i)
    {
    for (vtkIdType j = 0; j < 10; ++j)
      {
      g->AddVertex();
      x->InsertNextValue(i);
      y->InsertNextValue(j);
      z->InsertNextValue(1);
      }
    }
  g->GetVertexData()->AddArray(x);
  g->GetVertexData()->AddArray(y);
  g->GetVertexData()->AddArray(z);
  cerr << "... done" << endl;

  VTK_CREATE(vtkPassThrough, pass);
  pass->SetInputData(g);
  pass->Update();
  vtkGraph *output = vtkGraph::SafeDownCast(pass->GetOutput());

  if (!CompareData(g, output))
    {
    cerr << "ERROR: Graphs not identical!" << endl;
    return 1;
    }
  return 0;
}
