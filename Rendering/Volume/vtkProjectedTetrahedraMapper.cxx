/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProjectedTetrahedraMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkProjectedTetrahedraMapper.h"

#include "vtkCellArray.h"
#include "vtkCellCenterDepthSort.h"
#include "vtkCellData.h"
#include "vtkColorTransferFunction.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGarbageCollector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVisibilitySort.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

#include <math.h>
#include <algorithm>

//-----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkProjectedTetrahedraMapper,
                     VisibilitySort, vtkVisibilitySort);

//-----------------------------------------------------------------------------
// Return NULL if no override is supplied.
vtkAbstractObjectFactoryNewMacro(vtkProjectedTetrahedraMapper)

//-----------------------------------------------------------------------------

vtkProjectedTetrahedraMapper::vtkProjectedTetrahedraMapper()
{
  this->VisibilitySort = vtkCellCenterDepthSort::New();
}

vtkProjectedTetrahedraMapper::~vtkProjectedTetrahedraMapper()
{
  this->SetVisibilitySort(NULL);
}

void vtkProjectedTetrahedraMapper::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "VisibilitySort: " << this->VisibilitySort << endl;

}

//-----------------------------------------------------------------------------

void vtkProjectedTetrahedraMapper::ReportReferences(vtkGarbageCollector *collector)
{
  this->Superclass::ReportReferences(collector);

  vtkGarbageCollectorReport(collector, this->VisibilitySort, "VisibilitySort");
}

//-----------------------------------------------------------------------------

template<class point_type>
void vtkProjectedTetrahedraMapperTransformPoints(const point_type *in_points,
                                                 vtkIdType num_points,
                                                 const float projection_mat[16],
                                                 const float modelview_mat[16],
                                                 float *out_points)
{
  float mat[16];
  int row, col;
  vtkIdType i;
  const point_type *in_p;
  float *out_p;

  // Combine two transforms into one transform.
  for (col = 0; col < 4; col++)
    {
    for (row = 0; row < 4; row++)
      {
      mat[col*4+row] = (  projection_mat[0*4+row]*modelview_mat[col*4+0]
                        + projection_mat[1*4+row]*modelview_mat[col*4+1]
                        + projection_mat[2*4+row]*modelview_mat[col*4+2]
                        + projection_mat[3*4+row]*modelview_mat[col*4+3]);
      }
    }

  // Transform all points.
  for (i = 0, in_p = in_points, out_p = out_points; i < num_points;
       i++, in_p += 3, out_p += 3)
    {
    for (row = 0; row < 3; row++)
      {
      out_p[row] = (  mat[0*4+row]*in_p[0] + mat[1*4+row]*in_p[1]
                    + mat[2*4+row]*in_p[2] + mat[3*4+row]);
      }
    }

  // Check to see if we need to divide by w.
  if (   (mat[0*4+3] != 0) || (mat[1*4+3] != 0)
      || (mat[2*4+3] != 0) || (mat[3*4+3] != 1) )
    {
    for (i = 0, in_p = in_points, out_p = out_points; i < num_points;
         i++, in_p += 3, out_p += 3)
      {
      float w = (  mat[0*4+3]*in_p[0] + mat[1*4+3]*in_p[1]
                 + mat[2*4+3]*in_p[2] + mat[3*4+3]);
      if (w > 0.0)
        {
        out_p[0] /= w;
        out_p[1] /= w;
        out_p[2] /= w;
        }
      else
        {
        // A negative w probably means the point is behind the viewer.  Things
        // can get screwy if we try to inverse-project that.  Instead, just
        // set the position somewhere very far behind us.
        out_p[2] = -VTK_LARGE_FLOAT;
        }
      }
    }
}

void vtkProjectedTetrahedraMapper::TransformPoints(
                                                 vtkPoints *inPoints,
                                                 const float projection_mat[16],
                                                 const float modelview_mat[16],
                                                 vtkFloatArray *outPoints)
{
  outPoints->SetNumberOfComponents(3);
  outPoints->SetNumberOfTuples(inPoints->GetNumberOfPoints());
  switch (inPoints->GetDataType())
    {
    vtkTemplateMacro(vtkProjectedTetrahedraMapperTransformPoints(
                                    (const VTK_TT *)inPoints->GetVoidPointer(0),
                                     inPoints->GetNumberOfPoints(),
                                     projection_mat, modelview_mat,
                                     outPoints->GetPointer(0)));
    }
}

