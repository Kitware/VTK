/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPOVExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkPOVExporter.cxx

Copyright (c) 2007, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007. Los Alamos National Security, LLC. 
This software was produced under U.S. Government contract DE-AC52-06NA25396 
for Los Alamos National Laboratory (LANL), which is operated by 
Los Alamos National Security, LLC for the U.S. Department of Energy. 
The U.S. Government has rights to use, reproduce, and distribute this software. 
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.  
If software is modified to produce derivative works, such modified software 
should be clearly marked, so as not to confuse it with the version available 
from LANL.
 
Additionally, redistribution and use in source and binary forms, with or 
without modification, are permitted provided that the following conditions 
are met:
-   Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer. 
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software 
    without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkPOVExporter.h"

#include "vtkAssemblyPath.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkGeometryFilter.h"
#include "vtkProperty.h"
#include "vtkTexture.h"
#include "vtkMapper.h"
#include "vtkMatrix4x4.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkTypeTraits.h"
#include <vtksys/ios/sstream>

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPOVExporter);

//Can't use printf("%d", a_vtkIdType) because vtkIdType is not always int.
//This internal class holds format strings vtkPOVExporter can use instead.
class vtkPOVInternals
{
public:
  vtkPOVInternals()
  {
    this->CountFormat = new char[100]; //"\t\t%d,\n"
    strcpy(this->CountFormat, "\t\t");
    strcat(this->CountFormat, vtkTypeTraits<vtkIdType>::ParseFormat());
    strcat(this->CountFormat, ",\n");

    char *triFormat = new char[100]; //"%d, %d, %d"
    strcpy(triFormat, vtkTypeTraits<vtkIdType>::ParseFormat());
    strcat(triFormat, ", ");
    strcat(triFormat, vtkTypeTraits<vtkIdType>::ParseFormat());
    strcat(triFormat, ", ");
    strcat(triFormat, vtkTypeTraits<vtkIdType>::ParseFormat());  

    this->TriangleFormat1 = new char[100]; //"\t\t<%d, %d, %d>,"
    strcpy(this->TriangleFormat1, "\t\t<");
    strcat(this->TriangleFormat1, triFormat);
    strcat(this->TriangleFormat1, ">,");

    this->TriangleFormat2 = new char[100]; //" %d, %d, %d,\n"
    strcpy(this->TriangleFormat2, " ");
    strcat(this->TriangleFormat2, triFormat);
    strcat(this->TriangleFormat2, ",\n");  

    delete[] triFormat;
  }

  ~vtkPOVInternals()
  {
    delete[] this->CountFormat;
    delete[] this->TriangleFormat1;
    delete[] this->TriangleFormat2;
  }

  char *CountFormat;
  char *TriangleFormat1;
  char *TriangleFormat2;
};

#define VTKPOV_CNTFMT this->Internals->CountFormat
#define VTKPOV_TRIFMT1 this->Internals->TriangleFormat1
#define VTKPOV_TRIFMT2 this->Internals->TriangleFormat2

//============================================================================
vtkPOVExporter::vtkPOVExporter()
{
  this->FileName = NULL;
  this->FilePtr = NULL;
  this->Internals = new vtkPOVInternals;
}

vtkPOVExporter::~vtkPOVExporter()
{
  if(this->FileName!=0)
    {
    delete[] this->FileName;
    }
  delete this->Internals;
}

