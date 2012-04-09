#include "vtkLinearSelector.h"

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

vtkStandardNewMacro(vtkLinearSelector);
vtkCxxSetObjectMacro(vtkLinearSelector,Points,vtkPoints);

// ----------------------------------------------------------------------
vtkLinearSelector::vtkLinearSelector()
{
  
  this->StartPoint[0] = this->StartPoint[1] = this->StartPoint[2] = 0.0;
  this->EndPoint[0] = this->EndPoint[1] = this->EndPoint[2] = 1.0;
  this->Tolerance = 0.;
  this->IncludeVertices = true;
  this->VertexEliminationTolerance = 1.e-6;
  this->Points = 0;
}

// ----------------------------------------------------------------------
vtkLinearSelector::~vtkLinearSelector()
{
  this->SetPoints( 0 );
}

// ----------------------------------------------------------------------
void vtkLinearSelector::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os,indent );

  os << indent
     << "Point 1: ("
     << this->StartPoint[0]
     << ", "
     << this->StartPoint [1]
     << ", "
     << this->StartPoint [2]
     << ")\n";

  os << indent
     << "Point 2: ("
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
     << "Tolerance: "
     << this->Tolerance
     << "\n";

  os << indent
     << "Include Vertices: "
     << ( this->IncludeVertices ? "Yes": "No" )
     << "\n";

  os << indent
     << "VertexEliminationTolerance: "
     << this->VertexEliminationTolerance
     << "\n";

}

// ----------------------------------------------------------------------
int vtkLinearSelector::FillInputPortInformation( int vtkNotUsed( port ),
                                                  vtkInformation *info )
{
  info->Set( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet" );

  return 1;
}

// ----------------------------------------------------------------------
int vtkLinearSelector::RequestData( vtkInformation *vtkNotUsed( request ),
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
    vtkErrorMacro( <<"vtkLinearSelector: filter does not have any output." );
    return 0;
    }	// if ( ! output )

  if ( ! compositeInput )
    {
    vtkErrorMacro( <<"vtkLinearSelector: filter does not have any input." );
    return 0;
    }	// if ( ! compositeInput )

  // Now traverse the input
  vtkCompositeDataIterator* inputIterator = vtkCompositeDataIterator::New();
  inputIterator->SetDataSet( compositeInput );
  inputIterator->VisitOnlyLeavesOn();
  inputIterator->SkipEmptyNodesOn();
  inputIterator->InitTraversal();
  inputIterator->GoToFirstItem();
  for ( ; ! inputIterator->IsDoneWithTraversal(); inputIterator->GoToNextItem() )
    {
    // Retrieve indices of current object
    vtkDataSet* input = vtkDataSet::SafeDownCast( inputIterator->GetCurrentDataObject() );
    vtkIdTypeArray* indices = vtkIdTypeArray::New();
    this->SeekIntersectingCells( input, indices );

    // Create and add selection node
    vtkSelectionNode* n = vtkSelectionNode::New();
    n->SetContentType( vtkSelectionNode::INDICES );
    n->SetFieldType( vtkSelectionNode::CELL );
    n->GetProperties()->Set( vtkSelectionNode::COMPOSITE_INDEX(), inputIterator->GetCurrentFlatIndex() );
    n->SetSelectionList( indices );
    output->AddNode( n );

    // Clean up
    n->Delete();
    indices->Delete();
    }

  // Clean up
  inputIterator->Delete();

  return 1;
}

// ----------------------------------------------------------------------
void vtkLinearSelector::SeekIntersectingCells( vtkDataSet* input, vtkIdTypeArray* outIndices )
{
  vtkIdType nSegments = this->Points ? this->Points->GetNumberOfPoints() - 1 : 1;

  // Reject meaningless parameterizations
  if ( nSegments < 1 )
    {
    vtkWarningMacro( <<"Cannot intersect: not enough points to define a broken line.");
    return;
    }

  // Prepare lists of start and end points
  vtkIdType nCoords = 3 * nSegments;
  double* startPoints = new double[nCoords];
  double* endPoints = new double[nCoords];

  if ( this->Points )
    {
    // Prepare and store segment vertices
    if ( this->IncludeVertices )
      {
      // Vertices are included, use full segment extent
      for ( vtkIdType i = 0; i < nSegments; ++ i )
        {
        vtkIdType offset = 3 * i;
        this->Points->GetPoint( i, startPoints + offset );
        this->Points->GetPoint( i + 1, endPoints + offset );
        cerr << i - 1 << ": " 
             << startPoints[offset]
             << " "
             << startPoints[offset + 1]
             << " "
             << startPoints[offset + 2]
             << endl;
        }
      } // if ( this->IncludeVertices )
    else
      {
      // Vertices are excluded, reduce segment by given ratio
      for ( vtkIdType i = 0; i < nSegments; ++ i )
        {
        vtkIdType offset = 3 * i;
        this->Points->GetPoint( i, startPoints + offset );
        this->Points->GetPoint( i + 1, endPoints + offset );
        for ( int j = 0; j < 3; ++ j, ++ offset )
          {
          double delta =  this->VertexEliminationTolerance * ( endPoints[offset] - startPoints[offset] );
          startPoints[offset] += delta;
          endPoints[offset] -= delta;
          } // for ( j )
        } // for ( i )
      } // else
    } // if ( this->Points )
  else // if ( this->Points )
    {
    // Prepare and store segment vertices
    if ( this->IncludeVertices )
      {
      // Vertices are included, use full segment extent
      for ( int i = 0; i < 3; ++ i )
        {
        startPoints[i] = this->StartPoint[i];
        endPoints[i] = this->EndPoint[i];
        }
      } // if ( this->IncludeVertices )
    else 
      {
      // Vertices are excluded, reduce segment by given ratio
      for ( int i = 0; i < 3; ++ i )
        {
        double delta =  this->VertexEliminationTolerance * ( this->EndPoint[i] - this->StartPoint[i] );
        startPoints[i] = this->StartPoint[i] + delta;
        endPoints[i] = this->EndPoint[i] - delta;
        }
      } // else
    } // else // if ( this->Points )

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
      
      // Seek intersection of cell with each segment
      for ( vtkIdType i = 0; i < nSegments; ++ i )
        {
        // Intersection with a line segment
        vtkIdType offset = 3 * i;
        if ( cell->IntersectWithLine ( startPoints + offset, 
                                       endPoints + offset, 
                                       this->Tolerance, 
                                       t, 
                                       coords, 
                                       pcoords,
                                       subId ) )
          {
          outIndices->InsertNextValue( id );
          }
        } // for ( i )  
      }	// if ( cell )
    } // for ( vtkIdType id = 0; id < nCells; ++ id )

  // Clean up
  delete [] startPoints;
  delete [] endPoints;
}
