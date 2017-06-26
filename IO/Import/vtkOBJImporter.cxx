/*=========================================================================
  Program:   Visualization Toolkit
  Module:    vtkOBJImporter.cxx
  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "vtkOBJImporter.h"

#include "vtkActor.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtksys/SystemTools.hxx"

#include <sstream>
#include <cctype>
#include <cstdio>
#include <list>
#include <set>
#include <map>
#include <memory>
#include "vtkOBJImporterInternals.h"

#if defined(_WIN32)
  #pragma warning(disable : 4267)
#endif

vtkStandardNewMacro(vtkOBJImporter)
vtkStandardNewMacro(vtkOBJPolyDataProcessor)

//----------------------------------------------------------------------------
  vtkOBJImporter::vtkOBJImporter()
{
  this->Impl = vtkSmartPointer<vtkOBJPolyDataProcessor>::New();
}

//----------------------------------------------------------------------------
vtkOBJImporter::~vtkOBJImporter()
{
}

int CanReadFile( vtkObject* that, const std::string& fname )
{
  FILE* fileFD = fopen (fname.c_str(), "rb");
  if (fileFD == NULL)
  {
    vtkErrorWithObjectMacro(that,<< "Unable to open file: "<< fname.c_str());
    return 0;
  }
  fclose(fileFD);
  return 1;
}

int vtkOBJImporter::ImportBegin()
{
  if (!CanReadFile(this, this->GetFileName()))
  {
    return 0;
  }
  if( !std::string(GetFileNameMTL()).empty() && !CanReadFile(this,this->GetFileNameMTL()))
  {
      return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkOBJImporter::ImportEnd()
{
  vtkDebugMacro("Done with "<<this->GetClassName()<<"::"<<__FUNCTION__);
}

//----------------------------------------------------------------------------
void vtkOBJImporter::ReadData()
{
  this->Impl->Update();
  if (Impl->GetSuccessParsingFiles())
  {
    bindTexturedPolydataToRenderWindow(this->RenderWindow,this->Renderer,Impl.Get());
  }
}

//----------------------------------------------------------------------------
void vtkOBJImporter::PrintSelf(std::ostream &os, vtkIndent indent)
{
  vtkImporter::PrintSelf(os,indent);
}

void vtkOBJImporter::SetFileName(const char *arg)
{
  this->Impl->SetFileName(arg);
}

void vtkOBJImporter::SetFileNameMTL(const char *arg)
{
  this->Impl->SetMTLfileName(arg);
}

void vtkOBJImporter::SetTexturePath(const char *path)
{
  return this->Impl->SetTexturePath(path);
}

const char* vtkOBJImporter::GetFileName() const
{
  return this->Impl->GetFileName().data();
}

const char* vtkOBJImporter::GetFileNameMTL() const
{
  return this->Impl->GetMTLFileName().data();
}

const char* vtkOBJImporter::GetTexturePath( ) const
{
  return this->Impl->GetTexturePath().data();
}

std::string vtkOBJImporter::GetOutputDescription(int idx)
{
  vtkOBJImportedMaterial *mtl = this->Impl->GetMaterial(idx);
  std::stringstream ss;
  ss << "data output " << idx;
  if (mtl)
  {
    ss << " with material named " << mtl->name
      << " texture file " << (mtl->texture_filename[0] == '\0' ? "none" : mtl->texture_filename)
      << " diffuse color ("
      << mtl->diff[0] << ", " << mtl->diff[1] << ", " << mtl->diff[2] << ")"
      << " ambient color ("
      << mtl->amb[0] << ", " << mtl->amb[1] << ", " << mtl->amb[2] << ")"
      << " specular color ("
      << mtl->spec[0] << ", " << mtl->spec[1] << ", " << mtl->spec[2] << ")"
      << " specular power " << mtl->shiny
      << " opacity " << mtl->trans;
  }
  else
  {
    ss << " with no material";
  }

  return ss.str();
}


///////////////////////////////////////////


struct vtkOBJImportedPolyDataWithMaterial
{
  ~vtkOBJImportedPolyDataWithMaterial()
  {
  }
  vtkOBJImportedPolyDataWithMaterial()
  { // initialize some structures to store the file contents in
    points            = vtkSmartPointer<vtkPoints>::New();
    tcoords           = vtkSmartPointer<vtkFloatArray>::New();
    normals           = vtkSmartPointer<vtkFloatArray>::New();
    polys             = vtkSmartPointer<vtkCellArray>::New();
    tcoord_polys      = vtkSmartPointer<vtkCellArray>::New();
    pointElems        = vtkSmartPointer<vtkCellArray>::New();
    lineElems         = vtkSmartPointer<vtkCellArray>::New();
    normal_polys      = vtkSmartPointer<vtkCellArray>::New();
    tcoords->SetNumberOfComponents(2);
    normals->SetNumberOfComponents(3);

    materialName  = "";
    mtlProperties = NULL;
  }

  // these can be shared
  vtkSmartPointer<vtkPoints> points          ;
  vtkSmartPointer<vtkFloatArray> normals     ;

  void SetSharedPoints( vtkSmartPointer<vtkPoints> arg )
  {
    points = arg;
  }
  void SetSharedNormals( vtkSmartPointer<vtkFloatArray> arg )
  {
    normals = arg;
  }

  // these are unique per entity
  vtkSmartPointer<vtkFloatArray> tcoords     ;
  vtkSmartPointer<vtkCellArray> polys        ;
  vtkSmartPointer<vtkCellArray> tcoord_polys ;
  vtkSmartPointer<vtkCellArray> pointElems   ;
  vtkSmartPointer<vtkCellArray> lineElems    ;
  vtkSmartPointer<vtkCellArray> normal_polys ;

  typedef std::map<std::string,vtkOBJImportedPolyDataWithMaterial*> NamedMaterials;
  std::string    materialName;
  vtkOBJImportedMaterial*  mtlProperties;
};

//----------------------------------------------------------------------------
vtkOBJPolyDataProcessor::vtkOBJPolyDataProcessor()
{
  // Instantiate object with NULL filename, and no materials yet loaded.
  this->FileName    = "";
  this->MTLFileName = "";
  this->DefaultMTLFileName = true;
  this->TexturePath = ".";
  this->VertexScale = 1.0;
  this->SuccessParsingFiles = 1;
  this->SetNumberOfInputPorts(0);
  /** multi-poly-data paradigm: pivot based on named materials */
  vtkOBJImportedPolyDataWithMaterial* default_poly = (new vtkOBJImportedPolyDataWithMaterial);
  poly_list.push_back(default_poly);
  this->SetNumberOfOutputPorts(poly_list.size());
}

