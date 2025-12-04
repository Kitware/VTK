// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkResourceStream.h"

#include <array>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
#ifndef __VTK_WRAP__
vtkResourceStream::SeekDirection seekdirToSeekDirection(std::ios_base::seekdir dir)
{
  switch (dir)
  {
    case std::ios_base::beg:
      return vtkResourceStream::SeekDirection::Begin;
    case std::ios_base::cur:
      return vtkResourceStream::SeekDirection::Current;
    case std::ios_base::end:
      return vtkResourceStream::SeekDirection::End;
    default:
      return vtkResourceStream::SeekDirection::Begin;
  }
}

/**
 * Utilities class within vtkResourceStream to encapsulate it into a basic_streambuf
 * which can then be used to create an basic_istream.
 * See vtkOpenVDBReader for an example usage.
 * Uses std coding style volontarily
 */
template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_resource_streambuf : public std::basic_streambuf<CharT, Traits>
{
private:
  using parent_type = std::basic_streambuf<CharT, Traits>;

public:
  using char_type = CharT;
  using traits_type = Traits;
  using int_type = typename Traits::int_type;
  using pos_type = typename Traits::pos_type;
  using off_type = typename Traits::off_type;

  static constexpr std::size_t buf_size{ 128 };

  explicit basic_resource_streambuf(vtkResourceStream* stream)
    : m_stream{ stream }
  {
  }

protected:
  int_type underflow() override
  {
    if (parent_type::gptr() == parent_type::egptr())
    {
      const auto read =
        m_stream->Read(reinterpret_cast<char*>(std::data(m_buffer)), buf_size * sizeof(char_type));
      if (read == 0)
      {
        return traits_type::eof();
      }

      parent_type::setg(
        std::data(m_buffer), std::data(m_buffer), std::data(m_buffer) + (read / sizeof(char_type)));
    }

    return traits_type::to_int_type(*parent_type::gptr());
  }

  std::streamsize xsgetn(char_type* str, std::streamsize count) override
  {
    const auto read = m_stream->Read(reinterpret_cast<char*>(str), count * sizeof(char_type));

    return static_cast<std::streamsize>(read / sizeof(char_type));
  }

  pos_type seekoff(
    off_type off, std::ios_base::seekdir dir, std::ios_base::openmode vtkNotUsed(which)) override
  {
    return m_stream->Seek(off, ::seekdirToSeekDirection(dir));
  }

  pos_type seekpos(pos_type pos, std::ios_base::openmode vtkNotUsed(which)) override
  {
    return m_stream->Seek(pos, vtkResourceStream::SeekDirection::Begin);
  }

private:
  vtkSmartPointer<vtkResourceStream> m_stream;
  std::array<CharT, buf_size> m_buffer{};
};
}
#endif

struct vtkResourceStream::vtkInternals
{
  bool SupportSeek;
};

//------------------------------------------------------------------------------
vtkResourceStream::vtkResourceStream(bool supportSeek)
  : Impl{ new vtkInternals{} }
{
  this->Impl->SupportSeek = supportSeek;
}

//------------------------------------------------------------------------------
vtkResourceStream::~vtkResourceStream() = default;

//------------------------------------------------------------------------------
vtkTypeInt64 vtkResourceStream::Seek(vtkTypeInt64, SeekDirection)
{
  return 0;
}

//------------------------------------------------------------------------------
vtkTypeInt64 vtkResourceStream::Tell()
{
  return this->Seek(0, SeekDirection::Current);
}

bool vtkResourceStream::SupportSeek() const
{
  return this->Impl->SupportSeek;
}

//------------------------------------------------------------------------------
void vtkResourceStream::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << indent << "Support seek: " << (this->Impl->SupportSeek ? "yes" : "no") << "\n";
}

#ifndef __VTK_WRAP__
//------------------------------------------------------------------------------
std::unique_ptr<std::streambuf> vtkResourceStream::ToStreambuf()
{
  return std::make_unique<basic_resource_streambuf<char>>(this);
}
#endif

VTK_ABI_NAMESPACE_END
