#include <vtkMutableDirectedGraph.h>
#include <vtkTree.h>
#include <vtkSmartPointer.h>

#include <vtkstd/vector>

#include "vtkTreeBFSIterator.h"

int TestTreeBFSIterator(int, char *[])
{
  vtkSmartPointer<vtkMutableDirectedGraph> g =
    vtkSmartPointer<vtkMutableDirectedGraph>::New();

  //create 3 vertices
  vtkIdType v0 = g->AddVertex();
  vtkIdType v1 = g->AddVertex();
  vtkIdType v2 = g->AddVertex();
  vtkIdType v3 = g->AddVertex();

  //create a fully connected graph
  g->AddEdge(v0, v1);
  g->AddEdge(v0, v2);
  g->AddEdge(v1, v3);

  vtkSmartPointer<vtkTree> tree =
      vtkSmartPointer<vtkTree>::New();
  tree->CheckedShallowCopy(g);

  vtkstd::vector<int> correctSequence;
  correctSequence.push_back(0);
  correctSequence.push_back(1);
  correctSequence.push_back(2);
  correctSequence.push_back(3);

  vtkIdType root = tree->GetRoot();

  vtkSmartPointer<vtkTreeBFSIterator> bfsIterator =
      vtkSmartPointer<vtkTreeBFSIterator>::New();
  bfsIterator->SetStartVertex(root);
  bfsIterator->SetTree(tree);

  int i = 0;
  //traverse the tree in a depth first fashion
  while(bfsIterator->HasNext())
    {
    vtkIdType nextVertex = bfsIterator->Next();
    if(nextVertex != correctSequence[i])
      {
      cout << "Next vertex should be " << correctSequence[i] << " but it is " << nextVertex << endl;
      return EXIT_FAILURE;
      }
    i++;
    }

  return EXIT_SUCCESS;
}
