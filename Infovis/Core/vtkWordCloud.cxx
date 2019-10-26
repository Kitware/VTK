/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWordCloud.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
bool AddWordToFinal(vtkWordCloud*, const std::string, const int, std::mt19937_64&,
  double orientation, std::vector<ExtentOffset>&, vtkImageBlend*, std::string&);

void ArchimedesSpiral(std::vector<ExtentOffset>&, vtkWordCloud::SizesContainer&);
void ReplaceMaskColorWithBackgroundColor(vtkImageData*, vtkWordCloud*);
void CreateBuiltInStopList(vtkWordCloud::StopWordsContainer& StopList);
void CreateStopListFromFile(std::string, vtkWordCloud::StopWordsContainer&);
void ExtractWordsFromString(std::string& str, std::vector<std::string>& words);
void ShowColorSeriesNames(ostream& os);
}

//----------------------------------------------------------------------------
vtkWordCloud::vtkWordCloud()
  : BackgroundColorName("MidnightBlue")
  , BWMask(false)
  , ColorDistribution({ { .6, 1.0 } })
  , ColorSchemeName("")
  , DPI(200)
  , FileName("")
  , FontFileName("")
  , FontMultiplier(6)
  , Gap(2)
  , MaskColorName("black")
  , MaskFileName("")
  , MaxFontSize(48)
  , MinFontSize(12)
  , MinFrequency(1)
  , OrientationDistribution({ { -20, 20 } })
  , Sizes({ { 640, 480 } })
  , StopListFileName("")
  , Title("")
  , WordColorName("")
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

//--------------------------------------------------------------------------
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

//--------------------------------------------------------------------------
int vtkWordCloud::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // Get the data object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkImageData* output = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Check some parameters before we start
  if (this->FileName.size() == 0)
  {
    vtkErrorMacro(<< "No FileName is set. Use SetFileName to set a file.");
    return 0;
  }
  if (!vtksys::SystemTools::FileExists(this->FileName, true))
  {
    vtkErrorMacro(<< "FileName " << this->FileName << " does not exist");
    return 0;
  }
  if (this->FontFileName.size() > 0 && !vtksys::SystemTools::FileExists(this->FontFileName, true))
  {
    vtkErrorMacro(<< "FontFileName " << this->FontFileName << " does not exist");
    return 0;
  }
  if (this->MaskFileName.size() > 0 && !vtksys::SystemTools::FileExists(this->MaskFileName, true))
  {
    vtkErrorMacro(<< "MaskFileName " << this->MaskFileName << " does not exist");
    return 0;
  }
  if (this->StopListFileName.size() > 0 &&
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
  std::vector< ::ExtentOffset> offset;
  ::ArchimedesSpiral(offset, this->Sizes);

  // Sort the word by frequency
  std::multiset<std::pair<std::string, int>, ::Comparator> sortedWords =
    ::FindWordsSortedByFrequency(s, this);

  // Create a mask image
  auto colors = vtkSmartPointer<vtkNamedColors>::New();
  vtkColor3ub maskColor = colors->GetColor3ub(this->MaskColorName.c_str());
  auto maskImage = vtkSmartPointer<vtkImageData>::New();

  // If a mask file is not defined, create a rectangular
  if (this->MaskFileName == "")
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
    if (this->Orientations.size() != 0)
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
      if (errorMessage.size() != 0)
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

//--------------------------------------------------------------------------
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
  for (auto s : this->GetStopWords())
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
  if (wordCloud->GetStopListFileName().size() > 0)
  {
    CreateStopListFromFile(wordCloud->GetStopListFileName(), stopList);
  }
  else
  {
    CreateBuiltInStopList(stopList);
  }

  // Add user stop words
  for (auto stop : wordCloud->GetStopWords())
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
    for (auto w : words)
    {
      stopList.insert(w);
    }
  }
}

