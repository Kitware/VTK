// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkGLTFDocumentLoader.h>
#include <vtkGLTFReader.h>
#include <vtkObjectFactory.h>

#include <algorithm>
#include <iostream>

//------------------------------------------------------------------------------
class vtkTestGLTFDocumentLoaderMeshopt : public vtkGLTFDocumentLoader
{
public:
  static vtkTestGLTFDocumentLoaderMeshopt* New();
  vtkTypeMacro(vtkTestGLTFDocumentLoaderMeshopt, vtkGLTFDocumentLoader);

  //----------------------------------------------------------------------------
  std::vector<std::string> GetSupportedExtensions() override
  {
    std::vector<std::string> extensions = this->Superclass::GetSupportedExtensions();
    extensions.emplace_back("KHR_meshopt_compression");
    return extensions;
  }
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkTestGLTFDocumentLoaderMeshopt);

//------------------------------------------------------------------------------
int TestGLTFDocumentLoaderMeshopt(int argc, char* argv[])
{
  if (argc <= 1)
  {
    std::cout << "Usage: " << argv[0] << "<gltf file>" << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkTestGLTFDocumentLoaderMeshopt> loader;
  loader->LoadModelMetaDataFromFile(argv[1]);

  std::shared_ptr<vtkGLTFDocumentLoader::Model> model = loader->GetInternalModel();

  if (!model)
  {
    std::cerr << "Failed to get the internal model." << std::endl;
    return EXIT_FAILURE;
  }

  bool meshoptExt = std::any_of(model->BufferViews.begin(), model->BufferViews.end(),
    [](const vtkGLTFDocumentLoader::BufferView& bufferView)
    { return bufferView.ExtensionMetaData.MeshoptMetaData.Buffer >= 0; });

  if (!meshoptExt)
  {
    std::cerr << "Failed to read KHR_meshopt_compression extension." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
