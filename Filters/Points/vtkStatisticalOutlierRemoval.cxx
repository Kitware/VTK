/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStatisticalOutlierRemoval.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStatisticalOutlierRemoval.h"

#include "vtkObjectFactory.h"
#include "vtkAbstractPointLocator.h"
#include "vtkStaticPointLocator.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkIdList.h"
#include "vtkSMPTools.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkStatisticalOutlierRemoval);
vtkCxxSetObjectMacro(vtkStatisticalOutlierRemoval,Locator,vtkAbstractPointLocator);

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace {

//----------------------------------------------------------------------------
// The threaded core of the algorithm (first pass)
template <typename T>
struct ComputeMeanDistance
{
  const T *Points;
  vtkAbstractPointLocator *Locator;
  int SampleSize;
  float *Distance;
  double Mean;

  // Don't want to allocate working arrays on every thread invocation. Thread local
  // storage lots of new/delete.
  vtkSMPThreadLocalObject<vtkIdList> PIds;
  vtkSMPThreadLocal<double> ThreadMean;
  vtkSMPThreadLocal<vtkIdType> ThreadCount;

  ComputeMeanDistance(T *points, vtkAbstractPointLocator *loc, int size, float *d) :
    Points(points), Locator(loc), SampleSize(size), Distance(d), Mean(0.0)
  {
  }

  // Just allocate a little bit of memory to get started.
  void Initialize()
  {
    vtkIdList*& pIds = this->PIds.Local();
    pIds->Allocate(128); //allocate some memory

    double &threadMean = this->ThreadMean.Local();
    threadMean = 0.0;

    vtkIdType &threadCount = this->ThreadCount.Local();
    threadCount = 0;
  }

  // Compute average distance for each point, plus accumlate summation of
  // mean distances and count (for averaging in the Reduce() method).
  void operator() (vtkIdType ptId, vtkIdType endPtId)
  {
      const T *px = this->Points + 3*ptId;
      const T *py;
      double x[3], y[3];
      vtkIdList*& pIds = this->PIds.Local();
      double &threadMean = this->ThreadMean.Local();
      vtkIdType &threadCount = this->ThreadCount.Local();

      for ( ; ptId < endPtId; ++ptId)
      {
        x[0] = static_cast<double>(*px++);
        x[1] = static_cast<double>(*px++);
        x[2] = static_cast<double>(*px++);

        // The method FindClosestNPoints will include the current point, so
        // we increase the sample size by one.
        this->Locator->FindClosestNPoints(this->SampleSize+1, x, pIds);
        vtkIdType numPts = pIds->GetNumberOfIds();

        double sum = 0.0;
        vtkIdType nei;
        for (int sample=0; sample < numPts; ++sample)
        {
          nei = pIds->GetId(sample);
          if ( nei != ptId ) //exclude ourselves
          {
            py = this->Points + 3*nei;
            y[0] = static_cast<double>(*py++);
            y[1] = static_cast<double>(*py++);
            y[2] = static_cast<double>(*py);
            sum += sqrt( vtkMath::Distance2BetweenPoints(x,y) );
          }
        }//sum the lengths of all samples exclusing current point

        // Average the lengths; again exclude ourselves
        if ( numPts > 0 )
        {
          this->Distance[ptId] = sum / static_cast<double>(numPts-1);
          threadMean += this->Distance[ptId];
          threadCount++;
        }
        else //ignore if no points are found, something bad has happened
        {
          this->Distance[ptId] = VTK_FLOAT_MAX; //the effect is to eliminate it
        }
      }
  }

  // Compute the mean by compositing all threads
  void Reduce()
  {
      double mean=0.0;
      vtkIdType count=0;

      vtkSMPThreadLocal<double>::iterator mItr;
      vtkSMPThreadLocal<double>::iterator mEnd = this->ThreadMean.end();
      for ( mItr=this->ThreadMean.begin(); mItr != mEnd; ++mItr )
      {
        mean += *mItr;
      }

      vtkSMPThreadLocal<vtkIdType>::iterator cItr;
      vtkSMPThreadLocal<vtkIdType>::iterator cEnd = this->ThreadCount.end();
      for ( cItr=this->ThreadCount.begin(); cItr != cEnd; ++cItr )
      {
        count += *cItr;
      }

      count = ( count < 1 ? 1 : count);
      this->Mean = mean / static_cast<double>(count);
  }

  static void Execute(vtkStatisticalOutlierRemoval *self, vtkIdType numPts,
                      T *points, float *distances, double& mean)
  {
      ComputeMeanDistance compute(points, self->GetLocator(),
                                  self->GetSampleSize(), distances);
      vtkSMPTools::For(0, numPts, compute);
      mean = compute.Mean;
  }

}; //ComputeMeanDistance

//----------------------------------------------------------------------------
// Now that the mean is known, compute the standard deviation
struct ComputeStdDev
{
  float *Distances;
  double Mean;
  double StdDev;
  vtkSMPThreadLocal<double> ThreadSigma;
  vtkSMPThreadLocal<vtkIdType> ThreadCount;

  ComputeStdDev(float *d, double mean) : Distances(d), Mean(mean), StdDev(0.0)
  {
  }

  void Initialize()
  {
    double &threadSigma = this->ThreadSigma.Local();
    threadSigma = 0.0;

    vtkIdType &threadCount = this->ThreadCount.Local();
    threadCount = 0;
  }

  void operator() (vtkIdType ptId, vtkIdType endPtId)
  {
      double &threadSigma = this->ThreadSigma.Local();
      vtkIdType &threadCount = this->ThreadCount.Local();
      float d;

      for ( ; ptId < endPtId; ++ptId)
      {
        d = this->Distances[ptId];
        if ( d < VTK_FLOAT_MAX )
        {
          threadSigma += (this->Mean - d) * (this->Mean - d);
          threadCount++;
        }
        else
        {
          continue; //skip bad point
        }
      }
  }

  void Reduce()
  {
      double sigma=0.0;
      vtkIdType count=0;

      vtkSMPThreadLocal<double>::iterator sItr;
      vtkSMPThreadLocal<double>::iterator sEnd = this->ThreadSigma.end();
      for ( sItr=this->ThreadSigma.begin(); sItr != sEnd; ++sItr )
      {
        sigma += *sItr;
      }

      vtkSMPThreadLocal<vtkIdType>::iterator cItr;
      vtkSMPThreadLocal<vtkIdType>::iterator cEnd = this->ThreadCount.end();
      for ( cItr=this->ThreadCount.begin(); cItr != cEnd; ++cItr )
      {
        count += *cItr;
      }

      this->StdDev = sqrt( sigma / static_cast<double>(count) );
  }

  static void Execute(vtkIdType numPts, float *distances,
                      double mean, double& sigma)
  {
      ComputeStdDev stdDev(distances, mean);
      vtkSMPTools::For(0, numPts, stdDev);
      sigma = stdDev.StdDev;
  }

}; //ComputeStdDev

//----------------------------------------------------------------------------
// Statistics are computed, now filter the points
struct RemoveOutliers
{
  double Mean;
  double Sigma;
  float *Distances;
  vtkIdType *PointMap;

  RemoveOutliers(double mean, double sigma, float *distances, vtkIdType *map) :
    Mean(mean), Sigma(sigma), Distances(distances), PointMap(map)
  {
  }

  void operator() (vtkIdType ptId, vtkIdType endPtId)
  {
      vtkIdType *map = this->PointMap + ptId;
      float *d = this->Distances + ptId;
      double mean=this->Mean, sigma=this->Sigma;

      for ( ; ptId < endPtId; ++ptId)
      {
        *map++ = ( fabs(*d++ - mean) <= sigma ? 1 : -1 );
      }
  }

  static void Execute(vtkIdType numPts, float *distances, double mean,
                      double sigma, vtkIdType *map)
  {
      RemoveOutliers remove(mean, sigma, distances, map);
      vtkSMPTools::For(0, numPts, remove);
  }

}; //RemoveOutliers


} //anonymous namespace


