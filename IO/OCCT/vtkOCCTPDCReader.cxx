// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOCCTPDCReader.h"

#include <Standard_Version.hxx>
#define VTK_OCCT_VERSION(major, minor, maint) ((major) << 16 | (minor) << 8 | (maint))

#if VTK_OCCT_VERSION(7, 4, 1) <= OCC_VERSION_HEX
#define VTK_OCCT_USE_PROGRESS 1
#else
#define VTK_OCCT_USE_PROGRESS 0
#endif

#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCommand.h>
#include <vtkCompositeDataSet.h>
#include <vtkDataAssembly.h>
#include <vtkFileResourceStream.h>
#include <vtkFloatArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMatrix4x4.h>
#include <vtkObjectFactory.h>
#include <vtkPartitionedDataSet.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>

#include <vtksys/SystemTools.hxx>

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <BinTools.hxx>
#include <BinXCAFDrivers.hxx>
#include <IGESCAFControl_Reader.hxx>
#include <Interface_Static.hxx>
#include <Message.hxx>
#include <Message_PrinterOStream.hxx>
#if VTK_OCCT_USE_PROGRESS
#include <Message_ProgressIndicator.hxx>
#endif
#include <NCollection_IndexedDataMap.hxx>
#include <NCollection_Sequence.hxx>
#include <PCDM_ReaderStatus.hxx>
#include <Poly.hxx>
#include <Quantity_Color.hxx>
#if VTK_OCCT_VERSION(7, 5, 0) <= OCC_VERSION_HEX
#include <Quantity_ColorRGBA.hxx>
#endif
#include <STEPCAFControl_Reader.hxx>
#include <TCollection_AsciiString.hxx>
#include <TDF_Label.hxx>
#include <TDataStd_Name.hxx>
#include <TDocStd_Application.hxx>
#include <TDocStd_Document.hxx>
#include <TNaming_NamedShape.hxx>
#include <TopAbs_Orientation.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopTools_ShapeMapHasher.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFPrs.hxx>
#include <XCAFPrs_Style.hxx>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <map>
#include <sstream>
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

namespace
{

// Define some helper methods and classes

// Simple struct for representing the number of
// times (count) a node (nodeId)'s name was encountered
// while traversing an assembly
struct NodeInfo
{
  NodeInfo() = default;
  NodeInfo(int nodeId, unsigned int count)
    : NodeId(nodeId)
    , Count(count)
  {
  }
  int NodeId = 0;
  unsigned int Count = 0;
};

#if VTK_OCCT_USE_PROGRESS
//----------------------------------------------------------------------------
// This class is for reporting OpenCascade progress while reading information
class ProgressIndicator : public Message_ProgressIndicator
{
public:
  ProgressIndicator(vtkOCCTPDCReader* reader) { this->Reader = reader; }

protected:
  void Show(const Message_ProgressScope&, const Standard_Boolean) override
  {
    double currentPosition = this->GetPosition();
    if (currentPosition - this->LastPosition > 0.01)
    {
      double localProgress = 0.5 * currentPosition;
      this->Reader->UpdateProgress(localProgress);
      this->LastPosition = currentPosition;
    }
  }

private:
  double LastPosition = 0.0;
  vtkOCCTPDCReader* Reader = nullptr;
};
#endif

//----------------------------------------------------------------------------
// Template for reading either STEP or IGES files
template <typename T>
bool TransferToDocument(vtkOCCTPDCReader* that, T& reader, Handle(TDocStd_Document) doc)
{
  reader.SetColorMode(true);
  reader.SetNameMode(true);
  reader.SetLayerMode(true);

  if (that->GetStream())
  {
    auto streambuf = that->GetStream()->ToStreambuf();
    std::istream input(streambuf.get());

    const char* name = that->GetFileName() ? that->GetFileName() : "vtkResourceStream";
    if (reader.ReadStream(name, input) != IFSelect_RetDone)
    {
      vtkErrorWithObjectMacro(that, "Failed processing stream");
      return false;
    }
  }
  else if (that->GetFileName())
  {
    if (reader.ReadFile(that->GetFileName()) != IFSelect_RetDone)
    {
      vtkErrorWithObjectMacro(that, "Failed opening file " << that->GetFileName());
      return false;
    }
  }
  else
  {
    vtkErrorWithObjectMacro(that, "No FileName or Stream specified.");
    return false;
  }

#if VTK_OCCT_USE_PROGRESS
  ProgressIndicator pi(that);
  return reader.Transfer(doc, pi.Start());
#else
  return reader.Transfer(doc);
#endif
}

//----------------------------------------------------------------------------
// Determine if the header is XBF format.
// This is done by seeing is the first 7 bytes represent "BINFILE" followed by
// an 8 byte magic number 0x0000000704030201
bool IsXBFHeader(const std::string& header)
{
  static constexpr unsigned char magic[] = { 0x01, 0x02, 0x03, 0x04, 0x07, 0x00, 0x00, 0x00 };

  if (header.size() < 15)
  {
    return false;
  }

  if (header.compare(0, 7, "BINFILE") != 0)
  {
    return false;
  }

  return std::memcmp(header.data() + 7, magic, sizeof(magic)) == 0;
}

//----------------------------------------------------------------------------
// Determine whether a header appears to be OCCT BREP text.
bool IsBRepHeader(const std::string& header)
{
  return header.rfind("DBRep_DrawableShape", 0) == 0 ||
    header.find("CASCADE Topology") != std::string::npos;
}

//----------------------------------------------------------------------------
// Determine whether a header appears to be an OCCT binary BREP.
bool IsBinaryBRepHeader(const std::string& header)
{
  const auto firstNonWhitespace =
    std::find_if(header.begin(), header.end(), [](unsigned char ch) { return !std::isspace(ch); });
  return std::string(firstNonWhitespace, header.end()).rfind("Open CASCADE Topology V", 0) == 0;
}

//----------------------------------------------------------------------------
// Determine whether the input is an OCCT binary BREP. Preserve the stream
// position so that probing does not affect the subsequent read.
bool IsBinaryBRep(vtkResourceStream* stream)
{
  if (!stream)
  {
    return false;
  }

  stream->Seek(0, vtkResourceStream::SeekDirection::Begin);
  auto streambuf = stream->ToStreambuf();
  if (!streambuf)
  {
    return false;
  }

  std::istream input(streambuf.get());
  std::string header(32, '\0');
  input.read(header.data(), static_cast<std::streamsize>(header.size()));
  header.resize(static_cast<std::size_t>(input.gcount()));

  stream->Seek(0, vtkResourceStream::SeekDirection::Begin);
  return IsBinaryBRepHeader(header);
}

//----------------------------------------------------------------------------
// Determine the data format from input stream
vtkOCCTPDCReader::Format GetFormatFromStream(vtkResourceStream* stream)
{
  stream->Seek(0, vtkResourceStream::SeekDirection::Begin); // Make sure we are at the beginning
  auto streambuf = stream->ToStreambuf();
  if (!streambuf)
  {
    return vtkOCCTPDCReader::Format::UNSUPPORTED;
  }

  std::istream input(streambuf.get());

  std::string header;
  char c;
  constexpr std::size_t MaxProbeSize = 4096;

  // Read in the header information
  while (header.size() < MaxProbeSize && input.get(c))
  {
    header.push_back(c);
  }

  stream->Seek(0, vtkResourceStream::SeekDirection::Begin); // Reset to the beginning

  auto trimLeading = [](std::string s)
  {
    s.erase(s.begin(),
      std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    return s;
  };

  std::string h = trimLeading(header);

  if (h.rfind("ISO-10303-21;", 0) == 0)
  {
    return vtkOCCTPDCReader::Format::STEP;
  }

  // XBF / binary OCAF document
  if (IsXBFHeader(header))
  {
    return vtkOCCTPDCReader::Format::XBF;
  }

  // ASCII or binary OCCT BREP. Both encodings are exposed as BREP; the
  // appropriate OCCT reader is selected when loading the shape.
  if (IsBRepHeader(header))
  {
    return vtkOCCTPDCReader::Format::BREP;
  }

  // Common IGES heuristic: section letters in fixed-column records.
  std::istringstream lines(header);
  std::string line;
  while (std::getline(lines, line))
  {
    if (line.size() >= 73)
    {
      char section = line[72];
      if (section == 'S' || section == 'G' || section == 'D' || section == 'P' || section == 'T')
      {
        return vtkOCCTPDCReader::Format::IGES;
      }
    }
  }
  return vtkOCCTPDCReader::Format::UNSUPPORTED;
}

} // End Anonymous Namespace

// Define the data representation used for hashing OpenCascade Labels
#if VTK_OCCT_VERSION(7, 8, 0) <= OCC_VERSION_HEX
using OCC_IdType = size_t;
#else
using OCC_IdType = int;
#endif

#if VTK_OCCT_VERSION(7, 5, 0) <= OCC_VERSION_HEX
using OCC_ColorType = Quantity_ColorRGBA;
#else
using OCC_ColorType = Quantity_Color;
#endif

using OCC_LabelSequence = NCollection_Sequence<TDF_Label>;

//----------------------------------------------------------------------------
// Internal class for the reader that handles all OpenCascade functionality
class vtkOCCTPDCReader::vtkInternals
{
public:
  using StyleMap = NCollection_IndexedDataMap<TopoDS_Shape, XCAFPrs_Style, TopTools_ShapeMapHasher>;

