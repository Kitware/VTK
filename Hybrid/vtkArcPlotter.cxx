/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArcPlotter.cxx
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
#include "vtkArcPlotter.h"
#include "vtkMath.h"
#include "vtkPlane.h"

vtkArcPlotter::vtkArcPlotter()
{
  this->Camera = NULL;
  this->PlotMode = VTK_PLOT_SCALARS;
  this->PlotComponent = (-1); //plot all components
  this->Radius = 0.5;
  this->Height = 0.5;
  this->Offset = 0.0;
  
  this->DefaultNormal[0] = this->DefaultNormal[1] = 0.0;
  this->DefaultNormal[2] = 1.0;
  this->UseDefaultNormal = 0;
  
  this->FieldDataArray = 0;

  this->DataRange = NULL;
  this->Tuple = NULL;
  this->ActiveComponent = 0;
  this->NumberOfComponents = 0;
}

vtkArcPlotter::~vtkArcPlotter()
{
  if ( this->DataRange )
    {
    delete [] this->DataRange;
    delete [] this->Tuple;
    }
  if ( this->Camera )
    {
    this->Camera->UnRegister (this);
    this->Camera = NULL;
    }
}

void vtkArcPlotter::Execute()
{
  vtkPolyData *input=this->GetInput();
  vtkPolyData *output=this->GetOutput();
  vtkPointData *inPD=input->GetPointData();
  vtkPoints *inPts;
  vtkCellArray *inLines;
  int numPts, i, j;
  float x[3], normal[3], point[3], aveNormal[3];
  int id;
  vtkIdType *pts, npts;
  float *x1, *x2, x21[3], n[3];
  vtkNormals *lineNormals;
  vtkPoints *newPts;
  vtkCellArray *newLines;
  float *range, offset;
  int plotNum, compNum;
  vtkPoints *projPts;
  
  // Initialize
  //
  vtkDebugMacro(<<"Plotting along arc");

  if ( !(inPts=input->GetPoints()) || 
  (numPts=inPts->GetNumberOfPoints()) < 1 ||
  !(inLines=input->GetLines()) || inLines->GetNumberOfCells() < 1 )
    {
    vtkErrorMacro(<< "No input data!");
    return;
    }

  // Process attribute data to determine ranges, cumber of components, etc.
  if ( this->ProcessComponents(numPts, inPD) <= 0 )
    {
    return;
    }

  // Allocate points for projection
  //
  // Determine the projection plane. Project to a plane if camera is available
  // and defulat normal is not desired.
  if ( this->Camera && ! this->UseDefaultNormal )
    {
    float xProj[3];
    projPts = vtkPoints::New();
    projPts->SetNumberOfPoints(numPts);
    this->Camera->GetViewPlaneNormal(normal);
    this->Camera->GetFocalPoint(point);
    vtkMath::Normalize(normal);
    for ( i=0; i < numPts; i++ )
      {
      inPts->GetPoint(i, x);
      vtkPlane::ProjectPoint(x, point, normal, xProj);
      projPts->SetPoint(i,xProj);
      }
    }
  else
    {
    normal[0] = this->DefaultNormal[0];
    normal[1] = this->DefaultNormal[1];
    normal[2] = this->DefaultNormal[2];
    vtkMath::Normalize(normal);
    projPts = inPts; //use existing points
    }

  // For each polyline, compute a normal that lies in the 
  // projection plane and is roughly perpendicular to the projected
  // polyline. Then generate the arc.
  //
  newPts = vtkPoints::New();
  newPts->Allocate(numPts,numPts);
  lineNormals = vtkNormals::New();
 
  newLines = vtkCellArray::New();
  newLines->Allocate(inLines->GetSize());

  for (inLines->InitTraversal(); inLines->GetNextCell(npts,pts); )
    {
    lineNormals->SetNumberOfNormals(npts);
    if ( !this->Camera || this->UseDefaultNormal )
      {//use default normal
      for (i=0; i < npts; i++)
        {
        lineNormals->SetNormal(i,normal);
        }
      }
    else //generate normals
      {
      // Compute normals on each line segment perpendicular to view normal
      for (i=0; i < (npts-1); i++)
        {
        x1 = projPts->GetPoint(pts[i]);
        x2 = projPts->GetPoint(pts[i+1]);
        for (j=0; j<3; j++)
          {
          x21[j] = x2[j] - x1[j];
          }
        vtkMath::Cross(normal,x21,n);
        vtkMath::Normalize(n);
        lineNormals->SetNormal(i,n);
        }
      lineNormals->SetNormal(npts-1,n);
      }
    
    // Now average the normal calculation to get smoother results
    //
    int window = npts / 100;
    if ( window < 5 )
      {
      window = 5;
      }
    // Start by computing an initial average normal
    aveNormal[0] = aveNormal[1] = aveNormal[2] = 0.0;
    for (i=0; i < npts && i < window; i++)
      {
      lineNormals->GetNormal(i,n);
      aveNormal[0] += n[0]; aveNormal[1] += n[1]; aveNormal[2] += n[2];
      }

    for (i=0; i < npts; i++)
      {
      if ( (i+window) < npts )
        {
        lineNormals->GetNormal(i+window,n);
        aveNormal[0] += n[0]; aveNormal[1] += n[1]; aveNormal[2] += n[2];
        }
      if ( (i-window) >= 0 )
        {
        lineNormals->GetNormal(i-window,n);
        aveNormal[0] -= n[0]; aveNormal[1] -= n[1]; aveNormal[2] -= n[2];
        }
      n[0] = aveNormal[0]; n[1] = aveNormal[1]; n[2] = aveNormal[2];
      vtkMath::Normalize(n);
      lineNormals->SetNormal(i, n);
      }
    this->UpdateProgress(0.50);

    // For each component, create an offset plot.
    for (plotNum=0, compNum=this->StartComp; compNum <= this->EndComp; 
         compNum++, plotNum++)
      {
      offset = this->Radius + plotNum*this->Offset;
      range = this->DataRange + 2*compNum;

      newLines->InsertNextCell(npts);

      // Continue average normal computation using sliding window
      for (i=0; i < npts; i++)
        {
        this->Data->GetTuple(pts[i], this->Tuple);
        lineNormals->GetNormal(i,n);
        id = this->OffsetPoint(pts[i], inPts, n, newPts, 
                               offset, range, this->Tuple[compNum]);
        newLines->InsertCellPoint(id);
        }
      } //for all components
    } //for all polylines
  this->UpdateProgress(0.90);
  
  lineNormals->Delete();
  if ( projPts != inPts )
    {
    projPts->Delete();
    }

  // Update output
  output->SetPoints(newPts);
  newPts->Delete();
  output->SetLines(newLines);
  newLines->Delete();
}

