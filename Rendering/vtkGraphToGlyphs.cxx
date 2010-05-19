/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphToGlyphs.cxx

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
#include "vtkGraphToGlyphs.h"

#include "vtkArrayCalculator.h"
#include "vtkDirectedGraph.h"
#include "vtkDistanceToCamera.h"
#include "vtkGlyph3D.h"
#include "vtkGlyphSource2D.h"
#include "vtkGraph.h"
#include "vtkGraphToPoints.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSphereSource.h"
#include "vtkTable.h"
#include "vtkUndirectedGraph.h"

vtkStandardNewMacro(vtkGraphToGlyphs);

vtkGraphToGlyphs::vtkGraphToGlyphs()
{
  this->GraphToPoints        = vtkSmartPointer<vtkGraphToPoints>::New();
  this->Sphere               = vtkSmartPointer<vtkSphereSource>::New();
  this->GlyphSource          = vtkSmartPointer<vtkGlyphSource2D>::New();
  this->DistanceToCamera     = vtkSmartPointer<vtkDistanceToCamera>::New();
  this->Glyph                = vtkSmartPointer<vtkGlyph3D>::New();
  this->GlyphType = CIRCLE;
  this->Filled = true;
  this->ScreenSize = 10;
  this->Sphere->SetRadius(0.5);
  this->Sphere->SetPhiResolution(8);
  this->Sphere->SetThetaResolution(8);
  this->GlyphSource->SetScale(0.5);
  this->Glyph->SetScaleModeToScaleByScalar();
  this->Glyph->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "DistanceToCamera");
  this->Glyph->FillCellDataOn();
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "scale");
}

vtkGraphToGlyphs::~vtkGraphToGlyphs()
{
}

int vtkGraphToGlyphs::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  return 1;
}

void vtkGraphToGlyphs::SetRenderer(vtkRenderer* ren)
{
  this->DistanceToCamera->SetRenderer(ren);
  this->Modified();
}

vtkRenderer* vtkGraphToGlyphs::GetRenderer()
{
  return this->DistanceToCamera->GetRenderer();
}

void vtkGraphToGlyphs::SetScaling(bool b)
{
  this->DistanceToCamera->SetScaling(b);
  this->Modified();
}

bool vtkGraphToGlyphs::GetScaling()
{
  return this->DistanceToCamera->GetScaling();
}

unsigned long vtkGraphToGlyphs::GetMTime()
{
  unsigned long mtime = this->Superclass::GetMTime();
  if (this->GlyphType != VERTEX &&
      this->DistanceToCamera->GetMTime() > mtime)
    {
    mtime = this->DistanceToCamera->GetMTime();
    }
  return mtime;
}

int vtkGraphToGlyphs::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkGraph *input = vtkGraph::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!this->DistanceToCamera->GetRenderer())
    {
    vtkErrorMacro("Need renderer set before updating the filter.");
    return 0;
    }

  vtkSmartPointer<vtkGraph> inputCopy;
  if (vtkDirectedGraph::SafeDownCast(input))
    {
    inputCopy.TakeReference(vtkDirectedGraph::New());
    }
  else
    {
    inputCopy.TakeReference(vtkUndirectedGraph::New());
    }
  inputCopy->ShallowCopy(input);

  this->DistanceToCamera->SetScreenSize(this->ScreenSize);
  this->GlyphSource->SetFilled(this->Filled);

  this->GraphToPoints->SetInput(inputCopy);
  vtkAbstractArray* arr = this->GetInputArrayToProcess(0, inputVector);
  if (arr)
    {
    this->DistanceToCamera->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, arr->GetName());
    }
  this->DistanceToCamera->SetInputConnection(
    this->GraphToPoints->GetOutputPort());
  this->Glyph->SetInputConnection(
    0, this->DistanceToCamera->GetOutputPort());
  if (this->GlyphType == SPHERE)
    {
    this->Glyph->SetInputConnection(
      1, this->Sphere->GetOutputPort());
    }
  else
    {
    this->Glyph->SetInputConnection(
      1, this->GlyphSource->GetOutputPort());
    this->GlyphSource->SetGlyphType(this->GlyphType);
    }
  this->Glyph->Update();

  output->ShallowCopy(this->Glyph->GetOutput());
  
  return 1;
}

void vtkGraphToGlyphs::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Filled: " << this->Filled << endl;
  os << indent << "ScreenSize: " << this->ScreenSize << endl;
  os << indent << "GlyphType: " << this->GlyphType << endl;
}
