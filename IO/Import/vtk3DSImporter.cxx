/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk3DSImporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtk3DSImporter.h"

#include "vtkActor.h"
#include "vtkByteSwap.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkLight.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkStripper.h"

vtkStandardNewMacro(vtk3DSImporter);

// Silence warning like
// "dereferencing type-punned pointer will break strict-aliasing rules"
// This file just has too many of them.
// This is due to the use of (vtk3DSList **)&root in VTK_LIST_* macros
// defined in vtk3DS.h
// pragma GCC diagnostic is available since gcc>=4.2
#if defined(__GNUC__) && (__GNUC__>4) || (__GNUC__==4 && __GNUC_MINOR__>=2)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

static vtk3DSColour Black = {0.0, 0.0, 0.0};
static char   obj_name[80] = "";
static vtk3DSColour fog_colour = {0.0, 0.0, 0.0};
static vtk3DSColour col        = {0.0, 0.0, 0.0};
static vtk3DSColour global_amb = {0.1, 0.1, 0.1};
static vtk3DSVector pos        = {0.0, 0.0, 0.0};
static vtk3DSVector target     = {0.0, 0.0, 0.0};
static float  hotspot = -1;
static float  falloff = -1;
/* Default material property */
static vtk3DSMatProp DefaultMaterial =
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
static void list_insert (vtk3DSList **root, vtk3DSList *new_node);
static void *list_find (vtk3DSList **root, const char *name);
static void list_kill (vtk3DSList **root);
static vtk3DSMatProp *create_mprop (void);
static vtk3DSMesh *create_mesh (char *name, int vertices, int faces);
static int parse_3ds_file (vtk3DSImporter *importer);
static void parse_3ds (vtk3DSImporter *importer, vtk3DSChunk *mainchunk);
static void parse_mdata (vtk3DSImporter *importer, vtk3DSChunk *mainchunk);
static void parse_fog (vtk3DSImporter *importer, vtk3DSChunk *mainchunk);
static void parse_fog_bgnd (vtk3DSImporter *importer);
static void parse_mat_entry (vtk3DSImporter *importer, vtk3DSChunk *mainchunk);
static char *parse_mapname (vtk3DSImporter *importer, vtk3DSChunk *mainchunk);
static void parse_named_object (vtk3DSImporter *importer, vtk3DSChunk *mainchunk);
static void parse_n_tri_object (vtk3DSImporter *importer, vtk3DSChunk *mainchunk);
static void parse_point_array (vtk3DSImporter *importer, vtk3DSMesh *mesh);
static void parse_face_array (vtk3DSImporter *importer, vtk3DSMesh *mesh, vtk3DSChunk *mainchunk);
static void parse_msh_mat_group (vtk3DSImporter *importer, vtk3DSMesh *mesh);
static void parse_smooth_group (vtk3DSImporter *importer);
static void parse_mesh_matrix (vtk3DSImporter *importer, vtk3DSMesh *mesh);
static void parse_n_direct_light (vtk3DSImporter *importer, vtk3DSChunk *mainchunk);
static void parse_dl_spotlight (vtk3DSImporter *importer);
static void parse_n_camera (vtk3DSImporter *importer);
static void parse_colour (vtk3DSImporter *importer, vtk3DSColour *colour);
static void parse_colour_f (vtk3DSImporter *importer, vtk3DSColour *colour);
static void parse_colour_24 (vtk3DSImporter *importer, vtk3DSColour_24 *colour);
static float parse_percentage (vtk3DSImporter *importer);
static short parse_int_percentage (vtk3DSImporter *importer);
static float parse_float_percentage (vtk3DSImporter *importer);
static vtk3DSMaterial *update_materials (vtk3DSImporter *importer, const char *new_material, int ext);
static void start_chunk (vtk3DSImporter *importer, vtk3DSChunk *chunk);
static void end_chunk (vtk3DSImporter *importer, vtk3DSChunk *chunk);
static byte read_byte (vtk3DSImporter *importer);
static word read_word (vtk3DSImporter *importer);
static word peek_word (vtk3DSImporter *importer);
static dword peek_dword (vtk3DSImporter *importer);
static float read_float (vtk3DSImporter *importer);
static void read_point (vtk3DSImporter *importer, vtk3DSVector v);
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
  vtk3DSMatProp *aMaterial;

  if (parse_3ds_file (this) == 0)
    {
    vtkErrorMacro (<<  "Error readings .3ds file: " << this->FileName << "\n");
    return 0;
    }


  // create a vtk3DSMatProp and fill if in with default
  aMaterial = (vtk3DSMatProp *) malloc (sizeof (vtk3DSMatProp));
  *aMaterial = DefaultMaterial;
  aMaterial->aProperty = vtkProperty::New ();
  VTK_LIST_INSERT (this->MatPropList, aMaterial);
  return 1;
}

