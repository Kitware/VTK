// Copyright (c) 2014-2018 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_HPP
#define TAO_PEGTL_HPP

#include "pegtl/config.hpp"
#include "pegtl/version.hpp"

#include "pegtl/ascii.hpp"
#include "pegtl/parse.hpp"
#include "pegtl/rules.hpp"
#include "pegtl/uint16.hpp"
#include "pegtl/uint32.hpp"
#include "pegtl/uint64.hpp"
#include "pegtl/uint8.hpp"
#include "pegtl/utf16.hpp"
#include "pegtl/utf32.hpp"
#include "pegtl/utf8.hpp"

#include "pegtl/argv_input.hpp"
#include "pegtl/buffer_input.hpp"
#include "pegtl/cstream_input.hpp"
#include "pegtl/file_input.hpp"
#include "pegtl/istream_input.hpp"
#include "pegtl/memory_input.hpp"
#include "pegtl/read_input.hpp"
#include "pegtl/string_input.hpp"

// The following are not included by
// default because they include <iostream>.

// #include "pegtl/analyze.hpp"

#endif
