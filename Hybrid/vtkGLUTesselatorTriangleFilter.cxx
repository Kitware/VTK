/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGLUTesselatorTriangleFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Tom Citriniti who implemented this class


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
#include "vtkGLUTesselatorTriangleFilter.h"
#include "vtkPolygon.h"
#include "vtkTriangleStrip.h"
#include "vtkObjectFactory.h"


// GLU support methods
static void _stdcall GluError(GLenum err) 
{
  const GLubyte* pByte = gluErrorString(err);
}
static void _stdcall GlBegin(GLenum mode, void * polygon_data);
static void _stdcall GlEnd(void * polygon_data);
static void _stdcall GlVertex3dv(void * vertex_data , void * polygon_data);
static void _stdcall GLCombineData(GLdouble coords[3], void *vertex_data[4], 
                                   GLfloat weight[4], void **outData, 
                                   void * polygon_data); 

//--------------------------------------------------------------------------
vtkGLUTesselatorTriangleFilter* vtkGLUTesselatorTriangleFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = 
    vtkObjectFactory::CreateInstance("vtkGLUTesselatorTriangleFilter");
  if(ret)
    {
    return (vtkGLUTesselatorTriangleFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkGLUTesselatorTriangleFilter;
}


vtkGLUTesselatorTriangleFilter::vtkGLUTesselatorTriangleFilter()
 : PassVerts(1), 
   PassLines(1) 
{
  GLUTesselator = gluNewTess ();
  if (GLUTesselator) 
    {
    gluTessCallback(GLUTesselator, GLU_TESS_BEGIN_DATA, 
                    (void (_stdcall *) ())&GlBegin);  
    gluTessCallback(GLUTesselator, GLU_TESS_VERTEX_DATA, 
                    (void (_stdcall *) ())&GlVertex3dv);  
    gluTessCallback(GLUTesselator, GLU_TESS_END_DATA, 
                    (void (_stdcall *) ())&GlEnd);
    gluTessCallback(GLUTesselator, GLU_TESS_COMBINE_DATA, 
                    (void (_stdcall *) ())&GLCombineData);
    gluTessCallback(GLUTesselator, GLU_ERROR, 
                    (void (_stdcall *) ())&GluError);
    }
}

vtkGLUTesselatorTriangleFilter::~vtkGLUTesselatorTriangleFilter() 
{
  gluDeleteTess(GLUTesselator);
}


void vtkGLUTesselatorTriangleFilter::Execute()
{
  vtkPolyData *input = this->GetInput();
  vtkIdType numCells=input->GetNumberOfCells(), cellNum, newId;
  int dim, i, j, pts[3], numPts, numSimplices, type;
  vtkIdList *ptIds=vtkIdList::New();
  vtkPoints *spts=vtkPoints::New();
  vtkPolyData *output=this->GetOutput();
  vtkCellData *inCD=input->GetCellData();
  vtkCellData *outCD=output->GetCellData();
  vtkCell *cell;
  int updateInterval;
  vtkIdType numPoints=input->GetNumberOfPoints();
  
  output->Allocate(numPoints, numPoints);
  outCD->CopyAllocate(inCD,numPoints);
  output->SetPoints(input->GetPoints());
  output->GetPointData()->PassData(input->GetPointData());

  updateInterval = (int)(numCells/100.0);
  updateInterval = ( updateInterval < 1 ? 1 : updateInterval );

  double dTol = 1.0;
  gluTessProperty(GLUTesselator, GLU_TESS_TOLERANCE, dTol);
  gluTessBeginPolygon (GLUTesselator, this);
  for (cellNum=0; cellNum < numCells; cellNum++)
    {
    if ( ! (cellNum % updateInterval) ) //manage progress reports / early abort
      {
      this->UpdateProgress ((float)cellNum / numCells);
      if ( this->GetAbortExecute() ) 
        {
        break;
        }
      }

    cell = input->GetCell(cellNum);
    dim = cell->GetCellDimension() + 1;

    // Run the GLU Tesselator on the cell
    if (cell->GetCellType() == VTK_POLYGON ||
        cell->GetCellType() == VTK_QUAD ||
        cell->GetCellType() == VTK_TRIANGLE ) 
      {
      //double *v = new double[numPoints*3];
      double v[3];
      if (cellNum == 0) 
        {
        gluTessBeginContour (GLUTesselator);
        }
      else 
        {
        gluNextContour(GLUTesselator, GLU_INTERIOR); 
        }
      for (int ii=0;ii<cell->PointIds->GetNumberOfIds();ii++) 
        {
        input->GetPoints()->GetPoint(cell->PointIds->GetId(ii), v);
        gluTessVertex (GLUTesselator, v, (void *)cell->PointIds->GetId(ii));
        }
      //delete [] v;
      //continue;
      }
    else 
      {
      numPts = cell->PointIds->GetNumberOfIds();
      numSimplices = numPts / dim;
      
      if ( dim == 3 || (this->PassVerts && dim == 1) ||
           (this->PassLines && dim == 2) )
                                
        {
        type = (dim == 3 ? VTK_TRIANGLE : (dim == 2 ? VTK_LINE : VTK_VERTEX ));
        for ( i=0; i < numSimplices; i++ )
          {
          for (j=0; j<dim; j++)
            {
            pts[j] = cell->PointIds->GetId(dim*i+j);
            }
          // copy cell data
          newId = output->InsertNextCell(type, dim, pts);
          outCD->CopyData(inCD, cellNum, newId);
          }//for each simplex
        }//if polygon or strip or (line or verts and passed on)
      }// If not tesselated.
    }//for all cells
  gluTessEndContour (GLUTesselator);
  gluTessEndPolygon (GLUTesselator);

  ptIds->Delete();
  spts->Delete();
  
  // Update output
  output->Squeeze();
  output->GetPolys()->Modified();

  vtkDebugMacro(<<"Converted " << input->GetNumberOfCells()
                << "input cells to "
                << output->GetNumberOfCells()
                <<" output cells");
}

void vtkGLUTesselatorTriangleFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Pass Verts: " << (this->PassVerts ? "On\n" : "Off\n");
  os << indent << "Pass Lines: " << (this->PassLines ? "On\n" : "Off\n");

}