int vtkArcPlotter::ProcessComponents(int numPts, vtkPointData *pd)
{
  int i, j;
  float *range;
  vtkFieldData *fd;
  
  this->Data = NULL;
  switch (this->PlotMode)
    {
    case VTK_PLOT_SCALARS:
      if ( pd->GetScalars() )
        {
        this->Data = pd->GetScalars()->GetData();
        }
      break;
    case VTK_PLOT_VECTORS:   
      if ( pd->GetVectors() )
        {
        this->Data = pd->GetVectors()->GetData();
        }
      break;
    case VTK_PLOT_NORMALS:    
      if ( pd->GetNormals() )
        {
        this->Data = pd->GetNormals()->GetData();
        }
      break;
    case VTK_PLOT_TCOORDS:    
      if ( pd->GetTCoords() )
        {
        this->Data = pd->GetTCoords()->GetData();
        }
      break;
    case VTK_PLOT_TENSORS:    
      if ( pd->GetTensors() )
        {
        this->Data = pd->GetTensors()->GetData();
        }
      break;
    case VTK_PLOT_FIELD_DATA:
      if ( (fd=pd->GetFieldData()) )
        {
        int arrayNum = (this->FieldDataArray < fd->GetNumberOfArrays() ?
                    this->FieldDataArray : fd->GetNumberOfArrays() - 1);
        this->Data = pd->GetFieldData()->GetArray(arrayNum);
        }
      break;
    }

  // Determine the number of components
  if ( this->Data )
    {
    this->NumberOfComponents = this->Data->GetNumberOfComponents();
    if ( this->PlotComponent >= 0 )
      {
      this->ActiveComponent = (this->PlotComponent < this->NumberOfComponents ?
                           this->PlotComponent : this->NumberOfComponents - 1);
      this->StartComp = this->EndComp = this->ActiveComponent;
      } 
    else 
      {
      this->StartComp = 0;
      this->EndComp = this->NumberOfComponents - 1;
      }
    }
  else
    {
    vtkErrorMacro(<<"Need input data to plot");
    return 0;
    }
  
  // Get the range of the components (for scaling the plot later)
  if ( this->DataRange )
    {
    delete [] this->DataRange;
    delete [] this->Tuple;
    }
  
  this->DataRange = new float [2*this->NumberOfComponents];
  this->Tuple = new float [this->NumberOfComponents];

  for (i=this->StartComp; i <= this->EndComp; i++)
    {
    range = this->DataRange + 2*i;
    range[0] =  VTK_LARGE_FLOAT;
    range[1] =  -VTK_LARGE_FLOAT;
    }
  
  for (i=0; i<numPts; i++)
    {
    this->Data->GetTuple(i, this->Tuple);

    for (j=this->StartComp; j <= this->EndComp; j++)
      {
      range = this->DataRange + 2*j;
      if ( this->Tuple[j] < range[0] )
        {
        range[0] = this->Tuple[j];
        }
      if ( this->Tuple[j] > range[1] )
        {
        range[1] = this->Tuple[j];
        }
      }
    }

  return this->NumberOfComponents;
}


