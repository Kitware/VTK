// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkBatchedLabeledDataMapper.h"
#include "Private/vtkLabeledFormatter.h"

#include "vtkArrayDispatch.h"
#include "vtkDataArrayRange.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkFreeTypeTools.h"
#include "vtkIdTypeArray.h"
#include "vtkImageAppend.h"
#include "vtkImageClip.h"
#include "vtkImageConstantPad.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPlaneCollection.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkStringFormatter.h"
#include "vtkStringScanner.h"
#include "vtkTextProperty.h"
#include "vtkTimeStamp.h"
#include "vtkTransform.h"

#include <array>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

struct WordRecord2D
{
  int PropId;
  vtkSmartPointer<vtkImageData> Texture;
};

class vtkBatchedLabeledDataMapper::vtkInternals
{
public:
  vtkInternals()
  {
    this->GlyphExtents->SetNumberOfComponents(4);
    this->GlyphExtents->SetName("glyphExtents");
    this->Coff->SetNumberOfComponents(1);
    this->Coff->SetName("coff");
    this->PId->SetNumberOfComponents(1);
    this->PId->SetName("pid");
    this->PropId->SetNumberOfComponents(1);
    this->PropId->SetName("propid");
    this->Framecolors->SetNumberOfComponents(3);
    this->Framecolors->SetName("framecolors");
    this->ImageAppender->PreserveExtentsOn();
    this->TextProperties[0] = vtkSmartPointer<vtkTextProperty>::New();
  }

  ~vtkInternals() { this->InputPlusArrays->Initialize(); }

  void FreshIPA()
  {
    this->InputPlusArrays->Initialize();
    this->InputPlusArrays->Allocate();
    this->GlyphExtents->Initialize();
    this->Coff->Initialize();
    this->PId->Initialize();
    this->PropId->Initialize();
    this->Framecolors->Initialize();
  }

  WordRecord2D MakeWordTexture(vtkStdString word, vtkTextProperty* prop, int propID)
  {
    auto nchar = vtkSmartPointer<vtkImageData>::New();
    int textdims[2];

    auto tren = vtkFreeTypeTools::GetInstance();
    bool lasts2p2 = tren->GetScaleToPowerTwo();
    tren->ScaleToPowerTwoOff();
    bool iWasFramed = prop->GetFrame();
    prop->FrameOff();

    tren->RenderString(prop, word, this->DPI, nchar, textdims);

    if (this->Descenders[propID] < 0)
    {
      auto faceMetrics = tren->GetFaceMetrics(prop);
      int descender = -faceMetrics.Descender * prop->GetFontSize() / faceMetrics.UnitsPerEM;
      this->Descenders[propID] = descender;
    }

    prop->SetFrame(iWasFramed);
    tren->SetScaleToPowerTwo(lasts2p2);

    int charExt[6];
    nchar->GetExtent(charExt);
    if (charExt[1] < charExt[0] || charExt[3] < charExt[2])
    {
      return {};
    }
    double bg[4];
    prop->GetBackgroundColor(bg);
    bg[0] = bg[0] * 255;
    bg[1] = bg[1] * 255;
    bg[2] = bg[2] * 255;
    bg[3] = prop->GetBackgroundOpacity() * 255;
    vtkNew<vtkDoubleArray> componentConstants;
    componentConstants->SetArray(bg, 4, 1);
    int clipPix = ((bg[3] > 0) ? 2 : 0);
    this->ImageClipper->SetInputData(nchar);
    this->ImageClipper->SetOutputWholeExtent(
      charExt[0] + clipPix, charExt[1] - clipPix, charExt[2] + clipPix, charExt[3] - clipPix, 0, 0);

    this->ImagePadder->SetInputConnection(this->ImageClipper->GetOutputPort());
    this->ImagePadder->SetComponentConstants(componentConstants);
    this->ImagePadder->SetOutputWholeExtent(
      charExt[0] + clipPix - vtkBatchedLabeledDataMapper::GlyphAtlasPadding,
      charExt[1] - clipPix + vtkBatchedLabeledDataMapper::GlyphAtlasPadding,
      charExt[2] + clipPix - vtkBatchedLabeledDataMapper::GlyphAtlasPadding,
      charExt[3] - clipPix + vtkBatchedLabeledDataMapper::GlyphAtlasPadding, 0, 0);
    this->ImagePadder->Update();
    vtkImageData* outI = this->ImagePadder->GetOutput();
    outI->GetExtent(charExt);
    nchar->ShallowCopy(outI);

    WordRecord2D wr;
    wr.PropId = propID;
    wr.Texture = nchar;
    return wr;
  }

