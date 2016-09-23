/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParticlePathFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkParticlePathFilter.h"

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSetGet.h"
#include "vtkSmartPointer.h"

#include <vector>

vtkObjectFactoryNewMacro(vtkParticlePathFilter)

void ParticlePathFilterInternal::Initialize(vtkParticleTracerBase* filter)
{
  this->Filter = filter;
  this->Filter->SetForceReinjectionEveryNSteps(0);
  this->Filter->SetIgnorePipelineTime(1);
  this->ClearCache = false;
}

void ParticlePathFilterInternal::Reset()
{
  this->Filter->vtkParticleTracerBase::ResetCache();
  this->Paths.clear();
}

int ParticlePathFilterInternal::OutputParticles(vtkPolyData* particles)
{
  if(!this->Filter->Output || this->ClearCache)
  {
    this->Filter->Output = vtkSmartPointer<vtkPolyData>::New();
    this->Filter->Output->SetPoints(vtkSmartPointer<vtkPoints>::New());
    this->Filter->Output->GetPointData()->CopyAllocate(particles->GetPointData());
  }
  if(this->ClearCache)
  { // clear cache no matter what
    this->Paths.clear();
  }

  vtkPoints* pts = particles->GetPoints();
  if(!pts || pts->GetNumberOfPoints()==0)
  {
    return 0;
  }

  vtkPointData* outPd = this->Filter->Output->GetPointData();
  vtkPoints* outPoints = this->Filter->Output->GetPoints();

  //Get the input arrays
  vtkPointData* pd = particles->GetPointData();
  vtkIntArray* particleIds = vtkArrayDownCast<vtkIntArray>(pd->GetArray("ParticleId"));

  //Append the input arrays to the output arrays
  int begin = outPoints->GetNumberOfPoints();
  for(int i=0; i<pts->GetNumberOfPoints(); i++)
  {
    outPoints->InsertNextPoint(pts->GetPoint(i));
  }
  vtkDataSetAttributes::FieldList ptList(1);
  ptList.InitializeFieldList(pd);
  for(int i=0, j = begin; i<pts->GetNumberOfPoints(); i++,j++)
  {
    outPd->CopyData(ptList,pd,0,i,j);
  }

  //Augment the paths
  for(vtkIdType i=0; i<pts->GetNumberOfPoints(); i++)
  {
    int outId =  i+begin;

    int pid = particleIds->GetValue(i);
    for(int j= static_cast<int>(this->Paths.size()); j<=pid; j++)
    {
      this->Paths.push_back( vtkSmartPointer<vtkIdList>::New());
    }

    vtkIdList* path = this->Paths[pid];

#ifdef DEBUG
    if(path->GetNumberOfIds()>0)
    {
      vtkFloatArray* outParticleAge = vtkArrayDownCast<vtkFloatArray>(outPd->GetArray("ParticleAge"));
      if(outParticleAge->GetValue(outId) < outParticleAge->GetValue(path->GetId(path->GetNumberOfIds()-1)))
      {
        vtkOStrStreamWrapper vtkmsg;
        vtkmsg << "ERROR: In " __FILE__ ", line " << __LINE__
               << "\n" << "): " <<" new particles have wrong ages"<< "\n\n";
      }
    }
#endif
    path->InsertNextId(outId);
  }

  return 1;
}
void ParticlePathFilterInternal::Finalize()
{
  this->Filter->Output->SetLines(vtkSmartPointer<vtkCellArray>::New());
  vtkCellArray* outLines = this->Filter->Output->GetLines();
  if(!outLines)
  {
    vtkOStrStreamWrapper vtkmsg;
    vtkmsg << "ERROR: In " __FILE__ ", line " << __LINE__
           << "\n" << "): " <<" no lines in the output"<< "\n\n";
    return;
  }
  for(unsigned int i=0; i<this->Paths.size(); i++)
  {
    if(this->Paths[i]->GetNumberOfIds()>1)
    {
      outLines->InsertNextCell(this->Paths[i]);
    }
  }
}

vtkParticlePathFilter::vtkParticlePathFilter()
{
  this->It.Initialize(this);
  this->SimulationTime = NULL;
  this->SimulationTimeStep = NULL;
}

vtkParticlePathFilter::~vtkParticlePathFilter()
{
  if(this->SimulationTime)
  {
    this->SimulationTime->Delete();
    this->SimulationTime = NULL;
  }
  if(this->SimulationTimeStep)
  {
    this->SimulationTimeStep->Delete();
    this->SimulationTimeStep = NULL;
  }
}

void vtkParticlePathFilter::ResetCache()
{
  Superclass::ResetCache();
  this->It.Reset();
}

void vtkParticlePathFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}

int vtkParticlePathFilter::OutputParticles(vtkPolyData* particles)
{
  return this->It.OutputParticles(particles);
}

void vtkParticlePathFilter::InitializeExtraPointDataArrays(vtkPointData* outputPD)
{
  if(this->SimulationTime == NULL)
  {
    this->SimulationTime = vtkDoubleArray::New();
    this->SimulationTime->SetName("SimulationTime");
  }
  if(outputPD->GetArray("SimulationTime"))
  {
    outputPD->RemoveArray("SimulationTime");
  }
  this->SimulationTime->SetNumberOfTuples(0);
  outputPD->AddArray(this->SimulationTime);

  if(this->SimulationTimeStep == NULL)
  {
    this->SimulationTimeStep = vtkIntArray::New();
    this->SimulationTimeStep->SetName("SimulationTimeStep");
  }
  if(outputPD->GetArray("SimulationTimeStep"))
  {
    outputPD->RemoveArray("SimulationTimeStep");
  }
  this->SimulationTimeStep->SetNumberOfTuples(0);
  outputPD->AddArray(this->SimulationTimeStep);
}

void vtkParticlePathFilter::AppendToExtraPointDataArrays(
  vtkParticleTracerBaseNamespace::ParticleInformation &info)
{
  this->SimulationTime->InsertNextValue(info.SimulationTime);
  this->SimulationTimeStep->InsertNextValue(info.InjectedStepId+info.TimeStepAge);
}

void vtkParticlePathFilter::Finalize()
{
  this->It.Finalize();
}
