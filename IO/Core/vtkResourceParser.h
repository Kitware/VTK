// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkResourceParser_h
#define vtkResourceParser_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkObject.h"
#include "vtkResourceStream.h" // For SeekDirection and vtkResourceStream
#include "vtkSmartPointer.h"   // For vtkSmartPointer

#include <array>       // for std::array
#include <cstdint>     // for std::int32_t
#include <cstdlib>     // for std::size_t
#include <functional>  // for std::function
#include <limits>      // for std::numeric_limits
#include <memory>      // for std::unique_ptr
#include <string>      // for std::string
#include <type_traits> // for SFINAE helpers

VTK_ABI_NAMESPACE_BEGIN

#ifndef __VTK_WRAP__ // do not wrap

/**
 * @brief Result of a vtkResouceParser parsing operation
 *
 * This enumeration gives information about what the parsing operation did.
 * When doing basic parsing, you may just need to check `result != vtkParseResult::Ok`.
 *
 * For more complex parsing, you can configure the parser to stop on newlines
 */
enum class vtkParseResult : std::int32_t
{
  Error = -1,      // Value not parsed because of type or formatting error
  Ok = 0,          // Value parsed successfully, no special status
  EndOfStream = 1, // No value parsed, stream reached its end
  EndOfLine = 2,   // No value parsed, this is an end of line
  Limit = 3,       // Value parsed successfully, limit has been reached
};

/**
 * @brief Helper class to perform formatted input from vtkResourceStream
 *
 * vtkResourceParser is a helper class that format input from an associated vtkResourceResource.
 * This class defines function to read integers, floats, booleans and strings. Other utility
 * functions such as ReadUntil or DiscardUntil are also available.
 *
 * Quick how to:
 * - Assign a stream to the parser using `SetStream`
 * - Perform input using one of the `Parse` overload
 * - Perform low level read using `Parse(char*, size)` overload
 * - Read data until a predicate is met using `ReadUntil`
 * - Discard data until a predicate is met using `DiscardUntil`
 * - Use `Seek` and `Tell` functions to modify/get cursor position including parser context
 * - Use `Reset` when the stream has been modified externally
 */
class VTKIOCORE_EXPORT vtkResourceParser : public vtkObject
{
public:
  /**
   * @brief predicate type used by `ReadUntil` and `DiscardUntil` functions
   */
  using PredicateType = std::function<bool(char c)>;

  ///@{
  /**
   * @brief Prebuild predicates for common cases
   *
   * DiscardNone: discard no character before parsing.
   * DiscardWhitespace: discard `\n`, `\r`, `\t`, `\v`, `\f` and spaces.
   * This is the default predicate.
   * DiscardNonAlphaNumeric: discard everything except [a-z], [A-Z] and [0-9].
   */
  static const PredicateType DiscardNone;
  static const PredicateType DiscardWhitespace;
  static const PredicateType DiscardNonAlphaNumeric;
  ///@}

  /**
   * @brief receiver type used by `ReadUntil` function
   */
  using DataReceiverType = std::function<void(const char* data, std::size_t size)>;

  static constexpr std::size_t NoLimit = (std::numeric_limits<std::size_t>::max)();

private:
  /**
   * @brief Internal class for parser context, contains the real implementation
   */
  class vtkInternals;

  /**
   * @brief Class for parser context, serves as a bridge between public API and real implementation
   */
  class VTKIOCORE_EXPORT vtkParserContext
  {
  public:
    vtkParserContext();
    ~vtkParserContext();
    vtkParserContext(const vtkParserContext&) = delete;
    vtkParserContext& operator=(const vtkParserContext&) = delete;

    /// @see ResourceParser#SetStream
    void SetStream(vtkResourceStream* stream);
    /// @see ResourceParser#GetStream
    vtkResourceStream* GetStream() const;

    /// @see ResourceParser#GetStopOnNewLine
    bool GetStopOnNewLine() const;
    /// @see ResourceParser#SetStopOnNewLine
    void SetStopOnNewLine(bool on);

    /// @see ResourceParser#Seek
    vtkTypeInt64 Seek(vtkTypeInt64 pos, vtkResourceStream::SeekDirection dir);
    /// @see ResourceParser#Tell
    vtkTypeInt64 Tell();
    /// @see ResourceParser#Read
    std::size_t Read(char* output, std::size_t size);
    /// @see ResourceParser#Reset
    void Reset();

