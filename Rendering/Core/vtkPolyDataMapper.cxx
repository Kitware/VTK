/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyDataMapper.h"

#include "vtkExecutive.h"
#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkPolyData.h"
#include "vtkRenderWindow.h"
#include "vtkStreamingDemandDrivenPipeline.h"

//----------------------------------------------------------------------------
// Return NULL if no override is supplied.
vtkAbstractObjectFactoryNewMacro(vtkPolyDataMapper)

//----------------------------------------------------------------------------
vtkPolyDataMapper::vtkPolyDataMapper()
{
  this->Piece = 0;
  this->NumberOfPieces = 1;
  this->NumberOfSubPieces = 1;
  this->GhostLevel = 0;
}

//----------------------------------------------------------------------------
void vtkPolyDataMapper::Render(vtkRenderer *ren, vtkActor *act)
{
  if (this->Static)
  {
    this->RenderPiece(ren,act);
    return;
  }

  vtkInformation *inInfo = this->GetInputInformation();
  if (inInfo == NULL)
  {
    vtkErrorMacro("Mapper has no input.");
    return;
  }

  int nPieces = this->NumberOfPieces * this->NumberOfSubPieces;
  for (int i = 0; i < this->NumberOfSubPieces; i++)
  {
    // If more than one pieces, render in loop.
    int currentPiece = this->NumberOfSubPieces * this->Piece + i;
    this->GetInputAlgorithm()->UpdateInformation();
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), currentPiece);
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), nPieces);
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
      this->GhostLevel);
    this->RenderPiece(ren, act);
  }
}

//----------------------------------------------------------------------------
void vtkPolyDataMapper::SetInputData(vtkPolyData *input)
{
  this->SetInputDataInternal(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkPolyData *vtkPolyDataMapper::GetInput()
{
  return vtkPolyData::SafeDownCast(
    this->GetExecutive()->GetInputData(0, 0));
}

//----------------------------------------------------------------------------
int vtkPolyDataMapper::ProcessRequest(vtkInformation* request,
                                      vtkInformationVector** inputVector,
                                      vtkInformationVector*)
{
  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    int currentPiece = this->NumberOfSubPieces * this->Piece;
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), currentPiece);
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
      this->NumberOfSubPieces * this->NumberOfPieces);
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
      this->GhostLevel);
  }
  return 1;
}

//----------------------------------------------------------------------------
// Get the bounds for the input of this mapper as
// (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double *vtkPolyDataMapper::GetBounds()
{
  // do we have an input
  if ( !this->GetNumberOfInputConnections(0))
  {
      vtkMath::UninitializeBounds(this->Bounds);
      return this->Bounds;
  }
  else
  {
    if (!this->Static)
    {
      vtkInformation* inInfo = this->GetInputInformation();
      if (inInfo)
      {
        this->GetInputAlgorithm()->UpdateInformation();
        int currentPiece = this->NumberOfSubPieces * this->Piece;
        this->GetInputAlgorithm()->UpdatePiece(currentPiece,
          this->NumberOfSubPieces * this->NumberOfPieces,
          this->GhostLevel);
      }
    }
    this->ComputeBounds();

    // if the bounds indicate NAN and subpieces are being used then
    // return NULL
    if (!vtkMath::AreBoundsInitialized(this->Bounds)
        && this->NumberOfSubPieces > 1)
    {
      return NULL;
    }
    return this->Bounds;
  }
}

//----------------------------------------------------------------------------
void vtkPolyDataMapper::ComputeBounds()
{
  vtkPolyData *input = this->GetInput();
  if (input)
  {
    input->GetBounds(this->Bounds);
  }
  else
  {
    vtkMath::UninitializeBounds(this->Bounds);
  }
}

//----------------------------------------------------------------------------
void vtkPolyDataMapper::ShallowCopy(vtkAbstractMapper *mapper)
{
  vtkPolyDataMapper *m = vtkPolyDataMapper::SafeDownCast(mapper);
  if (m != NULL)
  {
    this->SetInputConnection(m->GetInputConnection(0, 0));
    this->SetGhostLevel(m->GetGhostLevel());
    this->SetNumberOfPieces(m->GetNumberOfPieces());
    this->SetNumberOfSubPieces(m->GetNumberOfSubPieces());
  }

  // Now do superclass
  this->vtkMapper::ShallowCopy(mapper);
}

//----------------------------------------------------------------------------
void vtkPolyDataMapper::MapDataArrayToVertexAttribute(
    const char* vtkNotUsed(vertexAttributeName),
    const char* vtkNotUsed(dataArrayName),
    int vtkNotUsed(fieldAssociation),
    int vtkNotUsed(componentno)
    )
{
  vtkErrorMacro("Not implemented at this level...");
}

//----------------------------------------------------------------------------
void vtkPolyDataMapper::MapDataArrayToMultiTextureAttribute(
    int vtkNotUsed(unit),
    const char* vtkNotUsed(dataArrayName),
    int vtkNotUsed(fieldAssociation),
    int vtkNotUsed(componentno)
    )
{
  vtkErrorMacro("Not implemented at this level...");
}

//----------------------------------------------------------------------------
void vtkPolyDataMapper::RemoveVertexAttributeMapping(
  const char* vtkNotUsed(vertexAttributeName))
{
  vtkErrorMacro("Not implemented at this level...");
}

//----------------------------------------------------------------------------
void vtkPolyDataMapper::RemoveAllVertexAttributeMappings()
{
  vtkErrorMacro("Not implemented at this level...");
}

//----------------------------------------------------------------------------
void vtkPolyDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Piece : " << this->Piece << endl;
  os << indent << "NumberOfPieces : " << this->NumberOfPieces << endl;
  os << indent << "GhostLevel: " << this->GhostLevel << endl;
  os << indent << "Number of sub pieces: " << this->NumberOfSubPieces
     << endl;
}

//----------------------------------------------------------------------------
int vtkPolyDataMapper::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  return 1;
}


//----------------------------------------------------------------------------
void vtkPolyDataMapper::Update(int port)
{
  if (this->Static)
  {
    return;
  }
  this->Superclass::Update(port);
}

//----------------------------------------------------------------------------
void vtkPolyDataMapper::Update()
{
  if (this->Static)
  {
    return;
  }
  this->Superclass::Update();
}

//----------------------------------------------------------------------------
int vtkPolyDataMapper::Update(int port, vtkInformationVector* requests)
{
  if (this->Static)
  {
    return 1;
  }
  return this->Superclass::Update(port, requests);
}

//----------------------------------------------------------------------------
int vtkPolyDataMapper::Update(vtkInformation* requests)
{
  if (this->Static)
  {
    return 1;
  }
  return this->Superclass::Update(requests);
}
