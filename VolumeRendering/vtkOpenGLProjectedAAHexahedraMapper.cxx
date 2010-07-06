/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLProjectedAAHexahedraMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// High quality volume renderer for axis-aligned hexahedra
// Implementation by Stephane Marchesin (stephane.marchesin@gmail.com)
// CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France
// BP12, F-91297 Arpajon, France.
//
// This file implements the paper 
// "High-Quality, Semi-Analytical Volume Rendering for AMR Data", 
// Stephane Marchesin and Guillaume Colin de Verdiere, IEEE Vis 2009.


#include "vtkOpenGLProjectedAAHexahedraMapper.h"

#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellCenterDepthSort.h"
#include "vtkCellData.h"
#include "vtkColorTransferFunction.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkGarbageCollector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridPreIntegration.h"
#include "vtkVisibilitySort.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkVolumeRenderingFactory.h"

#include "vtkOpenGL.h"

#include <math.h>
#include <vtkstd/algorithm>

//-----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkOpenGLProjectedAAHexahedraMapper,
                     VisibilitySort, vtkVisibilitySort);

//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkOpenGLProjectedAAHexahedraMapper);

// ----------------------------------------------------------------------------
vtkOpenGLProjectedAAHexahedraMapper::vtkOpenGLProjectedAAHexahedraMapper()
{
  this->VisibilitySort = vtkCellCenterDepthSort::New();
  this->ConvertedPoints = vtkFloatArray::New();
  this->ConvertedScalars = vtkFloatArray::New();

  this->LastProperty = NULL;

  this->PreintTexture = 0;
  this->MaxCellSize = 0;

  this->GaveError = 0;
  this->Initialized=false;

}

// ----------------------------------------------------------------------------
vtkOpenGLProjectedAAHexahedraMapper::~vtkOpenGLProjectedAAHexahedraMapper()
{
  this->SetVisibilitySort(NULL);
  this->ConvertedPoints->Delete();
  this->ConvertedScalars->Delete();
}

// ----------------------------------------------------------------------------
void vtkOpenGLProjectedAAHexahedraMapper::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "VisibilitySort: " << this->VisibilitySort << endl;

}

//-----------------------------------------------------------------------------

void vtkOpenGLProjectedAAHexahedraMapper::ReportReferences(vtkGarbageCollector *collector)
{
  this->Superclass::ReportReferences(collector);

  vtkGarbageCollectorReport(collector, this->VisibilitySort, "VisibilitySort");
}

// ----------------------------------------------------------------------------
bool vtkOpenGLProjectedAAHexahedraMapper::IsRenderSupported(vtkRenderWindow *w)
{
  vtkOpenGLExtensionManager *e=
    static_cast<vtkOpenGLRenderWindow *>(w)->GetExtensionManager();

  bool texture3D=e->ExtensionSupported("GL_VERSION_1_2") ||
    e->ExtensionSupported("GL_EXT_texture3D");

  bool multiTexture=e->ExtensionSupported("GL_VERSION_1_3") ||
    e->ExtensionSupported("GL_ARB_multitexture");

  bool glsl=e->ExtensionSupported("GL_VERSION_2_0") ||
    (e->ExtensionSupported("GL_ARB_shading_language_100") &&
     e->ExtensionSupported("GL_ARB_shader_objects") &&
     e->ExtensionSupported("GL_ARB_vertex_shader") &&
     e->ExtensionSupported("GL_ARB_fragment_shader"));

  bool geometry_shader=e->ExtensionSupported("GL_EXT_geometry_shader4");

  return multiTexture && glsl && geometry_shader;
}

//-----------------------------------------------------------------------------

void vtkOpenGLProjectedAAHexahedraMapper::Initialize(vtkRenderer *ren, vtkVolume *vol)
{
  vtkOpenGLExtensionManager *e=static_cast<vtkOpenGLRenderWindow *>(
    ren->GetRenderWindow())->GetExtensionManager();

  bool gl12=e->ExtensionSupported("GL_VERSION_1_2")==1;
  bool gl13=e->ExtensionSupported("GL_VERSION_1_3")==1;
  bool gl20=e->ExtensionSupported("GL_VERSION_2_0")==1;

  bool texture3D=gl12 || e->ExtensionSupported("GL_EXT_texture3D");
  bool multiTexture=gl13 || e->ExtensionSupported("GL_ARB_multitexture");
  bool glsl=gl20 || (e->ExtensionSupported("GL_ARB_shading_language_100") &&
                     e->ExtensionSupported("GL_ARB_shader_objects") &&
                     e->ExtensionSupported("GL_ARB_vertex_shader") &&
                     e->ExtensionSupported("GL_ARB_fragment_shader"));
  bool geometry_shader=e->ExtensionSupported("GL_EXT_geometry_shader4");

  bool result=multiTexture && glsl && geometry_shader;

  if(result)
    {
    if(gl12)
      {
      e->LoadExtension("GL_VERSION_1_2");
      }
    else
      {
      e->LoadCorePromotedExtension("GL_EXT_texture3D");
      }
    if(gl13)
      {
      e->LoadExtension("GL_VERSION_1_3");
      }
    else
      {
      e->LoadCorePromotedExtension("GL_ARB_multitexture");
      }
    if(gl20)
      {
      e->LoadExtension("GL_VERSION_2_0");
      }
    else
      {
      e->LoadCorePromotedExtension("GL_ARB_shading_language_100");
      e->LoadCorePromotedExtension("GL_ARB_shader_objects");
      e->LoadCorePromotedExtension("GL_ARB_vertex_shader");
      e->LoadCorePromotedExtension("GL_ARB_fragment_shader");
      }
    e->LoadExtension("GL_EXT_geometry_shader4");

    this->Initialized=true;
    this->CreateProgram();
    pos_points = new float[3*max_points];
    min_points = new float[3*max_points];
    node_data1 = new float[4*max_points];
    node_data2 = new float[4*max_points];
    }
}

