#include "vtkGLTFTexture.h"

#include "vtkGLTFDocumentLoader.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkTexture.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkGLTFTexture);

//------------------------------------------------------------------------------
vtkSmartPointer<vtkTexture> vtkGLTFTexture::GetVTKTexture()
{
  vtkNew<vtkTexture> texture;
  texture->SetColorModeToDirectScalars();
  texture->SetBlendingMode(vtkTexture::VTK_TEXTURE_BLENDING_MODE_MODULATE);
  // Approximate filtering settings
  if (this->Sampler.MinFilter == vtkGLTFDocumentLoader::Sampler::FilterType::NEAREST ||
    this->Sampler.MinFilter == vtkGLTFDocumentLoader::Sampler::FilterType::LINEAR)
  {
    texture->MipmapOff();
  }
  else
  {
    texture->MipmapOn();
  }

  if (this->Sampler.WrapS == vtkGLTFDocumentLoader::Sampler::WrapType::CLAMP_TO_EDGE ||
    this->Sampler.WrapT == vtkGLTFDocumentLoader::Sampler::WrapType::CLAMP_TO_EDGE)
  {
    texture->RepeatOff();
    texture->EdgeClampOn();
  }
  else if (this->Sampler.WrapS == vtkGLTFDocumentLoader::Sampler::WrapType::REPEAT ||
    this->Sampler.WrapT == vtkGLTFDocumentLoader::Sampler::WrapType::REPEAT)
  {
    texture->RepeatOn();
    texture->EdgeClampOff();
  }
  else
  {
    vtkWarningWithObjectMacro(nullptr, "Mirrored texture wrapping is not supported!");
  }

  if (this->Sampler.MinFilter == vtkGLTFDocumentLoader::Sampler::FilterType::LINEAR ||
    this->Sampler.MinFilter == vtkGLTFDocumentLoader::Sampler::FilterType::LINEAR_MIPMAP_NEAREST ||
    this->Sampler.MinFilter == vtkGLTFDocumentLoader::Sampler::FilterType::NEAREST_MIPMAP_LINEAR ||
    this->Sampler.MinFilter == vtkGLTFDocumentLoader::Sampler::FilterType::LINEAR_MIPMAP_LINEAR ||
    this->Sampler.MagFilter == vtkGLTFDocumentLoader::Sampler::FilterType::LINEAR ||
    this->Sampler.MagFilter == vtkGLTFDocumentLoader::Sampler::FilterType::LINEAR_MIPMAP_NEAREST ||
    this->Sampler.MagFilter == vtkGLTFDocumentLoader::Sampler::FilterType::NEAREST_MIPMAP_LINEAR ||
    this->Sampler.MagFilter == vtkGLTFDocumentLoader::Sampler::FilterType::LINEAR_MIPMAP_LINEAR)
  {
    texture->InterpolateOn();
  }
  texture->SetInputData(this->Image);
  return texture;
}
