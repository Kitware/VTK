/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCategoricalColors.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLookupTable.h"
#include "vtkBrewerColors.h"
#include "vtkDoubleArray.h"
#include "vtkUnsignedCharArray.h"

int TestCategoricalColors( int argc, char* argv[] )
{
  // Create the LUT and add some annotations.
  vtkLookupTable* lut = vtkLookupTable::New();
  lut->SetAnnotation( 0., "Zero" );
  lut->SetAnnotation( .5, "Half" );
  lut->SetAnnotation( 1., "Ichi" );
  lut->SetAnnotation( 1., "One" );
  lut->SetAnnotation( 2., "Ni" );
  lut->SetAnnotation( 2., "Two" );
  lut->SetAnnotation( 3, "San" );
  lut->SetAnnotation( 4, "Floor" );
  lut->SetAnnotation( 5, "Hive" );
  lut->SetAnnotation( 6, "Licks" );
  lut->SetAnnotation( 7, "Leaven" );
  lut->SetAnnotation( 9, "Kyuu" );
  lut->RemoveAnnotation( 2. );

  vtkBrewerColors* schemes = vtkBrewerColors::New();
#if 0
  vtkIdType numSchemes = schemes->GetNumberOfSchemes();
  for ( vtkIdType i = 0; i < numSchemes; ++ i )
    {
    cout << i << ": " << schemes->GetScheme( i ) << "\n";
    }
#endif
  schemes->SetCurrentScheme( "Qualitative Accent (8)" );
  schemes->CreateLookupTable( lut );

  cout.setf( std::ios_base::hex, std::ios::basefield );
  const unsigned char* rgba = lut->MapValue( 0. );
  cout
    << static_cast<int>(rgba[0]) << ","
    << static_cast<int>(rgba[1]) << ","
    << static_cast<int>(rgba[2])
    << endl;
  rgba = lut->MapValue( 3. );
  cout
    << static_cast<int>(rgba[0]) << ","
    << static_cast<int>(rgba[1]) << ","
    << static_cast<int>(rgba[2])
    << endl;

  vtkDoubleArray* data = vtkDoubleArray::New();
  data->InsertNextValue( 0. );
  data->InsertNextValue( 9. );
  data->InsertNextValue( 1. );
  data->InsertNextValue( 2. );
  data->InsertNextValue( 3. );
  data->InsertNextValue( .5 );

  vtkUnsignedCharArray* color = lut->MapScalars( data, VTK_RGBA, 0 );
  unsigned char* cval;
  for ( vtkIdType i = 0; i < color->GetNumberOfTuples(); ++ i )
    {
    cval = color->GetPointer( i * 4 );
    cout
      << data->GetTuple1( i ) << ": "
      << static_cast<int>( cval[0] ) << " "
      << static_cast<int>( cval[1] ) << " "
      << static_cast<int>( cval[2] ) << " "
      << static_cast<int>( cval[3] ) << "\n";
    }
  cval = lut->GetNanColorAsUnsignedChars();
  cout
    << "NaN: "
    << static_cast<int>( cval[0] ) << " "
    << static_cast<int>( cval[1] ) << " "
    << static_cast<int>( cval[2] ) << " "
    << static_cast<int>( cval[3] ) << "\n";

  color->Delete();
  data->Delete();
  lut->Delete();
  schemes->Delete();
  return 0;
}