//-----------------------------------------------------------------------------
// sort, iterate the hexahedra and call the rendering function
void vtkOpenGLProjectedAAHexahedraMapper::Render(vtkRenderer *renderer,
		vtkVolume *volume)
{
	if ( !this->Initialized )
		this->Initialize(renderer, volume);

	vtkUnstructuredGrid *input = this->GetInput();
	vtkVolumeProperty *property = volume->GetProperty();

	float last_max_cell_size = this->MaxCellSize;

	// Check to see if input changed.
	if (   (this->InputAnalyzedTime < this->MTime)
			|| (this->InputAnalyzedTime < input->GetMTime()) )
	{
		this->GaveError = 0;

		vtkCellArray *cells = input->GetCells();
		if (!cells)
		{
			// Apparently, the input has no cells.  Just do nothing.
			return;
		}

		vtkIdType npts, *pts, i;
		cells->InitTraversal();
		for (i = 0; cells->GetNextCell(npts, pts); i++)
		{
			int j;
			if (npts != 8)
			{
				if (!this->GaveError)
				{
					vtkErrorMacro("Encountered non-hexahedral cell!");
					this->GaveError = 1;
				}
				continue;
			}

			double p[3];
			input->GetPoint(pts[0], p);
			double min[3] = {p[0],p[1],p[2]},
			       max[3] = {p[0],p[1],p[2]};

			for(j = 1; j < npts; j++)
			{
				input->GetPoint(pts[j], p);

				if (p[0]<min[0]) min[0] = p[0];
				if (p[1]<min[1]) min[1] = p[1];
				if (p[2]<min[2]) min[2] = p[2];
				if (p[0]>max[0]) max[0] = p[0];
				if (p[1]>max[1]) max[1] = p[1];
				if (p[2]>max[2]) max[2] = p[2];
			}

			float size = (float)vtkMath::Distance2BetweenPoints(min, max);
			if (size > this->MaxCellSize) this->MaxCellSize = size;
		}

		this->InputAnalyzedTime.Modified();
	}

	if (renderer->GetRenderWindow()->CheckAbortStatus() || this->GaveError)
	{
		return;
	}

	// Check to see if we need to rebuild preintegartion texture.
	if (   !this->PreintTexture
			|| (last_max_cell_size != this->MaxCellSize)
			|| (this->LastProperty != property)
			|| (this->PreintTextureTime < property->GetMTime()) )
	{
		if (!this->PreintTexture)
		{
			GLuint texid;
			glGenTextures(1, &texid);
			this->PreintTexture = texid;
		}
		float unit_distance = property->GetScalarOpacityUnitDistance();
		vtkDataArray *scalars = this->GetScalars(input, this->ScalarMode,
				this->ArrayAccessMode,
				this->ArrayId, this->ArrayName,
				this->UsingCellColors);
		if (!scalars)
		{
			vtkErrorMacro(<< "Can't use projected tetrahedra without scalars!");
			return;
		}

		this->UpdatePreintegrationTexture(volume, scalars);

		this->PreintTextureTime.Modified();

    		this->LastProperty = property;
	}

	if (renderer->GetRenderWindow()->CheckAbortStatus())
	{
		return;
	}

	this->Timer->StartTimer();

	this->ProjectHexahedra(renderer, volume);

	this->Timer->StopTimer();
	this->TimeToDraw = this->Timer->GetElapsedTime();
}

//-----------------------------------------------------------------------------
void vtkOpenGLProjectedAAHexahedraMapper::UpdatePreintegrationTexture(vtkVolume *volume, vtkDataArray *scalars)
{
	// rebuild the preintegration texture
	vtkUnstructuredGridPreIntegration *pi = vtkUnstructuredGridPreIntegration::New();
	pi->Initialize(volume, scalars);
	int tableSize = 0;
	// We only render the first field
	float *table = pi->GetPreIntegrationTable(0);
	int ScalarSize = pi->GetIntegrationTableScalarResolution();
	int LengthSize = pi->GetIntegrationTableLengthResolution();

	this->ScalarScale = pi->GetIntegrationTableScalarScale();
	this->ScalarResolution = pi->GetIntegrationTableScalarResolution();
	this->ScalarShift = pi->GetIntegrationTableScalarShift();
	this->LengthScale = (pi->GetIntegrationTableLengthResolution() - 2) / pi->GetIntegrationTableLengthScale() ;


	glEnable(GL_TEXTURE_3D);
	glBindTexture(GL_TEXTURE_3D, this->PreintTexture);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	vtkgl::TexImage3D(GL_TEXTURE_3D, 0, vtkgl::RGBA16_EXT, ScalarSize, ScalarSize, LengthSize, 0, GL_RGBA, GL_FLOAT, table);

	pi->Delete();

}

/* inputs of VS
 * vertexpos.xyz : cell position (xmin,ymin,zmin)
 * texcoord0.xyz : cell position (xmax,ymax,zmax)
 * texcoord1.xyzw: node data 0,1,2,3
 * texcoord2.xyzw: node data 4,5,6,7
 */
const char *vtkOpenGLProjectedAAHexahedraMapper::VertSource =
"void main()"
"{"
"	gl_TexCoord[0] = gl_MultiTexCoord0;"
"	gl_TexCoord[1] = gl_MultiTexCoord1;"
"	gl_TexCoord[2] = gl_MultiTexCoord2;"
"	gl_Position = gl_Vertex;"
"}";


/* inputs of GS
 * vertexpos.xyz : cell position (xmin,ymin,zmin)
 * texcoord0.xyz : cell position (xmax,ymax,zmax)
 * texcoord1.xyzw: node data 0,1,2,3
 * texcoord2.xyzw: ode data 4,5,6,7
 */
