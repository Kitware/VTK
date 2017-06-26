/*=========================================================================
  Program:   Visualization Toolkit
  Module:    vtkOBJImporterInternals.cxx
  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "vtkSmartPointer.h"
#include "vtkOBJImporter.h"
#include "vtkOBJImporterInternals.h"
#include "vtkJPEGReader.h"
#include "vtkPNGReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtksys/SystemTools.hxx"

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <map>

#if defined(_WIN32)
#pragma warning(disable : 4267)
#pragma warning(disable : 4800)
#endif

const int OBJ_LINE_SIZE = 4096;

namespace
{
char strequal(const char *s1, const char *s2)
{
  if(strcmp(s1, s2) == 0)
  {
    return 1;
  }
  return 0;
}

int localVerbosity = 0;

}

void obj_set_material_defaults(vtkOBJImportedMaterial* mtl)
{
  mtl->amb[0] = 0.2;
  mtl->amb[1] = 0.2;
  mtl->amb[2] = 0.2;
  mtl->diff[0] = 0.8;
  mtl->diff[1] = 0.8;
  mtl->diff[2] = 0.8;
  mtl->spec[0] = 1.0;
  mtl->spec[1] = 1.0;
  mtl->spec[2] = 1.0;
  mtl->reflect = 0.0;
  mtl->trans = 1;
  mtl->glossy = 98;
  mtl->shiny = 0;
  mtl->refract_index = 1;
  mtl->texture_filename[0] = '\0';

  if( localVerbosity > 0 )
  {
    vtkGenericWarningMacro("Created a default vtkOBJImportedMaterial, texture filename is "
                           << std::string(mtl->texture_filename));
  }
}

std::vector<vtkOBJImportedMaterial*> vtkOBJPolyDataProcessor::ParseOBJandMTL(
  std::string Filename, int& result_code)
{

  std::vector<vtkOBJImportedMaterial*>  listOfMaterials;
  result_code    = 0;
  if (Filename.empty())
  {
    return listOfMaterials;
  }
  const char* filename = Filename.c_str();

  int line_number = 0;
  char *current_token;
  char current_line[OBJ_LINE_SIZE];
  char material_open = 0;
  vtkOBJImportedMaterial* current_mtl = NULL;
  FILE *mtl_file_stream;

  // open scene
  mtl_file_stream = fopen( filename, "r");
  if(mtl_file_stream == 0)
  {
    vtkErrorMacro("Error reading file: " << filename);
    result_code = -1;
    return listOfMaterials;
  }

  while( fgets(current_line, OBJ_LINE_SIZE, mtl_file_stream) )
  {
    current_token = strtok( current_line, " \t\n\r");
    line_number++;

    //skip comments
    if( current_token == NULL || strequal(current_token, "//") || strequal(current_token, "#"))
    {
      continue;
    }

    //start material
    else if( strequal(current_token, "newmtl"))
    {
      material_open = 1;
      current_mtl = (new vtkOBJImportedMaterial);
      listOfMaterials.push_back(current_mtl);
      obj_set_material_defaults(current_mtl);

      // material names can have spaces in them
      // get the name
      strncpy(current_mtl->name, strtok(NULL, "\t\n\r"), MATERIAL_NAME_SIZE);
      // be safe with strncpy
      if (current_mtl->name[MATERIAL_NAME_SIZE-1] != '\0')
      {
        current_mtl->name[MATERIAL_NAME_SIZE-1] = '\0';
        vtkErrorMacro("material name too long, truncated");
      }
    }

    //ambient
    else if( strequal(current_token, "Ka") && material_open)
    {
      // But this is ... right? no?
      current_mtl->amb[0] = atof( strtok(NULL, " \t"));
      current_mtl->amb[1] = atof( strtok(NULL, " \t"));
      current_mtl->amb[2] = atof( strtok(NULL, " \t"));
    }

    //diff
    else if( strequal(current_token, "Kd") && material_open)
    {
      current_mtl->diff[0] = atof( strtok(NULL, " \t"));
      current_mtl->diff[1] = atof( strtok(NULL, " \t"));
      current_mtl->diff[2] = atof( strtok(NULL, " \t"));
    }

    //specular
    else if( strequal(current_token, "Ks") && material_open)
    {
      current_mtl->spec[0] = atof( strtok(NULL, " \t"));
      current_mtl->spec[1] = atof( strtok(NULL, " \t"));
      current_mtl->spec[2] = atof( strtok(NULL, " \t"));
    }
    //shiny
    else if( strequal(current_token, "Ns") && material_open)
    {
      current_mtl->shiny = atof( strtok(NULL, " \t"));
    }
    //transparent
    else if( strequal(current_token, "d") && material_open)
    {
      current_mtl->trans = atof( strtok(NULL, " \t"));
    }
    //reflection
    else if( strequal(current_token, "r") && material_open)
    {
      current_mtl->reflect = atof( strtok(NULL, " \t"));
    }
    //glossy
    else if( strequal(current_token, "sharpness") && material_open)
    {
      current_mtl->glossy = atof( strtok(NULL, " \t"));
    }
    //refract index
    else if( strequal(current_token, "Ni") && material_open)
    {
      current_mtl->refract_index = atof( strtok(NULL, " \t"));
    }
    // illumination type
    else if( strequal(current_token, "illum") && material_open)
    {
    }
    // texture map
    else if( (strequal(current_token, "map_kd") || strequal(current_token, "map_Kd")) && material_open)
    {
      /** (pk note: why was this map_Ka initially? should map_Ka be supported? ) */
      // tmap may be null so we test first before doing a strncpy
      char *tmap = strtok(NULL, " \t\n\r");
      if (tmap)
      {
        strncpy(current_mtl->texture_filename, tmap, OBJ_FILENAME_LENGTH);
        // be safe with strncpy
        if (current_mtl->texture_filename[OBJ_FILENAME_LENGTH-1] != '\0')
        {
          current_mtl->texture_filename[OBJ_FILENAME_LENGTH-1] = '\0';
          vtkErrorMacro("texture name too long, truncated");
        }
        bool bFileExistsNoPath    = vtksys::SystemTools::FileExists(current_mtl->texture_filename);
        std::vector<std::string> path_and_file(2);
        path_and_file[0]   = this->GetTexturePath();
        path_and_file[1]   = std::string(current_mtl->texture_filename);
        std::string joined =  vtksys::SystemTools::JoinPath(path_and_file);
        bool bFileExistsInPath    = vtksys::SystemTools::FileExists( joined );
        if(! (bFileExistsNoPath || bFileExistsInPath ) )
        {
          vtkGenericWarningMacro(
            << "mtl file " << current_mtl->name
            << "requests texture file that appears not to exist: "
            << current_mtl->texture_filename << "; texture path: " << this->TexturePath << "\n");
        }
      }
    }
    else
    {
      // just skip it; got an unsupported feature or a comment in file.
      vtkDebugMacro("Unknown command " << current_token
                    << " in material file " << filename
                    << " at line " << line_number
                    << ":\n\t" << current_line);
    }
  }

  fclose(mtl_file_stream);

  return listOfMaterials;
}


