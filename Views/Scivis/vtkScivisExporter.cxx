// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkScivisExporter.h"

#include "vtkBMPWriter.h"
#include "vtkGLTFExporter.h"
#include "vtkImageData.h"
#include "vtkImageWriter.h"
#include "vtkJPEGWriter.h"
#include "vtkNew.h"
#include "vtkOBJExporter.h"
#include "vtkObjectFactory.h"
#include "vtkPNGWriter.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkScivisView.h"
#include "vtkTIFFWriter.h"
#include "vtkVRMLExporter.h"
#include "vtkWindowToImageFilter.h"
#include "vtkX3DExporter.h"

#include <vtksys/SystemTools.hxx>

#include <string>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkScivisExporter);

namespace
{
// The extension of `filename`, lower cased, or an empty string when it has none.
std::string Extension(const char* filename)
{
  return vtksys::SystemTools::LowerCase(
    vtksys::SystemTools::GetFilenameLastExtension(filename ? filename : ""));
}
}

//------------------------------------------------------------------------------
vtkScivisExporter::vtkScivisExporter()
{
  this->Magnification = 1;
  this->TransparentBackground = false;
}

//------------------------------------------------------------------------------
vtkScivisExporter::~vtkScivisExporter() = default;

//------------------------------------------------------------------------------
void vtkScivisExporter::SetView(vtkScivisView* view)
{
  this->View = view;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkImageData> vtkScivisExporter::CaptureImage()
{
  if (!this->View)
  {
    vtkErrorMacro("There is no view to capture.");
    return nullptr;
  }

  // What comes out should be what is on screen.
  this->View->Render();

  vtkRenderer* renderer = this->View->GetRenderer();
  const double background = renderer->GetBackgroundAlpha();
  const bool gradient = renderer->GetGradientBackground();
  if (this->TransparentBackground)
  {
    renderer->SetBackgroundAlpha(0.0);
    renderer->SetGradientBackground(false);
  }

  vtkNew<vtkWindowToImageFilter> toImage;
  toImage->SetInput(this->View->GetRenderWindow());
  toImage->SetScale(this->Magnification);
  toImage->SetFixBoundary(true);
  toImage->ReadFrontBufferOff();
  if (this->TransparentBackground)
  {
    toImage->SetInputBufferTypeToRGBA();
  }
  toImage->Update();
  vtkSmartPointer<vtkImageData> image = toImage->GetOutput();

  // Put back what was there, so a capture leaves no trace on the view.
  if (this->TransparentBackground)
  {
    renderer->SetBackgroundAlpha(background);
    renderer->SetGradientBackground(gradient);
  }

  return image;
}

//------------------------------------------------------------------------------
bool vtkScivisExporter::SaveScreenshot(const char* filename)
{
  const std::string extension = ::Extension(filename);

  vtkSmartPointer<vtkImageWriter> writer;
  if (extension == ".png")
  {
    writer = vtkSmartPointer<vtkPNGWriter>::New();
  }
  else if (extension == ".jpg" || extension == ".jpeg")
  {
    writer = vtkSmartPointer<vtkJPEGWriter>::New();
  }
  else if (extension == ".tif" || extension == ".tiff")
  {
    writer = vtkSmartPointer<vtkTIFFWriter>::New();
  }
  else if (extension == ".bmp")
  {
    writer = vtkSmartPointer<vtkBMPWriter>::New();
  }
  else
  {
    vtkErrorMacro("Cannot write \"" << (filename ? filename : "")
                                    << "\": an image is written as .png, .jpg, .tif or .bmp.");
    return false;
  }

  // Only capture once the name is known to be one we can write, so that a
  // misspelled extension does not cost a render.
  vtkSmartPointer<vtkImageData> image = this->CaptureImage();
  if (!image)
  {
    return false;
  }

  writer->SetFileName(filename);
  writer->SetInputData(image);
  writer->Write();
  return true;
}

//------------------------------------------------------------------------------
bool vtkScivisExporter::ExportScene(const char* filename)
{
  if (!this->View)
  {
    vtkErrorMacro("There is no view to export.");
    return false;
  }

  const std::string extension = ::Extension(filename);
  if (extension != ".gltf" && extension != ".obj" && extension != ".vrml" && extension != ".wrl" &&
    extension != ".x3d")
  {
    vtkErrorMacro("Cannot write \""
      << (filename ? filename : "")
      << "\": a scene is written as .gltf, .obj, .vrml, .wrl or .x3d.");
    return false;
  }

  this->View->Render();
  vtkRenderWindow* window = this->View->GetRenderWindow();
  vtkRenderer* renderer = this->View->GetRenderer();

  if (extension == ".gltf")
  {
    vtkNew<vtkGLTFExporter> exporter;
    exporter->SetRenderWindow(window);
    exporter->SetActiveRenderer(renderer);
    exporter->SetFileName(filename);
    exporter->Write();
  }
  else if (extension == ".obj")
  {
    vtkNew<vtkOBJExporter> exporter;
    exporter->SetRenderWindow(window);
    exporter->SetActiveRenderer(renderer);
    // This exporter names its own files, appending .obj and .mtl to a prefix,
    // so the extension asked for is taken off again.
    std::string prefix = vtksys::SystemTools::GetFilenameWithoutLastExtension(filename);
    const std::string directory = vtksys::SystemTools::GetFilenamePath(filename);
    if (!directory.empty())
    {
      prefix = directory + "/" + prefix;
    }
    exporter->SetFilePrefix(prefix.c_str());
    exporter->Write();
  }
  else if (extension == ".vrml" || extension == ".wrl")
  {
    vtkNew<vtkVRMLExporter> exporter;
    exporter->SetRenderWindow(window);
    exporter->SetActiveRenderer(renderer);
    exporter->SetFileName(filename);
    exporter->Write();
  }
  else
  {
    vtkNew<vtkX3DExporter> exporter;
    exporter->SetRenderWindow(window);
    exporter->SetActiveRenderer(renderer);
    exporter->SetFileName(filename);
    exporter->Write();
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkScivisExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Magnification: " << this->Magnification << "\n";
  os << indent << "TransparentBackground: " << this->TransparentBackground << "\n";
  os << indent << "View: " << (this->View ? "set" : "(none)") << "\n";
}
VTK_ABI_NAMESPACE_END