  void AppendToWordTexture(vtkImageData* in, int propIdx, int& sx, int& sy, int& ex, int& ey)
  {
    constexpr int PAD = 1;
    int x0, x1, y0, y1;
    int wordsdims[3];
    in->GetDimensions(wordsdims);

    bool nextcolumn = (JPos == vtkBatchedLabeledDataMapper::GlyphAtlasColumnSize);
    if (nextcolumn)
    {
      x0 = ex;
      x1 = ex + wordsdims[0] - 1;
      y0 = 0;
      y1 = 0 + wordsdims[1] - 1;
      sx = ex;
      ex = std::max(ex, x1 + PAD);
      sy = 0;
      ey = y1 + PAD;
    }
    else
    {
      x0 = sx;
      x1 = sx + wordsdims[0] - 1;
      y0 = ey;
      y1 = ey + wordsdims[1] - 1;
      ex = std::max(ex, x1 + PAD);
      sy = ey;
      ey = y1 + PAD;
    }

    in->SetExtent(x0, x1, y0, y1, 0, 0);
    this->MaxGlyphHeights[propIdx] = std::max(this->MaxGlyphHeights[propIdx],
      y1 - y0 + 1 - (2 * vtkBatchedLabeledDataMapper::GlyphAtlasPadding));

    if (JPos == vtkBatchedLabeledDataMapper::GlyphAtlasColumnSize)
    {
      IPos++;
      JPos = 0;
    }
    else
    {
      JPos++;
    }
  }

  void MakeItSo()
  {
    this->ImageAppender->RemoveAllInputs();
    auto it = this->AllStrings.begin();
    int cnt = 0;
    while (it != this->AllStrings.end())
    {
      cnt++;
      this->ImageAppender->AddInputData(it->second.Texture);
      if (!(cnt % 10000))
      {
        cnt = 0;
        this->ImageAppender->Update();
        auto tempID = vtkSmartPointer<vtkImageData>::New();
        tempID->DeepCopy(this->ImageAppender->GetOutput());
        this->ImageAppender->RemoveAllInputs();
        this->ImageAppender->AddInputData(tempID);
      }
      it++;
    }
    this->ImageAppender->Update();
    this->WordsTexture = this->ImageAppender->GetOutput();
  }

  void UpdateTextPropertyAttributeArrays()
  {
    std::array<double, 4> bg{ 0. };
    std::array<double, 4> frame{ 0. };
    int frameWidth = 0;

    for (std::size_t i = 0; i < vtkBatchedLabeledDataMapper::MaxTextProperties; ++i)
    {
      vtkTextProperty* prop = this->TextProperties[i].Get();
      if (prop)
      {
        prop->GetBackgroundColor(bg.data());
        bg[3] = prop->GetBackgroundOpacity();
        prop->GetFrameColor(frame.data());
        frame[3] = 1.;
        frameWidth = prop->GetFrame() ? prop->GetFrameWidth() : 0;
      }
      this->BackgroundColors[i][0] = static_cast<float>(bg[0]);
      this->BackgroundColors[i][1] = static_cast<float>(bg[1]);
      this->BackgroundColors[i][2] = static_cast<float>(bg[2]);
      this->BackgroundColors[i][3] = static_cast<float>(bg[3]);
      this->FrameWidths[i] = frameWidth;
    }
  }

