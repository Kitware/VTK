/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProcrustesAlignmentFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Tim Hutton who developed and contributed this class

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProcrustesAlignmentFilter.h"
#include "vtkObjectFactory.h"
#include "vtkLandmarkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkPolyData.h"

vtkCxxRevisionMacro(vtkProcrustesAlignmentFilter, "1.1");
vtkStandardNewMacro(vtkProcrustesAlignmentFilter);

//----------------------------------------------------------------------------
// protected
vtkProcrustesAlignmentFilter::vtkProcrustesAlignmentFilter()
{
  this->LandmarkTransform = vtkLandmarkTransform::New();
}

//----------------------------------------------------------------------------
// protected
vtkProcrustesAlignmentFilter::~vtkProcrustesAlignmentFilter()
{
  if(this->LandmarkTransform)
    {
    this->LandmarkTransform->Delete();
    }
}

//----------------------------------------------------------------------------
// protected
void vtkProcrustesAlignmentFilter::Execute()
{
  vtkDebugMacro(<<"Execute()");

  if(!this->vtkProcessObject::Inputs)
    {
    vtkErrorMacro(<<"No input!");
    return;
    }

  int i,v;

  const int N_SETS = this->vtkProcessObject::GetNumberOfInputs();

  // copy the inputs across
  // (really actually only the points need to be deep copied since the rest stays the same)
  for(i=0;i<N_SETS;i++)
    {
    this->GetOutput(i)->DeepCopy(this->GetInput(i));
    }

  // the number of points is determined by the first input (they must all be the same)
  const int N_POINTS = this->GetInput(0)->GetNumberOfPoints();

  vtkDebugMacro(<<"N_POINTS is " <<N_POINTS);

  // all the inputs must have the same number of points to consider executing
  for(i=1;i<N_SETS;i++) 
    {
    if(this->GetInput(i)->GetNumberOfPoints() != N_POINTS)
      {
      vtkErrorMacro(<<"The inputs have different numbers of points!");
      return;
      }
    }

  vtkPoints *mean_points = vtkPoints::New();
  mean_points->DeepCopy(this->GetInput(0)->GetPoints());
  // we could have just called SetNumberOfPoints here but this way we get a meaningful
  // estimate of the initial difference in the means.

  float point[3],*p;

  this->LandmarkTransform->SetTargetLandmarks(mean_points);

  // compute mean and align all the shapes to it, until convergence
  BOOL converged=false;
  int iterations=0;
  const int MAX_ITERATIONS=5;
  float difference; 
  do { 

      // compute the average of each point across all the shapes
      difference=0.0F;
      for(v=0;v<N_POINTS;v++)
        {
        point[0]=0.0F;
        point[1]=0.0F;
        point[2]=0.0F;
        for(i=0;i<N_SETS;i++)
          {
          p = this->GetOutput(i)->GetPoint(v);
          point[0]+=p[0];
          point[1]+=p[1];
          point[2]+=p[2];
          }
        point[0]/=(float)N_SETS;
        point[1]/=(float)N_SETS;
        point[2]/=(float)N_SETS;
        p = mean_points->GetPoint(v);
        difference += point[0]-p[0];
        difference += point[1]-p[1];
        difference += point[2]-p[2];
        p[0]=point[0];
        p[1]=point[1];
        p[2]=point[2];
        }

      vtkDebugMacro( << "Difference after " << (iterations+1) << " iteration(s) is: " << difference);

      // align each pointset with the mean
      for(i=0;i<N_SETS;i++) {
        this->LandmarkTransform->SetSourceLandmarks(this->GetOutput(i)->GetPoints());
        this->LandmarkTransform->Update();
        for(v=0;v<N_POINTS;v++) {
          this->LandmarkTransform->InternalTransformPoint(
              this->GetOutput(i)->GetPoint(v),
              this->GetOutput(i)->GetPoint(v));
          }
        } 

      // test for convergence
      iterations++;
      if(fabs(difference)<1e-4 || iterations>=MAX_ITERATIONS) {
        converged=true;
        }

      // The convergence test is a simple sum of differences of changing mean.
      // Procrustes shouldn't need more than 2 iterations but with large input sets
      // you can get significant numerical oscillation, so we impose a limit for safety.

  } while(!converged);  

  if(iterations>=MAX_ITERATIONS) {
    vtkDebugMacro( << "Procrustes did not converge in  " << MAX_ITERATIONS << " iterations! Objects may not be aligned. Difference = " <<
      difference);
    // we don't throw an Error here since the shapes most probably *are* aligned, but the 
    // numerical precision is worse than our convergence test anticipated.
    }
  else {
    vtkDebugMacro( << "Procrustes required " << iterations << " iterations to converge to " <<
      difference);
    }

  // clean up
  mean_points->Delete();
}

//----------------------------------------------------------------------------
// public
void vtkProcrustesAlignmentFilter::SetNumberOfInputs(int n)
{ 
  this->vtkProcessObject::SetNumberOfInputs(n);
  this->vtkSource::SetNumberOfOutputs(n);

  // initialise the outputs
  for(int i=0;i<n;i++)
    {
    vtkPoints *points = vtkPoints::New();
    vtkPolyData *ps = vtkPolyData::New();
    ps->SetPoints(points);
    points->Delete();
    this->vtkSource::SetNthOutput(i,ps);
    ps->Delete();
    }

  // is this the right thing to be doing here? if we don't initialise the outputs here
  // then the filter crashes but vtkPolyData may not be the type of the inputs
}

//----------------------------------------------------------------------------
// public
void vtkProcrustesAlignmentFilter::SetInput(int idx,vtkPointSet* p) 
{ 
  this->vtkProcessObject::SetNthInput(idx,p);
  // (let vtkProcessObject deal with out of range issues)
}

//----------------------------------------------------------------------------
// protected
vtkPointSet* vtkProcrustesAlignmentFilter::GetInput(int idx) 
{
  if(idx<0 || idx>=this->vtkProcessObject::GetNumberOfInputs())
    {
    vtkErrorMacro(<<"Index out of bounds in GetInput!");
    return NULL;
    }
  
  return static_cast<vtkPointSet*>(this->vtkProcessObject::Inputs[idx]);
}

//----------------------------------------------------------------------------
// public
vtkPointSet* vtkProcrustesAlignmentFilter::GetOutput(int idx) 
{ 
  return static_cast<vtkPointSet*>(this->vtkSource::GetOutput(idx));
  // (let vtkSource deal with out of range issues)
}

//----------------------------------------------------------------------------
// public
void vtkProcrustesAlignmentFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  this->LandmarkTransform->PrintSelf(os,indent);
}

