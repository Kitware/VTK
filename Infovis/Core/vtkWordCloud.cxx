// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWordCloud.h"

// Older versions of GCC do not implement regex. The first working
// version was 4.9. See https://tinyurl.com/yy7lvhp3 for more details
#if defined(__GNUC__) && (__GNUC__ >= 5)
#define HAS_STD_REGEX
#endif

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkFreeTypeTools.h"
#include "vtkImageBlend.h"
#include "vtkImageData.h"
#include "vtkImageIterator.h"

#include "vtkColorSeries.h"
#include "vtkImageCanvasSource2D.h"
#include "vtkMath.h"
#include "vtkNamedColors.h"
#include "vtkTextProperty.h"

#include "vtkImageAppendComponents.h"
#include "vtkImageExtractComponents.h"
#include "vtkImageReader2.h"
#include "vtkImageReader2Factory.h"
#include "vtkImageResize.h"

#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"
#ifdef HAS_STD_REGEX
#include <regex>
#else
#include <vtksys/RegularExpression.hxx>
#endif
// stl
#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkWordCloud);

namespace
{
// Declaring the type of Predicate that accepts 2 pairs and return a bool
typedef std::function<bool(std::pair<std::string, int>, std::pair<std::string, int>)> Comparator;

std::multiset<std::pair<std::string, int>, Comparator> FindWordsSortedByFrequency(
  std::string&, vtkWordCloud*);
struct ExtentOffset
{
  ExtentOffset(int _x = 0.0, int _y = 0.0)
    : x(_x)
    , y(_y)
  {
  }
  int x, y;
};
struct ArchimedesValue
{
  ArchimedesValue(double _x = 0.0, double _y = 0.0)
    : x(_x)
    , y(_y)
  {
  }
  double x, y;
};
void AddReplacementPairsToStopList(vtkWordCloud*, vtkWordCloud::StopWordsContainer&);
bool AddWordToFinal(vtkWordCloud*, std::string, int, std::mt19937_64&, double orientation,
  std::vector<ExtentOffset>&, vtkImageBlend*, std::string&);

void ArchimedesSpiral(std::vector<ExtentOffset>&, vtkWordCloud::SizesContainer&);
void ReplaceMaskColorWithBackgroundColor(vtkImageData*, vtkWordCloud*);
void CreateBuiltInStopList(vtkWordCloud::StopWordsContainer& StopList);
void CreateStopListFromFile(std::string, vtkWordCloud::StopWordsContainer&);
void ExtractWordsFromString(std::string& str, std::vector<std::string>& words);
void ShowColorSeriesNames(ostream& os);
}

//------------------------------------------------------------------------------
vtkWordCloud::vtkWordCloud()
  : BackgroundColorName("MidnightBlue")
  , BWMask(false)
  , ColorDistribution({ { .6, 1.0 } })
  , DPI(200)
  , FontMultiplier(6)
  , Gap(2)
  , MaskColorName("black")
  , MaxFontSize(48)
  , MinFontSize(12)
  , MinFrequency(1)
  , OrientationDistribution({ { -20, 20 } })
  , Sizes({ { 640, 480 } })
{
  this->SetNumberOfInputPorts(0);

  this->OffsetDistribution[0] = -this->Sizes[0] / 100.0;
  this->OffsetDistribution[1] = this->Sizes[1] / 100.0;

  this->ImageData = vtkSmartPointer<vtkImageData>::New();
  this->ImageData->SetDimensions(640, 480, 1);
  this->ImageData->AllocateScalars(VTK_UNSIGNED_CHAR, 3);

  this->WholeExtent[0] = 0;
  this->WholeExtent[1] = 0;
  this->WholeExtent[2] = 0;
  this->WholeExtent[3] = 0;
  this->WholeExtent[4] = 0;
  this->WholeExtent[5] = 0;
}

//------------------------------------------------------------------------------
int vtkWordCloud::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // Get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->WholeExtent, 6);

  outInfo->Set(vtkDataObject::SPACING(), 1.0, 1.0, 1.0);
  outInfo->Set(vtkDataObject::ORIGIN(), 0.0, 0.0, 0.0);

  vtkDataObject::SetPointDataActiveScalarInfo(
    outInfo, this->ImageData->GetScalarType(), this->ImageData->GetNumberOfScalarComponents());
  return 1;
}

