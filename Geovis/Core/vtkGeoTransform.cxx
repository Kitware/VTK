// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkGeoTransform.h"

#include "vtkDoubleArray.h"
#include "vtkGeoProjection.h"
#include "vtkMath.h"
#include "vtkPoints.h"

#include "vtk_libproj.h"
#include <cmath>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkGeoTransform);
vtkCxxSetObjectMacro(vtkGeoTransform, SourceProjection, vtkGeoProjection);
vtkCxxSetObjectMacro(vtkGeoTransform, DestinationProjection, vtkGeoProjection);

vtkGeoTransform::vtkGeoTransform()
{
  this->SourceProjection = nullptr;
  this->DestinationProjection = nullptr;
  this->TransformZCoordinate = false;
}

vtkGeoTransform::~vtkGeoTransform()
{
  if (this->SourceProjection)
  {
    this->SourceProjection->Delete();
  }
  if (this->DestinationProjection)
  {
    this->DestinationProjection->Delete();
  }
}

void vtkGeoTransform::SetSourceProjection(const char* proj)
{
  if (this->SourceProjection)
  {
    this->SourceProjection->Delete();
  }
  this->SourceProjection = vtkGeoProjection::New();
  this->SourceProjection->SetPROJ4String(proj);
}

void vtkGeoTransform::SetDestinationProjection(const char* proj)
{
  if (this->DestinationProjection)
  {
    this->DestinationProjection->Delete();
  }
  this->DestinationProjection = vtkGeoProjection::New();
  this->DestinationProjection->SetPROJ4String(proj);
}

void vtkGeoTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SourceProjection: " << this->SourceProjection << "\n";
  os << indent << "DestinationProjection: " << this->DestinationProjection << "\n";
}

void vtkGeoTransform::TransformPoints(vtkPoints* srcPts, vtkPoints* dstPts)
{
  if (!srcPts || !dstPts)
  {
    return;
  }

  vtkDoubleArray* srcCoords = vtkArrayDownCast<vtkDoubleArray>(srcPts->GetData());
  vtkDoubleArray* dstCoords = vtkArrayDownCast<vtkDoubleArray>(dstPts->GetData());
  if (!srcCoords || !dstCoords)
  {
    // data not in a form we can use directly anyway...
    double coord[3];
    vtkIdType numPts = srcPts->GetNumberOfPoints();
    for (vtkIdType ptId = 0; ptId < numPts; ++ptId)
    {
      srcPts->GetPoint(ptId, coord);
      this->InternalTransformPoint(coord, coord);
      dstPts->InsertNextPoint(coord);
    }
    return;
  }
  dstCoords->DeepCopy(srcCoords);

  projPJ src = this->SourceProjection ? this->SourceProjection->GetProjection() : nullptr;
  projPJ dst = this->DestinationProjection ? this->DestinationProjection->GetProjection() : nullptr;
  if (!src && !dst)
  {
    // we've already copied srcCoords to dstCoords and src=dst=0 implies no transform...
    return;
  }

  if (srcCoords->GetNumberOfComponents() < 2)
  {
    vtkErrorMacro(<< "Source coordinate array " << srcCoords << " only has "
                  << srcCoords->GetNumberOfComponents()
                  << " components and at least 2 are required for geographic projections.");
    return;
  }

  this->InternalTransformPoints(
    dstCoords->GetPointer(0), dstCoords->GetNumberOfTuples(), dstCoords->GetNumberOfComponents());
}

void vtkGeoTransform::TransformPointsNormalsVectors(vtkPoints* srcPts, vtkPoints* dstPts,
  vtkDataArray* srcNms, vtkDataArray*, vtkDataArray* srcVrs, vtkDataArray*, int nOptionalVectors,
  vtkDataArray**, vtkDataArray**)
{
  this->TransformPoints(srcPts, dstPts);

  if (srcNms || srcVrs || nOptionalVectors)
  {
    vtkErrorMacro("Error: Normal transformation is not implemented. Please remove normals"
                  " using vtkPassSelectedArrays and regenerate normals after the transform.");
  }
}

void vtkGeoTransform::Inverse()
{
  vtkGeoProjection* tmp = this->SourceProjection;
  this->SourceProjection = this->DestinationProjection;
  this->DestinationProjection = tmp;
  this->Modified();
}

void vtkGeoTransform::InternalDeepCopy(vtkAbstractTransform* transform)
{
  vtkGeoTransform* t = static_cast<vtkGeoTransform*>(transform);
  this->SetSourceProjection(t->GetSourceProjection());
  this->SetDestinationProjection(t->GetDestinationProjection());
  this->SetTransformZCoordinate(t->GetTransformZCoordinate());
}

void vtkGeoTransform::InternalTransformPoint(const float in[3], float out[3])
{
  double ind[3];
  double oud[3];
  for (int i = 0; i < 3; ++i)
  {
    ind[i] = in[i];
  }
  this->InternalTransformPoint(ind, oud);
  for (int i = 0; i < 3; ++i)
  {
    out[i] = static_cast<float>(oud[i]);
  }
}

void vtkGeoTransform::InternalTransformPoint(const double in[3], double out[3])
{
  for (int i = 0; i < 3; ++i)
  {
    out[i] = in[i];
  }
  this->InternalTransformPoints(out, 1, 3);
}