void vtkPOVExporter::WriteData()
{
  // make sure user specified a filename
  if (this->FileName == NULL)
    {
    vtkErrorMacro(<< "Please specify file name to create");
    return;
    }

  //get the renderer
  vtkRenderer *renderer = 
    this->RenderWindow->GetRenderers()->GetFirstRenderer();
  // make sure it has at least one actor
  if (renderer->GetActors()->GetNumberOfItems() < 1) 
    {
    vtkErrorMacro(<< "no actors found for writing .pov file.");
    return;
    }
    
  // try opening the file
  this->FilePtr = fopen(this->FileName, "w");
  if (this->FilePtr == NULL) 
    {
    vtkErrorMacro (<< "Cannot open " << this->FileName);
    return;
    }

    
  // write header
  this->WriteHeader(renderer);
  
  // write camera
  this->WriteCamera(renderer->GetActiveCamera());
  
  // write lights
  vtkLightCollection *lc = renderer->GetLights();
  vtkCollectionSimpleIterator sit;
  lc->InitTraversal(sit);
  if (lc->GetNextLight(sit) == NULL) 
    {
    vtkWarningMacro(<< "No light defined, creating one at camera position");
    renderer->CreateLight();
    }
  vtkLight *light;
  for (lc->InitTraversal(sit); (light = lc->GetNextLight(sit));) 
    {
    if (light->GetSwitch())
      {
      this->WriteLight(light);
      }
    }
  
  // write actors
  vtkActorCollection *ac = renderer->GetActors();
  vtkAssemblyPath *apath;
  vtkCollectionSimpleIterator ait;
  vtkActor *anActor, *aPart;
  for (ac->InitTraversal(ait); (anActor = ac->GetNextActor(ait)); ) 
    {
    for (anActor->InitPathTraversal(); (apath = anActor->GetNextPath()); ) 
      {
      aPart = static_cast<vtkActor *>(apath->GetLastNode()->GetViewProp());
      this->WriteActor(aPart);
      }
    }
  
  fclose(this->FilePtr);
}

void vtkPOVExporter::WriteHeader(vtkRenderer *renderer)
{
  fprintf(this->FilePtr, "// POVRay file exported by vtkPOVExporter\n");
  fprintf(this->FilePtr, "//\n");
  
  // width and height of output image, 
  //and other default command line args to POVRay
  int *size = renderer->GetSize();
  fprintf(this->FilePtr, "// +W%d +H%d\n\n", size[0], size[1]);
  
  // global settings
  fprintf(this->FilePtr, "global_settings {\n");
  fprintf(this->FilePtr, "\tambient_light color rgb <1.0, 1.0, 1.0>\n");
  fprintf(this->FilePtr, "\tassumed_gamma 2\n");
  fprintf(this->FilePtr, "}\n\n");
  
  // background
  double *color = renderer->GetBackground();
  fprintf(this->FilePtr, "background { color rgb <%f, %f, %f>}\n\n", 
          color[0], color[1], color[2]); 
}

void vtkPOVExporter::WriteCamera(vtkCamera *camera)
{
  fprintf(this->FilePtr, "camera {\n");
  if (camera->GetParallelProjection()) 
    {
    fprintf(this->FilePtr, "\torthographic\n");
    } 
  else 
    {
    fprintf(this->FilePtr, "\tperspective\n");
    }
  
  double *position = camera->GetPosition();
  fprintf(this->FilePtr, "\tlocation <%f, %f, %f>\n", 
          position[0], position[1], position[2]);
  
  double *up = camera->GetViewUp();
  // the camera up vector is called "sky" in POVRay
  fprintf(this->FilePtr, "\tsky <%f, %f, %f>\n", up[0], up[1], up[2]);
  
  // make POVRay to use left handed system to right handed
  // TODO: aspect ratio
  fprintf(this->FilePtr, "\tright <-1, 0, 0>\n");
  //fprintf(this->FilePtr, "\tup <-1, 0, 0>\n");
  
  fprintf(this->FilePtr, "\tangle %f\n", camera->GetViewAngle());
  
  double *focal = camera->GetFocalPoint();
  fprintf(this->FilePtr, "\tlook_at <%f, %f, %f>\n", 
          focal[0], focal[1], focal[2]);
  
  fprintf(this->FilePtr, "}\n\n");
}

void vtkPOVExporter::WriteLight(vtkLight *light)
{
  fprintf(this->FilePtr, "light_source {\n");
  
  double *position = light->GetPosition();
  fprintf(this->FilePtr, "\t<%f, %f, %f>\n", 
          position[0], position[1], position[2]);
  
  double *color = light->GetDiffuseColor();
  fprintf(this->FilePtr, "\tcolor <%f, %f, %f>*%f\n", 
          color[0], color[1], color[2],
          light->GetIntensity());
  
  if (light->GetPositional()) 
    {
    fprintf(this->FilePtr, "\tspotlight\n");
    fprintf(this->FilePtr, "\tradius %f\n", light->GetConeAngle());
    fprintf(this->FilePtr, "\tfalloff %f\n", light->GetExponent());
    } 
  else 
    {
    fprintf(this->FilePtr, "\tparallel\n");
    }
  double *focal    = light->GetFocalPoint();
  fprintf(this->FilePtr, "\tpoint_at <%f, %f, %f>\n", 
          focal[0], focal[1], focal[2]);
  
  fprintf(this->FilePtr, "}\n\n");  
}