const char *vtkOpenGLProjectedAAHexahedraMapper::GeomSource =
"#version 120													\n"
"#extension GL_EXT_geometry_shader4 : enable									\n"
"void draw_cell(vec4 scalars0, vec4 scalars1, vec3 m, vec3 M)							\n"
"{														\n"
"	// common node data											\n"
"	gl_TexCoord[2].xyzw = scalars0;										\n"
"	gl_TexCoord[3].xyzw = scalars1;										\n"
"	float cs = M.x - m.x;											\n"
"	vec4 p0 = vec4(m.x,m.y,m.z,1.0);									\n"
"	vec4 p1 = vec4(m.x,m.y,M.z,1.0);									\n"
"	vec4 p2 = vec4(m.x,M.y,m.z,1.0);									\n"
"	vec4 p3 = vec4(m.x,M.y,M.z,1.0);									\n"
"	vec4 p4 = vec4(M.x,m.y,m.z,1.0);									\n"
"	vec4 p5 = vec4(M.x,m.y,M.z,1.0);									\n"
"	vec4 p6 = vec4(M.x,M.y,m.z,1.0);									\n"
"	vec4 p7 = vec4(M.x,M.y,M.z,1.0);									\n"
"	vec4 t0 = gl_ModelViewProjectionMatrix * p0;								\n"
"	vec4 t1 = gl_ModelViewProjectionMatrix * p1;								\n"
"	vec4 t2 = gl_ModelViewProjectionMatrix * p2;								\n"
"	vec4 t3 = gl_ModelViewProjectionMatrix * p3;								\n"
"	vec4 t4 = gl_ModelViewProjectionMatrix * p4;								\n"
"	vec4 t5 = gl_ModelViewProjectionMatrix * p5;								\n"
"	vec4 t6 = gl_ModelViewProjectionMatrix * p6;								\n"
"	vec4 t7 = gl_ModelViewProjectionMatrix * p7;								\n"
"														\n"
	// face 0
"	gl_TexCoord[0] = vec4(1.0,0.0,0.0,cs);									\n"
"	gl_TexCoord[1] = p4;											\n"
"	gl_Position  = t4;											\n"
"	EmitVertex(); 												\n"
"	gl_TexCoord[0] = vec4(1.0,0.0,1.0,cs);									\n"
"	gl_TexCoord[1] = p5;											\n"
"	gl_Position  = t5;											\n"
"	EmitVertex(); 												\n"
"	gl_TexCoord[0] = vec4(1.0,1.0,0.0,cs);									\n"
"	gl_TexCoord[1] = p6;											\n"
"	gl_Position  = t6;											\n"
"	EmitVertex(); 												\n"
"	gl_TexCoord[0] = vec4(1.0,1.0,1.0,cs);									\n"
"	gl_TexCoord[1] = p7;											\n"
"	gl_Position  = t7;											\n"
"	EmitVertex(); 												\n"
"	EndPrimitive();												\n"
	// face 1
"	gl_TexCoord[0] = vec4(0.0,1.0,0.0,cs);									\n"
"	gl_TexCoord[1] = p2;											\n"
"	gl_Position  = t2;											\n"
"	EmitVertex(); 												\n"
"	gl_TexCoord[0] = vec4(1.0,1.0,0.0,cs);									\n"
"	gl_TexCoord[1] = p6;											\n"
"	gl_Position  = t6;											\n"
"	EmitVertex(); 												\n"
"	gl_TexCoord[0] = vec4(0.0,1.0,1.0,cs);									\n"
"	gl_TexCoord[1] = p3;											\n"
"	gl_Position  = t3;											\n"
"	EmitVertex(); 												\n"
"	gl_TexCoord[0] = vec4(1.0,1.0,1.0,cs);									\n"
"	gl_TexCoord[1] = p7;											\n"
"	gl_Position  = t7;											\n"
"	EmitVertex(); 												\n"
"	EndPrimitive();												\n"
	// face 2
"	gl_TexCoord[0] = vec4(0.0,1.0,1.0,cs);									\n"
"	gl_TexCoord[1] = p3;											\n"
"	gl_Position  = t3;											\n"
"	EmitVertex(); 												\n"
"	gl_TexCoord[0] = vec4(1.0,1.0,1.0,cs);									\n"
"	gl_TexCoord[1] = p7;											\n"
"	gl_Position  = t7;											\n"
"	EmitVertex(); 												\n"
"	gl_TexCoord[0] = vec4(0.0,0.0,1.0,cs);									\n"
"	gl_TexCoord[1] = p1;											\n"
"	gl_Position  = t1;											\n"
"	EmitVertex(); 												\n"
"	gl_TexCoord[0] = vec4(1.0,0.0,1.0,cs);									\n"
"	gl_TexCoord[1] = p5;											\n"
"	gl_Position  = t5;											\n"
"	EmitVertex(); 												\n"
"	EndPrimitive();												\n"
	// face 3
"	gl_TexCoord[0] = vec4(0.0,1.0,0.0,cs);									\n"
"	gl_TexCoord[1] = p2;											\n"
"	gl_Position  = t2;											\n"
"	EmitVertex();  												\n"
"	gl_TexCoord[0] = vec4(0.0,1.0,1.0,cs);									\n"
"	gl_TexCoord[1] = p3;											\n"
"	gl_Position  = t3;											\n"
"	EmitVertex();  	        										\n"
"	gl_TexCoord[0] = vec4(0.0,0.0,0.0,cs);									\n"
"	gl_TexCoord[1] = p0;											\n"
"	gl_Position  = t0;											\n"
"	EmitVertex();  	        										\n"
"	gl_TexCoord[0] = vec4(0.0,0.0,1.0,cs);									\n"
"	gl_TexCoord[1] = p1;											\n"
"	gl_Position  = t1;											\n"
"	EmitVertex();  												\n"
"	EndPrimitive();	 											\n"
	// face 4
