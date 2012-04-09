#ifndef __vtkMultiCorrelativeStatisticsAssessFunctor_h
#define __vtkMultiCorrelativeStatisticsAssessFunctor_h

#include "vtkStatisticsAlgorithm.h"

#include <vector>

class vtkDataArray;
class vtkTable;

#define VTK_MULTICORRELATIVE_KEYCOLUMN1 "Column1"
#define VTK_MULTICORRELATIVE_KEYCOLUMN2 "Column2"
#define VTK_MULTICORRELATIVE_ENTRIESCOL "Entries"
#define VTK_MULTICORRELATIVE_AVERAGECOL "Mean"
#define VTK_MULTICORRELATIVE_COLUMNAMES "Column"

class vtkMultiCorrelativeAssessFunctor : public vtkStatisticsAlgorithm::AssessFunctor
{
public:
  static vtkMultiCorrelativeAssessFunctor* New();

  vtkMultiCorrelativeAssessFunctor() { }
  virtual ~vtkMultiCorrelativeAssessFunctor() { }
  virtual bool Initialize( vtkTable* inData, vtkTable* reqModel, bool cholesky = true );

  virtual void operator () ( vtkVariantArray* result, vtkIdType row );

  vtkIdType GetNumberOfColumns() { return static_cast<vtkIdType>( this->Columns.size() ); }
  vtkDataArray* GetColumn( vtkIdType colIdx ) { return this->Columns[colIdx]; }

  std::vector<vtkDataArray*> Columns; // Source of data
  double* Center; // Offset per column (usu. to re-center the data about the mean)
  std::vector<double> Factor; // Weights per column
  //double Normalization; // Scale factor for the volume under a multivariate Gaussian used to normalize the CDF
  std::vector<double> Tuple; // Place to store product of detrended input tuple and Cholesky inverse
  std::vector<double> EmptyTuple; // Used to quickly initialize Tuple for each datum
};

#endif // __vtkMultiCorrelativeStatisticsAssessFunctor_h
// VTK-HeaderTest-Exclude: vtkMultiCorrelativeStatisticsAssessFunctor.h
