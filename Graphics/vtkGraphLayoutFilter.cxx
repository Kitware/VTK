/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphLayoutFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Scott Hill of RPI for developing this class
             Mark Lacy for Procter & Gamble for support


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
#include "vtkGraphLayoutFilter.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

//--------------------------------------------------------------------------
vtkGraphLayoutFilter* vtkGraphLayoutFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkGraphLayoutFilter");
  if(ret)
    {
    return (vtkGraphLayoutFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkGraphLayoutFilter;
}

vtkGraphLayoutFilter::vtkGraphLayoutFilter()
{
  this->GraphBounds[0] = this->GraphBounds[2] = this->GraphBounds[4] = -0.5;
  this->GraphBounds[1] = this->GraphBounds[3] = this->GraphBounds[5] =  0.5;
  this->MaxNumberOfIterations = 100;
  this->CoolDownRate = 10.0;
  this->AutomaticBoundsComputation = 1;
  this->ThreeDimensionalLayout = 1;
  this->BoundsFactor = 0.1;
}

// A vertex contains a position and a displacement.
typedef struct _vtkLayoutVertex
{
  float x[3];
  float d[3];
} vtkLayoutVertex;

// An edge consists of two vertices joined together.
// This struct acts as a "pointer" to those two vertices.
typedef struct _vtkLayoutEdge
{
  int t;
  int u;
} vtkLayoutEdge;

// Cool-down function.
static inline float CoolDown(float t, float r) 
{ 
  return t-(t/r); 
}

static inline float forceAttract(float x, float k) 
{ 
  return (x * x) / k; 
}

static inline float forceRepulse(float x, float k)
{
  if (x != 0.0)
    {
    return k * k / x;
    }
  else
    {
    return VTK_LARGE_FLOAT;
    }
}

// Returns the minimum of the absolute value of the three numbers x, y, and z.
static float forceMin(float x, float y, float z)
{
  x = fabs(x);
  y = fabs(y);
  z = fabs(z);
  if(x>y)
    {
    if(y>z) {return z;}
    else {return y;}
    }
  else if(x>z)
    {
    return z;
    }
  else
    {
    return x;
    }
}

void vtkGraphLayoutFilter::Execute()
{
  vtkPolyData *input = (vtkPolyData *)this->GetInput();
  vtkPoints *pts = input->GetPoints();
  vtkCellArray *lines = input->GetLines();
  int numLines = lines->GetNumberOfCells();  //Number of lines/edges.
  vtkPolyData *output = (vtkPolyData *)this->GetOutput();
  int numPts = input->GetNumberOfPoints();  //Number of points/vertices.
  float diff[3], len;  //The difference vector.
  int i, j, l;  //Iteration variables.
  vtkIdType npts, *cellPts;
  float fa, fr, minimum;

  vtkDebugMacro(<<"Drawing graph");

  if ( numPts <= 0 || numLines <= 0)
    {                   
    vtkErrorMacro(<<"No input");
    return;
    }

  // Generate bounds automatically if necessary.
  if ( this->AutomaticBoundsComputation )
    {
    pts->GetBounds(this->GraphBounds);
    }

  for (i=0; i<3; i++)
    {
    if ( this->GraphBounds[2*i+1] <= this->GraphBounds[2*i] ) 
      {
      this->GraphBounds[2*i+1] = this->GraphBounds[2*i] + 1;
      }
    }
  
  if ( this->AutomaticBoundsComputation )
    {
    for (i=0; i<3; i++)
      {
      len = this->GraphBounds[2*i+1] - this->GraphBounds[2*i];
      this->GraphBounds[2*i] = this->GraphBounds[2*i] - len*this->BoundsFactor;
      this->GraphBounds[2*i+1] = 
        this->GraphBounds[2*i+1] + len*this->BoundsFactor;
      }
    }

  // Allocate memory for structures. Break polylines into line segments.
  numLines=0;
  for (i=0, lines->InitTraversal(); lines->GetNextCell(npts, cellPts); i++)
    {
    numLines += npts - 1;
    }
    
  vtkLayoutVertex *v = new vtkLayoutVertex [numPts];
  vtkLayoutEdge *e = new vtkLayoutEdge [numLines];
        
  // Get the points, either x,y,0 or x,y,z
  for (i=0; i<numPts; i++)
    {
    pts->GetPoint(i,v[i].x);
    if ( ! this->ThreeDimensionalLayout )
      {
      v[i].x[2] = 0;
      }
    }

  // Get the edges
  numLines=0;
  for (i=0, lines->InitTraversal(); lines->GetNextCell(npts, cellPts); i++)
    {
    for (j=0; j<npts-1; j++)
      {
      e[numLines].t = cellPts[j];
      e[numLines++].u = cellPts[j+1];
      }
    }

  // More variable definitions:
  float volume = (this->GraphBounds[1] - this->GraphBounds[0]) *
    (this->GraphBounds[3] - this->GraphBounds[2]) *
    (this->GraphBounds[5] - this->GraphBounds[4]);
  float temp = 1000.0;
  // The optimal distance between vertices.
  float k = pow((double)volume/numPts,0.33333);

  // Begin iterations.
  float norm;
  for(i=0; i<this->MaxNumberOfIterations; i++)
    {
    // Calculate the repulsive forces.
    for(j=0; j<numPts; j++)
      {
      v[j].d[0] = 0.0;
      v[j].d[1] = 0.0;
      v[j].d[2] = 0.0;
      for(l=0; l<numPts; l++)
        {
        if (j != l)
          {
          diff[0] = v[j].x[0] - v[l].x[0];
          diff[1] = v[j].x[1] - v[l].x[1];
          diff[2] = v[j].x[2] - v[l].x[2];
          norm = vtkMath::Normalize(diff);
          fr = forceRepulse(norm,k);
          v[j].d[0] += diff[0] * fr;
          v[j].d[1] += diff[1] * fr;
          v[j].d[2] += diff[2] * fr;
          }
        }
      }

    // Calculate the attractive forces.
    for (j=0; j<numLines; j++)
      {
      diff[0] = v[e[j].u].x[0] - v[e[j].t].x[0];
      diff[1] = v[e[j].u].x[1] - v[e[j].t].x[1];
      diff[2] = v[e[j].u].x[2] - v[e[j].t].x[2];
      norm = vtkMath::Normalize(diff);
      fa = forceAttract(norm,k);
      v[e[j].u].d[0] -= diff[0] * fa;
      v[e[j].u].d[1] -= diff[1] * fa;
      v[e[j].u].d[2] -= diff[2] * fa;
      v[e[j].t].d[0] += diff[0] * fa;
      v[e[j].t].d[1] += diff[1] * fa;
      v[e[j].t].d[2] += diff[2] * fa;
      }

    // Combine the forces for a new configuration
    for (j=0; j<numPts; j++)
      {
      norm = vtkMath::Normalize(v[j].d);
      minimum = (norm < temp ? norm : temp);
      v[j].x[0] += v[j].d[0] * minimum;
      v[j].x[1] += v[j].d[1] * minimum;
      v[j].x[2] += v[j].d[2] * minimum;
      }

    // Reduce temperature as layout approaches a better configuration.
    temp = CoolDown(temp, this->CoolDownRate);
    }

  // Get the bounds of the graph and scale and translate to
  // bring them within the bounds specified.
  vtkPoints *newPts = vtkPoints::New();
  newPts->SetNumberOfPoints(numPts);
  for (i=0; i<numPts; i++)
    {
    newPts->SetPoint(i,v[i].x);
    }
  float bounds[6], sf[3], translation[3], *x, xNew[3];
  newPts->GetBounds(bounds);
  for (i=0; i<3; i++)
    {
    sf[i] = (bounds[2*i+1] - bounds[2*i]) /
      (this->GraphBounds[2*i+1] - this->GraphBounds[2*i]);
    translation[i] = (bounds[2*i+1] + bounds[2*i])/2.0 -
      (this->GraphBounds[2*i+1] + this->GraphBounds[2*i])/2.0;
    }
  for (i=0; i<numPts; i++)
    {
    x = newPts->GetPoint(i);
    for (j=0; j<3; j++)
      {
      xNew[j] = x[j];
      }
    newPts->SetPoint(i,xNew);
    }

  // Send the data to output.
  output->SetPoints(newPts);
  output->SetLines(lines);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  // Clean up.
  newPts->Delete();
  delete [] v;
  delete [] e;
}

void vtkGraphLayoutFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "AutomaticBoundsComputation: " 
     << (this->AutomaticBoundsComputation ? "On\n" : "Off\n");

  os << indent << "GraphBounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->GraphBounds[0] << ", " 
     << this->GraphBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->GraphBounds[2] << ", " 
     << this->GraphBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->GraphBounds[4] << ", " 
     << this->GraphBounds[5] << ")\n";

  os << indent << "BoundsFactor: " 
     << this->BoundsFactor << endl;

  os << indent << "MaxNumberOfIterations: " 
     << this->MaxNumberOfIterations << endl;

  os << indent << "CoolDownRate: " 
     << this->CoolDownRate << endl;

  os << indent << "Three Dimensional Layout: " 
     << (this->ThreeDimensionalLayout ? "On\n" : "Off\n");

}
