#include "vtkLabelSizeCalculator.h"

#include "vtkCellData.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkFieldData.h"
#include "vtkFreeTypeUtilities.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLabelHierarchy.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTextProperty.h"

#include <map>

class vtkLabelSizeCalculator::Internals
{
public:
  std::map<int, vtkSmartPointer<vtkTextProperty> > FontProperties;
};

vtkStandardNewMacro(vtkLabelSizeCalculator);
vtkCxxSetObjectMacro(vtkLabelSizeCalculator,FontUtil,vtkFreeTypeUtilities);

vtkLabelSizeCalculator::vtkLabelSizeCalculator()
{
  this->Implementation = new Internals;
  // Always defined but user may set to NULL.
  this->Implementation->FontProperties[0] = vtkSmartPointer<vtkTextProperty>::New();
  this->FontUtil = vtkFreeTypeUtilities::New(); // Never a NULL moment.
  this->LabelSizeArrayName = NULL;
  this->SetLabelSizeArrayName( "LabelSize" );
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "LabelText");
  this->SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Type");
}

vtkLabelSizeCalculator::~vtkLabelSizeCalculator()
{
  this->SetFontUtil( 0 );
  this->SetLabelSizeArrayName( NULL );
  delete this->Implementation;
}

void vtkLabelSizeCalculator::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "LabelSizeArrayName: " << this->LabelSizeArrayName << "\n";
  os << indent << "FontProperties: ";
  std::map<int, vtkSmartPointer<vtkTextProperty> >::iterator it, itEnd;
  it = this->Implementation->FontProperties.begin();
  itEnd = this->Implementation->FontProperties.end();
  for ( ; it != itEnd; ++it )
    {
    os << indent << "  " << it->first << ": " << it->second.GetPointer() << endl;
    }
  os << indent << "FontUtil: " << this->FontUtil << "\n";
}

int vtkLabelSizeCalculator::FillInputPortInformation( int vtkNotUsed(port), vtkInformation* info )
{
  info->Remove( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE() );
  info->Append( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet" );
  info->Append( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph" );
  return 1;
}

void vtkLabelSizeCalculator::SetFontProperty(vtkTextProperty* prop, int type)
{
  this->Implementation->FontProperties[type] = prop;
}

vtkTextProperty* vtkLabelSizeCalculator::GetFontProperty(int type)
{
  if (this->Implementation->FontProperties.find(type) !=
      this->Implementation->FontProperties.end())
    {
    return this->Implementation->FontProperties[type];
    }
  return 0;
}

int vtkLabelSizeCalculator::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector )
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject( 0 );
  vtkInformation* outInfo = outputVector->GetInformationObject( 0 );

  // get the input and output
  vtkDataObject* input = inInfo->Get( vtkDataObject::DATA_OBJECT() );
  vtkDataObject* output = outInfo->Get( vtkDataObject::DATA_OBJECT() );

  vtkFieldData* inFD = 0;
  vtkFieldData* outFD = 0;

  vtkDataSet* dsInput = vtkDataSet::SafeDownCast( input );
  vtkDataSet* dsOutput = vtkDataSet::SafeDownCast( output );
  vtkGraph* graphInput = vtkGraph::SafeDownCast( input );
  vtkGraph* graphOutput = vtkGraph::SafeDownCast( output );

  // if input is empty, we are done
  if (graphInput && graphInput->GetNumberOfVertices() == 0)
    {
    return 1;
    }
  if (dsInput && dsInput->GetNumberOfPoints() == 0)
    {
    return 1;
    }

  if ( ! this->Implementation->FontProperties[0] )
    {
    vtkErrorMacro( "NULL default font property, so I cannot compute label sizes." );
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
  vtkIntArray* typeArr = vtkIntArray::SafeDownCast(
    this->GetInputAbstractArrayToProcess( 1, inputVector ));

  vtkInformation* inArrInfo = this->GetInputArrayInformation( 0 );
  int fieldAssoc = inArrInfo->Get( vtkDataObject::FIELD_ASSOCIATION() );

  vtkIntArray* lsz = this->LabelSizesForArray( inArr, typeArr );
#if 0
  cout
    << "Input array... port: " << port << " connection: " << connection
    << " field association: " << fieldAssoc << " attribute type: " << attribType << "\n";
#endif // 0

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
      outFD = dsOutput->GetPointData();
      outFD->AddArray( lsz );
      }
    if (
      ! inFD && (
        fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS ||
        fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_CELLS ||
        fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_EDGES
      ) )
      {
      outFD = dsOutput->GetCellData();
      outFD->AddArray( lsz );
      }
    vtkLabelHierarchy* hierarchyOutput = vtkLabelHierarchy::SafeDownCast( output );
    if ( hierarchyOutput )
      {
      hierarchyOutput->SetSizes( lsz );
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
      outFD = graphOutput->GetVertexData();
      outFD->AddArray( lsz );
      }
    if (
      ! inFD && (
        fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS ||
        fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_CELLS ||
        fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_EDGES
      ) )
      {
      outFD = graphOutput->GetEdgeData();
      outFD->AddArray( lsz );
      }
    }
  lsz->Delete();

  return 1;
}

vtkIntArray* vtkLabelSizeCalculator::LabelSizesForArray(
  vtkAbstractArray* labels,
  vtkIntArray* types )
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
    int type = 0;
    if ( types )
      {
      type = types->GetValue( i );
      }
    vtkTextProperty* prop = this->Implementation->FontProperties[type];
    if (!prop)
      {
      prop = this->Implementation->FontProperties[0];
      }
    this->FontUtil->GetBoundingBox(
      prop, labels->GetVariantValue( i ).ToString().c_str(), bbox );
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