    /// @see ResourceParser#Parse
    template <typename T>
    vtkParseResult Parse(T& output, const PredicateType& discardPred);

    /// @see ResourceParser#ReadUntil
    vtkParseResult ReadUntil(
      const PredicateType& discardPred, const DataReceiverType& receiver, std::size_t limit);
    /// @see ResourceParser#DiscardUntil
    vtkParseResult DiscardUntil(const PredicateType& discardPred);
    /// @see ResourceParser#ReadLine
    vtkParseResult ReadLine(const DataReceiverType& receiver, std::size_t limit);

    /// @see ResourceParser#PrintSelf
    void PrintSelf(ostream& os, vtkIndent indent);

  private:
    std::unique_ptr<vtkInternals> Impl;
  };

  /**
   * @brief Check if type is supported for parsing
   *
   * To support add a supported type you have to:
   * - Add it in this check
   * - Add extern template declaration below in this file
   * - Add extern template instantiation in source file
   * - Implement what is needed in vtkInternals
   */
  template <typename T>
  static constexpr bool IsSupported()
  {
    // Only remove references to check const and volatile
    using Type = typename std::remove_reference<T>::type;

    return std::is_same<Type, char>::value || std::is_same<Type, signed char>::value ||
      std::is_same<Type, unsigned char>::value || std::is_same<Type, short>::value ||
      std::is_same<Type, unsigned short>::value || std::is_same<Type, int>::value ||
      std::is_same<Type, unsigned int>::value || std::is_same<Type, long>::value ||
      std::is_same<Type, unsigned long>::value || std::is_same<Type, long long>::value ||
      std::is_same<Type, unsigned long long>::value || std::is_same<Type, float>::value ||
      std::is_same<Type, double>::value || std::is_same<Type, bool>::value ||
      std::is_same<Type, std::string>::value;
  }

public:
  vtkTypeMacro(vtkResourceParser, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkResourceParser* New();

  /**
   * @brief Set the stream to parse
   *
   * Automatically reset the parser state if `stream != this->GetStream()`.
   * Parsing starts at the stream position when set.
   *
   * @param stream a vtkResourceStream
   */
  void SetStream(vtkResourceStream* stream) { this->Context.SetStream(stream); }

  /**
   * @brief Get the parsed stream
   *
   * @return the vtkResourceStream used by this parser
   */
  vtkResourceStream* GetStream() const { return this->Context.GetStream(); }

  ///@{
  /**
   * @brief Specifies if the parser should handle newlines as a special token to stop on
   *
   * When is property is `true` the function `Parse` will break when encountering a new line.
   * When breaking will not modify output value and will return vtkParseResult::EndOfLine
   *
   * Default value: false
   */
  bool GetStopOnNewLine() const { return this->Context.GetStopOnNewLine(); }
  void SetStopOnNewLine(bool on) { this->Context.SetStopOnNewLine(on); }
  vtkBooleanMacro(StopOnNewLine, bool);
  ///@}

  /**
   * @brief Move stream cursor
   *
   * Calling `Read` or `Seek` on the stream associated to the parser may break the parser context
   * and result in unexpected behaviour. To prevent this, `Reset` must be called if  the stream
   * is externally modified before a `Parse`.
   *
   * This function will take into account the parser context.
   * This function will move the stream and reset the parser context if needed, as-if by calling
   * `this->GetStream()->Seek(pos, dir)` followed by `this->Reset()`, but may be more efficient.
   *
   * @param pos Seeked position, same as `vtkResourceStream::Seek`
   * @param dir Seek direction, same as `vtkResourceStream::Seek`
   * @return The position of the cursor from parser context, see `Tell`. -1 if associated
   * stream does not support seeking.
   */
  vtkTypeInt64 Seek(vtkTypeInt64 pos, vtkResourceStream::SeekDirection dir)
  {
    return this->Context.Seek(pos, dir);
  }

  /**
   * @brief Get stream cursor position from parser context
   *
   * `Tell()` will give the real position of the cursor from the parser context.
   * `vtkResourceParser` will buffer data read for the stream to parse it, this is the context.
   * Because of this, the stream position will always by "in advance" of the parser real input
   * position.
   * `this->Tell()` will always be lesser or equal to `this->GetStream()->Tell()`.
   *
   * @return The position of the cursor from parser context. -1 if associated
   * stream does not support seeking.
   */
  vtkTypeInt64 Tell() { return this->Context.Tell(); }

  /**
   * @brief Read data from the input stream
   *
   * Read at most `size` bytes from input stream. Read less than `size` bytes if EOS is reached.
   * If less than `size` bytes are read, bytes outside of [0; returnSize[ are not modified.
   * It is the equivalent of the `Read` function of `vtkResourceStream`,
   * but it takes parser context into account.
   *
   * @return the number of read bytes
   */
  std::size_t Read(char* output, std::size_t size) { return this->Context.Read(output, size); }

  /**
   * @brief Reset parser internal state
   *
   * This may be required in case the stream has been modified using `Seek`, `Read` or any other
   * subclass specific member function the will break the internal state, e.g. changing the input
   * file.
   * Using multiple parsers on the same stream is valid as long as each parser get reset before use
   * each time another one was used, and that only one parser is used concurrently.
   */
  void Reset() { this->Context.Reset(); }

  /**
   * @brief Main parsing function
   *
   * __**Parsing operation:**__
   *
   * The parsing operation is divided in 2 `Steps`:
   *
   * 1. Leading *discarded characters* are discarded:
   *    * A character is a *discarded characters* if `discardPred` returns `true`.
   *    * If no `discardPred` is specified, the default predicate is `DiscardWhitespace`
   *    * If StopOnNewLine is true, this function will return `vtkParseResult::EndOfLine` if it
   *      encounters a new line regardless of what `discardPred` returns for `\n` and `\r`.
   *    * If end of stream is reached, returns `vtkParseResult::EndOfStream`.
   * 2. The value is parsed using different algorithms, depending on its type, see below.
   *
   * `vtkParseResult::EndOfStream` will only be signaled if it is reached during step `1`.
   * If it is reached during step `2`, it will return the result of the decoding operation, and
   * return `vtkParseResult::EndOfStream` during the next Parse step `1`.
   *
   * ---
   *
   * __**Supported types:**__
   * Supported types are `char`, `signed char`, `unsigned char`, `short`, `unsigned short`, `int`,
   * `unsigned int`, `long`, `unsigned long`, `long long`, `unsigned long long`, `float`, `double`,
   * `bool` and `std::string`.
   *
   * `char` parsing will read a byte and will return the value as-is.
   *
   * `std::string` parsing reads characters until discardPred returns true. This last character is
   * not appended to the string nor discarded from input.
   *
   * Other types are parsed using ::vtkValueFromString.
   *
   * @param output A reference to the variable where the result will be written. Parsed type can
   * be determined from this.
   * @param discardPred The discard predicate to use during `Step 1`
   *
   * @return vtkParseResult::Error if parsing of value failed, in that case, the internal context
   * is not modified by `Step 2`, next `Parse` will try to parse the same data as long as the
   * discard predicate is semantically the same. vtkParseResult::EndOfLine if a new line is reached
   * during `Step 1`, the new line marker will be consumed. vtkParseResult::EndOfStream if no data
   * remains after `Step 1`. vtkParseResult::Ok otherwise.
   */
  template <typename T, typename std::enable_if<IsSupported<T>(), bool>::type = true>
  vtkParseResult Parse(T& output, const PredicateType& discardPred = DiscardWhitespace)
  {
    // Static check to prevent cryptic linker error
    static_assert(IsSupported<T>(), "Unsupported type given to Parse function");
    return this->Context.Parse(output, discardPred);
  }

  /**
   * @brief Read data from the input stream until the perdicate is met.
   *
   * @param discardPred function matching `bool(char c)` prototype of `PredicateType`.
   * @param receiver function matching the `void(const char* data, std::size_t size)` prototype
   * of `DataReceiverType`.
   * @param limit maximum amount of character to read, useful for statically sized buffers
   * (default: no limit)
   *
   * @return
   * vtkParseResult::EndOfStream if EOS is reached before pred is met or limit is reached.
   * vtkParseResult::Limit if limit is reached before pred is met.
   * vtkParseResult::Ok otherwise.
   */
  vtkParseResult ReadUntil(
    const PredicateType& discardPred, const DataReceiverType& receiver, std::size_t limit = NoLimit)
  {
    return this->Context.ReadUntil(discardPred, receiver, limit);
  }

  /**
   * Structure returned by Read*To functions
   */
  template <typename It>
  struct ReadToResult
  {
    /**
     * vtkParseResult::EndOfStream if EOS is reached before pred is met or limit is reached.
     * vtkParseResult::Limit if limit is reached before pred is met.
     * vtkParseResult::Ok otherwise.
     */
    vtkParseResult Result;

    /**
     * Iterator one past the last written value.
     */
    It Output;
  };

  /**
   * @brief Read data from the input stream to any output iterator until the perdicate is met.
   *
   * @param discardPred function matching `bool(char c)` prototype of `PredicateType`.
   * @param output Output iterator where data will be written
   * @param limit maximum amount of character to read, useful for statically sized buffers.
   * (default: no limit)
   *
   * @return A ReadToResult.
   */
  template <typename OutputIt>
  ReadToResult<OutputIt> ReadUntilTo(
    const PredicateType& discardPred, OutputIt output, std::size_t limit = NoLimit)
  {
    const auto result = this->ReadUntil(
      discardPred,
      [&output](const char* data, std::size_t size) mutable
      {
        for (std::size_t i{}; i < size; ++i)
        {
          *output++ = data[i];
        }
      },
      limit);

    return ReadToResult<OutputIt>{ result, output };
  }

  /**
   * @brief Read data from the input stream to any output range until the perdicate is met.
   *
   * @param discardPred function matching `bool(char c)` prototype of `PredicateType`
   * @param begin Forward iterator where data will be written (range begin)
   * @param end Forward iterator one past the last available output space (range end)
   *
   * @return A ReadToResult
   */
  template <typename ForwardIt>
  ReadToResult<ForwardIt> ReadUntilTo(
    const PredicateType& discardPred, ForwardIt begin, ForwardIt end)
  {
    return this->ReadUntilTo(discardPred, begin, std::distance(begin, end));
  }

  /**
   * @brief Discard data from the input stream until the perdicate is met.
   *
   * @param pred function matching `bool(char c)` prototype of `PredicateType`
   *
   * vtkParseResult::EndOfStream if EOS is reached before pred is met.
   * vtkParseResult::Ok otherwise.
   */
  vtkParseResult DiscardUntil(const PredicateType& pred)
  {
    return this->Context.DiscardUntil(pred);
  }

  /**
   * @brief Read an entire line from the input stream
   *
   * This function is similar to `std::getline` or `std::fgets`.
   * This function handles both `\r`, `\r\n` and `\n`.
   * The new line marker, either `\r`, `\r\n` or `\n`, will be discarded. i.e. won't be passed to
   * receiver nor kept in input stream.
   *
   * Return value will be false only if the stream does not contains any characters:
   * - `\n`, `\r` and `\r\n` will return true, after calling receiver once with size == 0.
   * - `""` will return false without calling receiver at all
   *
   * When limit is reached right before an end of line identifier, it won't be discarded:
   * - `"abc\n"` with `limit == 3` will give `"abc"` to receiver
   * and keep `"\n"` for the next operation.
   *
   * @param receiver function matching the `void(const char* data, std::size_t size)` prototype
   * of `DataReceiverType`.
   * @param limit maximum amount of character to extract
   *
   * @return
   * vtkParseResult::EndOfStream if EOS is reached before any character is read,
   * vtkParseResult::Limit if limit is reached before an end of line marker,
   * vtkParseResult::EndOfLine otherwise.
   * This function never returns vtkParseResult::Ok, vtkParseResult::EndOfLine indicates success.
   */
  vtkParseResult ReadLine(const DataReceiverType& receiver, std::size_t limit = NoLimit)
  {
    return this->Context.ReadLine(receiver, limit);
  }

  /**
   * @brief Read an entire line from the input stream
   *
   * Behaves like ReadLine(const DataReceiverType& receiver, std::size_t limit) except that the
   * output data is written to given std::string. This is the closest function to std::getline.
   *
   * @param output std::string to write line to, it is automatically cleared and resized
   * @param limit maximum amount of character to extract
   *
   * @return
   * vtkParseResult::EndOfStream if EOS is reached before pred is met or limit is reached,
   * vtkParseResult::Limit if limit is reached before pred is met,
   * vtkParseResult::EndOfLine otherwise.
   * This function never returns vtkParseResult::Ok, vtkParseResult::EndOfLine indicates success.
   */
  template <typename Allocator>
  vtkParseResult ReadLine(
    std::basic_string<char, std::char_traits<char>, Allocator>& output, std::size_t limit = NoLimit)
  {
    output.clear();

    return this->ReadLine(
      [&output](const char* data, std::size_t size) { output.append(data, size); }, limit);
  }

  /**
   * @brief Read an entire line from the input stream
   *
   * Behaves like ReadLine(const DataReceiverType& receiver, std::size_t limit) except that the
   * output data is written to given output iterator.
   *
   * @param output Output iterator to write line to
   * @param limit maximum amount of character to extract
   *
   * @return A ReadUntilToResult.
   * This function never returns vtkParseResult::Ok, vtkParseResult::EndOfLine indicates success.
   */
  template <typename OutputIt>
  ReadToResult<OutputIt> ReadLineTo(OutputIt output, std::size_t limit = NoLimit)
  {
    const auto result = this->ReadLine(
      [&output](const char* data, std::size_t size)
      {
        for (std::size_t i{}; i < size; ++i)
        {
          *output++ = data[i];
        }
      },
      limit);

    return ReadToResult<OutputIt>{ result, output };
  }

  /**
   * @brief Read an entire line from the input stream
   *
   * Behaves like ReadLine(const DataReceiverType& receiver, std::size_t limit) except that the
   * output data is written to given range and limit is determined by end.
   *
   * @param begin Forward iterator where data will be written (range begin)
   * @param end Forward iterator one past the last available output space (range end)
   *
   * @return A ReadUntilToResult.
   * This function never returns vtkParseResult::Ok, vtkParseResult::EndOfLine indicates success.
   */
  template <typename ForwardIt>
  ReadToResult<ForwardIt> ReadLineTo(ForwardIt begin, ForwardIt end)
  {
    return this->ReadLineTo(begin, std::distance(begin, end));
  }

  /**
   * @brief Discard a line from the input stream.
   *
   * @param limit maximum amount of character to discard
   *
   * vtkParseResult::EndOfStream if EOS is reached before any character is discarded,
   * vtkParseResult::Limit if limit is reached before an end of line marker,
   * vtkParseResult::EndOfLine otherwise.
   * This function never returns vtkParseResult::Ok, vtkParseResult::EndOfLine indicates success.
   */
  vtkParseResult DiscardLine(std::size_t limit = NoLimit)
  {
    return this->ReadLine([](const char*, std::size_t) {}, limit);
  }

protected:
  /**
   * @brief Constructor
   */
  vtkResourceParser() = default;
  ~vtkResourceParser() override = default;
  vtkResourceParser(const vtkResourceParser&) = delete;
  vtkResourceParser& operator=(const vtkResourceParser&) = delete;

private:
  vtkParserContext Context;
};

#define DECLARE_PARSE_EXTERN_TEMPLATE(type)                                                        \
  extern template VTKIOCORE_EXPORT vtkParseResult                                                  \
  vtkResourceParser::vtkParserContext::Parse<type>(type&, const PredicateType& discardPred)

// Declare explicit instantiation for all supported types
DECLARE_PARSE_EXTERN_TEMPLATE(char);
DECLARE_PARSE_EXTERN_TEMPLATE(signed char);
DECLARE_PARSE_EXTERN_TEMPLATE(unsigned char);
DECLARE_PARSE_EXTERN_TEMPLATE(short);
DECLARE_PARSE_EXTERN_TEMPLATE(unsigned short);
DECLARE_PARSE_EXTERN_TEMPLATE(int);
DECLARE_PARSE_EXTERN_TEMPLATE(unsigned int);
DECLARE_PARSE_EXTERN_TEMPLATE(long);
DECLARE_PARSE_EXTERN_TEMPLATE(unsigned long);
DECLARE_PARSE_EXTERN_TEMPLATE(long long);
DECLARE_PARSE_EXTERN_TEMPLATE(unsigned long long);
DECLARE_PARSE_EXTERN_TEMPLATE(float);
DECLARE_PARSE_EXTERN_TEMPLATE(double);
DECLARE_PARSE_EXTERN_TEMPLATE(bool);
DECLARE_PARSE_EXTERN_TEMPLATE(std::string);

#undef DECLARE_PARSE_EXTERN_TEMPLATE

#endif

VTK_ABI_NAMESPACE_END

#endif
