#include "vtkKMeansDistanceFunctor.h"

#include "vtkObjectFactory.h"
#include "vtkDoubleArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"
#include "vtkIntArray.h"
#include "vtkIdTypeArray.h"


// ----------------------------------------------------------------------
vtkKMeansDistanceFunctor::~vtkKMeansDistanceFunctor()
{
  if(EmptyTuple) EmptyTuple->Delete();
}

// ----------------------------------------------------------------------
vtkKMeansDistanceFunctor::vtkKMeansDistanceFunctor()
{
  this->EmptyTuple = vtkVariantArray::New();
}

// ----------------------------------------------------------------------
vtkVariantArray* vtkKMeansDefaultDistanceFunctor::GetEmptyTuple( vtkIdType dimension ) 
{
  if(this->EmptyTuple->GetNumberOfValues() != dimension )
    {

    cout << "GetEmptyTuple: START EmptyTuple SetNumberOfValues " << dimension << endl;
    this->EmptyTuple->SetNumberOfValues( dimension );
    cout << "GetEmptyTuple: END EmptyTuple SetNumberOfValues " << dimension << endl;
    for( vtkIdType i = 0; i < dimension; i++ )
      {
      this->EmptyTuple->SetValue( i, 0.0 );
      }
    }
  return this->EmptyTuple; 
} 

// ----------------------------------------------------------------------
vtkKMeansDefaultDistanceFunctor * vtkKMeansDefaultDistanceFunctor::New()
{
return new vtkKMeansDefaultDistanceFunctor;
}

// ----------------------------------------------------------------------
void vtkKMeansDefaultDistanceFunctor::operator() ( double &distance, vtkVariantArray *clusterCoord, 
                                                                     vtkVariantArray *dataCoord)
{

  distance = 0.0;
  if(clusterCoord->GetNumberOfValues() != dataCoord->GetNumberOfValues() )
    {
    cout << "The dimensions of the cluster and data do not match." << endl;
    distance = -1;
    }

  for(vtkIdType i = 0; i < clusterCoord->GetNumberOfValues(); i++ )
    {
    distance += ( clusterCoord->GetValue( i ).ToDouble() - dataCoord->GetValue( i ).ToDouble() ) *
                ( clusterCoord->GetValue( i ).ToDouble() - dataCoord->GetValue( i ).ToDouble() );
    }
}

// ----------------------------------------------------------------------
void vtkKMeansDefaultDistanceFunctor::PairwiseUpdate( vtkTable* clusterCoords,
                                                      vtkIdType rowIndex,
                                                      vtkVariantArray* dataCoord,
                                                      vtkIdType dataCoordCardinality, 
                                                      vtkIdType totalCardinality)
{
  if(clusterCoords->GetNumberOfColumns() != dataCoord->GetNumberOfValues() )
    {
    cout << "The dimensions of the cluster and/or data do not match." << endl;
    return;
    }

  for(vtkIdType i = 0; i < clusterCoords->GetNumberOfColumns(); i++ )
    {
    double curCoord = clusterCoords->GetValue( rowIndex, i ).ToDouble();
    clusterCoords->SetValue( rowIndex, i, curCoord + static_cast<double>(dataCoordCardinality)*
                                          (dataCoord->GetValue( i ).ToDouble() - curCoord)/
                                          static_cast<double>( totalCardinality ) );
    }

}


// ----------------------------------------------------------------------
void vtkKMeansDefaultDistanceFunctor::PerturbElement( vtkTable* newClusterElements, 
                                                      vtkTable* curClusterElements, 
                                                      vtkIdType changeID, 
                                                      vtkIdType startRunID, 
                                                      vtkIdType endRunID,
                                                      double alpha ) 
{
  double numInRange = static_cast<double>( endRunID-startRunID );
  vtkIdType dimension = newClusterElements->GetNumberOfColumns();
  vtkstd::vector<double> perturbedValues( dimension );

  for( vtkIdType i = startRunID; i < endRunID; i++)
    {
    for( vtkIdType j = 0; j < dimension; j++ )
      {
      if( i  == changeID )
        {
        perturbedValues[j] = alpha*curClusterElements->GetValue( i, j ).ToDouble();
        }
      else 
        {
        if ( numInRange > 1.0) 
          {
          perturbedValues[j] = ( 1.0-alpha )/( numInRange-1.0 )*
                               curClusterElements->GetValue( i, j ).ToDouble();
          }
        else
          {
          perturbedValues[j] = ( 1.0-alpha )/( numInRange )*
                               curClusterElements->GetValue( i, j ).ToDouble();
          }
        }
      }
    } 
}

// ----------------------------------------------------------------------
void* vtkKMeansDefaultDistanceFunctor::AllocateElementArray( vtkIdType size )
{
  return new double[size];
}

// ----------------------------------------------------------------------
void vtkKMeansDefaultDistanceFunctor::PackElements( vtkTable* curTable, void* vElements )
{
  vtkIdType numCols = curTable->GetNumberOfColumns();
  vtkIdType numRows = curTable->GetNumberOfRows();
  double* localElements = static_cast< double* >( vElements );
 
  for( vtkIdType col = 0; col < numCols; col++ )
    {
    vtkDoubleArray* doubleArr = vtkDoubleArray::SafeDownCast( curTable->GetColumn( col ) );
    memcpy( &(localElements[col*numRows]), doubleArr->GetPointer( 0 ), numRows*sizeof( double ) );
    }
}


// ----------------------------------------------------------------------
void vtkKMeansDefaultDistanceFunctor::UnPackElements( vtkTable* curTable, vtkTable* newTable, void* vLocalElements, void* vGlobalElements, int np )
{
  double *globalElements = static_cast< double* >( vGlobalElements );
  double *localElements = static_cast< double* >( vLocalElements );
  vtkIdType numCols = curTable->GetNumberOfColumns();
  vtkIdType numRows = curTable->GetNumberOfRows();
  vtkIdType numElements = numCols*numRows;
  for( vtkIdType col = 0; col < numCols; col++ )
    {
    vtkDoubleArray *doubleArr = vtkDoubleArray::New();
    doubleArr->SetName( curTable->GetColumnName( col ) );
    doubleArr->SetNumberOfComponents( 1 );
    doubleArr->SetNumberOfTuples( numRows*np );
    for( int j = 0; j < np; j++ )
      {
      double *ptr = doubleArr->GetPointer(j*numRows);
      memcpy(ptr, &(globalElements[j*numElements+col*numRows]), numRows*sizeof( double ) );
      }
    newTable->AddColumn(doubleArr);
    doubleArr->Delete();
    }
  delete [] localElements; 
  delete [] globalElements; 
}

// ----------------------------------------------------------------------
int vtkKMeansDefaultDistanceFunctor::GetDataType() 
{
  return VTK_DOUBLE;
}
