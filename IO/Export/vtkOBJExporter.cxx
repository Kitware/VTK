/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOBJExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOBJExporter.h"

#include "vtkActorCollection.h"
#include "vtkAssemblyNode.h"
#include "vtkAssemblyPath.h"
#include "vtkCellArray.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkGeometryFilter.h"
#include "vtkImageData.h"
#include "vtkImageFlip.h"
#include "vtkMapper.h"
#include "vtkNew.h"
#include "vtkNumberToString.h"
#include "vtkObjectFactory.h"
#include "vtkPNGWriter.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkTexture.h"
#include "vtkTransform.h"

#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#include <sstream>

vtkStandardNewMacro(vtkOBJExporter);

vtkOBJExporter::vtkOBJExporter()
{
  this->FilePrefix = nullptr;
  this->OBJFileComment = nullptr;
  this->MTLFileComment = nullptr;
  this->FlipTexture = false;
  this->SetOBJFileComment("wavefront obj file written by the visualization toolkit");
  this->SetMTLFileComment("wavefront mtl file written by the visualization toolkit");
}

vtkOBJExporter::~vtkOBJExporter()
{
  delete[] this->OBJFileComment;
  delete[] this->MTLFileComment;
  delete[] this->FilePrefix;
}

void vtkOBJExporter::WriteData()
{
  // make sure the user specified a filename
  if (this->FilePrefix == nullptr)
  {
    vtkErrorMacro(<< "Please specify file prefix to use");
    return;
  }

  vtkRenderer* ren = this->ActiveRenderer;
  if (!ren)
  {
    ren = this->RenderWindow->GetRenderers()->GetFirstRenderer();
  }

  // make sure it has at least one actor
  if (ren->GetActors()->GetNumberOfItems() < 1)
  {
    vtkErrorMacro(<< "no actors found for writing .obj file.");
    return;
  }

  // try opening the files
  std::string objFilePath = std::string(this->FilePrefix) + ".obj";
  std::string filePrefix(this->FilePrefix);
  // get the model name which isthe last component of the FilePrefix
  std::string modelName;
  if (filePrefix.find_last_of("/") != std::string::npos)
  {
    modelName = filePrefix.substr(filePrefix.find_last_of("/") + 1);
  }
  else
  {
    modelName = filePrefix;
  }

  vtksys::ofstream fpObj(objFilePath.c_str(), ios::out);
  if (!fpObj)
  {
    vtkErrorMacro(<< "unable to open " << objFilePath);
    return;
  }
  std::string mtlFilePath = std::string(this->FilePrefix) + ".mtl";
  vtksys::ofstream fpMtl(mtlFilePath.c_str(), ios::out);
  if (!fpMtl)
  {
    fpMtl.close();
    vtkErrorMacro(<< "unable to open .mtl files ");
    return;
  }

  //
  //  Write header
  //
  vtkDebugMacro("Writing wavefront files");
  if (this->GetOBJFileComment())
  {
    fpObj << "#  " << this->GetOBJFileComment() << "\n\n";
  }

  std::string mtlFileName = vtksys::SystemTools::GetFilenameName(mtlFilePath);
  fpObj << "mtllib " << mtlFileName << "\n\n";
  if (this->GetMTLFileComment())
  {
    fpMtl << "# " << this->GetMTLFileComment() << "\n\n";
  }

  vtkActorCollection* allActors = ren->GetActors();
  vtkCollectionSimpleIterator actorsIt;
  vtkActor* anActor;
  int idStart = 1;
  for (allActors->InitTraversal(actorsIt); (anActor = allActors->GetNextActor(actorsIt));)
  {
    vtkAssemblyPath* aPath;
    for (anActor->InitPathTraversal(); (aPath = anActor->GetNextPath());)
    {
      vtkActor* aPart = vtkActor::SafeDownCast(aPath->GetLastNode()->GetViewProp());
      this->WriteAnActor(aPart, fpObj, fpMtl, modelName, idStart);
    }
  }
  // Write texture files
  for (auto t : this->TextureFileMap)
  {
    std::stringstream fullFileName;
    fullFileName << this->FilePrefix << t.first;
    auto writeTexture = vtkSmartPointer<vtkPNGWriter>::New();
    if (this->FlipTexture)
    {
      auto flip = vtkSmartPointer<vtkImageFlip>::New();
      flip->SetInputData(t.second->GetInput());
      flip->SetFilteredAxis(1);
      flip->Update();
      writeTexture->SetInputData(flip->GetOutput());
    }
    else
    {
      writeTexture->SetInputData(t.second->GetInput());
    }
    writeTexture->SetFileName(fullFileName.str().c_str());
    writeTexture->Write();
  }
  fpObj.close();
  fpMtl.close();
}

