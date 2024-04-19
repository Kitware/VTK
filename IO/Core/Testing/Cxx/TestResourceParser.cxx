// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMemoryResourceStream.h"
#include "vtkNew.h"
#include "vtkResourceParser.h"

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>

#define Check(expr, message)                                                                       \
  if (!(expr))                                                                                     \
  {                                                                                                \
    std::cout << __FILE__ << " L." << __LINE__ << " | " << #expr << " failed: \n"                  \
              << message << std::endl;                                                             \
    return false;                                                                                  \
  }                                                                                                \
  static_cast<void>(0)

#define CheckOk(expr) Check((expr) == vtkParseResult::Ok, "Parsing failed")
#define CheckError(expr) Check((expr) == vtkParseResult::Error, "Expected failure")
#define CheckEndOfLine(expr) Check((expr) == vtkParseResult::EndOfLine, "Expected end of line")
#define CheckEndOfStream(expr)                                                                     \
  Check((expr) == vtkParseResult::EndOfStream, "Expected end of stream")
#define CheckLimit(expr) Check((expr) == vtkParseResult::Limit, "Expected limit to be reached")

bool TestIntParse()
{
  std::string input{ "42 0b010010110\r\n 0xbEeF 0b2 0x 0Xx 0x23x0 0o777 0x7F 0x100 1283618724687246"
                     " 0b010101001010010100101001010010111001010010100101000101010011100101011" };

  vtkNew<vtkMemoryResourceStream> stream;
  stream->SetBuffer(input.data(), input.size());

  vtkNew<vtkResourceParser> parser;
  parser->SetStream(stream);

  int i{};
  CheckOk(parser->Parse(i));
  Check(i == 42, "Wrong value, expected 42 got " << i);

  CheckOk(parser->Parse(i));
  Check(i == 150, "Wrong value, expected 150 got " << i);

  CheckOk(parser->Parse(i));
  Check(i == 0xbEeF, "Wrong value, expected 0xbEeF () got " << i);

  CheckError(parser->Parse(i)); // '2' in binary int
  parser->Seek(3, vtkResourceStream::SeekDirection::Current);
  CheckError(parser->Parse(i)); // No digit in hex value
  parser->Seek(2, vtkResourceStream::SeekDirection::Current);
  CheckError(parser->Parse(i)); // No digit in hex value
  parser->Seek(3, vtkResourceStream::SeekDirection::Current);

  CheckOk(parser->Parse(i));
  Check(i == 0x23, "Wrong value, expected 0x23 got " << i);

  parser->Seek(2, vtkResourceStream::SeekDirection::Current);
  CheckOk(parser->Parse(i));
  Check(i == 0777, "Wrong value, expected 0777 got " << i);

  signed char sc{};
  CheckOk(parser->Parse(sc));
  Check(sc == 127, "Wrong value, expected 127 got " << static_cast<int>(sc));

  CheckError(parser->Parse(sc)); // value overflow (128 in signed char)
  parser->Seek(5, vtkResourceStream::SeekDirection::Current);

  CheckError(parser->Parse(i)); // value overflow
  parser->Seek(17, vtkResourceStream::SeekDirection::Current);
  CheckError(parser->Parse(i)); // value overflow

  return true;
}

bool TestFloatParse()
{
  std::string input{ "84327.3432,\n 5.8413e4 nAN\r iNf -InF" };

  vtkNew<vtkMemoryResourceStream> stream;
  stream->SetBuffer(input.data(), input.size());

  vtkNew<vtkResourceParser> parser;
  parser->SetStream(stream);

  double d{};
  CheckOk(parser->Parse(d));
  Check(84327.3431 <= d && d <= 84327.3433, "Wrong value, expected 84327.3432 got " << d);

  std::string str{ "Check if this string is erased before parsing :)" };
  CheckOk(parser->Parse(str));
  Check(str == ",", "Parsing failed");

  float f{};
  CheckOk(parser->Parse(f));
  Check(5.8412e4 <= f && f <= 5.8414e4, "Wrong value, expected 5.8413e4 got " << f);
  CheckOk(parser->Parse(f));
  Check(std::isnan(f), "Wrong value expected NaN, got " << f);
  CheckOk(parser->Parse(f));
  Check(
    std::isinf(f) && f > std::numeric_limits<float>::max(), "Wrong value, expected +Inf got " << f);
  CheckOk(parser->Parse(f));
  Check(std::isinf(f) && f < std::numeric_limits<float>::lowest(),
    "Wrong value, expected -Inf got " << f);

  return true;
}

bool TestBoolParse()
{
  std::string input{ "true false\v Yes hello False\f 0 1" };

  vtkNew<vtkMemoryResourceStream> stream;
  stream->SetBuffer(input.data(), input.size());

  vtkNew<vtkResourceParser> parser;
  parser->SetStream(stream);

  bool b{};
  CheckOk(parser->Parse(b));
  Check(b, "Wrong value");

  CheckOk(parser->Parse(b));
  Check(!b, "Wrong value");

  CheckError(parser->Parse(b));
  parser->Seek(3, vtkResourceStream::SeekDirection::Current);
  CheckError(parser->Parse(b));
  parser->Seek(5, vtkResourceStream::SeekDirection::Current);

  CheckOk(parser->Parse(b));
  Check(!b, "Wrong value");
  CheckOk(parser->Parse(b));
  Check(!b, "Wrong value");

  CheckOk(parser->Parse(b));
  Check(b, "Wrong value");

  return true;
}

