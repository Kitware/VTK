/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkStreaklineFilter.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStreaklineFilter.h"
#include "vtkObjectFactory.h"
#include "vtkSetGet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkPointData.h"
#include "vtkIntArray.h"
#include "vtkSmartPointer.h"
#include "vtkNew.h"
#include "vtkFloatArray.h"

#include <vector>
#include <cassert>
#include <algorithm>

#define DEBUGSTREAKLINEFILTER 1
#ifdef DEBUGSTREAKLINEFILTER
#define Assert(a) assert(a)
#else
#define Assert(a)
#endif

class StreakParticle
{
public:
  vtkIdType Id;
  float Age;

  StreakParticle(vtkIdType id, float age)
    :Id(id),Age(age)
  {
  }

  bool operator<(const StreakParticle& other) const
  {
    return this->Age > other.Age;
  }
};

typedef std::vector<StreakParticle> Streak;

vtkObjectFactoryNewMacro(vtkStreaklineFilter)


void StreaklineFilterInternal::Initialize(vtkParticleTracerBase* filter)
{
  this->Filter = filter;
  this->Filter->ForceReinjectionEveryNSteps = 1;
  this->Filter->IgnorePipelineTime = 1;
}

int StreaklineFilterInternal::OutputParticles(vtkPolyData* particles)
{
  this->Filter->Output = particles;

  return 1;
}

void StreaklineFilterInternal::Finalize()
{
  vtkPoints* points = this->Filter->Output->GetPoints();
  if(!points)
    {
    return;
    }

  vtkPointData* pd = this->Filter->Output->GetPointData();
  Assert(pd);
  vtkFloatArray* particleAge = vtkFloatArray::SafeDownCast(pd->GetArray("ParticleAge"));
  Assert(particleAge);
  vtkIntArray* seedIds = vtkIntArray::SafeDownCast(pd->GetArray("InjectedPointId"));
  Assert(seedIds);

  if(seedIds)
    {
    std::vector<Streak> streaks; //the streak lines in the current time step
    for(vtkIdType i=0; i<points->GetNumberOfPoints(); i++)
      {
      int streakId = seedIds->GetValue(i);
      for(int j=static_cast<int>(streaks.size()); j<=streakId; j++)
        {
        streaks.push_back(Streak());
        }
      Streak& streak = streaks[streakId];
      float age = particleAge->GetValue(i);
      streak.push_back(StreakParticle(i,age));
      }

    //sort streaks based on age
    for(unsigned int i=0; i<streaks.size();i++)
      {
      Streak& streak(streaks[i]);
      std::sort(streak.begin(),streak.end());
      }

    this->Filter->Output->SetLines(vtkSmartPointer<vtkCellArray>::New());
    this->Filter->Output->SetVerts(0);
    vtkCellArray* outLines = this->Filter->Output->GetLines();
    Assert(outLines->GetNumberOfCells()==0);
    Assert(outLines);
    for(unsigned int i=0; i<streaks.size();i++)
      {
      const Streak& streak(streaks[i]);
      vtkNew<vtkIdList> ids;

      for(unsigned int j=0; j<streak.size();j++)
        {
        Assert(j==0 || streak[j].Age <= streak[j-1].Age);
        if(j==0 || streak[j].Age < streak[j-1].Age)
          {
          ids->InsertNextId(streak[j].Id);
          }
        }
      if(ids->GetNumberOfIds()>1)
        {
        outLines->InsertNextCell(ids.GetPointer());
        }
      }
    }
}


vtkStreaklineFilter::vtkStreaklineFilter()
{
  this->It.Initialize(this);
}

int vtkStreaklineFilter::OutputParticles(vtkPolyData* particles)
{
  return this->It.OutputParticles(particles);
}

void vtkStreaklineFilter::Finalize()
{
  return this->It.Finalize();
}

void vtkStreaklineFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}