void  bindTexturedPolydataToRenderWindow( vtkRenderWindow* renderWindow,
                                          vtkRenderer* renderer,
                                          vtkOBJPolyDataProcessor* reader )
{
  if( NULL == (renderWindow) )
  {
    vtkErrorWithObjectMacro(reader, "RenderWindow is null, failure!");
    return;
  }
  if( NULL == (renderer) )
  {
    vtkErrorWithObjectMacro(reader, "Renderer is null, failure!");
    return;
  }
  if( NULL == (reader) )
  {
    vtkErrorWithObjectMacro(reader, "vtkOBJPolyDataProcessor is null, failure!");
    return;
  }

  reader->actor_list.clear();
  reader->actor_list.reserve( reader->GetNumberOfOutputPorts() );

  for( int port_idx=0; port_idx < reader->GetNumberOfOutputPorts(); port_idx++)
  {
    vtkPolyData* objPoly = reader->GetOutput(port_idx);

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(objPoly);

    vtkDebugWithObjectMacro(reader, "Grabbed objPoly " << objPoly
                            << ", port index " << port_idx << "\n"
                            << "numPolys = " << objPoly->GetNumberOfPolys()
                            << " numPoints = " << objPoly->GetNumberOfPoints());

    // For each named material, load and bind the texture, add it to the renderer
    vtkSmartPointer<vtkTexture> vtk_texture = vtkSmartPointer<vtkTexture>::New();

    std::string textureFilename = reader->GetTextureFilename(port_idx);

    vtkSmartPointer<vtkJPEGReader> tex_jpg_Loader = vtkSmartPointer<vtkJPEGReader>::New();
    vtkSmartPointer<vtkPNGReader>  tex_png_Loader = vtkSmartPointer<vtkPNGReader>::New();
    int bIsReadableJPEG = tex_jpg_Loader->CanReadFile( textureFilename.c_str() );
    int bIsReadablePNG  = tex_png_Loader->CanReadFile( textureFilename.c_str() );

    bool haveTexture = false;
    if (!textureFilename.empty())
    {
      if( bIsReadableJPEG )
      {
        tex_jpg_Loader->SetFileName( textureFilename.c_str() );
        tex_jpg_Loader->Update();
        vtk_texture->AddInputConnection( tex_jpg_Loader->GetOutputPort() );
        haveTexture = true;
      }
      else if( bIsReadablePNG )
      {
        tex_png_Loader->SetFileName( textureFilename.c_str() );
        tex_png_Loader->Update();
        vtk_texture->AddInputConnection( tex_png_Loader->GetOutputPort() );
        haveTexture = true;
      }
      else
      {
        if(!textureFilename.empty()) // OK to have no texture image, but if its not empty it ought to exist.
        {
          vtkErrorWithObjectMacro(reader, "Nonexistent texture image type!? imagefile: "
            <<textureFilename);
        }
      }
      vtk_texture->InterpolateOff(); // Faster?? (yes clearly faster for largish texture)
    }

    vtkSmartPointer<vtkActor> actor =
      vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    if (haveTexture)
    {
      actor->SetTexture(vtk_texture);
    }
    vtkSmartPointer<vtkProperty> properties =
      vtkSmartPointer<vtkProperty>::New();

    vtkOBJImportedMaterial* raw_mtl_data =
      reader->GetMaterial(port_idx);
    if (raw_mtl_data)
    {
      properties->SetDiffuseColor(raw_mtl_data->diff);
      properties->SetSpecularColor(raw_mtl_data->spec);
      properties->SetAmbientColor(raw_mtl_data->amb);
      properties->SetOpacity(raw_mtl_data->trans);
      properties->SetInterpolationToPhong();
      properties->SetLighting(true);
      properties->SetSpecular( raw_mtl_data->get_spec_coeff() );
      properties->SetAmbient( raw_mtl_data->get_amb_coeff() );
      properties->SetDiffuse( raw_mtl_data->get_diff_coeff() );
      actor->SetProperty(properties);
    }
    renderer->AddActor(actor);

    //properties->ShadingOn(); // use ShadingOn() if loading vtkMaterial from xml
    // available in mtl parser are:
    //    double amb[3];
    //    double diff[3];
    //    double spec[3];
    //    double reflect;
    //    double refract;
    //    double trans;
    //    double shiny;
    //    double glossy;
    //    double refract_index;

    reader->actor_list.push_back(actor); // keep a handle on actors to animate later
  }
  /** post-condition of this function: the renderer has had a bunch of actors added to it */
}


vtkOBJImportedMaterial::vtkOBJImportedMaterial()
{
  name[0] = 'x';
  name[1] = '\0';
  obj_set_material_defaults(this);
}