bool TestReadLine()
{
  std::string input{ "This is a line that end with \\r\\n!\r\n"
                     "This is a line that end with \\r!\r"
                     "This is a line that end with \\n!\n"
                     "\r"   // empty line
                     "\r\n" // empty line
                     "\n"   // empty line
                     "This is the last line" };

  static const std::array<const char*, 7> expected{ "This is a line that end with \\r\\n!",
    "This is a line that end with \\r!", "This is a line that end with \\n!", "", "", "",
    "This is the last line" };

  vtkNew<vtkMemoryResourceStream> stream;
  stream->SetBuffer(input.data(), input.size());

  vtkNew<vtkResourceParser> parser;
  parser->SetStream(stream);

  std::string line;
  std::size_t index{};
  while (parser->ReadLine(line) == vtkParseResult::EndOfLine) // EndOfFile means success
  {
    Check(index < expected.size(), "Read to many lines");
    Check(
      line == expected[index], "Expected \"" << expected[index] << "\" but got \"" << line << "\"");
    ++index;
  }

  Check(index == expected.size(), "Not enough lines were read");

  return true;
}

bool TestReadLineLimit()
{
  std::string input{ "55555\r\n333\n" };

  static const std::array<const char*, 4> expectedValues{ "555", "55", "333", "" };
  static const std::array<vtkParseResult, 5> expectedResults{
    vtkParseResult::Limit,
    vtkParseResult::EndOfLine,
    vtkParseResult::Limit,
    vtkParseResult::EndOfLine,
    vtkParseResult::EndOfStream,
  };

  vtkNew<vtkMemoryResourceStream> stream;
  stream->SetBuffer(input.data(), input.size());

  vtkNew<vtkResourceParser> parser;
  parser->SetStream(stream);

  std::string line;
  std::size_t index{};
  while (parser->ReadLine(line, 3) == expectedResults[index])
  {
    if (index >= expectedValues.size())
    {
      return true;
    }

    Check(line == expectedValues[index],
      "Expected \"" << expectedValues[index] << "\" but got \"" << line << "\"");
    ++index;
  }

  return true;
}

bool TestReadLineTo()
{
  std::string input{ "55555\r\n" };

  vtkNew<vtkMemoryResourceStream> stream;
  stream->SetBuffer(input.data(), input.size());

  vtkNew<vtkResourceParser> parser;
  parser->SetStream(stream);

  std::string line;
  CheckLimit(parser->ReadLineTo(std::back_inserter(line), 3).Result);
  Check(line.size() == 3, "Read to much data");
  Check(line == "555", "Expected \"555\" but got \"" << line << "\"");

  line.clear();
  line.resize(2);
  const auto result = parser->ReadLineTo(line.begin(), line.end());
  CheckLimit(result.Result);
  Check(result.Output == line.end(), "Wrong output iterator");
  Check(line == "55", "Expected \"55\" but got \"" << line << "\"");

  line.clear();
  CheckEndOfLine(parser->ReadLineTo(std::back_inserter(line)).Result);
  Check(line.empty(), "Read to much data");

  return true;
}

