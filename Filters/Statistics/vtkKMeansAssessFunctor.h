#ifndef vtkKMeansAssessFunctor_h
#define vtkKMeansAssessFunctor_h

class vtkKMeansAssessFunctor : public vtkStatisticsAlgorithm::AssessFunctor
{
  vtkDoubleArray* Distances;
  vtkIdTypeArray* ClusterMemberIDs;
  int NumRuns;

public:
  static vtkKMeansAssessFunctor* New();
  vtkKMeansAssessFunctor() { }
  ~vtkKMeansAssessFunctor() VTK_OVERRIDE;
  void operator () ( vtkDoubleArray* result, vtkIdType row ) VTK_OVERRIDE;
  bool Initialize( vtkTable *inData, vtkTable *reqModel, vtkKMeansDistanceFunctor *distFunc );
  int GetNumberOfRuns() { return NumRuns; }
};

#endif // vtkKMeansAssessFunctor_h
// VTK-HeaderTest-Exclude: vtkKMeansAssessFunctor.h
