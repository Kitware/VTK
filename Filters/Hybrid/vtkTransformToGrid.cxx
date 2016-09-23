/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformToGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransformToGrid.h"

#include "vtkAbstractTransform.h"
#include "vtkIdentityTransform.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkTransformToGrid);

vtkCxxSetObjectMacro(vtkTransformToGrid,Input,vtkAbstractTransform);

//----------------------------------------------------------------------------
vtkTransformToGrid::vtkTransformToGrid()
{
  this->Input = NULL;

  this->GridScalarType = VTK_FLOAT;

  for (int i = 0; i < 3; i++)
  {
    this->GridExtent[2*i] = this->GridExtent[2*i+1] = 0;
    this->GridOrigin[i] = 0.0;
    this->GridSpacing[i] = 1.0;
  }

  this->DisplacementScale = 1.0;
  this->DisplacementShift = 0.0;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkTransformToGrid::~vtkTransformToGrid()
{
  this->SetInput(static_cast<vtkAbstractTransform*>(0));
}

//----------------------------------------------------------------------------
void vtkTransformToGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  int i;

  this->Superclass::PrintSelf(os,indent);

  os << indent << "Input: (" << this->Input << ")\n";

  os << indent << "GridSpacing: (" << this->GridSpacing[0];
  for (i = 1; i < 3; ++i)
  {
    os << ", " << this->GridSpacing[i];
  }
  os << ")\n";

  os << indent << "GridOrigin: (" << this->GridOrigin[0];
  for (i = 1; i < 3; ++i)
  {
    os << ", " << this->GridOrigin[i];
  }
  os << ")\n";

  os << indent << "GridExtent: (" << this->GridExtent[0];
  for (i = 1; i < 6; ++i)
  {
    os << ", " << this->GridExtent[i];
  }
  os << ")\n";

  os << indent << "GridScalarType: " <<
    vtkImageScalarTypeNameMacro(this->GridScalarType) << "\n";

  this->UpdateShiftScale();

  os << indent << "DisplacementScale: " << this->DisplacementScale << "\n";
  os << indent << "DisplacementShift: " << this->DisplacementShift << "\n";
}

//----------------------------------------------------------------------------
// This method returns the largest data that can be generated.
void vtkTransformToGrid::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector ** vtkNotUsed( inputVector ),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);


  if (this->GetInput() == NULL)
  {
    vtkErrorMacro("Missing input");
    return;
  }

  // update the transform, maybe in the future make transforms part of the
  // pipeline
  this->Input->Update();

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               this->GridExtent,6);
  outInfo->Set(vtkDataObject::SPACING(),this->GridSpacing,3);
  outInfo->Set(vtkDataObject::ORIGIN(),this->GridOrigin,3);
  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, this->GridScalarType, 3);
}

