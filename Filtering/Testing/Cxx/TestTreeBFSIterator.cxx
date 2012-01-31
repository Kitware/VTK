#include <vtkMutableDirectedGraph.h>
#include <vtkTree.h>
#include <vtkNew.h>

#include <vector>

#include "vtkTreeBFSIterator.h"

int TestTreeBFSIterator(int, char *[])
{
  vtkNew<vtkMutableDirectedGraph> g;

  // Create vertices:
  // Level 0
  vtkIdType v0 = g->AddVertex();
  // Level 1
  vtkIdType v1 = g->AddVertex();
  vtkIdType v2 = g->AddVertex();
  // Level 2
  vtkIdType v3 = g->AddVertex();
  vtkIdType v4 = g->AddVertex();
  vtkIdType v5 = g->AddVertex();
  // Level 3
  vtkIdType v6 = g->AddVertex();
  vtkIdType v7 = g->AddVertex();
  vtkIdType v8 = g->AddVertex();

  //create a fully connected graph
  g->AddEdge(v0, v1);
  g->AddEdge(v0, v2);
  g->AddEdge(v1, v3);
  g->AddEdge(v2, v4);
  g->AddEdge(v2, v5);
  g->AddEdge(v4, v6);
  g->AddEdge(v4, v7);
  g->AddEdge(v5, v8);

  vtkNew<vtkTree> tree;
  tree->CheckedShallowCopy(g.GetPointer());

  std::vector<int> correctSequence;
  for(int i = 0; i <= 8; i++)
    {
    correctSequence.push_back(i);
    }

  vtkNew<vtkTreeBFSIterator> bfsIterator;
  bfsIterator->SetTree(tree.GetPointer());

  if(bfsIterator->GetStartVertex() != tree->GetRoot())
    {
    cout << "StartVertex is not defaulting to root" << endl;
    return EXIT_FAILURE;
    }

  //traverse the tree in a depth first fashion
  for(size_t i = 0; i < correctSequence.size(); i++)
    {
    if(!bfsIterator->HasNext())
      {
      cout << "HasNext() returned false before the end of the tree" << endl;
      return EXIT_FAILURE;
      }

    vtkIdType nextVertex = bfsIterator->Next();
    if(nextVertex != correctSequence[i])
      {
      cout << "Next vertex should be " << correctSequence[i] << " but it is " << nextVertex << endl;
      return EXIT_FAILURE;
      }
    }

  return EXIT_SUCCESS;
}
