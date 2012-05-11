/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkROIStencilSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkROIStencilSource.h"

#include "vtkMath.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkImageStencilData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <math.h>

vtkStandardNewMacro(vtkROIStencilSource);

//----------------------------------------------------------------------------
vtkROIStencilSource::vtkROIStencilSource()
{
  this->SetNumberOfInputPorts(0);

  this->Shape = vtkROIStencilSource::BOX;

  this->Bounds[0] = 0.0;
  this->Bounds[1] = 0.0;
  this->Bounds[2] = 0.0;
  this->Bounds[3] = 0.0;
  this->Bounds[4] = 0.0;
  this->Bounds[5] = 0.0;
}

//----------------------------------------------------------------------------
vtkROIStencilSource::~vtkROIStencilSource()
{
}

//----------------------------------------------------------------------------
void vtkROIStencilSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Shape: " << this->GetShapeAsString() << "\n";
  os << indent << "Bounds: " << this->Bounds[0] << " "
     << this->Bounds[1] << " " << this->Bounds[2] << " "
     << this->Bounds[3] << " " << this->Bounds[4] << " "
     << this->Bounds[5] << "\n";
}

//----------------------------------------------------------------------------
const char *vtkROIStencilSource::GetShapeAsString()
{
  switch (this->Shape)
    {
    case vtkROIStencilSource::BOX:
      return "Box";
    case vtkROIStencilSource::ELLIPSOID:
      return "Ellipsoid";
    case vtkROIStencilSource::CYLINDERX:
      return "CylinderX";
    case vtkROIStencilSource::CYLINDERY:
      return "CylinderY";
    case vtkROIStencilSource::CYLINDERZ:
      return "CylinderZ";
    }
  return "";
}

//----------------------------------------------------------------------------
// tolerance for stencil operations

#define VTK_STENCIL_TOL 7.62939453125e-06

//----------------------------------------------------------------------------
// Compute a reduced extent based on the Center and Size of the shape.
//
// Also returns the center and radius in voxel-index units.
static void vtkROIStencilSourceSubExtent(
  vtkROIStencilSource *self,
  const double origin[3], const double spacing[3], const int extent[6],
  int subextent[6], double icenter[3], double iradius[3])
{
  double bounds[6];
  self->GetBounds(bounds);

  for (int i = 0; i < 3; i++)
    {
    icenter[i] = (0.5*(bounds[2*i] + bounds[2*i+1]) - origin[i])/spacing[i];
    iradius[i] = 0.5*(bounds[2*i+1] - bounds[2*i])/spacing[i];

    if (iradius[i] < 0)
      {
      iradius[i] = -iradius[i];
      }
    iradius[i] += VTK_STENCIL_TOL;

    double emin = icenter[i] - iradius[i];
    double emax = icenter[i] + iradius[i];

    subextent[2*i] = extent[2*i];
    subextent[2*i+1] = extent[2*i+1];

    if (extent[2*i] < emin)
      {
      subextent[2*i] = VTK_INT_MAX;
      if (extent[2*i+1] >= emin)
        {
        subextent[2*i] = vtkMath::Floor(emin) + 1;
        }
      }

    if (extent[2*i+1] > emax)
      {
      subextent[2*i+1] = VTK_INT_MIN;
      if (extent[2*i] <= emax)
        {
        subextent[2*i+1] = vtkMath::Floor(emax);
        }
      }
    }
}

//----------------------------------------------------------------------------
static int vtkROIStencilSourceBox(
  vtkROIStencilSource *self, vtkImageStencilData *data,
  const int extent[6], const double origin[3], const double spacing[3])
{
  int subextent[6];
  double icenter[3];
  double iradius[3];

  vtkROIStencilSourceSubExtent(self, origin, spacing, extent,
    subextent, icenter, iradius);

  // for keeping track of progress
  unsigned long count = 0;
  unsigned long target = static_cast<unsigned long>(
    (subextent[5] - subextent[4] + 1)*
    (subextent[3] - subextent[2] + 1)/50.0);
  target++;

  for (int idZ = subextent[4]; idZ <= subextent[5]; idZ++)
    {
    for (int idY = subextent[2]; idY <= subextent[3]; idY++)
      {
      if (count%target == 0)
        {
        self->UpdateProgress(count/(50.0*target));
        }
      count++;

      int r1 = subextent[0];
      int r2 = subextent[1];

      if (r2 >= r1)
        {
        data->InsertNextExtent(r1, r2, idY, idZ);
        }
      } // for idY
    } // for idZ

  return 1;
}

