/*=========================================================================
  Program:   Visualization Toolkit
  Module:    vtkOBJImporterInternals.h
  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#ifndef vtkOBJImporterInternals_h
#define vtkOBJImporterInternals_h

#include <string>
#include "vtkOBJImporter.h"
#include "vtkPolyDataAlgorithm.h"
#include <memory>
#include <vector>
#include <map>
#include "vtkActor.h"

const int OBJ_FILENAME_LENGTH = 8192;
const int MATERIAL_NAME_SIZE  = 8192;

struct vtkOBJImportedMaterial
{
  char name[MATERIAL_NAME_SIZE]; // use std::array<char,N> when got {gcc4.7+,vs2012+}
  char texture_filename[OBJ_FILENAME_LENGTH];
  double amb[3];
  double diff[3];
  double spec[3];
  double reflect;
  double refract;
  double trans;
  double shiny;
  double glossy;
  double refract_index;
  double get_amb_coeff()
  {
    return sqrt( amb[0]*amb[0]+amb[1]*amb[1]+amb[2]*amb[2] );
  }
  double get_diff_coeff()
  {
    return sqrt( diff[0]*diff[0]+diff[1]*diff[1]+diff[2]*diff[2] );
  }
  double get_spec_coeff()
  {
    return sqrt( spec[0]*spec[0]+spec[1]*spec[1]+spec[2]*spec[2] );
  }
  const char *GetClassName() {return "vtkOBJImportedMaterial";}
  vtkOBJImportedMaterial();
};


void obj_set_material_defaults(vtkOBJImportedMaterial* mtl);

struct vtkOBJImportedPolyDataWithMaterial;

class vtkOBJPolyDataProcessor : public vtkPolyDataAlgorithm
{
public:
  static vtkOBJPolyDataProcessor *New();
  vtkTypeMacro(vtkOBJPolyDataProcessor,vtkPolyDataAlgorithm)
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // Description:
  // Specify file name of Wavefront .obj file.
  void SetFileName(const char* arg)
  {
    if (arg == NULL)
    {
      return;
    }
    if (!strcmp(this->FileName.c_str(), arg))
    {
      return;
    }
    FileName    = std::string(arg);
  }
  void SetMTLfileName( const char* arg )
  {
    if (arg == NULL)
    {
      return;
    }
    if (!strcmp(this->MTLFileName.c_str(), arg))
    {
      return;
    }
    MTLFileName = std::string(arg);
    this->DefaultMTLFileName = false;
  }
  void SetTexturePath( const char* arg )
  {
    TexturePath = std::string(arg);
    if(TexturePath.empty())
      return;
#if defined(_WIN32)
    const char sep = '\\';
#else
    const char sep = '/';
#endif
    if(TexturePath.at(TexturePath.size()-1) != sep )
      TexturePath += sep;
  }
  const std::string& GetTexturePath(  ) const
  {
    return TexturePath;
  }

  const std::string& GetFileName(  ) const
  {
    return FileName;
  }

  const std::string& GetMTLFileName(  ) const
  {
    return MTLFileName;
  }

  vtkSetMacro(VertexScale,double)
  vtkGetMacro(VertexScale,double)
  vtkGetMacro(SuccessParsingFiles,int)

  virtual vtkPolyData* GetOutput(int idx);

  vtkOBJImportedMaterial*  GetMaterial(int k);

  std::string GetTextureFilename( int idx ); // return string by index

  double VertexScale; // scale vertices by this during import

  std::vector<vtkOBJImportedMaterial*>  parsedMTLs;
  std::map<std::string,vtkOBJImportedMaterial*>  mtlName_to_mtlData;

  // our internal parsing/storage
  std::vector<vtkOBJImportedPolyDataWithMaterial*> poly_list;

  // what gets returned to client code via GetOutput()
  std::vector<vtkSmartPointer<vtkPolyData> >  outVector_of_vtkPolyData;

  std::vector<vtkSmartPointer<vtkActor> >  actor_list;
  /////////////////////

  std::vector<vtkOBJImportedMaterial*> ParseOBJandMTL(std::string filename,int& result_code);

  void ReadVertices(bool gotFirstUseMaterialTag, char *pLine, float xyz, int lineNr, const double v_scale, bool everything_ok, vtkPoints* points, const bool use_scale);
protected:
  vtkOBJPolyDataProcessor();
  ~vtkOBJPolyDataProcessor() VTK_OVERRIDE;
  int RequestData(vtkInformation *,
                  vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE /*override*/;

  vtkSetMacro(SuccessParsingFiles,int)

  std::string FileName;     // filename (.obj) being read
  std::string MTLFileName;  // associated .mtl to *.obj, typically it is *.obj.mtl
  bool DefaultMTLFileName;  // tells whether default of *.obj.mtl to be used
  std::string TexturePath;
  int         SuccessParsingFiles;

private:
  vtkOBJPolyDataProcessor(const vtkOBJPolyDataProcessor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOBJPolyDataProcessor&) VTK_DELETE_FUNCTION;
};

class vtkRenderWindow;
class vtkRenderer;
void  bindTexturedPolydataToRenderWindow( vtkRenderWindow* renderWindow,
                                          vtkRenderer* renderer,
                                          vtkOBJPolyDataProcessor* reader );

#endif
// VTK-HeaderTest-Exclude: vtkOBJImporterInternals.h