  vtkInternals(vtkOCCTPDCReader* parent);

  // Get the hash representation of a label
  OCC_IdType GetHash(const TDF_Label& label);

  // Read in BREP Data
  bool AddBRepToDocument(Handle(TDocStd_Document) & doc);

  // Read in XBF Data
  bool ReadXBFToDocument(Handle(TDocStd_Document) & doc);

  // Return the color of a shape if it or its ancestors has color
  // else return false;
  bool GetShapeColor(const TopoDS_Shape& shape, XCAFDoc_ColorType type, OCC_ColorType& color);

  // Return the color of a label if it or its ancestors has color
  // else return false;
  bool GetLabelColor(const TDF_Label& label, XCAFDoc_ColorType type, OCC_ColorType& color);

  // Return a subshape color, falling back to the containing shape.
  bool GetSubShapeColor(const TopoDS_Shape& subShape, const TopoDS_Shape& shape,
    XCAFDoc_ColorType type, OCC_ColorType& color);

  // Convert an OCCT color to an RGBA tuple.
  static std::array<unsigned char, 4> GetRGBA(const Quantity_Color& color);
#if VTK_OCCT_VERSION(7, 5, 0) <= OCC_VERSION_HEX
  static std::array<unsigned char, 4> GetRGBA(const Quantity_ColorRGBA& color);
#endif

  // Collect definition-level XCAF styles and propagate them to face and edge leaves.
  StyleMap CollectInheritedStyles(const TDF_Label& label, const TopoDS_Shape& shape);
  static void PassDownStyle(const XCAFPrs_Style& parent, XCAFPrs_Style& child);

  // Create a polydata representation for the faces of a shape
  vtkSmartPointer<vtkPolyData> SurfacesToPolyData(
    const TopoDS_Shape& shape, const StyleMap& styles);

  // Create a polydata representation for the wires/curves of a shape
  vtkSmartPointer<vtkPolyData> WiresToPolyData(const TopoDS_Shape& shape, const StyleMap& styles);

  // Populate a vtkPartitionedDataSetCollection with a label's contents
  void AddLabelToOutput(const TDF_Label& label, const TopLoc_Location& parentLocation,
    vtkPartitionedDataSetCollection* output, vtkDataAssembly* assembly, int parentNode,
    unsigned int& datasetIndex, std::unordered_map<std::string, NodeInfo>& nodeNameCount);

  // Reset clears all maps in order to start fresh
  void reset()
  {
    this->ShapeMap.clear();
    this->WireMap.clear();
    this->NodeMap.clear();
  }

  // Return the name of the label if one exists, else return an empty string
  static std::string GetLabelName(const TDF_Label& label);

  // Convert a OCC Location matrix into a VTK Matrix
  static void GetMatrix(const TopLoc_Location& loc, vtkMatrix4x4* mat);

  // Create the appropriate vtkPartionedDataSets and update both the vtkDataAssebly and
  // vtkPartitionedDataSetCollection
  static void AddToDataSetCollection(const std::string& name, vtkPolyData* surfaces,
    vtkPolyData* curves, vtkPartitionedDataSetCollection* output, vtkDataAssembly* assembly,
    int node, unsigned int& datasetIndex);