//------------------------------------------------------------------------------
int vtkWordCloud::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // Get the data object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkImageData* output = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Check some parameters before we start
  if (this->FileName.empty())
  {
    vtkErrorMacro(<< "No FileName is set. Use SetFileName to set a file.");
    return 0;
  }
  if (!vtksys::SystemTools::FileExists(this->FileName, true))
  {
    vtkErrorMacro(<< "FileName " << this->FileName << " does not exist");
    return 0;
  }
  if (!this->FontFileName.empty() && !vtksys::SystemTools::FileExists(this->FontFileName, true))
  {
    vtkErrorMacro(<< "FontFileName " << this->FontFileName << " does not exist");
    return 0;
  }
  if (!this->MaskFileName.empty() && !vtksys::SystemTools::FileExists(this->MaskFileName, true))
  {
    vtkErrorMacro(<< "MaskFileName " << this->MaskFileName << " does not exist");
    return 0;
  }
  if (!this->StopListFileName.empty() &&
    !vtksys::SystemTools::FileExists(this->StopListFileName, true))
  {
    vtkErrorMacro(<< "StopListFileName " << this->StopListFileName << " does not exist");
    return 0;
  }
  // Open the text file
  vtksys::ifstream t(this->FileName.c_str());
  std::stringstream buffer;
  buffer << t.rdbuf();
  std::string s = buffer.str();
  t.close();

  this->KeptWords.resize(0);
  this->StoppedWords.resize(0);
  this->SkippedWords.resize(0);

  // Generate a path for placement of words
  std::vector<::ExtentOffset> offset;
  ::ArchimedesSpiral(offset, this->Sizes);

  // Sort the word by frequency
  std::multiset<std::pair<std::string, int>, ::Comparator> sortedWords =
    ::FindWordsSortedByFrequency(s, this);

  // Create a mask image
  auto colors = vtkSmartPointer<vtkNamedColors>::New();
  vtkColor3ub maskColor = colors->GetColor3ub(this->MaskColorName.c_str());
  auto maskImage = vtkSmartPointer<vtkImageData>::New();

  // If a mask file is not defined, create a rectangular
  if (this->MaskFileName.empty())
  {
    auto defaultMask = vtkSmartPointer<vtkImageCanvasSource2D>::New();
    defaultMask->SetScalarTypeToUnsignedChar();
    defaultMask->SetNumberOfScalarComponents(3);
    defaultMask->SetExtent(0, this->Sizes[0] - 1, 0, this->Sizes[1] - 1, 0, 0);
    defaultMask->SetDrawColor(
      maskColor.GetData()[0], maskColor.GetData()[1], maskColor.GetData()[2]);
    defaultMask->FillBox(0, this->Sizes[0] - 1, 0, this->Sizes[1] - 1);
    defaultMask->Update();
    maskImage = defaultMask->GetOutput();
    this->AdjustedSizes[0] = this->Sizes[0];
    this->AdjustedSizes[1] = this->Sizes[1];
  }
  else
  {
    // Read the mask file
    auto readerFactory = vtkSmartPointer<vtkImageReader2Factory>::New();
    vtkSmartPointer<vtkImageReader2> reader;
    reader.TakeReference(readerFactory->CreateImageReader2(this->MaskFileName.c_str()));
    reader->SetFileName(this->MaskFileName.c_str());
    reader->Update();
    int dimensions[3];
    reader->GetOutput()->GetDimensions(dimensions);

    // Resize the mask image to match the size of the final image
    auto resize = vtkSmartPointer<vtkImageResize>::New();
    resize->SetInputData(reader->GetOutput());
    resize->InterpolateOff();
    double aspect = static_cast<double>(dimensions[1]) / static_cast<double>(dimensions[0]) *
      static_cast<double>(this->Sizes[0]) / static_cast<double>(this->Sizes[1]);
    this->AdjustedSizes[0] = this->Sizes[0];
    this->AdjustedSizes[1] = aspect * this->Sizes[1];
    resize->SetOutputDimensions(this->AdjustedSizes[0], this->AdjustedSizes[1], 1);
    // If the mask file has a single channel, create a 3-channel mask
    // image
    if (this->BWMask)
    {
      auto appendFilter = vtkSmartPointer<vtkImageAppendComponents>::New();
      appendFilter->SetInputConnection(0, resize->GetOutputPort());
      appendFilter->AddInputConnection(0, resize->GetOutputPort());
      appendFilter->AddInputConnection(0, resize->GetOutputPort());
      appendFilter->Update();
      maskImage = appendFilter->GetOutput();
    }
    else
    {
      auto rgbImage = vtkSmartPointer<vtkImageExtractComponents>::New();
      rgbImage->SetInputConnection(resize->GetOutputPort());
      rgbImage->SetComponents(0, 1, 2);
      rgbImage->Update();
      maskImage = rgbImage->GetOutput();
    }
  }
  // Create an image that will hold the final image
  auto final = vtkSmartPointer<vtkImageBlend>::New();
  final->AddInputData(maskImage);
  final->SetOpacity(0, .5);
  final->Update();

  // Initialize error message
  std::string errorMessage;

  // Try to add each word
  this->GetSkippedWords().resize(0);

  bool added;
  // Create a vector of orientations to try.
  std::mt19937_64 mt(4355412); // Standard mersenne twister engine
  for (auto element : sortedWords)
  {
    std::vector<double> orientations;
    // If discrete orientations are present use them, otherwise
    // generate random orientations
    if (!this->Orientations.empty())
    {
      orientations = this->Orientations;
    }
    else
    {
      std::uniform_real_distribution<> orientationDist(
        this->OrientationDistribution[0], this->OrientationDistribution[1]);
      orientations.push_back(orientationDist(mt));
    }
    std::shuffle(std::begin(orientations), std::end(orientations), mt);
    for (auto o : orientations)
    {
      added =
        ::AddWordToFinal(this, element.first, element.second, mt, o, offset, final, errorMessage);
      if (!errorMessage.empty())
      {
        vtkErrorMacro(<< errorMessage);
        return 0;
      }
      if (added)
      {
        this->GetKeptWords().push_back(element.first);
        break;
      }
      else
      {
        std::string skippedWord;
        skippedWord.resize((element.first).size());
        std::transform(
          (element.first).begin(), (element.first).end(), skippedWord.begin(), ::tolower);

        this->GetSkippedWords().push_back(skippedWord);
      }
    }
  }

  // If a mack file exists, replace the maskColor with the background color
  ::ReplaceMaskColorWithBackgroundColor(final->GetOutput(), this);

  output->DeepCopy(final->GetOutput());

  // Remove duplicates in generated word vectors.
  std::sort(this->StoppedWords.begin(), this->StoppedWords.end());
  this->StoppedWords.erase(
    std::unique(this->StoppedWords.begin(), this->StoppedWords.end()), this->StoppedWords.end());
  std::sort(this->SkippedWords.begin(), this->SkippedWords.end());
  this->SkippedWords.erase(
    std::unique(this->SkippedWords.begin(), this->SkippedWords.end()), this->SkippedWords.end());
  std::sort(this->KeptWords.begin(), this->KeptWords.end());
  this->KeptWords.erase(
    std::unique(this->KeptWords.begin(), this->KeptWords.end()), this->KeptWords.end());

  return 1;
}

