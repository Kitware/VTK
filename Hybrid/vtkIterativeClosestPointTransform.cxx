/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIterativeClosestPointTransform.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Sebastien Barre who developed this class. Thanks to
             Tim Hutton too for the idea.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkIterativeClosestPointTransform.h"
#include "vtkCellLocator.h"
#include "vtkLandmarkTransform.h"
#include "vtkTransform.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"

//--------------------------------------------------------------------------

vtkIterativeClosestPointTransform* vtkIterativeClosestPointTransform::New()
{
  // First try to create the object from the vtkObjectFactory

  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkIterativeClosestPointTransform");
  if(ret)
    {
    return (vtkIterativeClosestPointTransform*)ret;
    }

  // If the factory was unable to create the object, then create it here.

  return new vtkIterativeClosestPointTransform;
}

//----------------------------------------------------------------------------

vtkIterativeClosestPointTransform::vtkIterativeClosestPointTransform()
  : vtkLinearTransform()
{
  this->Source = NULL;
  this->Target = NULL;
  this->Locator = NULL;
  this->LandmarkTransform = vtkLandmarkTransform::New();
  this->MaximumNumberOfIterations = 50;
  this->CheckMeanDistance = 0;
  this->MaximumMeanDistance = 0.01;
  this->MaximumNumberOfLandmarks = 200;
  this->StartByMatchingCentroids = 0;

  this->NumberOfIterations = 0;
  this->MeanDistance = 0.0;
}

//----------------------------------------------------------------------------

vtkIterativeClosestPointTransform::~vtkIterativeClosestPointTransform()
{
  ReleaseSource();
  ReleaseTarget();
  ReleaseLocator();
  this->LandmarkTransform->Delete();
}

//----------------------------------------------------------------------------

void vtkIterativeClosestPointTransform::SetSource(vtkDataSet *source)
{
  if (this->Source == source)
    {
    return;
    }

  if (this->Source)
    {
    this->ReleaseSource();
    }

  if (source)
    {
    source->Register(this);
    }

  this->Source = source;
  this->Modified();
}

//----------------------------------------------------------------------------

void vtkIterativeClosestPointTransform::ReleaseSource(void) {
  if (this->Source) 
    {
    this->Source->UnRegister(this);
    this->Source = NULL;
    }
}

//----------------------------------------------------------------------------

void vtkIterativeClosestPointTransform::SetTarget(vtkDataSet *target)
{
  if (this->Target == target)
    {
    return;
    }

  if (this->Target)
    {
    this->ReleaseTarget();
    }

  if (target)
    {
    target->Register(this);
    }

  this->Target = target;
  this->Modified();
}

//----------------------------------------------------------------------------

void vtkIterativeClosestPointTransform::ReleaseTarget(void) {
  if (this->Target) 
    {
    this->Target->UnRegister(this);
    this->Target = NULL;
    }
}

//----------------------------------------------------------------------------

void vtkIterativeClosestPointTransform::SetLocator(vtkCellLocator *locator)
{
  if (this->Locator == locator)
    {
    return;
    }

  if (this->Locator)
    {
    this->ReleaseLocator();
    }

  if (locator)
    {
    locator->Register(this);
    }

  this->Locator = locator;
  this->Modified();
}

//----------------------------------------------------------------------------

void vtkIterativeClosestPointTransform::ReleaseLocator(void) {
  if (this->Locator) 
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
}

//----------------------------------------------------------------------------

void vtkIterativeClosestPointTransform::CreateDefaultLocator() {
  if (this->Locator) 
    {
    this->ReleaseLocator();
    }

  this->Locator = vtkCellLocator::New();
}

//------------------------------------------------------------------------

unsigned long vtkIterativeClosestPointTransform::GetMTime()
{
  unsigned long result = this->vtkLinearTransform::GetMTime();
  unsigned long mtime;

  if (this->Source)
    {
    mtime = this->Source->GetMTime(); 
    if (mtime > result)
      {
      result = mtime;
      }
    }

  if (this->Target)
    {
    mtime = this->Target->GetMTime(); 
    if (mtime > result)
      {
      result = mtime;
      }
    }

  if (this->Locator)
    {
    mtime = this->Locator->GetMTime(); 
    if (mtime > result)
      {
      result = mtime;
      }
    }

  return result;
}