void vtkPOVExporter::WriteActor(vtkActor *actor)
{
  if (actor->GetMapper() == NULL) 
    {
    return;
    }
  
  // write geometry, first ask the pipeline to update data
  vtkDataSet *dataset = actor->GetMapper()->GetInput(); 
  if (dataset == NULL) 
    {
    return;
    }
  dataset->Update();
  
  // convert non polygon data to polygon data if needed
  vtkGeometryFilter *geometryFilter = NULL;
  vtkPolyData *polys = NULL;;
  if (dataset->GetDataObjectType() != VTK_POLY_DATA) 
    {
    geometryFilter = vtkGeometryFilter::New();
    geometryFilter->SetInput(dataset);
    geometryFilter->Update();
    polys = geometryFilter->GetOutput();
    } 
  else 
    {
    polys = static_cast<vtkPolyData *>(dataset);
    }
  
  // we only export Polygons and Triangle Strips
  if ((polys->GetNumberOfPolys() == 0) && (polys->GetNumberOfStrips() == 0))
    {
      return;
    }

  // write point coordinates
  vtkPoints *points = polys->GetPoints();

  // we use mesh2 since it maps better to how VTK stores
  // polygons/triangle strips
  fprintf(this->FilePtr, "mesh2 {\n");

  fprintf(this->FilePtr, "\tvertex_vectors {\n");
  fprintf(this->FilePtr, VTKPOV_CNTFMT, points->GetNumberOfPoints());
  for (vtkIdType i = 0; i < points->GetNumberOfPoints(); i++) 
    {
    double *pos = points->GetPoint(i);
    fprintf(this->FilePtr, "\t\t<%f, %f, %f>,\n", pos[0], pos[1], pos[2]);
    }
  fprintf(this->FilePtr, "\t}\n");
  
  // write vertex normal 
  vtkPointData *pointData = polys->GetPointData();
  if (pointData->GetNormals()) 
    {
    vtkDataArray *normals = pointData->GetNormals();
    fprintf(this->FilePtr, "\tnormal_vectors {\n");
    fprintf(this->FilePtr, VTKPOV_CNTFMT, normals->GetNumberOfTuples());
    for (vtkIdType i = 0; i < normals->GetNumberOfTuples(); i++) 
      {
      double *normal = normals->GetTuple(i);
      fprintf(this->FilePtr, "\t\t<%f, %f, %f>,\n", 
              normal[0], normal[1], normal[2]);
      }
    fprintf(this->FilePtr, "\t}\n");
    }
  
  // TODO: write texture coordinates (uv vectors)
  
  // write vertex texture, ask mapper to generate color for each vertex if
  // the scalar data visibility is on
  bool scalar_visible = false;
  if (actor->GetMapper()->GetScalarVisibility()) 
    {
    vtkUnsignedCharArray *color_array = actor->GetMapper()->MapScalars(1.0);
    if (color_array != NULL) 
      {
      scalar_visible = true;
      fprintf(this->FilePtr, "\ttexture_list {\n");
      fprintf(this->FilePtr, VTKPOV_CNTFMT, color_array->GetNumberOfTuples());
      for (vtkIdType i = 0; i < color_array->GetNumberOfTuples(); i++) {
        unsigned char *color = color_array->GetPointer(4*i);
        fprintf(this->FilePtr, 
                "\t\ttexture { pigment {color rgbf <%f, %f, %f, %f> } },\n",
                color[0]/255.0, 
                color[1]/255.0, 
                color[2]/255.0, 
                1.0 - color[3]/255.0);
        }
      fprintf(this->FilePtr, "\t}\n");
      }
    }
  
  // write polygons
  if (polys->GetNumberOfPolys() > 0) 
    {
    this->WritePolygons(polys, scalar_visible);
    }
  
  // write triangle strips
  if (polys->GetNumberOfStrips() > 0) 
    {
    this->WriteTriangleStrips(polys, scalar_visible);
    }
  
  // write transformation for the actor, it is column major and looks like transposed
  vtkMatrix4x4 *matrix = actor->GetMatrix();
  fprintf(this->FilePtr, "\tmatrix < %f, %f, %f,\n",
          matrix->GetElement(0, 0), 
          matrix->GetElement(1, 0), 
          matrix->GetElement(2, 0));
  fprintf(this->FilePtr, "\t\t %f, %f, %f,\n",
          matrix->GetElement(0, 1), 
          matrix->GetElement(1, 1), 
          matrix->GetElement(2, 1));
  fprintf(this->FilePtr, "\t\t %f, %f, %f,\n",
          matrix->GetElement(0, 2), 
          matrix->GetElement(1, 2), 
          matrix->GetElement(2, 2));
  fprintf(this->FilePtr, "\t\t %f, %f, %f >\n",
          matrix->GetElement(0, 3), 
          matrix->GetElement(1, 3), 
          matrix->GetElement(2, 3));
  
  // write property
  this->WriteProperty(actor->GetProperty());
  
  // done with this actor
  fprintf(this->FilePtr, "}\n\n");
  
  if (geometryFilter) 
    {
    geometryFilter->Delete();
    }
}

