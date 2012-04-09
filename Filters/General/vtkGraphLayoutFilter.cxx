/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphLayoutFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGraphLayoutFilter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkGraphLayoutFilter);

vtkGraphLayoutFilter::vtkGraphLayoutFilter()
{
  this->GraphBounds[0] = this->GraphBounds[2] = this->GraphBounds[4] = -0.5;
  this->GraphBounds[1] = this->GraphBounds[3] = this->GraphBounds[5] =  0.5;
  this->MaxNumberOfIterations = 50;
  this->CoolDownRate = 10.0;
  this->AutomaticBoundsComputation = 1;
  this->ThreeDimensionalLayout = 1;
}

// A vertex contains a position and a displacement.
typedef struct _vtkLayoutVertex
{
  double x[3];
  double d[3];
} vtkLayoutVertex;

// An edge consists of two vertices joined together.
// This struct acts as a "pointer" to those two vertices.
typedef struct _vtkLayoutEdge
{
  int t;
  int u;
} vtkLayoutEdge;

// Cool-down function.
static inline double CoolDown(double t, double r)
{
  return t-(t/r);
}

static inline double forceAttract(double x, double k)
{
  return (x * x) / k;
}

static inline double forceRepulse(double x, double k)
{
  if (x != 0.0)
    {
    return k * k / x;
    }
  else
    {
    return VTK_DOUBLE_MAX;
    }
}

int vtkGraphLayoutFilter::RequestData(
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

  vtkPoints *pts = input->GetPoints();
  vtkCellArray *lines = input->GetLines();
  int numLines = lines->GetNumberOfCells();  //Number of lines/edges.
  int numPts = input->GetNumberOfPoints();  //Number of points/vertices.
  double diff[3], len;  //The difference vector.
  int i, j, l;  //Iteration variables.
  vtkIdType npts = 0;
  vtkIdType *cellPts = 0;
  double fa, fr, minimum;

  vtkDebugMacro(<<"Drawing graph");

  if ( numPts <= 0 || numLines <= 0)
    {
    vtkErrorMacro(<<"No input");
    return 1;
    }

  // Generate bounds automatically if necessary. It's the same
  // as the input bounds.
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
  double volume = (this->GraphBounds[1] - this->GraphBounds[0]) *
    (this->GraphBounds[3] - this->GraphBounds[2]) *
    (this->GraphBounds[5] - this->GraphBounds[4]);
  double temp = sqrt( (this->GraphBounds[1]-this->GraphBounds[0])*
                     (this->GraphBounds[1]-this->GraphBounds[0]) +
                     (this->GraphBounds[3]-this->GraphBounds[2])*
                     (this->GraphBounds[3]-this->GraphBounds[2]) +
                     (this->GraphBounds[5]-this->GraphBounds[4])*
                     (this->GraphBounds[5]-this->GraphBounds[4]) );
  // The optimal distance between vertices.
  double k = pow(volume/numPts,0.33333);

  // Begin iterations.
  double norm;
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
  double bounds[6], sf[3], x[3], xNew[3];
  double center[3], graphCenter[3];
  newPts->GetBounds(bounds);
  for (i=0; i<3; i++)
    {
    if ( (len=(bounds[2*i+1] - bounds[2*i])) == 0.0 )
      {
      len = 1.0;
      }
    sf[i] = (this->GraphBounds[2*i+1] - this->GraphBounds[2*i]) / len;
    center[i] = (bounds[2*i+1] + bounds[2*i])/2.0;
    graphCenter[i] = (this->GraphBounds[2*i+1] + this->GraphBounds[2*i])/2.0;
    }

  double scale = sf[0];
  scale = (scale < sf[1] ? scale : sf[1]);
  scale = (scale < sf[2] ? scale : sf[2]);

  for (i=0; i<numPts; i++)
    {
    newPts->GetPoint(i, x);
    for (j=0; j<3; j++)
      {
      xNew[j] = graphCenter[j] + scale*(x[j]-center[j]);
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

  return 1;
}

void vtkGraphLayoutFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "AutomaticBoundsComputation: "
     << (this->AutomaticBoundsComputation ? "On\n" : "Off\n");

  os << indent << "GraphBounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->GraphBounds[0] << ", "
     << this->GraphBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->GraphBounds[2] << ", "
     << this->GraphBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->GraphBounds[4] << ", "
     << this->GraphBounds[5] << ")\n";

  os << indent << "MaxNumberOfIterations: "
     << this->MaxNumberOfIterations << endl;

  os << indent << "CoolDownRate: "
     << this->CoolDownRate << endl;

  os << indent << "Three Dimensional Layout: "
     << (this->ThreeDimensionalLayout ? "On\n" : "Off\n");
}
