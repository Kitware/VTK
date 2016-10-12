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

#include "vtkArrayDispatch.h"
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

#include <cmath>
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
namespace {
struct TransformPointsWorker
{
  const float *Proj;
  const float *ModelView;
  float *OutPoints;

  TransformPointsWorker(const float *proj, const float *mv, float *out)
    : Proj(proj), ModelView(mv), OutPoints(out)
  {}

  template<class ArrayT>
  void operator()(ArrayT *in_points)
  {
    float mat[16];
    int row, col;
    vtkIdType i;
    vtkIdType num_points = in_points->GetNumberOfTuples();
    typename ArrayT::ValueType in_p[3];
    float *out_p;

    // Combine two transforms into one transform.
    for (col = 0; col < 4; col++)
    {
      for (row = 0; row < 4; row++)
      {
        mat[col*4+row] = (  this->Proj[0*4+row] * this->ModelView[col*4+0]
                          + this->Proj[1*4+row] * this->ModelView[col*4+1]
                          + this->Proj[2*4+row] * this->ModelView[col*4+2]
                          + this->Proj[3*4+row] * this->ModelView[col*4+3]);
      }
    }

    // Transform all points.
    for (i = 0, out_p = this->OutPoints; i < num_points; i++, out_p += 3)
    {
      in_points->GetTypedTuple(i, in_p);
      for (row = 0; row < 3; row++)
      {
        out_p[row] = (  mat[0*4+row] * in_p[0] + mat[1*4+row] * in_p[1]
                      + mat[2*4+row] * in_p[2] + mat[3*4+row]);
      }
    }

    // Check to see if we need to divide by w.
    if (   (mat[0*4+3] != 0) || (mat[1*4+3] != 0)
        || (mat[2*4+3] != 0) || (mat[3*4+3] != 1) )
    {
      for (i = 0, out_p = this->OutPoints; i < num_points; i++, out_p += 3)
      {
        in_points->GetTypedTuple(i, in_p);
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
          out_p[2] = -VTK_FLOAT_MAX;
        }
      }
    }
  }

};
} // end anon namespace

//-----------------------------------------------------------------------------
void vtkProjectedTetrahedraMapper::TransformPoints(
                                                 vtkPoints *inPoints,
                                                 const float projection_mat[16],
                                                 const float modelview_mat[16],
                                                 vtkFloatArray *outPoints)
{
  if (!inPoints)
  {
    return;
  }
  outPoints->SetNumberOfComponents(3);
  outPoints->SetNumberOfTuples(inPoints->GetNumberOfPoints());
  TransformPointsWorker worker(projection_mat, modelview_mat,
                               outPoints->GetPointer(0));
  vtkArrayDispatch::Dispatch::Execute(inPoints->GetData(), worker);
}

//-----------------------------------------------------------------------------

namespace vtkProjectedTetrahedraMapperNamespace
{
  template <typename ColorArrayT, typename ScalarArrayT>
  void MapScalarsToColorsImpl(ColorArrayT *colors, vtkVolumeProperty *property,
                              ScalarArrayT *scalars);
  template <typename ColorArrayT, typename ScalarArrayT>
  void MapIndependentComponents(ColorArrayT *colors,
                                vtkVolumeProperty *property,
                                ScalarArrayT *scalars);
  template <typename ColorArrayT, typename ScalarArrayT>
  void Map2DependentComponents(ColorArrayT *colors, vtkVolumeProperty *property,
                               ScalarArrayT *scalars);
  template <typename ColorArrayT, typename ScalarArrayT>
  void Map4DependentComponents(ColorArrayT *colors, ScalarArrayT *scalars);

  struct Worker
  {
    vtkVolumeProperty *Property;

    Worker(vtkVolumeProperty *property) : Property(property) {}

    template <typename ColorArrayT, typename ScalarArrayT>
    void operator()(ColorArrayT *colors, ScalarArrayT *scalars)
    {
      MapScalarsToColorsImpl(colors, this->Property, scalars);
    }
  };
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