  vtkNew<vtkImageAppend> ImageAppender;
  vtkNew<vtkImageConstantPad> ImagePadder;
  vtkNew<vtkImageClip> ImageClipper;
  vtkNew<vtkPolyData> InputPlusArrays;
  vtkNew<vtkIntArray> GlyphExtents;
  vtkNew<vtkFloatArray> Coff;
  vtkNew<vtkIdTypeArray> PId;
  vtkNew<vtkFloatArray> PropId;
  vtkNew<vtkFloatArray> Framecolors;
  int IPos = 0;
  int JPos = 0;
  std::map<std::pair<std::string, int>, WordRecord2D> AllStrings;
  vtkSmartPointer<vtkImageData> WordsTexture;
  int DPI = 72;

  std::array<vtkSmartPointer<vtkTextProperty>, vtkBatchedLabeledDataMapper::MaxTextProperties>
    TextProperties;

  float BackgroundColors[vtkBatchedLabeledDataMapper::MaxTextProperties][4];
  int FrameWidths[vtkBatchedLabeledDataMapper::MaxTextProperties];
  int MaxGlyphHeights[vtkBatchedLabeledDataMapper::MaxTextProperties];
  int Descenders[vtkBatchedLabeledDataMapper::MaxTextProperties];

  vtkTimeStamp LastAtlasUploadTime;
};

//----------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkBatchedLabeledDataMapper);

//----------------------------------------------------------------------------
vtkBatchedLabeledDataMapper::vtkBatchedLabeledDataMapper()
{
  this->Impl = std::unique_ptr<vtkInternals>(new vtkInternals());
  this->AllocateLabels(50);

  auto prop = vtkSmartPointer<vtkTextProperty>::New();
  prop->SetFontFamilyAsString("Arial");
  prop->SetFontSize(24);
  prop->SetColor(1.0, 1.0, 1.0);
  prop->SetBackgroundColor(1.0, 0.0, 0.0);
  this->SetLabelTextProperty(prop);
}

//----------------------------------------------------------------------------
vtkBatchedLabeledDataMapper::~vtkBatchedLabeledDataMapper()
{
  this->SetFrameColorsName(nullptr);
}

//----------------------------------------------------------------------------
void vtkBatchedLabeledDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Frame Color Name: " << (this->FrameColorsName ? this->FrameColorsName : "(none)")
     << "\n";
  os << indent << "Display Offset: [" << this->DisplayOffset[0] << ", " << this->DisplayOffset[1]
     << "]\n";
}

//----------------------------------------------------------------------------
int vtkBatchedLabeledDataMapper::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkBatchedLabeledDataMapper::SetLabelTextProperty(vtkTextProperty* prop, int type)
{
  if (type >= vtkBatchedLabeledDataMapper::MaxTextProperties)
  {
    vtkErrorMacro("Maximum number of text properties exceeded ("
      << type << " >= " << vtkBatchedLabeledDataMapper::MaxTextProperties << ").");
    return;
  }

  if (this->Impl->TextProperties[type].Get() == prop)
  {
    return;
  }

  this->Impl->TextProperties[type] = prop;
  this->Impl->AllStrings.clear();
  std::fill_n(this->Impl->Descenders, vtkBatchedLabeledDataMapper::MaxTextProperties, -1);
  this->Impl->WordsTexture = vtkSmartPointer<vtkImageData>::New();
  this->Modified();
}

//----------------------------------------------------------------------------
vtkTextProperty* vtkBatchedLabeledDataMapper::GetLabelTextProperty(int type)
{
  if (type >= vtkBatchedLabeledDataMapper::MaxTextProperties)
  {
    vtkErrorMacro("Maximum number of text properties exceeded ("
      << type << " >= " << vtkBatchedLabeledDataMapper::MaxTextProperties << ").");
    return nullptr;
  }

  return this->Impl->TextProperties[type];
}

//----------------------------------------------------------------------------
void vtkBatchedLabeledDataMapper::AllocateLabels(int numLabels)
{
  this->NumberOfLabelsAllocated = std::max(numLabels, this->NumberOfLabelsAllocated);
}

