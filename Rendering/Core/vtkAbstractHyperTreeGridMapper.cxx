/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractHyperTreeGridMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAbstractHyperTreeGridMapper.h"

#include "vtkBitArray.h"
#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkExecutive.h"
#include "vtkImageProperty.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkInteractorObserver.h"
#include "vtkLookupTable.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkUniformHyperTreeGrid.h"

static const double sqrt2 = sqrt(2.);

vtkCxxSetObjectMacro(vtkAbstractHyperTreeGridMapper, ColorMap, vtkScalarsToColors);

//----------------------------------------------------------------------------
vtkAbstractHyperTreeGridMapper::vtkAbstractHyperTreeGridMapper()
{
  // No default renderer is provided
  this->Renderer = nullptr;

  // Default viewing radius
  this->Scale = 1;

  // Default viewing radius
  this->Radius = 1.;

  // Use xy-plane by default
  this->Orientation = 2;
  this->Axis1 = 0;
  this->Axis2 = 1;

  // By default do not limit DFS into trees
  this->LevelMax = -1;

  // Initialize default lookup table
  this->ScalarRange[0] = 0.;
  this->ScalarRange[1] = 1.;
  vtkLookupTable* lut = vtkLookupTable::New();
  lut->SetTableRange(this->ScalarRange);
  lut->Build();
  this->ColorMap = lut;
  this->ColorMap->Register(this);
  this->ColorMap->Delete();

  // Initialize rendering parameters
  this->ParallelProjection = false;
  this->LastCameraParallelScale = 0;
  this->LastRendererSize[0] = 0;
  this->LastRendererSize[1] = 0;
  this->LastCameraFocalPoint[0] = 0.;
  this->LastCameraFocalPoint[1] = 0.;
  this->LastCameraFocalPoint[2] = 0.;
  this->ViewOrientation = 0;
  this->WorldToViewMatrix = vtkMatrix4x4::New();
  this->ViewToWorldMatrix = vtkMatrix4x4::New();
  this->ViewportSize[0] = 0;
  this->ViewportSize[1] = 0;

  // Initialize frame buffer
  this->FrameBuffer = nullptr;

  // Initialize z-buffer
  this->ZBuffer = nullptr;
}

//----------------------------------------------------------------------------
vtkAbstractHyperTreeGridMapper::~vtkAbstractHyperTreeGridMapper()
{
  if (this->ColorMap)
  {
    this->ColorMap->Delete();
    this->ColorMap = nullptr;
  }

  if (this->WorldToViewMatrix)
  {
    this->WorldToViewMatrix->Delete();
    this->WorldToViewMatrix = nullptr;
  }

  if (this->ViewToWorldMatrix)
  {
    this->ViewToWorldMatrix->Delete();
    this->ViewToWorldMatrix = nullptr;
  }

  delete[] this->FrameBuffer;
  this->FrameBuffer = nullptr;

  delete[] this->ZBuffer;
  this->ZBuffer = nullptr;
}

//----------------------------------------------------------------------------
int vtkAbstractHyperTreeGridMapper::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUniformHyperTreeGrid");
  return 1;
}