"	gl_TexCoord[0] = vec4(0.0,0.0,0.0,cs);									\n"
"	gl_TexCoord[1] = p0;											\n"
"	gl_Position  = t0;											\n"
"	EmitVertex();  												\n"
"	gl_TexCoord[0] = vec4(0.0,0.0,1.0,cs);									\n"
"	gl_TexCoord[1] = p1;											\n"
"	gl_Position  = t1;											\n"
"	EmitVertex();  												\n"
"	gl_TexCoord[0] = vec4(1.0,0.0,0.0,cs);									\n"
"	gl_TexCoord[1] = p4;											\n"
"	gl_Position  = t4;											\n"
"	EmitVertex();  												\n"
"	gl_TexCoord[0] = vec4(1.0,0.0,1.0,cs);									\n"
"	gl_TexCoord[1] = p5;											\n"
"	gl_Position  = t5;											\n"
"	EmitVertex();  												\n"
"	EndPrimitive();	 											\n"
	// face 5
"	gl_TexCoord[0] = vec4(0.0,1.0,0.0,cs);									\n"
"	gl_TexCoord[1] = p2;											\n"
"	gl_Position  = t2;											\n"
"	EmitVertex();  												\n"
"	gl_TexCoord[0] = vec4(0.0,0.0,0.0,cs);									\n"
"	gl_TexCoord[1] = p0;											\n"
"	gl_Position  = t0;											\n"
"	EmitVertex();  												\n"
"	gl_TexCoord[0] = vec4(1.0,1.0,0.0,cs);									\n"
"	gl_TexCoord[1] = p6;											\n"
"	gl_Position  = t6;											\n"
"	EmitVertex();  												\n"
"	gl_TexCoord[0] = vec4(1.0,0.0,0.0,cs);									\n"
"	gl_TexCoord[1] = p4;											\n"
"	gl_Position  = t4;											\n"
"	EmitVertex();  												\n"
"	EndPrimitive();	 											\n"
"}														\n"
"void main()													\n"
"{														\n"
"	draw_cell(gl_TexCoordIn[0][1],gl_TexCoordIn[0][2],gl_PositionIn[0].xyz,gl_TexCoordIn[0][0].xyz);	\n"
"}														\n"
;


/* inputs of FS
 * texcoord0.xyz : position in the brick space [0,1]^3
 * texcoord0.w   : cell size
 * texcoord1.xyz : position in object space
 * texcoord2.xyzw: node data 0,1,2,3
 * texcoord3.xyzw: node data 4,5,6,7
 */
const char *vtkOpenGLProjectedAAHexahedraMapper::FragSource =
"uniform sampler3D preintegration_table;									\n"
"uniform vec3 observer;												\n"
"uniform float length_max;											\n"
"vec4 sample(float sample0, float sample1, float length)							\n"
"{														\n"
"	float corrected_length = length * gl_TexCoord[0].w / length_max ;					\n"
"	return texture3D(preintegration_table,vec3(sample0,sample1,corrected_length));				\n"
"}														\n"
"void main()													\n"
"{														\n"
"	vec3 pos = gl_TexCoord[0].xyz;										\n"
"	vec3 progression;											\n"
"	vec3 dist1,dist2,dist;											\n"
"	vec3 l=vec3(1.0,0.0,0.0);										\n"
"	float length;												\n"
"	float cell_length = gl_TexCoord[0].w;									\n"
"														\n"
"	progression.xyz = gl_TexCoord[1].xyz - observer.xyz;							\n"
"	progression = normalize(progression);									\n"
"														\n"
"	dist1.xyz = abs((1.0-pos.xyz)/progression.xyz);								\n"
"	dist2.xyz = abs((pos.xyz)/progression.xyz);								\n"
"	if (progression.x>0.0)											\n"
"		dist.x=dist1.x;											\n"
"	else													\n"
"		dist.x=dist2.x;											\n"
"	if (progression.y>0.0)											\n"
"		dist.y=dist1.y;											\n"
"	else													\n"
"		dist.y=dist2.y;											\n"
"	if (progression.z>0.0)											\n"
"		dist.z=dist1.z;											\n"
"	else													\n"
"		dist.z=dist2.z;											\n"
"														\n"
"	length = min(dist.x,min(dist.y,dist.z));								\n"
"	vec3 p1 = pos, p2 = pos + vec3(length) * progression;							\n"
"														\n"
"	float s0 = gl_TexCoord[2].x;										\n"
"	float s1 = gl_TexCoord[2].y;										\n"
"	float s2 = gl_TexCoord[2].z;										\n"
"	float s3 = gl_TexCoord[2].w;										\n"