//----------------------------------------------------------------------------
// Return the maximum absolute displacement of the transform over
// the entire grid extent -- this is extremely robust and extremely
// inefficient, it should be possible to do much better than this.
static void vtkTransformToGridMinMax(vtkTransformToGrid *self, int extent[6],
                                     double &minDisplacement, double &maxDisplacement)
{
  vtkAbstractTransform *transform = self->GetInput();
  transform->Update();

  if (!transform)
  {
    minDisplacement = -1.0;
    maxDisplacement = +1.0;
    return;
  }

  double *spacing = self->GetGridSpacing();
  double *origin = self->GetGridOrigin();

  maxDisplacement = -1e37;
  minDisplacement = +1e37;

  double point[3],newPoint[3],displacement;

  for (int k = extent[4]; k <= extent[5]; k++)
  {
    point[2] = k*spacing[2] + origin[2];
    for (int j = extent[2]; j <= extent[3]; j++)
    {
      point[1] = j*spacing[1] + origin[1];
      for (int i = extent[0]; i <= extent[1]; i++)
      {
        point[0] = i*spacing[0] + origin[0];

        transform->InternalTransformPoint(point,newPoint);

        for (int l = 0; l < 3; l++)
        {
          displacement = newPoint[l] - point[l];

          if (displacement > maxDisplacement)
          {
            maxDisplacement = displacement;
          }

          if (displacement < minDisplacement)
          {
            minDisplacement = displacement;
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkTransformToGrid::UpdateShiftScale()
{
  int gridType = this->GridScalarType;

  // nothing to do for double or float
  if (gridType == VTK_DOUBLE || gridType == VTK_FLOAT)
  {
    this->DisplacementShift = 0.0;
    this->DisplacementScale = 1.0;
    vtkDebugMacro(<< "displacement (scale, shift) = (" <<
                  this->DisplacementScale << ", " <<
                  this->DisplacementShift << ")");
    return;
  }

  // check mtime
  if (this->ShiftScaleTime.GetMTime() > this->GetMTime())
  {
    return;
  }

  // get the maximum displacement
  double minDisplacement, maxDisplacement;
  vtkTransformToGridMinMax(this,this->GridExtent,
                           minDisplacement,
                           maxDisplacement);

  vtkDebugMacro(<< "displacement (min, max) = (" <<
                minDisplacement << ", " << maxDisplacement << ")");

  double typeMin,typeMax;

  switch (gridType)
  {
    case VTK_SHORT:
      typeMin = VTK_SHORT_MIN;
      typeMax = VTK_SHORT_MAX;
      break;
    case VTK_UNSIGNED_SHORT:
      typeMin = VTK_UNSIGNED_SHORT_MIN;
      typeMax = VTK_UNSIGNED_SHORT_MAX;
      break;
    case VTK_CHAR:
      typeMin = VTK_CHAR_MIN;
      typeMax = VTK_CHAR_MAX;
      break;
    case VTK_UNSIGNED_CHAR:
      typeMin = VTK_UNSIGNED_CHAR_MIN;
      typeMax = VTK_UNSIGNED_CHAR_MAX;
      break;
    default:
      vtkErrorMacro(<< "UpdateShiftScale: Unknown input ScalarType");
      return;
  }

  this->DisplacementScale = ((maxDisplacement - minDisplacement)/
                             (typeMax - typeMin));
  this->DisplacementShift = ((typeMax*minDisplacement-typeMin*maxDisplacement)/
                             (typeMax - typeMin));

  if (this->DisplacementScale == 0.0)
  {
    this->DisplacementScale = 1.0;
  }

  vtkDebugMacro(<< "displacement (scale, shift) = (" <<
                this->DisplacementScale << ", " <<
                this->DisplacementShift << ")");

  this->ShiftScaleTime.Modified();
}

//----------------------------------------------------------------------------
// macros to ensure proper round-to-nearest behaviour

inline void vtkGridRound(double val, unsigned char& rnd)
{
  rnd = (unsigned char)(val+0.5f);
}

inline void vtkGridRound(double val, char& rnd)
{
  rnd = (char)((val+128.5f)-128);
}

inline void vtkGridRound(double val, short& rnd)
{
  rnd = (short)((int)(val+32768.5f)-32768);
}

inline void vtkGridRound(double val, unsigned short& rnd)
{
  rnd = (unsigned short)(val+0.5f);
}

inline void vtkGridRound(double val, float& rnd)
{
  rnd = (float)(val);
}

inline void vtkGridRound(double val, double& rnd)
{
  rnd = (double)(val);
}

//----------------------------------------------------------------------------
template<class T>
void vtkTransformToGridExecute(vtkTransformToGrid *self,
                               vtkImageData *grid, T *gridPtr, int extent[6],
                               double shift, double scale, int id)
{
  vtkAbstractTransform *transform = self->GetInput();
  int isIdentity = 0;
  if (transform == 0)
  {
    transform = vtkIdentityTransform::New();
    isIdentity = 1;
  }

  double *spacing = grid->GetSpacing();
  double *origin = grid->GetOrigin();
  vtkIdType *increments = grid->GetIncrements();

  double invScale = 1.0/scale;

  double point[3];
  double newPoint[3];

  T *gridPtr0 = gridPtr;

  unsigned long count = 0;
  unsigned long target = (unsigned long)
    ((extent[5]-extent[4]+1)*(extent[3]-extent[2]+1)/50.0);
  target++;

  for (int k = extent[4]; k <= extent[5]; k++)
  {
    point[2] = k*spacing[2] + origin[2];
    T *gridPtr1 = gridPtr0;

    for (int j = extent[2]; j <= extent[3]; j++)
    {

      if (id == 0)
      {
        if (count % target == 0)
        {
          self->UpdateProgress(count/(50.0*target));
        }
        count++;
      }

      point[1] = j*spacing[1] + origin[1];
      gridPtr = gridPtr1;

      for (int i = extent[0]; i <= extent[1]; i++)
      {
        point[0] = i*spacing[0] + origin[0];

        transform->InternalTransformPoint(point,newPoint);

        vtkGridRound((newPoint[0] - point[0] - shift)*invScale,*gridPtr++);
        vtkGridRound((newPoint[1] - point[1] - shift)*invScale,*gridPtr++);
        vtkGridRound((newPoint[2] - point[2] - shift)*invScale,*gridPtr++);
      }

      gridPtr1 += increments[1];
    }

    gridPtr0 += increments[2];
  }

  if (isIdentity)
  {
    transform->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkTransformToGrid::RequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector* outputVector)
{
  // get the data object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *grid = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  grid->SetExtent(
    outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  grid->AllocateScalars(outInfo);
  int *extent = grid->GetExtent();

  void *gridPtr = grid->GetScalarPointerForExtent(extent);
  int gridType = grid->GetScalarType();

  this->UpdateShiftScale();

  double scale = this->DisplacementScale;
  double shift = this->DisplacementShift;

  int id = 0;

  switch (gridType)
  {
    case VTK_DOUBLE:
      vtkTransformToGridExecute(this, grid, (double *)(gridPtr), extent,
                                shift,scale,id);
      break;
    case VTK_FLOAT:
      vtkTransformToGridExecute(this, grid, (float *)(gridPtr), extent,
                                shift,scale,id);
      break;
    case VTK_SHORT:
      vtkTransformToGridExecute(this, grid, (short *)(gridPtr), extent,
                                shift,scale,id);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkTransformToGridExecute(this, grid, (unsigned short *)(gridPtr),extent,
                                shift,scale,id);
      break;
    case VTK_CHAR:
      vtkTransformToGridExecute(this, grid, (char *)(gridPtr), extent,
                                shift,scale,id);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkTransformToGridExecute(this, grid, (unsigned char *)(gridPtr), extent,
                                shift,scale,id);
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
  }
}

//----------------------------------------------------------------------------
vtkMTimeType vtkTransformToGrid::GetMTime()
{
  vtkMTimeType mtime = this->Superclass::GetMTime();

  if (this->Input)
  {
    vtkMTimeType mtime2 = this->Input->GetMTime();
    if (mtime2 > mtime)
    {
      mtime = mtime2;
    }
  }

  return mtime;
}

//----------------------------------------------------------------------------
int vtkTransformToGrid::ProcessRequest(vtkInformation* request,
                                      vtkInformationVector** inputVector,
                                      vtkInformationVector* outputVector)
{
  // generate the data
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    this->RequestData(request, inputVector, outputVector);
    return 1;
  }

  // execute information
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    this->RequestInformation(request, inputVector, outputVector);
    // after executing set the origin and spacing from the
    // info
    int i;
    for (i = 0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkImageData *output =
        vtkImageData::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));
      // if execute info didn't set origin and spacing then we set them
      if (!info->Has(vtkDataObject::ORIGIN()))
      {
        info->Set(vtkDataObject::ORIGIN(),0,0,0);
        info->Set(vtkDataObject::SPACING(),1,1,1);
      }
      if (output)
      {
        output->SetOrigin(info->Get(vtkDataObject::ORIGIN()));
        output->SetSpacing(info->Get(vtkDataObject::SPACING()));
      }
    }
    return 1;
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
vtkImageData* vtkTransformToGrid::GetOutput()
{
  return vtkImageData::SafeDownCast(this->GetOutputDataObject(0));
}

int vtkTransformToGrid::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  return 1;
}