int  vtkArcPlotter::OffsetPoint(int ptId, vtkPoints *inPts, float n[3], 
                    vtkPoints *newPts, float offset, float *range, float v)
{
  float x[3], xNew[3];
  int i;
  float median = (range[0] + range[1])/2.0;
  float denom = range[1] - range[0];
  
  inPts->GetPoint(ptId, x);
  for (i=0; i<3; i++)
    {
    xNew[i] = x[i] + n[i] * (offset + ((v - median)/denom)*this->Height);
    }

  return newPts->InsertNextPoint(xNew);
}

unsigned long vtkArcPlotter::GetMTime()
{
  unsigned long mTime=this->vtkPolyDataToPolyDataFilter::GetMTime();
  unsigned long cameraMTime;

  if ( this->Camera && ! this->UseDefaultNormal )
    {
    cameraMTime = this->Camera->GetMTime();
    return (cameraMTime > mTime ? cameraMTime : mTime);
    }
  else
    {
    return mTime;
    }
}

void vtkArcPlotter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  if ( this->Camera )
    {
    os << indent << "Camera:\n";
    this->Camera->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Camera: (none)\n";
    }
  
  os << indent << "Plot Mode: ";
  if ( this->PlotMode == VTK_PLOT_SCALARS )
    {
    os << "Plot Scalars\n";
    }
  else if ( this->PlotMode == VTK_PLOT_VECTORS )
    {
    os << "Plot Vectors\n";
    }
  else if ( this->PlotMode == VTK_PLOT_NORMALS )
    {
    os << "Plot Normals\n";
    }
  else if ( this->PlotMode == VTK_PLOT_TCOORDS )
    {
    os << "Plot TCoords\n";
    }
  else if ( this->PlotMode == VTK_PLOT_TENSORS )
    {
    os << "Plot Tensors\n";
    }
  else
    {
    os << "Plot Field Data\n";
    }

  os << indent << "Plot Component: ";
  if ( this->PlotComponent < 0 )
    {
    os << "(All Components)\n";
    }
  else
    {
    os << this->PlotComponent << "\n";
    }

  os << indent << "Field Data Array: " << this->FieldDataArray << "\n";

  os << indent << "Use Default Normal: " 
     << (this->UseDefaultNormal ? "On\n" : "Off\n");
  os << indent << "Default Normal: " << "( " << this->DefaultNormal[0] 
     << ", " << this->DefaultNormal[1] << ", " << this->DefaultNormal[2] 
     << " )\n";
  
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Height: " << this->Height << "\n";
  os << indent << "Offset: " << this->Offset << "\n";
}