//----------------------------------------------------------------------------
void vtkBatchedLabeledDataMapper::MakeShaderArrays(int numCurLabels,
  const std::vector<std::string>& stringlist, vtkIntArray* typeArr, vtkFloatArray* fcolArr,
  const std::vector<bool>& visible)
{
  int pntcnt = 0;
  for (int i = 0; i < numCurLabels; i++)
  {
    if (!visible[i])
    {
      continue;
    }
    const auto& wordString = stringlist[i];
    int wordsPropId = 0;
    if (typeArr)
    {
      wordsPropId = typeArr->GetValue(i);
    }
    double fcolors[3] = { 0.0, 0.0, 0.0 };
    if (fcolArr)
    {
      fcolArr->GetTuple(i, fcolors);
    }
    else
    {
      this->Impl->TextProperties[wordsPropId]->GetFrameColor(fcolors);
    }
    vtkTextProperty* prop = this->GetLabelTextProperty(wordsPropId);
    if (!prop)
    {
      vtkErrorMacro("No text property available for type array entry '" << wordsPropId << "'.");
      continue;
    }

    double coffset = 0.0;
    vtkIdType startpt = pntcnt;
    for (std::size_t cidx = 0; cidx < wordString.size(); cidx++)
    {
      std::string c = wordString.substr(cidx, 1);
      WordRecord2D wrec = this->Impl->AllStrings.find(std::make_pair(c, wordsPropId))->second;
      vtkImageData* wr = wrec.Texture;
      int wordsextents[6];
      wr->GetExtent(wordsextents);
      vtkIdType ptlist = pntcnt;
      pntcnt++;
      this->Impl->InputPlusArrays->InsertNextCell(VTK_VERTEX, 1, &ptlist);
      this->Impl->PId->InsertNextValue(i);
      this->Impl->GlyphExtents->InsertNextTypedTuple(wordsextents);
      this->Impl->Coff->InsertNextValue(coffset);
      float width = wordsextents[1] - wordsextents[0] + 1 -
        (2 * vtkBatchedLabeledDataMapper::GlyphAtlasPadding);
      coffset += width;
      this->Impl->PropId->InsertNextValue(wrec.PropId);
      this->Impl->Framecolors->InsertNextTuple3(fcolors[0], fcolors[1], fcolors[2]);
    }
    // align glyphs
    if (this->TextAnchor == LowerLeft || this->TextAnchor == UpperLeft ||
      this->TextAnchor == LeftEdge)
    {
      coffset = 0.0;
    }
    else if (this->TextAnchor == LowerEdge || this->TextAnchor == UpperEdge ||
      this->TextAnchor == Center)
    {
      coffset = coffset / 2;
    }
    for (std::size_t cidx = 0; cidx < wordString.size(); cidx++)
    {
      double pos = this->Impl->Coff->GetValue(startpt + cidx);
      pos = pos - coffset;
      this->Impl->Coff->SetValue(startpt + cidx, pos);
    }
  }
  this->Impl->InputPlusArrays->GetPointData()->AddArray(this->Impl->GlyphExtents);
  this->Impl->InputPlusArrays->GetPointData()->AddArray(this->Impl->Coff);
  this->Impl->InputPlusArrays->GetPointData()->AddArray(this->Impl->PId);
  this->Impl->InputPlusArrays->GetPointData()->AddArray(this->Impl->PropId);
  this->Impl->InputPlusArrays->GetPointData()->AddArray(this->Impl->Framecolors);
  this->Impl->GlyphExtents->Modified();
  this->Impl->Coff->Modified();
  this->Impl->PId->Modified();
  this->Impl->PropId->Modified();
  this->Impl->Framecolors->Modified();
}

