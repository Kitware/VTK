/*=========================================================================

  Program:   Visualization Library
  Module:    Line.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Line.hh"
#include "vlMath.hh"

int vlLine::GenerateNormals(vlPoints *pts, vlCellArray *lines, vlFloatNormals *normals)
{
  int npts, *linePts;
  float s[3], q[3], norm[3], *n;
  float *p1, *p2;
  float s_norm, n_norm, q_norm;
  int i, j;
  vlMath math;
//
//  Loop over all lines
// 
  for (lines->InitTraversal(); lines->GetNextCell(npts,linePts); )
    {
//
//  Determine initial starting normal
// 
    if ( npts < 2 ) return 0;

    p1 = pts->GetPoint(linePts[0]);
    p2 = pts->GetPoint(linePts[1]);
    for (i=0; i<3; i++) s[i] = p2[i] - p1[i];

    if ( (s_norm = math.Norm(s)) == 0.0 ) return 0;
    for (i=0; i<3; i++) s[i] /= s_norm;

    for (i=0; i<3; i++) 
      {
      if ( s[i] != 0.0 ) 
        {
        norm[(i+2)%3] = 0.0;
        norm[(i+1)%3] = 1.0;
        norm[i] = -s[(i+1)%3]/s[i];
        break;
        }
      }
    n_norm = math.Norm(norm);
    for (i=0; i<3; i++) norm[i] /= n_norm;
    normals->InsertNormal(0,norm);
//
//  Generate normals for new point by projecting previous normal
// 
    for (j=1; j<npts; j++) 
      {
      p1 = pts->GetPoint(linePts[j-1]);
      p2 = pts->GetPoint(linePts[j]);
      for (i=0; i<3; i++) s[i] = p2[i] - p1[i];

      if ( (s_norm = math.Norm(s)) == 0.0 ) return 0;
      for (i=0; i<3; i++) s[i] /= s_norm;

      n = normals->GetNormal(j-1);

      math.Cross(s,n,q);

      if ( (q_norm = math.Norm(q)) == 0.0 ) return 0;
      for (i=0; i<3; i++) q[i] /= q_norm;

      math.Cross (q, s, norm);
      n_norm = math.Norm(norm);
      for (i=0; i<3; i++) norm[i] /= n_norm;
      normals->InsertNormal(j,norm);
      }
    }
  return 1;
}