//----------------------------------------------------------------------------
vtkOBJPolyDataProcessor::~vtkOBJPolyDataProcessor()
{
  // clear any old mtls
  for( size_t k = 0; k < this->parsedMTLs.size(); ++k )
  {
    delete this->parsedMTLs[k];
  }

  for( size_t k = 0; k < poly_list.size(); ++k)
  {
    delete poly_list[k];
    poly_list[k] = NULL;
  }
}

//----------------------------------------------------------------------------
vtkOBJImportedMaterial*  vtkOBJPolyDataProcessor::GetMaterial(int k)
{
  if (k >= static_cast<int>(poly_list.size()))
  {
    return NULL;
  }
  vtkOBJImportedPolyDataWithMaterial*  rpdmm = this->poly_list[k];
  return rpdmm->mtlProperties;
}

//----------------------------------------------------------------------------
std::string vtkOBJPolyDataProcessor::GetTextureFilename( int idx )
{
  vtkOBJImportedMaterial* mtl = this->GetMaterial(idx);

  if (mtl && strlen(mtl->texture_filename))
  {
    std::vector<std::string> path_and_filename(2);
    path_and_filename[0] = this->TexturePath;
    path_and_filename[1] = mtl->texture_filename;
    std::string joined   = vtksys::SystemTools::JoinPath( path_and_filename );
    return joined;
  }

  return std::string();
}


// initialize some structures to store the file contents in


