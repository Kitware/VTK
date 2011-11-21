#include "vtkLinearExtractionFilter.h"

#include <vtkObjectFactory.h>
#include <vtkCell.h>
#include <vtkDataSet.h>
#include <vtkDoubleArray.h>
#include <vtkIdTypeArray.h>
#include <vtkCellData.h>
#include <vtkPointData.h>
#include <vtkMath.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkSelection.h>
#include <vtkSelectionNode.h>
#include <vtkSmartPointer.h>
#include <vtkCompositeDataSet.h>
#include <vtkCompositeDataIterator.h>

#include <assert.h>

#include <vtksys/vector>
#include <vtksys/algorithm>

vtkStandardNewMacro(vtkLinearExtractionFilter);

// ----------------------------------------------------------------------
vtkLinearExtractionFilter::vtkLinearExtractionFilter ()
{
  this->Tolerance = 0. ;
  this->SetStartPoint( 0., 0., 0. );
  this->SetEndPoint( 1., 1. ,1. );
}

// ----------------------------------------------------------------------
vtkLinearExtractionFilter::~vtkLinearExtractionFilter ()
{
}

// ----------------------------------------------------------------------
void vtkLinearExtractionFilter::PrintSelf ( ostream& os, vtkIndent indent )
{
  Superclass::PrintSelf( os,indent );

  os << indent
     << "Point 1   : ("
     << this->StartPoint[0]
     << ", "
     << this->StartPoint [1]
     << ", "
     << this->StartPoint [2]
     << ")\n";

  os << indent
     << "Point 2   : ("
     << this->EndPoint [0]
     << ", "
     << this->EndPoint [1]
     << ", "
     << this->EndPoint [2]
     << ")\n";

  os << indent
     << "Tolerance : "
     << this->Tolerance
     << "\n";
}

// ----------------------------------------------------------------------
int vtkLinearExtractionFilter::FillInputPortInformation( int port, vtkInformation *info )
{
  info->Set( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),
            "vtkCompositeDataSet");
  //info->Set( vtkAlgorithm::INPUT_IS_OPTIONAL(), 0 );
  return 1;
}

// ----------------------------------------------------------------------
int vtkLinearExtractionFilter::RequestData( vtkInformation *vtkNotUsed( request ),
                                            vtkInformationVector **inputVector,
                                            vtkInformationVector *outputVector )
{
  // Get information objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject( 0 );
  vtkInformation *outInfo = outputVector->GetInformationObject( 0 );

  // Get input and ouptut
  vtkCompositeDataSet *compositeInput
    = vtkCompositeDataSet::SafeDownCast( inInfo->Get( vtkDataObject::DATA_OBJECT() ) );

  vtkSelection *output =
    vtkSelection::SafeDownCast( outInfo->Get( vtkDataObject::DATA_OBJECT() ) );

  // preparation de l'output
  if ( ! output )
    {
    vtkErrorMacro( <<"vtkLinearExtractionFilter: filter does not have any output." );
    return 0;
    }	// if ( ! output )

  if ( ! compositeInput )
    {
    vtkErrorMacro( <<"vtkLinearExtractionFilter: filter does not have any input." );
    return 0;
    }	// if ( ! compositeInput )

  vtkSmartPointer<vtkCompositeDataIterator> inputIterator = vtkCompositeDataIterator::New();
  inputIterator->SetDataSet( compositeInput );
  inputIterator->VisitOnlyLeavesOn();
  inputIterator->SkipEmptyNodesOn();
  inputIterator->InitTraversal();
  inputIterator->GoToFirstItem();
  while ( ! inputIterator->IsDoneWithTraversal() )
    {
    vtkDataSet* input = vtkDataSet::SafeDownCast( inputIterator->GetCurrentDataObject() );
    // Composite indices begin at 1
    int partNumber = inputIterator->GetCurrentFlatIndex() - 1;

    inputIterator->GoToNextItem();
    vtkSmartPointer<vtkIdTypeArray> indices = vtkSmartPointer<vtkIdTypeArray>::New();
    this->RequestDataInternal( input, indices );

    vtkSmartPointer<vtkSelectionNode> outSelNode = vtkSelectionNode::New();
    outSelNode->SetContentType( vtkSelectionNode::INDICES );
    outSelNode->SetFieldType( vtkSelectionNode::CELL );
    outSelNode->GetProperties()->Set( vtkSelectionNode::COMPOSITE_INDEX(), partNumber + 1 );
    outSelNode->SetSelectionList( indices );
    output->AddNode( outSelNode );
    }

  return 1;
}

// ----------------------------------------------------------------------
void vtkLinearExtractionFilter::RequestDataInternal ( vtkDataSet* input, vtkIdTypeArray* outIndices )
{
  // Storage for retained data with and distance à P1 :
  vector< pair<vtkIdType, double> > keptData;
  const vtkIdType cellNum = input->GetNumberOfCells();
  for ( vtkIdType id = 0; id < cellNum; ++ id )
    {
    vtkCell* cell = input->GetCell ( id );
    if ( 0 != cell )
      {
      // Storage for coordinates of intersection with the line
      double coords [3];
      double pcoords [3];
      double t = 0;
      int subId	= 0;

      // Intersection with a segment and not a line
      if ( cell->IntersectWithLine ( this->StartPoint, 
                                     this->EndPoint, 
                                     this->Tolerance, 
                                     t, 
                                     coords, 
                                     pcoords,
                                     subId ) )
        {
        outIndices->InsertNextValue( id );
        }
      }	// if ( cell )
    } // for ( vtkIdType id = 0; id < cellNum; ++ id )

}