void vtkPOVExporter::WritePolygons(vtkPolyData *polys, bool scalar_visible)
{
  // write polygons with on the fly triangulation, 
  // assuming polygon are simple and can be triangulated into "fans"
  vtkIdType numtriangles = 0;
  vtkCellArray *cells = polys->GetPolys();
  vtkIdType npts = 0, *pts = 0;

  // first pass, 
  // calculate how many triangles there will be after triangulation
  for (cells->InitTraversal(); cells->GetNextCell(npts, pts);) 
    {
    // the number of triangles for each polygon will be # of vertex - 2
    numtriangles += (npts - 2);
    }

  // second pass, triangulate and write face indices
  fprintf(this->FilePtr, "\tface_indices {\n");
  fprintf(this->FilePtr, VTKPOV_CNTFMT, numtriangles);
  for (cells->InitTraversal(); cells->GetNextCell(npts, pts);) 
    {
    vtkIdType triangle[3];
    // the first triangle  
    triangle[0] = pts[0];
    triangle[1] = pts[1];
    triangle[2] = pts[2];
    
    fprintf(this->FilePtr, VTKPOV_TRIFMT1,
            triangle[0], triangle[1], triangle[2]);
    if (scalar_visible)
      {
      fprintf(this->FilePtr, VTKPOV_TRIFMT2,
              triangle[0], triangle[1], triangle[2]);
      }
    else
      {
      fprintf(this->FilePtr, "\n");
      }
    
  // the rest of triangles            
    for (vtkIdType i = 3; i < npts; i++) 
      {    
      triangle[1] = triangle[2];
      triangle[2] = pts[i];
      fprintf(this->FilePtr, VTKPOV_TRIFMT1,
              triangle[0], triangle[1], triangle[2]);
      if (scalar_visible)
        {
        fprintf(this->FilePtr, VTKPOV_TRIFMT2,
                triangle[0], triangle[1], triangle[2]);
        }
      else
        {
        fprintf(this->FilePtr, "\n");        
        }
      }
    }
  fprintf(this->FilePtr, "\t}\n");
  
  // third pass, the same thing as 2nd pass but for normal_indices
  if (polys->GetPointData()->GetNormals()) 
    {
    fprintf(this->FilePtr, "\tnormal_indices {\n");
    fprintf(this->FilePtr, VTKPOV_CNTFMT, numtriangles);
    for (cells->InitTraversal(); cells->GetNextCell(npts, pts);) 
      {
      vtkIdType triangle[3];
      // the first triangle  
      triangle[0] = pts[0];
      triangle[1] = pts[1];
      triangle[2] = pts[2];
      
      fprintf(this->FilePtr, VTKPOV_TRIFMT1,
              triangle[0], triangle[1], triangle[2]);
      fprintf(this->FilePtr, "\n");
      
      // the rest of triangles            
      for (vtkIdType i = 3; i < npts; i++) 
        {    
        triangle[1] = triangle[2];
        triangle[2] = pts[i];
        fprintf(this->FilePtr, VTKPOV_TRIFMT1,
                triangle[0], triangle[1], triangle[2]);
        fprintf(this->FilePtr, "\n");
        }
      }
    fprintf(this->FilePtr, "\t}\n");
    }
  
  // TODO: 4th pass, texture indices

}