"	float s4 = gl_TexCoord[3].x;										\n"
"	float s5 = gl_TexCoord[3].y;										\n"
"	float s6 = gl_TexCoord[3].z;										\n"
"	float s7 = gl_TexCoord[3].w;										\n"
"	float 	x0 = p1.x,											\n"
"		x1 = p2.x - p1.x,										\n"
"		y0 = p1.y,											\n"
"		y1 = p2.y - p1.y, 										\n"
"		z0 = p1.z,											\n"
"		z1 = p2.z - p1.z; 										\n"
"	float a = (s3 - s0 + s1 + s4 + s6 - s2 - s5 - s7) *x1*y1*z1;						\n"
"	float b = (-x0*y1*z1 - x1*y0*z1 - x1*y1*z0 + x1*z1)*s7							\n"
"		+ (x0*y1*z1 + x1*y0*z1 + x1*y1*z0)*s6								\n"
"		+ (y1*z1 - x0*y1*z1 - x1*y0*z1 - x1*y1*z0)*s5							\n"
"		+ (-x1*z1 + x1*y1*z0 - y1*z1 + x0*y1*z1 + x1*y0*z1)*s4						\n"
"		+ (-x1*z1 + x1*y0*z1 + x1*y1*z0 - x1*y1 + x0*y1*z1)*s3						\n"
"		+ (-x1*y0*z1 - x0*y1*z1 + x1*y1 - x1*y1*z0)*s2							\n"
"		+ (x1*y1 + y1*z1 - x1*y1*z0 + x1*z1 - x0*y1*z1 - x1*y0*z1)*s0					\n"
"		+ (x1*y1*z0 - y1*z1 - x1*y1 + x0*y1*z1 + x1*y0*z1)*s1;						\n"
"	float c = (-x0*y0*z1 + x0*z1 + x1*z0 - x1*y0*z0 - x0*y1*z0)*s7						\n"
"		+ (x1*y0*z0 + x0*y1*z0 + x0*y0*z1)*s6								\n"
"		+ (y0*z1 - x0*y1*z0 - x1*y0*z0 + y1*z0 - x0*y0*z1)*s5						\n"
"		+ (x0*y0*z1 + z1 - y0*z1 - y1*z0 - x0*z1 - x1*z0 + x0*y1*z0 + x1*y0*z0)*s4			\n"
"		+ (x1*y0*z0 + x0*y0*z1 + x0*y1*z0 - x1*z0 - x1*y0 - x0*z1 - x0*y1 + x1)*s3			\n"
"		+ (x0*y1 + x1*y0 - x0*y0*z1 - x0*y1*z0 - x1*y0*z0)*s2						\n"
"		+ (-x1*y0 + x0*y1*z0 - y0*z1 - x0*y1 + x0*y0*z1 + y1 + x1*y0*z0 - y1*z0)*s1			\n"
"		+ (-x0*y1*z0 - z1 + x1*y0 - x0*y0*z1 - x1*y0*z0 - y1 + y0*z1 + x1*z0				\n"
"		+ y1*z0 + x0*y1 - x1 + x0*z1)*s0;								\n"
"	float d = (x0*z0 - x0*y0*z0)*s7 + (y0*z0 - x0*y0*z0)*s5							\n"
"		+ (-x0*z0 - y0*z0 + x0*y0*z0 + z0)*s4 + (-x0*z0 + x0 + x0*y0*z0 - x0*y0)*s3			\n"
"		+ (x0*y0 - x0*y0*z0)*s2 + (-y0*z0 - x0*y0 + y0 + x0*y0*z0)*s1					\n"
"		+ (-y0 - z0 - x0*y0*z0 + x0*z0 + y0*z0 - x0 + x0*y0 + 1.0)*s0 + s6*x0*y0*z0;			\n"
"	float r[4];												\n"
"	r[0] = 0.0;												\n"
"	r[1] = 0.0;												\n"
"	r[2] = 0.0;												\n"
"	r[3] = 0.0;												\n"
"	int numsteps = 0;											\n"
"														\n"
	// at this point P(t) = a.t^3 + b.t^2 + c.t + d
"														\n"
"	if ( (abs(a)<=0.00001) && (abs(b)<=0.00001) )								\n"
"	{													\n"
		// P(t) is linear
"		numsteps = 0;											\n"
"	}													\n"
"	else if (abs(a)<=0.00001)										\n"
"	{													\n"
		// P(t) is quadratic
"		r[0] = -c/(2.0*b);										\n"
"														\n"
"		if ((r[0] <= 0.0) || (r[0] >= 1.0))								\n"
"		{												\n"
"			numsteps = 0;										\n"
"		}												\n"
"		else												\n"
"		{												\n"
"			numsteps = 1;										\n"
"		}												\n"
"	} 													\n"
"	else 													\n"
"	{													\n"
		// P(t) is cubic
		// small optimization here : we divide delta by 4, and simplify r[0]/r[1] by 2
"		float delta = b*b - 3.0*a*c;									\n"
"		if (delta < 0.0)										\n"
"		{												\n"
"			numsteps = 0;										\n"
"		} else {											\n"
"			numsteps = 2;										\n"
"			r[0] = (-b  - sqrt(delta))/(3.0*a);							\n"
"			r[1] = (-b  + sqrt(delta))/(3.0*a);							\n"
"														\n"
"			if ((r[1] <= 0.0) || (r[1] >= 1.0))							\n"
"			{											\n"
"				numsteps--;									\n"
"			}											\n"
"														\n"
"			if ((r[0] <= 0.0) || (r[0] >= 1.0))							\n"
"			{											\n"
"				numsteps--;									\n"
"				r[0] = r[1];									\n"
"			}											\n"
"		}												\n"
"	}													\n"
"														\n"
#if 0
	// handle light extrema as well
"	if (abs(e)>0.00001)											\n"
"	{													\n"
		// Q(t) is quadratic
"		if ((-f/(2.0*e) > 0.0) && (-f/(2.0*e) < 1.0))							\n"
"		{												\n"
"		r[numsteps] = -f/(2.0*e);									\n"
"		numsteps++;											\n"
"		}												\n"
"	}													\n"
#endif
"	vec4 result, val0, val1, val2, val3;									\n"
"	float sample0,sample1,sample2,sample3,sample4;								\n"
"	if (numsteps==0)											\n"
"	{													\n"
		// single preintegration over [0,1]
"														\n"
		// evaluate the scalar value at the 2 points :
		// sample0 at t = 0.0;
		// sample1 at t = 1.0;
"		sample0 = d;											\n"
"		sample1 = d + c + b + a;									\n"
"														\n"
		// preintegrate over [0,1.0] -> [sample0,sample1]
"		val0 = sample(sample0,sample1,length);								\n"
"														\n"
		// blend values	
