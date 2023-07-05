// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkKMeansAssessFunctor_h
#define vtkKMeansAssessFunctor_h

VTK_ABI_NAMESPACE_BEGIN
class vtkKMeansAssessFunctor : public vtkStatisticsAlgorithm::AssessFunctor
{
  vtkDoubleArray* Distances;
  vtkIdTypeArray* ClusterMemberIDs;
  int NumRuns;

public:
  static vtkKMeansAssessFunctor* New();
  vtkKMeansAssessFunctor() = default;
  ~vtkKMeansAssessFunctor() override;
  void operator()(vtkDoubleArray* result, vtkIdType row) override;
  bool Initialize(vtkTable* inData, vtkTable* reqModel, vtkKMeansDistanceFunctor* distFunc);
  int GetNumberOfRuns() { return NumRuns; }
};

VTK_ABI_NAMESPACE_END
#endif // vtkKMeansAssessFunctor_h
// VTK-HeaderTest-Exclude: vtkKMeansAssessFunctor.h
