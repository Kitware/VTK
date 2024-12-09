set(headers
  include/vtknlohmann/adl_serializer.hpp
  include/vtknlohmann/byte_container_with_subtype.hpp
  include/vtknlohmann/json.hpp
  include/vtknlohmann/json_fwd.hpp
  include/vtknlohmann/ordered_map.hpp

  include/vtknlohmann/detail/abi_macros.hpp
  include/vtknlohmann/detail/conversions/from_json.hpp
  include/vtknlohmann/detail/conversions/to_chars.hpp
  include/vtknlohmann/detail/conversions/to_json.hpp
  include/vtknlohmann/detail/exceptions.hpp
  include/vtknlohmann/detail/hash.hpp
  include/vtknlohmann/detail/input/binary_reader.hpp
  include/vtknlohmann/detail/input/input_adapters.hpp
  include/vtknlohmann/detail/input/json_sax.hpp
  include/vtknlohmann/detail/input/lexer.hpp
  include/vtknlohmann/detail/input/parser.hpp
  include/vtknlohmann/detail/input/position_t.hpp
  include/vtknlohmann/detail/iterators/internal_iterator.hpp
  include/vtknlohmann/detail/iterators/iter_impl.hpp
  include/vtknlohmann/detail/iterators/iteration_proxy.hpp
  include/vtknlohmann/detail/iterators/iterator_traits.hpp
  include/vtknlohmann/detail/iterators/json_reverse_iterator.hpp
  include/vtknlohmann/detail/iterators/primitive_iterator.hpp
  include/vtknlohmann/detail/json_custom_base_class.hpp
  include/vtknlohmann/detail/json_pointer.hpp
  include/vtknlohmann/detail/json_ref.hpp
  include/vtknlohmann/detail/macro_scope.hpp
  include/vtknlohmann/detail/macro_unscope.hpp
  include/vtknlohmann/detail/meta/call_std/begin.hpp
  include/vtknlohmann/detail/meta/call_std/end.hpp
  include/vtknlohmann/detail/meta/cpp_future.hpp
  include/vtknlohmann/detail/meta/detected.hpp
  include/vtknlohmann/detail/meta/identity_tag.hpp
  include/vtknlohmann/detail/meta/is_sax.hpp
  include/vtknlohmann/detail/meta/std_fs.hpp
  include/vtknlohmann/detail/meta/type_traits.hpp
  include/vtknlohmann/detail/meta/void_t.hpp
  include/vtknlohmann/detail/output/binary_writer.hpp
  include/vtknlohmann/detail/output/output_adapters.hpp
  include/vtknlohmann/detail/output/serializer.hpp
  include/vtknlohmann/detail/string_concat.hpp
  include/vtknlohmann/detail/string_escape.hpp
  include/vtknlohmann/detail/value_t.hpp

  include/vtknlohmann/thirdparty/hedley/hedley.hpp
  include/vtknlohmann/thirdparty/hedley/hedley_undef.hpp)

vtk_module_add_module(VTK::nlohmannjson
  HEADER_ONLY HEADER_DIRECTORIES
  HEADERS ${headers}
  HEADERS_SUBDIR vtknlohmannjson)
vtk_module_include(VTK::nlohmannjson
  INTERFACE
    # Required because the files are included via `<nlohmannjson/…>` names.
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${_vtk_build_HEADERS_DESTINATION}/vtknlohmannjson/include>")
