/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVtkJSViewNodeFactory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkJSONRenderWindowExporter.h"

#include <vtkDataObject.h>
#include <vtkDataSet.h>
#include <vtkObjectFactory.h>
#include <vtkRenderWindow.h>
#include <vtkViewNode.h>

#include "vtkArchiver.h"
#include "vtkJSONDataSetWriter.h"
#include "vtkVtkJSSceneGraphSerializer.h"
#include "vtkVtkJSViewNodeFactory.h"

#include <memory>
#include <sstream>

namespace
{
// When exporting a VTK render window, we must also write the datasets
// associated with the render window into the same archive. To do this, we
// construct an intermediate archiver that neither opens nore closes the
// archive and simply pipes its contents into a subdirectory of a parent
// archive.
class vtkJSONDataSetArchiver : public vtkArchiver
{
public:
  static vtkJSONDataSetArchiver* New();
  vtkTypeMacro(vtkJSONDataSetArchiver, vtkArchiver);

  virtual void SetRenderWindowArchiver(vtkArchiver*);
  vtkGetObjectMacro(RenderWindowArchiver, vtkArchiver);

  virtual void OpenArchive() override {}
  virtual void CloseArchive() override {}
  virtual void InsertIntoArchive(
    const std::string& relativePath, const char* data, std::size_t size) override
  {
    this->RenderWindowArchiver->InsertIntoArchive(this->SubArchiveName(relativePath), data, size);
  }

  virtual bool Contains(const std::string& relativePath) override
  {
    return this->RenderWindowArchiver->Contains(this->SubArchiveName(relativePath));
  }

private:
  vtkJSONDataSetArchiver() { this->RenderWindowArchiver = vtkArchiver::New(); }
  virtual ~vtkJSONDataSetArchiver() override { this->SetRenderWindowArchiver(nullptr); }

  std::string SubArchiveName(const std::string& relativePath)
  {
    return std::string(this->GetArchiveName()) + "/" + relativePath;
  }

  vtkArchiver* RenderWindowArchiver;
};
vtkStandardNewMacro(vtkJSONDataSetArchiver);
vtkCxxSetObjectMacro(vtkJSONDataSetArchiver, RenderWindowArchiver, vtkArchiver);
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkJSONRenderWindowExporter);
vtkCxxSetObjectMacro(vtkJSONRenderWindowExporter, Archiver, vtkArchiver);

//----------------------------------------------------------------------------
vtkJSONRenderWindowExporter::vtkJSONRenderWindowExporter()
{
  this->Serializer = vtkVtkJSSceneGraphSerializer::New();
  this->Archiver = vtkArchiver::New();
  this->Factory = vtkVtkJSViewNodeFactory::New();
  this->Factory->SetSerializer(this->Serializer);
  this->CompactOutput = true;
}

//----------------------------------------------------------------------------
vtkJSONRenderWindowExporter::~vtkJSONRenderWindowExporter()
{
  this->SetSerializer(nullptr);
  this->SetArchiver(nullptr);
  this->Factory->Delete();
}

//----------------------------------------------------------------------------
void vtkJSONRenderWindowExporter::SetSerializer(vtkVtkJSSceneGraphSerializer* args)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting Serializer to " << args);
  if (this->Serializer != args)
  {
    vtkVtkJSSceneGraphSerializer* tempSGMacroVar = this->Serializer;
    this->Serializer = args;
    if (this->Serializer != nullptr)
    {
      this->Serializer->Register(this);
    }
    if (tempSGMacroVar != nullptr)
    {
      tempSGMacroVar->UnRegister(this);
    }
    this->Factory->SetSerializer(this->Serializer);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkJSONRenderWindowExporter::WriteData()
{
  if (this->GetSerializer() == nullptr)
  {
    vtkErrorMacro(<< "No scene!");
    return;
  }
  this->GetSerializer()->Reset();

  if (this->GetArchiver() == nullptr)
  {
    vtkErrorMacro(<< "No archiver!");
    return;
  }

  if (this->GetArchiver()->GetArchiveName() == nullptr)
  {
    vtkErrorMacro(<< "Please specify Archive Name to use");
    return;
  }

  // Populate the scene instance
  {
    // Construct a top-level node for the render window
    vtkViewNode* vn = this->Factory->CreateNode(this->RenderWindow);

    // Build the scene graph
    vn->Traverse(vtkViewNode::build);

    // Construct the vtk-js representation of the scene graph
    vn->Traverse(vtkViewNode::synchronize);

    // Update the datasets associated with the scene graph
    vn->Traverse(vtkViewNode::render);

    // Delete the top level node
    vn->Delete();
  }

  // Open the archive for writing
  this->GetArchiver()->OpenArchive();

  // Write the top-level index file describing the scene elements and their
  // topology.
  {
    std::stringstream stream;
    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    builder["indentation"] = this->CompactOutput ? "" : "  ";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    writer->write(this->GetSerializer()->GetRoot(), &stream);

    std::string index = stream.str();
    this->GetArchiver()->InsertIntoArchive("index.json", index.c_str(), index.size());
  }

  // Write the associated data arrays into the archive
  {
    vtkNew<vtkJSONDataSetWriter> dsWriter;
    vtkNew<vtkJSONDataSetArchiver> dsArchiver;
    dsArchiver->SetRenderWindowArchiver(this->GetArchiver());
    dsWriter->SetArchiver(dsArchiver);
    dsWriter->GetArchiver()->SetArchiveName("data");

    for (vtkIdType i = 0; i < this->GetSerializer()->GetNumberOfDataArrays(); ++i)
    {
      std::string daArchiveName = this->GetSerializer()->GetDataArrayId(i);

      // Only write the array if its id (which is its hash) has not yet been
      // added to the archive.
      if (!dsArchiver->Contains(daArchiveName))
      {
        dsWriter->WriteArrayContents(this->GetSerializer()->GetDataArray(i), daArchiveName.c_str());
      }
    }
  }

  // Close the archive
  this->GetArchiver()->CloseArchive();
}

//----------------------------------------------------------------------------
void vtkJSONRenderWindowExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
