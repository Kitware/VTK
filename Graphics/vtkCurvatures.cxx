/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCurvatures.cxx
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
#include "vtkCurvatures.h"

#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataNormals.h"
#include "vtkPolygon.h"
#include "vtkTensor.h"
#include "vtkTriangle.h"

vtkCxxRevisionMacro(vtkCurvatures, "1.8");
vtkStandardNewMacro(vtkCurvatures);

//------------------------------------------------------------------------------
#if VTK3
vtkCurvatures* vtkCurvatures::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkCurvatures");
  if(ret)
    {
    return (vtkCurvatures*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkCurvatures;
}
#endif
//-------------------------------------------------------//
vtkCurvatures::vtkCurvatures()
{
  this->CurvatureType = VTK_CURVATURE_GAUSS;
  this->InvertMeanCurvature = 0;
}
//-------------------------------------------------------//
void vtkCurvatures::GetMeanCurvature()
{
    vtkDebugMacro("Start vtkCurvatures::GetMeanCurvature");
    vtkPolyData* mesh = this->GetOutput();

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
    f = 0;
    F = mesh->GetNumberOfCells();
    Hf = 0.0;
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
          Hf = 0.5*meanCurvatureData[v]/(double)num_neighb[v];
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
    if (num_neighb) delete [] num_neighb;
};
//--------------------------------------------
#define CLAMP_MACRO(v)    ((v)<(-1) ? (-1) : (v) > (1) ? (1) : v)
void vtkCurvatures::GetGaussCurvature()
{
    vtkDebugMacro("Start vtkCurvatures::GetGaussCurvature()");
    // vtk data
    vtkPolyData* output = this->GetOutput();
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
    if (K)              delete [] K;
    if (dA)             delete [] dA;
    if (gaussCurvature) gaussCurvature->Delete();
    /*******************************************************/
};

//-------------------------------------------------------
void vtkCurvatures::Execute()
{
  vtkPolyData *input  = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  // Null input check
  if (!input)
    {
    return;
    }

  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetFieldData()->PassData(input->GetFieldData());

  //-------------------------------------------------------//
  //    Set Curvatures as PointData  Scalars               //
  //-------------------------------------------------------//

  if ( this->CurvatureType == VTK_CURVATURE_GAUSS )
      {
      this->GetGaussCurvature();
      }
  else if ( this->CurvatureType == VTK_CURVATURE_MEAN )
      {
      this->GetMeanCurvature();
      }
  else
      {
      vtkErrorMacro("Only Gauss and Mean Curvature type available");
      }
}
/*-------------------------------------------------------*/
void vtkCurvatures::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);
  os << indent << "CurvatureType: " << this->CurvatureType << "\n";
  os << indent << "InvertMeanCurvature: " << this->InvertMeanCurvature << "\n";
}

