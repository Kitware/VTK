/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReebGraphSurfaceSkeletonFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkReebGraphSurfaceSkeletonFilter.h"

#include "vtkContourFilter.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkEdgeListIterator.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkReebGraph.h"
#include "vtkTable.h"
#include "vtkTriangle.h"
#include "vtkVariantArray.h"

vtkStandardNewMacro(vtkReebGraphSurfaceSkeletonFilter);

//----------------------------------------------------------------------------
vtkReebGraphSurfaceSkeletonFilter::vtkReebGraphSurfaceSkeletonFilter()
{
  this->SetNumberOfInputPorts(2);
  this->FieldId = 0;
  this->NumberOfSamples = 5;
  this->NumberOfSmoothingIterations = 30;
}

//----------------------------------------------------------------------------
vtkReebGraphSurfaceSkeletonFilter::~vtkReebGraphSurfaceSkeletonFilter()
{
}

//----------------------------------------------------------------------------
int vtkReebGraphSurfaceSkeletonFilter::FillInputPortInformation(
  int portNumber, vtkInformation *info)
{
  switch(portNumber)
  {
    case 0:
      info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
      info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
      break;
    case 1:
      info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
      info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkReebGraph");
      break;
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkReebGraphSurfaceSkeletonFilter::FillOutputPortInformation(int vtkNotUsed(portNumber), vtkInformation *info)
{

  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");

  return 1;
}

//----------------------------------------------------------------------------
void vtkReebGraphSurfaceSkeletonFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Field Id: " << this->FieldId << "\n";
  os << indent << "Number of Samples: " << this->NumberOfSamples << "\n";
  os << indent << "Number of Smoothing Iterations: " << this->NumberOfSmoothingIterations << "\n";
}

//----------------------------------------------------------------------------
vtkTable* vtkReebGraphSurfaceSkeletonFilter::GetOutput()
{
  return vtkTable::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
int vtkReebGraphSurfaceSkeletonFilter::RequestData(vtkInformation* vtkNotUsed(request), vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{

  vtkInformation  *inInfoMesh = inputVector[0]->GetInformationObject(0),
                  *inInfoGraph = inputVector[1]->GetInformationObject(0);

  if ((!inInfoMesh)||(!inInfoGraph))
  {
    return 0;
  }
  vtkPolyData *inputMesh = vtkPolyData::SafeDownCast(
    inInfoMesh->Get(vtkPolyData::DATA_OBJECT()));

  vtkReebGraph *inputGraph = vtkReebGraph::SafeDownCast(
    inInfoGraph->Get(vtkReebGraph::DATA_OBJECT()));

  if ((inputMesh)&&(inputGraph))
  {
    vtkInformation  *outInfo = outputVector->GetInformationObject(0);
    vtkTable        *output = vtkTable::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));

    if(output)
    {

      // Retrieve the information regarding the critical noes.
      vtkDataArray  *vertexInfo = vtkArrayDownCast<vtkDataArray>(
        inputGraph->GetVertexData()->GetAbstractArray("Vertex Ids"));
      if(!vertexInfo)
        // invalid Reeb graph (no information associated to the vertices)
        return 0;

      vtkVariantArray *edgeInfo = vtkArrayDownCast<vtkVariantArray>(
        inputGraph->GetEdgeData()->GetAbstractArray("Vertex Ids"));
      if(!edgeInfo)
        // invalid Reeb graph (no information associated to the edges)
        return 0;

      vtkDataArray *scalarField = inputMesh->GetPointData()->GetArray(FieldId);
      if(!scalarField)
        // invalid input mesh (no scalar field associated to it)
        return 0;


      vtkEdgeListIterator *eIt = vtkEdgeListIterator::New();
      inputGraph->GetEdges(eIt);
      std::pair<int, int> criticalNodeIds;

      std::vector<std::vector<std::vector<double> > > skeleton;

      do
      {
        vtkEdgeType e = eIt->Next();
        vtkAbstractArray *vertexList = edgeInfo->GetPointer(e.Id)->ToArray();
        if((vertexInfo->GetTuple(e.Source))&&(vertexInfo->GetTuple(e.Target)))
        {
          criticalNodeIds.first = (int) *(vertexInfo->GetTuple(e.Source));
          criticalNodeIds.second = (int) *(vertexInfo->GetTuple(e.Target));
        }
        else
          // invalid Reeb graph
          return 0;


        // intermediate sub-mesh
        vtkPolyData     *subMesh = vtkPolyData::New();
        vtkDoubleArray  *subField = vtkDoubleArray::New();
        vtkPoints       *subPointSet = vtkPoints::New();
        vtkDoubleArray  *subCoordinates = vtkDoubleArray::New();
        std::vector<vtkIdType> meshToSubMeshMap(inputMesh->GetNumberOfPoints());

        subCoordinates->SetNumberOfComponents(3);
        subField->SetNumberOfComponents(1);

        subMesh->Allocate();


        std::vector<bool>   visitedTriangles(inputMesh->GetNumberOfCells());
        for(unsigned int i = 0; i < visitedTriangles.size(); i++)
          visitedTriangles[i] = false;

        std::vector<bool>   visitedVertices(inputMesh->GetNumberOfPoints());
        for(unsigned int i = 0; i < visitedVertices.size(); i++)
          visitedVertices[i] = false;

        // add the vertices to the subMesh
        for(int i = 0; i < vertexList->GetNumberOfTuples(); i++)
        {
          vtkIdList *starTriangleList = vtkIdList::New();
          inputMesh->GetPointCells(
            vertexList->GetVariantValue(i).ToInt(), starTriangleList);

          for(int j = 0; j < starTriangleList->GetNumberOfIds(); j++)
          {
            vtkIdType tId = starTriangleList->GetId(j);
            vtkTriangle *t =
              vtkTriangle::SafeDownCast(inputMesh->GetCell(tId));


            std::vector<vtkIdType> vertices(3);
            std::vector<double *> points(3);

            for(int k = 0; k < 3; k++)
            {
              vertices[k] = t->GetPointIds()->GetId(k);
              if(!visitedVertices[vertices[k]])
              {
                // add also its scalar value to the subField
                points[k] = (double *) malloc(sizeof(double)*3);
                inputMesh->GetPoint(vertices[k], points[k]);
                meshToSubMeshMap[vertices[k]] =
                  subCoordinates->InsertNextTypedTuple(points[k]);
                double scalarFieldValue =
                  scalarField->GetComponent(vertices[k], 0);
                subField->InsertNextTypedTuple(&scalarFieldValue);
                visitedVertices[vertices[k]] = true;
                free(points[k]);
              }
            }
          }

          starTriangleList->Delete();
        }

        subPointSet->SetData(subCoordinates);
        subMesh->SetPoints(subPointSet);
        subMesh->GetPointData()->SetScalars(subField);

        // add the triangles to the subMesh
        for(int i = 0; i < vertexList->GetNumberOfTuples(); i++)
        {
          // add the triangles to the chart.
          vtkIdList *starTriangleList = vtkIdList::New();
          inputMesh->GetPointCells(
            vertexList->GetVariantValue(i).ToInt(), starTriangleList);

          for(int j = 0; j < starTriangleList->GetNumberOfIds(); j++)
          {
            vtkIdType tId = starTriangleList->GetId(j);
            if(!visitedTriangles[tId])
            {
              vtkTriangle *t =
                vtkTriangle::SafeDownCast(inputMesh->GetCell(tId));

              vtkIdType   *vertexIds =
                (vtkIdType *) malloc(sizeof(vtkIdType)*3);
              for(int k = 0; k < 3; k++)
                vertexIds[k] = meshToSubMeshMap[t->GetPointIds()->GetId(k)];

              subMesh->InsertNextCell(VTK_TRIANGLE, 3, vertexIds);
              free(vertexIds);
              visitedTriangles[tId] = true;
            }
          }
          starTriangleList->Delete();
        }

        // now launch the level set traversal
        double  minValue = scalarField->GetComponent(criticalNodeIds.first, 0),
                maxValue = scalarField->GetComponent(criticalNodeIds.second, 0);

        std::vector<std::vector<double> > arcSkeleton;

        // add the first critical point at the origin of the arc skeleton
        double *criticalPoint = (double *) malloc(sizeof(double)*3);
        inputMesh->GetPoint(criticalNodeIds.first, criticalPoint);
        std::vector<double> arcEntry(3);
        for(int i = 0; i < 3; i++)
          arcEntry[i] = criticalPoint[i];
        arcSkeleton.push_back(arcEntry);
        free(criticalPoint);

        if(vertexList->GetNumberOfTuples() > 1)
        {
          for(int i = 0; i < NumberOfSamples; i++)
          {
            vtkContourFilter *contourFilter = vtkContourFilter::New();

            contourFilter->SetNumberOfContours(1);
            contourFilter->SetValue(0, minValue +
              (i + 1.0)*(maxValue - minValue)/(((double)NumberOfSamples) + 1.0));
            contourFilter->SetInputData(subMesh);
            contourFilter->Update();

            vtkPolyData *contourMesh = contourFilter->GetOutput();
            std::vector<double> baryCenter(3);
            for(int j = 0; j < 3; j++)
            {
              baryCenter[j] = 0.0;
            }

            if(contourMesh->GetNumberOfPoints() > 1)
            {
              // if the current arc of the Reeb graph has not deg-2 node, then
              // the level set will most likely be empty.

              for(int j = 0; j < contourMesh->GetNumberOfPoints(); j++)
              {
                double *point = (double *) malloc(sizeof(double)*3);
                contourMesh->GetPoint(j, point);
                for(int k = 0; k < 3; k++)
                  baryCenter[k] += point[k];
                free(point);
              }
              for(int j = 0; j < 3; j++)
                baryCenter[j] /= contourMesh->GetNumberOfPoints();
                            arcSkeleton.push_back(baryCenter);
            }
            contourFilter->Delete();
          }
        }
        criticalPoint = (double *) malloc(sizeof(double)*3);
        inputMesh->GetPoint(criticalNodeIds.second, criticalPoint);
        for(int i = 0; i < 3; i++)
          arcEntry[i] = criticalPoint[i];
        arcSkeleton.push_back(arcEntry);
        free(criticalPoint);

        // if we have an empty arc skeleton, let's fill the blanks to have an
        // homogeneous output
        if(arcSkeleton.size() == 2)
        {
          arcSkeleton.resize(NumberOfSamples + 2);
          std::vector<double> lastPoint = arcEntry;
          for(int i = 1; i <= NumberOfSamples; i++)
          {
            for(int j = 0; j < 3; j++)
            {
              arcEntry[j] = arcSkeleton[0][j]
                + (((double) i)/(NumberOfSamples + 1.0))
                  *(lastPoint[j] - arcSkeleton[0][j]);
            }
            arcSkeleton[i] = arcEntry;
          }
          arcSkeleton[arcSkeleton.size() - 1] = lastPoint;
        }

        // now do the smoothing of the arc skeleton
        std::vector<std::vector<double> > smoothedArc;
        for(int i = 0; i < NumberOfSmoothingIterations; i++)
        {
          smoothedArc.push_back(arcSkeleton[0]);
          if(arcSkeleton.size() > 2)
          {
            for(unsigned int j = 1; j < arcSkeleton.size() - 1; j++)
            {
              std::vector<double> smoothedSample(3);
              for(int k = 0; k < 3; k++)
                smoothedSample[k] =
                  (arcSkeleton[j-1][k]
                  + arcSkeleton[j+1][k] + arcSkeleton[j][k])/3;

              smoothedArc.push_back(smoothedSample);
            }
          }
          smoothedArc.push_back(arcSkeleton[arcSkeleton.size() - 1]);

          // now, replace arcSkeleton with smoohtedArc for the next Iteration
          for(unsigned int j = 0; j < arcSkeleton.size(); j++)
            arcSkeleton[j] = smoothedArc[j];
        }

        // now add the arc skeleton to the output.
        skeleton.push_back(arcSkeleton);
        subCoordinates->Delete();
        subPointSet->Delete();
        subField->Delete();
        subMesh->Delete();

      }while(eIt->HasNext());

      eIt->Delete();

      // now prepare the output
      output->Initialize();
      for(unsigned int i = 0; i < skeleton.size(); i++)
      {
        vtkDoubleArray *outputArc = vtkDoubleArray::New();
        outputArc->SetNumberOfComponents(3);
        for(unsigned int j = 0; j < skeleton[i].size(); j++)
        {
          double *point = (double *) malloc(sizeof(double)*3);
          for(int k = 0; k < 3; k++)
            point[k] = skeleton[i][j][k];
          outputArc->InsertNextTypedTuple(point);
          free(point);
        }
        output->AddColumn(outputArc);
        outputArc->Delete();
      }
    }

    return 1;
  }
  return 0;
}
