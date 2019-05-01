/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * vtkADIOS2ReaderDriver.cxx
 *
 *  Created on: May 1, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */
#include "vtkADIOS2Reader.h"

#include <iostream>

#include "ADIOS2Helper.h"
#include "ADIOS2Schema.h"
#include "xml_vtk/ADIOS2xmlVTI.h"

#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vtk_pugixml.h>
#include <vtksys/SystemTools.hxx>

#include <adios2.h>

// Impl
class vtkADIOS2Reader::Impl
{
public:
  size_t m_Step = 0;

  Impl();
  ~Impl() = default;

  void Update(const std::string& streamName, const size_t step = 0,
    const std::string& schemaName = "vtk.xml");
  void Fill(vtkMultiBlockDataSet* multiblock);

private:
  std::string m_StreamName;
  adios2::ADIOS m_ADIOS;
  adios2::IO m_IO;
  adios2::Engine m_Engine;

  std::string m_SchemaName;
  std::unique_ptr<adios2vtk::ADIOS2Schema> m_Reader;

  static const std::set<std::string> m_SupportedTypes;

  void InitReader();

  // we can extend this to add more schemas
  bool InitReaderVTKXML();
};

vtkStandardNewMacro(vtkADIOS2Reader);

vtkADIOS2Reader::vtkADIOS2Reader()
  : FileName(nullptr)
  , m_Impl(new Impl)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

int vtkADIOS2Reader::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* output)
{
  m_Impl->Update(FileName); // check if FileName changed

  return 1;
}

int vtkADIOS2Reader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());
  vtkMultiBlockDataSet* multiBlock = vtkMultiBlockDataSet::SafeDownCast(output);

  if (info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    std::cout << "Step was updated\n";
  }

  // Update step
  double newStep;
  info->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &newStep, 1);

  m_Impl->m_Step = static_cast<size_t>(newStep);
  std::cout << "Requested Step: " << newStep << " " << m_Impl->m_Step << "\n";

  m_Impl->Fill(multiBlock); // check if step changed

  return 1;
}

void vtkADIOS2Reader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "File Name: " << (this->FileName ? this->FileName : "(none)") << "\n";
}

// Impl Starts
vtkADIOS2Reader::Impl::Impl()
  : m_ADIOS(adios2vtk::helper::MPIGetComm())
{
}

void vtkADIOS2Reader::Impl::Update(
  const std::string& streamName, const size_t step, const std::string& schemaName)
{
  if (!m_IO && !m_Engine)
  {
    m_StreamName = streamName;
    if (m_ADIOS)
    {
      std::cout << "Opening " << m_StreamName << "\n";
      m_IO = m_ADIOS.DeclareIO(m_StreamName);
      std::cout << "DeclaredIO\n";

      m_Engine = m_IO.Open(m_StreamName, adios2::Mode::Read);
      std::cout << "Opened Engine\n";
      m_SchemaName = schemaName;
    }
    InitReader();
  }
  else
  {
    // TODO: check if variables changed
  }
}

void vtkADIOS2Reader::Impl::Fill(vtkMultiBlockDataSet* multiBlock)
{
  m_Reader->Fill(multiBlock, m_Step);
}

const std::set<std::string> vtkADIOS2Reader::Impl::m_SupportedTypes = { "ImageData",
  "StructuredGrid", "UnstructuredGrid" };

void vtkADIOS2Reader::Impl::InitReader()
{
  if (InitReaderVTKXML())
  {
    return;
  }
  // else if( InitReaderOther() ) {}
  // here we can make it extensible by trying to find other schema types for
  // now we stick with VTK XML schemas
}

bool vtkADIOS2Reader::Impl::InitReaderVTKXML()
{
  pugi::xml_document xmlDocument;
  std::string xmlContents;

  bool isSchemaFile = false;

  // auto lf_GetXMLDoc = [&]() {
  // check if it's file, not optimizing with MPI_Bcast
  std::string xmlFileName;
  if (vtksys::SystemTools::FileIsDirectory(m_Engine.Name()))
  {
    xmlFileName = m_Engine.Name() + "/" + m_SchemaName;
  }
  else if (vtksys::SystemTools::FileIsDirectory(m_Engine.Name() + ".dir"))
  {
    xmlFileName = m_Engine.Name() + ".dir/" + m_SchemaName;
  }

  if (!xmlFileName.empty())
  {
    if (vtksys::SystemTools::FileExists(xmlFileName))
    {
      xmlContents = adios2vtk::helper::FileToString(xmlFileName);
      xmlDocument =
        adios2vtk::helper::XMLDocument(xmlContents, true, "when reading " + m_SchemaName + " file");
      isSchemaFile = true;
    }
  }

  if (!isSchemaFile)
  {
    const adios2::Attribute<std::string> vtkXMLAttribute =
      m_IO.InquireAttribute<std::string>(m_SchemaName);
    const std::vector<std::string> vtkAttributes = vtkXMLAttribute.Data();
    if (vtkAttributes.empty())
    {
      throw std::runtime_error("ERROR: neither " + m_SchemaName +
        " file or bp attribute was found in " + m_Engine.Name() + "\n");
    }

    xmlContents = vtkAttributes.front();
    xmlDocument = adios2vtk::helper::XMLDocument(
      xmlContents, true, "when reading " + m_SchemaName + " attribute");
  }

  // auto lf_InitReader = [&]() {
  constexpr bool isDebug = true;
  constexpr bool isMandatory = true;
  constexpr bool isUnique = true;

  const pugi::xml_node vtkXMLFileNode = adios2vtk::helper::XMLNode("VTKFile", xmlDocument, isDebug,
    "when reading VTKFile node in " + m_Engine.Name(), isMandatory, isUnique);

  const pugi::xml_attribute typeXML = adios2vtk::helper::XMLAttribute("type", vtkXMLFileNode, true,
    "when reading type xml attribute in vtk.xml " + m_Engine.Name(), isMandatory);

  const std::string type = std::string(typeXML.value());

  if (m_SupportedTypes.count(type) == 0)
  {
    throw std::runtime_error(
      "ERROR: ADIOS2Reader only supports types= " + adios2vtk::helper::SetToCSV(m_SupportedTypes) +
      " when reading type xml attribute in " + m_SchemaName + " from " + m_Engine.Name() + "\n");
  }

  if (type == "ImageData")
  {
    m_Reader.reset(new adios2vtk::xml::ADIOS2xmlVTI(xmlContents, &m_IO, &m_Engine));
  }

  const bool success = m_Reader ? true : false;
  return success;
}
