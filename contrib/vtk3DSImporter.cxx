/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk3DSImporter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtk3DSImporter.h"
#include "vtkByteSwap.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkStripper.h"

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
static Material *update_materials (vtk3DSImporter *importer, char *new_material, int ext);
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
}

int vtk3DSImporter::ImportBegin ()
{
  vtkDebugMacro(<< "Opening import file as binary");
  fclose (this->FileFD);
  this->FileFD = fopen (this->FileName, "rb");
  if (this->FileFD == NULL)
    {
    vtkErrorMacro(<< "Unable to open file: "<< this->FileName);
    return 0;
    }
  return this->Read3DS ();
}

int vtk3DSImporter::Read3DS ()
{
  if (parse_3ds_file (this) == 0)
    {
    vtkErrorMacro (<<  "Error readings .3ds file: " << this->FileName << "\n");
    return 0;
    }


  DefaultMaterial.aProperty = vtkProperty::New ();
  LIST_INSERT (this->MatPropList, &DefaultMaterial);
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
  for (mesh = this->MeshList; mesh != (Mesh *) NULL; mesh = (Mesh *) mesh->next)
    {

    if (mesh->faces == 0)
      {
      vtkWarningMacro (<< "part " << mesh->name << " has zero faces... skipping\n");
      continue;
      }

    polyData = GeneratePolyData (mesh);
    polyMapper = vtkPolyDataMapper::New ();
    polyStripper = vtkStripper::New ();

    // if ComputeNormals is on, insert a vtkPolyDataNormals filter
    if (this->ComputeNormals)
      {
      polyNormals = vtkPolyDataNormals::New ();
      polyNormals->SetInput (polyData);
      polyStripper->SetInput (polyNormals->GetOutput ());
      }
    else
      {
      polyStripper->SetInput (polyData);
      }
    
    polyMapper->SetInput (polyStripper->GetOutput ());
    vtkDebugMacro (<< "Importing Actor: " << mesh->name);
    actor = vtkActor::New ();
    actor->SetMapper (polyMapper);
    material = (MatProp *) LIST_FIND (this->MatPropList, mesh->mtl[0]->name);
    actor->SetProperty (material->aProperty);
    renderer->AddActor (actor);
  }
}

vtkPolyData *vtk3DSImporter::GeneratePolyData (Mesh *mesh)
{
  int i;
  Face	*face;
  vtkCellArray *triangles;
  vtkFloatPoints *vertices;
  vtkPolyData *polyData;

  face = mesh->face;
  triangles = vtkCellArray::New ();
  triangles->Allocate(mesh->faces * 3);
  for (i = 0; i < mesh->faces; i++, face++)
    {
    triangles->InsertNextCell (3);
    triangles->InsertCellPoint (face->a);
    triangles->InsertCellPoint (face->b);
    triangles->InsertCellPoint (face->c);
    }

  vertices = vtkFloatPoints::New ();
  vertices->Allocate(mesh->vertices);
  for (i = 0; i < mesh->vertices; i++)
    {
    vertices->InsertPoint (i, (float *) mesh->vertex[i]);
    }
  polyData = vtkPolyData::New ();
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
    aCamera = vtkCamera::New ();	
    aCamera->SetPosition (camera->pos[0], camera->pos[1], camera->pos[2]);
    aCamera->SetFocalPoint (camera->target[0], camera->target[1], camera->target[2]);
    aCamera->SetViewUp (0, 0, 1);
    aCamera->SetClippingRange (.1,10000);
    aCamera->Roll (camera->bank);
    aCamera->ComputeViewPlaneNormal ();
    renderer->SetActiveCamera (aCamera);
    vtkDebugMacro (<< "Importing Camera: " << camera->name);
  }
}

