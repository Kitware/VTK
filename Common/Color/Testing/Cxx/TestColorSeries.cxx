/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestColorSeries.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTestErrorObserver.h"
#include "vtkLookupTable.h"
#include "vtkColorSeries.h"
#include "vtkColor.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkPNGWriter.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkTesting.h"
#include "vtkTestUtilities.h"
#include "vtkTrivialProducer.h"
#include "vtkUnsignedCharArray.h"

#include <cstdio> // For EXIT_SUCCESS

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestColorSeries( int argc, char* argv[] )
{
  int valResult = vtkTesting::PASSED;
  vtkTesting* t = vtkTesting::New();
  for ( int cc = 1; cc < argc; ++ cc )
  {
    t->AddArgument(argv[cc]);
  }

  VTK_CREATE(vtkColorSeries,palettes);
  vtkColor3ub color;
  vtkColor3ub black( 0, 0, 0 );

  // Create a new, custom palette:
  int pid = palettes->SetColorSchemeByName( "Foo" );

  // Should return black as there are no colors:
  color = palettes->GetColor( 0 );
  if ( ! black.Compare( color, 1 ) )
  {
    vtkGenericWarningMacro( "Failure: GetColor on empty palette" );
    valResult = vtkTesting::FAILED;
  }
  // Should return black as there are no colors:
  color = palettes->GetColorRepeating( 0 );
  if ( ! black.Compare( color, 1 ) )
  {
    vtkGenericWarningMacro( "Failure: GetColorRepeating on empty palette" );
    valResult = vtkTesting::FAILED;
  }

  // Test appending colors:
  color = vtkColor3ub( 255, 255, 255 ); palettes->AddColor( color );
  color = vtkColor3ub(   0, 255,   0 ); palettes->AddColor( color );
  color = vtkColor3ub(   0,   0, 255 ); palettes->AddColor( color );
  // Test insertion (as opposed to append):
  color = vtkColor3ub( 255,   0,   0 ); palettes->InsertColor( 1, color );
  // Test removing a color
  palettes->RemoveColor( 0 );

  // Iterate over all the palettes, testing GetColorRepeating
  // (w/ a non-empty palette) and the palette iteration API
  int np = palettes->GetNumberOfColorSchemes();
  VTK_CREATE(vtkImageData,img);
  VTK_CREATE(vtkTrivialProducer,exec);
  VTK_CREATE(vtkUnsignedCharArray,pix);
  exec->SetOutput( img );
  pix->SetNumberOfComponents(3);
  // First, find the largest number of colors in any palette:
  int mps = 0; // maximum palette size
  for ( int p = 0; p < np; ++ p )
  {
    palettes->SetColorScheme( p );
    int nc = palettes->GetNumberOfColors(); // in the current scheme
    if ( nc > mps )
      mps = nc;
  }
  // Now size the test image properly and generate swatches
  pix->SetNumberOfTuples( np * 5 * mps * 5 );
  pix->FillComponent( 0, 255 );
  pix->FillComponent( 1, 255 );
  pix->FillComponent( 2, 255 );
  img->SetExtent( 0, mps * 5 - 1, 0, np * 5 - 1, 0, 0 );
  img->GetPointData()->SetScalars( pix.GetPointer() );
  for ( int p = 0; p < np; ++ p )
  {
    palettes->SetColorScheme( p );
    int nc = palettes->GetNumberOfColors(); // in the current scheme
    /*
    cout
      << "  " << palettes->GetColorSchemeName()
      << ", " << palettes->GetNumberOfColors() << " colors\n";
      */
    int yoff = ( np - p - 1 ) * 5; // Put palette 0 at the top of the image
    for ( int c = 0; c < nc; ++ c )
    {
      color = palettes->GetColorRepeating( c );
      for ( int i = 1; i < 4; ++ i )
      {
        for ( int j = 1; j < 4; ++ j )
        {
          vtkIdType coord = ( ( ( yoff + i ) * mps + c ) * 5 + j ) * 3;
          /*
          cout
            << "i " << i << " j " << j << " c " << c << " p " << p
            << " off " << coord << " poff " << coord/3 <<  "\n";
            */
          pix->SetValue( coord ++, color.GetRed() );
          pix->SetValue( coord ++, color.GetGreen() );
          pix->SetValue( coord ++, color.GetBlue() );
        }
      }
    }
  }

  /* Uncomment to save an updated test image.
  VTK_CREATE(vtkPNGWriter,wri);
  wri->SetFileName( "/tmp/TestColorSeries.png" );
  wri->SetInputConnection( exec->GetOutputPort() );
  wri->Write();
  */

  int imgResult = t->RegressionTest( exec.GetPointer(), 0.);

  palettes->SetColorScheme( vtkColorSeries::BREWER_SEQUENTIAL_BLUE_GREEN_9 );
  // Adding a color now should create a copy of the palette. Verify the name changed.
  color = vtkColor3ub( 255, 255, 255 ); palettes->AddColor( color );
  vtkStdString palName = palettes->GetColorSchemeName();
  vtkStdString expected( "Brewer Sequential Blue-Green (9) copy" );
  if ( palName != expected )
  {
    vtkGenericWarningMacro(
      << "Failure: Palette copy-on-write: name should have been "
      << "\"" << expected.c_str() << "\" but was "
      << "\"" << palName.c_str() << "\" instead." );
    valResult = vtkTesting::FAILED;
  }
  if ( palettes->GetNumberOfColors() != 10 )
  {
    vtkGenericWarningMacro(
      << "Modified palette should have had 10 entries but had "
      << palettes->GetNumberOfColors() << " instead."
      );
    valResult = vtkTesting::FAILED;
  }

  palettes->SetColorSchemeName(""); // Test bad name... should have no effect.
  palName = palettes->GetColorSchemeName();
  if ( palName != expected )
  {
    vtkGenericWarningMacro(
      << "Failure: Setting empty palette name should have no effect."
    );
    valResult = vtkTesting::FAILED;
  }

  // Check setting a custom palette name and non-copy-on-write
  // behavior for custom palettes:
  palettes->SetColorSchemeName("Unoriginal Blue-Green");
  palettes->SetColorSchemeByName("Unoriginal Blue-Green");
  if ( np != palettes->GetColorScheme() )
  {
    vtkGenericWarningMacro(
      << "Modified palette had ID " << palettes->GetColorScheme()
      << " not expected ID " << np
    );
    valResult = vtkTesting::FAILED;
  }

  palettes->SetNumberOfColors( 8 );
  if ( palettes->GetNumberOfColors() != 8 )
  {
    vtkGenericWarningMacro(
      << "Resized palette should have had 8 entries but had "
      << palettes->GetNumberOfColors() << " instead."
      );
    valResult = vtkTesting::FAILED;
  }

  palettes->ClearColors();
  if ( palettes->GetNumberOfColors() != 0 )
  {
    vtkGenericWarningMacro(
      << "Cleared palette should have had 0 entries but had "
      << palettes->GetNumberOfColors() << " instead."
      );
    valResult = vtkTesting::FAILED;
  }


  // Make sure our custom scheme is still around
  palettes->SetColorScheme( pid );
  // Now test GetColor on a non-empty palette
  color = palettes->GetColor( 2 ); // Should return blue
  vtkColor3ub blue( 0, 0, 255 );
  if ( ! blue.Compare( color, 1 ) )
  {
    vtkGenericWarningMacro( "Failure: GetColor on small test palette" );
    valResult = vtkTesting::FAILED;
  }

  // Test DeepCopy
  VTK_CREATE(vtkColorSeries,other);
  other->DeepCopy(palettes);
  if ( other->GetColorScheme() != palettes->GetColorScheme() )
  {
    vtkGenericWarningMacro( "Failure: DeepCopy did not preserve current scheme" );
    valResult = vtkTesting::FAILED;
  }
  other->DeepCopy(NULL);

  // Test SetColor
  other->SetColorScheme( pid );
  other->SetColor( 0, blue );
  color = other->GetColor( 0 );
  if ( ! blue.Compare( color, 1 ) )
  {
    vtkGenericWarningMacro( "Failure: SetColor on test palette" );
    valResult = vtkTesting::FAILED;
  }

  // Build a lookup table
  vtkLookupTable* lut = palettes->CreateLookupTable();
  lut->Print(std::cout);
  lut->Delete();

  // Test scheme out of range warning
  VTK_CREATE(vtkTest::ErrorObserver, warningObserver);
  palettes->AddObserver(vtkCommand::WarningEvent, warningObserver);
  palettes->SetColorScheme(1000);
  if (warningObserver->GetWarning())
  {
    std::cout << "Caught expected warning: "
              << warningObserver->GetWarningMessage() << std::endl;
  }
  else
  {
    vtkGenericWarningMacro( "Failure: SetColorScheme(1000) did not produce expected warning" );
    valResult = vtkTesting::FAILED;
  }
  vtkIndent indent;
  palettes->PrintSelf(std::cout, indent);

  t->Delete();
  return (
    imgResult == vtkTesting::PASSED &&
    valResult == vtkTesting::PASSED ) ?
    EXIT_SUCCESS : EXIT_FAILURE;
}
