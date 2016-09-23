/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCurvatures.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCurvatures.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataNormals.h"
#include "vtkPolygon.h"
#include "vtkTriangle.h"

vtkStandardNewMacro(vtkCurvatures);

//-------------------------------------------------------//
vtkCurvatures::vtkCurvatures()
{
  this->CurvatureType = VTK_CURVATURE_GAUSS;
  this->InvertMeanCurvature = 0;
}
//-------------------------------------------------------//
void vtkCurvatures::GetMeanCurvature(vtkPolyData *mesh)
{
    vtkDebugMacro("Start vtkCurvatures::GetMeanCurvature");

    // Empty array check
    if (mesh->GetNumberOfPolys()==0 || mesh->GetNumberOfPoints()==0)
    {
      vtkErrorMacro("No points/cells to operate on");
      return;
    }

    int numPts = mesh->GetNumberOfPoints();

    // vtkData
    vtkIdList* vertices, *vertices_n, *neighbours;

    vtkTriangle* facet;
    vtkTriangle* neighbour;
    //     create-allocate
    vertices   = vtkIdList::New();
    vertices_n = vtkIdList::New();
    neighbours = vtkIdList::New();
    facet      = vtkTriangle::New();
    neighbour  = vtkTriangle::New();
    vtkDoubleArray* meanCurvature = vtkDoubleArray::New();
    meanCurvature->SetName("Mean_Curvature");
    meanCurvature->SetNumberOfComponents(1);
    meanCurvature->SetNumberOfTuples(numPts);
    // Get the array so we can write to it directly
    double *meanCurvatureData = meanCurvature->GetPointer(0);
    //     data
    int v, v_l, v_r, v_o,  f, F, n, nv;// n short for neighbor

    //     create-allocate
    double n_f[3]; // normal of facet (could be stored for later?)
    double n_n[3]; // normal of edge
    double t[3];   // to store the cross product of n_f n_n
    double ore[3]; // origin of e
    double end[3]; // end of e
    double oth[3]; //     third vertex necessary for comp of n
    double vn0[3];
    double vn1[3]; // vertices for computation of neighbour's n
    double vn2[3];
    double e[3];   // edge (oriented)

    double cs, sn;    // cs: cos; sn sin
    double angle, length, Af, Hf;  // temporary store

    mesh->BuildLinks();
    //data init
    F = mesh->GetNumberOfCells();
    // init, preallocate the mean curvature
    int* num_neighb = new int[numPts];
    for (v = 0; v < numPts; v++)
    {
      meanCurvatureData[v] = 0.0;
      num_neighb[v] = 0;
    }

    //     main loop
    vtkDebugMacro(<<"Main loop: loop over facets such that id > id of neighb");
    vtkDebugMacro(<<"so that every edge comes only once");
    //
    for (f = 0; f < F; f++)
    {
      mesh->GetCellPoints(f,vertices);
      nv = vertices->GetNumberOfIds();

      for (v = 0; v < nv; v++)
      {
        // get neighbour
        v_l = vertices->GetId(v);
        v_r = vertices->GetId((v+1) % nv);
        v_o = vertices->GetId((v+2) % nv);
        mesh->GetCellEdgeNeighbors(f,v_l,v_r,neighbours);

        // compute only if there is really ONE neighbour
        // AND meanCurvature has not been computed yet!
        // (ensured by n > f)
        if (neighbours->GetNumberOfIds() == 1 && (n = neighbours->GetId(0)) > f)
        {
          // find 3 corners of f: in order!
          mesh->GetPoint(v_l,ore);
          mesh->GetPoint(v_r,end);
          mesh->GetPoint(v_o,oth);
          // compute normal of f
          facet->ComputeNormal(ore,end,oth,n_f);
          // compute common edge
          e[0] = end[0]; e[1] = end[1]; e[2] = end[2];
          e[0] -= ore[0]; e[1] -= ore[1]; e[2] -= ore[2];
          length = double(vtkMath::Normalize(e));
          Af = double(facet->TriangleArea(ore,end,oth));
          // find 3 corners of n: in order!
          mesh->GetCellPoints(n,vertices_n);
          mesh->GetPoint(vertices_n->GetId(0),vn0);
          mesh->GetPoint(vertices_n->GetId(1),vn1);
          mesh->GetPoint(vertices_n->GetId(2),vn2);
          Af += double(facet->TriangleArea(vn0,vn1,vn2));
          // compute normal of n
          neighbour->ComputeNormal(vn0,vn1,vn2,n_n);
          // the cosine is n_f * n_n
          cs = double(vtkMath::Dot(n_f,n_n));
          // the sin is (n_f x n_n) * e
          vtkMath::Cross(n_f,n_n,t);
          sn = double(vtkMath::Dot(t,e));
          // signed angle in [-pi,pi]
          if (sn!=0.0 || cs!=0.0)
          {
            angle = atan2(sn,cs);
            Hf    = length*angle;
          }
          else
          {
            Hf = 0.0;
          }
          // add weighted Hf to scalar at v_l and v_r
          if (Af!=0.0)
          {
            (Hf /= Af) *=3.0;
          }
          meanCurvatureData[v_l] += Hf;
          meanCurvatureData[v_r] += Hf;
          num_neighb[v_l] += 1;
          num_neighb[v_r] += 1;
        }
      }
    }

    // put curvature in vtkArray
    for (v = 0; v < numPts; v++)
    {
        if (num_neighb[v]>0)
        {
          Hf = 0.5*meanCurvatureData[v]/num_neighb[v];
          if (this->InvertMeanCurvature)
          {
            meanCurvatureData[v] = -Hf;
          }
          else
          {
            meanCurvatureData[v] = Hf;
          }
        }
        else
        {
          meanCurvatureData[v] = 0.0;
        }
    }

    mesh->GetPointData()->AddArray(meanCurvature);
    mesh->GetPointData()->SetActiveScalars("Mean_Curvature");

    vtkDebugMacro("Set Values of Mean Curvature: Done");
    // clean
    vertices  ->Delete();
    vertices_n->Delete();
    neighbours->Delete();
    facet     ->Delete();
    neighbour ->Delete();

    if (meanCurvature) meanCurvature->Delete();
    delete [] num_neighb;
};
//--------------------------------------------
#define CLAMP_MACRO(v)    ((v)<(-1) ? (-1) : (v) > (1) ? (1) : (v))
void vtkCurvatures::GetGaussCurvature(vtkPolyData *output)
{
    vtkDebugMacro("Start vtkCurvatures::GetGaussCurvature()");
    // vtk data
    vtkCellArray* facets = output->GetPolys();

    // Empty array check
    if (output->GetNumberOfPolys()==0 || output->GetNumberOfPoints()==0)
    {
      vtkErrorMacro("No points/cells to operate on");
      return;
    }

    vtkTriangle* facet = vtkTriangle::New();

    // other data
    vtkIdType Nv   = output->GetNumberOfPoints();

    double* K = new double[Nv];
    double* dA = new double[Nv];
    double pi2 = 2.0*vtkMath::Pi();
    for (int k = 0; k < Nv; k++)
    {
      K[k]  = pi2;
      dA[k] = 0.0;
    }

    double v0[3], v1[3], v2[3], e0[3], e1[3], e2[3];

    double A, alpha0, alpha1, alpha2;

    vtkIdType f, *vert=0;
    facets->InitTraversal();
    while (facets->GetNextCell(f,vert))
    {
      output->GetPoint(vert[0],v0);
      output->GetPoint(vert[1],v1);
      output->GetPoint(vert[2],v2);
      // edges
      e0[0] = v1[0] ; e0[1] = v1[1] ; e0[2] = v1[2] ;
      e0[0] -= v0[0]; e0[1] -= v0[1]; e0[2] -= v0[2];

      e1[0] = v2[0] ; e1[1] = v2[1] ; e1[2] = v2[2] ;
      e1[0] -= v1[0]; e1[1] -= v1[1]; e1[2] -= v1[2];

      e2[0] = v0[0] ; e2[1] = v0[1] ; e2[2] = v0[2] ;
      e2[0] -= v2[0]; e2[1] -= v2[1]; e2[2] -= v2[2];

      // normalise
      vtkMath::Normalize(e0); vtkMath::Normalize(e1); vtkMath::Normalize(e2);
      // angles
      // I get lots of acos domain errors so clamp the value to +/-1 as the
      // normalize function can return 1.000000001 etc (I think)
      double ac1 = vtkMath::Dot(e1,e2);
      double ac2 = vtkMath::Dot(e2,e0);
      double ac3 = vtkMath::Dot(e0,e1);
      alpha0 = acos(-CLAMP_MACRO(ac1));
      alpha1 = acos(-CLAMP_MACRO(ac2));
      alpha2 = acos(-CLAMP_MACRO(ac3));

      // surf. area
      A = double(facet->TriangleArea(v0,v1,v2));
      // UPDATE
      dA[vert[0]] += A;
      dA[vert[1]] += A;
      dA[vert[2]] += A;
      K[vert[0]] -= alpha1;
      K[vert[1]] -= alpha2;
      K[vert[2]] -= alpha0;
    }

    int numPts = output->GetNumberOfPoints();
    // put curvature in vtkArray
    vtkDoubleArray* gaussCurvature = vtkDoubleArray::New();
    gaussCurvature->SetName("Gauss_Curvature");
    gaussCurvature->SetNumberOfComponents(1);
    gaussCurvature->SetNumberOfTuples(numPts);
    double *gaussCurvatureData = gaussCurvature->GetPointer(0);

    for (int v = 0; v < Nv; v++)
    {
      if (dA[v]>0.0)
      {
        gaussCurvatureData[v] = 3.0*K[v]/dA[v];
      }
      else
      {
        gaussCurvatureData[v] = 0.0;
      }
    }

    output->GetPointData()->AddArray(gaussCurvature);
    output->GetPointData()->SetActiveScalars("Gauss_Curvature");

    vtkDebugMacro("Set Values of Gauss Curvature: Done");
    /*******************************************************/
    if (facet) facet->Delete();
    delete [] K;
    delete [] dA;
    if (gaussCurvature) gaussCurvature->Delete();
    /*******************************************************/
};

