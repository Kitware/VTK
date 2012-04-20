#include "vtkDIMACSGraphReader.h"
#include "vtkGraph.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"

#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestDIMACSGraphReader(int argc, char* argv[])
{
  VTK_CREATE(vtkDIMACSGraphReader, src_pattern);
  VTK_CREATE(vtkDIMACSGraphReader, src_target);
  VTK_CREATE(vtkDIMACSGraphReader, src_flow);

  char* file_pattern = vtkTestUtilities::ExpandDataFileName(argc, argv,
                           "Data/Infovis/DimacsGraphs/iso_pattern.gr");
  char* file_target = vtkTestUtilities::ExpandDataFileName(argc, argv,
                           "Data/Infovis/DimacsGraphs/iso_target.gr");
  char* file_flow = vtkTestUtilities::ExpandDataFileName(argc, argv,
                           "Data/Infovis/DimacsGraphs/maxflow.max");

  src_pattern->SetFileName(file_pattern);
  src_target->SetFileName(file_target);
  src_flow->SetFileName(file_flow);

  delete[] file_pattern;
  delete[] file_target;
  delete[] file_flow;

  src_pattern->GetFileName();
  src_target->GetFileName();
  src_flow->GetFileName();

  src_pattern->Update();
  src_target->Update();
  src_flow->Update();

  // Do a quick check on the data, the pattern graph should have
  // 5 edges and 5 vertices
  vtkGraph * G = vtkGraph::SafeDownCast( src_pattern->GetOutput() );
  if(G->GetNumberOfVertices() != 5)
    {
    cout << "\tERROR: iso_pattern.gr vertex count wrong. "
         << "Expected 5, Got " << G->GetNumberOfVertices()
         << endl;
    return 1;
    }
  if(G->GetNumberOfEdges() != 5)
    {
    cout << "\tERROR: iso_pattern.gr edge count wrong. "
         << "Expected 5, Got " << G->GetNumberOfEdges()
         << endl;
    return 1;
    }

  return 0;
}
