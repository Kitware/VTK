// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkResourceParser.h"

#include "vtkObjectFactory.h"
#include "vtkResourceStream.h"
#include "vtkValueFromString.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <utility>

VTK_ABI_NAMESPACE_BEGIN

class vtkResourceParser::vtkInternals
{
public:
  static constexpr std::size_t BufferTail = 256;
  static constexpr std::size_t BufferSize = 512;
  static_assert(BufferSize >= BufferTail, "BufferSize must be at least BufferTail");

  void SetStream(vtkResourceStream* stream)
  {
    if (stream != this->Stream)
    {
      this->Stream = vtk::MakeSmartPointer(stream);
      this->Reset();
    }
  }

  vtkResourceStream* GetStream() const { return this->Stream.Get(); }

  bool GetStopOnNewLine() const { return this->StopOnNewLine; }

  void SetStopOnNewLine(bool on) { this->StopOnNewLine = on; }

  vtkTypeInt64 Seek(vtkTypeInt64 pos, vtkResourceStream::SeekDirection dir)
  {
    if (!this->Stream->SupportSeek())
    {
      return -1;
    }

    // we may not need the reset entirely the internal state
    // try to emulate seek in the current range
    // can not do this for End direction since we can't know where is the end
    if (dir != vtkResourceStream::SeekDirection::End && !this->RangeEmpty())
    {
      vtkTypeInt64 seeked{};
      if (dir == vtkResourceStream::SeekDirection::Current)
      {
        seeked = this->Tell() + pos;
      }
      else
      {
        seeked = pos;
      }

      const vtkTypeInt64 streamPos{ this->Stream->Tell() };
      const vtkTypeInt64 parserPos{ streamPos - static_cast<vtkTypeInt64>(this->RangeSize()) };

      if (parserPos <= seeked && seeked <= streamPos)
      {
        // range.end() always correspond to the streamPos
        const vtkTypeInt64 offset{ streamPos - seeked };
        this->Begin = this->End - offset;

        return seeked;
      }

      this->Reset();
      return this->Stream->Seek(seeked, vtkResourceStream::SeekDirection::Begin);
    }

    // otherwise juste reset and seek
    this->Reset();
    return this->Stream->Seek(pos, dir);
  }

  vtkTypeInt64 Tell()
  {
    if (!this->Stream->SupportSeek())
    {
      return -1;
    }

    return this->Stream->Tell() - static_cast<vtkTypeInt64>(this->RangeSize());
  }

  void Reset()
  {
    this->Begin = nullptr;
    this->End = nullptr;
  }

  template <typename T>
  vtkParseResult Parse(T& output, const PredicateType& discardPred)
  {
    switch (DiscardLeadingCharacters(discardPred))
    {
      case vtkParseResult::Error:
        return vtkParseResult::Error;
      case vtkParseResult::EndOfStream:
        return vtkParseResult::EndOfStream;
      case vtkParseResult::EndOfLine:
        return vtkParseResult::EndOfLine;
      default:
        break;
    }

    // ensure that there is at least BufferTail characters available
    if (this->RangeSize() < BufferTail && !this->Stream->EndOfStream())
    {
      // if we reached the end of the range we have to copy the leftover to the beginning
      const auto begin = std::copy(this->Begin, this->End, this->Buffer.begin());
      const auto offset = std::distance(this->Buffer.begin(), begin);

      // Always read buffer size, may be beneficial for the stream to have a constant, aligned, size
      const auto read = this->Stream->Read(std::addressof(*begin), BufferSize);
      this->Begin = this->Buffer.data();
      this->End = this->Buffer.data() + offset + read;
    }

    const auto consumed = vtkValueFromString(this->Begin, this->End, output);
    if (consumed == 0)
    {
      return vtkParseResult::Error;
    }

    this->Begin += consumed;

    return vtkParseResult::Ok;
  }