void vtk3DSImporter::ImportLights (vtkRenderer *renderer)
{
  OmniLight *omniLight;
  SpotLight *spot_light_ptr;
  vtkLight *aLight;

  // just walk the list of omni lights, creating lymb lights
  for (omniLight = this->OmniList; omniLight != (OmniLight *) NULL; omniLight = (OmniLight *) omniLight->next)
  {

  aLight = vtkLight::New ();
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
}

void vtk3DSImporter::ImportProperties (vtkRenderer *vtkNotUsed(renderer))
{
  float amb = 0.1, dif = 0.9, spec = 1.0;
  float dist_white, dist_diff, phong, phong_size;
  Colour ambient;
  vtkProperty *property;
  MatProp *m;

  // just walk the list of material properties, creating vtk properties */
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
      dif = .1; amb = .8; spec = 1;
      ambient = m->diffuse;
      }
    else
      {
      ambient = m->ambient;
      }

      if (m->reflection > 0.0) spec = (m->specular.red + m->specular.green + m->specular.blue)/3.0;

      phong_size = 0.7*m->shininess;
      if (phong_size < 1.0) phong_size = 1.0;

      if (phong_size > 30.0)
  	phong = 1.0;
      else
  	phong = phong_size/30.0;

  property = m->aProperty;
  property->SetAmbientColor (m->ambient.red, m->ambient.green, m->ambient.blue);
  property->SetAmbient (amb);
  property->SetDiffuseColor (m->diffuse.red, m->diffuse.green, m->diffuse.blue);
  property->SetDiffuse (dif);
  property->SetSpecularColor (m->specular.red, m->specular.green, m->specular.blue);
  property->SetSpecular (phong);
  property->SetSpecularPower (phong_size);
  property->SetOpacity (1.0 - m->transparency);
  vtkDebugMacro (<< "Importing Property: " << m->name);
  
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
static void *list_find (List **root, char *name)
{
    List *p;
    for (p = *root; p != (List *) NULL; p = (List *) p->next) {
	if (strcmp (p->name, name) == 0)
	    break;
    }
    return (void *)p;
}

/* Delete the indicated node from the list */
static void list_delete (List **root, List *node)
{
    List *prev;

    prev = *root;
    while (prev != (List *) NULL && prev->next != node)
	prev = (List *) prev->next;

    if (prev == NULL)
	*root = (List *) node->next;
    else
	prev->next = node->next;

    free (node);
}

/* Delete the entire list */
static void list_kill (List **root)
{
    List *temp;

    while (*root != (List *) NULL) {
	temp = *root;
	*root = (List *) (*root)->next;
	free (temp);
    }
}

/* Add a new material to the material list */
static Material *update_materials (vtk3DSImporter *importer, char *new_material, int ext)
{
    Material *p;

    p = (Material *) LIST_FIND (importer->MaterialList, new_material);

    if (p == NULL) {
	p = (Material *) malloc (sizeof (*p));
	strcpy (p->name, new_material);
	p->external = ext;

	LIST_INSERT (importer->MaterialList, p);
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
    new_mprop->self_illum = FALSE;

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
	new_mesh->vertex = NULL;
    else {
	new_mesh->vertex = (Vector *) malloc (vertices * sizeof(*new_mesh->vertex));
    }

    new_mesh->faces = faces;

    if (faces <= 0) {
	new_mesh->face = NULL;
	new_mesh->mtl = NULL;
    }
    else {
	new_mesh->face = (Face *) malloc (faces * sizeof(*new_mesh->face));
	new_mesh->mtl = (Material **) malloc (faces * sizeof(*new_mesh->mtl));
    }

    new_mesh->hidden = FALSE;
    new_mesh->shadow = TRUE;

    return new_mesh;
}


/* Free all data associated with mesh object */
static void free_mesh_data (Mesh *mesh)
{
    if (mesh->vertex != (Vector *) NULL)
	free (mesh->vertex);

    if (mesh->face != (Face *) NULL)
	free (mesh->face);

    if (mesh->mtl != (Material **) NULL)
	free (mesh->mtl);
}


static int parse_3ds_file(vtk3DSImporter *importer)
{
    Chunk chunk;

    start_chunk(importer, &chunk);

    if (chunk.tag == 0x4D4D)
	parse_3ds (importer, &chunk);
    else {
	fprintf (stderr, "%s: Error: Input file is not .3DS format\n", "threed_studio_reader");
	return 0;
    }

    end_chunk (importer, &chunk);
    return 1;
}

static void parse_3ds (vtk3DSImporter *importer, Chunk *mainchunk)
{
    Chunk chunk;

    do  {
	start_chunk (importer, &chunk);

	if (chunk.end <= mainchunk->end) {
	    switch (chunk.tag) {
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

    do  {
	start_chunk (importer, &chunk);

	if (chunk.end <= mainchunk->end) {
	    switch (chunk.tag) {
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
    fog_distance = read_float(importer);
    (void)read_float(importer);

    parse_colour (importer, &fog_colour);

    do  {
	start_chunk (importer, &chunk);

	if (chunk.end <= mainchunk->end) {
	    switch (chunk.tag) {
		case 0x2210: parse_fog_bgnd(importer);
			     break;
	    }
	}

	end_chunk (importer, &chunk);
    } while (chunk.end <= mainchunk->end);
}


static void parse_fog_bgnd(vtk3DSImporter *vtkNotUsed(importer))
{
  vtkGenericWarningMacro(<< "fog background detected but not used\n");
}


static void parse_mat_entry (vtk3DSImporter *importer, Chunk *mainchunk)
{
    Chunk chunk;
    MatProp *mprop;

    mprop = create_mprop();

    do  {
	start_chunk (importer, &chunk);
	if (chunk.end <= mainchunk->end) {
	    switch (chunk.tag) {
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

		case 0xA080: mprop->self_illum = TRUE;
			     break;

		case 0xA220: mprop->reflection = parse_percentage(importer);
			     (void)parse_mapname (importer, &chunk);
			     break;

		case 0xA310: if (mprop->reflection == 0.0)
				 mprop->reflection = 1.0;
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

    LIST_INSERT (importer->MatPropList, mprop);
}


static char *parse_mapname (vtk3DSImporter *importer, Chunk *mainchunk)
{
    static char name[80] = "";
    Chunk chunk;

    do  {
	start_chunk (importer, &chunk);

	if (chunk.end <= mainchunk->end) {
	    switch (chunk.tag) {
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

    do  {
	start_chunk (importer, &chunk);
	if (chunk.end <= mainchunk->end) {
	    switch (chunk.tag) {
		case 0x4100: parse_n_tri_object (importer, &chunk);
			     break;
		case 0x4600: parse_n_direct_light (importer, &chunk);
			     break;
		case 0x4700: parse_n_camera(importer);
			     break;
		case 0x4010: if (mesh != NULL) mesh->hidden = TRUE;
			     break;
		case 0x4012: if (mesh != NULL) mesh->shadow = FALSE;
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

    do  {
	start_chunk (importer, &chunk);

	if (chunk.end <= mainchunk->end) {
	    switch (chunk.tag) {
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
    LIST_INSERT (importer->MeshList, mesh);
}


static void parse_point_array(vtk3DSImporter *importer, Mesh *mesh)
{
    int i;

    mesh->vertices = read_word(importer);
    mesh->vertex = (Vector *) malloc (mesh->vertices * sizeof(*(mesh->vertex)));
    for (i = 0; i < mesh->vertices; i++)
	read_point (importer, mesh->vertex[i]);
}

static void parse_face_array (vtk3DSImporter *importer, Mesh *mesh, Chunk *mainchunk)
{
    Chunk chunk;
    int i;

    mesh->faces = read_word(importer);
    mesh->face = (Face *) malloc (mesh->faces * sizeof(*(mesh->face)));
    mesh->mtl = (Material **) malloc (mesh->faces * sizeof(*(mesh->mtl)));

    for (i = 0; i < mesh->faces; i++) {
	mesh->face[i].a = read_word(importer);
	mesh->face[i].b = read_word(importer);
	mesh->face[i].c = read_word(importer);
	(void)read_word(importer);

	mesh->mtl[i] = NULL;
    }

    do  {
	start_chunk (importer, &chunk);
	if (chunk.end <= mainchunk->end) {
	    switch (chunk.tag) {
		case 0x4130: parse_msh_mat_group(importer, mesh);
			     break;
		case 0x4150: parse_smooth_group(importer);
			     break;
	    }
	}

	end_chunk (importer, &chunk);
    } while (chunk.end <= mainchunk->end);

    for (i = 0; i < mesh->faces; i++) {
	if (mesh->mtl[i] == (Material *) NULL)
	    mesh->mtl[i] = update_materials (importer, "Default", 0);
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

    for (i = 0; i < mtlcnt; i++) {
	face = read_word(importer);
	mesh->mtl[face] = new_mtl;
    }

}

static void parse_smooth_group(vtk3DSImporter *importer)
{
}

static void parse_mesh_matrix(vtk3DSImporter *vtkNotUsed(importer), Mesh *vtkNotUsed(mesh))
{
  vtkGenericWarningMacro(<< "mesh matrix detected but not used\n");
}


static void parse_n_direct_light (vtk3DSImporter *importer, Chunk *mainchunk)
{
    Chunk chunk;
    SpotLight *s;
    OmniLight *o;
    int light_off = FALSE;
    int spot_flag = FALSE;

    read_point (importer, pos);
    parse_colour (importer, &col);

    do  {
	start_chunk (importer, &chunk);

	if (chunk.end <= mainchunk->end) {
	    switch (chunk.tag) {
		case 0x4620: light_off = TRUE;
			     break;
		case 0x4610: parse_dl_spotlight(importer);
			     spot_flag = TRUE;
			     break;
	    }
	}

	end_chunk (importer, &chunk);
    } while (chunk.end <= mainchunk->end);

    if (!spot_flag) {
	    o = (OmniLight *) LIST_FIND (importer->OmniList, obj_name);

	    if (o != NULL) {
		pos[X] = o->pos[X];
		pos[Y] = o->pos[Y];
		pos[Z] = o->pos[Z];
		col    = o->col;
	    }
	    else {
		o = (OmniLight *) malloc (sizeof (*o));
		o->pos[X] = pos[X];
		o->pos[Y] = pos[Y];
		o->pos[Z] = pos[Z];
		o->col = col   ;
		strcpy (o->name, obj_name);
		LIST_INSERT (importer->OmniList, o);
 	    }
    }
    else {
	    s = (SpotLight *) LIST_FIND (importer->SpotLightList, obj_name);

	    if (s != NULL) {
		pos[X]    = s->pos[X];
		pos[Y]    = s->pos[Y];
		pos[Z]    = s->pos[Z];
		target[X] = s->target[X];
		target[Y] = s->target[Y];
		target[Z] = s->target[Z];
		col       = s->col;
		hotspot   = s->hotspot;
		falloff   = s->falloff;
	    }
	    else {
	if (falloff <= 0.0)
	    falloff = 180.0;

	if (hotspot <= 0.0)
	    hotspot = 0.7*falloff;

		s = (SpotLight *) malloc (sizeof (*s));
		s->pos[X] = pos[X];
		s->pos[Y] = pos[Y];
		s->pos[Z] = pos[Z];
		s->target[X] = target[X];
		s->target[Y] = target[Y];
		s->target[Z] = target[Z];
		s->col = col   ;
		s->hotspot = hotspot;
		s->falloff = falloff;
		strcpy (s->name, obj_name);
		LIST_INSERT (importer->SpotLightList, s);
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
	c->pos[X] = pos[X];
	c->pos[Y] = pos[Y];
	c->pos[Z] = pos[Z];
	c->target[X] = target[X];
	c->target[Y] = target[Y];
	c->target[Z] = target[Z];
	c->lens = lens;
	c->bank = bank;

	LIST_INSERT (importer->CameraList, c);
}

static float findfov (float lens)
{
    static float lens_table[13] =
		 { 15.0, 17.0, 24.0, 35.0, 50.0, 85.0, 100.0, 135.0, 200.0,
		   500.0, 625.0, 800.0, 1000.0 };
    static float fov_table[13] =
		 { 115.0, 102.0, 84.0, 63.0, 46.0, 28.0, 24.0, 18.0,
		   12.0, 5.0, 4.0, 3.125, 2.5 };

    float fov, f1, f2, l1, l2;
    int   i;

    if (lens < 15.0)
	lens = 15.0;
    else if (lens > 1000.0)
	lens = 1000.0;

    for (i = 0; i < 13; i++)
	if (lens < lens_table[i])
	    break;

    if (i == 13)
	i = 12;
    else if (i == 0)
	i = 1;

    f1 = fov_table[i-1];
    f2 = fov_table[i];
    l1 = lens_table[i-1];
    l2 = lens_table[i];

    fov = f1 + (lens - l1) * (f2 - f1) / (l2 - l1);

    return fov;
}

static void parse_colour (vtk3DSImporter *importer, Colour *colour)
{
    Chunk chunk;
    Colour_24 colour_24;

    start_chunk (importer, &chunk);

    switch (chunk.tag) {
	case 0x0010: parse_colour_f (importer, colour);
		     break;

	case 0x0011: parse_colour_24 (importer, &colour_24);
		     colour->red   = colour_24.red/255.0;
		     colour->green = colour_24.green/255.0;
		     colour->blue  = colour_24.blue/255.0;
		     break;

	default:     fprintf (stderr, "%s: Error parsing colour\n", "threed_studio_reader");
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

    switch (chunk.tag) {
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
    chunk->start  = ftell(importer->FileFD);
    chunk->tag    = read_word(importer);
    chunk->length = read_dword(importer);
    if (chunk->length == 0) chunk->length = 1;

    chunk->end    = chunk->start + chunk->length;
}


static void end_chunk (vtk3DSImporter *importer, Chunk *chunk)
{
    fseek (importer->FileFD, chunk->end, 0);
}


static byte read_byte(vtk3DSImporter *importer)
{
    byte data;

    data = fgetc (importer->FileFD);

    return data;
}


static word read_word(vtk3DSImporter *importer)
{
    word data;

    fread (&data, 2, 1, importer->FileFD);
    vtkByteSwap::Swap2LE ((short *) &data);
/*    swab ((char *) &data, (char *) &sdata, 2);*/

    return data;
}

static dword read_dword(vtk3DSImporter *importer)
{
    dword data;

    fread (&data, 4, 1, importer->FileFD);

    vtkByteSwap::Swap4LE ((char *) &data);
/*    TIFFSwabLong (&data);*/

    return data;
}


static float read_float(vtk3DSImporter *importer)
{
    float data;

    fread (&data, 4, 1, importer->FileFD);
    vtkByteSwap::Swap4LE ((char *) &data);
/*    TIFFSwabLong (&data);*/

    return data;
}


static void read_point (vtk3DSImporter *importer, Vector v)
{
    v[X] = read_float(importer);
    v[Y] = read_float(importer);
    v[Z] = read_float(importer);
}


static char *read_string(vtk3DSImporter *importer)
{
    static char string[80];
    int i;

    for (i = 0; i < 80; i++) {
	string[i] = read_byte(importer);

	if (string[i] == '\0')
	    break;
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
	i++;

    strcpy (tmp, &name[i]);

    /* Remove any trailing blanks or quotes */
    for (i = strlen(tmp)-1; i >= 0; i--) {
	if (isprint(tmp[i]) && !isspace(tmp[i]) && tmp[i] != '"')
	    break;
	else
	    tmp[i] = '\0';
    }

    strcpy (name, tmp);

    /* Prefix the letter 'N' to materials that begin with a digit */
    if (!isdigit (name[0]))
       strcpy (tmp, name);
    else {
       tmp[0] = 'N';
       strcpy (&tmp[1], name);
    }

    /* Replace all illegal charaters in name with underscores */
    for (i = 0; tmp[i] != '\0'; i++) {
       if (!isalnum(tmp[i]))
	   tmp[i] = '_';
    }

    strcpy (name, tmp);

    free (tmp);
}

void vtk3DSImporter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtk3DSImporter::PrintSelf(os,indent);
}






