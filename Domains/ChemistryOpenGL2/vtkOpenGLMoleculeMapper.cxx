/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLMoleculeMapper.h"

#include "vtkOpenGLSphereMapper.h"
#include "vtkOpenGLStickMapper.h"

#include "vtkEventForwarderCommand.h"
#include "vtkGlyph3DMapper.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPeriodicTable.h"
#include "vtkTrivialProducer.h"


//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLMoleculeMapper)

//----------------------------------------------------------------------------
vtkOpenGLMoleculeMapper::vtkOpenGLMoleculeMapper()
{
  // Setup glyph mappers
  vtkNew<vtkLookupTable> lut;
  this->PeriodicTable->GetDefaultLUT(lut.GetPointer());
  this->FastAtomMapper->SetLookupTable(lut.GetPointer());
  this->FastAtomMapper->SetScalarRange
    (0, this->PeriodicTable->GetNumberOfElements());
  this->FastAtomMapper->SetColorModeToMapScalars();
  this->FastAtomMapper->SetScalarModeToUsePointFieldData();

  // Forward commands to instance mappers
  vtkNew<vtkEventForwarderCommand> cb;
  cb->SetTarget(this);

  this->FastAtomMapper->AddObserver(vtkCommand::StartEvent, cb.GetPointer());
  this->FastAtomMapper->AddObserver(vtkCommand::EndEvent, cb.GetPointer());
  this->FastAtomMapper->AddObserver(vtkCommand::ProgressEvent,
                                     cb.GetPointer());

  this->FastBondMapper->AddObserver(vtkCommand::StartEvent, cb.GetPointer());
  this->FastBondMapper->AddObserver(vtkCommand::EndEvent, cb.GetPointer());
  this->FastBondMapper->AddObserver(vtkCommand::ProgressEvent,
                                     cb.GetPointer());

  // Connect the trivial producers to forward the glyph polydata
  this->FastAtomMapper->SetInputConnection
    (this->AtomGlyphPointOutput->GetOutputPort());
  this->FastBondMapper->SetInputConnection
    (this->BondGlyphPointOutput->GetOutputPort());
}

vtkOpenGLMoleculeMapper::~vtkOpenGLMoleculeMapper()
{
}

//----------------------------------------------------------------------------
void vtkOpenGLMoleculeMapper::Render(vtkRenderer *ren, vtkActor *act )
{
  // Update cached polydata if needed
  this->UpdateGlyphPolyData();

  // Pass rendering call on
  if (this->RenderAtoms)
    {
    this->FastAtomMapper->Render(ren, act);
    //  this->AtomGlyphMapper->Render(ren, act);
    }

  if (this->RenderBonds)
    {
    this->FastBondMapper->Render(ren, act);
    //  this->BondGlyphMapper->Render(ren, act);
    }
}

//----------------------------------------------------------------------------
void vtkOpenGLMoleculeMapper::ReleaseGraphicsResources(vtkWindow *w)
{
  this->FastAtomMapper->ReleaseGraphicsResources(w);
  this->FastBondMapper->ReleaseGraphicsResources(w);
  this->Superclass::ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------------
// Generate scale and position information for each atom sphere
void vtkOpenGLMoleculeMapper::UpdateAtomGlyphPolyData()
{
  this->Superclass::UpdateAtomGlyphPolyData();
  this->FastAtomMapper->SelectColorArray("Atomic Numbers");
  this->FastAtomMapper->SetScaleArray("Scale Factors");
}

//----------------------------------------------------------------------------
// Generate position, scale, and orientation vectors for each bond cylinder
void vtkOpenGLMoleculeMapper::UpdateBondGlyphPolyData()
{
  switch(this->BondColorMode)
    {
    case SingleColor:
      this->FastBondMapper->SetColorModeToDefault();
      this->FastBondMapper->SetScalarModeToUsePointData();
      break;
    default:
    case DiscreteByAtom:
      vtkNew<vtkLookupTable> lut;
      this->PeriodicTable->GetDefaultLUT(lut.GetPointer());
      this->FastBondMapper->SetLookupTable(lut.GetPointer());
      this->FastBondMapper->SetScalarRange
        (0, this->PeriodicTable->GetNumberOfElements());
      this->FastBondMapper->SetScalarModeToUsePointData();
      break;
    }

  // Setup glypher
  this->Superclass::UpdateBondGlyphPolyData();
  this->FastBondMapper->SetScaleArray("Scale Factors");
  this->FastBondMapper->SetOrientationArray("Orientation Vectors");
  this->FastBondMapper->SetSelectionIdArray("Selection Ids");
}