void vtkCurvatures::GetMaximumCurvature(vtkPolyData *input,vtkPolyData *output)
{
  this->GetGaussCurvature(output);
  this->GetMeanCurvature(output);

  vtkIdType numPts = input->GetNumberOfPoints();

  vtkDoubleArray *maximumCurvature = vtkDoubleArray::New();
  maximumCurvature->SetNumberOfComponents(1);
  maximumCurvature->SetNumberOfTuples(numPts);
  maximumCurvature->SetName("Maximum_Curvature");
  output->GetPointData()->AddArray(maximumCurvature);
  output->GetPointData()->SetActiveScalars("Maximum_Curvature");
  maximumCurvature->Delete();

  vtkDoubleArray *gauss = static_cast<vtkDoubleArray *>(
    output->GetPointData()->GetArray("Gauss_Curvature"));
  vtkDoubleArray *mean = static_cast<vtkDoubleArray *>(
    output->GetPointData()->GetArray("Mean_Curvature"));
  double k, h, k_max,tmp;

  for (vtkIdType i = 0; i<numPts; i++)
  {
    k = gauss->GetComponent(i,0);
    h = mean->GetComponent(i,0);
    tmp = h*h - k;
    if (tmp >= 0)
    {
      k_max = h + sqrt(tmp);
    }
    else
    {
      vtkDebugMacro(<< "Maximum Curvature undefined at point: " << i);
      // k_max can be any real number. Undefined points will be indistinguishable
      // from points that actually have a k_max == 0
      k_max = 0;
    }
    maximumCurvature->SetComponent(i, 0, k_max);
  }
}

