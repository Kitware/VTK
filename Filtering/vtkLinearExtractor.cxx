#include "vtkLinearExtractor.h"

#include "vtkObjectFactory.h"
#include "vtkCell.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataIterator.h"

#include <vtksys/stl/vector>

vtkStandardNewMacro(vtkLinearExtractor);
vtkCxxSetObjectMacro(vtkLinearExtractor,Points,vtkPoints);

// ----------------------------------------------------------------------
vtkLinearExtractor::vtkLinearExtractor ()
{
  this->SetStartPoint( 0., 0., 0. );
  this->SetEndPoint( 1., 1. ,1. );
  this->Tolerance = 0.;
  this->Points = 0;
}

// ----------------------------------------------------------------------
vtkLinearExtractor::~vtkLinearExtractor ()
{
}

// ----------------------------------------------------------------------
void vtkLinearExtractor::PrintSelf ( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os,indent );

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
     << "Points: ";
  if ( this->Points )
    {
    this->Points->PrintSelf( os, indent );
    }
  else
    {
    os << "(none)" << endl;
    }

  os << indent
     << "Tolerance : "
     << this->Tolerance
     << "\n";
}

// ----------------------------------------------------------------------
int vtkLinearExtractor::FillInputPortInformation( int vtkNotUsed( port ),
                                                  vtkInformation *info )
{
  info->Set( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet" );

  return 1;
}

// ----------------------------------------------------------------------
int vtkLinearExtractor::RequestData( vtkInformation *vtkNotUsed( request ),
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
    vtkErrorMacro( <<"vtkLinearExtractor: filter does not have any output." );
    return 0;
    }	// if ( ! output )

  if ( ! compositeInput )
    {
    vtkErrorMacro( <<"vtkLinearExtractor: filter does not have any input." );
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
void vtkLinearExtractor::RequestDataInternal ( vtkDataSet* input, vtkIdTypeArray* outIndices )
{
  // Iterate over cells
  const vtkIdType nCells = input->GetNumberOfCells();
  for ( vtkIdType id = 0; id < nCells; ++ id )
    {
    vtkCell* cell = input->GetCell ( id );
    if ( cell )
      {
      // Storage for coordinates of intersection with the line
      double coords [3];
      double pcoords [3];
      double t = 0;
      int subId	= 0;

      // Branch out between interesection methods depending on input parameters
      if ( this->Points )
        {
        // Intersection with a broken line, exit early if less than 2 points
        vtkIdType nPoints = this->Points->GetNumberOfPoints();
        if ( nPoints < 2 )
          {
          vtkWarningMacro( <<"Cannot intersect: not enough points ("
                           << nPoints
                           << ") to define a broken line.");
          return;
          }

        // Iterate over contiguous segments defining broken line
        double startPoint[3];
        double endPoint[3];
        for ( vtkIdType i = 1; i < nPoints; ++ i )
          {
          this->Points->GetPoint( i - 1, startPoint );
          this->Points->GetPoint( i, endPoint );
          if ( cell->IntersectWithLine ( startPoint, 
                                         endPoint, 
                                         this->Tolerance, 
                                         t,
                                         coords, 
                                         pcoords,
                                         subId ) )
            {
            outIndices->InsertNextValue( id );
            }
          } // for ( vtkIdType i = 1; i < nPoints; ++ i )
        }
      else // if ( this->Points )
        {
        // Intersection with a line segment
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
        }
      }	// if ( cell )
    } // for ( vtkIdType id = 0; id < nCells; ++ id )

}
