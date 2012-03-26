/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDecimatePolylineFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */
// -*- c++ -*- *******************************************************

#include "vtkDecimatePolylineFilter.h"
#include "vtkMath.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkProperty.h"

int TestDecimatePolylineFilter(int, char *[])
{
  const unsigned int numberofpoints = 100;

  vtkPolyData* circle = vtkPolyData::New();
  vtkPoints*   points = vtkPoints::New();
  vtkCellArray* lines = vtkCellArray::New();
  vtkIdType* lineIndices = new vtkIdType[numberofpoints+1];

  for( unsigned int i = 0; i < numberofpoints; i++ )
    {
    const double angle = 2.0 * vtkMath::Pi() * static_cast< double >( i ) /
      static_cast< double >( numberofpoints );
    points->InsertPoint( static_cast< vtkIdType >( i ),
                         cos( angle ),
                         sin( angle ),
                         0. );
    lineIndices[i] = static_cast< vtkIdType >( i );
    }
  lineIndices[numberofpoints] = 0;
  lines->InsertNextCell( numberofpoints+1, lineIndices );
  delete[] lineIndices;

  circle->SetPoints( points );
  circle->SetLines( lines );
  points->Delete();
  lines->Delete();

  vtkPolyDataMapper* c_mapper = vtkPolyDataMapper::New();
  c_mapper->SetInputData( circle );

  vtkActor* c_actor = vtkActor::New();
  c_actor->SetMapper( c_mapper );

  vtkDecimatePolylineFilter* decimate = vtkDecimatePolylineFilter::New();
  decimate->SetInputData( circle );
  decimate->SetTargetReduction( 0.95 );
  decimate->Update();

  vtkPolyDataMapper* d_mapper = vtkPolyDataMapper::New();
  d_mapper->SetInputConnection( decimate->GetOutputPort() );

  vtkActor* d_actor = vtkActor::New();
  d_actor->SetMapper( d_mapper );
  d_actor->GetProperty()->SetColor( 1., 0. ,0. );

  vtkRenderer* ren = vtkRenderer::New();
  ren->AddActor( c_actor );
  ren->AddActor( d_actor );

  vtkRenderWindow* renwin = vtkRenderWindow::New();
  renwin->AddRenderer( ren );

  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow( renwin );

  renwin->Render();

  iren->CreateOneShotTimer( 1 );

  iren->Delete();
  renwin->Delete();
  ren->Delete();

  d_actor->Delete();
  d_mapper->Delete();

  c_actor->Delete();
  c_mapper->Delete();

  decimate->Delete();
  circle->Delete();

  return 0;
}