void vtkCurvatures::GetMinimumCurvature(vtkPolyData *input,vtkPolyData *output)
{
  this->GetGaussCurvature(output);
  this->GetMeanCurvature(output);

  vtkIdType numPts = input->GetNumberOfPoints();

  vtkDoubleArray *minimumCurvature = vtkDoubleArray::New();
  minimumCurvature->SetNumberOfComponents(1);
  minimumCurvature->SetNumberOfTuples(numPts);
  minimumCurvature->SetName("Minimum_Curvature");
  output->GetPointData()->AddArray(minimumCurvature);
  output->GetPointData()->SetActiveScalars("Minimum_Curvature");
  minimumCurvature->Delete();

  vtkDoubleArray *gauss = static_cast<vtkDoubleArray *>(
    output->GetPointData()->GetArray("Gauss_Curvature"));
  vtkDoubleArray *mean = static_cast<vtkDoubleArray *>(
    output->GetPointData()->GetArray("Mean_Curvature"));
  double k, h, k_min,tmp;

  for (vtkIdType i = 0; i<numPts; i++)
  {
    k = gauss->GetComponent(i,0);
    h = mean->GetComponent(i,0);
    tmp = h*h - k;
    if (tmp >= 0)
    {
      k_min = h - sqrt(tmp);
    }
    else
    {
      vtkDebugMacro(<< "Minimum Curvature undefined at point: " << i);
      // k_min can be any real number. Undefined points will be indistinguishable
      // from points that actually have a k_min == 0
      k_min = 0;
    }
    minimumCurvature->SetComponent(i, 0, k_min);
  }
}

//-------------------------------------------------------
int vtkCurvatures::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Null input check
  if (!input)
  {
    return 0;
  }

  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());
  output->GetFieldData()->PassData(input->GetFieldData());

  //-------------------------------------------------------//
  //    Set Curvatures as PointData  Scalars               //
  //-------------------------------------------------------//

  if ( this->CurvatureType == VTK_CURVATURE_GAUSS )
  {
    this->GetGaussCurvature(output);
  }
  else if ( this->CurvatureType == VTK_CURVATURE_MEAN )
  {
    this->GetMeanCurvature(output);
  }
  else if ( this->CurvatureType ==  VTK_CURVATURE_MAXIMUM )
  {
    this->GetMaximumCurvature(input, output);
  }
  else if ( this->CurvatureType ==  VTK_CURVATURE_MINIMUM )
  {
    this->GetMinimumCurvature(input, output);
  }
  else
  {
    vtkErrorMacro("Only Gauss, Mean, Max, and Min Curvature type available");
    return 1;
  }

  return 1;
}
/*-------------------------------------------------------*/
void vtkCurvatures::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "CurvatureType: " << this->CurvatureType << "\n";
  os << indent << "InvertMeanCurvature: " << this->InvertMeanCurvature << "\n";
}
