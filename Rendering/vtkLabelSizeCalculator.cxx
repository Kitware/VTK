#include "vtkLabelSizeCalculator.h"

#include "vtkCellData.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkFieldData.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkFreeTypeUtilities.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkStringArray.h"
#include "vtkTextProperty.h"

vtkStandardNewMacro(vtkLabelSizeCalculator);
vtkCxxRevisionMacro(vtkLabelSizeCalculator,"1.2");
vtkCxxSetObjectMacro(vtkLabelSizeCalculator,FontProperty,vtkTextProperty);
vtkCxxSetObjectMacro(vtkLabelSizeCalculator,FontUtil,vtkFreeTypeUtilities);

vtkLabelSizeCalculator::vtkLabelSizeCalculator()
{
  this->FontProperty = vtkTextProperty::New(); // Always defined but user may set to NULL.
  this->FontUtil = vtkFreeTypeUtilities::New(); // Never a NULL moment.
  this->LabelSizeArrayName = 0;
  this->SetLabelSizeArrayName( "LabelSize" );
}

vtkLabelSizeCalculator::~vtkLabelSizeCalculator()
{
  this->SetFontProperty( 0 );
  this->SetFontUtil( 0 );
  this->SetLabelSizeArrayName( 0 );
}

void vtkLabelSizeCalculator::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "LabelSizeArrayName: " << this->LabelSizeArrayName << "\n";
  os << indent << "FontProperty: " << this->FontProperty << "\n";
  os << indent << "FontUtil: " << this->FontUtil << "\n";
}

int vtkLabelSizeCalculator::FillInputPortInformation( int vtkNotUsed(port), vtkInformation* info )
{
  info->Remove( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE() );
  info->Append( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet" );
  info->Append( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph" );
  return 1;
}

int vtkLabelSizeCalculator::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector )
{
  if ( ! this->FontProperty )
    {
    vtkErrorMacro( "NULL font property, so I cannot compute label sizes." );
    return 0;
    }

  if ( ! this->LabelSizeArrayName )
    {
    vtkErrorMacro( "NULL value for LabelSizeArrayName." );
    return 0;
    }

  // Figure out which array to process
  vtkAbstractArray* inArr = this->GetInputAbstractArrayToProcess( 0, inputVector );
  if ( ! inArr )
    {
    vtkErrorMacro( "No input array available." );
    return 0;
    }
  //cout << "Input arr: \"" << inStr->GetName() << "\"\n";

  vtkInformation* inArrInfo = this->GetInputArrayInformation( 0 );
  //int port = inArrInfo->Get( vtkAlgorithm::INPUT_PORT() );
  //int connection = inArrInfo->Get( vtkAlgorithm::INPUT_CONNECTION() );
  int fieldAssoc = inArrInfo->Get( vtkDataObject::FIELD_ASSOCIATION() );
  //int attribType = inArrInfo->Get( vtkDataObject::FIELD_ATTRIBUTE_TYPE() );

  //vtkIdType numTuples = -1;
  //int attributeDataType = -1;

  vtkIntArray* lsz = this->LabelSizesForArray( inArr );
#if 0
  cout
    << "Input array... port: " << port << " connection: " << connection
    << " field association: " << fieldAssoc << " attribute type: " << attribType << "\n";
#endif // 0

  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject( 0 );
  vtkInformation* outInfo = outputVector->GetInformationObject( 0 );

  // get the input and ouptut
  vtkDataObject* input = inInfo->Get( vtkDataObject::DATA_OBJECT() );
  vtkDataObject* output = outInfo->Get( vtkDataObject::DATA_OBJECT() );

  vtkFieldData* inFD = 0;
  vtkFieldData* outFD = 0;

  vtkDataSet* dsInput = vtkDataSet::SafeDownCast( input );
  vtkDataSet* dsOutput = vtkDataSet::SafeDownCast( output );
  vtkGraph* graphInput = vtkGraph::SafeDownCast( input );
  vtkGraph* graphOutput = vtkGraph::SafeDownCast( output );
  //vtkPointSet* psInput = vtkPointSet::SafeDownCast( input );
  //vtkPointSet* psOutput = vtkPointSet::SafeDownCast( output );
  if ( dsInput )
    {
    dsOutput->CopyStructure( dsInput );
    dsOutput->CopyAttributes( dsInput );
    if (
      fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_NONE ||
      fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_POINTS ||
      fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS ||
      fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_VERTICES ||
      fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_NONE )
      { 
      //inFD = dsInput->GetPointData();
      outFD = dsOutput->GetPointData();
      outFD->AddArray( lsz );
      //attributeDataType = 0;
      //numTuples = dsInput->GetNumberOfPoints();
      }
    if (
      ! inFD && (
        fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS ||
        fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_CELLS ||
        fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_EDGES
      ) )
      {
      //inFD = dsInput->GetCellData();
      outFD = dsOutput->GetCellData();
      outFD->AddArray( lsz );
      //attributeDataType = 1;
      //numTuples = dsInput->GetNumberOfCells();
      }
    }
  else if ( graphInput )
    {
    graphOutput->ShallowCopy( graphInput );
    if (
      fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_NONE ||
      fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_POINTS ||
      fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS ||
      fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_VERTICES ||
      fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_NONE )
      {
      //inFD = graphInput->GetVertexData();
      outFD = graphOutput->GetVertexData();
      outFD->AddArray( lsz );
      //attributeDataType = 0;
      //numTuples = graphInput->GetNumberOfVertices();
      }
    if (
      ! inFD && (
        fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS ||
        fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_CELLS ||
        fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_EDGES
      ) )
      {
      //inFD = graphInput->GetEdgeData();
      outFD = graphOutput->GetEdgeData();
      outFD->AddArray( lsz );
      //attributeDataType = 1;
      //numTuples = graphInput->GetNumberOfEdges();
      }
    }
  lsz->Delete();

  return 1;
}

vtkIntArray* vtkLabelSizeCalculator::LabelSizesForArray( vtkAbstractArray* labels )
{
  vtkIdType nl = labels->GetNumberOfTuples();

  vtkIntArray* lsz = vtkIntArray::New();
  lsz->SetName( this->LabelSizeArrayName );
  lsz->SetNumberOfComponents( 4 );
  lsz->SetNumberOfTuples( nl );

  int bbox[4];
  int* bds = lsz->GetPointer( 0 );
  for ( vtkIdType i = 0; i < nl; ++ i )
    {
    this->FontUtil->GetBoundingBox(
      this->FontProperty, labels->GetVariantValue( i ).ToString().c_str(), bbox );
    bds[0] = bbox[1] - bbox[0];
    bds[1] = bbox[3] - bbox[2];
    bds[2] = bbox[0];
    bds[3] = bbox[2];

    if( this->GetDebug() )
      {
      cout << "LSC: "
           << bds[0] << " " << bds[1] << " " << bds[2] << " " << bds[3]
           << " \"" << labels->GetVariantValue( i ).ToString().c_str() << "\"\n";
      }
    
    bds += 4;
    }

  return lsz;
}

