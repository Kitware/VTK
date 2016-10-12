/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSelectVisiblePoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSelectVisiblePoints.h"

#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkDataSet.h"
#include "vtkMatrix4x4.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkSelectVisiblePoints);

// Instantiate object with no renderer; window selection turned off;
// tolerance set to 0.01; and select invisible off.
vtkSelectVisiblePoints::vtkSelectVisiblePoints()
{
  this->Renderer = NULL;
  this->SelectionWindow = 0;
  this->Selection[0] = this->Selection[2] = 0;
  this->Selection[1] = this->Selection[3] = 1600;
  this->InternalSelection[0] = this->InternalSelection[2] = 0;
  this->InternalSelection[1] = this->InternalSelection[3] = 1600;
  this->CompositePerspectiveTransform = vtkMatrix4x4::New();
  this->Tolerance = 0.01;
  this->SelectInvisible = 0;
}

vtkSelectVisiblePoints::~vtkSelectVisiblePoints()
{
  this->SetRenderer(NULL);
  this->CompositePerspectiveTransform->Delete();
}

int vtkSelectVisiblePoints::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType ptId, cellId;
  int visible;
  vtkPointData *inPD=input->GetPointData();
  vtkPointData *outPD=output->GetPointData();
  vtkIdType numPts=input->GetNumberOfPoints();
  double x[4];

  // Nothing to extract if there are no points in the data set.
  if ( numPts < 1 )
  {
    return 1;
  }

  if ( this->Renderer == NULL )
  {
    vtkErrorMacro(<<"Renderer must be set");
    return 0;
  }

  if ( ! this->Renderer->GetRenderWindow() )
  {
    vtkErrorMacro( "No render window -- can't get window size to query z buffer." );
    return 0;
  }

  // This will trigger if you do something like ResetCamera before the Renderer or
  // RenderWindow have allocated their appropriate system resources (like creating
  // an OpenGL context)." Resource allocation must occur before we can use the Z
  // buffer.
  if ( this->Renderer->GetRenderWindow()->GetNeverRendered() )
  {
    vtkDebugMacro( "RenderWindow not initialized -- aborting update." );
    return 1;
  }

  vtkCamera* cam = this->Renderer->GetActiveCamera();
  if ( ! cam )
  {
    return 1;
  }

  vtkPoints *outPts = vtkPoints::New();
  outPts->Allocate(numPts/2+1);
  outPD->CopyAllocate(inPD);

  vtkCellArray *outputVertices = vtkCellArray::New();
  output->SetVerts(outputVertices);
  outputVertices->Delete();

  const int SimpleQueryLimit = 25;
  bool getZbuff = numPts > SimpleQueryLimit ? true : false;

  float *zPtr = this->Initialize(getZbuff);

  int abort=0;
  vtkIdType progressInterval=numPts/20+1;
  x[3] = 1.0;
  for (cellId=(-1), ptId=0; ptId < numPts && !abort; ptId++)
  {
    // perform conversion
    input->GetPoint(ptId,x);

    if ( ! (ptId % progressInterval) )
    {
      this->UpdateProgress(static_cast<double>(ptId)/numPts);
      abort = this->GetAbortExecute();
    }

    visible = IsPointOccluded(x, zPtr);

    if ( (visible && !this->SelectInvisible) ||
         (!visible && this->SelectInvisible) )
    {
      cellId = outPts->InsertNextPoint(x);
      output->InsertNextCell(VTK_VERTEX, 1, &cellId);
      outPD->CopyData(inPD,ptId,cellId);
    }
  }//for all points

  output->SetPoints(outPts);
  outPts->Delete();
  output->Squeeze();

  delete [] zPtr;

  vtkDebugMacro(<<"Selected " << cellId + 1 << " out of "
                << numPts << " original points");

  return 1;
}

vtkMTimeType vtkSelectVisiblePoints::GetMTime()
{
  vtkMTimeType mTime=this->Superclass::GetMTime();
  vtkMTimeType time;

  if ( this->Renderer != NULL )
  {
    time = this->Renderer->GetMTime();
    mTime = ( time > mTime ? time : mTime );
  }

  return mTime;
}

int vtkSelectVisiblePoints::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

void vtkSelectVisiblePoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Renderer: " << this->Renderer << "\n";
  os << indent << "Selection Window: "
     << (this->SelectionWindow ? "On\n" : "Off\n");

  os << indent << "Selection: \n";
  os << indent << "  Xmin,Xmax: (" << this->Selection[0] << ", "
     << this->Selection[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->Selection[2] << ", "
     << this->Selection[3] << ")\n";

  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent << "Select Invisible: "
     << (this->SelectInvisible ? "On\n" : "Off\n");
}

float * vtkSelectVisiblePoints::Initialize(bool getZbuff)
{

  int *size = this->Renderer->GetRenderWindow()->GetSize();

  // specify a selection window to avoid querying
  if ( this->SelectionWindow )
  {
    for (int i=0; i<4; i++)
    {
      this->InternalSelection[i] = this->Selection[i];
    }
  }
  else
  {
    this->InternalSelection[0] = this->InternalSelection[2] = 0;
    this->InternalSelection[1] = size[0] - 1;
    this->InternalSelection[3] = size[1] - 1;
  }

  // Grab the composite perspective transform.  This matrix is used to convert
  // each point to view coordinates.  vtkRenderer provides a WorldToView()
  // method but it computes the composite perspective transform each time
  // WorldToView() is called.  This is expensive, so we get the matrix once
  // and handle the transformation ourselves.
  this->CompositePerspectiveTransform->DeepCopy(this->Renderer->GetActiveCamera()->
                   GetCompositeProjectionTransformMatrix(
                   this->Renderer->GetTiledAspectRatio(),0,1));

  // If we have more than a few query points, we grab the z-buffer for the
  // selection region all at once and probe the resulting array.  When we
  // have just a few points, we perform individual z-buffer queries.
  if (getZbuff)
  {
    return this->Renderer->GetRenderWindow()->GetZbufferData(
      this->InternalSelection[0],
      this->InternalSelection[2],
      this->InternalSelection[1],
      this->InternalSelection[3]);
  }
  return NULL;
}

bool vtkSelectVisiblePoints::IsPointOccluded(
  const double x[3],
  const float *zPtr
  )
{
  double view[4];
  double dx[3], z;
  double xx[4] = {x[0], x[1], x[2], 1.0};

  this->CompositePerspectiveTransform->MultiplyPoint(xx, view);
  if (view[3] == 0.0)
  {
    return false;
  }
  this->Renderer->SetViewPoint(view[0]/view[3], view[1]/view[3], view[2]/view[3]);
  this->Renderer->ViewToDisplay();
  this->Renderer->GetDisplayPoint(dx);

  // check whether visible and in selection window
  if ( dx[0] >= this->InternalSelection[0] && dx[0] <= this->InternalSelection[1] &&
    dx[1] >= this->InternalSelection[2] && dx[1] <= this->InternalSelection[3] )
  {
    if (zPtr != NULL)
    {
      // Access the value from the captured zbuffer.  Note, we only
      // captured a portion of the zbuffer, so we need to offset dx by
      // the selection window.
      z = zPtr[static_cast<int>(dx[0]) - this->InternalSelection[0]
      + (static_cast<int>(dx[1]) - this->InternalSelection[2])
        *(this->InternalSelection[1] - this->InternalSelection[0] + 1)];
    }
    else
    {
      z = this->Renderer->GetZ(static_cast<int>(dx[0]),
        static_cast<int>(dx[1]));
    }

    if( dx[2] < (z + this->Tolerance) )
    {
      return true;
    }
  }

  return false;
}