  Worker worker(property);
  if (!vtkArrayDispatch::Dispatch2::Execute(tmpColors, scalars, worker))
  {
    vtkGenericWarningMacro("Dispatch failed for scalar array "
                           << scalars->GetName());
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
  template <typename ColorArrayT, typename ScalarArrayT>
  void MapScalarsToColorsImpl(ColorArrayT *colors, vtkVolumeProperty *property,
                              ScalarArrayT *scalars)
  {
    if (property->GetIndependentComponents())
    {
      MapIndependentComponents(colors, property, scalars);
    }
    else
    {
      switch (scalars->GetNumberOfComponents())
      {
        case 2:
          Map2DependentComponents(colors, property, scalars);
          break;
        case 4:
          Map4DependentComponents(colors, scalars);
          break;
        default:
          vtkGenericWarningMacro("Attempted to map scalar with "
                                 << scalars->GetNumberOfComponents()
                                 << " with dependent components");
          break;
      }
    }
  }

  template <typename ColorArrayT, typename ScalarArrayT>
  void MapIndependentComponents(ColorArrayT *colors,
                                vtkVolumeProperty *property,
                                ScalarArrayT *scalars)
  {
    // I don't really know what to do if there is more than one component.
    // How am I supposed to mix the resulting colors?  Since I don't know
    // what to do, and the whole thing seems kinda pointless anyway, I'm just
    // going to punt and copy over the first scalar.
    vtkIdType i;
    vtkIdType num_scalars = scalars->GetNumberOfTuples();

    typedef typename ScalarArrayT::ValueType ScalarType;
    typedef typename ColorArrayT::ValueType ColorType;
    ColorType c[4];

    if (property->GetColorChannels() == 1)
    {
      vtkPiecewiseFunction *gray = property->GetGrayTransferFunction();
      vtkPiecewiseFunction *alpha = property->GetScalarOpacity();

      for (i = 0; i < num_scalars; i++)
      {
        ScalarType s = scalars->GetTypedComponent(i, 0);
        c[0] = c[1] = c[2] = static_cast<ColorType>(gray->GetValue(s));
        c[3] = static_cast<ColorType>(alpha->GetValue(s));
        colors->SetTypedTuple(i, c);
      }
    }
    else
    {
      vtkColorTransferFunction *rgb = property->GetRGBTransferFunction();
      vtkPiecewiseFunction *alpha = property->GetScalarOpacity();

      for (i = 0; i < num_scalars; i++)
      {
        ScalarType s = scalars->GetTypedComponent(i, 0);
        double trgb[3];
        rgb->GetColor(s, trgb);
        c[0] = static_cast<ColorType>(trgb[0]);
        c[1] = static_cast<ColorType>(trgb[1]);
        c[2] = static_cast<ColorType>(trgb[2]);
        c[3] = static_cast<ColorType>(alpha->GetValue(s));
        colors->SetTypedTuple(i, c);
      }
    }
  }

  template <typename ColorArrayT, typename ScalarArrayT>
  void Map2DependentComponents(ColorArrayT *colors, vtkVolumeProperty *property,
                               ScalarArrayT *scalars)
  {
    typedef typename ScalarArrayT::ValueType ScalarType;
    vtkColorTransferFunction *rgb = property->GetRGBTransferFunction();
    vtkPiecewiseFunction *alpha = property->GetScalarOpacity();
    vtkIdType num_scalars = scalars->GetNumberOfTuples();
    double rgbColor[4];
    ScalarType scalar[2];

    for (vtkIdType i = 0; i < num_scalars; i++)
    {
      scalars->GetTypedTuple(i, scalar);
      rgb->GetColor(scalar[0], rgbColor);
      rgbColor[3] = alpha->GetValue(scalar[1]);
      colors->SetTuple(i, rgbColor);
    }
  }

  template <typename ColorArrayT, typename ScalarArrayT>
  void Map4DependentComponents(ColorArrayT *colors, ScalarArrayT *scalars)
  {
    double val[4];
    vtkIdType num_scalars = scalars->GetNumberOfTuples();
    for (vtkIdType i = 0; i < num_scalars; i++)
    {
      scalars->GetTuple(i, val);
      colors->SetTuple(i, val);
    }
  }

}