/*---------------------------------------------------------------------------*\

This is only partial support for the OBJ format, which is quite complicated.
To find a full specification, search the net for "OBJ format", eg.:

    http://en.wikipedia.org/wiki/Obj
    http://netghost.narod.ru/gff/graphics/summary/waveobj.htm

We support the following types:

v <x> <y> <z>

    vertex

vn <x> <y> <z>

    vertex normal

vt <x> <y>

    texture coordinate

f <v_a> <v_b> <v_c> ...

    polygonal face linking vertices v_a, v_b, v_c, etc. which
    are 1-based indices into the vertex list

f <v_a>/<t_a> <v_b>/<t_b> ...

    polygonal face as above, but with texture coordinates for
    each vertex. t_a etc. are 1-based indices into the texture
    coordinates list (from the vt lines)

f <v_a>/<t_a>/<n_a> <v_b>/<t_b>/<n_b> ...

    polygonal face as above, with a normal at each vertex, as a
    1-based index into the normals list (from the vn lines)

f <v_a>//<n_a> <v_b>//<n_b> ...

    polygonal face as above but without texture coordinates.

    Per-face tcoords and normals are supported by duplicating
    the vertices on each face as necessary.

l <v_a> <v_b> ...

    lines linking vertices v_a, v_b, etc. which are 1-based
    indices into the vertex list

p <v_a> <v_b> ...

    points located at the vertices v_a, v_b, etc. which are 1-based
    indices into the vertex list

\*---------------------------------------------------------------------------*/

