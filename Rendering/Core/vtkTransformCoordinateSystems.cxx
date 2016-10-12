/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformCoordinateSystems.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransformCoordinateSystems.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkCoordinate.h"
#include "vtkViewport.h"
#include "vtkPointSet.h"

vtkStandardNewMacro(vtkTransformCoordinateSystems);

//------------------------------------------------------------------------
vtkTransformCoordinateSystems::vtkTransformCoordinateSystems()
{
  this->TransformCoordinate = vtkCoordinate::New();
  this->TransformCoordinate->SetCoordinateSystemToWorld();
  this->InputCoordinateSystem = VTK_WORLD;
  this->OutputCoordinateSystem = VTK_DISPLAY;
  this->Viewport = NULL;
}

//------------------------------------------------------------------------
vtkTransformCoordinateSystems::~vtkTransformCoordinateSystems()
{
  this->TransformCoordinate->Delete();
}

// ----------------------------------------------------------------------------
// Set the viewport. This is a raw pointer, not a weak pointer or a reference
// counted object to avoid cycle reference loop between rendering classes
// and filter classes.
void vtkTransformCoordinateSystems::SetViewport(vtkViewport *viewport)
{
  if(this->Viewport!=viewport)
  {
    this->Viewport=viewport;
    this->Modified();
  }
}

//------------------------------------------------------------------------
int vtkTransformCoordinateSystems::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet *input = vtkPointSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPointSet *output = vtkPointSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPoints *inPts;
  vtkPoints *newPts;
  vtkIdType numPts;

  vtkDebugMacro(<<"Executing transform coordinates filter");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );
  output->CopyAttributes( input );

  // Check input
  //
  inPts = input->GetPoints();

  if ( !inPts )
  {
    return 1;
  }

  numPts = inPts->GetNumberOfPoints();

  newPts = vtkPoints::New();
  newPts->SetNumberOfPoints(numPts);
  this->UpdateProgress (.2);

  // Configure the input
  this->TransformCoordinate->SetViewport(this->Viewport);
  switch ( this->InputCoordinateSystem )
  {
    case VTK_DISPLAY:
      this->TransformCoordinate->SetCoordinateSystemToDisplay();
      break;
    case VTK_VIEWPORT:
      this->TransformCoordinate->SetCoordinateSystemToViewport();
      break;
    case VTK_WORLD:
      this->TransformCoordinate->SetCoordinateSystemToWorld();
      break;
  }

  // Loop over all points, updating position
  vtkIdType ptId;
  double *itmp;
  if ( this->OutputCoordinateSystem == VTK_DISPLAY )
  {
    for (ptId=0; ptId < numPts; ptId++)
    {
      this->TransformCoordinate->SetValue(inPts->GetPoint(ptId));
      itmp = this->TransformCoordinate->
        GetComputedDoubleDisplayValue(this->Viewport);
      newPts->SetPoint(ptId, itmp[0],itmp[1],0.0);
    }
  }
  else if ( this->OutputCoordinateSystem == VTK_VIEWPORT )
  {
    for (ptId=0; ptId < numPts; ptId++)
    {
      this->TransformCoordinate->SetValue(inPts->GetPoint(ptId));
      itmp = this->TransformCoordinate->
        GetComputedDoubleViewportValue(this->Viewport);
      newPts->SetPoint(ptId, itmp[0],itmp[1],0.0);
    }
  }
  else if ( this->OutputCoordinateSystem == VTK_WORLD )
  {
    for (ptId=0; ptId < numPts; ptId++)
    {
      this->TransformCoordinate->SetValue(inPts->GetPoint(ptId));
      itmp = this->TransformCoordinate->
        GetComputedWorldValue(this->Viewport);
      newPts->SetPoint(ptId, itmp[0],itmp[1],itmp[2]);
    }
  }
  this->UpdateProgress (.9);

  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();

  return 1;
}

//------------------------------------------------------------------------
vtkMTimeType vtkTransformCoordinateSystems::GetMTime()
{
  vtkMTimeType mTime=this->MTime.GetMTime();
  vtkMTimeType viewMTime;

  if ( this->Viewport )
  {
    viewMTime = this->Viewport->GetMTime();
    mTime = ( viewMTime > mTime ? viewMTime : mTime );
  }

  return mTime;
}

//------------------------------------------------------------------------
void vtkTransformCoordinateSystems::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Input Coordinate System: ";
  if ( this->InputCoordinateSystem == VTK_DISPLAY )
  {
    os << " DISPLAY\n";
  }
  else if ( this->InputCoordinateSystem == VTK_WORLD )
  {
    os << " WORLD\n";
  }
  else //if ( this->InputCoordinateSystem == VTK_VIEWPORT )
  {
    os << " VIEWPORT\n";
  }

  os << indent << "Output Coordinate System: ";
  if ( this->OutputCoordinateSystem == VTK_DISPLAY )
  {
    os << " DISPLAY\n";
  }
  else if ( this->OutputCoordinateSystem == VTK_WORLD )
  {
    os << " WORLD\n";
  }
  else //if ( this->OutputCoordinateSystem == VTK_VIEWPORT )
  {
    os << " VIEWPORT\n";
  }

  os << indent << "Viewport: ";
  if (this->Viewport)
  {
    os << this->Viewport << "\n";
  }
  else
  {
    os << "(none)\n";
  }
}
