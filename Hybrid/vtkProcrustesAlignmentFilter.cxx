/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProcrustesAlignmentFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
#include "vtkMath.h"

vtkCxxRevisionMacro(vtkProcrustesAlignmentFilter, "1.7");
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
// Calculate the centroid of a point cloud
void static inline Centroid(vtkPoints* pd, float *cp)
{
  // Center point
  cp[0] = 0; cp[1] = 0; cp[2] = 0;
  
  int np = pd->GetNumberOfPoints();
  
  // Calculate center of shape
  for (int i = 0; i < np; i++)
  {
    float *p = pd->GetPoint(i);
    cp[0] += p[0]; cp[1] += p[1]; cp[2] += p[2];
  }
  cp[0] /= np; cp[1] /= np; cp[2] /= np;
}

//----------------------------------------------------------------------------
// Calculate the centroid size of a point cloud
double static inline CentroidSize(vtkPoints* pd, float *cp)
{
  Centroid(pd, cp);
  
  double S = 0;
  for (int i = 0; i < pd->GetNumberOfPoints(); i++)
  {
    float *p = pd->GetPoint(i);
    
    S += sqrt(vtkMath::Distance2BetweenPoints(p,cp));
  }
  
  return S;
}

//----------------------------------------------------------------------------
// Translation of point cloud. Could be done using transformations
void static inline TranslateShape(vtkPoints* pd, float *tp)
{
  for (int i = 0; i < pd->GetNumberOfPoints(); i++)
  {
    float *p = pd->GetPoint(i);
    
    pd->SetPoint(i, p[0]+tp[0], p[1]+tp[1], p[2]+tp[2]);
  }
}

//----------------------------------------------------------------------------
// Scaling of point cloud. Could be done using transformations
void static inline ScaleShape(vtkPoints* pd, double S)
{
  for (int i = 0; i < pd->GetNumberOfPoints(); i++)
  {
    float *p = pd->GetPoint(i);
    
    pd->SetPoint(i, p[0]*S, p[1]*S, p[2]*S);
  }
}

//----------------------------------------------------------------------------
// Normalise a point cloud to have centroid (0,0,0) and centroid size 1
int static inline NormaliseShape(vtkPoints* pd)
{
  float cp[3];
  double S = CentroidSize(pd, cp);
  if (S == 0)
    return 0;
  
  float tp[3];
  tp[0] = -cp[0]; tp[1] = -cp[1]; tp[2] = -cp[2]; 
  TranslateShape(pd, tp);
  ScaleShape(pd, 1/S);
  return 1;
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

  if(N_POINTS == 0)
    {
    vtkErrorMacro(<<"No points!");
    return;
    }

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
  // our initial estimate of the mean comes from the first example in the set

  // we keep a record of the first mean to fix the orientation and scale
  // (which are otherwise undefined and the loop will not converge)
  vtkPoints *first_mean = vtkPoints::New();
  first_mean->DeepCopy(mean_points);


  // If the similarity transform is used, the mean shape must be normalised
  // to avoid shrinking
  if (this->LandmarkTransform->GetMode() == VTK_LANDMARK_SIMILARITY)
  {
    if (!NormaliseShape(mean_points)) {
      vtkErrorMacro(<<"Centroid size zero");
      return;
    }
    if (!NormaliseShape(first_mean)) {
      vtkErrorMacro(<<"Centroid size zero");
      return;
    }
  }
  

  // storage for the new mean that is being calculated
  vtkPoints *new_mean = vtkPoints::New();
  new_mean->SetNumberOfPoints(N_POINTS);

  // compute mean and align all the shapes to it, until convergence
  int converged=0; // bool converged=false
  int iterations=0;
  const int MAX_ITERATIONS=5;
  float difference; 
  float point[3],*p,*p2;
  do { // (while not converged)

    // align each pointset with the mean
    for(i=0;i<N_SETS;i++) {
      this->LandmarkTransform->SetSourceLandmarks(this->GetOutput(i)->GetPoints());
      this->LandmarkTransform->SetTargetLandmarks(mean_points);
      this->LandmarkTransform->Update();
      for(v=0;v<N_POINTS;v++) {
        this->LandmarkTransform->InternalTransformPoint(
            this->GetOutput(i)->GetPoint(v),
            this->GetOutput(i)->GetPoint(v));
        }
      } 

    // compute the new mean (just average the point locations)
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
      p = new_mean->GetPoint(v);
      p[0] = point[0]/(float)N_SETS;
      p[1] = point[1]/(float)N_SETS;
      p[2] = point[2]/(float)N_SETS;
      }

    // align the new mean with the fixed mean if the transform
    // is similarity or rigidbody. It is not yet decided what to do with affine
    if (this->LandmarkTransform->GetMode() == VTK_LANDMARK_SIMILARITY || 
        this->LandmarkTransform->GetMode() == VTK_LANDMARK_RIGIDBODY){
      this->LandmarkTransform->SetSourceLandmarks(new_mean);
      this->LandmarkTransform->SetTargetLandmarks(first_mean);
      this->LandmarkTransform->Update();
      for(v=0;v<N_POINTS;v++) {
        this->LandmarkTransform->InternalTransformPoint(
          new_mean->GetPoint(v),new_mean->GetPoint(v));
      }
    }

    // If the similarity transform is used, the mean shape must be normalised
    // to avoid shrinking
    if (this->LandmarkTransform->GetMode() == VTK_LANDMARK_SIMILARITY) {
      if (!NormaliseShape(new_mean)) {
        vtkErrorMacro(<<"Centroid size zero");
        return;
      }
    }


    // the new mean becomes our mean
    // compute the difference between the two
    difference = 0.0F;
    for(v=0;v<N_POINTS;v++)
      {
        p = new_mean->GetPoint(v);
        p2 = mean_points->GetPoint(v);
        difference += vtkMath::Distance2BetweenPoints(p,p2);
        p2[0] = p[0];
        p2[1] = p[1];
        p2[2] = p[2];
      }

    // test for convergence
    iterations++;
    vtkDebugMacro( << "Difference after " << iterations << " iteration(s) is: " << difference);
    if(difference<1e-6 || iterations>=MAX_ITERATIONS) {
      converged=1; // true
      }

    // The convergence test is that the sum of the distances between the
    // points on mean(t) and mean(t-1) is less than a very small number.
    // Procrustes shouldn't need more than 2 or 3 iterations but things could go wrong
    // so we impose an iteration limit to avoid getting stuck in an infinite loop.

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
  first_mean->Delete();
  new_mean->Delete();
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
  if(idx<0 || idx>=this->vtkProcessObject::GetNumberOfInputs())
    {
    vtkErrorMacro(<<"Index out of bounds in SetInput!");
    return;
    }
  
  this->vtkProcessObject::SetNthInput(idx,p);
}

//----------------------------------------------------------------------------
// public
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
  if(idx<0 || idx>=this->vtkSource::GetNumberOfOutputs())
    {
    vtkErrorMacro(<<"Index out of bounds in GetOutput!");
    return NULL;
    }
  
  return static_cast<vtkPointSet*>(this->vtkSource::GetOutput(idx));
}

//----------------------------------------------------------------------------
// public
void vtkProcrustesAlignmentFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  this->LandmarkTransform->PrintSelf(os,indent);
}