//------------------------------------------------------------------------------
void vtkWordCloud::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << "  BackgroundColorName: " << this->GetBackgroundColorName() << std::endl;
  os << "  BWMask: " << (this->GetBWMask() ? "true" : "false") << std::endl;
  os << "  ColorDistribution: " << this->GetColorDistribution()[0] << " "
     << this->GetColorDistribution()[1] << std::endl;
  os << "  ColorSchemeName: " << this->GetColorSchemeName() << std::endl;
  os << "  DPI: " << this->GetDPI() << std::endl;
  os << "  FontFileName: " << this->GetFontFileName() << std::endl;
  os << "  FontMultiplier: " << this->GetFontMultiplier() << std::endl;
  os << "  Gap: " << this->GetGap() << std::endl;
  os << "  MaskColorName: " << this->GetMaskColorName() << std::endl;
  os << "  MaskFileName: " << this->GetMaskFileName() << std::endl;
  os << "  MinFontSize: " << this->GetMinFontSize() << std::endl;
  os << "  MaxFontSize: " << this->GetMaxFontSize() << std::endl;
  os << "  MinFrequency: " << this->GetMinFrequency() << std::endl;
  os << "  OffsetDistribution: " << this->GetOffsetDistribution()[0] << " "
     << this->GetOffsetDistribution()[1] << std::endl;
  os << "  OrientationDistribution: " << this->GetOrientationDistribution()[0] << " "
     << this->GetOrientationDistribution()[1] << std::endl;
  os << "  Orientations: ";
  for (auto o : this->Orientations)
  {
    os << o << " ";
  }
  os << std::endl;
  os << "  ReplacementPairs: ";
  for (auto p : this->ReplacementPairs)
  {
    os << std::get<0>(p) << "->" << std::get<1>(p) << " ";
  }
  os << std::endl;
  os << "  Sizes: " << this->GetSizes()[0] << " " << this->GetSizes()[1] << std::endl;
  os << "  StopWords: ";
  for (const auto& s : this->GetStopWords())
  {
    os << s << " ";
  }
  os << std::endl;
  os << "  StopListFileName: " << this->GetStopListFileName() << std::endl;
  os << "  FileName: " << this->GetFileName() << std::endl;
  os << "  Title: " << GetTitle() << std::endl;
  os << "  WordColorName: " << GetWordColorName() << std::endl;
}