// GLU support methods
static unsigned int iVCnt = 0;
static GLenum eMode;
void _stdcall GlBegin(GLenum mode , void * /*polygon_data*/)
{
  iVCnt = 0;
  eMode = mode;
}

void _stdcall GlEnd(void * /*polygon_data*/)
{
}

// We need to only output triangles
void _stdcall GlVertex3dv(void * vertex_data , void * polygon_data)
{
  vtkGLUTesselatorTriangleFilter *pThis = 
    (vtkGLUTesselatorTriangleFilter*)polygon_data;
  vtkCellArray *pPolys = pThis->GetOutput()->GetPolys();
  int iThisVertex = (int)vertex_data;
  static int iCurTri[3] = {0, 0, 0};
  static int idx[2][3]={{0,1,2},{1,0,2}};

  switch (eMode) 
    {
    case GL_TRIANGLE_FAN:
      if (iVCnt < 3) 
        {
        iCurTri[iVCnt] = iThisVertex;
        if (iVCnt == 2) 
          {
          pPolys->InsertNextCell(3);
          for (int ii=0;ii<3;ii++)
            {
            pPolys->InsertCellPoint(iCurTri[ii]);
            }
          }
        }
      else 
        {
        iCurTri[1] = iCurTri[2];
        iCurTri[2] = iThisVertex;
        pPolys->InsertNextCell(3);
        for (int ii=0;ii<3;ii++) 
          {
          pPolys->InsertCellPoint(iCurTri[ii]);
          }
        }
      break;
    case GL_TRIANGLE_STRIP:
      if (iVCnt < 3) 
        {
        iCurTri[iVCnt] = iThisVertex;
        if (iVCnt == 2) 
          {
          pPolys->InsertNextCell(3);
          for (int ii=0;ii<3;ii++) 
            {
            pPolys->InsertCellPoint(iCurTri[ii]);
            }
          }
        }
      else 
        {
        iCurTri[0] = iCurTri[1];
        iCurTri[1] = iCurTri[2];
        iCurTri[2] = iThisVertex;
        int order = iVCnt%2;
        pPolys->InsertNextCell(3);
        for (int ii=0;ii<3;ii++) 
          {
          pPolys->InsertCellPoint(iCurTri[idx[order][ii]]);
          }
        }
      break;
    case GL_TRIANGLES:
      if ((iVCnt % 3) == 0) 
        {
        pPolys->InsertNextCell(3);
        }
      pPolys->InsertCellPoint(iThisVertex);
      break;
    }
  iVCnt++;
}

// Combine verticies
static void _stdcall GLCombineData(GLdouble coords[3], void *vertex_data[4], GLfloat weight[4], void **outData, void * polygon_data)
{
  vtkGLUTesselatorTriangleFilter *pThis = 
    (vtkGLUTesselatorTriangleFilter*)polygon_data;
  *outData = (void*)pThis->GetOutput()->GetPoints()->InsertNextPoint(coords);
}
