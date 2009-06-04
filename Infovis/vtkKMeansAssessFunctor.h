#ifndef __vtkKMeansAssessFunctor_h
#define __vtkKMeansAssessFunctor_h

#include "vtkStatisticsAlgorithm.h"

#include <vtkstd/vector>

class vtkDataArray;
class vtkIdTypeArray;
class vtkTable;
class vtkKMeansDistanceFunctor;

class vtkKMeansAssessFunctor : public vtkStatisticsAlgorithm::AssessFunctor
{
  vtkDoubleArray* Distances;
  vtkIntArray* ClusterMemberIDs;
  int NumRuns;
  
public:
  static vtkKMeansAssessFunctor* New();
  vtkKMeansAssessFunctor() { }
  virtual ~vtkKMeansAssessFunctor();
  virtual void operator () ( vtkVariantArray* result, vtkIdType row );
  bool Initialize( vtkTable *inData, vtkTable *reqModel, vtkKMeansDistanceFunctor *distFunc );
  int GetNumberOfRuns() { return NumRuns; }
};

#endif // __vtkKMeansAssessFunctor_h