struct vtkBatchedLabeledDataMapper::vtkBatchedLabeledDataMapperFormatter
  : vtkLabeledFormatterInterface
{
  int RebuildCount;
  int NumCurChars;
  std::vector<std::string> Strings;

  vtkBatchedLabeledDataMapperFormatter(
    vtkBatchedLabeledDataMapper* self, vtkIntArray* typeArr, int numCurLabels)
    : vtkLabeledFormatterInterface(self, typeArr, numCurLabels)
    , RebuildCount(0)
    , NumCurChars(0)
  {
    this->Strings.resize(numCurLabels);
  }

  void SetFormattedString(int i, const char* cResultString) override
  {
    auto* batchedSelf = static_cast<vtkBatchedLabeledDataMapper*>(this->Self);
    std::string_view resultString(cResultString);
    for (std::size_t cidx = 0; cidx < resultString.size(); cidx++)
    {
      auto c = std::string(resultString.substr(cidx, 1));
      auto hasTexture = batchedSelf->Impl->AllStrings.find(std::make_pair(c, 0));
      if (hasTexture == batchedSelf->Impl->AllStrings.end())
      {
        this->RebuildCount++;
        for (int tid = 0; tid < vtkBatchedLabeledDataMapper::MaxTextProperties; ++tid)
        {
          vtkTextProperty* prop = batchedSelf->Impl->TextProperties[tid];
          if (prop)
          {
            WordRecord2D wr = batchedSelf->Impl->MakeWordTexture(c, prop, tid);
            batchedSelf->Impl->AllStrings[std::make_pair(c, tid)] = wr;
          }
        }
      }
    }
    this->Strings[i] = resultString;
  }
};