  std::size_t Parse(char* output, std::size_t size)
  {
    if (size > this->RangeSize())
    {
      const auto first = this->RangeSize();
      std::copy_n(this->Begin, first, output);
      this->Reset(); // empty range

      // buffer only if remaining size does fit, otherwise do a direct read from stream
      const auto remaining = size - first;
      if (remaining < BufferSize)
      {
        this->FillRange();

        std::copy_n(this->Begin, remaining, output + first);
        this->Begin += remaining;

        return size;
      }
      else
      {
        return first + this->Stream->Read(output + first, remaining);
      }
    }

    std::copy_n(this->Begin, size, output);
    this->Begin += size;

    return size;
  }

  template <typename Pred, typename DataReceiver>
  vtkParseResult ReadUntil(Pred&& pred, DataReceiver&& receiver, std::size_t limit)
  {
    if (this->RangeEmpty())
    {
      this->FillRange();
    }

    std::size_t total{};
    while (!this->RangeEmpty())
    {
      const auto it = std::find_if(this->Begin, this->End, pred);
      const auto size = static_cast<std::size_t>(std::distance(this->Begin, it));

      if (limit != NoLimit)
      {
        if (total + size >= limit) // stop here
        {
          const auto limitedSize = limit - total;

          receiver(this->Begin, limitedSize);
          this->Begin += limitedSize;

          return vtkParseResult::Limit;
        }

        total += size;
      }

      if (it != this->End) // stop here
      {
        receiver(this->Begin, size);
        this->Begin = it;

        return vtkParseResult::Ok;
      }

      receiver(this->Begin, this->RangeSize());
      this->FillRange();
    }

    return vtkParseResult::EndOfStream; // don't find a char that match predicate before EOS
  }

  template <typename Pred>
  vtkParseResult DiscardUntil(Pred&& pred)
  {
    return this->ReadUntil(
      std::forward<Pred>(pred), [](const char*, std::size_t) {},
      std::numeric_limits<std::size_t>::max());
  }

  vtkParseResult ReadLine(const DataReceiverType& receiver, std::size_t limit)
  {
    if (this->RangeEmpty())
    {
      this->FillRange();
    }

    std::size_t total{};
    while (!this->RangeEmpty())
    {
      auto it = std::find_if(this->Begin, this->End, [](char c) { return c == '\n' || c == '\r'; });

      const auto size = static_cast<std::size_t>(std::distance(this->Begin, it));

      if (limit != std::numeric_limits<std::size_t>::max())
      {
        if (total + size >= limit) // stop here
        {
          const auto limitedSize = limit - total;

          receiver(this->Begin, limitedSize);
          this->Begin += limitedSize;

          return vtkParseResult::Limit;
        }
      }

      total += size;

      if (it != this->End) // stop here
      {
        receiver(this->Begin, size);
        this->Begin = it;

        if (*this->Begin == '\r') // check \r\n and unique \r
        {
          ++this->Begin;
          if (this->Begin == this->End)
          {
            // read more data from stream if possible
            if (this->FillRange() == 0)
            {
              return vtkParseResult::EndOfLine; // it was the last line
            }
          }

          if (*this->Begin == '\n') // discard both
          {
            ++this->Begin;
          }
        }
        else // discard \n
        {
          ++this->Begin;
        }

        return vtkParseResult::EndOfLine;
      }

      receiver(this->Begin, this->RangeSize());
      this->FillRange();
    }

    if (total == 0)
    {
      return vtkParseResult::EndOfStream;
    }

    return vtkParseResult::EndOfLine;
  }

  void PrintSelf(ostream& os, vtkIndent indent)
  {
    if (this->Stream)
    {
      this->Stream->PrintSelf(os, indent.GetNextIndent());
    }
    else
    {
      os << indent << "Stream: None" << std::endl;
    }

    os << indent << "Buffer size: " << BufferSize << std::endl;
    os << indent << "Buffer tail: " << BufferTail << std::endl;
    os << indent << "Current buffered size: " << this->RangeSize() << std::endl;

    if (this->Stream->SupportSeek())
    {
      os << indent << "Cursor position: " << this->Tell() << std::endl;
      os << indent << "Stream cursor position: " << this->Stream->Tell() << std::endl;
    }
    else
    {
      os << indent << "Cursor position: seek not supported" << std::endl;
      os << indent << "Stream cursor position: seek not supported" << std::endl;
    }
  }

private:
  std::size_t FillRange()
  {
    const auto read = this->Stream->Read(this->Buffer.data(), BufferSize);
    this->Begin = this->Buffer.data();
    this->End = this->Buffer.data() + read;

    return read;
  }

