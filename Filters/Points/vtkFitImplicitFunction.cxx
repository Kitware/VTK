/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFitImplicitFunction.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFitImplicitFunction.h"

#include "vtkObjectFactory.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkImplicitFunction.h"
#include "vtkSMPTools.h"

vtkStandardNewMacro(vtkFitImplicitFunction);
vtkCxxSetObjectMacro(vtkFitImplicitFunction,ImplicitFunction,vtkImplicitFunction);

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace {

//----------------------------------------------------------------------------
// The threaded core of the algorithm
template <typename T>
struct ExtractPoints
{
  const T *Points;
  vtkImplicitFunction *Function;
  double Threshold;
  vtkIdType *PointMap;

  ExtractPoints(T *points, vtkImplicitFunction *f, double thresh, vtkIdType *map) :
    Points(points), Function(f), Threshold(thresh), PointMap(map)
  {
  }

  void operator() (vtkIdType ptId, vtkIdType endPtId)
  {
      const T *p = this->Points + 3*ptId;
      vtkIdType *map = this->PointMap + ptId;
      vtkImplicitFunction *f = this->Function;
      double x[3], val;
      double tMin = (-this->Threshold);
      double tMax = this->Threshold;

      for ( ; ptId < endPtId; ++ptId)
      {
        x[0] = static_cast<double>(*p++);
        x[1] = static_cast<double>(*p++);
        x[2] = static_cast<double>(*p++);

        val = f->FunctionValue(x);
        *map++ = ( (val >= tMin && val < tMax) ? 1 : -1 );
      }
  }

  static void Execute(vtkFitImplicitFunction *self, vtkIdType numPts,
                      T *points, vtkIdType *map)
  {
      ExtractPoints extract(points, self->GetImplicitFunction(),
                            self->GetThreshold(), map);
      vtkSMPTools::For(0, numPts, extract);
  }

}; //ExtractPoints

} //anonymous namespace

//================= Begin class proper =======================================
//----------------------------------------------------------------------------
vtkFitImplicitFunction::vtkFitImplicitFunction()
{
  this->ImplicitFunction = NULL;
  this->Threshold = 0.01;
}

//----------------------------------------------------------------------------
vtkFitImplicitFunction::~vtkFitImplicitFunction()
{
  this->SetImplicitFunction(NULL);
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If implicit function is modified,
// then this object is modified as well.
vtkMTimeType vtkFitImplicitFunction::GetMTime()
{
  vtkMTimeType mTime=this->MTime.GetMTime();
  vtkMTimeType impFuncMTime;

  if ( this->ImplicitFunction != NULL )
  {
    impFuncMTime = this->ImplicitFunction->GetMTime();
    mTime = ( impFuncMTime > mTime ? impFuncMTime : mTime );
  }

  return mTime;
}

//----------------------------------------------------------------------------
// Traverse all the input points and extract those that lie near the surface
// of an implicit function.
int vtkFitImplicitFunction::FilterPoints(vtkPointSet *input)
{
  // Check the input.
  if ( !this->ImplicitFunction )
  {
    vtkErrorMacro(<<"Implicit function required\n");
    return 0;
  }

  // Determine which points, if any, should be removed. We create a map
  // to keep track. The bulk of the algorithmic work is done in this pass.
  vtkIdType numPts = input->GetNumberOfPoints();
  void *inPtr = input->GetPoints()->GetVoidPointer(0);
  switch (input->GetPoints()->GetDataType())
  {
    vtkTemplateMacro(ExtractPoints<VTK_TT>::
                     Execute(this, numPts, (VTK_TT *)inPtr, this->PointMap));
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkFitImplicitFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Implicit Function: "
     << static_cast<void *>(this->ImplicitFunction) << "\n";
  os << indent << "Threshold: " << this->Threshold << "\n";
}