//----------------------------------------------------------------------------
void vtkBatchedLabeledDataMapper::BuildLabelsInternal(vtkDataSet* input)
{
  auto formatterInput =
    this->ResolveLabeledFormatterInput(input->GetPointData(), input->GetNumberOfPoints(), input);
  if (!formatterInput.Valid)
  {
    return;
  }
  if (this->NumberOfLabelsAllocated < (this->NumberOfLabels + formatterInput.NumCurLabels))
  {
    vtkErrorMacro("Number of labels must be allocated before this method is called.");
    return;
  }

  vtkBatchedLabeledDataMapperFormatter formatter(
    this, formatterInput.TypeArr, formatterInput.NumCurLabels);
  formatter.Dispatch(formatterInput);
  this->NumberOfLabels += formatterInput.NumCurLabels;

  auto* fcolArr =
    vtkArrayDownCast<vtkFloatArray>(input->GetPointData()->GetAbstractArray(this->FrameColorsName));

  // Compute transformed positions and clipping visibility for all labels upfront so both
  // the shader-array build and the point-buffer build use consistent filtered data.
  const int numLabels = formatterInput.NumCurLabels;
  std::vector<std::array<double, 3>> transformedPts(numLabels);
  std::vector<bool> visible(numLabels, true);
  const bool hasTransform = this->Transform && !this->Transform->GetMatrix()->IsIdentity();
  const bool hasClipping = this->ClippingPlanes && this->GetNumberOfClippingPlanes() > 0;
  for (int i = 0; i < numLabels; i++)
  {
    double pt[3];
    input->GetPoint(i, pt);
    if (hasTransform)
    {
      const double* tpt = this->Transform->TransformDoublePoint(pt);
      pt[0] = tpt[0];
      pt[1] = tpt[1];
      pt[2] = tpt[2];
    }
    transformedPts[i] = { pt[0], pt[1], pt[2] };
    if (hasClipping)
    {
      for (int p = 0; p < this->GetNumberOfClippingPlanes(); ++p)
      {
        if (this->ClippingPlanes->GetItem(p)->FunctionValue(pt) < 0.0)
        {
          visible[i] = false;
          break;
        }
      }
    }
  }

  // Path A: input changed but no string/atlas rebuild needed — refresh shader arrays only.
  // Also fires on mapper mtime change (transform or clipping) to reflect the updated visibility.
  if (((input->GetMTime() > this->BuildTime) || (this->GetMTime() > this->BuildTime)) &&
    !formatter.RebuildCount)
  {
    this->Impl->FreshIPA();
    this->MakeShaderArrays(numLabels, formatter.Strings, formatterInput.TypeArr, fcolArr, visible);
  }
  // Path B: string content changed — full atlas + shader-array rebuild.
  if (formatter.RebuildCount)
  {
    std::fill_n(this->Impl->MaxGlyphHeights, vtkBatchedLabeledDataMapper::MaxTextProperties, 0);
    this->Impl->IPos = 0;
    this->Impl->JPos = 0;
    int sx = 0, sy = 0, ex = 0, ey = 0;
    this->Impl->FreshIPA();
    for (auto writ = this->Impl->AllStrings.begin(); writ != this->Impl->AllStrings.end(); ++writ)
    {
      int propIdx = writ->first.second;
      WordRecord2D wr = writ->second;
      this->Impl->AppendToWordTexture(wr.Texture, propIdx, sx, sy, ex, ey);
    }
    this->MakeShaderArrays(numLabels, formatter.Strings, formatterInput.TypeArr, fcolArr, visible);
    this->Impl->MakeItSo();
  }

  const vtkMTimeType imageMTime = this->Impl->WordsTexture->GetMTime();
  if (formatter.RebuildCount || this->Impl->LastAtlasUploadTime < imageMTime)
  {
    this->UploadGlyphAtlas(this->Impl->WordsTexture);
    this->Impl->LastAtlasUploadTime.Modified();
  }

  // Rebuild point buffer when input, strings, transform, or clipping planes change.
  // FreshIPA (called in path A and B above) clears the points, so this must fire
  // whenever either path A or B fires.
  if (input->GetMTime() > this->BuildTime || formatter.RebuildCount ||
    this->GetMTime() > this->BuildTime)
  {
    auto pts = vtkSmartPointer<vtkPoints>::New();
    this->Impl->InputPlusArrays->SetPoints(pts);
    for (int i = 0; i < numLabels; i++)
    {
      if (!visible[i])
      {
        continue;
      }
      const auto& wordString = formatter.Strings[i];
      const auto& pt = transformedPts[i];
      for (unsigned int cidx = 0; cidx < wordString.size(); cidx++)
      {
        pts->InsertNextPoint(pt.data());
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkBatchedLabeledDataMapper::BuildLabels()
{
  this->Superclass::BuildLabels();
  this->Impl->UpdateTextPropertyAttributeArrays();
}

//----------------------------------------------------------------------------
vtkImageData* vtkBatchedLabeledDataMapper::GetGlyphAtlas() const
{
  return this->Impl->WordsTexture;
}

//----------------------------------------------------------------------------
vtkPolyData* vtkBatchedLabeledDataMapper::GetPreparedPolyData() const
{
  return this->Impl->InputPlusArrays;
}

//----------------------------------------------------------------------------
void vtkBatchedLabeledDataMapper::UpdateRenderWindowDPI(int dpi)
{
  if (this->Impl->DPI != dpi)
  {
    this->Impl->DPI = dpi;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
const float* vtkBatchedLabeledDataMapper::GetBackgroundColors() const
{
  return &this->Impl->BackgroundColors[0][0];
}

//----------------------------------------------------------------------------
const int* vtkBatchedLabeledDataMapper::GetFrameWidths() const
{
  return this->Impl->FrameWidths;
}

//----------------------------------------------------------------------------
const int* vtkBatchedLabeledDataMapper::GetMaxGlyphHeights() const
{
  return this->Impl->MaxGlyphHeights;
}

//----------------------------------------------------------------------------
const int* vtkBatchedLabeledDataMapper::GetDescenders() const
{
  return this->Impl->Descenders;
}

//----------------------------------------------------------------------
vtkMTimeType vtkBatchedLabeledDataMapper::GetMTime()
{
  vtkMTimeType mtime = this->Superclass::GetMTime();

  for (vtkTextProperty* tprop : this->Impl->TextProperties)
  {
    if (tprop)
    {
      mtime = std::max(mtime, tprop->GetMTime());
    }
  }

  if (this->Transform)
  {
    mtime = std::max(mtime, this->Transform->GetMTime());
  }

  return mtime;
}

//----------------------------------------------------------------------------
void vtkBatchedLabeledDataMapper::ReleaseGraphicsResources(vtkWindow*)
{
  this->Impl->LastAtlasUploadTime = vtkTimeStamp{};
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkBatchedLabeledDataMapper::UploadGlyphAtlas(vtkImageData*) {}

//----------------------------------------------------------------------------
void vtkBatchedLabeledDataMapper::ActivateGlyphTexture() {}

//----------------------------------------------------------------------------
void vtkBatchedLabeledDataMapper::DeactivateGlyphTexture() {}

VTK_ABI_NAMESPACE_END