//----------------------------------------------------------------------------

void vtkIterativeClosestPointTransform::Inverse()
{
  vtkDataSet *tmp1 = this->Source;
  this->Source = this->Target;
  this->Target = tmp1;
  this->Modified();
}

//----------------------------------------------------------------------------

vtkAbstractTransform *vtkIterativeClosestPointTransform::MakeTransform()
{
  return vtkIterativeClosestPointTransform::New(); 
}

//----------------------------------------------------------------------------

void vtkIterativeClosestPointTransform::InternalDeepCopy(vtkAbstractTransform *transform)
{
  vtkIterativeClosestPointTransform *t = (vtkIterativeClosestPointTransform *)transform;

  this->SetSource(t->GetSource());
  this->SetTarget(t->GetTarget());
  this->SetLocator(t->GetLocator());
  this->SetMaximumNumberOfIterations(t->GetMaximumNumberOfIterations());
  this->SetCheckMeanDistance(t->GetCheckMeanDistance());
  this->SetMaximumMeanDistance(t->GetMaximumMeanDistance());
  this->SetMaximumNumberOfLandmarks(t->GetMaximumNumberOfLandmarks());

  this->Modified();
}

//----------------------------------------------------------------------------

void vtkIterativeClosestPointTransform::InternalUpdate()
{
  // Check source, target

  if (this->Source == NULL || !this->Source->GetNumberOfPoints())
    {
    vtkErrorMacro(<<"Can't execute with NULL or empty input");
    return;
    }

  if (this->Target == NULL || !this->Target->GetNumberOfPoints())
    {
    vtkErrorMacro(<<"Can't execute with NULL or empty target");
    return;
    }

  // Create locator

  this->CreateDefaultLocator();
  this->Locator->SetDataSet(this->Target);
  this->Locator->SetNumberOfCellsPerBucket(1);
  this->Locator->BuildLocator();

  // Create two sets of points to handle iteration

  int step = 1;
  if (this->Source->GetNumberOfPoints() > this->MaximumNumberOfLandmarks)
    {
    step = this->Source->GetNumberOfPoints() / this->MaximumNumberOfLandmarks;
    vtkDebugMacro(<< "Landmarks step is now : " << step);
    }

  int nb_points = this->Source->GetNumberOfPoints() / step;

  // Allocate some points.
  // - closestp is used so that the internal state of LandmarkTransform remains
  //   correct whenever the iteration process is stopped (hence its source
  //   and landmark points might be used in a vtkThinPlateSplineTransform).
  // - points2 could have been avoided, but do not ask me why 
  //   InternalTransformPoint is not working correctly on my computer when
  //   in and out are the same pointer.

  vtkPoints *points1 = vtkPoints::New();
  points1->SetNumberOfPoints(nb_points);

  vtkPoints *closestp = vtkPoints::New();
  closestp->SetNumberOfPoints(nb_points);

  vtkPoints *points2 = vtkPoints::New();
  points2->SetNumberOfPoints(nb_points);

  // Fill with initial positions (sample dataset using step)

  vtkTransform *accumulate = vtkTransform::New();
  accumulate->PostMultiply();

  int i, j;
  float *p1, *p2;

  if (StartByMatchingCentroids)
    {
    float source_centroid[3] = {0,0,0};
    for (i = 0; i < this->Source->GetNumberOfPoints(); i++)
      {
      p1 = this->Source->GetPoint(i);
      source_centroid[0] += p1[0];
      source_centroid[1] += p1[1];
      source_centroid[2] += p1[2];
      }
    source_centroid[0] /= this->Source->GetNumberOfPoints();
    source_centroid[1] /= this->Source->GetNumberOfPoints();
    source_centroid[2] /= this->Source->GetNumberOfPoints();

    float target_centroid[3] = {0,0,0};
    for (i = 0; i < this->Target->GetNumberOfPoints(); i++)
      {
      p2 = this->Target->GetPoint(i);
      target_centroid[0] += p2[0];
      target_centroid[1] += p2[1];
      target_centroid[2] += p2[2];
      }
    target_centroid[0] /= this->Target->GetNumberOfPoints();
    target_centroid[1] /= this->Target->GetNumberOfPoints();
    target_centroid[2] /= this->Target->GetNumberOfPoints();

    accumulate->Translate(target_centroid[0] - source_centroid[0],
                          target_centroid[1] - source_centroid[1],
                          target_centroid[2] - source_centroid[2]);
    accumulate->Update();

    for (i = 0, j = 0; i < nb_points; i++, j += step)
      {
      accumulate->InternalTransformPoint(this->Source->GetPoint(j), 
                                         points1->GetPoint(i));
      }
    }
  else 
    {
    for (i = 0, j = 0; i < nb_points; i++, j += step)
      {
      points1->SetPoint(i, this->Source->GetPoint(j));
      }
    }

  // Go
  
  int cell_id,sub_id;
  float dist2, totaldist2;

  vtkPoints *temp, *a = points1, *b = points2;

  this->NumberOfIterations = 0;

  do 
    {
    // Fill points with the closest points to each vertex in input

    for(i = 0; i < nb_points; i++)
      {
      this->Locator->FindClosestPoint(a->GetPoint(i),
                                      closestp->GetPoint(i),
                                      cell_id,
                                      sub_id,
                                      dist2);
      }
    
    // Build the landmark transform

    this->LandmarkTransform->SetSourceLandmarks(a);
    this->LandmarkTransform->SetTargetLandmarks(closestp);
    this->LandmarkTransform->Update();

    // Concatenate (can't use this->Concatenate directly)
    
    accumulate->Concatenate(this->LandmarkTransform->GetMatrix());
  
    this->NumberOfIterations++;
    vtkDebugMacro(<< "Iteration: " << this->NumberOfIterations);
    if (this->NumberOfIterations >= this->MaximumNumberOfIterations) 
      {
      break;
      }

    // Move mesh and compute mean distance if needed

    if (this->CheckMeanDistance)
      {
      totaldist2 = 0.0;
      }

    for(i = 0; i < nb_points; i++)
      {
      p1 = a->GetPoint(i);
      p2 = b->GetPoint(i);
      this->LandmarkTransform->InternalTransformPoint(p1, p2);
      if (this->CheckMeanDistance)
        {
        totaldist2 += vtkMath::Distance2BetweenPoints(p1, p2);
        }
      }

    if (this->CheckMeanDistance)
      {
      this->MeanDistance = sqrt(totaldist2 / (float)nb_points);
      vtkDebugMacro("Mean distance: " << this->MeanDistance);
      if (this->MeanDistance <= this->MaximumMeanDistance)
        {
        break;
        }
      }

    temp = a;
    a = b;
    b = temp;

    } 
  while (1);

  // Now recover accumulated result

  this->Matrix->DeepCopy(accumulate->GetMatrix());

  accumulate->Delete();
  points1->Delete();
  closestp->Delete();
  points2->Delete();
}

