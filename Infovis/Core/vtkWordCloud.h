// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWordCloud_h
#define vtkWordCloud_h

#include "vtkImageAlgorithm.h"
#include "vtkImageData.h"         // For ImageData
#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkSmartPointer.h"      // For SmartPointer

#include <array>      // For stl array
#include <functional> // for function
#include <set>        // for stl multiset
#include <string>     // For stl string
#include <vector>     // For stl vector

/**
 * @class   vtkWordCloud
 * @brief   generate a word cloud visualization of a text document
 *
 * Word Clouds, AKA Tag Clouds
 * (https://en.wikipedia.org/wiki/Tag_cloud), are a text visualization
 * technique that displays individual words with properties that
 * depend on the frequency of a word in a document. vtkWordCloud
 * varies the font size based on word frequency. Word Clouds are useful
 * for quickly perceiving the most prominent terms in a document.
 * Also, Word Clouds can identify trends and patterns that would
 * otherwise be unclear or difficult to see in a tabular
 * format. Frequently used keywords stand out better in a word
 * cloud. Common words that might be overlooked in tabular form are
 * highlighted in the larger text, making them pop out when displayed
 * in a word cloud.
 *
 * There is some controversy about the usefulness of word
 * clouds. Their best use may be for presentations, see
 * https://tinyurl.com/y59hy7oa
 *
 * The generation of the word cloud proceeds as follows:
 * 1. Read the text file
 * 2. Split text into words to be processed
 *    Extract words from the text
 *    Drop the case of each word for filtering
 *    Filter the words
 *      Replace words from the ReplacementPairs list
 *      Skip the word if it is in the stop list or contains a digit
 *      Skip single character words
 *    Raise the case of the first letter in each word
 *    Sort the word list by frequency
 * 3. Create a rectangular mask image or read a mask image
 * 4. For each word
 *    Render the word into an image
 *    Try to add the word to the word cloud image.
 *      For each orientation, see if the word "fits"
 *        If no fit, move along a path to try another location
 *
 * NOTE: A word fits if all of the non-zero word cloud pixels in the
 * extent of the text image are background pixels.
 *
 * NOTE: The path is an Archimedean Spiral
 * (https://en.wikipedia.org/wiki/Archimedean_spiral)

 * NOTE: vtkWordCloud has a built-in list of stop word. Stop words are
 * words that are filtered out before processing of the text, such as
 * the, is, at, which, and so on.
 *
 * NOTE: Color names are defined in vtkNamedColors. A visual
 * representation of color names is here: https://tinyurl.com/y3yxcxj6
 *
 * NOTE: vtkWordCloud offers Several methods to customize the resulting
 * visualization. The class provides defaults that provide a reasonable
 * result.
 *
 * BackgroundColorName - The vtkNamedColors name for the background
 * (MidNightBlue). See https://tinyurl.com/y3yxcxj6 for a visual
 * representation of the named colors.
 *
 * ColorDistribution - Distribution of random colors(.6 1.0), if
 * WordColorName is empty.
 *
 * ColorSchemeName - Name of a color scheme from vtkColorSeries to be
 * used to select colors for the words (), if WordColorName is empty.
 * See https://tinyurl.com/y3j6c27o for a visual representation of the
 * color schemes.
 *
 * DPI - Dots per inch(200) of the rendered text. DPI is used as a
 * scaling mechanism for the words. As DPI increases, the word size
 * increases. If there are too, few skipped words, increase this value,
 * too many, decrease it.
 *
 * FontFileName - If empty, the built-in Arial font is used(). The
 * FontFileName is the name of a file that contains a TrueType font.
 * https://www.1001freefonts.com/ is a good source for free TrueType
 * fonts.
 *
 * FontMultiplier - Font multiplier(6). The final FontSize is this value
 * times the word frequency.
 *
 * Gap - Space gap of words (2). The gap is the number of spaces added to
 * the beginning and end of each word.
 *
 * MaskColorName - Name of the color for the mask (black). This is the
 * name of the vtknamedColors that defines the foreground of the
 * mask. Usually black or white.  See https://tinyurl.com/y3yxcxj6 for
 * a visual representation of the named colors.
 *
 * MaskFileName - Mask file name(). If a mask file is specified, it will be
 * used as the mask. Otherwise, a black square is used as the mask. The
 * mask file should contain three channels of unsigned char values. If
 * the mask file is just a single unsigned char, specify turn the boolean
 * BWMask on.  If BWmask is on, the class will create a three channel
 * image using vtkImageAppendComponents.
 *
 * BWMask - Mask image has a single channel(false). Mask images typically
 * have three channels (r,g,b).
 *
 * MaxFontSize - Maximum font size(48).
 *
 * MinFontSize - Minimum font size(8).
 *
 * MinFrequency - Minimum word frequency accepted(2). Word with
 * frequencies less than this will be ignored.
 *
 * OffsetDistribution - Range of uniform random offsets(-size[0]/100.0
 * -size{1]/100.0)(-20 20). These offsets are offsets from the generated
 * path for word layout.
 *
 * OrientationDistribution - Ranges of random orientations(-20 20). If
 * discrete orientations are not defined, these orientations will be
 * generated.
 *
 * Orientations - Vector of discrete orientations(). If non-empty,
 * these will be used instead of the orientations distribution");
 *
 * ReplacementPairs - Replace the first word with another second word
 * ().  The each word will also added to the StopList. The second
 * argument can contain multiple words. For example you could replace
 * "bill" with "Bill Lorensen" or, "vtk" with "VTK . Remember that
 * words are always stored internally with lower case, even though the
 * first letter is capitalized in the Word Cloud.
 *
 * Sizes - Size of image(640 480).
 *
 * StopWords - User provided stop words(). Stop words are words that
 * are filtered out before processing of the text, such as the, is,
 * at, which, and so on.  vtkWordClass has built-in stop words. The
 * user-provided stop words are added to the built-in list. See
 * https://en.wikipedia.org/wiki/Stop_words for a description.  The
 * built-in stop words were derived from the english stop words at
 * https://www.ranks.nl/stopwords. Stop words for other languages are
 * also available.
 *
 * StopListFileName - the name of a file that contains stop words,
 * one word per line (). If present, the stop words in the file
 * replace the built-in stop list.
 *
 * Title - Add this word to the document's words and set a high
 * frequency, so that is will be rendered first.
 *
 * WordColorName - Name of the color for the words(). The name is
 * selected from vtkNamedColors. If the name is empty, the
 * ColorDistribution will generate random colors.  See
 * https://tinyurl.com/y3yxcxj6 for a visual representation of the
 * named colors.
 *
 * The class also provided Get methods that return vectors
 * StopWords, SkippedWords and KeptWords.
*/