void vtk3DSImporter::ImportActors (vtkRenderer *renderer)
{
  vtk3DSMatProp *material;
  vtk3DSMesh *mesh;
  vtkStripper *polyStripper;
  vtkPolyDataNormals *polyNormals;
  vtkPolyDataMapper *polyMapper;
  vtkPolyData *polyData;
  vtkActor *actor;

  // walk the list of meshes, creating actors
  for (mesh = this->MeshList; mesh != (vtk3DSMesh *) NULL;
       mesh = (vtk3DSMesh *) mesh->next)
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
      polyNormals->SetInputData (polyData);
      polyStripper->SetInputConnection(polyNormals->GetOutputPort());
      }
    else
      {
      polyStripper->SetInputData(polyData);
      }

    polyMapper->SetInputConnection(polyStripper->GetOutputPort());
    vtkDebugMacro (<< "Importing Actor: " << mesh->name);
    mesh->anActor = actor = vtkActor::New ();
    actor->SetMapper (polyMapper);
    material = (vtk3DSMatProp *)VTK_LIST_FIND(this->MatPropList, mesh->mtl[0]->name);
    actor->SetProperty (material->aProperty);
    renderer->AddActor (actor);
  }
}

vtkPolyData *vtk3DSImporter::GeneratePolyData (vtk3DSMesh *mesh)
{
  int i;
  vtk3DSFace  *face;
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
  vtk3DSCamera *camera;

  // walk the list of cameras and create vtk cameras
  for (camera = this->CameraList; camera != (vtk3DSCamera *) NULL; camera = (vtk3DSCamera *) camera->next)
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
  vtk3DSOmniLight *omniLight;
  vtk3DSSpotLight *spotLight;
  vtkLight *aLight;

  // just walk the list of omni lights, creating vtk lights
  for (omniLight = this->OmniList; omniLight != (vtk3DSOmniLight *) NULL;
       omniLight = (vtk3DSOmniLight *) omniLight->next)
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
  for (spotLight = this->SpotLightList; spotLight != (vtk3DSSpotLight *) NULL;
       spotLight = (vtk3DSSpotLight *) spotLight->next)
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
  vtk3DSMatProp *m;

  // just walk the list of material properties, creating vtk properties
  for (m = this->MatPropList; m != (vtk3DSMatProp *) NULL; m = (vtk3DSMatProp *) m->next)
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
static void list_insert (vtk3DSList **root, vtk3DSList *new_node)
{
  new_node->next = *root;
  *root = new_node;
}


/* Find the node with the specified name */
static void *list_find (vtk3DSList **root, const char *name)
{
  vtk3DSList *p;
  for (p = *root; p != (vtk3DSList *) NULL; p = (vtk3DSList *) p->next)
    {
    if (strcmp (p->name, name) == 0)
      {
      break;
      }
    }
  return (void *)p;
}

/* Delete the entire list */
static void list_kill (vtk3DSList **root)
{
  vtk3DSList *temp;

  while (*root != (vtk3DSList *) NULL)
    {
    temp = *root;
    *root = (vtk3DSList *) (*root)->next;
    free (temp);
    }
}

/* Add a new material to the material list */
static vtk3DSMaterial *update_materials (vtk3DSImporter *importer, const char *new_material, int ext)
{
  vtk3DSMaterial *p;

  p = (vtk3DSMaterial *) VTK_LIST_FIND (importer->MaterialList, new_material);

  if (p == NULL)
    {
    p = (vtk3DSMaterial *) malloc (sizeof (*p));
    strcpy (p->name, new_material);
    p->external = ext;
    VTK_LIST_INSERT (importer->MaterialList, p);
    }
  return p;
}


static vtk3DSMatProp *create_mprop()
{
  vtk3DSMatProp *new_mprop;

  new_mprop = (vtk3DSMatProp *) malloc (sizeof(*new_mprop));
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
static vtk3DSMesh *create_mesh (char *name, int vertices, int faces)
{
  vtk3DSMesh *new_mesh;

  new_mesh = (vtk3DSMesh *) malloc (sizeof(*new_mesh));
  strcpy (new_mesh->name, name);

  new_mesh->vertices = vertices;

  if (vertices <= 0)
    {
    new_mesh->vertex = NULL;
    }
  else
    {
    new_mesh->vertex = (vtk3DSVector *) malloc(vertices * sizeof(*new_mesh->vertex));
    }

  new_mesh->faces = faces;

  if (faces <= 0)
    {
    new_mesh->face = NULL;
    new_mesh->mtl = NULL;
    }
  else
    {
    new_mesh->face = (vtk3DSFace *) malloc (faces * sizeof(*new_mesh->face));
    new_mesh->mtl = (vtk3DSMaterial **) malloc (faces * sizeof(*new_mesh->mtl));
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
  vtk3DSChunk chunk;

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

static void parse_3ds (vtk3DSImporter *importer, vtk3DSChunk *mainchunk)
{
  vtk3DSChunk chunk;

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


static void parse_mdata (vtk3DSImporter *importer, vtk3DSChunk *mainchunk)
{
  vtk3DSChunk chunk;
  vtk3DSColour bgnd_colour;

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


static void parse_fog (vtk3DSImporter *importer, vtk3DSChunk *mainchunk)
{
  vtk3DSChunk chunk;

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


static void parse_mat_entry (vtk3DSImporter *importer, vtk3DSChunk *mainchunk)
{
  vtk3DSChunk chunk;
  vtk3DSMatProp *mprop;

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


static char *parse_mapname (vtk3DSImporter *importer, vtk3DSChunk *mainchunk)
{
  static char name[80] = "";
  vtk3DSChunk chunk;

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


static void parse_named_object (vtk3DSImporter *importer, vtk3DSChunk *mainchunk)
{
  vtk3DSMesh *mesh;
  vtk3DSChunk chunk;

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

static void parse_n_tri_object (vtk3DSImporter *importer, vtk3DSChunk *mainchunk)
{
  vtk3DSMesh *mesh;
  vtk3DSChunk chunk;

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


static void parse_point_array(vtk3DSImporter *importer, vtk3DSMesh *mesh)
{
  int i;

  mesh->vertices = read_word(importer);
  mesh->vertex = (vtk3DSVector *) malloc (mesh->vertices * sizeof(*(mesh->vertex)));
  for (i = 0; i < mesh->vertices; i++)
    {
    read_point (importer, mesh->vertex[i]);
    }
}

static void parse_face_array (vtk3DSImporter *importer, vtk3DSMesh *mesh, vtk3DSChunk *mainchunk)
{
  vtk3DSChunk chunk;
  int i;

  mesh->faces = read_word(importer);
  mesh->face = (vtk3DSFace *) malloc (mesh->faces * sizeof(*(mesh->face)));
  mesh->mtl = (vtk3DSMaterial **) malloc (mesh->faces * sizeof(*(mesh->mtl)));

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
    if (mesh->mtl[i] == (vtk3DSMaterial *) NULL)
      {
      mesh->mtl[i] = update_materials (importer, "Default", 0);
      }
    }
}


static void parse_msh_mat_group(vtk3DSImporter *importer, vtk3DSMesh *mesh)
{
  vtk3DSMaterial *new_mtl;
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

static void parse_mesh_matrix(vtk3DSImporter *vtkNotUsed(importer), vtk3DSMesh *vtkNotUsed(mesh))
{
  //  vtkGenericWarningMacro(<< "mesh matrix detected but not used\n");
}


static void parse_n_direct_light (vtk3DSImporter *importer, vtk3DSChunk *mainchunk)
{
  vtk3DSChunk chunk;
  vtk3DSSpotLight *s;
  vtk3DSOmniLight *o;
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
    o = (vtk3DSOmniLight *) VTK_LIST_FIND (importer->OmniList, obj_name);

    if (o != NULL)
      {
      pos[0] = o->pos[0];
      pos[1] = o->pos[1];
      pos[2] = o->pos[2];
      col    = o->col;
      }
    else
      {
      o = (vtk3DSOmniLight *) malloc (sizeof (*o));
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
    s = (vtk3DSSpotLight *) VTK_LIST_FIND (importer->SpotLightList, obj_name);

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
      s = (vtk3DSSpotLight *) malloc (sizeof (*s));
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
  vtk3DSCamera *c = (vtk3DSCamera *) malloc (sizeof (vtk3DSCamera));

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

static void parse_colour (vtk3DSImporter *importer, vtk3DSColour *colour)
{
  vtk3DSChunk chunk;
  vtk3DSColour_24 colour_24;

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


static void parse_colour_f (vtk3DSImporter *importer, vtk3DSColour *colour)
{
  colour->red   = read_float(importer);
  colour->green = read_float(importer);
  colour->blue  = read_float(importer);
}


static void parse_colour_24 (vtk3DSImporter *importer, vtk3DSColour_24 *colour)
{
  colour->red   = read_byte(importer);
  colour->green = read_byte(importer);
  colour->blue  = read_byte(importer);
}


static float parse_percentage(vtk3DSImporter *importer)
{
  vtk3DSChunk chunk;
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


static void start_chunk (vtk3DSImporter *importer, vtk3DSChunk *chunk)
{
  chunk->start  = ftell(importer->GetFileFD());
  chunk->tag    = peek_word(importer);
  chunk->length = peek_dword(importer);
  if (chunk->length == 0)
    {
    chunk->length = 1;
    }
  chunk->end    = chunk->start + chunk->length;
}


static void end_chunk (vtk3DSImporter *importer, vtk3DSChunk *chunk)
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

  if (fread (&data, 2, 1, importer->GetFileFD()) != 1)
    {
    vtkErrorWithObjectMacro(
      importer, "Pre-mature end of file in read_word\n");
    data = 0;
    }
  vtkByteSwap::Swap2LE ((short *) &data);
  return data;
}

static word peek_word(vtk3DSImporter *importer)
{
  word data;

  if (fread (&data, 2, 1, importer->GetFileFD()) != 1)
    {
    data = 0;
    }
  vtkByteSwap::Swap2LE ((short *) &data);
  return data;
}

static dword peek_dword(vtk3DSImporter *importer)
{
  dword data;

  if (fread (&data, 4, 1, importer->GetFileFD()) != 1)
    {
    data = 0;
    }

  vtkByteSwap::Swap4LE ((char *) &data);
  return data;
}

static float read_float(vtk3DSImporter *importer)
{
  float data;

  if (fread (&data, 4, 1, importer->GetFileFD()) != 1)
    {
    vtkErrorWithObjectMacro(
      importer, "Pre-mature end of file in read_float\n");
    data = 0;
    }

  vtkByteSwap::Swap4LE ((char *) &data);
  return data;
}


static void read_point (vtk3DSImporter *importer, vtk3DSVector v)
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
  for (i = static_cast<int>(strlen(tmp))-1; i >= 0; i--)
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
  vtk3DSOmniLight *omniLight;
  vtk3DSSpotLight *spotLight;

  // walk the light list and delete vtk objects
  for (omniLight = this->OmniList; omniLight != (vtk3DSOmniLight *) NULL; omniLight = (vtk3DSOmniLight *) omniLight->next)
    {
    omniLight->aLight->Delete();
    }
  VTK_LIST_KILL (this->OmniList);

  // walk the spot light list and delete vtk objects
  for (spotLight = this->SpotLightList; spotLight != (vtk3DSSpotLight *) NULL;
       spotLight = (vtk3DSSpotLight *) spotLight->next)
    {
    spotLight->aLight->Delete();
    }
  VTK_LIST_KILL (this->SpotLightList);

  vtk3DSCamera *camera;
  // walk the camera list and delete vtk objects
  for (camera = this->CameraList; camera != (vtk3DSCamera *) NULL;
       camera = (vtk3DSCamera *) camera->next)
    {
    camera->aCamera->Delete ();
    }
  VTK_LIST_KILL (this->CameraList);

  // walk the mesh list and delete malloced datra and vtk objects
  vtk3DSMesh *mesh;
  for (mesh = this->MeshList; mesh != (vtk3DSMesh *) NULL;
       mesh = (vtk3DSMesh *) mesh->next)
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
  vtk3DSMatProp *m;
  // just walk the list of material properties, deleting vtk properties
  for (m = this->MatPropList; m != (vtk3DSMatProp *) NULL; m = (vtk3DSMatProp *) m->next)
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
  this->Superclass::PrintSelf(os,indent);
  os << indent << "File Name: "
     << (this->FileName ? this->FileName : "(none)") << "\n";

  os << indent << "Compute Normals: "
     << (this->ComputeNormals ? "On\n" : "Off\n");
}