  // Discard characters before parsing
  vtkParseResult DiscardLeadingCharacters(const PredicateType& discardPred)
  {
    if (this->StopOnNewLine)
    {
      bool newLine{};
      const auto result = this->DiscardUntil([&discardPred, &newLine](char c) {
        if (c == '\r' || c == '\n')
        {
          newLine = true;
          return true;
        }

        return !discardPred(c);
      });

      if (newLine)
      {
        return this->ReadLine([](const char*, std::size_t) {}, NoLimit); // discard new line marker
      }

      return result;
    }
    else
    {
      return this->DiscardUntil([&discardPred](char c) { return !discardPred(c); });
    }
  }

  std::size_t RangeSize() const { return static_cast<std::size_t>(this->End - this->Begin); }

  bool RangeEmpty() const { return this->RangeSize() == 0; }

  vtkSmartPointer<vtkResourceStream> Stream;
  bool StopOnNewLine{ false };
  const char* Begin{};
  const char* End{};
  // Buffer last byte must remain '\0' since we are working with null-terminated strings
  // End must always be less than Buffer.end() to ensure that this last char will stay null
  std::array<char, BufferSize + BufferTail> Buffer{};
};

// GCC has a bug that prevent template specialization definitions directly in the class
// c.f. https://gcc.gnu.org/bugzilla/show_bug.cgi?id=85282
template <>
vtkParseResult vtkResourceParser::vtkInternals::Parse<char>(
  char& output, const PredicateType& discardPred)
{
  switch (DiscardLeadingCharacters(discardPred))
  {
    case vtkParseResult::Error:
      return vtkParseResult::Error;
    case vtkParseResult::EndOfStream:
      return vtkParseResult::EndOfStream;
    case vtkParseResult::EndOfLine:
      return vtkParseResult::EndOfLine;
    default:
      break;
  }

  if (this->RangeEmpty() && this->FillRange() == 0)
  {
    return vtkParseResult::EndOfStream;
  }

  output = *this->Begin++;

  return vtkParseResult::Ok;
}

template <>
vtkParseResult vtkResourceParser::vtkInternals::Parse<std::string>(
  std::string& output, const PredicateType& discardPred)
{
  output.clear();

  switch (DiscardLeadingCharacters(discardPred))
  {
    case vtkParseResult::Error:
      return vtkParseResult::Error;
    case vtkParseResult::EndOfStream:
      return vtkParseResult::EndOfStream;
    case vtkParseResult::EndOfLine:
      return vtkParseResult::EndOfLine;
    default:
      break;
  }

  const auto receiver = [&output](
                          const char* data, std::size_t size) { output.append(data, size); };

  this->ReadUntil(discardPred, receiver, std::numeric_limits<std::size_t>::max());

  if (output.empty())
  {
    return vtkParseResult::EndOfStream;
  }

  return vtkParseResult::Ok;
}

//------------------------------------------------------------------------------
vtkResourceParser::vtkParserContext::vtkParserContext()
  : Impl{ new vtkInternals }
{
}

//------------------------------------------------------------------------------
vtkResourceParser::vtkParserContext::~vtkParserContext() = default;

//------------------------------------------------------------------------------
void vtkResourceParser::vtkParserContext::SetStream(vtkResourceStream* stream)
{
  this->Impl->SetStream(stream);
}

//------------------------------------------------------------------------------
vtkResourceStream* vtkResourceParser::vtkParserContext::GetStream() const
{
  return this->Impl->GetStream();
}

bool vtkResourceParser::vtkParserContext::GetStopOnNewLine() const
{
  return this->Impl->GetStopOnNewLine();
}

void vtkResourceParser::vtkParserContext::SetStopOnNewLine(bool on)
{
  this->Impl->SetStopOnNewLine(on);
}

//------------------------------------------------------------------------------
vtkTypeInt64 vtkResourceParser::vtkParserContext::Seek(
  vtkTypeInt64 pos, vtkResourceStream::SeekDirection dir)
{
  return this->Impl->Seek(pos, dir);
}

