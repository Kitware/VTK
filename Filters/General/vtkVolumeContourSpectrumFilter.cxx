/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVolumeContourSpectrumFilter.h"

#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkEdgeListIterator.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkReebGraph.h"
#include "vtkTable.h"
#include "vtkTetra.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVariantArray.h"

vtkStandardNewMacro(vtkVolumeContourSpectrumFilter);

//----------------------------------------------------------------------------
vtkVolumeContourSpectrumFilter::vtkVolumeContourSpectrumFilter()
{
  this->SetNumberOfInputPorts(2);
  this->ArcId = 0;
  this->FieldId = 0;
  this->NumberOfSamples = 100;
}

//----------------------------------------------------------------------------
vtkVolumeContourSpectrumFilter::~vtkVolumeContourSpectrumFilter()
{
}

//----------------------------------------------------------------------------
int vtkVolumeContourSpectrumFilter::FillInputPortInformation(
  int portNumber, vtkInformation *info)
{
  switch(portNumber)
    {
    case 0:
      info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
      info->Append(
        vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
      break;
    case 1:
      info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
      info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkReebGraph");
      break;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkVolumeContourSpectrumFilter::FillOutputPortInformation(int vtkNotUsed(portNumber), vtkInformation *info)
{

  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");

  return 1;
}

//----------------------------------------------------------------------------
void vtkVolumeContourSpectrumFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Arc Id: " << this->ArcId << "\n";
  os << indent << "Number of Samples: " << this->NumberOfSamples << "\n";
  os << indent << "Field Id: " << this->FieldId << "\n";
}

//----------------------------------------------------------------------------
vtkTable* vtkVolumeContourSpectrumFilter::GetOutput()
{
  return vtkTable::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
int vtkVolumeContourSpectrumFilter::RequestData(vtkInformation* vtkNotUsed(request), vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{

  vtkInformation  *inInfoMesh = inputVector[0]->GetInformationObject(0),
                  *inInfoGraph = inputVector[1]->GetInformationObject(0);

  if ((!inInfoMesh)||(!inInfoGraph))
    {
    return 0;
    }
  vtkUnstructuredGrid *inputMesh = vtkUnstructuredGrid::SafeDownCast(
    inInfoMesh->Get(vtkUnstructuredGrid::DATA_OBJECT()));

  vtkReebGraph *inputGraph = vtkReebGraph::SafeDownCast(
    inInfoGraph->Get(vtkReebGraph::DATA_OBJECT()));

  if ((inputMesh)&&(inputGraph))
    {
    vtkInformation  *outInfo = outputVector->GetInformationObject(0);
    vtkTable     *output = vtkTable::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));

    if(output)
      {

      // Retrieve the arc given by ArcId
      vtkVariantArray *edgeInfo = vtkVariantArray::SafeDownCast(
        inputGraph->GetEdgeData()->GetAbstractArray("Vertex Ids"));
      // Invalid Reeb graph (no information associated to the edges)
      if(!edgeInfo)
        return 0;

      // Retrieve the information to get the critical vertices Ids
      vtkDataArray *criticalPointIds = vtkDataArray::SafeDownCast(
        inputGraph->GetVertexData()->GetAbstractArray("Vertex Ids"));
      // Invalid Reeb graph (no information associated to the vertices)
      if(!criticalPointIds)
        return 0;

      vtkAbstractArray *vertexList = edgeInfo->GetPointer(ArcId)->ToArray();

      // the arc defined by ArcId does not exist (out of bound?)
      if(!vertexList)
        return 0;

      vtkDataArray *scalarField =
        inputMesh->GetPointData()->GetArray(FieldId);
      if(!scalarField)
        return 0;

      // parse the input vertex list (region in which the connectivity of the
      // level sets does not change) and compute the area signature
      double cumulativeVolume = 0;
      std::vector<double> scalarValues, volumeSignature;
      std::vector<int>    vertexIds;
      std::vector<bool>   visitedTetrahedra;

      visitedTetrahedra.resize(inputMesh->GetNumberOfCells());
      vertexIds.resize(vertexList->GetNumberOfTuples() + 2);
      scalarValues.resize(vertexIds.size());
      volumeSignature.resize(vertexIds.size());

      // include the critical points in the computation
      //  - iterates through the edges of the Reeb graph until we found the arc
      //  we're looking for
      //  - retrieve the Source and Target of the edge
      //  - pick the corresponding mesh vertex Ids in the VertexData.
      std::pair<int, int> criticalPoints;
      vtkEdgeListIterator *eIt = vtkEdgeListIterator::New();
      inputGraph->GetEdges(eIt);

      do{
        vtkEdgeType e = eIt->Next();
        if(e.Id == ArcId)
          {
          if((criticalPointIds->GetTuple(e.Source))
            &&(criticalPointIds->GetTuple(e.Target)))
            {
            criticalPoints.first =
              (int) *(criticalPointIds->GetTuple(e.Source));
            criticalPoints.second =
              (int) *(criticalPointIds->GetTuple(e.Target));
            }
          else
            // invalid Reeb graph
            return 0;
          }
      }while(eIt->HasNext());

      eIt->Delete();

      vertexIds[0] = criticalPoints.first;
      vertexIds[vertexIds.size() - 1] = criticalPoints.second;
      // NB: the vertices of vertexList are already in sorted order of function
      // value.
      for(int i = 0; i < vertexList->GetNumberOfTuples(); i++)
        vertexIds[i + 1] = vertexList->GetVariantValue(i).ToInt();

      // mark all the input triangles as non visited.
      for(unsigned int i = 0; i < visitedTetrahedra.size(); i++)
        visitedTetrahedra[i] = false;

      // now do the parsing
      double  min = scalarField->GetComponent(vertexIds[0], 0),
              max = scalarField->GetComponent(vertexIds[vertexIds.size()-1], 0);
      for(unsigned int i = 0; i < vertexIds.size(); i++)
        {
        scalarValues[i] = scalarField->GetComponent(vertexIds[i], 0);

        vtkIdList *starTetrahedronList = vtkIdList::New();
        inputMesh->GetPointCells(vertexIds[i], starTetrahedronList);

        for(int j = 0; j < starTetrahedronList->GetNumberOfIds(); j++)
          {
          vtkIdType tId = starTetrahedronList->GetId(j);
          if(!visitedTetrahedra[tId])
            {
            vtkTetra *t = vtkTetra::SafeDownCast(inputMesh->GetCell(tId));

            if((scalarField->GetComponent(t->GetPointIds()->GetId(0), 0)
              <= scalarValues[i])
              &&
              (scalarField->GetComponent(t->GetPointIds()->GetId(1), 0)
              <= scalarValues[i])
              &&
              (scalarField->GetComponent(t->GetPointIds()->GetId(2), 0)
              <= scalarValues[i])
              &&
              (scalarField->GetComponent(t->GetPointIds()->GetId(3), 0)
              <= scalarValues[i])
              &&
              (scalarField->GetComponent(t->GetPointIds()->GetId(0), 0) >= min)
              &&
              (scalarField->GetComponent(t->GetPointIds()->GetId(1), 0) >= min)
              &&
              (scalarField->GetComponent(t->GetPointIds()->GetId(2), 0) >= min)
              &&
              (scalarField->GetComponent(t->GetPointIds()->GetId(3), 0) >= min)
              )
              {
              // make sure the triangle is strictly in the covered function
              // span.

              double p0[3], p1[3], p2[3], p3[3];
              inputMesh->GetPoint(t->GetPointIds()->GetId(0), p0);
              inputMesh->GetPoint(t->GetPointIds()->GetId(1), p1);
              inputMesh->GetPoint(t->GetPointIds()->GetId(2), p2);
              inputMesh->GetPoint(t->GetPointIds()->GetId(3), p3);
              cumulativeVolume += vtkTetra::ComputeVolume(p0, p1, p2, p3);

              visitedTetrahedra[tId] = true;
              }
            }
          }
        volumeSignature[i] = cumulativeVolume;
        starTetrahedronList->Delete();
        }

      // now adjust to the desired sampling
      std::vector<std::pair<int, double> > samples(NumberOfSamples);
      unsigned int pos = 0;
      for(int i = 0; i < NumberOfSamples; i++)
        {
        samples[i].first = 0;
        samples[i].second = 0;
        while((scalarValues[pos] < min +
           (i+1.0)*((max - min)/((double)NumberOfSamples)))
           &&(pos < scalarValues.size()))
          {
            samples[i].first++;
            samples[i].second += volumeSignature[pos];
            pos++;
          }
        if(samples[i].first) samples[i].second /= samples[i].first;
        }

      // no value at the start? put 0
      if(!samples[0].first)
        {
        samples[0].first = 1;
        samples[0].second = 0;
        }
      // no value at the end? put the cumulative area
      if(!samples[samples.size() - 1].first)
        {
        samples[samples.size() - 1].first = 1;
        samples[samples.size() - 1].second = cumulativeVolume;
        }

      // fill out the blanks
      int lastSample = 0;
      for(int i = 0; i < NumberOfSamples; i++)
        {
        if(!samples[i].first)
          {
          // not enough vertices in the region for the number of desired
          // samples. we have to interpolate.

          // first, search for the next valid sample
          int nextSample = i;
          for(; nextSample < NumberOfSamples; nextSample++)
            {
            if(samples[nextSample].first) break;
            }

          // next interpolate
          samples[i].second = samples[lastSample].second +
            (i - lastSample)
            *(samples[nextSample].second - samples[lastSample].second)
            /(nextSample - lastSample);

          }
        else lastSample = i;
        }

      // now prepare the output
      vtkVariantArray *outputSignature = vtkVariantArray::New();
      outputSignature->SetNumberOfTuples(samples.size());
      for(unsigned int i = 0; i < samples.size(); i++)
        {
        outputSignature->SetValue(i, samples[i].second);
        }
      output->Initialize();
      output->AddColumn(outputSignature);
      outputSignature->Delete();
      }

    return 1;
    }
  return 0;
}
