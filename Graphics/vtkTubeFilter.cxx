/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTubeFilter.cxx
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
#include "vtkTubeFilter.h"
#include "vtkPolyLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"

vtkCxxRevisionMacro(vtkTubeFilter, "1.59");
vtkStandardNewMacro(vtkTubeFilter);

// Construct object with radius 0.5, radius variation turned off, the number 
// of sides set to 3, and radius factor of 10.
vtkTubeFilter::vtkTubeFilter()
{
  this->Radius = 0.5;
  this->VaryRadius = VTK_VARY_RADIUS_OFF;
  this->NumberOfSides = 3;
  this->RadiusFactor = 10;

  this->DefaultNormal[0] = this->DefaultNormal[1] = 0.0;
  this->DefaultNormal[2] = 1.0;
  
  this->UseDefaultNormal = 0;
  this->SidesShareVertices = 1;
  this->Capping = 0;
  this->OnRatio = 1;
  this->Offset = 0;
}

void vtkTubeFilter::Execute()
{
  vtkIdType i;
  int j, k;
  vtkPoints *inPts;
  vtkCellArray *inLines = NULL;
  vtkDataArray *inNormals;
  vtkDataArray *inScalars=NULL;
  vtkDataArray *inVectors=NULL;
  vtkIdType numPts = 0;
  vtkIdType numNewPts;
  vtkPoints *newPts;
  vtkFloatArray *newNormals;
  vtkCellArray *newStrips;
  int i1, i2, ptOffset=0;
  vtkIdType *pts, npts;
  float p[3], pNext[3];
  float maxSpeed = 0;
  float *n, normal[3], nP[3];
  float s[3], sNext[3], sPrev[3], w[3];
  double BevelAngle;
  float theta=2.0*vtkMath::Pi()/this->NumberOfSides;
  vtkIdType ptId;
  int deleteNormals=0;
  float sFactor=1.0, range[2];
  vtkPointData *pd, *outPD;
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  // Keep caps separate (add later for simplicity).
  vtkPoints *capPoints = vtkPoints::New();
  vtkFloatArray *capNormals = vtkFloatArray::New();
  float capNorm[3];
  int capPointFlag;

  capNormals->SetNumberOfComponents(3);
  // Initialize
  //
  vtkDebugMacro(<<"Creating tube");

  if ( !(inPts=input->GetPoints()) || 
      (numPts = inPts->GetNumberOfPoints()) < 1 ||
      !(inLines = input->GetLines()) || inLines->GetNumberOfCells() < 1 )
    {
    vtkDebugMacro(<< ": No input data!\n");
    capPoints->Delete();
    capNormals->Delete();
    return;
    }
  numNewPts = numPts * this->NumberOfSides;

  // copy scalars, vectors, tcoords. Normals may be computed here.
  pd = input->GetPointData();
  outPD = output->GetPointData();
  outPD->CopyNormalsOff();
  outPD->CopyAllocate(pd,numNewPts);

  int generate_normals = 0;
  vtkPolyLine *lineNormalGenerator;
  
  if ( !(inNormals=pd->GetNormals()) || this->UseDefaultNormal )
    {
    deleteNormals = 1;
    inNormals = vtkFloatArray::New();
    inNormals->SetNumberOfComponents(3);
    inNormals->SetNumberOfTuples(numPts);

    if ( this->UseDefaultNormal )
      {
      for ( i=0; i < numPts; i++)
        {
        inNormals->SetTuple(i,this->DefaultNormal);
        }
      }
    else
      {
      // Normal generation has been moved to lower in the function.
      // This allows each different polylines to share vertices, but have
      // their normals (and hence their tubes) calculated independently
      generate_normals = 1;
      }      
    }
  // If varying width, get appropriate info.
  //
  if ( this->VaryRadius == VTK_VARY_RADIUS_BY_SCALAR && 
  (inScalars=pd->GetScalars()) )
    {
    inScalars->GetRange(range,0);
    }
  else if ( this->VaryRadius == VTK_VARY_RADIUS_BY_VECTOR && 
  (inVectors=pd->GetVectors()) )
    {
    maxSpeed = inVectors->GetMaxNorm();
    }

  newPts = vtkPoints::New();
  newPts->Allocate(numNewPts);
  newNormals = vtkFloatArray::New();
  newNormals->SetNumberOfComponents(3);
  newNormals->Allocate(3*numNewPts);
  newStrips = vtkCellArray::New();
  newStrips->Allocate(newStrips->EstimateSize(1,numNewPts));

  //  Create points along the line that are later connected into a 
  //  triangle strip.
  //
  lineNormalGenerator = vtkPolyLine::New();
  for (inLines->InitTraversal(); inLines->GetNextCell(npts,pts); )
    {
    // if necessary calculate normals, each polyline calculates its
    // normals independently, avoiding conflicts at shared vertices
    vtkCellArray *singlePolyline = vtkCellArray::New();
    if (generate_normals) 
      {
      singlePolyline->InsertNextCell( npts, pts );
    
      if ( !lineNormalGenerator->GenerateSlidingNormals(inPts,singlePolyline,
                                                        inNormals) )
        {
        vtkErrorMacro(<< "No normals for line!\n");
        if (deleteNormals)
          {
          inNormals->Delete();
          }
        newPts->Delete();
        newNormals->Delete();
        newStrips->Delete();
        singlePolyline->Delete();
        lineNormalGenerator->Delete();
        capPoints->Delete();
        capNormals->Delete();
        return;
        }
      }

    // Use "averaged" segment to create beveled effect. 
    // Watch out for first and last points.
    //
    for (j=0; j < npts; j++)
      {
      if (npts < 2)
        {
        vtkErrorMacro(<< "less than two points");
        }

      if ( j == 0 ) //first point
        {
        inPts->GetPoint(pts[0],p);
        inPts->GetPoint(pts[1],pNext);
        for (i=0; i<3; i++) 
          {
          sNext[i] = pNext[i] - p[i];
          sPrev[i] = sNext[i];
          capNorm[i] = -sPrev[i];
          }
        capPointFlag = 1;
        }
      else if ( j == (npts-1) ) //last point
        {
        for (i=0; i<3; i++)
          {
          sPrev[i] = sNext[i];
          p[i] = pNext[i];
          capNorm[i] = sNext[i];
          }
        capPointFlag = 1;
        }
      else
        {
        for (i=0; i<3; i++)
          {
          p[i] = pNext[i];
          }
        inPts->GetPoint(pts[j+1],pNext);
        for (i=0; i<3; i++)
          {
          sPrev[i] = sNext[i];
          sNext[i] = pNext[i] - p[i];
          }
        capPointFlag = 0;
        }

      n = inNormals->GetTuple(pts[j]);

      if ( vtkMath::Normalize(sNext) == 0.0 )
        {
        vtkErrorMacro(<<"Coincident points!");
        if (deleteNormals)
          {
          inNormals->Delete();
          }
        newPts->Delete();
        newNormals->Delete();
        newStrips->Delete();
        lineNormalGenerator->Delete();
        capPoints->Delete();
        capNormals->Delete();
        singlePolyline->Delete();
        return;
        }

      for (i=0; i<3; i++)
        {
        s[i] = (sPrev[i] + sNext[i]) / 2.0; //average vector
        }
      // if s is zero then just use sPrev cross n
      if (vtkMath::Normalize(s) == 0.0)
        {
        vtkWarningMacro(<< "using alternate bevel vector");
        vtkMath::Cross(sPrev,n,s);
        if (vtkMath::Normalize(s) == 0.0)
          {
          vtkErrorMacro(<< "using alternate bevel vector");
          }
        }
      
      if ( (BevelAngle = vtkMath::Dot(sNext,sPrev)) > 1.0 )
        {
        BevelAngle = 1.0;
        }
      if ( BevelAngle < -1.0 )
        {
        BevelAngle = -1.0;
        }
      BevelAngle = acos((double)BevelAngle) / 2.0; //(0->90 degrees)
      if ( (BevelAngle = cos(BevelAngle)) == 0.0 )
        {
        BevelAngle = 1.0;
        }

      BevelAngle = this->Radius / BevelAngle; //keep tube constant radius

      vtkMath::Cross(s,n,w);
      if ( vtkMath::Normalize(w) == 0.0)
        {
        vtkErrorMacro(<<"Bad normal s = " << s[0] << " " << s[1] << " " << s[2] 
                      << " n = " << n[0] << " " << n[1] << " " << n[2]);
        if (deleteNormals)
          {
          inNormals->Delete();
          }
        newPts->Delete();
        newNormals->Delete();
        newStrips->Delete();
        lineNormalGenerator->Delete();
        capPoints->Delete();
        capNormals->Delete();
        singlePolyline->Delete();
        return;
        }
      
      vtkMath::Cross(w,s,nP); //create orthogonal coordinate system
      vtkMath::Normalize(nP);

      if ( inScalars ) // varying by scalar values
        {
        sFactor = 1.0 + ((this->RadiusFactor - 1.0) * 
                  (inScalars->GetComponent(pts[j],0) - range[0]) 
                         / (range[1]-range[0]));
        if ((range[1] - range[0]) == 0.0)
          {
          vtkErrorMacro(<< "Dividing by zero");
          }
        }
      else if ( inVectors ) // use flux preserving relationship
        {
        sFactor = 
          sqrt((double)maxSpeed/vtkMath::Norm(inVectors->GetTuple(pts[j])));
        if ( sFactor > this->RadiusFactor )
          {
          sFactor = this->RadiusFactor;
          }
        }

      //create points around line
      if (this->SidesShareVertices)
        {
        for (k=0; k < this->NumberOfSides; k++)
          {
          for (i=0; i<3; i++) 
            {
            normal[i] = w[i]*cos((double)k*theta) + nP[i]*sin((double)k*theta);
            s[i] = p[i] + this->Radius * sFactor * normal[i];
            }
          if (this->Capping && capPointFlag)
            {
            vtkMath::Normalize(capNorm);
            capPoints->InsertNextPoint(s);
            capNormals->InsertNextTuple(capNorm);
            }
          ptId = newPts->InsertNextPoint(s);
          newNormals->InsertTuple(ptId,normal);
          outPD->CopyData(pd,pts[j],ptId);
          }
        } 
      else
        {
        float n_left[3], n_right[3];
        for (k=0; k < this->NumberOfSides; k++)
          {
          for (i=0; i<3; i++)
            {
            // Create duplicate vertices at each point
            // and adjust the associated normals so that they are
            // oriented with the facets. This preserves the tube's
            // polygonal appearance, as if by flat-shading around the tube,
            // while still allowing smooth (gouraud) shading along the
            // tube as it bends.
            normal[i]  = w[i]*cos((double)(k+0.0)*theta) + nP[i]*sin((double)(k+0.0)*theta);
            n_right[i] = w[i]*cos((double)(k-0.5)*theta) + nP[i]*sin((double)(k-0.5)*theta);
            n_left[i]  = w[i]*cos((double)(k+0.5)*theta) + nP[i]*sin((double)(k+0.5)*theta);
            s[i] = p[i] + this->Radius * sFactor * normal[i];
            }
          if (this->Capping && capPointFlag)
            {
            vtkMath::Normalize(capNorm);
            capPoints->InsertNextPoint(s);
            capNormals->InsertNextTuple(capNorm);
            }
          ptId = newPts->InsertNextPoint(s);
          newNormals->InsertTuple(ptId,n_right);
          outPD->CopyData(pd,pts[j],ptId);
          ptId = newPts->InsertNextPoint(s);
          newNormals->InsertTuple(ptId,n_left);
          outPD->CopyData(pd,pts[j],ptId);
          }
        }
      }//for all points in polyline

    // Generate the strips
    //
    if (this->SidesShareVertices)
      {
      for (k=this->Offset; k<(this->NumberOfSides+this->Offset); k+=this->OnRatio)
        {
        i1 = (k+1) % this->NumberOfSides;
        newStrips->InsertNextCell(npts*2);
        for (i=0; i < npts; i++) 
          {
          i2 = i*this->NumberOfSides;
          newStrips->InsertCellPoint(ptOffset+i2+i1);
          newStrips->InsertCellPoint(ptOffset+i2+k);
          }
        } //for each side of the tube
      
      ptOffset += this->NumberOfSides*npts;
      singlePolyline->Delete();
      }
    else
      {
      for (k=this->Offset; k<(this->NumberOfSides+this->Offset); k+=this->OnRatio)
        {
        i1 = (k+1) % this->NumberOfSides;
        newStrips->InsertNextCell(npts*2);
        for (i=0; i < npts; i++) 
          {
          i2 = 2*i*this->NumberOfSides;
          newStrips->InsertCellPoint(ptOffset+i2+i1*2);
          newStrips->InsertCellPoint(ptOffset+i2+k*2+1);
          }
        } //for each side of the tube
      
      ptOffset += 2*this->NumberOfSides*npts;
      singlePolyline->Delete();
      }

    }//for each polyline


  // Take care of capping
  if (this->Capping)
    {
    vtkIdType offset = newPts->GetNumberOfPoints();
    vtkIdType st, e;
    // Insert new points (different normals)
    vtkIdType num = capPoints->GetNumberOfPoints();
    float *tmp;
    
    for (i = 0; i < num; ++i)
      {
      tmp = capPoints->GetPoint(i);
      newPts->InsertNextPoint(tmp);
      tmp = capNormals->GetTuple(i);
      newNormals->InsertNextTuple(tmp);
      }
    // Now add the triangle strips.
    num = num / this->NumberOfSides;
    for (i = 0; i < num; )
      {
      newStrips->InsertNextCell(this->NumberOfSides);
      newStrips->InsertCellPoint(offset);
      newStrips->InsertCellPoint(offset+1);
      st = offset+2;
      e = offset + this->NumberOfSides - 1;
      while (st <= e)
        {
        newStrips->InsertCellPoint(e);
        if (e != st)
          {
          newStrips->InsertCellPoint(st);
          }
        ++st;
        --e;
        }
      offset += this->NumberOfSides;
      ++i;
      // ends have to have a different order than starts.
      newStrips->InsertNextCell(this->NumberOfSides);
      newStrips->InsertCellPoint(offset+1);
      newStrips->InsertCellPoint(offset);
      st = offset+2;
      e = offset + this->NumberOfSides - 1;
      while (st <= e)
        {
        newStrips->InsertCellPoint(st);
        if (e != st)
          {
          newStrips->InsertCellPoint(e);
          }
        ++st;
        --e;
        }
      offset += this->NumberOfSides;
      ++i;
      }
    }
  capPoints->Delete();
  capNormals->Delete();

  // Update ourselves
  //
  if ( deleteNormals )
    {
    inNormals->Delete();
    }

  output->SetPoints(newPts);
  newPts->Delete();

  output->SetStrips(newStrips);
  newStrips->Delete();

  outPD->SetNormals(newNormals);
  newNormals->Delete();
  lineNormalGenerator->Delete();

  output->Squeeze();
}

void vtkTubeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Vary Radius: " << this->GetVaryRadiusAsString() << endl;
  os << indent << "Radius Factor: " << this->RadiusFactor << "\n";
  os << indent << "Number Of Sides: " << this->NumberOfSides << "\n";
  os << indent << "On Ratio: " << this->OnRatio << "\n";
  os << indent << "Offset: " << this->Offset << "\n";

  os << indent << "Use Default Normal: " 
     << (this->UseDefaultNormal ? "On\n" : "Off\n");
  os << indent << "Sides Share Vertices: " 
     << (this->SidesShareVertices ? "On\n" : "Off\n");
  os << indent << "Default Normal: " << "( " << this->DefaultNormal[0] <<
     ", " << this->DefaultNormal[1] << ", " << this->DefaultNormal[2] <<
     " )\n";
  os << indent << "Capping: " << this->Capping << endl;
}