namespace
{
std::multiset<std::pair<std::string, int>, Comparator> FindWordsSortedByFrequency(
  std::string& s, vtkWordCloud* wordCloud)
{
  // Create a stop list
  vtkWordCloud::StopWordsContainer stopList;

  // If a StopListFileName is defined, use it, otherwise use the
  // built-in StopList.
  if (!wordCloud->GetStopListFileName().empty())
  {
    CreateStopListFromFile(wordCloud->GetStopListFileName(), stopList);
  }
  else
  {
    CreateBuiltInStopList(stopList);
  }

  // Add user stop words
  for (const auto& stop : wordCloud->GetStopWords())
  {
    stopList.insert(stop);
  }

  // Add replacement pairs to StopList
  ::AddReplacementPairsToStopList(wordCloud, stopList);

  // Drop the case of all words
  std::transform(s.begin(), s.end(), s.begin(), ::tolower);

  // Extract words
  std::vector<std::string> extractedWords;
  ::ExtractWordsFromString(s, extractedWords);

  // Store the words in a map that will contain frequencies
  std::map<std::string, int> wordContainer;

  // If a title is present add it with a high frequency
  if (wordCloud->GetTitle().length() > 0)
  {
    wordContainer[wordCloud->GetTitle()] = 1000;
  }
  const int N = 1;

  for (auto w : extractedWords)
  {
    // Replace words with another
    for (auto p : wordCloud->GetReplacementPairs())
    {
      std::string from = std::get<0>(p);
      std::string to = std::get<1>(p);
      size_t pos = 0;
      pos = w.find(from, pos);
      if (w.length() == from.length() && pos == 0)
      {
        w.replace(pos, to.length(), to);
        stopList.insert(from);
      }
    }

    // Skip the word if it is in the stop list or contains a digit
    auto it = stopList.find(w);
    const auto digit = w.find_first_of("0123456789");
    if (it != stopList.end() || digit != std::string::npos)
    {
      wordCloud->GetStoppedWords().push_back(*it);
      continue;
    }

    // Only include words that have more than N characters
    if (w.size() > N)
    {
      // Raise the case of the first letter in the word
      std::transform(w.begin(), w.begin() + 1, w.begin(), ::toupper);
      wordContainer[w]++;
    }
  }

  // Defining a lambda function to compare two pairs. It will compare
  // two pairs using second field
  Comparator compFunctor = [](const std::pair<std::string, int>& elem1,
                             const std::pair<std::string, int>& elem2) {
    if (elem1.second == elem2.second)
    {
      return elem1.first.length() > elem2.first.length();
    }
    return elem1.second > elem2.second;
  };

  // Declaring a multiset that will store the pairs using above
  // comparison logic
  std::multiset<std::pair<std::string, int>, Comparator> setOfWords(
    wordContainer.begin(), wordContainer.end(), compFunctor);
  return setOfWords;
}

void AddReplacementPairsToStopList(
  vtkWordCloud* wordCloud, vtkWordCloud::StopWordsContainer& stopList)
{
  for (auto p : wordCloud->GetReplacementPairs())
  {
    std::string from = std::get<0>(p);
    std::string to = std::get<1>(p);
    stopList.insert(from);

    // The replacement may contain multiple strings and may have upper
    // case letters
    std::transform(to.begin(), to.end(), to.begin(), ::tolower);
    // The to string may have more thn one word
    std::vector<std::string> words;
    ::ExtractWordsFromString(to, words);

    // Add each replacement to the stop list
    for (const auto& w : words)
    {
      stopList.insert(w);
    }
  }
}

bool AddWordToFinal(vtkWordCloud* wordCloud, std::string word, int frequency, std::mt19937_64& mt,
  double orientation, std::vector<ExtentOffset>& offset, vtkImageBlend* final,
  std::string& errorMessage)
{
  // Skip words below MinFrequency
  if (frequency < wordCloud->GetMinFrequency())
  {
    return false;
  }

  // Create random distributions
  std::uniform_real_distribution<> colorDist(
    wordCloud->GetColorDistribution()[0], wordCloud->GetColorDistribution()[1]);

  // Setup a property for the strings containing fixed parameters
  auto colors = vtkSmartPointer<vtkNamedColors>::New();
  auto textProperty = vtkSmartPointer<vtkTextProperty>::New();
  if (wordCloud->GetWordColorName().length() > 0)
  {
    textProperty->SetColor(colors->GetColor3d(wordCloud->GetWordColorName()).GetData());
  }
  else if (wordCloud->GetColorSchemeName().length() > 0)
  {
    auto colorScheme = vtkSmartPointer<vtkColorSeries>::New();
    colorScheme->SetColorSchemeByName(wordCloud->GetColorSchemeName());
    vtkColor3ub color =
      colorScheme->GetColorRepeating(static_cast<int>(wordCloud->GetKeptWords().size()));
    if (color.Compare(colors->GetColor3ub("black"), 1) && wordCloud->GetKeptWords().empty())
    {
      std::ostringstream validNames;
      ShowColorSeriesNames(validNames);
      std::ostringstream message;
      message << "The color scheme " << wordCloud->GetColorSchemeName() << " does not exist.\n"
              << validNames.str();
      errorMessage = message.str();
    }
    textProperty->SetColor(static_cast<double>(color.GetRed()) / 255.0,
      static_cast<double>(color.GetGreen()) / 255.0, static_cast<double>(color.GetBlue()) / 255.0);
  }
  else
  {
    textProperty->SetColor(colorDist(mt), colorDist(mt), colorDist(mt));
  }
  textProperty->SetVerticalJustificationToCentered();
  textProperty->SetJustificationToCentered();
  textProperty->SetLineOffset(4);

  // Check if a font file is present
  if (wordCloud->GetFontFileName().length() > 0)
  {
    textProperty->SetFontFile(wordCloud->GetFontFileName().c_str());
    textProperty->SetFontFamily(VTK_FONT_FILE);
  }
  else
  {
    textProperty->SetFontFamilyToArial();
  }

  // Set the font size
  int fontSize = wordCloud->GetFontMultiplier() * frequency;
  if (fontSize > wordCloud->GetMaxFontSize())
  {
    fontSize = wordCloud->GetMaxFontSize();
  }
  if (fontSize < wordCloud->GetMinFontSize())
  {
    fontSize = wordCloud->GetMinFontSize();
  }
  if (frequency == 1000)
  {
    fontSize *= 1.2;
  }
  textProperty->SetFontSize(fontSize);
  textProperty->SetOrientation(orientation);

  // Add gap
  std::string spaces;
  for (int p = 0; p < wordCloud->GetGap(); ++p)
  {
    spaces.push_back(' ');
  }

  // For each string, create an image and see if it overlaps with other images,
  // if so, skip it
  // Create an image of the string
  vtkFreeTypeTools* freeType = vtkFreeTypeTools::GetInstance();
  freeType->ScaleToPowerTwoOff();

  auto textImage = vtkSmartPointer<vtkImageData>::New();
  freeType->RenderString(
    textProperty, spaces + word + spaces, wordCloud->GetDPI(), textImage.GetPointer());

  // Set the extent of the text image
  std::array<int, 4> bb;
  freeType->GetBoundingBox(textProperty, spaces + word + spaces, wordCloud->GetDPI(), bb.data());
  vtkColor3ub maskColor = colors->GetColor3ub(wordCloud->GetMaskColorName().c_str());
  unsigned char maskR = maskColor.GetData()[0];
  unsigned char maskG = maskColor.GetData()[1];
  unsigned char maskB = maskColor.GetData()[2];

  std::uniform_real_distribution<> offsetDist(
    wordCloud->GetOffsetDistribution()[0], wordCloud->GetOffsetDistribution()[1]);

  for (auto of : offset)
  {
    int offsetX = of.x + offsetDist(mt); // add some noise to the offset
    int offsetY = of.y + offsetDist(mt);
    // Make sure the text image will fit on the final image
    if (offsetX + bb[1] - bb[0] < wordCloud->GetAdjustedSizes()[0] - 1 &&
      offsetY + bb[3] - bb[2] < wordCloud->GetAdjustedSizes()[1] - 1 && offsetX >= 0 &&
      offsetY >= 0)
    {
      textImage->SetExtent(
        offsetX, offsetX + bb[1] - bb[0], offsetY, offsetY + bb[3] - bb[2], 0, 0);
      auto image = vtkSmartPointer<vtkImageData>::New();
      final->Update();

      // Does the text image overlap with images on the final image
      vtkImageIterator<unsigned char> finalIt(final->GetOutput(), textImage->GetExtent());
      bool good = true;
      while (!finalIt.IsAtEnd())
      {
        auto finalSpan = finalIt.BeginSpan();
        while (finalSpan != finalIt.EndSpan())
        {
          unsigned char R, G, B;
          R = *finalSpan++;
          G = *finalSpan++;
          B = *finalSpan++;
          // If the pixel does not contain the background color, the word will not fit
          if (R != maskR && G != maskG && B != maskB)
          {
            good = false;
            break;
          }
        }
        if (!good)
        {
          break;
        }
        finalIt.NextSpan();
      }
      if (good)
      {
        final->AddInputData(textImage);
        final->Update();
        return true;
      }
    }
  }
  return false;
}

void ArchimedesSpiral(std::vector<ExtentOffset>& offset, vtkWordCloud::SizesContainer& sizes)
{
  const int centerX = sizes[0] / 2.0;
  const int centerY = sizes[1] / 2.0;

  const std::size_t N = 10000;
  constexpr auto pi = 3.141592653589793238462643383279502884L; /* pi */
  const double deltaAngle = pi * 20 / N;
  double maxX = -1000.0;
  double minX = 1000.0;
  double maxY = -1000.0;
  double minY = 1000.0;
  double range = -1000;
  double e = sizes[0] / sizes[1];
  std::vector<ArchimedesValue> archimedes;
  for (std::size_t i = 0; i < N; i += 10)
  {
    double x, y;
    double angle = deltaAngle * i;
    x = e * angle * std::cos(angle);
    y = e * angle * std::sin(angle);
    archimedes.emplace_back(x, y);
    maxX = std::max(maxX, x);
    minX = std::min(minX, x);
    maxY = std::max(maxY, y);
    minY = std::min(minY, y);
    range = std::max(maxX - minX, maxY - minY);
  }
  double scaleX = 1.0 / range * sizes[0];
  for (auto ar : archimedes)
  {
    if (ar.x * scaleX + centerX - 50 < 0 || ar.y * scaleX + centerY < 0)
      continue;
    offset.emplace_back(ar.x * scaleX + centerX - 50, ar.y * scaleX + centerY);
  }
}
void ReplaceMaskColorWithBackgroundColor(vtkImageData* finalImage, vtkWordCloud* wordCloud)
{
  auto colors = vtkSmartPointer<vtkNamedColors>::New();

  vtkColor3ub backgroundColor = colors->GetColor3ub(wordCloud->GetBackgroundColorName().c_str());
  unsigned char bkgR = backgroundColor.GetData()[0];
  unsigned char bkgG = backgroundColor.GetData()[1];
  unsigned char bkgB = backgroundColor.GetData()[2];

  vtkColor3ub maskColor = colors->GetColor3ub(wordCloud->GetMaskColorName().c_str());
  unsigned char maskR = maskColor.GetData()[0];
  unsigned char maskG = maskColor.GetData()[1];
  unsigned char maskB = maskColor.GetData()[2];

  vtkImageIterator<unsigned char> finalIt(finalImage, finalImage->GetExtent());
  while (!finalIt.IsAtEnd())
  {
    auto finalSpan = finalIt.BeginSpan();
    while (finalSpan != finalIt.EndSpan())
    {
      unsigned char R, G, B;
      R = *finalSpan;
      G = *(finalSpan + 1);
      B = *(finalSpan + 2);
      // If the pixel does not contain the background color, skip
      // it. Otherwise replace it with the background color.
      if (R != maskR && G != maskG && B != maskB)
      {
        finalSpan += 3;
        continue;
      }
      else
      {
        *finalSpan = bkgR;
        *(finalSpan + 1) = bkgG;
        *(finalSpan + 2) = bkgB;
      }
      finalSpan += 3;
    }
    finalIt.NextSpan();
  }
}

void ShowColorSeriesNames(ostream& os)
{
  auto colorSeries = vtkSmartPointer<vtkColorSeries>::New();
  os << "Valid schemes" << std::endl;
  for (auto i = 0; i < colorSeries->GetNumberOfColorSchemes(); ++i)
  {
    colorSeries->SetColorScheme(i);
    os << "  " << colorSeries->GetColorSchemeName() << std::endl;
  }
}

void CreateStopListFromFile(std::string fileName, vtkWordCloud::StopWordsContainer& stopList)
{
  vtksys::ifstream t(fileName.c_str());
  std::stringstream buffer;
  buffer << t.rdbuf();
  std::string s = buffer.str();
  t.close();

  // Extract words
  std::vector<std::string> words;
  ::ExtractWordsFromString(s, words);
  for (const auto& w : words)
  {
    stopList.insert(w);
  }
}

void ExtractWordsFromString(std::string& str, std::vector<std::string>& words)
{
#ifdef HAS_STD_REGEX
  std::regex wordRegex("(\\w+)");
  auto wordsBegin = std::sregex_iterator(str.begin(), str.end(), wordRegex);
  auto wordsEnd = std::sregex_iterator();
  for (auto i = wordsBegin; i != wordsEnd; ++i)
  {
    words.push_back((*i).str());
  }
#else
  vtksys::RegularExpression re("([0-9A-Za-z]+)");
  size_t next = 0;
  while (re.find(str.substr(next)))
  {
    words.push_back(str.substr(next + re.start(), re.end() - re.start()));
    next += re.end();
  }
#endif
}

const char* const stop_words[] = { "a", "able", "about", "above", "abst", "accordance", "according",
  "accordingly", "across", "act", "actually", "added", "adj", "affected", "affecting", "affects",
  "after", "afterwards", "again", "against", "ah", "all", "almost", "alone", "along", "already",
  "also", "although", "always", "am", "among", "amongst", "an", "and", "announce", "another", "any",
  "anybody", "anyhow", "anymore", "anyone", "anything", "anyway", "anyways", "anywhere",
  "apparently", "approximately", "are", "aren", "arent", "arise", "around", "as", "aside", "ask",
  "asking", "at", "auth", "available", "away", "awfully", "b", "back", "be", "became", "because",
  "become", "becomes", "becoming", "been", "before", "beforehand", "begin", "beginning",
  "beginnings", "begins", "behind", "being", "believe", "below", "beside", "besides", "between",
  "beyond", "biol", "both", "brief", "briefly", "but", "by", "c", "ca", "came", "can", "cannot",
  "can't", "cause", "causes", "certain", "certainly", "co", "com", "come", "comes", "contain",
  "containing", "contains", "could", "couldnt", "cum", "d", "date", "did", "didn't", "different",
  "do", "does", "doesn't", "doing", "done", "don't", "down", "downwards", "due", "dr", "during",
  "e", "each", "ed", "edu", "effect", "eg", "eight", "eighty", "either", "else", "elsewhere", "end",
  "ending", "enough", "especially", "et", "et-al", "etc", "even", "ever", "every", "everybody",
  "everyone", "everything", "everywhere", "ex", "except", "f", "far", "few", "ff", "fifth", "first",
  "five", "fix", "followed", "following", "follows", "for", "former", "formerly", "forth", "found",
  "four", "from", "further", "furthermore", "g", "gave", "get", "gets", "getting", "give", "given",
  "gives", "giving", "go", "goes", "gone", "got", "gotten", "h", "had", "happens", "hardly", "has",
  "hasn", "have", "haven", "having", "he", "hed", "hence", "her", "here", "hereafter", "hereby",
  "herein", "heres", "hereupon", "hers", "herself", "hes", "hi", "hid", "him", "himself", "his",
  "hither", "home", "how", "howbeit", "however", "hundred", "i", "id", "ie", "if", "im",
  "immediate", "immediately", "importance", "important", "in", "inc", "indeed", "index",
  "information", "instead", "into", "invention", "inward", "is", "isn", "it", "itd", "it", "its",
  "itself", "j", "jr", "just", "k", "keep", "keeps", "kept", "kg", "km", "know", "known", "knows",
  "l", "largely", "last", "lately", "later", "latter", "latterly", "laude", "least", "less", "lest",
  "let", "lets", "like", "liked", "likely", "line", "little", "ll", "look", "looking", "looks",
  "ltd", "m", "made", "mainly", "make", "makes", "many", "may", "maybe", "me", "mean", "means",
  "meantime", "meanwhile", "merely", "met", "mg", "mic", "might", "million", "miss", "ml", "more",
  "moreover", "most", "mostly", "mr", "mrs", "much", "mug", "must", "my", "myself", "n", "na",
  "name", "namely", "nay", "nd", "near", "nearly", "necessarily", "necessary", "need", "needs",
  "neither", "never", "nevertheless", "new", "next", "nine", "ninety", "no", "nobody", "non",
  "none", "nonetheless", "noone", "nor", "normally", "nos", "not", "noted", "nothing", "now",
  "nowhere", "o", "obtain", "obtained", "obviously", "of", "off", "often", "oh", "ok", "okay",
  "old", "omitted", "on", "once", "one", "ones", "only", "onto", "or", "ord", "org", "other",
  "others", "otherwise", "ought", "our", "ours", "ourselves", "out", "outside", "over", "overall",
  "owing", "own", "p", "page", "pages", "part", "particular", "particularly", "past", "per",
  "perhaps", "ph", "placed", "please", "plus", "poorly", "possible", "possibly", "potentially",
  "pp", "predominantly", "present", "previously", "primarily", "probably", "promptly", "proud",
  "provides", "put", "q", "que", "quickly", "quite", "qv", "r", "ran", "rather", "rd", "re",
  "readily", "really", "recent", "recently", "ref", "refs", "regarding", "regardless", "regards",
  "related", "relatively", "research", "respectively", "resulted", "resulting", "results", "right",
  "run", "s", "said", "same", "saw", "sat", "say", "saying", "says", "sec", "section", "see",
  "seeing", "seem", "seemed", "seeming", "seems", "seen", "self", "selves", "sent", "seven",
  "several", "shall", "she", "shed", "shes", "should", "shouldn", "show", "showed", "shown",
  "showns", "shows", "significant", "significantly", "similar", "similarly", "since", "six",
  "slightly", "so", "some", "somebody", "somehow", "someone", "somethan", "something", "sometime",
  "sometimes", "somewhat", "somewhere", "soon", "sorry", "specifically", "specified", "specify",
  "specifying", "still", "stop", "strongly", "sub", "substantially", "successfully", "such",
  "sufficiently", "suggest", "sup", "sure", "t", "take", "taken", "taking", "tell", "tends", "th",
  "than", "thank", "thanks", "thanx", "that", "thats", "the", "their", "theirs", "them",
  "themselves", "then", "thence", "there", "thereafter", "thereby", "thered", "therefore",
  "therein", "thereof", "therere", "theres", "thereto", "thereupon", "these", "they", "theyd",
  "theyre", "think", "this", "those", "thou", "though", "thoughh", "thousand", "throug", "through",
  "throughout", "thru", "thus", "til", "tip", "to", "together", "too", "took", "toward", "towards",
  "tried", "tries", "truly", "try", "trying", "ts", "twice", "two", "u", "un", "under",
  "unfortunately", "unless", "unlike", "unlikely", "until", "unto", "up", "upon", "ups", "us",
  "use", "used", "useful", "usefully", "usefulness", "uses", "using", "usually", "v", "value",
  "various", "ve", "very", "via", "viz", "vol", "vols", "vs", "w", "want", "wants", "was", "wasnt",
  "wasnt", "way", "we", "wed", "welcome", "went", "were", "werent", "what", "whatever", "whats",
  "when", "whence", "whenever", "where", "whereafter", "whereas", "whereby", "wherein", "wheres",
  "whereupon", "wherever", "whether", "which", "while", "whim", "whither", "who", "whod", "whoever",
  "whole", "whom", "whomever", "whos", "whose", "why", "widely", "will", "willing", "wish", "with",
  "within", "without", "wont", "words", "world", "would", "wouldnt", "www", "x", "y", "yes", "yet",
  "you", "youd", "your", "youre", "yours", "yourself", "yourselves", "z", "zero" };

void CreateBuiltInStopList(vtkWordCloud::StopWordsContainer& stopList)
{
  for (auto* word : stop_words)
  {
    stopList.insert(word);
  }
}
}

//  LocalWords:  FontFileName
VTK_ABI_NAMESPACE_END
