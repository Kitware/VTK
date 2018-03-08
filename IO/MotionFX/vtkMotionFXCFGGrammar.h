/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMotionFXCFGGrammar.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkMotionFXCFGGrammar_h
#define vtkMotionFXCFGGrammar_h

// Internal header used by vtkMotionFXCFGReader.
// We define the various grammars here rather than clobbering the
// vtkMotionFXCFGReader.cxx.

#include <vtk_pegtl.h>

// for debugging
// #include <vtkpegtl/include/tao/pegtl/contrib/tracer.hpp>

namespace MotionFX
{
using namespace tao::pegtl;

//-----------------------------------------------------------------------------
// lets define some common rules here.
namespace Common
{
struct Sign : sor<one<'+'>, one<'-'>> {};
struct Exponent : seq<sor<one<'e'>, one<'E'> >, opt<Sign>, plus<digit>> {};
struct Number : seq<opt<Sign>,
                    sor<seq<plus<digit>, one<'.'>, star<digit>>,
                        seq<one<'.'>, plus<digit> >, plus<digit>>,
                    opt<Exponent>> {};

// delimiter for columns in files such as the position files
// this can be ',' separated by optional spaces or just spaces
struct Delimiter : sor<
                    seq<star<space>, one<','>, star<space>>,
                    plus<space> > {};
} // namespace Common

//-----------------------------------------------------------------------------
// rules for parsing a position file in legacy format, also called old rot.vel.
// format.
namespace LegacyPositionFile
{
using namespace Common;

// format: time CoMx CoMy CoMz Fx Fy Fz
struct Row
  : seq<star<space>, Number, Delimiter, Number, Delimiter, Number, Delimiter, Number,
      Delimiter, Number, Delimiter, Number, Delimiter, Number, star<space>> {};

struct Grammar : star<Row> {};
} // namepsace LegacyPositionFile

//-----------------------------------------------------------------------------
// rules for parsing a position file in orientations formation.
namespace OrientationsPositionFile
{
using namespace Common;

// format: time CoMx CoMy CoMz cosX cosY cosZ Orientation (radians)
struct Row :
  seq<star<space>, Number, Delimiter, Number, Delimiter, Number,
      Delimiter, Number, Delimiter, Number, Delimiter, Number,
      Delimiter, Number, Delimiter, Number, star<space>> {};

struct Grammar : star<Row> {};
} // namespace OrientationsPositionFile


//-----------------------------------------------------------------------------
// rules to parse CFG file.
namespace CFG
{
using namespace Common;

// Rule that matches a Comment. Consume everything on the line following a ';'
struct Comment : seq<string<';'>, until<eolf>> {};

// rule for "<num> <num>...."
struct Tuple : seq<one<'"'>, plus<pad<Number, space>>, one<'"'> > {};

struct WS_Required : sor<Comment, eol, plus<space>> {};
struct WS : star<WS_Required> {};

struct FileName : sor<list<list<identifier, one<'.'> >, one<'/'>>> {};
struct StringValue : sor<FileName, identifier> {};
struct DoubleValue : sor<Number, Tuple> {};
struct Value : sor<StringValue, DoubleValue> {};

struct ParameterName : identifier {};
struct Statement : seq<ParameterName, WS_Required, Value> {};

struct Motion : seq<TAO_PEGTL_STRING("motion"), WS, one<'{'>, WS, list<Statement, WS>, WS, one<'}'>> {};
struct Motions : seq<TAO_PEGTL_STRING("motions"), WS, one<'{'>, WS, list<Motion, WS>, WS, one<'}'>> {};

struct Lines : sor<Comment, space, Motions> {};

struct Grammar : star<Lines> {};

} // namespace CFG

} // namespace MotionFX

#endif
// VTK-HeaderTest-Exclude: vtkMotionFXCFGGrammar.h