void vtkOBJExporter::WriteAnActor(
  vtkActor* anActor, std::ostream& fpObj, std::ostream& fpMtl, std::string& modelName, int& idStart)
{
  vtkDataSet* ds;
  vtkNew<vtkPolyData> pd;
  vtkPointData* pntData;
  vtkPoints* points;
  vtkDataArray* tcoords;
  int i, i1, i2, idNext;
  vtkProperty* prop;
  double* tempd;
  double* p;
  vtkCellArray* cells;
  vtkNew<vtkTransform> trans;
  vtkIdType npts = 0;
  const vtkIdType* indx = nullptr;

  // see if the actor has a mapper. it could be an assembly
  if (anActor->GetMapper() == nullptr)
  {
    return;
  }
  // write out the material properties to the mat file
  prop = anActor->GetProperty();
  if (anActor->GetVisibility() == 0)
  {
    return;
  }
  vtkNumberToString convert;
  double temp;
  fpMtl << "newmtl mtl" << idStart << "\n";
  tempd = prop->GetAmbientColor();
  temp = prop->GetAmbient();
  fpMtl << "Ka " << convert(temp * tempd[0]) << " " << convert(temp * tempd[1]) << " "
        << convert(temp * tempd[2]) << "\n";
  tempd = prop->GetDiffuseColor();
  temp = prop->GetDiffuse();
  fpMtl << "Kd " << convert(temp * tempd[0]) << " " << convert(temp * tempd[1]) << " "
        << convert(temp * tempd[2]) << "\n";
  tempd = prop->GetSpecularColor();
  temp = prop->GetSpecular();
  fpMtl << "Ks " << convert(temp * tempd[0]) << " " << convert(temp * tempd[1]) << " "
        << convert(temp * tempd[2]) << "\n";
  fpMtl << "Ns " << convert(prop->GetSpecularPower()) << "\n";
  fpMtl << "Tr " << convert(prop->GetOpacity()) << "\n";
  fpMtl << "illum 3\n";

  // Actor has the texture
  bool hasTexture = anActor->GetTexture() != nullptr;
  ;

  // Actor's property has the texture. We choose the albedo texture
  // since it seems to be similar to the texture we expect
  auto allTextures = prop->GetAllTextures();
  bool hasTextureProp = allTextures.find("albedoTex") != allTextures.end();
  if (hasTexture)
  {
    std::stringstream textureFileName;
    textureFileName << "texture" << idStart << ".png";
    fpMtl << "map_Kd " << modelName << textureFileName.str() << "\n\n";
    this->TextureFileMap[textureFileName.str()] = anActor->GetTexture();
  }
  else if (hasTextureProp)
  {
    std::stringstream textureFileName;
    textureFileName << "albedoTex"
                    << "_" << idStart << ".png";
    fpMtl << "map_Kd " << modelName << textureFileName.str() << "\n\n";
    auto albedoTexture = allTextures.find("albedoTex");
    this->TextureFileMap[textureFileName.str()] = albedoTexture->second;
    this->FlipTexture = true;
  }

  // get the mappers input and matrix
  ds = anActor->GetMapper()->GetInput();
  // see if the mapper has an input.
  if (ds == nullptr)
  {
    return;
  }
  anActor->GetMapper()->GetInputAlgorithm()->Update();
  trans->SetMatrix(anActor->vtkProp3D::GetMatrix());

  // we really want polydata
  if (ds->GetDataObjectType() != VTK_POLY_DATA)
  {
    vtkNew<vtkGeometryFilter> gf;
    gf->SetInputConnection(anActor->GetMapper()->GetInputConnection(0, 0));
    gf->Update();
    pd->DeepCopy(gf->GetOutput());
  }
  else
  {
    pd->DeepCopy(ds);
  }

  // write out the points
  points = vtkPoints::New();
  trans->TransformPoints(pd->GetPoints(), points);
  for (i = 0; i < points->GetNumberOfPoints(); i++)
  {
    p = points->GetPoint(i);
    fpObj << "v " << convert(p[0]) << " " << convert(p[1]) << " " << convert(p[2]) << "\n";
  }
  idNext = idStart + static_cast<int>(points->GetNumberOfPoints());
  points->Delete();

  // write out the point data
  pntData = pd->GetPointData();
  if (pntData->GetNormals())
  {
    vtkNew<vtkFloatArray> normals;
    normals->SetNumberOfComponents(3);
    trans->TransformNormals(pntData->GetNormals(), normals);
    for (i = 0; i < normals->GetNumberOfTuples(); i++)
    {
      p = normals->GetTuple(i);
      fpObj << "vn " << convert(p[0]) << " " << convert(p[1]) << " " << convert(p[2]) << "\n";
    }
  }

  tcoords = pntData->GetTCoords();
  if (tcoords)
  {
    for (i = 0; i < tcoords->GetNumberOfTuples(); i++)
    {
      p = tcoords->GetTuple(i);
      fpObj << "vt " << convert(p[0]) << " " << convert(p[1]) << " " << 0.0 << "\n";
    }
  }

  // write out a group name and material
  fpObj << "\ng grp" << idStart << "\n";
  fpObj << "usemtl mtl" << idStart << "\n";
  // write out verts if any
  if (pd->GetNumberOfVerts() > 0)
  {
    cells = pd->GetVerts();
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
    {
      fpObj << "p ";
      for (i = 0; i < npts; i++)
      {
        // treating vtkIdType as int
        fpObj << static_cast<int>(indx[i]) + idStart << " ";
      }
      fpObj << "\n";
    }
  }

  // write out lines if any
  if (pd->GetNumberOfLines() > 0)
  {
    cells = pd->GetLines();
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
    {
      fpObj << "l ";
      if (tcoords)
      {
        for (i = 0; i < npts; i++)
        {
          // treating vtkIdType as int
          fpObj << static_cast<int>(indx[i]) + idStart << "/"
                << static_cast<int>(indx[i]) + idStart;
        }
      }
      else
      {
        for (i = 0; i < npts; i++)
        {
          // treating vtkIdType as int
          fpObj << static_cast<int>(indx[i]) + idStart << " ";
        }
      }
      fpObj << "\n";
    }
  }
  // write out polys if any
  if (pd->GetNumberOfPolys() > 0)
  {
    cells = pd->GetPolys();
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
    {
      fpObj << "f ";
      for (i = 0; i < npts; i++)
      {
        if (pntData->GetNormals())
        {
          if (tcoords)
          {
            // treating vtkIdType as int
            fpObj << static_cast<int>(indx[i]) + idStart << "/"
                  << static_cast<int>(indx[i]) + idStart << "/"
                  << static_cast<int>(indx[i]) + idStart << " ";
          }
          else
          {
            // treating vtkIdType as int
            fpObj << static_cast<int>(indx[i]) + idStart << "//"
                  << static_cast<int>(indx[i]) + idStart << " ";
          }
        }
        else
        {
          if (tcoords)
          {
            // treating vtkIdType as int
            fpObj << static_cast<int>(indx[i]) + idStart << "/"
                  << static_cast<int>(indx[i]) + idStart << " ";
          }
          else
          {
            // treating vtkIdType as int
            fpObj << static_cast<int>(indx[i]) + idStart << " ";
          }
        }
      }
      fpObj << "\n";
    }
  }

  // write out tstrips if any
  if (pd->GetNumberOfStrips() > 0)
  {
    cells = pd->GetStrips();
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
    {
      for (i = 2; i < npts; i++)
      {
        if (i % 2 == 0)
        {
          i1 = i - 2;
          i2 = i - 1;
        }
        else
        {
          i1 = i - 1;
          i2 = i - 2;
        }
        if (pntData->GetNormals())
        {
          if (tcoords)
          {
            // treating vtkIdType as int
            fpObj << "f " << static_cast<int>(indx[i1]) + idStart << "/"
                  << static_cast<int>(indx[i1]) + idStart << "/"
                  << static_cast<int>(indx[i1]) + idStart << " ";
            fpObj << static_cast<int>(indx[i2]) + idStart << "/"
                  << static_cast<int>(indx[i2]) + idStart << "/"
                  << static_cast<int>(indx[i2]) + idStart << " ";
            fpObj << static_cast<int>(indx[i]) + idStart << "/"
                  << static_cast<int>(indx[i]) + idStart << "/"
                  << static_cast<int>(indx[i]) + idStart << "\n";
          }
          else
          {
            // treating vtkIdType as int
            fpObj << static_cast<int>(indx[i2]) + idStart << "//"
                  << static_cast<int>(indx[i2]) + idStart << " ";
            fpObj << static_cast<int>(indx[i]) + idStart << "//"
                  << static_cast<int>(indx[i]) + idStart << "\n";
          }
        }
        else
        {
          if (tcoords)
          {
            // treating vtkIdType as int
            fpObj << "f " << static_cast<int>(indx[i1]) + idStart << "/"
                  << static_cast<int>(indx[i1]) + idStart << " ";
            fpObj << static_cast<int>(indx[i2]) + idStart << "/"
                  << static_cast<int>(indx[i2]) + idStart << " ";
            fpObj << static_cast<int>(indx[i]) + idStart << "/"
                  << static_cast<int>(indx[i]) + idStart << "\n";
          }
          else
          {
            // treating vtkIdType as int
            fpObj << "f " << static_cast<int>(indx[i1]) + idStart << " "
                  << static_cast<int>(indx[i2]) + idStart << " "
                  << static_cast<int>(indx[i]) + idStart << "\n";
          }
        }
      }
    }
  }

  idStart = idNext;
}

void vtkOBJExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FilePrefix: " << (this->FilePrefix ? this->FilePrefix : "(null)") << "\n";
  os << indent << "OBJFileComment: " << (this->OBJFileComment ? this->OBJFileComment : "(null)")
     << "\n";
  os << indent << "MTLFileComment: " << (this->MTLFileComment ? this->MTLFileComment : "(null)")
     << "\n";
}