void vtkGeoTransform::InternalTransformDerivative(
  const float in[3], float out[3], float derivative[3][3])
{
  double ind[3];
  double oud[3];
  double drd[3][3];
  int i;
  for (i = 0; i < 3; ++i)
    ind[i] = in[i];
  this->InternalTransformDerivative(ind, oud, drd);
  for (i = 0; i < 3; ++i)
  {
    out[i] = static_cast<float>(oud[i]);
    for (int j = 0; j < 3; ++j)
    {
      derivative[i][j] = drd[i][j];
    }
  }
}

void vtkGeoTransform::InternalTransformDerivative(
  const double in[3], double out[3], double derivative[3][3])
{
  this->InternalTransformPoint(in, out);

  // FIXME: Need to use pj_factors for both source and inverted dest projection
  vtkMath::Identity3x3(derivative);
  vtkErrorMacro("Error: Normal transformation is not implemented. Please remove normals"
                " using vtkPassSelectedArrays and regenerate normals after the transform.");
}

vtkAbstractTransform* vtkGeoTransform::MakeTransform()
{
  vtkGeoTransform* geoTrans = vtkGeoTransform::New();
  return geoTrans;
}

void vtkGeoTransform::InternalTransformPoints(double* x, vtkIdType numPts, int stride)
{
#if PROJ_VERSION_MAJOR < 5
  vtkErrorMacro("VTK requires proj version >= 5.0.0 ");
#else
  projPJ src = this->SourceProjection ? this->SourceProjection->GetProjection() : nullptr;
  projPJ dst = this->DestinationProjection ? this->DestinationProjection->GetProjection() : nullptr;
  int delta = stride - 2;
  if (!this->TransformZCoordinate)
  {
    PJ_COORD c, c_out;
    if (src)
    {
      // Convert from src system to lat/long using inverse of src transform
      double* coord = x;
      for (vtkIdType i = 0; i < numPts; ++i)
      {
        c.xy.x = coord[0];
        c.xy.y = coord[1];
        c_out = proj_trans(src, PJ_INV, c);
        coord[0] = c_out.lp.lam;
        coord[1] = c_out.lp.phi;
        coord += stride;
      }
    }
    else // ! src
    {
      // src coords are in degrees, convert to radians
      double* coord = x;
      for (vtkIdType i = 0; i < numPts; ++i)
      {
        for (int j = 0; j < 2; ++j, ++coord)
        {
          *coord = vtkMath::RadiansFromDegrees(*coord);
        }
        coord += delta;
      }
    }
    if (dst)
    {
      double* coord = x;
      for (vtkIdType i = 0; i < numPts; ++i)
      {
        c.lp.lam = coord[0];
        c.lp.phi = coord[1];
        c_out = proj_trans(dst, PJ_FWD, c);
        coord[0] = c_out.xy.x;
        coord[1] = c_out.xy.y;
        coord += stride;
      }
    }
    else // ! dst
    {
      // dst coords are in radians, convert to degrees
      double* coord = x;
      for (vtkIdType i = 0; i < numPts; ++i)
      {
        for (int j = 0; j < 2; ++j, ++coord)
        {
          *coord = vtkMath::DegreesFromRadians(*coord);
        }
        coord += delta;
      }
    }
  }
  else
  {
    PJ* P;
    P = proj_create_crs_to_crs(PJ_DEFAULT_CTX, this->SourceProjection->GetPROJ4String(),
      this->DestinationProjection->GetPROJ4String(), nullptr);
    if (P == nullptr)
    {
      vtkErrorMacro("proj_create_crs_to_crs failed: " << proj_errno_string(proj_errno(nullptr)));
      return;
    }
    /* For that particular use case, this is not needed. */
    /* proj_normalize_for_visualization() ensures that the coordinate */
    /* order expected and returned by proj_trans() will be longitude, */
    /* latitude for geographic CRS, and easting, northing for projected */
    /* CRS. If instead of using PROJ strings as above, "EPSG:XXXX" codes */
    /* had been used, this might had been necessary. */
    PJ* P_for_GIS = proj_normalize_for_visualization(PJ_DEFAULT_CTX, P);
    if (P_for_GIS == nullptr)
    {
      proj_destroy(P);
      vtkErrorMacro(
        "proj_normalize_for_visualization failed: " << proj_errno_string(proj_errno(nullptr)));
      return;
    }
    proj_destroy(P);
    P = P_for_GIS;

    proj_trans_generic(P, PJ_FWD, x, sizeof(*x) * stride, numPts, x + 1, sizeof(*x) * stride,
      numPts, x + 2, sizeof(*x) * stride, numPts, nullptr, 0, 0);
    proj_destroy(P);
  }
#endif
}

int vtkGeoTransform::ComputeUTMZone(double lon, double lat)
{
  lon = std::fmod(lon + 180, 360) - 180;
  lat = std::fmod(lat + 90, 180) - 90;
  int result = 0;
  // UTM is not defined outside of these limits
  if (lat <= 84 && lat >= -80)
  {
    // first special case
    if (lat >= 72 && lon >= 0 && lon < 42)
    {
      if (lon < 9)
      {
        result = 31;
      }
      else if (lon < 21)
      {
        result = 33;
      }
      else if (lon < 33)
      {
        result = 35;
      }
      else
      {
        result = 37;
      }
    }
    // second special case
    else if (lat >= 56 && lat < 64 && lon >= 0 && lon < 12)
    {
      if (lon < 3)
      {
        result = 31;
      }
      else
      {
        result = 32;
      }
    }
    else
    {
      // general case: zones are 6 degrees, from 1 to 60.
      result = (static_cast<int>(lon) + 180) / 6 + 1;
    }
  }
  return result;
}
VTK_ABI_NAMESPACE_END
