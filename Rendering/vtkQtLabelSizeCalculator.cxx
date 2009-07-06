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
#include "vtkUnicodeString.h"

#include <QApplication>
#include <QFont>
#include <QFontMetrics>

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <vtkstd/map>

class vtkQtLabelSizeCalculator::Internals
{
public:
  vtkstd::map<int, vtkSmartPointer<vtkTextProperty> > FontProperties;
};

vtkStandardNewMacro(vtkQtLabelSizeCalculator);
vtkCxxRevisionMacro(vtkQtLabelSizeCalculator,"1.4");

vtkQtLabelSizeCalculator::vtkQtLabelSizeCalculator()
{
  if(!QApplication::instance())
    {
    int argc = 0;
    new QApplication(argc, 0);
    } 
}

vtkQtLabelSizeCalculator::~vtkQtLabelSizeCalculator()
{
}

void vtkQtLabelSizeCalculator::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

vtkIntArray* vtkQtLabelSizeCalculator::LabelSizesForArray( 
  vtkAbstractArray* labels, vtkIntArray* types )
{
  vtkIdType nl = labels->GetNumberOfTuples();
  
  vtkIntArray* lsz = vtkIntArray::New();
  lsz->SetName( this->LabelSizeArrayName );
  lsz->SetNumberOfComponents( 4 );
  lsz->SetNumberOfTuples( nl );

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
 
    QFont fontSpec( prop->GetFontFamilyAsString() );
    fontSpec.setBold( prop->GetBold() );
    fontSpec.setItalic( prop->GetItalic() );
    fontSpec.setPointSize( prop->GetFontSize() );

    QFontMetrics fontMetric( fontSpec );
//    bds[0] = fontMetric.width( labels->GetVariantValue(i).ToString().c_str() );
    bds[0] = fontMetric.width( QString::fromUtf8( labels->GetVariantValue(i).ToUnicodeString().utf8_str() ) );
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
