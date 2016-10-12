/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractPoints.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractPoints.h"

#include "vtkObjectFactory.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkImplicitFunction.h"
#include "vtkSMPTools.h"

vtkStandardNewMacro(vtkExtractPoints);
vtkCxxSetObjectMacro(vtkExtractPoints,ImplicitFunction,vtkImplicitFunction);

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
  bool ExtractInside;
  vtkIdType *PointMap;

  ExtractPoints(T *points, vtkImplicitFunction *f, bool inside, vtkIdType *map) :
    Points(points), Function(f), ExtractInside(inside), PointMap(map)
  {
  }

  void operator() (vtkIdType ptId, vtkIdType endPtId)
  {
      const T *p = this->Points + 3*ptId;
      vtkIdType *map = this->PointMap + ptId;
      vtkImplicitFunction *f = this->Function;
      double x[3];
      double inside = (this->ExtractInside ? 1.0 : -1.0);

      for ( ; ptId < endPtId; ++ptId)
      {
        x[0] = static_cast<double>(*p++);
        x[1] = static_cast<double>(*p++);
        x[2] = static_cast<double>(*p++);

        *map++ = ((f->FunctionValue(x) * inside) <= 0.0 ? 1 : -1 );
      }
  }

  static void Execute(vtkExtractPoints *self, vtkIdType numPts,
                      T *points, vtkIdType *map)
  {
      ExtractPoints extract(points, self->GetImplicitFunction(),
                            self->GetExtractInside(), map);
      vtkSMPTools::For(0, numPts, extract);
  }

}; //ExtractPoints

} //anonymous namespace

//================= Begin class proper =======================================
//----------------------------------------------------------------------------
vtkExtractPoints::vtkExtractPoints()
{
  this->ImplicitFunction = NULL;
  this->ExtractInside = true;
}

//----------------------------------------------------------------------------
vtkExtractPoints::~vtkExtractPoints()
{
  this->SetImplicitFunction(NULL);
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If implicit function is modified,
// then this object is modified as well.
vtkMTimeType vtkExtractPoints::GetMTime()
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
// Traverse all the input points and extract points that are contained within
// and implicit function.
int vtkExtractPoints::FilterPoints(vtkPointSet *input)
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
void vtkExtractPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Implicit Function: "
     << static_cast<void *>(this->ImplicitFunction) << "\n";
  os << indent << "Extract Inside: "
     << (this->ExtractInside ? "On\n" : "Off\n");
}