//----------------------------------------------------------------------------
static int vtkROIStencilSourceEllipsoid(
  vtkROIStencilSource *self, vtkImageStencilData *data,
  const int extent[6], const double origin[3], const double spacing[3])
{
  int subextent[6];
  double icenter[3];
  double iradius[3];

  vtkROIStencilSourceSubExtent(self, origin, spacing, extent,
    subextent, icenter, iradius);

  // for keeping track of progress
  unsigned long count = 0;
  unsigned long target = static_cast<unsigned long>(
    (subextent[5] - subextent[4] + 1)*
    (subextent[3] - subextent[2] + 1)/50.0);
  target++;

  for (int idZ = subextent[4]; idZ <= subextent[5]; idZ++)
    {
    double z = (idZ - icenter[2])/iradius[2];

    for (int idY = subextent[2]; idY <= subextent[3]; idY++)
      {
      if (count%target == 0)
        {
        self->UpdateProgress(count/(50.0*target));
        }
      count++;

      double y = (idY - icenter[1])/iradius[1];
      double x2 = 1.0 - y*y - z*z;
      if (x2 < 0)
        {
        continue;
        }
      double x = sqrt(x2);

      int r1 = subextent[0];
      int r2 = subextent[1];
      double xmin = icenter[0] - x*iradius[0];
      double xmax = icenter[0] + x*iradius[0];

      if (r1 < xmin)
        {
        r1 = vtkMath::Floor(xmin) + 1;
        }
      if (r2 > xmax)
        {
        r2 = vtkMath::Floor(xmax);
        }

      if (r2 >= r1)
        {
        data->InsertNextExtent(r1, r2, idY, idZ);
        }
      } // for idY
    } // for idZ

  return 1;
}

//----------------------------------------------------------------------------
static int vtkROIStencilSourceCylinderX(
  vtkROIStencilSource *self, vtkImageStencilData *data,
  const int extent[6], const double origin[3], const double spacing[3])
{
  int subextent[6];
  double icenter[3];
  double iradius[3];

  vtkROIStencilSourceSubExtent(self, origin, spacing, extent,
    subextent, icenter, iradius);

  // for keeping track of progress
  unsigned long count = 0;
  unsigned long target = static_cast<unsigned long>(
    (subextent[5] - subextent[4] + 1)*
    (subextent[3] - subextent[2] + 1)/50.0);
  target++;

  for (int idZ = subextent[4]; idZ <= subextent[5]; idZ++)
    {
    double z = (idZ - icenter[2])/iradius[2];

    for (int idY = subextent[2]; idY <= subextent[3]; idY++)
      {
      if (count%target == 0)
        {
        self->UpdateProgress(count/(50.0*target));
        }
      count++;

      double y = (idY - icenter[1])/iradius[1];
      if (y*y + z*z > 1.0)
        {
        continue;
        }

      int r1 = subextent[0];
      int r2 = subextent[1];

      if (r2 >= r1)
        {
        data->InsertNextExtent(r1, r2, idY, idZ);
        }
      } // for idY
    } // for idZ

  return 1;
}

