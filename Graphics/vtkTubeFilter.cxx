/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTubeFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkTubeFilter.h"
#include "vtkPolyLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

//--------------------------------------------------------------------------
vtkTubeFilter* vtkTubeFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkTubeFilter");
  if(ret)
    {
    return (vtkTubeFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkTubeFilter;
}

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
  vtkNormals *inNormals;
  vtkDataArray *inScalars=NULL;
  vtkVectors *inVectors=NULL;
  vtkIdType numPts = 0;
  vtkIdType numNewPts;
  vtkPoints *newPts;
  vtkNormals *newNormals;
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
  vtkNormals *capNormals = vtkNormals::New();
  float capNorm[3];
  int capPointFlag;

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
    inNormals = vtkNormals::New();
    inNormals->SetNumberOfNormals(numPts);

    if ( this->UseDefaultNormal )
      {
      for ( i=0; i < numPts; i++)
        {
        inNormals->SetNormal(i,this->DefaultNormal);
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
  (inScalars=pd->GetActiveScalars()) )
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
  newNormals = vtkNormals::New();
  newNormals->Allocate(numNewPts);
  newStrips = vtkCellArray::New();
  newStrips->Allocate(newStrips->EstimateSize(1,numNewPts));

  //  Create points along the line that are later connected into a 
  //  triangle strip.
  //
  lineNormalGenerator = vtkPolyLine::New();
  for (inLines->InitTraversal(); inLines->GetNextCell(npts,pts); )
    {
    // if necessary calculate normals, each polyline calculates it's
    // normals independently, avoiding conflicts at shared vertices
    vtkCellArray *singlePolyline = vtkCellArray::New();
    if (generate_normals) 
      {
      singlePolyline->InsertNextCell( npts, pts );
    
      if ( !lineNormalGenerator->GenerateSlidingNormals(inPts,singlePolyline,
                                          (vtkNormals*)inNormals) )
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

      n = inNormals->GetNormal(pts[j]);

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
          sqrt((double)maxSpeed/vtkMath::Norm(inVectors->GetVector(pts[j])));
        if ( sFactor > this->RadiusFactor )
          {
          sFactor = this->RadiusFactor;
          }
        }

      //create points around line
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
          capNormals->InsertNextNormal(capNorm);
          }
        ptId = newPts->InsertNextPoint(s);
        newNormals->InsertNormal(ptId,normal);
        outPD->CopyData(pd,pts[j],ptId);
        }
      }//for all points in polyline

    // Generate the strips
    //
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
      tmp = capNormals->GetNormal(i);
      newNormals->InsertNextNormal(tmp);
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
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Vary Radius: " << this->GetVaryRadiusAsString() << endl;
  os << indent << "Radius Factor: " << this->RadiusFactor << "\n";
  os << indent << "Number Of Sides: " << this->NumberOfSides << "\n";
  os << indent << "On Ratio: " << this->OnRatio << "\n";
  os << indent << "Offset: " << this->Offset << "\n";

  os << indent << "Use Default Normal: " 
     << (this->UseDefaultNormal ? "On\n" : "Off\n");
  os << indent << "Default Normal: " << "( " << this->DefaultNormal[0] <<
     ", " << this->DefaultNormal[1] << ", " << this->DefaultNormal[2] <<
     " )\n";
  os << indent << "Capping: " << this->Capping << endl;
}