  // Define some constants to be used as defaults when not provided in OpenCascade
  static constexpr float DefaultNormal[3] = { 0.0, 0.0, 1.0 };
  static constexpr float DefaultUV[3] = { 0.0, 0.0 };

  // Maps used to store vtkPolyData representing Geometry from OpenCascade labels
  std::unordered_map<OCC_IdType, vtkSmartPointer<vtkPolyData>> ShapeMap;
  std::unordered_map<OCC_IdType, vtkSmartPointer<vtkPolyData>> WireMap;
  // Map for representing nodes whose names needed to be changed
  // and the node that has the original name.
  std::unordered_map<int, int> NodeMap;

  Handle(XCAFDoc_ShapeTool) ShapeTool;
  Handle(XCAFDoc_ColorTool) ColorTool;

  vtkOCCTPDCReader* Parent;
};

//----------------------------------------------------------------------------
vtkOCCTPDCReader::vtkInternals::vtkInternals(vtkOCCTPDCReader* parent)
  : Parent(parent)
{
}

//----------------------------------------------------------------------------
// Get the hash representation of a label
#if VTK_OCCT_VERSION(7, 8, 0) <= OCC_VERSION_HEX
OCC_IdType vtkOCCTPDCReader::vtkInternals::GetHash(const TDF_Label& label)
{
  TopoDS_Shape aShape;
  return this->ShapeTool->GetShape(label, aShape) ? std::hash<TopoDS_Shape>{}(aShape) : 0;
}
#else
OCC_IdType vtkOCCTPDCReader::vtkInternals::GetHash(const TDF_Label& label)
{
  TopoDS_Shape aShape;
  return this->ShapeTool->GetShape(label, aShape) ? aShape.HashCode(INT_MAX) : 0;
}
#endif

//----------------------------------------------------------------------------
// Return the name of the label if one exists, else return an empty string
std::string vtkOCCTPDCReader::vtkInternals::GetLabelName(const TDF_Label& label)
{
  Handle(TDataStd_Name) nameAttr;
  if (label.FindAttribute(TDataStd_Name::GetID(), nameAttr))
  {
    return TCollection_AsciiString(nameAttr->Get()).ToCString();
  }
  return "";
}

//----------------------------------------------------------------------------
// Return a vtkMatrix4x4 representation of an OpenCascade transformation
void vtkOCCTPDCReader::vtkInternals::GetMatrix(const TopLoc_Location& loc, vtkMatrix4x4* mat)
{
  const gp_Trsf& transfo = loc.Transformation();
  gp_Mat vecto = transfo.VectorialPart();
  gp_XYZ trans = transfo.TranslationPart();

  mat->Identity();

  for (int i = 0; i < 3; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      mat->SetElement(i, j, vecto(i + 1, j + 1));
    }
  }
  mat->SetElement(0, 3, trans.X());
  mat->SetElement(1, 3, trans.Y());
  mat->SetElement(2, 3, trans.Z());
}

//----------------------------------------------------------------------------
// Add BREP Data to the document
bool vtkOCCTPDCReader::vtkInternals::AddBRepToDocument(Handle(TDocStd_Document) & doc)
{
  TopoDS_Shape shape;

  bool ok = false;

  if (this->Parent->GetStream())
  {
    vtkResourceStream* stream = this->Parent->GetStream();
    const bool isBinary = IsBinaryBRep(stream);
    auto streambuf = this->Parent->GetStream()->ToStreambuf();
    std::istream input(streambuf.get());

    if (isBinary)
    {
      BinTools::Read(shape, input);
    }
    else
    {
      BRep_Builder builder;
      BRepTools::Read(shape, input, builder);
    }
    ok = !shape.IsNull();
  }
  else
  {
    const char* fileName = this->Parent->GetFileName();
    if (!fileName || fileName[0] == '\0')
    {
      vtkErrorWithObjectMacro(this->Parent, "No FileName or Stream specified for BREP input.");
      return false;
    }

    vtkNew<vtkFileResourceStream> stream;
    if (!stream->Open(fileName))
    {
      vtkErrorWithObjectMacro(this->Parent, "Failed opening file " << fileName);
      return false;
    }

    if (IsBinaryBRep(stream))
    {
      ok = BinTools::Read(shape, fileName);
    }
    else
    {
      BRep_Builder builder;
      ok = BRepTools::Read(shape, fileName, builder);
    }
  }

  if (!ok || shape.IsNull())
  {
    vtkErrorWithObjectMacro(this->Parent, "Failed reading BREP input.");
    return false;
  }

  // Add the BREP shape to the document
  auto shapeTool = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
  TDF_Label label = shapeTool->AddShape(shape, Standard_False);
  TDataStd_Name::Set(label, "BREP");
  return true;
}