VTK_ABI_NAMESPACE_BEGIN
class VTKINFOVISCORE_EXPORT vtkWordCloud : public vtkImageAlgorithm
{
public:
  vtkTypeMacro(vtkWordCloud, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with vertex cell generation turned off.
   */
  static vtkWordCloud* New();

  // Typedefs
  using ColorDistributionContainer = std::array<double, 2>;
  using OffsetDistributionContainer = std::array<int, 2>;
  using OrientationDistributionContainer = std::array<double, 2>;
  using OrientationsContainer = std::vector<double>;
  using PairType = std::tuple<std::string, std::string>;
  using ReplacementPairsContainer = std::vector<PairType>;
  using SizesContainer = std::array<int, 2>;
  using StopWordsContainer = std::set<std::string>;
  using StringContainer = std::vector<std::string>;

  ///@{
  /**
   * Return the AdjustedSizes of the resized mask file.
   */
  ///@}
  virtual SizesContainer GetAdjustedSizes() { return AdjustedSizes; }

#define SetStdContainerMacro(name, container)                                                      \
  virtual void Set##name(container arg)                                                            \
  {                                                                                                \
    bool changed = false;                                                                          \
    if (arg.size() != name.size())                                                                 \
    {                                                                                              \
      changed = true;                                                                              \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      auto a = arg.begin();                                                                        \
      for (auto r : name)                                                                          \
      {                                                                                            \
        if (*a != r)                                                                               \
        {                                                                                          \
          changed = true;                                                                          \
        }                                                                                          \
        a++;                                                                                       \
      }                                                                                            \
    }                                                                                              \
    if (changed)                                                                                   \
    {                                                                                              \
      name = arg;                                                                                  \
      this->Modified();                                                                            \
    }                                                                                              \
  }

  ///@{
  /**
   * Set/Get the vtkNamedColors name for the background(MidNightBlue).
   */
  ///@}
  virtual void SetBackgroundColorName(std::string arg)
  {
    if (arg != BackgroundColorName)
    {
      this->Modified();
      BackgroundColorName = arg;
    }
  }
  virtual std::string GetBackgroundColorName() { return BackgroundColorName; }

  ///@{
  /**
   * Set/Get boolean that indicates the mask image is a single
   * channel(false).
   */
  ///@}
  virtual void SetBWMask(bool arg)
  {
    if (BWMask != arg)
    {
      this->Modified();
      BWMask = arg;
    }
  }
  virtual bool GetBWMask() { return BWMask; }

  ///@{
  /**
   * Set/Get ColorSchemeName, the name of a color scheme from
   * vtkColorScheme to be used to select colors for the words (), if
   * WordColorName is empty. See https://tinyurl.com/y3j6c27o for a
   * visual representation of the color schemes.
   */
  ///@}
  virtual void SetColorSchemeName(std::string arg)
  {
    if (ColorSchemeName != arg)
    {
      this->Modified();
      ColorSchemeName = arg;
    }
  }
  virtual std::string GetColorSchemeName() { return ColorSchemeName; }

  ///@{
  /**
   * Set/GetDPI - Dots per inch(200) of the rendered text. DPI is
   * used as a scaling mechanism for the words. As DPI increases,
   * the word size increases. If there are too, few skipped words,
   * increase this value, too many, decrease it.
   */
  ///@}
  vtkSetMacro(DPI, int);
  vtkGetMacro(DPI, int);

  ///@{
  /**
   * Set/Get FileName, the name of the file that contains the text to
   * be processed.
   */
  ///@}
  virtual void SetFileName(VTK_FILEPATH std::string arg)
  {
    if (FileName != arg)
    {
      this->Modified();
      FileName = arg;
    }
  }
  virtual std::string GetFileName() VTK_FUTURE_CONST { return FileName; }

  ///@{
  /**
   * Set/Get FontFileName, If empty, the built-in Arial font is
   * used(). The FontFileName is the name of a file that contains a
   * TrueType font.
   */
  ///@}
  virtual void SetFontFileName(VTK_FILEPATH VTK_FUTURE_CONST std::string arg)
  {
    if (FontFileName != arg)
    {
      this->Modified();
      FontFileName = arg;
    }
  }
  virtual std::string GetFontFileName() VTK_FUTURE_CONST { return FontFileName; }

  ///@{
  /**
   * Set/Get Gap, the space gap of words (2). The gap is the number
   * of spaces added to the beginning and end of each word.
   */
  ///@}
  vtkSetMacro(Gap, int);
  vtkGetMacro(Gap, int);

  ///@{
  /**
   * Set/Get MaskColorName, the name of the color for the mask
   * (black). This is the name of the vtkNamedColors that defines
   * the foreground of the mask. Usually black or white.
   */
  ///@}
  virtual void SetMaskColorName(std::string arg)
  {
    if (MaskColorName != arg)
    {
      this->Modified();
      MaskColorName = arg;
    }
  }
  virtual std::string GetMaskColorName() { return MaskColorName; }

  ///@{
  /**
   * Set/Get MaskFileName, the mask file name(). If a mask file is
   * specified, it will be used as the mask. Otherwise, a black
   * square is used as the mask. The mask file should contain three
   * channels of unsigned char values. If the mask file is just a
   * single unsigned char, specify turn the boolean BWMask on.  If
   * BWmask is on, the class will create a three channel image using
   * vtkImageAppendComponents.
   */
  ///@}
  virtual void SetMaskFileName(VTK_FILEPATH VTK_FUTURE_CONST std::string arg)
  {
    if (MaskFileName != arg)
    {
      this->Modified();
      MaskFileName = arg;
    }
  }
  virtual std::string GetMaskFileName() VTK_FUTURE_CONST { return MaskFileName; }

  ///@{
  /**
   * Set/Get MaxFontSize, the maximum font size(48).
   */
  ///@}
  vtkSetMacro(MaxFontSize, int);
  vtkGetMacro(MaxFontSize, int);

  ///@{
  /**
   * Set/Get MinFontSize, the minimum font size(8).
   */
  ///@}
  vtkSetMacro(MinFontSize, int);
  vtkGetMacro(MinFontSize, int);

  ///@{
  /**
   * Set/Get MinFrequency, the minimum word frequency
   * accepted(2). Words with frequencies less than this will be
   * ignored.
   */
  ///@}
  vtkSetMacro(MinFrequency, int);
  vtkGetMacro(MinFrequency, int);

  ///@{
  /**
   * Set/Get FontMultiplier, the font multiplier(6). The final
   * FontSize is this value the word frequency.
   */
  ///@}
  vtkSetMacro(FontMultiplier, int);
  vtkGetMacro(FontMultiplier, int);

  ///@{
  /**
   * Set/Get ColorDistribution, the distribution of random colors(.6
   * 1.0), if WordColorName is empty.
   */
  ///@}
  SetStdContainerMacro(ColorDistribution, ColorDistributionContainer);
  virtual ColorDistributionContainer GetColorDistribution() { return ColorDistribution; }

  ///@{
  /**
   * Set/Get OffsetDistribution, the range of uniform random
   * offsets(-size[0]/100.0 -size{1]/100.0)(-20 20). These offsets
   * are offsets from the generated path for word layout.
   */
  ///@}
  SetStdContainerMacro(OffsetDistribution, OffsetDistributionContainer);
  virtual OffsetDistributionContainer GetOffsetDistribution() { return OffsetDistribution; }

  ///@{
  /**
   * Set/Get OrientationDistribution, ranges of random
   * orientations(-20 20). If discrete orientations are not defined,
   * these orientations will be generated.
   */
  ///@}
  SetStdContainerMacro(OrientationDistribution, OrientationDistributionContainer);
  virtual OrientationDistributionContainer GetOrientationDistribution()
  {
    return OrientationDistribution;
  }

  ///@{
  /**
   * Set/Add/Get Orientations, a vector of discrete orientations (). If
   * non-empty, these will be used instead of the orientations
   * distribution").
   */
  ///@}
  SetStdContainerMacro(Orientations, OrientationsContainer);
  void AddOrientation(double arg)
  {
    Orientations.push_back(arg);
    this->Modified();
  }
  virtual OrientationsContainer GetOrientations() { return Orientations; }

  ///@{
  /**
   * Set/Add/Get ReplacementPairs, a vector of words that replace the
   * first word with another second word (). The first word is also
   * added to the StopList.
   */
  ///@}
  SetStdContainerMacro(ReplacementPairs, ReplacementPairsContainer);
  void AddReplacementPair(PairType arg)
  {
    ReplacementPairs.push_back(arg);
    this->Modified();
  }

  virtual ReplacementPairsContainer GetReplacementPairs() { return ReplacementPairs; }

  ///@{
  /**
   * Set/Get Sizes, the size of the output image(640 480).
   */
  ///@}
  SetStdContainerMacro(Sizes, SizesContainer);
  virtual SizesContainer GetSizes() { return Sizes; }

  ///@{
  /**
   * Set/Add/Get StopWords, a set of user provided stop
   * words(). vtkWordClass has built-in stop words. The user-provided
   * stop words are added to the built-in list.
   */
  ///@}
  SetStdContainerMacro(StopWords, StopWordsContainer);
  void AddStopWord(std::string word)
  {
    StopWords.insert(word);
    this->Modified();
  }
  void ClearStopWords()
  {
    StopWords.clear();
    this->Modified();
  }
  virtual StopWordsContainer GetStopWords() { return StopWords; }

  ///@{
  /**
   * Set/Get StopListFileName, the name of the file that contains the
   * stop words, one per line.
   */
  ///@}
  virtual void SetStopListFileName(VTK_FILEPATH VTK_FUTURE_CONST std::string arg)
  {
    if (StopListFileName != arg)
    {
      this->Modified();
      StopListFileName = arg;
    }
  }
  virtual std::string GetStopListFileName() VTK_FUTURE_CONST { return StopListFileName; }

  ///@{
  /**
   * Set/Get Title, add this word to the document's words and set a
   * high frequency, so that is will be rendered first.
   */
  ///@}
  virtual void SetTitle(std::string arg)
  {
    if (Title != arg)
    {
      this->Modified();
      Title = arg;
    }
  }
  virtual std::string GetTitle() { return Title; }

  ///@{
  /**
   * Set/Get WordColorName, the name of the color for the
   * words(). The name is selected from vtkNamedColors. If the name
   * is empty, the ColorDistribution will generate random colors.
   */
  ///@}
  virtual void SetWordColorName(std::string arg)
  {
    if (WordColorName != arg)
    {
      this->Modified();
      WordColorName = arg;
    }
  }
  virtual std::string GetWordColorName() { return WordColorName; }
  ///@{
  /**
   * Get a vector of words that are kept in the final image.
   */
  ///@}
  virtual std::vector<std::string>& GetKeptWords() { return KeptWords; }

  ///@{
  /**
   * Get a vector of words that are skipped. Skipped words do not fit
   * in the final image.
   */
  ///@}
  virtual std::vector<std::string>& GetSkippedWords() { return SkippedWords; }

  ///@{
  /**
   * Get a vector of words that were stopped in the final image.
   */
  ///@}
  virtual std::vector<std::string>& GetStoppedWords() { return StoppedWords; }

protected:
  vtkWordCloud();
  ~vtkWordCloud() override = default;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkSmartPointer<vtkImageData> ImageData;
  int WholeExtent[6];

  SizesContainer AdjustedSizes;
  std::string BackgroundColorName;
  bool BWMask;
  ColorDistributionContainer ColorDistribution;
  std::string ColorSchemeName;
  int DPI;
  std::string FileName;
  std::string FontFileName;
  int FontMultiplier;
  int Gap;
  std::string MaskColorName;
  std::string MaskFileName;
  int MaxFontSize;
  int MinFontSize;
  int MinFrequency;
  OffsetDistributionContainer OffsetDistribution;
  OrientationDistributionContainer OrientationDistribution;
  OrientationsContainer Orientations;
  ReplacementPairsContainer ReplacementPairs;
  SizesContainer Sizes;
  StopWordsContainer StopWords;
  std::string StopListFileName;
  std::string Title;
  std::string WordColorName;

  std::vector<std::string> KeptWords;
  std::vector<std::string> SkippedWords;
  std::vector<std::string> StoppedWords;

private:
  vtkWordCloud(const vtkWordCloud&) = delete;
  void operator=(const vtkWordCloud&) = delete;

  // Declaring the type of Predicate that accepts 2 pairs and returns a bool
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
};
VTK_ABI_NAMESPACE_END
#endif

//  LocalWords:  vtkNamedColors SetMaskColorName