//----------------------------------------------------------------------------
static int vtkROIStencilSourceCylinderY(
  vtkROIStencilSource *self, vtkImageStencilData *data,
  int extent[6], double origin[3], double spacing[3])
{
  int subextent[6];
  double icenter[3];
  double iradius[3];

  vtkROIStencilSourceSubExtent(self, origin, spacing, extent,
    subextent, icenter, iradius);

  // for keeping track of progress
  unsigned long count = 0;
  unsigned long target = static_cast<unsigned long>(
    (subextent[5] - subextent[4] + 1)*
    (subextent[3] - subextent[2] + 1)/50.0);
  target++;

  for (int idZ = subextent[4]; idZ <= subextent[5]; idZ++)
    {
    double z = (idZ - icenter[2])/iradius[2];

    for (int idY = subextent[2]; idY <= subextent[3]; idY++)
      {
      if (count%target == 0)
        {
        self->UpdateProgress(count/(50.0*target));
        }
      count++;

      double x2 = 1.0 - z*z;
      if (x2 < 0)
        {
        continue;
        }
      double x = sqrt(x2);

      int r1 = subextent[0];
      int r2 = subextent[1];
      double xmin = icenter[0] - x*iradius[0];
      double xmax = icenter[0] + x*iradius[0];

      if (r1 < xmin)
        {
        r1 = vtkMath::Floor(xmin) + 1;
        }
      if (r2 > xmax)
        {
        r2 = vtkMath::Floor(xmax);
        }

      if (r2 >= r1)
        {
        data->InsertNextExtent(r1, r2, idY, idZ);
        }
      } // for idY
    } // for idZ

  return 1;
}

//----------------------------------------------------------------------------
static int vtkROIStencilSourceCylinderZ(
  vtkROIStencilSource *self, vtkImageStencilData *data,
  int extent[6], double origin[3], double spacing[3])
{
  int subextent[6];
  double icenter[3];
  double iradius[3];

  vtkROIStencilSourceSubExtent(self, origin, spacing, extent,
    subextent, icenter, iradius);

  // for keeping track of progress
  unsigned long count = 0;
  unsigned long target = static_cast<unsigned long>(
    (subextent[5] - subextent[4] + 1)*
    (subextent[3] - subextent[2] + 1)/50.0);
  target++;

  for (int idZ = subextent[4]; idZ <= subextent[5]; idZ++)
    {
    for (int idY = subextent[2]; idY <= subextent[3]; idY++)
      {
      if (count%target == 0)
        {
        self->UpdateProgress(count/(50.0*target));
        }
      count++;

      double y = (idY - icenter[1])/iradius[1];
      double x2 = 1.0 - y*y;
      if (x2 < 0)
        {
        continue;
        }
      double x = sqrt(x2);

      int r1 = subextent[0];
      int r2 = subextent[1];
      double xmin = icenter[0] - x*iradius[0];
      double xmax = icenter[0] + x*iradius[0];

      if (r1 < xmin)
        {
        r1 = vtkMath::Floor(xmin) + 1;
        }
      if (r2 > xmax)
        {
        r2 = vtkMath::Floor(xmax);
        }

      if (r2 >= r1)
        {
        data->InsertNextExtent(r1, r2, idY, idZ);
        }
      } // for idY
    } // for idZ

  return 1;
}


//----------------------------------------------------------------------------
int vtkROIStencilSource::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  int extent[6];
  double origin[3];
  double spacing[3];
  int result = 1;

  this->Superclass::RequestData(request, inputVector, outputVector);

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageStencilData *data = vtkImageStencilData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent);
  outInfo->Get(vtkDataObject::ORIGIN(), origin);
  outInfo->Get(vtkDataObject::SPACING(), spacing);

  switch (this->Shape)
    {
    case vtkROIStencilSource::BOX:
      result = vtkROIStencilSourceBox(
        this, data, extent, origin, spacing);
      break;
    case vtkROIStencilSource::ELLIPSOID:
      result = vtkROIStencilSourceEllipsoid(
        this, data, extent, origin, spacing);
      break;
    case vtkROIStencilSource::CYLINDERX:
      result = vtkROIStencilSourceCylinderX(
        this, data, extent, origin, spacing);
      break;
    case vtkROIStencilSource::CYLINDERY:
      result = vtkROIStencilSourceCylinderY(
        this, data, extent, origin, spacing);
      break;
    case vtkROIStencilSource::CYLINDERZ:
      result = vtkROIStencilSourceCylinderZ(
        this, data, extent, origin, spacing);
      break;
    }

  return result;
}
