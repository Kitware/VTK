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
#include "vtkMolecule.h"
#include "vtkObjectFactory.h"
#include "vtkPeriodicTable.h"
#include "vtkTrivialProducer.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLMoleculeMapper)

//----------------------------------------------------------------------------
vtkOpenGLMoleculeMapper::vtkOpenGLMoleculeMapper()
{
  // Setup glyph mappers
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

  if (this->RenderLattice)
  {
    this->LatticeMapper->Render(ren, act);
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
  this->FastAtomMapper->SetLookupTable(this->AtomGlyphMapper->GetLookupTable());
  this->FastAtomMapper->SetScaleArray("Scale Factors");
  this->FastAtomMapper->SetScalarMaterialMode(this->GetScalarMaterialMode());

  // Copy the color array info:
  this->FastAtomMapper->SelectColorArray(this->AtomGlyphMapper->GetArrayId());
}

//----------------------------------------------------------------------------
// Generate position, scale, and orientation vectors for each bond cylinder
void vtkOpenGLMoleculeMapper::UpdateBondGlyphPolyData()
{
  this->Superclass::UpdateBondGlyphPolyData();

  switch(this->BondColorMode)
  {
    case SingleColor:
      this->FastBondMapper->SetColorModeToDefault();
      this->FastBondMapper->SetScalarModeToUsePointData();
      break;
    default:
    case DiscreteByAtom:
      this->FastBondMapper->SetLookupTable(
            this->BondGlyphMapper->GetLookupTable());
      this->FastBondMapper->SetScalarRange
        (0, this->PeriodicTable->GetNumberOfElements());
      this->FastBondMapper->SetScalarModeToUsePointData();
      this->FastBondMapper->SetScalarMaterialMode(this->GetScalarMaterialMode());
      break;
  }

  // Setup glypher
  this->FastBondMapper->SetScaleArray("Scale Factors");
  this->FastBondMapper->SetOrientationArray("Orientation Vectors");
  this->FastBondMapper->SetSelectionIdArray("Selection Ids");
}