bool AddWordToFinal(vtkWordCloud* wordCloud, const std::string word, const int frequency,
  std::mt19937_64& mt, double orientation, std::vector<ExtentOffset>& offset, vtkImageBlend* final,
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
    if (color.Compare(colors->GetColor3ub("black"), 1) && wordCloud->GetKeptWords().size() == 0)
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
    ;
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
    archimedes.push_back(ArchimedesValue(x, y));
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
    offset.push_back(ExtentOffset(ar.x * scaleX + centerX - 50, ar.y * scaleX + centerY));
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
  for (auto w : words)
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
  return;
#else
  vtksys::RegularExpression re("([0-9A-Za-z]+)");
  size_t next = 0;
  while (re.find(str.substr(next)))
  {
    words.push_back(str.substr(next + re.start(), re.end() - re.start()));
    next += re.end();
  }
  return;
#endif
}

void CreateBuiltInStopList(vtkWordCloud::StopWordsContainer& stopList)
{
  stopList.insert("a");
  stopList.insert("able");
  stopList.insert("about");
  stopList.insert("above");
  stopList.insert("abst");
  stopList.insert("accordance");
  stopList.insert("according");
  stopList.insert("accordingly");
  stopList.insert("across");
  stopList.insert("act");
  stopList.insert("actually");
  stopList.insert("added");
  stopList.insert("adj");
  stopList.insert("affected");
  stopList.insert("affecting");
  stopList.insert("affects");
  stopList.insert("after");
  stopList.insert("afterwards");
  stopList.insert("again");
  stopList.insert("against");
  stopList.insert("ah");
  stopList.insert("all");
  stopList.insert("almost");
  stopList.insert("alone");
  stopList.insert("along");
  stopList.insert("already");
  stopList.insert("also");
  stopList.insert("although");
  stopList.insert("always");
  stopList.insert("am");
  stopList.insert("among");
  stopList.insert("amongst");
  stopList.insert("an");
  stopList.insert("and");
  stopList.insert("announce");
  stopList.insert("another");
  stopList.insert("any");
  stopList.insert("anybody");
  stopList.insert("anyhow");
  stopList.insert("anymore");
  stopList.insert("anyone");
  stopList.insert("anything");
  stopList.insert("anyway");
  stopList.insert("anyways");
  stopList.insert("anywhere");
  stopList.insert("apparently");
  stopList.insert("approximately");
  stopList.insert("are");
  stopList.insert("aren");
  stopList.insert("arent");
  stopList.insert("arise");
  stopList.insert("around");
  stopList.insert("as");
  stopList.insert("aside");
  stopList.insert("ask");
  stopList.insert("asking");
  stopList.insert("at");
  stopList.insert("auth");
  stopList.insert("available");
  stopList.insert("away");
  stopList.insert("awfully");
  stopList.insert("b");
  stopList.insert("back");
  stopList.insert("be");
  stopList.insert("became");
  stopList.insert("because");
  stopList.insert("become");
  stopList.insert("becomes");
  stopList.insert("becoming");
  stopList.insert("been");
  stopList.insert("before");
  stopList.insert("beforehand");
  stopList.insert("begin");
  stopList.insert("beginning");
  stopList.insert("beginnings");
  stopList.insert("begins");
  stopList.insert("behind");
  stopList.insert("being");
  stopList.insert("believe");
  stopList.insert("below");
  stopList.insert("beside");
  stopList.insert("besides");
  stopList.insert("between");
  stopList.insert("beyond");
  stopList.insert("biol");
  stopList.insert("both");
  stopList.insert("brief");
  stopList.insert("briefly");
  stopList.insert("but");
  stopList.insert("by");
  stopList.insert("c");
  stopList.insert("ca");
  stopList.insert("came");
  stopList.insert("can");
  stopList.insert("cannot");
  stopList.insert("can't");
  stopList.insert("cause");
  stopList.insert("causes");
  stopList.insert("certain");
  stopList.insert("certainly");
  stopList.insert("co");
  stopList.insert("com");
  stopList.insert("come");
  stopList.insert("comes");
  stopList.insert("contain");
  stopList.insert("containing");
  stopList.insert("contains");
  stopList.insert("could");
  stopList.insert("couldnt");
  stopList.insert("cum");
  stopList.insert("d");
  stopList.insert("date");
  stopList.insert("did");
  stopList.insert("didn't");
  stopList.insert("different");
  stopList.insert("do");
  stopList.insert("does");
  stopList.insert("doesn't");
  stopList.insert("doing");
  stopList.insert("done");
  stopList.insert("don't");
  stopList.insert("down");
  stopList.insert("downwards");
  stopList.insert("due");
  stopList.insert("dr");
  stopList.insert("during");
  stopList.insert("e");
  stopList.insert("each");
  stopList.insert("ed");
  stopList.insert("edu");
  stopList.insert("effect");
  stopList.insert("eg");
  stopList.insert("eight");
  stopList.insert("eighty");
  stopList.insert("either");
  stopList.insert("else");
  stopList.insert("elsewhere");
  stopList.insert("end");
  stopList.insert("ending");
  stopList.insert("enough");
  stopList.insert("especially");
  stopList.insert("et");
  stopList.insert("et-al");
  stopList.insert("etc");
  stopList.insert("even");
  stopList.insert("ever");
  stopList.insert("every");
  stopList.insert("everybody");
  stopList.insert("everyone");
  stopList.insert("everything");
  stopList.insert("everywhere");
  stopList.insert("ex");
  stopList.insert("except");
  stopList.insert("f");
  stopList.insert("far");
  stopList.insert("few");
  stopList.insert("ff");
  stopList.insert("fifth");
  stopList.insert("first");
  stopList.insert("five");
  stopList.insert("fix");
  stopList.insert("followed");
  stopList.insert("following");
  stopList.insert("follows");
  stopList.insert("for");
  stopList.insert("former");
  stopList.insert("formerly");
  stopList.insert("forth");
  stopList.insert("found");
  stopList.insert("four");
  stopList.insert("from");
  stopList.insert("further");
  stopList.insert("furthermore");
  stopList.insert("g");
  stopList.insert("gave");
  stopList.insert("get");
  stopList.insert("gets");
  stopList.insert("getting");
  stopList.insert("give");
  stopList.insert("given");
  stopList.insert("gives");
  stopList.insert("giving");
  stopList.insert("go");
  stopList.insert("goes");
  stopList.insert("gone");
  stopList.insert("got");
  stopList.insert("gotten");
  stopList.insert("h");
  stopList.insert("had");
  stopList.insert("happens");
  stopList.insert("hardly");
  stopList.insert("has");
  stopList.insert("hasn");
  stopList.insert("have");
  stopList.insert("haven");
  stopList.insert("having");
  stopList.insert("he");
  stopList.insert("hed");
  stopList.insert("hence");
  stopList.insert("her");
  stopList.insert("here");
  stopList.insert("hereafter");
  stopList.insert("hereby");
  stopList.insert("herein");
  stopList.insert("heres");
  stopList.insert("hereupon");
  stopList.insert("hers");
  stopList.insert("herself");
  stopList.insert("hes");
  stopList.insert("hi");
  stopList.insert("hid");
  stopList.insert("him");
  stopList.insert("himself");
  stopList.insert("his");
  stopList.insert("hither");
  stopList.insert("home");
  stopList.insert("how");
  stopList.insert("howbeit");
  stopList.insert("however");
  stopList.insert("hundred");
  stopList.insert("i");
  stopList.insert("id");
  stopList.insert("ie");
  stopList.insert("if");
  stopList.insert("im");
  stopList.insert("immediate");
  stopList.insert("immediately");
  stopList.insert("importance");
  stopList.insert("important");
  stopList.insert("in");
  stopList.insert("inc");
  stopList.insert("indeed");
  stopList.insert("index");
  stopList.insert("information");
  stopList.insert("instead");
  stopList.insert("into");
  stopList.insert("invention");
  stopList.insert("inward");
  stopList.insert("is");
  stopList.insert("isn");
  stopList.insert("it");
  stopList.insert("itd");
  stopList.insert("it");
  stopList.insert("its");
  stopList.insert("itself");
  stopList.insert("j");
  stopList.insert("jr");
  stopList.insert("just");
  stopList.insert("k");
  stopList.insert("keep");
  stopList.insert("keeps");
  stopList.insert("kept");
  stopList.insert("kg");
  stopList.insert("km");
  stopList.insert("know");
  stopList.insert("known");
  stopList.insert("knows");
  stopList.insert("l");
  stopList.insert("largely");
  stopList.insert("last");
  stopList.insert("lately");
  stopList.insert("later");
  stopList.insert("latter");
  stopList.insert("latterly");
  stopList.insert("laude");
  stopList.insert("least");
  stopList.insert("less");
  stopList.insert("lest");
  stopList.insert("let");
  stopList.insert("lets");
  stopList.insert("like");
  stopList.insert("liked");
  stopList.insert("likely");
  stopList.insert("line");
  stopList.insert("little");
  stopList.insert("ll");
  stopList.insert("look");
  stopList.insert("looking");
  stopList.insert("looks");
  stopList.insert("ltd");
  stopList.insert("m");
  stopList.insert("made");
  stopList.insert("mainly");
  stopList.insert("make");
  stopList.insert("makes");
  stopList.insert("many");
  stopList.insert("may");
  stopList.insert("maybe");
  stopList.insert("me");
  stopList.insert("mean");
  stopList.insert("means");
  stopList.insert("meantime");
  stopList.insert("meanwhile");
  stopList.insert("merely");
  stopList.insert("met");
  stopList.insert("mg");
  stopList.insert("mic");
  stopList.insert("might");
  stopList.insert("million");
  stopList.insert("miss");
  stopList.insert("ml");
  stopList.insert("more");
  stopList.insert("moreover");
  stopList.insert("most");
  stopList.insert("mostly");
  stopList.insert("mr");
  stopList.insert("mrs");
  stopList.insert("much");
  stopList.insert("mug");
  stopList.insert("must");
  stopList.insert("my");
  stopList.insert("myself");
  stopList.insert("n");
  stopList.insert("na");
  stopList.insert("name");
  stopList.insert("namely");
  stopList.insert("nay");
  stopList.insert("nd");
  stopList.insert("near");
  stopList.insert("nearly");
  stopList.insert("necessarily");
  stopList.insert("necessary");
  stopList.insert("need");
  stopList.insert("needs");
  stopList.insert("neither");
  stopList.insert("never");
  stopList.insert("nevertheless");
  stopList.insert("new");
  stopList.insert("next");
  stopList.insert("nine");
  stopList.insert("ninety");
  stopList.insert("no");
  stopList.insert("nobody");
  stopList.insert("non");
  stopList.insert("none");
  stopList.insert("nonetheless");
  stopList.insert("noone");
  stopList.insert("nor");
  stopList.insert("normally");
  stopList.insert("nos");
  stopList.insert("not");
  stopList.insert("noted");
  stopList.insert("nothing");
  stopList.insert("now");
  stopList.insert("nowhere");
  stopList.insert("o");
  stopList.insert("obtain");
  stopList.insert("obtained");
  stopList.insert("obviously");
  stopList.insert("of");
  stopList.insert("off");
  stopList.insert("often");
  stopList.insert("oh");
  stopList.insert("ok");
  stopList.insert("okay");
  stopList.insert("old");
  stopList.insert("omitted");
  stopList.insert("on");
  stopList.insert("once");
  stopList.insert("one");
  stopList.insert("ones");
  stopList.insert("only");
  stopList.insert("onto");
  stopList.insert("or");
  stopList.insert("ord");
  stopList.insert("org");
  stopList.insert("other");
  stopList.insert("others");
  stopList.insert("otherwise");
  stopList.insert("ought");
  stopList.insert("our");
  stopList.insert("ours");
  stopList.insert("ourselves");
  stopList.insert("out");
  stopList.insert("outside");
  stopList.insert("over");
  stopList.insert("overall");
  stopList.insert("owing");
  stopList.insert("own");
  stopList.insert("p");
  stopList.insert("page");
  stopList.insert("pages");
  stopList.insert("part");
  stopList.insert("particular");
  stopList.insert("particularly");
  stopList.insert("past");
  stopList.insert("per");
  stopList.insert("perhaps");
  stopList.insert("ph");
  stopList.insert("placed");
  stopList.insert("please");
  stopList.insert("plus");
  stopList.insert("poorly");
  stopList.insert("possible");
  stopList.insert("possibly");
  stopList.insert("potentially");
  stopList.insert("pp");
  stopList.insert("predominantly");
  stopList.insert("present");
  stopList.insert("previously");
  stopList.insert("primarily");
  stopList.insert("probably");
  stopList.insert("promptly");
  stopList.insert("proud");
  stopList.insert("provides");
  stopList.insert("put");
  stopList.insert("q");
  stopList.insert("que");
  stopList.insert("quickly");
  stopList.insert("quite");
  stopList.insert("qv");
  stopList.insert("r");
  stopList.insert("ran");
  stopList.insert("rather");
  stopList.insert("rd");
  stopList.insert("re");
  stopList.insert("readily");
  stopList.insert("really");
  stopList.insert("recent");
  stopList.insert("recently");
  stopList.insert("ref");
  stopList.insert("refs");
  stopList.insert("regarding");
  stopList.insert("regardless");
  stopList.insert("regards");
  stopList.insert("related");
  stopList.insert("relatively");
  stopList.insert("research");
  stopList.insert("respectively");
  stopList.insert("resulted");
  stopList.insert("resulting");
  stopList.insert("results");
  stopList.insert("right");
  stopList.insert("run");
  stopList.insert("s");
  stopList.insert("said");
  stopList.insert("same");
  stopList.insert("saw");
  stopList.insert("sat");
  stopList.insert("say");
  stopList.insert("saying");
  stopList.insert("says");
  stopList.insert("sec");
  stopList.insert("section");
  stopList.insert("see");
  stopList.insert("seeing");
  stopList.insert("seem");
  stopList.insert("seemed");
  stopList.insert("seeming");
  stopList.insert("seems");
  stopList.insert("seen");
  stopList.insert("self");
  stopList.insert("selves");
  stopList.insert("sent");
  stopList.insert("seven");
  stopList.insert("several");
  stopList.insert("shall");
  stopList.insert("she");
  stopList.insert("shed");
  stopList.insert("shes");
  stopList.insert("should");
  stopList.insert("shouldn");
  stopList.insert("show");
  stopList.insert("showed");
  stopList.insert("shown");
  stopList.insert("showns");
  stopList.insert("shows");
  stopList.insert("significant");
  stopList.insert("significantly");
  stopList.insert("similar");
  stopList.insert("similarly");
  stopList.insert("since");
  stopList.insert("six");
  stopList.insert("slightly");
  stopList.insert("so");
  stopList.insert("some");
  stopList.insert("somebody");
  stopList.insert("somehow");
  stopList.insert("someone");
  stopList.insert("somethan");
  stopList.insert("something");
  stopList.insert("sometime");
  stopList.insert("sometimes");
  stopList.insert("somewhat");
  stopList.insert("somewhere");
  stopList.insert("soon");
  stopList.insert("sorry");
  stopList.insert("specifically");
  stopList.insert("specified");
  stopList.insert("specify");
  stopList.insert("specifying");
  stopList.insert("still");
  stopList.insert("stop");
  stopList.insert("strongly");
  stopList.insert("sub");
  stopList.insert("substantially");
  stopList.insert("successfully");
  stopList.insert("such");
  stopList.insert("sufficiently");
  stopList.insert("suggest");
  stopList.insert("sup");
  stopList.insert("sure");
  stopList.insert("t");
  stopList.insert("take");
  stopList.insert("taken");
  stopList.insert("taking");
  stopList.insert("tell");
  stopList.insert("tends");
  stopList.insert("th");
  stopList.insert("than");
  stopList.insert("thank");
  stopList.insert("thanks");
  stopList.insert("thanx");
  stopList.insert("that");
  stopList.insert("thats");
  stopList.insert("the");
  stopList.insert("their");
  stopList.insert("theirs");
  stopList.insert("them");
  stopList.insert("themselves");
  stopList.insert("then");
  stopList.insert("thence");
  stopList.insert("there");
  stopList.insert("thereafter");
  stopList.insert("thereby");
  stopList.insert("thered");
  stopList.insert("therefore");
  stopList.insert("therein");
  stopList.insert("thereof");
  stopList.insert("therere");
  stopList.insert("theres");
  stopList.insert("thereto");
  stopList.insert("thereupon");
  stopList.insert("these");
  stopList.insert("they");
  stopList.insert("theyd");
  stopList.insert("theyre");
  stopList.insert("think");
  stopList.insert("this");
  stopList.insert("those");
  stopList.insert("thou");
  stopList.insert("though");
  stopList.insert("thoughh");
  stopList.insert("thousand");
  stopList.insert("throug");
  stopList.insert("through");
  stopList.insert("throughout");
  stopList.insert("thru");
  stopList.insert("thus");
  stopList.insert("til");
  stopList.insert("tip");
  stopList.insert("to");
  stopList.insert("together");
  stopList.insert("too");
  stopList.insert("took");
  stopList.insert("toward");
  stopList.insert("towards");
  stopList.insert("tried");
  stopList.insert("tries");
  stopList.insert("truly");
  stopList.insert("try");
  stopList.insert("trying");
  stopList.insert("ts");
  stopList.insert("twice");
  stopList.insert("two");
  stopList.insert("u");
  stopList.insert("un");
  stopList.insert("under");
  stopList.insert("unfortunately");
  stopList.insert("unless");
  stopList.insert("unlike");
  stopList.insert("unlikely");
  stopList.insert("until");
  stopList.insert("unto");
  stopList.insert("up");
  stopList.insert("upon");
  stopList.insert("ups");
  stopList.insert("us");
  stopList.insert("use");
  stopList.insert("used");
  stopList.insert("useful");
  stopList.insert("usefully");
  stopList.insert("usefulness");
  stopList.insert("uses");
  stopList.insert("using");
  stopList.insert("usually");
  stopList.insert("v");
  stopList.insert("value");
  stopList.insert("various");
  stopList.insert("ve");
  stopList.insert("very");
  stopList.insert("via");
  stopList.insert("viz");
  stopList.insert("vol");
  stopList.insert("vols");
  stopList.insert("vs");
  stopList.insert("w");
  stopList.insert("want");
  stopList.insert("wants");
  stopList.insert("was");
  stopList.insert("wasnt");
  stopList.insert("wasnt");
  stopList.insert("way");
  stopList.insert("we");
  stopList.insert("wed");
  stopList.insert("welcome");
  stopList.insert("went");
  stopList.insert("were");
  stopList.insert("werent");
  stopList.insert("what");
  stopList.insert("whatever");
  stopList.insert("whats");
  stopList.insert("when");
  stopList.insert("whence");
  stopList.insert("whenever");
  stopList.insert("where");
  stopList.insert("whereafter");
  stopList.insert("whereas");
  stopList.insert("whereby");
  stopList.insert("wherein");
  stopList.insert("wheres");
  stopList.insert("whereupon");
  stopList.insert("wherever");
  stopList.insert("whether");
  stopList.insert("which");
  stopList.insert("while");
  stopList.insert("whim");
  stopList.insert("whither");
  stopList.insert("who");
  stopList.insert("whod");
  stopList.insert("whoever");
  stopList.insert("whole");
  stopList.insert("whom");
  stopList.insert("whomever");
  stopList.insert("whos");
  stopList.insert("whose");
  stopList.insert("why");
  stopList.insert("widely");
  stopList.insert("will");
  stopList.insert("willing");
  stopList.insert("wish");
  stopList.insert("with");
  stopList.insert("within");
  stopList.insert("without");
  stopList.insert("wont");
  stopList.insert("words");
  stopList.insert("world");
  stopList.insert("would");
  stopList.insert("wouldnt");
  stopList.insert("www");
  stopList.insert("x");
  stopList.insert("y");
  stopList.insert("yes");
  stopList.insert("yet");
  stopList.insert("you");
  stopList.insert("youd");
  stopList.insert("your");
  stopList.insert("youre");
  stopList.insert("yours");
  stopList.insert("yourself");
  stopList.insert("yourselves");
  stopList.insert("z");
  stopList.insert("zero");
}
}

//  LocalWords:  FontFileName