//================= Begin class proper =======================================
//----------------------------------------------------------------------------
vtkStatisticalOutlierRemoval::vtkStatisticalOutlierRemoval()
{
  this->SampleSize = 25;
  this->StandardDeviationFactor = 1.0;
  this->Locator = vtkStaticPointLocator::New();

  this->ComputedMean = 0.0;
  this->ComputedStandardDeviation = 0.0;

}

//----------------------------------------------------------------------------
vtkStatisticalOutlierRemoval::~vtkStatisticalOutlierRemoval()
{
  this->SetLocator(NULL);
}

//----------------------------------------------------------------------------
// Traverse all the input points and gather statistics about average distance
// between them, and the standard deviation of variation. Then filter points
// within a specified deviation from the mean.
int vtkStatisticalOutlierRemoval::FilterPoints(vtkPointSet *input)
{
  // Perform the point removal
  // Start by building the locator
  if ( !this->Locator )
  {
    vtkErrorMacro(<<"Point locator required\n");
    return 0;
  }
  this->Locator->SetDataSet(input);
  this->Locator->BuildLocator();

  // Compute statistics across the point cloud. Start my computing
  // mean distance to N closest neighbors.
  vtkIdType numPts = input->GetNumberOfPoints();
  float *dist = new float [numPts];
  void *inPtr = input->GetPoints()->GetVoidPointer(0);
  double mean=0.0, sigma=0.0;
  switch (input->GetPoints()->GetDataType())
  {
    vtkTemplateMacro(ComputeMeanDistance<VTK_TT>::
                     Execute(this, numPts, (VTK_TT *)inPtr, dist, mean));
  }

  // At this point the mean distance for each point, and across the point
  // cloud is known. Now compute global standard deviation.
  ComputeStdDev::Execute(numPts, dist, mean, sigma);

  // Finally filter the points based on specified deviation range.
  RemoveOutliers::Execute(numPts, dist, mean,
                          this->StandardDeviationFactor*sigma, this->PointMap);

  // Assign derived variable
  this->ComputedMean = mean;
  this->ComputedStandardDeviation = sigma;

  // Clean up
  delete [] dist;

  return 1;
}

//----------------------------------------------------------------------------
void vtkStatisticalOutlierRemoval::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Sample Size: " << this->SampleSize << "\n";
  os << indent << "Standard Deviation Factor: "
     << this->StandardDeviationFactor << "\n";
  os << indent << "Locator: " << this->Locator << "\n";

  os << indent << "Computed Mean: " << this->ComputedMean << "\n";
  os << indent << "Computed Standard Deviation: "
     << this->ComputedStandardDeviation << "\n";
}