"		result.rgba = val0.rgba;									\n"
"	} 													\n"
"	else if (numsteps==1)											\n"
"	{													\n"
		// double preintegration over [0,r[0]] and [r[0],1.0]
"														\n"
		// evaluate the scalar value at the 3 points :
		// sample0 at t = 0.0;
		// sample1 at t = r[0];
		// sample2 at t = 1.0;
"		sample0 = d;											\n"
"		sample1 = d + r[0]* (c + r[0]* (b + r[0]*a));							\n"
"		sample2 = d + c + b + a;									\n"
"														\n"
		// preintegrate over [0,r[0]] -> [sample0,sample1]
"		val0 = sample(sample0,sample1,r[0]*length);							\n"
		// preintegrate over [r[0],1] -> [sample1,sample2]
"		val1 = sample(sample1,sample2,(1.0 - r[0])*length);						\n"
"														\n"
		// blend values
"		result.rgba = val0.rgba + vec4(1.0 - val0.a) * val1.rgba;					\n"
"	}													\n"
"	else if (numsteps==2)											\n" 
"	{													\n"
		// numsteps==2
		// triple preintegration over [0,r[0]], [r[0],r[1]] and [r[1],1.0]
"														\n"
"		if (r[1]<r[0])											\n"
"		{												\n"
"			float tmp = r[0];									\n"
"			r[0] = r[1];										\n"
"			r[1] = tmp;										\n"
"		}												\n"
"														\n"
		// evaluate the scalar value at the 4 points :
		// sample0 at t = 0.0;
		// sample1 at t = r[0];
		// sample2 at t = r[1];
		// sample3 at t = 1.0;
"		sample0 = d;											\n"
"		sample1 = d + r[0]* (c + r[0]* (b + r[0]*a));							\n"
"		sample2 = d + r[1]* (c + r[1]* (b + r[1]*a));							\n"
"		sample3 = d + c + b + a;									\n"
"														\n"
		// preintegrate over [0,r[0]] -> [sample0,sample1]
"		val0 = sample(sample0,sample1,r[0]*length);							\n"
		// preintegrate over [r[0],r[1]] -> [sample1,sample2]
"		val1 = sample(sample1,sample2,(r[1] - r[0])*length);						\n"
		// preintegrate over [r[1],1] -> [sample2,sample3]
"		val2 = sample(sample2,sample3,(1.0 - r[1])*length);						\n"
"														\n"
		// blend values
"		result.rgba = val0.rgba + vec4(1.0 - val0.a) * (val1.rgba + vec4(1.0 - val1.a) * val2.rgba);	\n"
"	}													\n"
"	else													\n" 
"	{													\n"
		// numsteps==3
		// triple preintegration over [0,r[0]], [r[0],r[1]], [r[1],r[2]] and [r[2],1.0]
"														\n"
"		if (r[0]>r[1])											\n"
"		{												\n"
"			float tmp = r[0];									\n"
"			r[0] = r[1];										\n"
"			r[1] = tmp;										\n"
"		}												\n"
"		if (r[1]>r[2])											\n"
"		{												\n"
"			float tmp = r[2];									\n"
"			r[2] = r[1];										\n"
"			r[1] = tmp;										\n"
"		}												\n"
"		if (r[0]>r[1])											\n"
"		{												\n"
"			float tmp = r[0];									\n"
"			r[0] = r[1];										\n"
"			r[1] = tmp;										\n"
"		}												\n"
"														\n"
		// evaluate the scalar value at the 4 points :
		// sample0 at t = 0.0;
		// sample1 at t = r[0];
		// sample2 at t = r[1];
		// sample3 at t = 1.0;
"		sample0 = d;											\n"
"		sample1 = d + r[0]* (c + r[0]* (b + r[0]*a));							\n"
"		sample2 = d + r[1]* (c + r[1]* (b + r[1]*a));							\n"
"		sample3 = d + r[2]* (c + r[2]* (b + r[2]*a));							\n"
"		sample4 = d + c + b + a;									\n"
"														\n"
		// preintegrate over [0,r[0]] -> [sample0,sample1]
"		val0 = sample(sample0,sample1,r[0]*length);							\n"
		// preintegrate over [r[0],r[1]] -> [sample1,sample2]
"		val1 = sample(sample1,sample2,(r[1] - r[0])*length);						\n"
		// preintegrate over [r[1],r[2]] -> [sample2,sample3]
"		val2 = sample(sample2,sample3,(r[2] - r[1])*length);						\n"
		// preintegrate over [r[2],1] -> [sample3,sample4]
"		val3 = sample(sample3,sample4,(1.0 - r[2])*length);						\n"
"														\n"
		// blend values
"		result.rgba = val0.rgba + vec4(1.0 - val0.a) * (val1.rgba + vec4(1.0 - val1.a) * (val2.rgba + vec4(1.0 - val2.a) * val3.rgba));	\n"
"	}													\n"
"	gl_FragColor.rgba = result.rgba;									\n"
"}														\n"
;


