#include "vtkIncrementalOctreeNode.h"
#include "vtkIncrementalOctreePointLocator.h"
#include "vtkNew.h"
#include "vtkPoints.h"

#include <array>

int TestSimpleIncrementalOctreePointLocator(int, char** const)
{
  std::array<double, 6> bb = { 0, 1, 0, 1, 0, 1 };
  std::array<std::array<double, 3>, 18> pointsInput;
  for (size_t i = 0; i < pointsInput.size(); ++i)
  {
    for (size_t j = 0; j < pointsInput[i].size(); ++j)
      pointsInput[i][j] = (static_cast<double>(i + 1)) / (pointsInput.size() + 1);
  }
  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  vtkNew<vtkIncrementalOctreePointLocator> octree;
  // that is the minimum
  octree->SetMaxPointsPerLeaf(16);
  octree->InitPointInsertion(points, bb.data());
  for (auto point : pointsInput)
  {
    octree->InsertNextPoint(point.data());
  }
  // we expect the same number points
  if (static_cast<size_t>(points->GetNumberOfPoints()) != pointsInput.size())
  {
    std::cerr << "Wrong number of points: " << points->GetNumberOfPoints()
              << " expected: " << pointsInput.size() << std::endl;
    return EXIT_FAILURE;
  }
  // 2 levels
  int numberOfLevels = octree->GetNumberOfLevels();
  if (numberOfLevels != 2)
  {
    std::cerr << "Got number of levels: " << numberOfLevels << " expected: " << 2 << std::endl;
    return EXIT_FAILURE;
  }
  int numberOfNodes = octree->GetNumberOfNodes();
  if (numberOfNodes != 9)
  {
    std::cerr << "Got number of nodes: " << numberOfNodes << " expected: " << 9 << std::endl;
  }
  vtkIncrementalOctreeNode* root = octree->GetRoot();
  if (root->GetID() != 0)
  {
    std::cerr << "Expected node ID 0 but got: " << root->GetID() << std::endl;
    return EXIT_FAILURE;
  }
  for (int i = 0; i < 8; ++i)
  {
    vtkIncrementalOctreeNode* node = root->GetChild(i);
    if (node->GetID() != i + 1)
    {
      std::cerr << "Expected node ID " << (i + 1) << " but got: " << node->GetID() << std::endl;
      return EXIT_FAILURE;
    }
  }
  vtkIdType id = octree->FindClosestPoint(0, 0, 0);
  if (id != 0)
  {
    std::cerr << "Point closes to 0 should be at index 0 but it is at: " << id << std::endl;
    return EXIT_FAILURE;
  }
  double* p = points->GetPoint(id);
  std::cout << "Point: " << p[0] << ", " << p[1] << ", " << p[2] << std::endl;

  return EXIT_SUCCESS;
}