//------------------------------------------------------------------------------
vtkTypeInt64 vtkResourceParser::vtkParserContext::Tell()
{
  return this->Impl->Tell();
}

//------------------------------------------------------------------------------
std::size_t vtkResourceParser::vtkParserContext::Read(char* output, std::size_t size)
{
  return this->Impl->Parse(output, size);
}

//------------------------------------------------------------------------------
void vtkResourceParser::vtkParserContext::Reset()
{
  this->Impl->Reset();
}

//------------------------------------------------------------------------------
vtkParseResult vtkResourceParser::vtkParserContext::ReadUntil(
  const PredicateType& discardPred, const DataReceiverType& receiver, std::size_t limit)
{
  assert(discardPred && "discardPred must not be null");

  return this->Impl->ReadUntil(discardPred, receiver, limit);
}

//------------------------------------------------------------------------------
vtkParseResult vtkResourceParser::vtkParserContext::DiscardUntil(const PredicateType& discardPred)
{
  assert(discardPred && "discardPred must not be null");

  return this->Impl->DiscardUntil(discardPred);
}

vtkParseResult vtkResourceParser::vtkParserContext::ReadLine(
  const DataReceiverType& receiver, std::size_t limit)
{
  return this->Impl->ReadLine(receiver, limit);
}

//------------------------------------------------------------------------------
template <typename T>
vtkParseResult vtkResourceParser::vtkParserContext::Parse(
  T& output, const PredicateType& discardPred)
{
  assert(discardPred && "discardPred must not be null");

  return this->Impl->Parse(output, discardPred);
}

//------------------------------------------------------------------------------
// explicit instantiation for all supported types
#define INSTANTIATE_PARSE_EXTERN_TEMPLATE(type)                                                    \
  template vtkParseResult vtkResourceParser::vtkParserContext::Parse<type>(                        \
    type&, const PredicateType& discardPred)

// Declare explicit instantiation for all supported types
INSTANTIATE_PARSE_EXTERN_TEMPLATE(char);
INSTANTIATE_PARSE_EXTERN_TEMPLATE(signed char);
INSTANTIATE_PARSE_EXTERN_TEMPLATE(unsigned char);
INSTANTIATE_PARSE_EXTERN_TEMPLATE(short);
INSTANTIATE_PARSE_EXTERN_TEMPLATE(unsigned short);
INSTANTIATE_PARSE_EXTERN_TEMPLATE(int);
INSTANTIATE_PARSE_EXTERN_TEMPLATE(unsigned int);
INSTANTIATE_PARSE_EXTERN_TEMPLATE(long);
INSTANTIATE_PARSE_EXTERN_TEMPLATE(unsigned long);
INSTANTIATE_PARSE_EXTERN_TEMPLATE(long long);
INSTANTIATE_PARSE_EXTERN_TEMPLATE(unsigned long long);
INSTANTIATE_PARSE_EXTERN_TEMPLATE(float);
INSTANTIATE_PARSE_EXTERN_TEMPLATE(double);
INSTANTIATE_PARSE_EXTERN_TEMPLATE(bool);
INSTANTIATE_PARSE_EXTERN_TEMPLATE(std::string);

#undef INSTANTIATE_PARSE_EXTERN_TEMPLATE

//------------------------------------------------------------------------------
void vtkResourceParser::vtkParserContext::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Impl->PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
const vtkResourceParser::PredicateType vtkResourceParser::DiscardNone = [](char) { return false; };

//------------------------------------------------------------------------------
const vtkResourceParser::PredicateType vtkResourceParser::DiscardWhitespace = [](char c) {
  return std::isspace(static_cast<unsigned char>(c));
};

//------------------------------------------------------------------------------
const vtkResourceParser::PredicateType vtkResourceParser::DiscardNonAlphaNumeric = [](char c) {
  return !std::isalnum(static_cast<unsigned char>(c));
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkResourceParser);

//------------------------------------------------------------------------------
void vtkResourceParser::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  this->Context.PrintSelf(os, indent);
}

VTK_ABI_NAMESPACE_END