bool TestStringParse()
{
  // \xC3\xA9 is a e with acute accent
  std::string input{ "Hello world! // _// - 7,\t\xC3\xA9 this will be skipped" };

  const auto dataBegin = static_cast<std::int64_t>(input.size());
  for (std::size_t i{}; i < 100; ++i) // generate 2600 letters at the end of data to test buffering
  {
    input += "abcdefghijklmnopqrstuvwxyz";
  }

  vtkNew<vtkMemoryResourceStream> stream;
  stream->SetBuffer(input.data(), input.size());

  vtkNew<vtkResourceParser> parser;
  parser->SetStream(stream);

  std::string str{ "Check if this string is erased before parsing :)" };
  CheckOk(parser->Parse(str));
  Check(str == "Hello", "Parsing failed");
  CheckOk(parser->Parse(str));
  Check(str == "world!", "Parsing failed");
  parser->DiscardUntil([](char c) { return c == '-'; });
  CheckOk(parser->Parse(str));
  Check(str == "-", "Parsing failed");

  char ch{};
  CheckOk(parser->Parse(ch));
  Check(ch == '7', "Parsing failed");
  CheckOk(parser->Parse(ch));
  Check(ch == ',', "Parsing failed");
  CheckOk(parser->Parse(ch));
  Check(ch == '\xC3', "Parsing failed");
  CheckOk(parser->Parse(ch));
  Check(ch == '\xA9', "Parsing failed");

  // Buffer checks
  Check(parser->Seek(dataBegin, vtkResourceStream::SeekDirection::Begin) == dataBegin,
    "Wrong seek position");

  str.resize(26);
  // NOLINTNEXTLINE(readability-container-data-pointer)
  Check(parser->Read(&str[0], 26) == 26, "Parsing failed");
  Check(str == "abcdefghijklmnopqrstuvwxyz", "Wrong value");

  Check(parser->Tell() == dataBegin + 26, "Wrong parser position");
  Check(parser->GetStream()->Tell() > parser->Tell(), "Wrong stream position");

  Check(parser->Seek(10, vtkResourceStream::SeekDirection::Current) == dataBegin + 26 + 10,
    "Wrong position");

  str.resize(16);
  // NOLINTNEXTLINE(readability-container-data-pointer)
  Check(parser->Read(&str[0], 16) == 16, "Parsing failed");
  Check(str == "klmnopqrstuvwxyz", "Wrong value");

  str.clear();
  CheckOk(parser->ReadUntil([](char c) { return c == 'n'; },
    [&str](const char* data, std::size_t size) { str.append(data, size); }));

  Check(str == "abcdefghijklm", "Wrong value expected \"abcdefghijklm\" got " << str);

  const auto beforeDiscard = parser->Tell();
  CheckOk(parser->DiscardUntil([](char c) { return c == 'a'; }));

  Check(parser->Tell() - beforeDiscard == 13, "Wrong position");

  CheckOk(parser->Parse(ch));
  Check(ch == 'a', "Wrong value");

  Check(parser->Seek(-26, vtkResourceStream::SeekDirection::End) == dataBegin + 2600 - 26,
    "Wrong position");
  str.resize(26);
  // NOLINTNEXTLINE(readability-container-data-pointer)
  Check(parser->Read(&str[0], 26) == 26, "Parsing failed");
  Check(str == "abcdefghijklmnopqrstuvwxyz", "Wrong value");

  CheckEndOfStream(parser->Parse(str));

  return true;
}

bool TestStopOnNewLine()
{
  std::string data{ "12,\ntrue\r\n\v\r3.14 \t,\n" };

  vtkNew<vtkMemoryResourceStream> stream;
  stream->SetBuffer(data.data(), data.size());

  vtkNew<vtkResourceParser> parser;
  parser->StopOnNewLineOn();
  parser->SetStream(stream);

  int i{};
  CheckOk(parser->Parse(i));
  Check(i == 12, "Wrong value");

  std::string str{ "Check if this string is erased before parsing :)" };
  CheckOk(parser->Parse(str));
  Check(str == ",", "Wrong value");

  bool b{};
  CheckEndOfLine(parser->Parse(b));
  CheckOk(parser->Parse(b));
  Check(b, "Wrong value");

  double d{};
  CheckEndOfLine(parser->Parse(d));
  CheckEndOfLine(parser->Parse(d));
  CheckOk(parser->Parse(d));
  Check(3.1399 <= d && d <= 3.1401, "Wrong value");

  char c{};
  CheckOk(parser->Parse(c));
  Check(c == ',', "Wrong value");
  CheckEndOfLine(parser->Parse(c));
  CheckEndOfStream(parser->Parse(c));

  return true;
}

bool TestDiscardPredicate()
{
  std::string input{ "--a\n\r----12\r\n-hello---" };

  vtkNew<vtkMemoryResourceStream> stream;
  stream->SetBuffer(input.data(), input.size());

  vtkNew<vtkResourceParser> parser;
  parser->StopOnNewLineOn();
  parser->SetStream(stream);

  const auto discard = [](char c) { return c == '-'; };

  char c{};
  CheckOk(parser->Parse(c, discard));
  Check(c == 'a', "Wrong value");

  int i{};
  CheckEndOfLine(parser->Parse(i, discard));
  CheckEndOfLine(parser->Parse(i, discard));
  CheckOk(parser->Parse(i, discard));
  Check(i == 12, "Wrong value");

  std::string str{ "Check if this string is erased before parsing :)" };
  CheckEndOfLine(parser->Parse(str, discard));
  CheckOk(parser->Parse(str, discard));
  Check(str == "hello", "Wrong value");
  CheckEndOfStream(parser->Parse(str, discard));

  return true;
}

int TestResourceParser(int, char*[])
{
  if (!TestIntParse())
  {
    return EXIT_FAILURE;
  }

  if (!TestFloatParse())
  {
    return EXIT_FAILURE;
  }

  if (!TestBoolParse())
  {
    return EXIT_FAILURE;
  }

  if (!TestStringParse())
  {
    return EXIT_FAILURE;
  }

  if (!TestReadLine())
  {
    return EXIT_FAILURE;
  }

  if (!TestReadLineLimit())
  {
    return EXIT_FAILURE;
  }

  if (!TestReadLineTo())
  {
    return EXIT_FAILURE;
  }

  if (!TestStopOnNewLine())
  {
    return EXIT_FAILURE;
  }

  if (!TestDiscardPredicate())
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
