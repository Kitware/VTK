/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtLabelMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkQtLabelMapper.h"

#include "vtkActor2D.h"
#include "vtkExecutive.h"
#include "vtkImageData.h"
#include "vtkLabelHierarchy.h"
#include "vtkObjectFactory.h"
#include "vtkQtLabelSizeCalculator.h"
#include "vtkQtLabelSurface.h"
#include "vtkPointSetToLabelHierarchy.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkRenderer.h"
#include "vtkTextProperty.h"
#include "vtkTexture.h"
#include "vtkTexturedActor2D.h"
#include "vtkViewport.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkCxxRevisionMacro(vtkQtLabelMapper, "1.4");
vtkStandardNewMacro(vtkQtLabelMapper);

//----------------------------------------------------------------------------
// Creates a new label mapper
vtkQtLabelMapper::vtkQtLabelMapper()
{
  this->QtLabelSurface = vtkSmartPointer<vtkQtLabelSurface>::New();
  this->polyDataMapper2 = vtkSmartPointer<vtkPolyDataMapper2D>::New(); 

  vtkTextProperty* prop = vtkTextProperty::New();
  prop->SetFontSize(12);
  prop->SetBold(1);
  prop->SetItalic(0);
  prop->SetShadow(1);
  prop->SetFontFamilyToArial();
  prop->SetJustificationToCentered();
  prop->SetVerticalJustificationToCentered();
  prop->SetColor(1, 1, 1);
  this->SetLabelTextProperty(prop);
  prop->Delete();
}

//----------------------------------------------------------------------------
vtkQtLabelMapper::~vtkQtLabelMapper()
{
}

//----------------------------------------------------------------------------
void vtkQtLabelMapper::RenderOpaqueGeometry(vtkViewport *viewport, 
                                                vtkActor2D *actor)
{
  vtkRenderer *ren = vtkRenderer::SafeDownCast(viewport);

  vtkDataObject *input = this->GetExecutive()->GetInputData(0, 0);
  if( !input )
    {
    vtkErrorMacro(<<"Need input data to render labels.");
    return;
    }

  this->QtLabelSurface->SetInput( input );
  
  this->QtLabelSurface->SetRenderer( ren );
  this->QtLabelSurface->SetLabelTextProperty( this->GetLabelTextProperty() );
  this->QtLabelSurface->SetFieldDataName("LabelText");
  this->QtLabelSurface->SetTextRotationArrayName( "TextRotation" );
  
  this->polyDataMapper2->SetInputConnection( this->QtLabelSurface->GetOutputPort(1) );
  this->QtLabelSurface->Update();
  VTK_CREATE( vtkTexture, texture );
  texture->SetInput( this->QtLabelSurface->GetOutput() );
  
  vtkTexturedActor2D* outActor = vtkTexturedActor2D::SafeDownCast( actor );
  if( !outActor )
    {
    vtkErrorMacro("Expected a TexturedActor2D in QtLabelMapper.");
    return;
    }
  outActor->SetMapper( this->polyDataMapper2 );
  outActor->SetTexture( texture );
  
  this->polyDataMapper2->RenderOpaqueGeometry( viewport, actor );
}

//----------------------------------------------------------------------------
void vtkQtLabelMapper::RenderOverlay(vtkViewport *viewport, 
                                            vtkActor2D *actor)
{
  vtkRenderer *ren = vtkRenderer::SafeDownCast(viewport);
  
  vtkDataObject *input = this->GetExecutive()->GetInputData(0, 0);
  if( !input )
    {
    vtkErrorMacro(<<"Need input data to render labels.");
    return;
    }

  this->QtLabelSurface->SetInput( input );

  this->QtLabelSurface->SetRenderer( ren );
  this->QtLabelSurface->SetLabelTextProperty(this->GetLabelTextProperty());
  this->QtLabelSurface->SetFieldDataName("LabelText");
  this->QtLabelSurface->SetTextRotationArrayName( "TextRotation" );
  
  this->polyDataMapper2->SetInputConnection( this->QtLabelSurface->GetOutputPort(1) );
  this->QtLabelSurface->Update();
  VTK_CREATE( vtkTexture, texture );
  texture->SetInput( this->QtLabelSurface->GetOutput() );
  
  vtkTexturedActor2D* outActor = vtkTexturedActor2D::SafeDownCast( actor );
  if( !outActor )
    {
    vtkErrorMacro("Expected a TexturedActor2D in QtLabelMapper.");
    return;
    }
  outActor->SetMapper( this->polyDataMapper2 );
  outActor->SetTexture( texture );
  
  this->polyDataMapper2->RenderOverlay( viewport, actor );
}

//----------------------------------------------------------------------------
void vtkQtLabelMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