void vtkOpenGLProjectedAAHexahedraMapper::CreateProgram()
{
	int ok;

	vtkgl::GLhandleARB gs=vtkgl::CreateShader(vtkgl::GEOMETRY_SHADER_EXT);
	vtkgl::GLhandleARB vs=vtkgl::CreateShader(vtkgl::VERTEX_SHADER_ARB);
	vtkgl::GLhandleARB fs=vtkgl::CreateShader(vtkgl::FRAGMENT_SHADER_ARB);

	vtkgl::ShaderSource(gs, 1, &this->GeomSource,NULL);
	vtkgl::ShaderSource(vs, 1, &this->VertSource,NULL);
	vtkgl::ShaderSource(fs, 1, &this->FragSource,NULL);

	vtkgl::CompileShader(gs);
        vtkgl::GetShaderiv(gs,vtkgl::COMPILE_STATUS,&ok);
	if (!ok)
		return;

	vtkgl::CompileShader(vs);
        vtkgl::GetShaderiv(vs,vtkgl::COMPILE_STATUS,&ok);
	if (!ok)
		return;

	vtkgl::CompileShader(fs);
        vtkgl::GetShaderiv(fs,vtkgl::COMPILE_STATUS,&ok);
	if (!ok)
		return;

	vtkgl::GLhandleARB p = vtkgl::CreateProgram();
	vtkgl::AttachShader(p,gs);
	vtkgl::AttachShader(p,vs);
	vtkgl::AttachShader(p,fs);

	vtkgl::ProgramParameteriEXT(p,vtkgl::GEOMETRY_VERTICES_OUT_EXT,24);
	vtkgl::ProgramParameteriEXT(p,vtkgl::GEOMETRY_INPUT_TYPE_EXT,GL_POINTS);
	vtkgl::ProgramParameteriEXT(p,vtkgl::GEOMETRY_OUTPUT_TYPE_EXT,GL_TRIANGLE_STRIP);

	vtkgl::LinkProgram(p);
	vtkgl::GetProgramiv(p,vtkgl::OBJECT_LINK_STATUS_ARB,&ok);
	if (!ok)
		return;
	
	Shader=p;
}


void vtkOpenGLProjectedAAHexahedraMapper::SetState(double* observer)
{
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glCullFace(GL_BACK);
	glDepthFunc( GL_ALWAYS );
	glDisable( GL_DEPTH_TEST );

	vtkgl::UseProgram(this->Shader);

	vtkgl::ActiveTexture( vtkgl::TEXTURE0_ARB );
	glDisable (GL_TEXTURE_2D);
	glEnable (GL_TEXTURE_3D);
	glBindTexture(GL_TEXTURE_3D, this->PreintTexture);

	/* preintegration table */
	vtkgl::Uniform1i(vtkgl::GetUniformLocation(this->Shader, "preintegration_table"),0);
	/* observer position */
	vtkgl::Uniform3f(vtkgl::GetUniformLocation(this->Shader, "observer"), observer[0], observer[1], observer[2]);
	/* max length of preint table */
	vtkgl::Uniform1f(vtkgl::GetUniformLocation(this->Shader, "length_max"), this->LengthScale);

	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( 3, GL_FLOAT, 0, pos_points);

	vtkgl::ActiveTexture( vtkgl::TEXTURE0_ARB );
	vtkgl::ClientActiveTexture(vtkgl::TEXTURE0_ARB);
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer( 3, GL_FLOAT, 0, min_points);

	vtkgl::ActiveTexture( vtkgl::TEXTURE1_ARB );
	vtkgl::ClientActiveTexture(vtkgl::TEXTURE1_ARB);
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer( 4, GL_FLOAT, 0, node_data1);

	vtkgl::ActiveTexture( vtkgl::TEXTURE2_ARB );
	vtkgl::ClientActiveTexture(vtkgl::TEXTURE2_ARB);
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer( 4, GL_FLOAT, 0, node_data2);

	num_points = 0;
}


inline void vtkOpenGLProjectedAAHexahedraMapper::RenderHexahedron(float vmin[3], float vmax[3], float scalars[8])
{
	this->pos_points[num_points * 3 + 0] = vmin[0];
	this->pos_points[num_points * 3 + 1] = vmin[1];
	this->pos_points[num_points * 3 + 2] = vmin[2];

	this->min_points[num_points * 3 + 0] = vmax[0];
	this->min_points[num_points * 3 + 1] = vmax[1];
	this->min_points[num_points * 3 + 2] = vmax[2];

	this->node_data1[num_points * 4 + 0] = scalars[0];
	this->node_data1[num_points * 4 + 1] = scalars[1];
	this->node_data1[num_points * 4 + 2] = scalars[2];
	this->node_data1[num_points * 4 + 3] = scalars[3];

	this->node_data2[num_points * 4 + 0] = scalars[4];
	this->node_data2[num_points * 4 + 1] = scalars[5];
	this->node_data2[num_points * 4 + 2] = scalars[6];
	this->node_data2[num_points * 4 + 3] = scalars[7];

	num_points++;

	// need to flush?
	if (num_points >= max_points)
	{
		glDrawArrays(GL_POINTS, 0, num_points);
		num_points=0;
	}
}

void vtkOpenGLProjectedAAHexahedraMapper::UnsetState()
{

	// flush what remains of our points
	if (num_points>0)
	{
		glDrawArrays(GL_POINTS, 0, num_points);
		num_points = 0;
	}

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	vtkgl::UseProgram(0);
}

//-----------------------------------------------------------------------------
template<class point_type>
void vtkOpenGLProjectedAAHexahedraMapperConvertScalars(const point_type *in_scalars,
						vtkIdType num_scalars,
                                                 float *out_scalars)
{
	for(int i=0;i<num_scalars;i++)
	{
		out_scalars[i] = (float)in_scalars[i];
	}
}

//-----------------------------------------------------------------------------
// convert all our scalars to floating point
float* vtkOpenGLProjectedAAHexahedraMapper::ConvertScalars(vtkDataArray* inScalars)
{
  ConvertedScalars->SetNumberOfComponents(1);
  ConvertedScalars->SetNumberOfTuples(inScalars->GetNumberOfTuples());
  switch (inScalars->GetDataType())
    {
	    vtkTemplateMacro(vtkOpenGLProjectedAAHexahedraMapperConvertScalars(
                                    (const VTK_TT *)inScalars->GetVoidPointer(0),
                                     inScalars->GetNumberOfTuples(),
                                     ConvertedScalars->GetPointer(0) ) );
    }
  return ConvertedScalars->GetPointer(0);
}

