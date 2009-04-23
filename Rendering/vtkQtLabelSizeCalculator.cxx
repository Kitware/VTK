/*=========================================================================
  
Program:   Visualization Toolkit
Module:    vtkQtLabelSizeCalculator.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkQtLabelSizeCalculator.h"

#include "vtkCellData.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkFieldData.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTextProperty.h"

#include <QApplication>
#include <QFont>
#include <QFontMetrics>

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkStandardNewMacro(vtkQtLabelSizeCalculator);
vtkCxxRevisionMacro(vtkQtLabelSizeCalculator,"1.2");
vtkCxxSetObjectMacro(vtkQtLabelSizeCalculator,FontProperty,vtkTextProperty);

vtkQtLabelSizeCalculator::vtkQtLabelSizeCalculator()
{
  if(!QApplication::instance())
    {
    int argc = 0;
    new QApplication(argc, 0);
    }

  this->FontProperty = vtkTextProperty::New(); // Always defined but user may set to NULL.
  this->LabelSizeArrayName = 0;
  this->SetLabelSizeArrayName( "LabelSize" );
}

vtkQtLabelSizeCalculator::~vtkQtLabelSizeCalculator()
{
  this->SetFontProperty( 0 );
  this->SetLabelSizeArrayName( 0 );
}

void vtkQtLabelSizeCalculator::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "LabelSizeArrayName: " << this->LabelSizeArrayName << "\n";
  os << indent << "FontProperty: " << this->FontProperty << "\n";
}

int vtkQtLabelSizeCalculator::FillInputPortInformation( int vtkNotUsed(port), vtkInformation* info )
{
  info->Remove( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE() );
  info->Append( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet" );
  info->Append( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph" );
  return 1;
}

int vtkQtLabelSizeCalculator::RequestData(
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

  vtkInformation* inArrInfo = this->GetInputArrayInformation( 0 );
  int fieldAssoc = inArrInfo->Get( vtkDataObject::FIELD_ASSOCIATION() );

  vtkIntArray* lsz = this->LabelSizesForArray( inArr );

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

vtkIntArray* vtkQtLabelSizeCalculator::LabelSizesForArray( vtkAbstractArray* labels )
{
  vtkIdType nl = labels->GetNumberOfTuples();
  
  vtkIntArray* lsz = vtkIntArray::New();
  lsz->SetName( this->LabelSizeArrayName );
  lsz->SetNumberOfComponents( 4 );
  lsz->SetNumberOfTuples( nl );

  QFont fontSpec( this->FontProperty->GetFontFamilyAsString() );
  fontSpec.setBold( this->FontProperty->GetBold() );
  fontSpec.setItalic( this->FontProperty->GetItalic() );
  fontSpec.setPointSize( this->FontProperty->GetFontSize() );

  int* bds = lsz->GetPointer( 0 );
  for ( vtkIdType i = 0; i < nl; ++ i )
    {
    QFontMetrics fontMetric( fontSpec );
    bds[0] = fontMetric.width( labels->GetVariantValue(i).ToString().c_str() );
    bds[1] = fontMetric.height();
    bds[2] = fontMetric.minLeftBearing();
    bds[3] = fontMetric.descent();

    if( this->GetDebug() )
      {
      cout << "QtLSC: " 
           << bds[0] << " " << bds[1] << " " << bds[2] << " " << bds[3]
           << " \"" << labels->GetVariantValue(i).ToString().c_str() 
           << "\"\n";
      }

    bds += 4;
    }

  return lsz;
}
