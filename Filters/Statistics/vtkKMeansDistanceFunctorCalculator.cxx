#include "vtkKMeansDistanceFunctorCalculator.h"

#include "vtkObjectFactory.h"
#include "vtkDoubleArray.h"
#include "vtkFunctionParser.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"
#include "vtkIntArray.h"
#include "vtkIdTypeArray.h"

#include <sstream>

vtkStandardNewMacro(vtkKMeansDistanceFunctorCalculator);
vtkCxxSetObjectMacro(vtkKMeansDistanceFunctorCalculator,FunctionParser,vtkFunctionParser);

// ----------------------------------------------------------------------
vtkKMeansDistanceFunctorCalculator::vtkKMeansDistanceFunctorCalculator()
{
  this->FunctionParser = vtkFunctionParser::New();
  this->DistanceExpression = 0;
  this->TupleSize = -1;
}

// ----------------------------------------------------------------------
vtkKMeansDistanceFunctorCalculator::~vtkKMeansDistanceFunctorCalculator()
{
  this->SetFunctionParser( 0 );
  this->SetDistanceExpression( 0 );
}

// ----------------------------------------------------------------------
void vtkKMeansDistanceFunctorCalculator::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "FunctionParser: " << this->FunctionParser << "\n";
  os << indent << "DistanceExpression: "
    << ( this->DistanceExpression && this->DistanceExpression[0] ? this->DistanceExpression : "NULL" )
    << "\n";
  os << indent << "TupleSize: " << this->TupleSize << "\n";
}

// ----------------------------------------------------------------------
void vtkKMeansDistanceFunctorCalculator::operator() (
  double& distance, vtkVariantArray* clusterCoord, vtkVariantArray* dataCoord )
{
  distance = 0.0;
  vtkIdType nv = clusterCoord->GetNumberOfValues();
  if ( nv != dataCoord->GetNumberOfValues() )
  {
    cout << "The dimensions of the cluster and data do not match." << endl;
    distance = -1;
    return;
  }

  if ( ! this->DistanceExpression )
  {
    distance = -1;
    return;
  }

  this->FunctionParser->SetFunction( this->DistanceExpression );
  if ( this->TupleSize != nv )
  { // Need to update the scalar variable names as well as values...
    this->FunctionParser->RemoveScalarVariables();
    for ( vtkIdType i = 0; i < nv; ++ i )
    {
      std::ostringstream xos;
      std::ostringstream yos;
      xos << "x" << i;
      yos << "y" << i;
      this->FunctionParser->SetScalarVariableValue( xos.str().c_str(), clusterCoord->GetValue( i ).ToDouble() );
      this->FunctionParser->SetScalarVariableValue( yos.str().c_str(), dataCoord->GetValue( i ).ToDouble() );
    }
  }
  else
  { // Use faster integer comparisons to set values...
    for ( vtkIdType i = 0; i < nv; ++ i )
    {
      this->FunctionParser->SetScalarVariableValue( 2 * i, clusterCoord->GetValue( i ).ToDouble() );
      this->FunctionParser->SetScalarVariableValue( 2 * i + 1, dataCoord->GetValue( i ).ToDouble() );
    }
  }
  distance = this->FunctionParser->GetScalarResult();
  /*
  cout << "f([";
  for ( vtkIdType i = 0; i < nv; ++ i )
    cout << " " << dataCoord->GetValue( i ).ToDouble();
  cout << " ],[";
  for ( vtkIdType i = 0; i < nv; ++ i )
    cout << " " << clusterCoord->GetValue( i ).ToDouble();
  cout << " ]) = " << distance << "\n";
  */
}