//-----------------------------------------------------------------------------
template<class point_type>
void vtkOpenGLProjectedAAHexahedraMapperConvertPoints(const point_type *in_points,
						vtkIdType num_points,
                                                 float *out_points)
{
	for(int i=0;i<num_points*3;i++)
	{
		out_points[i] = (float)in_points[i];
	}
}

//-----------------------------------------------------------------------------
// convert all our points to floating point
float* vtkOpenGLProjectedAAHexahedraMapper::ConvertPoints(vtkPoints* inPoints)
{
  ConvertedPoints->SetNumberOfComponents(3);
  ConvertedPoints->SetNumberOfTuples(inPoints->GetNumberOfPoints());
  switch (inPoints->GetDataType())
    {
	    vtkTemplateMacro(vtkOpenGLProjectedAAHexahedraMapperConvertPoints(
                                    (const VTK_TT *)inPoints->GetVoidPointer(0),
                                     inPoints->GetNumberOfPoints(),
                                     ConvertedPoints->GetPointer(0) ) );
    }
  return ConvertedPoints->GetPointer(0);
}

//-----------------------------------------------------------------------------
void vtkOpenGLProjectedAAHexahedraMapper::ProjectHexahedra(vtkRenderer *renderer,
                                                     vtkVolume *volume)
{
	vtkUnstructuredGrid *input = this->GetInput();

	this->VisibilitySort->SetInput(input);
	this->VisibilitySort->SetDirectionToBackToFront();
	this->VisibilitySort->SetModelTransform(volume->GetMatrix());
	this->VisibilitySort->SetCamera(renderer->GetActiveCamera());
	this->VisibilitySort->SetMaxCellsReturned(1000);

	double* observer = renderer->GetActiveCamera()->GetPosition();

	this->VisibilitySort->InitTraversal();

	float* points = ConvertPoints(input->GetPoints());

	float* scalars = ConvertScalars(this->GetScalars(input, this->ScalarMode,
				this->ArrayAccessMode,
				this->ArrayId, this->ArrayName,
				this->UsingCellColors) );

	if (renderer->GetRenderWindow()->CheckAbortStatus())
	{
		return;
	}

	glDisable(GL_LIGHTING);
	glDepthMask(GL_FALSE);

	// save the default blend function.
	glPushAttrib(GL_COLOR_BUFFER_BIT);

	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	this->SetState(observer);

	vtkIdType *cells = input->GetCells()->GetPointer();
	vtkIdType totalnumcells = input->GetNumberOfCells();
	vtkIdType numcellsrendered = 0;

	// Let's do it!
	for (vtkIdTypeArray *sorted_cell_ids = this->VisibilitySort->GetNextCells();
			sorted_cell_ids != NULL;
			sorted_cell_ids = this->VisibilitySort->GetNextCells())
	{
		this->UpdateProgress((double)numcellsrendered/totalnumcells);
		if (renderer->GetRenderWindow()->CheckAbortStatus())
		{
			break;
		}
		vtkIdType *cell_ids = sorted_cell_ids->GetPointer(0);
		vtkIdType num_cell_ids = sorted_cell_ids->GetNumberOfTuples();
		int num_visible_cells = 0;
		for (vtkIdType i = 0; i < num_cell_ids; i++)
		{
			vtkIdType cell = cell_ids[i];

			float corner_scalars[8];

			// get the data for the current hexahedron
			int index = cells [ 9 * cell + 1 ];
			float* p = points + 3 * index;

			float vmin[3] = {p[0],p[1],p[2]},
			      vmax[3] = {p[0],p[1],p[2]};

			int j;
			for(j = 1; j < 8; j++)
			{
				index = cells [ 9 * cell + 1 + j ];

				p = points + 3 * index;
				if (p[0]<vmin[0]) vmin[0] = p[0];
				if (p[1]<vmin[1]) vmin[1] = p[1];
				if (p[2]<vmin[2]) vmin[2] = p[2];
				if (p[0]>vmax[0]) vmax[0] = p[0];
				if (p[1]>vmax[1]) vmax[1] = p[1];
				if (p[2]>vmax[2]) vmax[2] = p[2];
			}


			float s = (scalars[index] * this->ScalarScale + this->ScalarShift + 0.5)/this->ScalarResolution;
			float mins = s;
			float maxs = s;

			corner_scalars[0] = s;

			for(j = 0; j < 8; j++)
			{
				index = cells [ 9 * cell + 1 + j ];

				p = points + 3 * index;
				int corner = 0;
				if (p[0]==vmax[0])
					corner += 4;
				if (p[1]==vmax[1])
					corner += 2;
				if (p[2]==vmax[2])
					corner += 1;
				static const int corner_tbl[] = {0, 4, 1, 5, 3, 7, 2, 6};

				s = (scalars[index] * this->ScalarScale + this->ScalarShift + 0.5)/this->ScalarResolution;
				if (s < mins) mins = s;
				if (s > maxs) maxs = s;

				corner_scalars[corner_tbl[corner]] = s;

			}

			this->RenderHexahedron(vmin,vmax,corner_scalars);

		}

		numcellsrendered += num_cell_ids;
	}

	this->UnsetState();

	// Restore the blend function.
	glPopAttrib();

	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glDepthMask(GL_TRUE);
	glEnable(GL_LIGHTING);

	this->UpdateProgress(1.0);
}

//-----------------------------------------------------------------------------

void vtkOpenGLProjectedAAHexahedraMapper::ReleaseGraphicsResources(vtkWindow *win)
{
  if (this->PreintTexture)
    {
    GLuint texid = this->PreintTexture;
    glDeleteTextures(1, &texid);
    this->PreintTexture = 0;
    }
  this->Superclass::ReleaseGraphicsResources(win);
  if(this->Initialized)
    {
    delete[] pos_points;
    delete[] min_points;
    delete[] node_data1;
    delete[] node_data2;
    this->Initialized=false;
    }
}