//----------------------------------------------------------------------------
// Read in XBF data
bool vtkOCCTPDCReader::vtkInternals::ReadXBFToDocument(Handle(TDocStd_Document) & doc)
{
  Handle(TDocStd_Application) app = XCAFApp_Application::GetApplication();

  // Register binary XCAF/OCAF persistence format support.
  BinXCAFDrivers::DefineFormat(app);

  if (this->Parent->GetStream())
  {
    vtkResourceStream* stream = this->Parent->GetStream();

    stream->Seek(0, vtkResourceStream::SeekDirection::Begin);

    auto streambuf = stream->ToStreambuf();
    if (!streambuf)
    {
      vtkErrorWithObjectMacro(this->Parent, "Could not create stream buffer for XBF input.");
      return false;
    }

    std::istream input(streambuf.get());

    PCDM_ReaderStatus status = app->Open(input, doc);

    stream->Seek(0, vtkResourceStream::SeekDirection::Begin);

    if (status != PCDM_RS_OK)
    {
      vtkErrorWithObjectMacro(this->Parent, "Failed to read XBF data from stream.");
      return false;
    }

    return true;
  }

  const char* fileName = this->Parent->GetFileName();
  if (!fileName || fileName[0] == '\0')
  {
    vtkErrorWithObjectMacro(this->Parent, "No FileName or Stream specified for XBF input.");
    return false;
  }

  PCDM_ReaderStatus status = app->Open(fileName, doc);
  if (status != PCDM_RS_OK)
  {
    vtkErrorWithObjectMacro(this->Parent, "Failed to read XBF file: " << fileName);
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
// Return the color of a shape if it or its ancestors has color
// else return false;
bool vtkOCCTPDCReader::vtkInternals::GetShapeColor(
  const TopoDS_Shape& shape, XCAFDoc_ColorType type, OCC_ColorType& color)
{
  // Get the label of the shape
  TDF_Label label;

  if (!this->ShapeTool->Search(shape, label))
  {
    return false;
  }

  // Now return the color information associated with the label
  return this->GetLabelColor(label, type, color);
}

//----------------------------------------------------------------------------
// Return the color of a label if it or its ancestors has color
// else return false;
bool vtkOCCTPDCReader::vtkInternals::GetLabelColor(
  const TDF_Label& label, XCAFDoc_ColorType type, OCC_ColorType& color)
{
  // If there is no label then there is no color
  if (label.IsNull())
  {
    return false;
  }

  if (this->ColorTool->GetColor(label, type, color))
  {
    return true;
  }

  if (this->ColorTool->GetColor(label, XCAFDoc_ColorGen, color))
  {
    return true;
  }

  // Get the label's parent for color
  return this->GetLabelColor(label.Father(), type, color);
}

//----------------------------------------------------------------------------
// Return a subshape color, falling back to the containing shape.
bool vtkOCCTPDCReader::vtkInternals::GetSubShapeColor(const TopoDS_Shape& subShape,
  const TopoDS_Shape& shape, XCAFDoc_ColorType type, OCC_ColorType& color)
{
  return this->GetShapeColor(subShape, type, color) || this->GetShapeColor(shape, type, color);
}

//----------------------------------------------------------------------------
// Convert an OCCT color to an RGBA tuple.
std::array<unsigned char, 4> vtkOCCTPDCReader::vtkInternals::GetRGBA(const Quantity_Color& color)
{
#if VTK_OCCT_VERSION(7, 5, 0) <= OCC_VERSION_HEX
  // OCCT stores color components in linear RGB. VTK color arrays are consumed
  // as display (sRGB) values, so convert them before quantizing to bytes.
  double rgb[3];
  color.Values(rgb[0], rgb[1], rgb[2], Quantity_TOC_sRGB);
#else
  // The explicit sRGB conversion API is not available in older OCCT releases.
  const double rgb[3] = { color.Red(), color.Green(), color.Blue() };
#endif
  return { static_cast<unsigned char>(255.0 * rgb[0]), static_cast<unsigned char>(255.0 * rgb[1]),
    static_cast<unsigned char>(255.0 * rgb[2]), 255 };
}

#if VTK_OCCT_VERSION(7, 5, 0) <= OCC_VERSION_HEX
//----------------------------------------------------------------------------
std::array<unsigned char, 4> vtkOCCTPDCReader::vtkInternals::GetRGBA(
  const Quantity_ColorRGBA& color)
{
  auto rgba = vtkOCCTPDCReader::vtkInternals::GetRGBA(color.GetRGB());
  rgba[3] = static_cast<unsigned char>(255.0 * color.Alpha());
  return rgba;
}
#endif

//----------------------------------------------------------------------------
void vtkOCCTPDCReader::vtkInternals::PassDownStyle(
  const XCAFPrs_Style& parent, XCAFPrs_Style& child)
{
  if (!child.IsSetColorCurv() && parent.IsSetColorCurv())
  {
    child.SetColorCurv(parent.GetColorCurv());
  }
  if (!child.IsSetColorSurf() && parent.IsSetColorSurf())
  {
#if VTK_OCCT_VERSION(7, 5, 0) <= OCC_VERSION_HEX
    child.SetColorSurf(parent.GetColorSurfRGBA());
#else
    child.SetColorSurf(parent.GetColorSurf());
#endif
  }
}

//----------------------------------------------------------------------------
vtkOCCTPDCReader::vtkInternals::StyleMap vtkOCCTPDCReader::vtkInternals::CollectInheritedStyles(
  const TDF_Label& label, const TopoDS_Shape& shape)
{
  StyleMap inheritedStyles;
  // Only collect styles from shape definitions. Assembly and instance styling
  // is deliberately outside the scope of this reader.
  if (label.IsNull() || this->ShapeTool->IsAssembly(label))
  {
    return inheritedStyles;
  }

  StyleMap collectedStyles;
  XCAFPrs::CollectStyleSettings(label, TopLoc_Location(), collectedStyles);

  // Process the most-specific topology first. PassDownStyle only fills unset
  // properties, so a face or edge color takes precedence over its ancestors.
  const auto compareShapeDepth = [](const TopoDS_Shape& left, const TopoDS_Shape& right)
  { return left.ShapeType() > right.ShapeType(); };
  std::multimap<TopoDS_Shape, XCAFPrs_Style, decltype(compareShapeDepth)> sortedStyles(
    compareShapeDepth);
  for (StyleMap::Iterator styleIt(collectedStyles); styleIt.More(); styleIt.Next())
  {
    sortedStyles.emplace(styleIt.Key(), styleIt.Value());
  }

  const auto passDownToLeaves = [&](TopAbs_ShapeEnum leafType)
  {
    for (const auto& entry : sortedStyles)
    {
      const auto applyStyle = [&](const TopoDS_Shape& leaf)
      {
        if (inheritedStyles.Contains(leaf))
        {
          this->PassDownStyle(entry.second, inheritedStyles.ChangeFromKey(leaf));
        }
        else
        {
          inheritedStyles.Add(leaf, entry.second);
        }
      };

      // TopExp_Explorer visits descendants, but not a root that already has
      // the requested type. Preserve styles assigned directly to a face/edge.
      if (entry.first.ShapeType() == leafType)
      {
        applyStyle(entry.first);
        continue;
      }
      for (TopExp_Explorer leafIt(entry.first, leafType); leafIt.More(); leafIt.Next())
      {
        applyStyle(leafIt.Current());
      }
    }
  };
  passDownToLeaves(TopAbs_FACE);
  if (this->Parent->GetReadWire())
  {
    passDownToLeaves(TopAbs_EDGE);
  }

  // Complete partially specified leaf styles with the definition's default.
  if (collectedStyles.Contains(shape))
  {
    const XCAFPrs_Style& defaultStyle = collectedStyles.FindFromKey(shape);
    for (StyleMap::Iterator styleIt(inheritedStyles); styleIt.More(); styleIt.Next())
    {
      this->PassDownStyle(defaultStyle, inheritedStyles.ChangeFromKey(styleIt.Key()));
    }
  }

  return inheritedStyles;
}

//----------------------------------------------------------------------------
// Returns a Polydata representing the tessellation of all of the faces related
// to the shape - if any of the faces have normals, colors, or uv data then
// the resulting polydata will contain the appropriate point/cell data
vtkSmartPointer<vtkPolyData> vtkOCCTPDCReader::vtkInternals::SurfacesToPolyData(
  const TopoDS_Shape& shape, const StyleMap& styles)
{
  vtkNew<vtkPoints> points;
  vtkNew<vtkFloatArray> normals;
  normals->SetNumberOfComponents(3);
  normals->SetName("Normal");
  vtkNew<vtkFloatArray> uvs;
  uvs->SetNumberOfComponents(2);
  uvs->SetName("UV");
  vtkNew<vtkUnsignedCharArray> colors;
  colors->SetNumberOfComponents(4);
  colors->SetName("Colors");
  vtkNew<vtkCellArray> trianglesCells;

  bool hasNormals = false;
  bool hasUVs = false;
  bool hasColors = false;

  Standard_Integer shift = 0;

  // Add all faces to polydata
  for (TopExp_Explorer exFace(shape, TopAbs_FACE); exFace.More(); exFace.Next())
  {
    TopoDS_Face face = TopoDS::Face(exFace.Current());

    TopLoc_Location location;
    Handle(Poly_Triangulation) poly = BRep_Tool::Triangulation(face, location);

    // Create a tessellation if none exists for the face
    if (poly.IsNull() || poly->NbTriangles() <= 0)
    {
      // meshing
      BRepMesh_IncrementalMesh(face, this->Parent->GetLinearDeflection(),
        this->Parent->GetRelativeDeflection(), this->Parent->GetAngularDeflection(), Standard_True);
      // Refresh the handle after meshing
      poly = BRep_Tool::Triangulation(face, location);
      ;
    }

    // If there is no tessellation then skip it
    if (poly.IsNull())
    {
      continue;
    }

    std::array<unsigned char, 4> rgba = { 255, 255, 255, 255 };
    if (styles.Contains(face) && styles.FindFromKey(face).IsSetColorSurf())
    {
#if VTK_OCCT_VERSION(7, 5, 0) <= OCC_VERSION_HEX
      rgba = this->GetRGBA(styles.FindFromKey(face).GetColorSurfRGBA());
#else
      rgba = this->GetRGBA(styles.FindFromKey(face).GetColorSurf());
#endif
      hasColors = true;
    }
    else
    {
      OCC_ColorType faceColor;
      if (this->GetSubShapeColor(face, shape, XCAFDoc_ColorSurf, faceColor))
      {
        rgba = this->GetRGBA(faceColor);
        hasColors = true;
      }
    }

    Poly::ComputeNormals(poly);
    TopAbs_Orientation faceOrientation = face.Orientation();

    Standard_Integer nbT = poly->NbTriangles();
    Standard_Integer nbV = poly->NbNodes();

    // Insert the Points
    for (Standard_Integer i = 1; i <= nbV; i++)
    {
      gp_Pnt pt = poly->Node(i).Transformed(location);
      points->InsertNextPoint(pt.X(), pt.Y(), pt.Z());
    }

    // Insert Normals if they exit else add a default normal
    if (poly->HasNormals())
    {
      hasNormals = true;
      for (Standard_Integer i = 1; i <= nbV; i++)
      {
        gp_Dir n = poly->Normal(i);
        float fn[3] = { static_cast<float>(n.X()), static_cast<float>(n.Y()),
          static_cast<float>(n.Z()) };
        if (faceOrientation == TopAbs_Orientation::TopAbs_REVERSED)
        {
          vtkMath::MultiplyScalar(fn, -1.f);
        }
        normals->InsertNextTypedTuple(fn);
      }
    }
    else
    {
      for (Standard_Integer i = 1; i <= nbV; i++)
      {
        normals->InsertNextTypedTuple(DefaultNormal);
      }
    }

    // Add UV Information if it exits, else add a default uv
    if (poly->HasUVNodes())
    {
      hasUVs = true;
      for (Standard_Integer i = 1; i <= nbV; i++)
      {
        gp_Pnt2d uv = poly->UVNode(i);
        float fn[2] = { static_cast<float>(uv.X()), static_cast<float>(uv.Y()) };
        uvs->InsertNextTypedTuple(fn);
      }
    }
    else
    {
      for (Standard_Integer i = 1; i <= nbV; i++)
      {
        uvs->InsertNextTypedTuple(DefaultUV);
      }
    }

    for (int i = 1; i <= nbT; i++)
    {
      int n1, n2, n3;
      poly->Triangle(i).Get(n1, n2, n3);

      vtkIdType cell[3] = { shift + n1 - 1, shift + n2 - 1, shift + n3 - 1 };
      if (faceOrientation != TopAbs_Orientation::TopAbs_FORWARD)
      {
        std::swap(cell[0], cell[2]);
      }
      trianglesCells->InsertNextCell(3, cell);
      colors->InsertNextTypedTuple(rgba.data());
    }
    shift += nbV;
  }

  // Did we find any polygons?
  if (!points->GetNumberOfPoints())
  {
    return nullptr;
  }
  vtkNew<vtkPolyData> polydata;
  polydata->SetPoints(points);
  polydata->SetPolys(trianglesCells);

  if (hasNormals)
  {
    polydata->GetPointData()->SetNormals(normals);
  }
  if (hasUVs)
  {
    polydata->GetPointData()->SetTCoords(uvs);
  }
  if (hasColors)
  {
    polydata->GetCellData()->SetScalars(colors);
  }
  polydata->Squeeze();

  return polydata;
}

//----------------------------------------------------------------------------
// Returns a Polydata representing the tessellation of all of the wires related
// to the shape - if any of the wires have colors then
// the resulting polydata will contain the appropriate cell data
vtkSmartPointer<vtkPolyData> vtkOCCTPDCReader::vtkInternals::WiresToPolyData(
  const TopoDS_Shape& shape, const StyleMap& styles)
{
  vtkNew<vtkPoints> points;
  vtkNew<vtkUnsignedCharArray> colors;
  colors->SetNumberOfComponents(4);
  colors->SetName("Colors");
  vtkNew<vtkCellArray> lineCells;
  bool hasColors = false;

  Standard_Integer shift = 0;

  // Meshing each edge independently can leave Polygon3D unavailable. As in
  // vtkF3DOCCTReader, mesh one compound containing every edge instead.
  std::vector<TopoDS_Edge> edges;
  TopoDS_Builder builder;
  TopoDS_Compound compound;
  builder.MakeCompound(compound);
  for (TopExp_Explorer exEdge(shape, TopAbs_EDGE); exEdge.More(); exEdge.Next())
  {
    TopoDS_Edge edge = TopoDS::Edge(exEdge.Current());
    builder.Add(compound, edge);
    edges.push_back(edge);
  }
  BRepMesh_IncrementalMesh(compound, this->Parent->GetLinearDeflection(),
    this->Parent->GetRelativeDeflection(), this->Parent->GetAngularDeflection(), Standard_True);

  // Add all wires to polydata
  for (const TopoDS_Edge& edge : edges)
  {
    TopLoc_Location location;
    Handle(Poly_Polygon3D) poly = BRep_Tool::Polygon3D(edge, location);

    // skip all wires without tessellation
    if (poly.IsNull() || poly->Nodes().Length() == 0)
    {
      continue;
    }

    std::array<unsigned char, 4> rgba = { 255, 255, 255, 255 };
    if (styles.Contains(edge) && styles.FindFromKey(edge).IsSetColorCurv())
    {
      rgba = this->GetRGBA(styles.FindFromKey(edge).GetColorCurv());
#if VTK_OCCT_VERSION(7, 5, 0) <= OCC_VERSION_HEX
      // XCAFPrs_Style stores curve color as RGB. Recover alpha from an exact
      // edge color assignment when the OCCT RGBA color API is available.
      OCC_ColorType edgeColor;
      if (this->GetShapeColor(edge, XCAFDoc_ColorCurv, edgeColor))
      {
        rgba[3] = this->GetRGBA(edgeColor)[3];
      }
#endif
      hasColors = true;
    }
    else
    {
      OCC_ColorType edgeColor;
      if (this->GetSubShapeColor(edge, shape, XCAFDoc_ColorCurv, edgeColor))
      {
        rgba = this->GetRGBA(edgeColor);
        hasColors = true;
      }
    }

    Standard_Integer nbV = poly->NbNodes();

    // Points
    const auto& aNodes = poly->Nodes();
    for (Standard_Integer i = 1; i <= nbV; i++)
    {
      gp_Pnt pt = aNodes(i).Transformed(location);
      points->InsertNextPoint(pt.X(), pt.Y(), pt.Z());
    }

    std::vector<vtkIdType> polyline(nbV);
    std::iota(polyline.begin(), polyline.end(), shift);
    lineCells->InsertNextCell(polyline.size(), polyline.data());
    colors->InsertNextTypedTuple(rgba.data());

    shift += nbV;
  }
  // Did we find any lines?
  if (!points->GetNumberOfPoints())
  {
    return nullptr;
  }
  vtkNew<vtkPolyData> polydata;
  polydata->SetPoints(points);
  polydata->SetLines(lineCells);

  if (hasColors)
  {
    polydata->GetCellData()->SetScalars(colors);
  }
  polydata->Squeeze();

  return polydata;
}

//----------------------------------------------------------------------------
// Creates vtkPartitionedDataSet(s) for surface and curve geometry and adds them
// to a vtkPartitionedDataSetCollection and vtkDataAssembly node
//
// For datasetIndex will be incremented for each vtkPartitionedDataSet created.

void vtkOCCTPDCReader::vtkInternals::AddToDataSetCollection(const std::string& name,
  vtkPolyData* surfaces, vtkPolyData* curves, vtkPartitionedDataSetCollection* output,
  vtkDataAssembly* assembly, int node, unsigned int& datasetIndex)
{
  // Do we have surfaces?
  if (surfaces)
  {
    auto pds = vtkSmartPointer<vtkPartitionedDataSet>::New();
    pds->SetPartition(0, surfaces);

    output->SetPartitionedDataSet(datasetIndex, pds);
    output->GetMetaData(datasetIndex)->Set(vtkCompositeDataSet::NAME(), name.c_str());

    assembly->AddDataSetIndex(node, datasetIndex);
    ++datasetIndex;
  }

  // Do we have curves?
  if (curves)
  {
    auto pds = vtkSmartPointer<vtkPartitionedDataSet>::New();
    pds->SetPartition(0, curves);

    output->SetPartitionedDataSet(datasetIndex, pds);
    // If we have also added surfaces, lets append _curves to the
    // name to make it unique
    std::string curveName = (surfaces ? name + "_curves" : name);
    output->GetMetaData(datasetIndex)->Set(vtkCompositeDataSet::NAME(), curveName.c_str());

    assembly->AddDataSetIndex(node, datasetIndex);
    ++datasetIndex;
  }
}
//----------------------------------------------------------------------------
// This method will process an OpenCascade label and update a
// vtkPartitionedDataSetCollection and vtkDataAssembly accordingly
//
// parentLocation - is the transformation that needs to be applied
// to geometry in order to get it into World Coordinates
//
// datasetIndex - the next available index in the vtkPartitionedDataSetCollection
//
// parentNameCount - a map that counts the number of times a label's name has been
// encountered for the OpenCascade assembly node that this label is a child of. This
// is used to make sure all nodes being inserted in the vtkDataAssembly parentNode are
// unique

void vtkOCCTPDCReader::vtkInternals::AddLabelToOutput(const TDF_Label& label,
  const TopLoc_Location& parentLocation, vtkPartitionedDataSetCollection* output,
  vtkDataAssembly* assembly, int parentNode, unsigned int& datasetIndex,
  std::unordered_map<std::string, NodeInfo>& parentNameCount)
{
  std::string name = this->GetLabelName(label);
  int node;

  // If the label is not named then we will create a node for it in the assembly and will use
  // the parent node instead
  if (name.empty())
  {
    node = parentNode;
  }
  else
  {
    // If the label is named, make sure the name is unique w/r to its siblings using nodeNameCount
    auto it = parentNameCount.find(name);
    if (it == parentNameCount.end())
    {
      // name is unique - add it to the map with an initial count of 1
      node = assembly->AddNode(name.c_str(), parentNode);
      NodeInfo ninfo(node, 1);
      parentNameCount[name] = ninfo;
    }
    else
    {
      std::ostringstream str;
      str << name << "_" << std::setw(3) << std::setfill('0') << it->second.Count;
      name = str.str();
      it->second.Count = it->second.Count + 1;
      node = assembly->AddNode(name.c_str(), parentNode);
      NodeMap[node] = it->second.NodeId;
    }
  }

  // If this is an assembly then process all of its children.
  if (this->ShapeTool->IsAssembly(label))
  {
    OCC_LabelSequence children;

    // If a new node was created then we need to provide an
    // empty map to make its children node names are unique
    // Else we will use the map passed to use from its parent
    std::unordered_map<std::string, NodeInfo> nodeCountMap;
    std::unordered_map<std::string, NodeInfo>& myChildrenNameCount =
      (node == parentNode ? parentNameCount : nodeCountMap);

    // Traverse the label's children
    this->ShapeTool->GetComponents(label, children);

    for (Standard_Integer i = 1; i <= children.Length(); ++i)
    {
      TDF_Label child = children.Value(i);

      // Combine the child's transformation with the transformation
      // that was passed in
      TopLoc_Location childLocation = parentLocation * this->ShapeTool->GetLocation(child);

      TDF_Label referred;
      // If the child refers to another piece of geometry, process the
      // reference - NOTE that this will use the reference's name in the
      // assembly tree and not the child
      if (this->ShapeTool->IsReference(child) && this->ShapeTool->GetReferredShape(child, referred))
      {
        AddLabelToOutput(
          referred, childLocation, output, assembly, node, datasetIndex, myChildrenNameCount);
      }
      else
      {
        AddLabelToOutput(
          child, childLocation, output, assembly, node, datasetIndex, myChildrenNameCount);
      }
    }

    return;
  }

  // If this label does not have geometry skip it
  if (!this->ShapeTool->IsShape(label))
  {
    return;
  }

  // Lets find the geometry that goes with this shape
  vtkPolyData *surfs, *wires;
  auto id = this->GetHash(label);
  auto sit = this->ShapeMap.find(id);
  surfs = (sit != this->ShapeMap.end() ? sit->second : nullptr);

  auto wit = this->WireMap.find(id);
  wires = (wit != this->WireMap.end() ? wit->second : nullptr);

  // If there are no geometry then return
  if ((surfs == nullptr) && (wires == nullptr))
  {
    vtkDebugWithObjectMacro(this->Parent, "Could not find geometry for " << name);
    return;
  }

  // Do we need to transform the geometry?  If not then just add directly from the map
  // and return;
  if (parentLocation.IsIdentity())
  {
    this->AddToDataSetCollection(name, surfs, wires, output, assembly, node, datasetIndex);
    return;
  }

  vtkSmartPointer<vtkPolyData> transSurfs, transWires;
  // Create a transformation filter and define a vtkMatrix
  // based on the OpenCascade transformation provided
  vtkNew<vtkMatrix4x4> mat;
  this->GetMatrix(parentLocation, mat);
  vtkNew<vtkTransformFilter> transfoFilter;
  vtkNew<vtkTransform> transfo;
  transfo->SetMatrix(mat);
  transfoFilter->SetTransform(transfo);

  // Transform the surfaces and wires if they exist
  if (surfs)
  {
    transfoFilter->SetInputData(surfs);
    transfoFilter->Update();
    transSurfs = vtkSmartPointer<vtkPolyData>::New();
    transSurfs->ShallowCopy(transfoFilter->GetOutput());
  }
  if (wires)
  {
    transfoFilter->SetInputData(wires);
    transfoFilter->Update();
    transWires = vtkSmartPointer<vtkPolyData>::New();
    transWires->ShallowCopy(transfoFilter->GetOutput());
  }

  this->AddToDataSetCollection(name, transSurfs, transWires, output, assembly, node, datasetIndex);
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOCCTPDCReader);
//----------------------------------------------------------------------------
vtkOCCTPDCReader::vtkOCCTPDCReader()
  : Internals(std::make_unique<vtkOCCTPDCReader::vtkInternals>(this))
{
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
vtkOCCTPDCReader::~vtkOCCTPDCReader()
{
  this->SetFileName(nullptr);
}

//----------------------------------------------------------------------------
void vtkOCCTPDCReader::SetRootNodeName(const char* name)
{
  if ((name == nullptr) || (name[0] == '\0'))
  {
    vtkWarningMacro("Can not set RootNodeName to be an empty string.");
    return;
  }
  if (this->RootNodeName == name)
  {
    return;
  }
  this->RootNodeName = name;
  this->Modified();
}

//----------------------------------------------------------------------------
vtkOCCTPDCReader::Format vtkOCCTPDCReader::DetermineDataFormat()
{
  // If the format is not set to auto then just return it
  if (this->FileFormat != Format::AUTO)
  {
    return static_cast<Format>(this->FileFormat);
  }

  if (this->Stream)
  {
    return GetFormatFromStream(this->Stream);
  }

  // Was the filename set?
  if (!this->FileName || this->FileName[0] == '\0')
  {
    vtkErrorMacro("No FileName or Stream specified.");
    return Format::UNSUPPORTED;
  }

  // Check the contents
  vtkNew<vtkFileResourceStream> stream;
  if (!stream->Open(this->FileName))
  {
    vtkErrorMacro("Could not open file to determine format.");
    return Format::UNSUPPORTED;
  }
  return GetFormatFromStream(stream);
}

//----------------------------------------------------------------------------
int vtkOCCTPDCReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkPartitionedDataSetCollection* output = vtkPartitionedDataSetCollection::GetData(outputVector);
  if (!output)
  {
    vtkErrorMacro("Invalid vtkPartitionedDataSetCollection output.");
    return 0;
  }

  // Reset the output and the Internals to start fresh
  output->Initialize();
  this->Internals->reset();

  Message::DefaultMessenger()->RemovePrinters(STANDARD_TYPE(Message_PrinterOStream));

  // Read in the geometry into OpenCascade
  Handle(TDocStd_Document) doc;
  XCAFApp_Application::GetApplication()->NewDocument("MDTV-XCAF", doc);
  auto format = this->DetermineDataFormat();
  if (format == Format::STEP)
  {
    STEPCAFControl_Reader reader;
    if (!TransferToDocument(this, reader, doc))
    {
      return 0;
    }
  }
  else if (format == Format::IGES)
  {
    // Stream support for IGES is not currently supported
    if (this->GetStream())
    {
      vtkErrorMacro("Streaming IGES date is unsupported.");
      return 0;
    }
    IGESCAFControl_Reader reader;
    if (!TransferToDocument(this, reader, doc))
    {
      return 0;
    }
  }
  else if (format == Format::BREP)
  {
    if (!this->Internals->AddBRepToDocument(doc))
    {
      return 0;
    }
  }
  else if (format == Format::XBF)
  {
    if (!this->Internals->ReadXBFToDocument(doc))
    {
      return 0;
    }
  }
  else
  {
    vtkErrorMacro("Data format: " << this->FormatToString(format) << " is unsupported.");
    return 0;
  }

  this->Internals->ShapeTool = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
  this->Internals->ColorTool = XCAFDoc_DocumentTool::ColorTool(doc->Main());

  OCC_LabelSequence topLevelShapes;

  // Visit all of the base geometric shapes and for each one create
  // vtkPolydata for its surfaces and optionally for its wires.
  // Insert each into its appropriate map
  this->Internals->ShapeTool->GetShapes(topLevelShapes);

  for (Standard_Integer iLabel = 1; iLabel <= topLevelShapes.Length(); ++iLabel)
  {
    TDF_Label label = topLevelShapes.Value(iLabel);

    TopoDS_Shape shape;
    this->Internals->ShapeTool->GetShape(label, shape);

    const auto styles = this->Internals->CollectInheritedStyles(label, shape);
    auto id = this->Internals->GetHash(label);
    vtkSmartPointer<vtkPolyData> pd = this->Internals->SurfacesToPolyData(shape, styles);
    if (pd)
    {
      this->Internals->ShapeMap[id] = pd;
    }

    if (this->GetReadWire())
    {
      vtkSmartPointer<vtkPolyData> pd = this->Internals->WiresToPolyData(shape, styles);
      if (pd)
      {
        this->Internals->WireMap[id] = pd;
      }
    }
    double progress = 0.5 + static_cast<double>(iLabel) / topLevelShapes.Length();
    this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
  }

  // create the partitioned dataset collection

  auto assembly = vtkSmartPointer<vtkDataAssembly>::New();
  assembly->Initialize();
  assembly->SetRootNodeName(this->RootNodeName.c_str());
  output->SetDataAssembly(assembly);
  unsigned int datasetIndex = 0;

  // Prepare to traverse all top level shapes which also includes top-level
  // assemblies
  this->Internals->ShapeTool->GetFreeShapes(topLevelShapes);

  // Create an identity location
  gp_Trsf ident; // Identity by default
  TopLoc_Location identLocation(ident);

  // Provide a map that will be used to make sure all sibling node names are unique
  std::unordered_map<std::string, NodeInfo> myChildrenNameCount;
  for (Standard_Integer iLabel = 1; iLabel <= topLevelShapes.Length(); ++iLabel)
  {
    this->Internals->AddLabelToOutput(topLevelShapes.Value(iLabel), identLocation, output, assembly,
      assembly->GetRootNode(), datasetIndex, myChildrenNameCount);
  }

  if (this->CreateRedundantMap && (!this->Internals->NodeMap.empty()))
  {
    // Create a 2 tuple vtk int array to store the map info
    vtkNew<vtkIntArray> nodeMapping;
    nodeMapping->SetName("NodeMapping");
    nodeMapping->SetNumberOfComponents(2);
    nodeMapping->SetNumberOfTuples(this->Internals->NodeMap.size());
    int i = 0;
    for (const auto& nodes : this->Internals->NodeMap)
    {
      nodeMapping->SetComponent(i, 0, nodes.first);
      nodeMapping->SetComponent(i, 1, nodes.second);
      ++i;
    }
    output->GetFieldData()->AddArray(nodeMapping);
  }
  return 1;
}

//----------------------------------------------------------------------------
bool vtkOCCTPDCReader::CanReadFile(vtkResourceStream* stream)
{
  Format f;
  return vtkOCCTPDCReader::CanReadFile(stream, f);
}
//----------------------------------------------------------------------------
bool vtkOCCTPDCReader::CanReadFile(vtkResourceStream* stream, vtkOCCTPDCReader::Format& format)
{
  format = GetFormatFromStream(stream);
  return (format != Format::UNSUPPORTED);
}
//----------------------------------------------------------------------------
std::string vtkOCCTPDCReader::FormatToString(unsigned int format)
{
  switch (format)
  {
    case Format::AUTO:
      return "AUTO";
    case Format::STEP:
      return "STEP";
    case Format::IGES:
      return "IGES";
    case Format::BREP:
      return "BREP";
    case Format::XBF:
      return "XBF";
    default:
      return "UNSUPPORTED";
  }
}

//----------------------------------------------------------------------------
void vtkOCCTPDCReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "LinearDeflection: " << this->LinearDeflection << "\n";
  os << indent << "AngularDeflection: " << this->AngularDeflection << "\n";
  os << indent << "RelativeDeflection: " << (this->RelativeDeflection ? "true" : "false") << "\n";
  os << indent << "ReadWire: " << (this->ReadWire ? "true" : "false") << "\n";
  os << indent << "FileFormat: " << this->FormatToString(this->FileFormat) << "\n";
  os << indent << "RootNodeName: " << this->RootNodeName << "\n";
  os << indent << "Stream: " << this->Stream.GetPointer() << "\n";
  os << indent << "CreateRedundantMap: " << (this->CreateRedundantMap ? "true" : "false") << "\n";
}
VTK_ABI_NAMESPACE_END