//----------------------------------------------------------------------------

void vtkIterativeClosestPointTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkLinearTransform::PrintSelf(os,indent);

  if ( this->Source ) 
    {
    os << indent << "Source: " << this->Source << "\n";
    }
  else 
    {
    os << indent << "Source: (none)\n";
    }

  if ( this->Target ) 
    {
    os << indent << "Target: " << this->Target << "\n";
    }
  else 
    {
    os << indent << "Target: (none)\n";
    }

  if ( this->Locator ) 
    {
    os << indent << "Locator: " << this->Locator << "\n";
    }
  else 
    {
    os << indent << "Locator: (none)\n";
    }

  os << indent << "MaximumNumberOfIterations: " << this->MaximumNumberOfIterations << "\n";
  os << indent << "CheckMeanDistance: " << this->CheckMeanDistance << "\n";
  os << indent << "MaximumMeanDistance: " << this->MaximumMeanDistance << "\n";
  os << indent << "MaximumNumberOfLandmarks: " << this->MaximumNumberOfLandmarks << "\n";
  os << indent << "StartByMatchingCentroids: " << this->StartByMatchingCentroids << "\n";
  os << indent << "NumberOfIterations: " << this->NumberOfIterations << "\n";
  os << indent << "MeanDistance: " << this->MeanDistance << "\n";
  if(this->LandmarkTransform)
    {
    os << indent << "LandmarkTransform:\n";
    this->LandmarkTransform->PrintSelf(os, indent.GetNextIndent());
    }
}
