#include <vtkDataSetAttributes.h>
#include <vtkDoubleArray.h>
#include <vtkMutableUndirectedGraph.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>

#include "vtkBoostExtractLargestComponent.h"

namespace
{
  int TestNormal(vtkMutableUndirectedGraph* g);
  int TestInverse(vtkMutableUndirectedGraph* g);
}

int TestBoostExtractLargestComponent(int, char *[])
{
  // Create a graph
  vtkSmartPointer<vtkMutableUndirectedGraph> g =
    vtkSmartPointer<vtkMutableUndirectedGraph>::New();

  // Add vertices to the graph
  vtkIdType v1 = g->AddVertex();
  vtkIdType v2 = g->AddVertex();
  vtkIdType v3 = g->AddVertex();
  vtkIdType v4 = g->AddVertex();
  vtkIdType v5 = g->AddVertex();
  vtkIdType v6 = g->AddVertex();
  vtkIdType v7 = g->AddVertex();

  // Create one connected component
  g->AddEdge(v1, v2);
  g->AddEdge(v1, v3);

  // Create some disconnected components
  g->AddEdge(v4, v5);
  g->AddEdge(v6, v7);

  std::vector<int> results;

  results.push_back(TestNormal(g));
  results.push_back(TestInverse(g));

  for(unsigned int i = 0; i < results.size(); i++)
  {
    if(results[i] == EXIT_SUCCESS)
    {
      std::cout << "Test " << i << " passed." << std::endl;
    }
    else
    {
      std::cout << "Test " << i << " failed!" << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

namespace
{

int TestNormal(vtkMutableUndirectedGraph* g)
{
  // Test normal operation (extract largest connected component)
  vtkSmartPointer<vtkBoostExtractLargestComponent> filter =
    vtkSmartPointer<vtkBoostExtractLargestComponent>::New();
  filter->SetInputData(g);
  filter->Update();

  if(filter->GetOutput()->GetNumberOfVertices() != 3)
  {
    std::cout << "Size of largest connected component: " << filter->GetOutput()->GetNumberOfVertices()
              << " (Should have been 3)." << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int TestInverse(vtkMutableUndirectedGraph* g)
{
  // Test inverse operation (extract everything but largest connected component)
  vtkSmartPointer<vtkBoostExtractLargestComponent> filter =
    vtkSmartPointer<vtkBoostExtractLargestComponent>::New();
  filter->SetInputData(g);
  filter->SetInvertSelection(true);
  filter->Update();

  if(filter->GetOutput()->GetNumberOfVertices() != 4)
  {
    std::cout << "Size of remainder: " << filter->GetOutput()->GetNumberOfVertices()
              << " (Should have been 4)." << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

} // End anonymous namespace
