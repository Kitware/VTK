/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk3DSImporter.cxx
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
#include "vtk3DSImporter.h"
#include "vtkByteSwap.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkStripper.h"
#include "vtkObjectFactory.h"

//-------------------------------------------------------------------------
vtk3DSImporter* vtk3DSImporter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtk3DSImporter");
  if(ret)
    {
    return (vtk3DSImporter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtk3DSImporter;
}

static Colour Black = {0.0, 0.0, 0.0};
static char   obj_name[80] = "";
static Colour fog_colour = {0.0, 0.0, 0.0};
static Colour col        = {0.0, 0.0, 0.0};
static Colour global_amb = {0.1, 0.1, 0.1};
static Vector pos        = {0.0, 0.0, 0.0};
static Vector target     = {0.0, 0.0, 0.0};
static float  hotspot = -1;
static float  falloff = -1;
/* Default material property */
static MatProp DefaultMaterial =
  { "Default", NULL,
    {1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}, {1.0, 1.0, 1.0},
    70.0, // shininess
    0.0,  // transparency
    0.0,  // reflection
    0,// self illumination
    "",   // tex_map
    0.0,  // tex_strength
    "",   // bump_map
    0.0,  // bump_strength
    NULL};// vtkProperty

static void cleanup_name (char *);
static void list_insert (List **root, List *new_node);
static void *list_find (List **root, const char *name);
static void list_kill (List **root);
static MatProp *create_mprop (void);
static Mesh *create_mesh (char *name, int vertices, int faces);
static int parse_3ds_file (vtk3DSImporter *importer);
static void parse_3ds (vtk3DSImporter *importer, Chunk *mainchunk);
static void parse_mdata (vtk3DSImporter *importer, Chunk *mainchunk);
static void parse_fog (vtk3DSImporter *importer, Chunk *mainchunk);
static void parse_fog_bgnd (vtk3DSImporter *importer);
static void parse_mat_entry (vtk3DSImporter *importer, Chunk *mainchunk);
static char *parse_mapname (vtk3DSImporter *importer, Chunk *mainchunk);
static void parse_named_object (vtk3DSImporter *importer, Chunk *mainchunk);
static void parse_n_tri_object (vtk3DSImporter *importer, Chunk *mainchunk);
static void parse_point_array (vtk3DSImporter *importer, Mesh *mesh);
static void parse_face_array (vtk3DSImporter *importer, Mesh *mesh, Chunk *mainchunk);
static void parse_msh_mat_group (vtk3DSImporter *importer, Mesh *mesh);
static void parse_smooth_group (vtk3DSImporter *importer);
static void parse_mesh_matrix (vtk3DSImporter *importer, Mesh *mesh);
static void parse_n_direct_light (vtk3DSImporter *importer, Chunk *mainchunk);
static void parse_dl_spotlight (vtk3DSImporter *importer);
static void parse_n_camera (vtk3DSImporter *importer);
static void parse_colour (vtk3DSImporter *importer, Colour *colour);
static void parse_colour_f (vtk3DSImporter *importer, Colour *colour);
static void parse_colour_24 (vtk3DSImporter *importer, Colour_24 *colour);
static float parse_percentage (vtk3DSImporter *importer);
static short parse_int_percentage (vtk3DSImporter *importer);
static float parse_float_percentage (vtk3DSImporter *importer);
static Material *update_materials (vtk3DSImporter *importer, const char *new_material, int ext);
static void start_chunk (vtk3DSImporter *importer, Chunk *chunk);
static void end_chunk (vtk3DSImporter *importer, Chunk *chunk);
static byte read_byte (vtk3DSImporter *importer);
static word read_word (vtk3DSImporter *importer);
static dword read_dword (vtk3DSImporter *importer);
static float read_float (vtk3DSImporter *importer);
static void read_point (vtk3DSImporter *importer, Vector v);
static char *read_string (vtk3DSImporter *importer);

vtk3DSImporter::vtk3DSImporter ()
{
  this->OmniList = NULL;
  this->SpotLightList = NULL;
  this->CameraList = NULL;
  this->MeshList = NULL;
  this->MaterialList = NULL;
  this->MatPropList = NULL;
  this->FileName = NULL;
  this->FileFD = NULL;
  this->ComputeNormals = 0;
}

int vtk3DSImporter::ImportBegin ()
{
  vtkDebugMacro(<< "Opening import file as binary");
  this->FileFD = fopen (this->FileName, "rb");
  if (this->FileFD == NULL)
    {
    vtkErrorMacro(<< "Unable to open file: "<< this->FileName);
    return 0;
    }
  return this->Read3DS ();
}

void vtk3DSImporter::ImportEnd ()
{
  vtkDebugMacro(<<"Closing import file");
  if ( this->FileFD != NULL )
    {
    fclose (this->FileFD);
    }
  this->FileFD = NULL;
}

int vtk3DSImporter::Read3DS ()
{
  MatProp *aMaterial;

  if (parse_3ds_file (this) == 0)
    {
    vtkErrorMacro (<<  "Error readings .3ds file: " << this->FileName << "\n");
    return 0;
    }


  // create a MatProp and fill if in with default
  aMaterial = (MatProp *) malloc (sizeof (MatProp));
  *aMaterial = DefaultMaterial;
  aMaterial->aProperty = vtkProperty::New ();
  VTK_LIST_INSERT (this->MatPropList, aMaterial);
  return 1;
}

void vtk3DSImporter::ImportActors (vtkRenderer *renderer)
{
  MatProp *material;
  Mesh *mesh;
  vtkStripper *polyStripper;
  vtkPolyDataNormals *polyNormals;
  vtkPolyDataMapper *polyMapper;
  vtkPolyData *polyData;
  vtkActor *actor;

  // walk the list of meshes, creating actors
  for (mesh = this->MeshList; mesh != (Mesh *) NULL; 
       mesh = (Mesh *) mesh->next)
    {
    if (mesh->faces == 0)
      {
      vtkWarningMacro (<< "part " << mesh->name << " has zero faces... skipping\n");
      continue;
      }

    polyData = this->GeneratePolyData (mesh);
    mesh->aMapper = polyMapper = vtkPolyDataMapper::New ();
    mesh->aStripper = polyStripper = vtkStripper::New ();

    // if ComputeNormals is on, insert a vtkPolyDataNormals filter
    if (this->ComputeNormals)
      {
      mesh->aNormals = polyNormals = vtkPolyDataNormals::New ();
      polyNormals->SetInput (polyData);
      polyStripper->SetInput (polyNormals->GetOutput ());
      }
    else
      {
      polyStripper->SetInput (polyData);
      }
    
    polyMapper->SetInput (polyStripper->GetOutput ());
    vtkDebugMacro (<< "Importing Actor: " << mesh->name);
    mesh->anActor = actor = vtkActor::New ();
    actor->SetMapper (polyMapper);
    material = (MatProp *)VTK_LIST_FIND(this->MatPropList, mesh->mtl[0]->name);
    actor->SetProperty (material->aProperty);
    renderer->AddActor (actor);
  }
}

vtkPolyData *vtk3DSImporter::GeneratePolyData (Mesh *mesh)
{
  int i;
  Face  *face;
  vtkCellArray *triangles;
  vtkPoints *vertices;
  vtkPolyData *polyData;

  face = mesh->face;
  mesh->aCellArray = triangles = vtkCellArray::New ();
  triangles->Allocate(mesh->faces * 3);
  for (i = 0; i < mesh->faces; i++, face++)
    {
    triangles->InsertNextCell (3);
    triangles->InsertCellPoint (face->a);
    triangles->InsertCellPoint (face->b);
    triangles->InsertCellPoint (face->c);
    }

  mesh->aPoints = vertices = vtkPoints::New ();
  vertices->Allocate(mesh->vertices);
  for (i = 0; i < mesh->vertices; i++)
    {
    vertices->InsertPoint (i, (float *) mesh->vertex[i]);
    }
  mesh->aPolyData = polyData = vtkPolyData::New ();
  polyData->SetPolys (triangles);
  polyData->SetPoints (vertices);

  return polyData;
}

void vtk3DSImporter::ImportCameras (vtkRenderer *renderer)
{
  vtkCamera *aCamera;
  Camera *camera;

  // walk the list of cameras and create vtk cameras
  for (camera = this->CameraList; camera != (Camera *) NULL; camera = (Camera *) camera->next)
    {
    camera->aCamera = aCamera = vtkCamera::New ();      
    aCamera->SetPosition (camera->pos[0], camera->pos[1], camera->pos[2]);
    aCamera->SetFocalPoint (camera->target[0], camera->target[1], camera->target[2]);
    aCamera->SetViewUp (0, 0, 1);
    aCamera->SetClippingRange (.1,10000);
    aCamera->Roll (camera->bank);
    renderer->SetActiveCamera (aCamera);
    vtkDebugMacro (<< "Importing Camera: " << camera->name);
  }
}

void vtk3DSImporter::ImportLights (vtkRenderer *renderer)
{
  OmniLight *omniLight;
  SpotLight *spotLight;
  vtkLight *aLight;

  // just walk the list of omni lights, creating vtk lights
  for (omniLight = this->OmniList; omniLight != (OmniLight *) NULL; 
       omniLight = (OmniLight *) omniLight->next)
  {
  omniLight->aLight = aLight = vtkLight::New ();
  aLight->SetPosition (omniLight->pos[0],
                       omniLight->pos[1],
                       omniLight->pos[2]);
  aLight->SetFocalPoint (0, 0, 0);
  aLight->SetColor (omniLight->col.red,
                    omniLight->col.green,
                    omniLight->col.blue);
  renderer->AddLight (aLight);
  vtkDebugMacro (<< "Importing Omni Light: " << omniLight->name);
  }                       

  // now walk the list of spot lights, creating vtk lights
  for (spotLight = this->SpotLightList; spotLight != (SpotLight *) NULL; 
       spotLight = (SpotLight *) spotLight->next)
  {
  spotLight->aLight = aLight = vtkLight::New ();
  aLight->PositionalOn ();
  aLight->SetPosition (spotLight->pos[0],
                       spotLight->pos[1],
                       spotLight->pos[2]);
  aLight->SetFocalPoint (spotLight->target[0],
                         spotLight->target[1],
                         spotLight->target[2]);
  aLight->SetColor (spotLight->col.red,
                    spotLight->col.green,
                    spotLight->col.blue);
  aLight->SetConeAngle (spotLight->falloff);
  renderer->AddLight (aLight);
  vtkDebugMacro (<< "Importing Spot Light: " << spotLight->name);
  }                       
}

void vtk3DSImporter::ImportProperties (vtkRenderer *vtkNotUsed(renderer))
{
  float amb = 0.1, dif = 0.9;
  float dist_white, dist_diff, phong, phong_size;
  vtkProperty *property;
  MatProp *m;

  // just walk the list of material properties, creating vtk properties
  for (m = this->MatPropList; m != (MatProp *) NULL; m = (MatProp *) m->next)
    {
    if (m->self_illum)
      {
      amb = 0.9;
      dif = 0.1;
      }

    dist_white = fabs(1.0 - m->specular.red) +
         fabs(1.0 - m->specular.green) +
         fabs(1.0 - m->specular.blue);

    dist_diff  = fabs(m->diffuse.red   - m->specular.red) +
         fabs(m->diffuse.green - m->specular.green) +
         fabs(m->diffuse.blue  - m->specular.blue);

    if (dist_diff < dist_white)
      {
      dif = .1; amb = .8;
      }

    phong_size = 0.7*m->shininess;
    if (phong_size < 1.0)
      {
      phong_size = 1.0;
      }
    if (phong_size > 30.0)
      {
      phong = 1.0;
      }
    else
      {
      phong = phong_size/30.0;
      }
  property = m->aProperty;
  property->SetAmbientColor(m->ambient.red, m->ambient.green, m->ambient.blue);
  property->SetAmbient (amb);
  property->SetDiffuseColor(m->diffuse.red, m->diffuse.green, m->diffuse.blue);
  property->SetDiffuse (dif);
  property->SetSpecularColor(m->specular.red, m->specular.green, m->specular.blue);
  property->SetSpecular (phong);
  property->SetSpecularPower(phong_size);
  property->SetOpacity(1.0 - m->transparency);
  vtkDebugMacro(<< "Importing Property: " << m->name);
  
  m->aProperty = property;
  }
}

/* Insert a new node into the list */
static void list_insert (List **root, List *new_node)
{
  new_node->next = *root;
  *root = new_node;
}


/* Find the node with the specified name */
static void *list_find (List **root, const char *name)
{
  List *p;
  for (p = *root; p != (List *) NULL; p = (List *) p->next)
    {
    if (strcmp (p->name, name) == 0)
      {
      break;
      }
    }
  return (void *)p;
}

/* Delete the entire list */
static void list_kill (List **root)
{
  List *temp;

  while (*root != (List *) NULL) 
    {
    temp = *root;
    *root = (List *) (*root)->next;
    free (temp);
    }
}

/* Add a new material to the material list */
static Material *update_materials (vtk3DSImporter *importer, const char *new_material, int ext)
{
  Material *p;

  p = (Material *) VTK_LIST_FIND (importer->MaterialList, new_material);

  if (p == NULL)
    {
    p = (Material *) malloc (sizeof (*p));
    strcpy (p->name, new_material);
    p->external = ext;
    VTK_LIST_INSERT (importer->MaterialList, p);
    }
  return p;
}


static MatProp *create_mprop()
{
  MatProp *new_mprop;

  new_mprop = (MatProp *) malloc (sizeof(*new_mprop));
  strcpy (new_mprop->name, "");
  new_mprop->ambient = Black;
  new_mprop->diffuse = Black;
  new_mprop->specular = Black;
  new_mprop->shininess = 0.0;
  new_mprop->transparency = 0.0;
  new_mprop->reflection = 0.0;
  new_mprop->self_illum = 0;

  strcpy (new_mprop->tex_map, "");
  new_mprop->tex_strength = 0.0;

  strcpy (new_mprop->bump_map, "");
  new_mprop->bump_strength = 0.0;

  new_mprop->aProperty = vtkProperty::New ();
  return new_mprop;
}


/* Create a new mesh */
static Mesh *create_mesh (char *name, int vertices, int faces)
{
  Mesh *new_mesh;

  new_mesh = (Mesh *) malloc (sizeof(*new_mesh));
  strcpy (new_mesh->name, name);

  new_mesh->vertices = vertices;

  if (vertices <= 0)
    {
    new_mesh->vertex = NULL;
    }
  else
    {
    new_mesh->vertex = (Vector *) malloc(vertices * sizeof(*new_mesh->vertex));
    }

  new_mesh->faces = faces;

  if (faces <= 0)
    {
    new_mesh->face = NULL;
    new_mesh->mtl = NULL;
    }
  else
    {
    new_mesh->face = (Face *) malloc (faces * sizeof(*new_mesh->face));
    new_mesh->mtl = (Material **) malloc (faces * sizeof(*new_mesh->mtl));
    }

  new_mesh->hidden = 0;
  new_mesh->shadow = 1;

  new_mesh->anActor = NULL;
  new_mesh->aMapper = NULL;
  new_mesh->aNormals = NULL;
  new_mesh->aStripper = NULL;
  new_mesh->aPoints = NULL;
  new_mesh->aCellArray = NULL;
  new_mesh->aPolyData = NULL;
  return new_mesh;
}


static int parse_3ds_file(vtk3DSImporter *importer)
{
  Chunk chunk;

  start_chunk(importer, &chunk);

  if (chunk.tag == 0x4D4D)
    {
    parse_3ds (importer, &chunk);
    }
  else
    {
    vtkGenericWarningMacro(<< "Error: Input file is not .3DS format\n");
    return 0;
    }

  end_chunk (importer, &chunk);
  return 1;
}

static void parse_3ds (vtk3DSImporter *importer, Chunk *mainchunk)
{
  Chunk chunk;

  do  
    {
    start_chunk (importer, &chunk);

    if (chunk.end <= mainchunk->end)
      {
      switch (chunk.tag)
        {
        case 0x3D3D: parse_mdata (importer, &chunk);
          break;
        }
      }
    end_chunk (importer, &chunk);
    } while (chunk.end <= mainchunk->end);
}


static void parse_mdata (vtk3DSImporter *importer, Chunk *mainchunk)
{
  Chunk chunk;
  Colour bgnd_colour;

  do  
    {
    start_chunk (importer, &chunk);
    
    if (chunk.end <= mainchunk->end) 
      {
      switch (chunk.tag) 
        {
        case 0x2100: parse_colour (importer, &global_amb);
          break;
        case 0x1200: parse_colour (importer, &bgnd_colour);
          break;
        case 0x2200: parse_fog (importer, &chunk);
          break;
        case 0x2210: parse_fog_bgnd(importer);
          break;
        case 0xAFFF: parse_mat_entry (importer, &chunk);
          break;
        case 0x4000: parse_named_object (importer, &chunk);
          break;
        }
      }

    end_chunk (importer, &chunk);
    } while (chunk.end <= mainchunk->end);
}


static void parse_fog (vtk3DSImporter *importer, Chunk *mainchunk)
{
  Chunk chunk;

  (void)read_float(importer);
  (void)read_float(importer);
  (void) read_float(importer);
  (void)read_float(importer);

  parse_colour (importer, &fog_colour);

  do  
    {
    start_chunk (importer, &chunk);

    if (chunk.end <= mainchunk->end) 
      {
      switch (chunk.tag) 
        {
        case 0x2210: parse_fog_bgnd(importer);
          break;
        }
      }

    end_chunk (importer, &chunk);
    } while (chunk.end <= mainchunk->end);
}


static void parse_fog_bgnd(vtk3DSImporter *vtkNotUsed(importer))
{
}


static void parse_mat_entry (vtk3DSImporter *importer, Chunk *mainchunk)
{
  Chunk chunk;
  MatProp *mprop;

  mprop = create_mprop();

  do  
    {
    start_chunk (importer, &chunk);
    if (chunk.end <= mainchunk->end) 
      {
      switch (chunk.tag) 
        {
        case 0xA000: strcpy (mprop->name, read_string(importer));
          cleanup_name (mprop->name);
          break;

        case 0xA010: parse_colour (importer, &mprop->ambient);
          break;

        case 0xA020: parse_colour (importer, &mprop->diffuse);
          break;

        case 0xA030: parse_colour (importer, &mprop->specular);
          break;

        case 0xA040: mprop->shininess = 100.0*parse_percentage(importer);
          break;

        case 0xA050: mprop->transparency = parse_percentage(importer);
          break;

        case 0xA080: mprop->self_illum = 1;
          break;

        case 0xA220: mprop->reflection = parse_percentage(importer);
          (void)parse_mapname (importer, &chunk);
          break;

        case 0xA310: if (mprop->reflection == 0.0)
          {
          mprop->reflection = 1.0;
          }
        break;

        case 0xA200: mprop->tex_strength = parse_percentage(importer);
          strcpy (mprop->tex_map, parse_mapname (importer, &chunk));
          break;

        case 0xA230: mprop->bump_strength = parse_percentage(importer);
          strcpy (mprop->bump_map, parse_mapname (importer, &chunk));
          break;
        }
      }

    end_chunk (importer, &chunk);
    } while (chunk.end <= mainchunk->end);

    VTK_LIST_INSERT (importer->MatPropList, mprop);
}


static char *parse_mapname (vtk3DSImporter *importer, Chunk *mainchunk)
{
  static char name[80] = "";
  Chunk chunk;

  do  
    {
    start_chunk (importer, &chunk);

    if (chunk.end <= mainchunk->end) 
      {
      switch (chunk.tag) 
        {
        case 0xA300: strcpy (name, read_string(importer));
          break;
        }
      }

    end_chunk (importer, &chunk);
    } while (chunk.end <= mainchunk->end);

    return name;
}


static void parse_named_object (vtk3DSImporter *importer, Chunk *mainchunk)
{
  Mesh *mesh;
  Chunk chunk;

  strcpy (obj_name, read_string(importer));
  cleanup_name (obj_name);

  mesh = NULL;

  do  
    {
    start_chunk (importer, &chunk);
    if (chunk.end <= mainchunk->end) 
      {
      switch (chunk.tag) 
        {
        case 0x4100: parse_n_tri_object (importer, &chunk);
          break;
        case 0x4600: parse_n_direct_light (importer, &chunk);
          break;
        case 0x4700: parse_n_camera(importer);
          break;
        case 0x4010: if (mesh != NULL)
          {
          mesh->hidden = 1;
          }
        break;
        case 0x4012: if (mesh != NULL)
          {
          mesh->shadow = 0;
          }
        break;
        }
      }

    end_chunk (importer, &chunk);
    } while (chunk.end <= mainchunk->end);

}

static void parse_n_tri_object (vtk3DSImporter *importer, Chunk *mainchunk)
{
  Mesh *mesh;
  Chunk chunk;

  mesh = create_mesh (obj_name, 0, 0);

  do  
    {
    start_chunk (importer, &chunk);

    if (chunk.end <= mainchunk->end) 
      {
      switch (chunk.tag) 
        {
        case 0x4110: parse_point_array(importer, mesh);
          break;
        case 0x4120: parse_face_array (importer, mesh, &chunk);
          break;
        case 0x4160: parse_mesh_matrix(importer, mesh);
          break;
        }
      }

    end_chunk (importer, &chunk);
    } while (chunk.end <= mainchunk->end);

  VTK_LIST_INSERT (importer->MeshList, mesh);
}


static void parse_point_array(vtk3DSImporter *importer, Mesh *mesh)
{
  int i;

  mesh->vertices = read_word(importer);
  mesh->vertex = (Vector *) malloc (mesh->vertices * sizeof(*(mesh->vertex)));
  for (i = 0; i < mesh->vertices; i++)
    {
    read_point (importer, mesh->vertex[i]);
    }
}

static void parse_face_array (vtk3DSImporter *importer, Mesh *mesh, Chunk *mainchunk)
{
  Chunk chunk;
  int i;

  mesh->faces = read_word(importer);
  mesh->face = (Face *) malloc (mesh->faces * sizeof(*(mesh->face)));
  mesh->mtl = (Material **) malloc (mesh->faces * sizeof(*(mesh->mtl)));

  for (i = 0; i < mesh->faces; i++) 
    {
    mesh->face[i].a = read_word(importer);
    mesh->face[i].b = read_word(importer);
    mesh->face[i].c = read_word(importer);
    (void)read_word(importer);
    
    mesh->mtl[i] = NULL;
    }

  do  
    {
    start_chunk (importer, &chunk);
    if (chunk.end <= mainchunk->end) 
      {
      switch (chunk.tag) 
        {
        case 0x4130: parse_msh_mat_group(importer, mesh);
          break;
        case 0x4150: parse_smooth_group(importer);
          break;
        }
      }

    end_chunk (importer, &chunk);
    } while (chunk.end <= mainchunk->end);

  for (i = 0; i < mesh->faces; i++)
    {
    if (mesh->mtl[i] == (Material *) NULL)
      {
      mesh->mtl[i] = update_materials (importer, "Default", 0);
      }
    }
}


static void parse_msh_mat_group(vtk3DSImporter *importer, Mesh *mesh)
{
  Material *new_mtl;
  char mtlname[80];
  int  mtlcnt;
  int  i, face;

  strcpy (mtlname, read_string(importer));
  cleanup_name (mtlname);

  new_mtl = update_materials (importer, mtlname, 0);

  mtlcnt = read_word(importer);

  for (i = 0; i < mtlcnt; i++) 
    {
    face = read_word(importer);
    mesh->mtl[face] = new_mtl;
    }
}

static void parse_smooth_group(vtk3DSImporter *vtkNotUsed(importer))
{
}

static void parse_mesh_matrix(vtk3DSImporter *vtkNotUsed(importer), Mesh *vtkNotUsed(mesh))
{
  //  vtkGenericWarningMacro(<< "mesh matrix detected but not used\n");
}


static void parse_n_direct_light (vtk3DSImporter *importer, Chunk *mainchunk)
{
  Chunk chunk;
  SpotLight *s;
  OmniLight *o;
  int spot_flag = 0;

  read_point (importer, pos);
  parse_colour (importer, &col);

  do  
    {
    start_chunk (importer, &chunk);

    if (chunk.end <= mainchunk->end) 
      {
      switch (chunk.tag) 
        {
        case 0x4620: break;
        case 0x4610: parse_dl_spotlight(importer);
          spot_flag = 1;
          break;
        }
      }

    end_chunk (importer, &chunk);
    } while (chunk.end <= mainchunk->end);

  if (!spot_flag)
    {
    o = (OmniLight *) VTK_LIST_FIND (importer->OmniList, obj_name);

    if (o != NULL)
      {
      pos[0] = o->pos[0];
      pos[1] = o->pos[1];
      pos[2] = o->pos[2];
      col    = o->col;
      }
    else
      {
      o = (OmniLight *) malloc (sizeof (*o));
      o->pos[0] = pos[0];
      o->pos[1] = pos[1];
      o->pos[2] = pos[2];
      o->col = col   ;
      strcpy (o->name, obj_name);
      VTK_LIST_INSERT (importer->OmniList, o);
      }
    }
  else
    {
    s = (SpotLight *) VTK_LIST_FIND (importer->SpotLightList, obj_name);

    if (s != NULL)
      {
      pos[0]    = s->pos[0];
      pos[1]    = s->pos[1];
      pos[2]    = s->pos[2];
      target[0] = s->target[0];
      target[1] = s->target[1];
      target[2] = s->target[2];
      col       = s->col;
      hotspot   = s->hotspot;
      falloff   = s->falloff;
      }
    else
      {
      if (falloff <= 0.0)
        {
        falloff = 180.0;
        }
      if (hotspot <= 0.0)
        {
        hotspot = 0.7*falloff;
        }
      s = (SpotLight *) malloc (sizeof (*s));
      s->pos[0] = pos[0];
      s->pos[1] = pos[1];
      s->pos[2] = pos[2];
      s->target[0] = target[0];
      s->target[1] = target[1];
      s->target[2] = target[2];
      s->col = col   ;
      s->hotspot = hotspot;
      s->falloff = falloff;
      strcpy (s->name, obj_name);
      VTK_LIST_INSERT (importer->SpotLightList, s);
      }
    }
}


static void parse_dl_spotlight(vtk3DSImporter *importer)
{
  read_point (importer, target);

  hotspot = read_float(importer);
  falloff = read_float(importer);
}


static void parse_n_camera(vtk3DSImporter *importer)
{
  float  bank;
  float  lens;
  Camera *c = (Camera *) malloc (sizeof (Camera));

  read_point (importer, pos);
  read_point (importer, target);
  bank = read_float(importer);
  lens = read_float(importer);

  strcpy (c->name, obj_name);
  c->pos[0] = pos[0];
  c->pos[1] = pos[1];
  c->pos[2] = pos[2];
  c->target[0] = target[0];
  c->target[1] = target[1];
  c->target[2] = target[2];
  c->lens = lens;
  c->bank = bank;

  VTK_LIST_INSERT (importer->CameraList, c);
}

static void parse_colour (vtk3DSImporter *importer, Colour *colour)
{
  Chunk chunk;
  Colour_24 colour_24;

  start_chunk (importer, &chunk);

  switch (chunk.tag) 
    {
    case 0x0010: parse_colour_f (importer, colour);
      break;

    case 0x0011: parse_colour_24 (importer, &colour_24);
      colour->red   = colour_24.red/255.0;
      colour->green = colour_24.green/255.0;
      colour->blue  = colour_24.blue/255.0;
      break;

    default: vtkGenericWarningMacro(<< "Error parsing colour");
    }

  end_chunk (importer, &chunk);
}


static void parse_colour_f (vtk3DSImporter *importer, Colour *colour)
{
  colour->red   = read_float(importer);
  colour->green = read_float(importer);
  colour->blue  = read_float(importer);
}


static void parse_colour_24 (vtk3DSImporter *importer, Colour_24 *colour)
{
  colour->red   = read_byte(importer);
  colour->green = read_byte(importer);
  colour->blue  = read_byte(importer);
}


static float parse_percentage(vtk3DSImporter *importer)
{
  Chunk chunk;
  float percent = 0.0;

  start_chunk (importer, &chunk);

  switch (chunk.tag) 
    {
    case 0x0030: percent = parse_int_percentage(importer)/100.0;
      break;

    case 0x0031: percent = parse_float_percentage(importer);
      break;

    default:     vtkGenericWarningMacro( << "Error parsing percentage\n");
    }

  end_chunk (importer, &chunk);

  return percent;
}


static short parse_int_percentage(vtk3DSImporter *importer)
{
  word percent = read_word(importer);

  return percent;
}


static float parse_float_percentage(vtk3DSImporter *importer)
{
  float percent = read_float(importer);

  return percent;
}


static void start_chunk (vtk3DSImporter *importer, Chunk *chunk)
{
  chunk->start  = ftell(importer->GetFileFD());
  chunk->tag    = read_word(importer);
  chunk->length = read_dword(importer);
  if (chunk->length == 0)
    {
    chunk->length = 1;
    }
  chunk->end    = chunk->start + chunk->length;
}


static void end_chunk (vtk3DSImporter *importer, Chunk *chunk)
{
  fseek (importer->GetFileFD(), chunk->end, 0);
}


static byte read_byte(vtk3DSImporter *importer)
{
  byte data;

  data = fgetc (importer->GetFileFD());

  return data;
}


static word read_word(vtk3DSImporter *importer)
{
  word data;

  fread (&data, 2, 1, importer->GetFileFD());
  vtkByteSwap::Swap2LE ((short *) &data);
/*    swab ((char *) &data, (char *) &sdata, 2);*/

  return data;
}

static dword read_dword(vtk3DSImporter *importer)
{
  dword data;

  if (fread (&data, 4, 1, importer->GetFileFD()) != 1)
    {
//    vtkGenericWarningMacro(<<"Pre-mature end of file in read_dword\n");
    data = 0;
    }

  vtkByteSwap::Swap4LE ((char *) &data);
  return data;
}


static float read_float(vtk3DSImporter *importer)
{
  float data;

  fread (&data, 4, 1, importer->GetFileFD());
  vtkByteSwap::Swap4LE ((char *) &data);
/*    TIFFSwabLong (&data);*/

    return data;
}


static void read_point (vtk3DSImporter *importer, Vector v)
{
  v[0] = read_float(importer);
  v[1] = read_float(importer);
  v[2] = read_float(importer);
}


static char *read_string(vtk3DSImporter *importer)
{
  static char string[80];
  int i;

  for (i = 0; i < 80; i++)
    {
    string[i] = read_byte(importer);

    if (string[i] == '\0')
      {
      break;
      }
    }

    return string;
}



static void cleanup_name (char *name)
{
  char *tmp = (char *) malloc (strlen(name)+2);
  int  i;

    /* Remove any leading blanks or quotes */
  i = 0;
  while ((name[i] == ' ' || name[i] == '"') && name[i] != '\0')
    {
    i++;
    }
  strcpy (tmp, &name[i]);

    /* Remove any trailing blanks or quotes */
  for (i = strlen(tmp)-1; i >= 0; i--)
    {
    if (isprint(tmp[i]) && !isspace(tmp[i]) && tmp[i] != '"')
      {
      break;
      }
    else
      {
      tmp[i] = '\0';
      }
    }

    strcpy (name, tmp);

    /* Prefix the letter 'N' to materials that begin with a digit */
    if (!isdigit (name[0]))
      {
      strcpy (tmp, name);
      }
    else 
      {
      tmp[0] = 'N';
      strcpy (&tmp[1], name);
      }

    /* Replace all illegal charaters in name with underscores */
    for (i = 0; tmp[i] != '\0'; i++)
      {
      if (!isalnum(tmp[i]))
        {
        tmp[i] = '_';
        }
      }

    strcpy (name, tmp);

    free (tmp);
}

vtk3DSImporter::~vtk3DSImporter()
{
  OmniLight *omniLight;
  SpotLight *spotLight;

  // walk the light list and delete vtk objects
  for (omniLight = this->OmniList; omniLight != (OmniLight *) NULL; omniLight = (OmniLight *) omniLight->next)
    {
    omniLight->aLight->Delete();
    }
  VTK_LIST_KILL (this->OmniList);

  // walk the spot light list and delete vtk objects
  for (spotLight = this->SpotLightList; spotLight != (SpotLight *) NULL; 
       spotLight = (SpotLight *) spotLight->next)
    {
    spotLight->aLight->Delete();
    }
  VTK_LIST_KILL (this->SpotLightList);

  Camera *camera;
  // walk the camera list and delete vtk objects
  for (camera = this->CameraList; camera != (Camera *) NULL; 
       camera = (Camera *) camera->next)
    {
    camera->aCamera->Delete ();
    }
  VTK_LIST_KILL (this->CameraList);

  // walk the mesh list and delete malloced datra and vtk objects
  Mesh *mesh;
  for (mesh = this->MeshList; mesh != (Mesh *) NULL; 
       mesh = (Mesh *) mesh->next)
    {
    if (mesh->anActor != NULL)
      {
      mesh->anActor->Delete ();
      }
    if (mesh->aMapper != NULL)
      {
      mesh->aMapper->Delete ();
      }
    if (mesh->aNormals != NULL)
      {
      mesh->aNormals->Delete ();
      }
    if (mesh->aStripper != NULL)
      {
      mesh->aStripper->Delete ();
      }
    if (mesh->aPoints != NULL)
      {
      mesh->aPoints->Delete ();
      }
    if (mesh->aCellArray != NULL)
      {
      mesh->aCellArray->Delete ();
      }
    if (mesh->aPolyData != NULL)
      {
      mesh->aPolyData->Delete ();
      }
    if (mesh->vertex)
      {
      free (mesh->vertex);
      }
    if (mesh->face)
      {
      free (mesh->face);
      }
    if (mesh->mtl)
      {
      free (mesh->mtl);  
      }
    }

  // then delete the list structure

  VTK_LIST_KILL (this->MeshList);
  VTK_LIST_KILL (this->MaterialList);

  // objects allocated in Material Property List
  MatProp *m;
  // just walk the list of material properties, deleting vtk properties
  for (m = this->MatPropList; m != (MatProp *) NULL; m = (MatProp *) m->next)
    {
    m->aProperty->Delete();
    }

  // then delete the list structure
  VTK_LIST_KILL (this->MatPropList);

  if (this->FileName)
    {
    delete [] this->FileName;
    }
}

void vtk3DSImporter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImporter::PrintSelf(os,indent);
  os << indent << "File Name: " 
     << (this->FileName ? this->FileName : "(none)") << "\n";

  os << indent << "Compute Normals: " 
     << (this->ComputeNormals ? "On\n" : "Off\n");
}