void vtkPOVExporter::WriteTriangleStrips(
  vtkPolyData *polys, bool scalar_visible)
{
  // convert triangle strips into triangles
  vtkIdType numtriangles = 0;
  vtkCellArray *cells = polys->GetStrips();
  vtkIdType npts = 0, *pts = 0;
  
  // first pass, calculate how many triangles there will be after conversion
  for (cells->InitTraversal(); cells->GetNextCell(npts, pts);) 
    {
    // the number of triangles for each polygon will be # of vertex - 2
    numtriangles += (npts - 2);
    }

  // second pass, convert to triangles and write face indices
  fprintf(this->FilePtr, "\tface_indices {\n");
  fprintf(this->FilePtr, VTKPOV_CNTFMT, numtriangles);
  for (cells->InitTraversal(); cells->GetNextCell(npts, pts);) 
    {
    vtkIdType triangle[3];
    // the first triangle  
    triangle[0] = pts[0];
    triangle[1] = pts[1];
    triangle[2] = pts[2];
    
    fprintf(this->FilePtr, VTKPOV_TRIFMT1,
            triangle[0], triangle[1], triangle[2]);
    if (scalar_visible)
      {
      fprintf(this->FilePtr, VTKPOV_TRIFMT2,
              triangle[0], triangle[1], triangle[2]);
      }
    else
      {
      fprintf(this->FilePtr, "\n");
      }
    
  // the rest of triangles            
    for (vtkIdType i = 3; i < npts; i++) 
      {    
      triangle[0] = triangle[1];
      triangle[1] = triangle[2];
      triangle[2] = pts[i];
      fprintf(this->FilePtr, VTKPOV_TRIFMT1,
              triangle[0], triangle[1], triangle[2]);
      if (scalar_visible)
        {
        fprintf(this->FilePtr, VTKPOV_TRIFMT2,
                triangle[0], triangle[1], triangle[2]);
        }
      else
        {
        fprintf(this->FilePtr, "\n");
        }
      }
    }
  fprintf(this->FilePtr, "\t}\n");
  
  // third pass, the same thing as 2nd pass but for normal_indices
  if (polys->GetPointData()->GetNormals()) 
    {
    fprintf(this->FilePtr, "\tnormal_indices {\n");
    fprintf(this->FilePtr, VTKPOV_CNTFMT, numtriangles);
    for (cells->InitTraversal(); cells->GetNextCell(npts, pts);) 
      {
      vtkIdType triangle[3];
      // the first triangle  
      triangle[0] = pts[0];
      triangle[1] = pts[1];
      triangle[2] = pts[2];
      
      fprintf(this->FilePtr, VTKPOV_TRIFMT1,
              triangle[0], triangle[1], triangle[2]);
      fprintf(this->FilePtr, "\n");
      
      // the rest of triangles            
      for (vtkIdType i = 3; i < npts; i++) 
        {
        triangle[0] = triangle[1];
        triangle[1] = triangle[2];
        triangle[2] = pts[i];
        fprintf(this->FilePtr, VTKPOV_TRIFMT1,
                triangle[0], triangle[1], triangle[2]);
        fprintf(this->FilePtr, "\n");
        }
      }
    fprintf(this->FilePtr, "\t}\n");
    }
  
  // TODO: 4th pass, texture indices
}

void vtkPOVExporter::WriteProperty(vtkProperty *property)
{
  fprintf(this->FilePtr, "\ttexture {\n");
  
  /* write color */
  fprintf(this->FilePtr, "\t\tpigment {\n");
  double *color   = property->GetColor();
  fprintf(this->FilePtr, "\t\t\tcolor rgbf <%f, %f, %f %f>\n",
          color[0], color[1], color[2], 1.0 - property->GetOpacity());
  fprintf(this->FilePtr, "\t\t}\n");
  
  /* write ambient, diffuse and specular coefficients */
  fprintf(this->FilePtr, "\t\tfinish {\n\t\t\t");
  fprintf(this->FilePtr, "ambient %f  ", property->GetAmbient());
  fprintf(this->FilePtr, "diffuse %f  ", property->GetDiffuse());
  fprintf(this->FilePtr, "phong %f  ", property->GetSpecular());
  fprintf(this->FilePtr, "phong_size %f  ", property->GetSpecularPower());
  fprintf(this->FilePtr, "\n\t\t}\n");
  
  fprintf(this->FilePtr, "\t}\n");
}

void vtkPOVExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->FileName)
    {
    os << indent << "FileName: " << this->FileName << "\n";
    } 
  else 
    {
    os << indent << "FileName: (null)\n";
    }
}