//----------------------------------------------------------------------------
int vtkOBJPolyDataProcessor::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *vtkNotUsed(outputVector))
{
  if (this->FileName.empty())
  {
    vtkErrorMacro(<< "A FileName must be specified.");
    return 0;
  }

  FILE *in = fopen(this->FileName.c_str(),"r");
  if (in == NULL)
  {
    vtkErrorMacro(<< "File " << this->FileName << " not found");
    return 0;
  }

  // clear old poly list
  for( size_t k = 0; k < poly_list.size(); ++k)
  {
    delete poly_list[k];
    poly_list[k] = NULL;
  }
  poly_list.clear();

  vtkDebugMacro(<<"Reading file" << this->FileName);

  // clear any old mtls
  for( size_t k = 0; k < this->parsedMTLs.size(); ++k )
  {
    delete this->parsedMTLs[k];
  }

  // If the MTLFileName is not set explicitly, we assume *.obj.mtl as the MTL
  // filename
  std::string mtlname = this->MTLFileName;
  if (this->DefaultMTLFileName)
  {
    mtlname = this->FileName + ".mtl";
  }
  FILE *defMTL = fopen(mtlname.c_str(), "r");
  if (defMTL == NULL)
  {
    if (!this->DefaultMTLFileName)
    {
      vtkErrorMacro(<< "The MTL file " << mtlname <<
                    " could not be found");
      return 0;
    }
  }
  else
  {
    this->MTLFileName = mtlname;
    fclose(defMTL);
  }

  int mtlParseResult;
  this->parsedMTLs = ParseOBJandMTL(MTLFileName,mtlParseResult);
  if (this->parsedMTLs.empty())
  { // construct a default material to define the single polydata's actor.
    this->parsedMTLs.push_back( new vtkOBJImportedMaterial );
  }

  vtkDebugMacro("vtkOBJPolyDataProcessor parsed "   << this->parsedMTLs.size()
                << " materials from "   << MTLFileName);

  vtkSmartPointer<vtkPoints>     shared_vertexs = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkFloatArray> shared_normals = vtkSmartPointer<vtkFloatArray>::New();
  shared_normals->SetNumberOfComponents(3);

  std::map<std::string,vtkOBJImportedPolyDataWithMaterial*>  mtlName_to_Actor;

  {
    // always have at least one output
    vtkOBJImportedPolyDataWithMaterial*  newMaterial = new vtkOBJImportedPolyDataWithMaterial;
    newMaterial->SetSharedPoints(shared_vertexs);
    newMaterial->SetSharedNormals(shared_normals);
    poly_list.push_back(newMaterial);
    poly_list[0]->mtlProperties = parsedMTLs[0];

    mtlName_to_mtlData.clear();
    for( size_t k = 0; k < this->parsedMTLs.size(); ++k )
    {
      std::string mtlname_k(this->parsedMTLs[k]->name);
      mtlName_to_mtlData[mtlname_k] = this->parsedMTLs[k];
    }
  }

  vtkPoints* points           = poly_list.back()->points;
  vtkFloatArray* tcoords      = poly_list.back()->tcoords;
  vtkFloatArray* normals      = poly_list.back()->normals;
  vtkCellArray* polys         = poly_list.back()->polys;
  vtkCellArray* tcoord_polys  = poly_list.back()->tcoord_polys;
  vtkCellArray* pointElems    = poly_list.back()->pointElems;
  vtkCellArray* lineElems     = poly_list.back()->lineElems;
  vtkCellArray* normal_polys  = poly_list.back()->normal_polys;

  bool gotFirstUseMaterialTag = false;

  int numPolysWithTCoords = 0;
  bool hasTCoords = false;
  bool hasNormals = false;
  bool tcoords_same_as_verts = true;
  bool normals_same_as_verts = true;
  bool everything_ok = true; // (use of this flag avoids early return and associated memory leak)
  const double v_scale   = this->VertexScale;
  const bool   use_scale = (fabs(v_scale-1.0) > 1e-3 ) ;

  // -- work through the file line by line, assigning into the above 7 structures as appropriate --
  { // (make a local scope section to emphasise that the variables below are only used here)

  const int MAX_LINE = 4096;
  char rawLine[MAX_LINE];
  float xyz[3];

  int lineNr = 0;
  while (everything_ok && fgets(rawLine, MAX_LINE, in) != NULL)
  { /** While OK and there is another line in the file */
    lineNr++;
    char *pLine = rawLine;
    char *pEnd = rawLine + strlen(rawLine);

    // find the first non-whitespace character
    while (isspace(*pLine) && pLine < pEnd)
    {
      pLine++;
    }

    // this first non-whitespace is the command
    const char *cmd = pLine;

    // skip over non-whitespace
    while (!isspace(*pLine) && pLine < pEnd)
    {
      pLine++;
    }

    // terminate command
    if (pLine < pEnd)
    {
      *pLine = '\0';
      pLine++;
    }

    // in the OBJ format the first characters determine how to interpret the line:
    if (strcmp(cmd, "v") == 0)
    {
      // this is a vertex definition, expect three floats, separated by whitespace:
      if (sscanf(pLine, "%f %f %f", xyz, xyz+1, xyz+2) == 3)
      {
        if ( use_scale )
        {
          xyz[0] *= v_scale; xyz[1] *= v_scale; xyz[2] *= v_scale;
        }
        points->InsertNextPoint(xyz);
      }
      else
      {
        vtkErrorMacro(<<"Error reading 'v' at line " << lineNr);
        everything_ok = false;
      }
      if ( gotFirstUseMaterialTag && this->GetDebug() )
      {
        vtkWarningMacro("attempting to add vertices after usemtl ... ");
      }
    }
    else if (strcmp(cmd, "vt") == 0) /** Texture Coord, whango! */
    {
      // this is a tcoord, expect two floats, separated by whitespace:
      if (sscanf(pLine, "%f %f", xyz, xyz+1) == 2)
      {
        tcoords->InsertNextTuple(xyz);
      }
      else
      {
        vtkErrorMacro(<<"Error reading 'vt' at line " << lineNr);
        everything_ok = false;
      }
    }
    else if (strcmp(cmd, "vn") == 0)
    {
      // this is a normal, expect three floats, separated by whitespace:
      if (sscanf(pLine, "%f %f %f", xyz, xyz+1, xyz+2) == 3)
      {
        normals->InsertNextTuple(xyz);
        hasNormals = true;
      }
      else
      {
        vtkErrorMacro(<<"Error reading 'vn' at line " << lineNr);
        everything_ok = false;
      }
    }
    else if (strcmp(cmd, "p") == 0)
    {
      // this is a point definition, consisting of 1-based indices separated by whitespace and /
      pointElems->InsertNextCell(0); // we don't yet know how many points are to come

      int nVerts=0; // keep a count of how many there are

      while (everything_ok && pLine < pEnd)
      {
        // find next non-whitespace character
        while (isspace(*pLine) && pLine < pEnd)
        {
          pLine++;
        }

        if (pLine < pEnd)         // there is still data left on this line
        {
          int iVert;
          if (sscanf(pLine, "%d", &iVert) == 1)
          {
            pointElems->InsertCellPoint(iVert-1);
            nVerts++;
          }
          else if (strcmp(pLine, "\\\n") == 0)
          {
            // handle backslash-newline continuation
            if (fgets(rawLine, MAX_LINE, in) != NULL)
            {
              lineNr++;
              pLine = rawLine;
              pEnd = rawLine + strlen(rawLine);
              continue;
            }
            else
            {
              vtkErrorMacro(<<"Error reading continuation line at line " << lineNr);
              everything_ok = false;
            }
          }
          else
          {
            vtkErrorMacro(<<"Error reading 'p' at line " << lineNr);
            everything_ok = false;
          }
          // skip over what we just sscanf'd
          // (find the first whitespace character)
          while (!isspace(*pLine) && pLine < pEnd)
          {
            pLine++;
          }
        }
      }

      if (nVerts < 1)
      {
        vtkErrorMacro
          (
            <<"Error reading file near line " << lineNr
            << " while processing the 'p' command"
            );
        everything_ok = false;
      }

      // now we know how many points there were in this cell
      pointElems->UpdateCellCount(nVerts);
    }
    else if (strcmp(cmd, "l") == 0)
    {
      // this is a line definition, consisting of 1-based indices separated by whitespace and /
      lineElems->InsertNextCell(0); // we don't yet know how many points are to come

      int nVerts=0; // keep a count of how many there are

      while (everything_ok && pLine < pEnd)
      {
        // find next non-whitespace character
        while (isspace(*pLine) && pLine < pEnd)
        {
          pLine++;
        }

        if (pLine < pEnd)         // there is still data left on this line
        {
          int iVert, dummyInt;
          if (sscanf(pLine, "%d/%d", &iVert, &dummyInt) == 2)
          {
            // we simply ignore texture information
            lineElems->InsertCellPoint(iVert-1);
            nVerts++;
          }
          else if (sscanf(pLine, "%d", &iVert) == 1)
          {
            lineElems->InsertCellPoint(iVert-1);
            nVerts++;
          }
          else if (strcmp(pLine, "\\\n") == 0)
          {
            // handle backslash-newline continuation
            if (fgets(rawLine, MAX_LINE, in) != NULL)
            {
              lineNr++;
              pLine = rawLine;
              pEnd = rawLine + strlen(rawLine);
              continue;
            }
            else
            {
              vtkErrorMacro(<<"Error reading continuation line at line " << lineNr);
              everything_ok = false;
            }
          }
          else
          {
            vtkErrorMacro(<<"Error reading 'l' at line " << lineNr);
            everything_ok = false;
          }
          // skip over what we just sscanf'd
          // (find the first whitespace character)
          while (!isspace(*pLine) && pLine < pEnd)
          {
            pLine++;
          }
        }
      }

      if (nVerts < 2)
      {
        vtkErrorMacro
          (
            <<"Error reading file near line " << lineNr
            << " while processing the 'l' command"
            );
        everything_ok = false;
      }

      // now we know how many points there were in this cell
      lineElems->UpdateCellCount(nVerts);
    }
    else if (strcmp(cmd, "f") == 0)
    {
      // this is a face definition, consisting of 1-based indices separated by whitespace and /

      polys->InsertNextCell(0); // we don't yet know how many points are to come
      tcoord_polys->InsertNextCell(0);
      normal_polys->InsertNextCell(0);

      int nVerts=0, nTCoords=0, nNormals=0; // keep a count of how many of each there are

      while (everything_ok && pLine < pEnd)
      {
        // find the first non-whitespace character
        while (isspace(*pLine) && pLine < pEnd)
        {
          pLine++;
        }

        if (pLine < pEnd)         // there is still data left on this line
        {
          int iVert,iTCoord,iNormal;
          if (sscanf(pLine, "%d/%d/%d", &iVert, &iTCoord, &iNormal) == 3)
          {
            polys->InsertCellPoint(iVert-1); // convert to 0-based index
            nVerts++;
            tcoord_polys->InsertCellPoint(iTCoord-1);
            nTCoords++;
            normal_polys->InsertCellPoint(iNormal-1);
            nNormals++;
            if (iTCoord != iVert)
              tcoords_same_as_verts = false;
            if (iNormal != iVert)
              normals_same_as_verts = false;
          }
          else if (sscanf(pLine, "%d//%d", &iVert, &iNormal) == 2)
          {
            polys->InsertCellPoint(iVert-1);
            nVerts++;
            normal_polys->InsertCellPoint(iNormal-1);
            nNormals++;
            if (iNormal != iVert)
              normals_same_as_verts = false;
          }
          else if (sscanf(pLine, "%d/%d", &iVert, &iTCoord) == 2)
          {
            polys->InsertCellPoint(iVert-1);
            nVerts++;
            tcoord_polys->InsertCellPoint(iTCoord-1);
            nTCoords++;
            if (iTCoord != iVert)
              tcoords_same_as_verts = false;
          }
          else if (sscanf(pLine, "%d", &iVert) == 1)
          {
            polys->InsertCellPoint(iVert-1);
            nVerts++;
          }
          else if (strcmp(pLine, "\\\n") == 0)
          {
            // handle backslash-newline continuation
            if (fgets(rawLine, MAX_LINE, in) != NULL)
            {
              lineNr++;
              pLine = rawLine;
              pEnd = rawLine + strlen(rawLine);
              continue;
            }
            else
            {
              vtkErrorMacro(<<"Error reading continuation line at line " << lineNr);
              everything_ok = false;
            }
          }
          else
          {
            vtkErrorMacro(<<"Error reading 'f' at line " << lineNr);
            everything_ok = false;
          }
          // skip over what we just read
          // (find the first whitespace character)
          while (!isspace(*pLine) && pLine < pEnd)
          {
            pLine++;
          }
        }
      }

      // count of tcoords and normals must be equal to number of vertices or zero
      if ( nVerts < 3 ||
           (nTCoords > 0 && nTCoords != nVerts) ||
           (nNormals > 0 && nNormals != nVerts)
        )
      {
        vtkErrorMacro
          (
            <<"Error reading file near line " << lineNr
            << " while processing the 'f' command"
            );
        everything_ok = false;
      }

      // now we know how many points there were in this cell
      polys->UpdateCellCount(nVerts);
      tcoord_polys->UpdateCellCount(nTCoords);
      normal_polys->UpdateCellCount(nNormals);

      // also make a note of whether any cells have tcoords, and whether any have normals
      numPolysWithTCoords += (int) (nTCoords)>0;
      if ( (!hasTCoords)&&(nTCoords > 0) )
      {
        vtkDebugMacro("got texture coords in obj file! nTCoords = " << nTCoords);
        hasTCoords = true;
      }
      else if (nTCoords==0)
      {
        vtkDebugMacro("did NOT get texture coords in obj file!");
      }
      if (nNormals > 0)
      {
        hasNormals = true;
      }
    }
    else if (strcmp(cmd, "usemtl") == 0)
    {
      // find the first non-whitespace character
      while (isspace(*pLine) && pLine < pEnd)
      {
        pLine++;
      }
      std::string strLine(pLine);
      vtkDebugMacro("strLine = " << strLine);
      int idxNewLine = strLine.find_first_of("\r\n");
      std::string mtl_name = strLine.substr(0, idxNewLine);
      vtkDebugMacro("'Use Material' command, usemtl with name: " << mtl_name);

      if (! mtlName_to_mtlData.count(mtl_name))
      {
        vtkErrorMacro(" material '" << mtl_name << "' appears in OBJ but not MTL file?");
      }
      // if this is the first usemtl then assign it to the
      // poly_list[0]
      if (!gotFirstUseMaterialTag)
      {
        poly_list[0]->materialName = mtl_name;
        poly_list[0]->mtlProperties = mtlName_to_mtlData[mtl_name];
        mtlName_to_Actor[mtl_name] = poly_list[0];
        // yep we have a usemtl command. check to make sure idiots don't try to add vertices later.
        gotFirstUseMaterialTag = true;
      }
      int mtlCount = mtlName_to_Actor.count(mtl_name);
      if ( 0 == mtlCount )
      {
        // new material encountered; bag and tag it, make a new named-poly-data-container
        vtkOBJImportedPolyDataWithMaterial*  newMaterial = new vtkOBJImportedPolyDataWithMaterial;
        newMaterial->SetSharedPoints(shared_vertexs);
        newMaterial->SetSharedNormals(shared_normals);
        poly_list.push_back(newMaterial);

        poly_list.back()->materialName  = mtl_name;
        poly_list.back()->mtlProperties = mtlName_to_mtlData[mtl_name];
        mtlName_to_Actor[mtl_name] = poly_list.back();

        vtkOBJImportedPolyDataWithMaterial* active = mtlName_to_Actor[mtl_name];

        vtkDebugMacro("name of material is: " << active->materialName);

        /** slightly tricky: all multi-polys share the vertex, normals, and tcoords,
                                 but define unique polygons... */
        polys           = active->polys; // Update pointers reading file further
        tcoord_polys    = active->tcoord_polys;
        pointElems      = active->pointElems;
        lineElems       = active->lineElems;
        normal_polys    = active->normal_polys;
      }
    else /** This material name already exists; switch back to it! */
    {
      vtkOBJImportedPolyDataWithMaterial* known_mtl = mtlName_to_Actor[mtl_name];
      vtkDebugMacro("switching to append faces with pre-existing material named "
                    << known_mtl->materialName);
      polys           = known_mtl->polys; // Update pointers reading file further
      tcoord_polys    = known_mtl->tcoord_polys;
      pointElems      = known_mtl->pointElems;
      lineElems       = known_mtl->lineElems;
      normal_polys    = known_mtl->normal_polys;
    }
    }
    else
    {
      vtkDebugMacro(<<"Ignoring line: "<<rawLine);
    }
  }  /** Looping over lines of file */ // (end of while loop)
  } // (end of local scope section)

  // we have finished with the file
  fclose(in);

  /** based on how many used materials are present,
                 set the number of output ports of vtkPolyData */
  this->SetNumberOfOutputPorts( poly_list.size() );
  vtkDebugMacro("vtkOBJPolyDataProcessor.cxx, set # of output ports to "
                << poly_list.size());
  this->outVector_of_vtkPolyData.clear();
  for( int i = 0; i < (int)poly_list.size(); ++i)
  {
    vtkSmartPointer<vtkPolyData> poly_data = vtkSmartPointer<vtkPolyData>::New();
    this->outVector_of_vtkPolyData.push_back(poly_data);
  }

  if (everything_ok)   // (otherwise just release allocated memory and return)
  {
    // -- now turn this lot into a useable vtkPolyData --
    // loop over the materials found in the obj file
    for(size_t outputIndex = 0; outputIndex < poly_list.size(); ++outputIndex)
    {
      vtkOBJImportedPolyDataWithMaterial* active = poly_list[outputIndex];
      vtkSmartPointer<vtkPolyData> output = outVector_of_vtkPolyData[outputIndex];
      polys           = active->polys; // Update pointers reading file further
      tcoord_polys    = active->tcoord_polys;
      pointElems      = active->pointElems;
      lineElems       = active->lineElems;
      normal_polys    = active->normal_polys;
      vtkDebugMacro("generating output polydata ....  \n"
                    << "tcoords same as verts!? " << tcoords_same_as_verts
                    << " ... hasTCoords?" << hasTCoords
                    << " ... numPolysWithTCoords = " << numPolysWithTCoords);
      // if there are no tcoords or normals or they match exactly
      // then we can just copy the data into the output (easy!)
      if ((!hasTCoords || tcoords_same_as_verts) &&
          (!hasNormals || normals_same_as_verts))
      { // ...
        vtkDebugMacro(<<"Copying file data into the output directly");

        output->SetPoints(points);
        if (pointElems->GetNumberOfCells())
        {
          output->SetVerts(pointElems);
        }
        if (lineElems->GetNumberOfCells())
        {
          output->SetLines(lineElems);
        }
        if (polys->GetNumberOfCells())
        {
          output->SetPolys(polys);
        }

        // if there is an exact correspondence between tcoords and vertices then can simply
        // assign the tcoords points as point data
        if (hasTCoords && tcoords_same_as_verts)
          output->GetPointData()->SetTCoords(tcoords);

        // if there is an exact correspondence between normals and vertices then can simply
        // assign the normals as point data
        if (hasNormals && normals_same_as_verts)
        {
          output->GetPointData()->SetNormals(normals);
        }
        output->Squeeze();
      }
      // otherwise we can duplicate the vertices as necessary (a bit slower)
      else
      {
        vtkDebugMacro(<<"Duplicating vertices so that tcoords and normals are correct");
        vtkPoints *new_points = vtkPoints::New();
        vtkFloatArray *new_tcoords = vtkFloatArray::New();
        new_tcoords->SetNumberOfComponents(2);
        vtkFloatArray *new_normals = vtkFloatArray::New();
        new_normals->SetNumberOfComponents(3);
        vtkCellArray *new_polys = vtkCellArray::New();

        // for each poly, copy its vertices into new_points (and point at them)
        // also copy its tcoords into new_tcoords
        // also copy its normals into new_normals
        polys->InitTraversal();
        tcoord_polys->InitTraversal();
        normal_polys->InitTraversal();

        vtkIdType dummy_warning_prevention_mechanism[1];
        vtkIdType n_pts=-1,*pts=dummy_warning_prevention_mechanism;
        vtkIdType n_tcoord_pts=-1,*tcoord_pts=dummy_warning_prevention_mechanism;
        vtkIdType n_normal_pts=-1,*normal_pts=dummy_warning_prevention_mechanism;
        for (int i = 0; i < polys->GetNumberOfCells(); ++i)
        {
          polys->GetNextCell(n_pts,pts);
          tcoord_polys->GetNextCell(n_tcoord_pts,tcoord_pts);
          normal_polys->GetNextCell(n_normal_pts,normal_pts);

          // If some vertices have tcoords and not others (likewise normals)
          // then we must do something else VTK will complain. (crash on render attempt)
          // Easiest solution is to delete polys that don't have complete tcoords (if there
          // are any tcoords in the dataset) or normals (if there are any normals in the dataset).

          if (
            (n_pts != n_tcoord_pts && hasTCoords) ||
            (n_pts != n_normal_pts && hasNormals)
            )
          {
            // skip this poly
            vtkDebugMacro(<<"Skipping poly "<<i+1<<" (1-based index)");
          }
          else
          {
            // copy the corresponding points, tcoords and normals across
            for (int j = 0; j < n_pts; ++j)
            {
              // copy the tcoord for this point across (if there is one)
              if (n_tcoord_pts>0)
              {
                new_tcoords->InsertNextTuple(tcoords->GetTuple(tcoord_pts[j]));
              }
              // copy the normal for this point across (if there is one)
              if (n_normal_pts>0)
              {
                new_normals->InsertNextTuple(normals->GetTuple(normal_pts[j]));
              }
              // copy the vertex into the new structure and update
              // the vertex index in the polys structure (pts is a pointer into it)
              pts[j] = new_points->InsertNextPoint(points->GetPoint(pts[j]));
            }
            // copy this poly (pointing at the new points) into the new polys list
            new_polys->InsertNextCell(n_pts,pts);
          }
        }

        // use the new structures for the output
        output->SetPoints(new_points);
        output->SetPolys(new_polys);
        vtkDebugMacro(" set new points, count = "
                      << new_points->GetNumberOfPoints() << " ...");
        vtkDebugMacro(" set new polys, count = "
                      << new_polys->GetNumberOfCells() << " ...");

        if (hasTCoords)
        {
          output->GetPointData()->SetTCoords(new_tcoords);
          vtkDebugMacro(" set new tcoords");
        }
        if (hasNormals)
        {
          output->GetPointData()->SetNormals(new_normals);
          vtkDebugMacro(" set new normals");
        }

        // TODO: fixup for pointElems and lineElems too
        output->Squeeze();

        new_points->Delete();
        new_polys->Delete();
        new_tcoords->Delete();
        new_normals->Delete();
      }
    }
  }

  if (!everything_ok)
  {
    SetSuccessParsingFiles(false);
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkOBJPolyDataProcessor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "FileName: "
     << (this->FileName.empty() ? this->FileName : "(none)") << "\n";
  os << indent << "MTLFileName: "
     << (this->MTLFileName.empty() ? this->MTLFileName : "(none)") << "\n";
  os << indent << "TexturePath: "
     << (this->TexturePath.empty() ? this->TexturePath : "(none)") << "\n";
}


//----------------------------------------------------------------------------
vtkPolyData* vtkOBJPolyDataProcessor::GetOutput(int idx)
{
  if ( idx < (int)outVector_of_vtkPolyData.size() )
  {
    return outVector_of_vtkPolyData[idx];
  }
  else
  {
    return NULL;
  }
}