//----------------------------------------------------------------------------
void vtkAbstractHyperTreeGridMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Scalars: ";
  if (this->Scalars)
  {
    os << endl;
    this->Scalars->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "WorldToViewMatrix: ";
  if (this->WorldToViewMatrix)
  {
    os << endl;
    this->WorldToViewMatrix->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "ViewToWorldMatrix: ";
  if (this->ViewToWorldMatrix)
  {
    os << endl;
    this->ViewToWorldMatrix->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "MustUpdateGrid: " << this->MustUpdateGrid << endl;
  os << indent << "Orientation: " << this->Orientation << endl;

  os << indent << "Renderer: ";
  if (this->Renderer)
  {
    os << endl;
    this->Renderer->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "ScalarRange: " << this->ScalarRange[0] << ", " << this->ScalarRange[1] << endl;

  os << indent << "LookupTable: ";
  if (this->ColorMap)
  {
    os << endl;
    this->ColorMap->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "Scale: " << this->Scale << endl;
  os << indent << "Radius: " << this->Radius << endl;
  os << indent << "Axis1: " << this->Axis1 << endl;
  os << indent << "Axis2: " << this->Axis2 << endl;
  os << indent << "LevelMax: " << this->LevelMax << endl;
  os << indent << "ParallelProjection: " << this->ParallelProjection << endl;
  os << indent << "LastCameraParallelScale: " << this->LastCameraParallelScale << endl;

  os << indent << "ViewportSize: " << this->ViewportSize[0] << ", " << this->ViewportSize[1]
     << endl;

  os << indent << "LastRendererSize: " << this->LastRendererSize[0] << ", "
     << this->LastRendererSize[1] << endl;

  os << indent << "LastCameraFocalPoint: " << this->LastCameraFocalPoint[0] << ", "
     << this->LastCameraFocalPoint[1] << ", " << this->LastCameraFocalPoint[2] << endl;

  os << indent << "ViewOrientation: " << this->ViewOrientation << endl;
  os << indent << "FrameBuffer: " << this->FrameBuffer << endl;
  os << indent << "ZBuffer: " << this->ZBuffer << endl;
}

//----------------------------------------------------------------------------
void vtkAbstractHyperTreeGridMapper::SetInputData(vtkUniformHyperTreeGrid* uhtg)
{
  this->SetInputDataInternal(0, uhtg);
}

//----------------------------------------------------------------------------
void vtkAbstractHyperTreeGridMapper::SetInputConnection(int port, vtkAlgorithmOutput* input)
{
  this->vtkAbstractVolumeMapper::SetInputConnection(port, input);
}

//----------------------------------------------------------------------------
vtkUniformHyperTreeGrid* vtkAbstractHyperTreeGridMapper::GetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return nullptr;
  }
  return vtkUniformHyperTreeGrid::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
}

//----------------------------------------------------------------------------
void vtkAbstractHyperTreeGridMapper::SetRenderer(vtkRenderer* ren)
{
  // Update internal renderer only when needed
  if (ren != this->Renderer)
  {
    // Assign new renderer and update references
    vtkRenderer* tmp = this->Renderer;
    this->Renderer = ren;
    if (tmp != nullptr)
    {
      tmp->UnRegister(this);
    }

    // Try to set look-up table NaN color
    vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->ColorMap);
    if (lut)
    {
      double* bg = this->Renderer->GetBackground();
      lut->SetNanColor(bg[0], bg[1], bg[2], 0.);
    }
    else
    {
      vtkColorTransferFunction* ctf = vtkColorTransferFunction::SafeDownCast(this->ColorMap);
      if (lut)
      {
        double* bg = this->Renderer->GetBackground();
        ctf->SetNanColor(bg[0], bg[1], bg[2]);
      }
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkAbstractHyperTreeGridMapper::SetScalarRange(double s0, double s1)
{
  // Update internal lookup table only when needed
  if (s0 != this->ScalarRange[0] || s1 != this->ScalarRange[1])
  {
    this->ScalarRange[0] = s0;
    this->ScalarRange[1] = s1;

    // Try to set look-up table range
    vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->ColorMap);
    if (lut)
    {
      lut->SetTableRange(s0, s1);
      lut->Build();
    }

    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkAbstractHyperTreeGridMapper::SetScalarRange(double* s)
{
  // No range checking performed here
  this->SetScalarRange(s[0], s[1]);
}

//----------------------------------------------------------------------------
vtkMTimeType vtkAbstractHyperTreeGridMapper::GetMTime()
{
  // Check for minimal changes
  if (this->Renderer)
  {
    vtkCamera* camera = this->Renderer->GetActiveCamera();
    if (camera)
    {
      // Update parallel projection if needed
      bool usePP = camera->GetParallelProjection() ? true : false;
      if (this->ParallelProjection != usePP)
      {
        this->ParallelProjection = usePP;
        this->Modified();
      }

      // Update renderer size if needed
      const int* s = this->Renderer->GetSize();
      if (this->LastRendererSize[0] != s[0] || this->LastRendererSize[1] != s[1])
      {
        this->LastRendererSize[0] = s[0];
        this->LastRendererSize[1] = s[1];
        this->Modified();
      }

      // Update camera focal point if needed
      double* f = camera->GetFocalPoint();
      if (this->LastCameraFocalPoint[0] != f[0] || this->LastCameraFocalPoint[1] != f[1] ||
        this->LastCameraFocalPoint[2] != f[2])
      {
        memcpy(this->LastCameraFocalPoint, f, 3 * sizeof(double));
        this->Modified();
      }

      // Update camera scale if needed
      double scale = camera->GetParallelScale();
      if (this->LastCameraParallelScale != scale)
      {
        this->LastCameraParallelScale = scale;
        this->Modified();
      }
    } // if ( camera )
  }   // if ( this->Renderer )

  // Return superclass mtime
  return this->Superclass::GetMTime();
}

//----------------------------------------------------------------------------
