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
#ifndef __VTK_WRAP__

#include <string>
#include "vtkOBJImporter.h"
#include "vtkPolyDataAlgorithm.h"
#include <memory>
#include <vector>
#include <map>
#include "vtkActor.h"

struct vtkOBJImportedMaterial
{
  std::string name;
  std::string texture_filename;
  double amb[3];
  double diff[3];
  double spec[3];
  double map_Kd_scale[3];
  double map_Kd_offset[3];
  int illum;
  double reflect;
  double refract;
  double trans;
  double specularPower;
  double glossy;
  double refract_index;
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
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // Specify file name of Wavefront .obj file.
  void SetFileName(const char* arg)
  {
    if (arg == nullptr)
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
    if (arg == nullptr)
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
  ~vtkOBJPolyDataProcessor() override;
  int RequestData(vtkInformation *,
                  vtkInformationVector **, vtkInformationVector *) override /*override*/;

  vtkSetMacro(SuccessParsingFiles,int)

  std::string FileName;     // filename (.obj) being read
  std::string MTLFileName;  // associated .mtl to *.obj, typically it is *.obj.mtl
  bool DefaultMTLFileName;  // tells whether default of *.obj.mtl to be used
  std::string TexturePath;
  int         SuccessParsingFiles;

private:
  vtkOBJPolyDataProcessor(const vtkOBJPolyDataProcessor&) = delete;
  void operator=(const vtkOBJPolyDataProcessor&) = delete;
};

class vtkRenderWindow;
class vtkRenderer;
void  bindTexturedPolydataToRenderWindow( vtkRenderWindow* renderWindow,
                                          vtkRenderer* renderer,
                                          vtkOBJPolyDataProcessor* reader );

#endif
#endif
// VTK-HeaderTest-Exclude: vtkOBJImporterInternals.h