//-----------------------------------------------------------------------------

namespace vtkProjectedTetrahedraMapperNamespace
{
  template<class ColorType>
  void MapScalarsToColors1(ColorType *colors, vtkVolumeProperty *property,
                           vtkDataArray *scalars);
  template<class ColorType, class ScalarType>
  void MapScalarsToColors2(ColorType *colors, vtkVolumeProperty *property,
                           ScalarType *scalars,
                           int num_scalar_components,
                           vtkIdType num_scalars);
  template<class ColorType, class ScalarType>
  void MapIndependentComponents(ColorType *colors,
                                vtkVolumeProperty *property,
                                ScalarType *scalars,
                                int num_scalar_components,
                                vtkIdType num_scalars);
  template<class ColorType, class ScalarType>
  void Map2DependentComponents(ColorType *colors,
                               vtkVolumeProperty *property,
                               ScalarType *scalars,
                               vtkIdType num_scalars);
  template<class ColorType, class ScalarType>
  void Map4DependentComponents(ColorType *colors, ScalarType *scalars,
                               vtkIdType num_scalars);
}

void vtkProjectedTetrahedraMapper::MapScalarsToColors(
                                                    vtkDataArray *colors,
                                                    vtkVolumeProperty *property,
                                                    vtkDataArray *scalars)
{
  using namespace vtkProjectedTetrahedraMapperNamespace;

  vtkDataArray *tmpColors;
  int castColors;

  if (   (colors->GetDataType() == VTK_UNSIGNED_CHAR)
         && ((   (scalars->GetDataType() != VTK_UNSIGNED_CHAR)
                 || (property->GetIndependentComponents()) )
             || ((!property->GetIndependentComponents())
                 && (scalars->GetNumberOfComponents() == 2))) )
    {
    // Special case.  Need to convert from range [0,1] to [0,255].
    tmpColors = vtkDoubleArray::New();
    castColors = 1;
    }
  else
    {
    tmpColors = colors;
    castColors = 0;
    }

  vtkIdType numscalars = scalars->GetNumberOfTuples();

  tmpColors->Initialize();
  tmpColors->SetNumberOfComponents(4);
  tmpColors->SetNumberOfTuples(numscalars);

  void *colorpointer = tmpColors->GetVoidPointer(0);
  switch (tmpColors->GetDataType())
    {
    vtkTemplateMacro(MapScalarsToColors1(static_cast<VTK_TT *>(colorpointer),
                                         property, scalars));
    }

  if (castColors)
    {
    // Special case.  Need to convert from range [0,1] to [0,255].
    colors->Initialize();
    colors->SetNumberOfComponents(4);
    colors->SetNumberOfTuples(scalars->GetNumberOfTuples());

    unsigned char *c
      = static_cast<vtkUnsignedCharArray *>(colors)->GetPointer(0);

    for (vtkIdType i = 0; i < numscalars; i++, c+= 4)
      {
      double *dc = tmpColors->GetTuple(i);
      c[0] = static_cast<unsigned char>(dc[0]*255.9999);
      c[1] = static_cast<unsigned char>(dc[1]*255.9999);
      c[2] = static_cast<unsigned char>(dc[2]*255.9999);
      c[3] = static_cast<unsigned char>(dc[3]*255.9999);
      }

    tmpColors->Delete();
    }
}

//-----------------------------------------------------------------------------
namespace vtkProjectedTetrahedraMapperNamespace
{

  template<class ColorType>
  void MapScalarsToColors1(ColorType *colors, vtkVolumeProperty *property,
                           vtkDataArray *scalars)
  {
    void *scalarpointer = scalars->GetVoidPointer(0);
    switch(scalars->GetDataType())
      {
      vtkTemplateMacro(MapScalarsToColors2(colors, property,
                                           static_cast<VTK_TT *>(scalarpointer),
                                           scalars->GetNumberOfComponents(),
                                           scalars->GetNumberOfTuples()));
      }
  }

  template<class ColorType, class ScalarType>
  void MapScalarsToColors2(ColorType *colors, vtkVolumeProperty *property,
                           ScalarType *scalars,
                           int num_scalar_components, vtkIdType num_scalars)
  {
    if (property->GetIndependentComponents())
      {
      MapIndependentComponents(colors, property,
                               scalars, num_scalar_components, num_scalars);
      }
    else
      {
      switch (num_scalar_components)
        {
        case 2:
          Map2DependentComponents(colors, property, scalars, num_scalars);
          break;
        case 4:
          Map4DependentComponents(colors, scalars, num_scalars);
          break;
        default:
          vtkGenericWarningMacro("Attempted to map scalar with "
                                 << num_scalar_components
                                 << " with dependent components");
          break;
        }
      }
  }

  template<class ColorType, class ScalarType>
  void MapIndependentComponents(ColorType *colors,
                                vtkVolumeProperty *property,
                                ScalarType *scalars,
                                int num_scalar_components,
                                vtkIdType num_scalars)
  {
    // I don't really know what to do if there is more than one component.
    // How am I supposed to mix the resulting colors?  Since I don't know
    // what to do, and the whole thing seems kinda pointless anyway, I'm just
    // going to punt and copy over the first scalar.
    ColorType *c = colors;
    ScalarType *s = scalars;
    vtkIdType i;

    if (property->GetColorChannels() == 1)
      {
      vtkPiecewiseFunction *gray = property->GetGrayTransferFunction();
      vtkPiecewiseFunction *alpha = property->GetScalarOpacity();

      for (i = 0; i < num_scalars; i++, c += 4, s += num_scalar_components)
        {
        c[0] = c[1] = c[2] = static_cast<ColorType>(gray->GetValue(s[0]));
        c[3] = static_cast<ColorType>(alpha->GetValue(s[0]));
        }
      }
    else
      {
      vtkColorTransferFunction *rgb = property->GetRGBTransferFunction();
      vtkPiecewiseFunction *alpha = property->GetScalarOpacity();

      for (i = 0; i < num_scalars; i++, c += 4, s += num_scalar_components)
        {
        double trgb[3];
        rgb->GetColor(s[0], trgb);
        c[0] = static_cast<ColorType>(trgb[0]);
        c[1] = static_cast<ColorType>(trgb[1]);
        c[2] = static_cast<ColorType>(trgb[2]);
        c[3] = static_cast<ColorType>(alpha->GetValue(s[0]));
        }
      }
  }

  template<class ColorType, class ScalarType>
  void Map2DependentComponents(ColorType *colors, vtkVolumeProperty *property,
                               ScalarType *scalars, vtkIdType num_scalars)
  {
    vtkColorTransferFunction *rgb = property->GetRGBTransferFunction();
    vtkPiecewiseFunction *alpha = property->GetScalarOpacity();
    double rgbColor[3];

    for (vtkIdType i = 0; i < num_scalars; i++)
      {
      rgb->GetColor(scalars[0], rgbColor);
      colors[0] = static_cast<ColorType>(rgbColor[0]);
      colors[1] = static_cast<ColorType>(rgbColor[1]);
      colors[2] = static_cast<ColorType>(rgbColor[2]);
      colors[3] = static_cast<ColorType>(alpha->GetValue(scalars[1]));

      colors += 4;
      scalars += 2;
      }
  }

  template<class ColorType, class ScalarType>
  void Map4DependentComponents(ColorType *colors, ScalarType *scalars,
                               vtkIdType num_scalars)
  {
    for (vtkIdType i = 0; i < num_scalars; i++)
      {
      colors[0] = static_cast<ColorType>(scalars[0]);
      colors[1] = static_cast<ColorType>(scalars[1]);
      colors[2] = static_cast<ColorType>(scalars[2]);
      colors[3] = static_cast<ColorType>(scalars[3]);

      colors += 4;
      scalars += 4;
      }
  }

}
