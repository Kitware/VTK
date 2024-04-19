#[==[.rst:
*****************
vtkModuleGraphviz
*****************
#]==]
#[==[.rst:
.. cmake:command:: _vtk_module_graphviz_module_node

  Output a node in the graph |module-impl|

  Queries the properties for modules and generates the node for it in the graph
  and its outgoing dependency edges.
#]==]
function (_vtk_module_graphviz_module_node var module)
  get_property(_vtk_graphviz_file GLOBAL
    PROPERTY "_vtk_module_${module}_file")
  if (_vtk_graphviz_file)
    get_property(_vtk_graphviz_module_third_party GLOBAL
      PROPERTY "_vtk_module_${module}_third_party")
    get_property(_vtk_graphviz_module_exclude_wrap GLOBAL
      PROPERTY "_vtk_module_${module}_exclude_wrap")
    get_property(_vtk_graphviz_module_depends GLOBAL
      PROPERTY "_vtk_module_${module}_depends")
    get_property(_vtk_graphviz_module_private_depends GLOBAL
      PROPERTY "_vtk_module_${module}_private_depends")
    get_property(_vtk_graphviz_module_optional_depends GLOBAL
      PROPERTY "_vtk_module_${module}_optional_depends")
    get_property(_vtk_graphviz_module_implements GLOBAL
      PROPERTY "_vtk_module_${module}_implements")
    get_property(_vtk_graphviz_module_implementable GLOBAL
      PROPERTY "_vtk_module_${module}_implementable")
  else ()
    get_property(_vtk_graphviz_module_third_party
      TARGET    "${module}"
      PROPERTY  "INTERFACE_vtk_module_third_party")
    get_property(_vtk_graphviz_module_exclude_wrap
      TARGET    "${module}"
      PROPERTY  "INTERFACE_vtk_module_exclude_wrap")
    get_property(_vtk_graphviz_module_depends
      TARGET    "${module}"
      PROPERTY  "INTERFACE_vtk_module_depends")
    set(_vtk_graphviz_module_private_depends)
    set(_vtk_graphviz_module_optional_depends)
    get_property(_vtk_graphviz_module_implements
      TARGET    "${module}"
      PROPERTY  "INTERFACE_vtk_module_implements")
    get_property(_vtk_graphviz_module_implementable
      TARGET    "${module}"
      PROPERTY  "INTERFACE_vtk_module_implementable")
  endif ()

  if (_vtk_graphviz_module_third_party)
    set(_vtk_graphviz_shape "${_vtk_graphviz_third_party}")
  else ()
    set(_vtk_graphviz_shape "${_vtk_graphviz_first_party}")
  endif ()

  if (_vtk_graphviz_file)
    if (DEFINED "VTK_MODULE_USE_EXTERNAL_${module}" AND VTK_MODULE_USE_EXTERNAL_${module})
      set(_vtk_graphviz_fillcolor "${_vtk_graphviz_external}")
    else ()
      set(_vtk_graphviz_fillcolor "${_vtk_graphviz_internal}")
    endif ()
  else ()
    set(_vtk_graphviz_fillcolor "${_vtk_graphviz_external}")
  endif ()

  if (_vtk_graphviz_module_exclude_wrap)
    set(_vtk_graphviz_penwidth "${_vtk_graphviz_exclude_wrap}")
  else ()
    set(_vtk_graphviz_penwidth "${_vtk_graphviz_include_wrap}")
  endif ()

  if (_vtk_graphviz_module_implementable)
    set(_vtk_graphviz_color "${_vtk_graphviz_implementable}")
  else ()
    set(_vtk_graphviz_color "${_vtk_graphviz_not_implementable}")
  endif ()

  set(_vtk_graphviz_node_block "\"${module}\" [
    label=\"${module}\"
    shape=${_vtk_graphviz_shape}
    style=filled
    color=${_vtk_graphviz_color}
    fillcolor=${_vtk_graphviz_fillcolor}
    penwidth=${_vtk_graphviz_penwidth}
];\n")

  foreach (_vtk_graphviz_module_implement IN LISTS _vtk_graphviz_module_implements)
    string(APPEND _vtk_graphviz_node_block
      "\"${module}\" -> \"${_vtk_graphviz_module_implement}\" [style=${_vtk_graphviz_implements}, arrowhead=${_vtk_graphviz_required_depends}];\n")
  endforeach ()

  foreach (_vtk_graphviz_module_depend IN LISTS _vtk_graphviz_module_depends)
    string(APPEND _vtk_graphviz_node_block
      "\"${module}\" -> \"${_vtk_graphviz_module_depend}\" [style=${_vtk_graphviz_public_depends}, arrowhead=${_vtk_graphviz_required_depends}];\n")
  endforeach ()

  if (_vtk_graphviz_PRIVATE_DEPENDENCIES)
    foreach (_vtk_graphviz_module_private_depend IN LISTS _vtk_graphviz_module_private_depends)
      string(APPEND _vtk_graphviz_node_block
        "\"${module}\" -> \"${_vtk_graphviz_module_private_depend}\" [style=${_vtk_graphviz_private_depends}, arrowhead=${_vtk_graphviz_required_depends}];\n")
    endforeach ()

    foreach (_vtk_graphviz_module_optional_depend IN LISTS _vtk_graphviz_module_optional_depends)
      string(APPEND _vtk_graphviz_node_block
        "\"${module}\" -> \"${_vtk_graphviz_module_optional_depend}\" [style=${_vtk_graphviz_optional_depends}, arrowhead=${_vtk_graphviz_optional_depends}];\n")
    endforeach ()
  endif ()

  set("${var}" "${_vtk_graphviz_node_block}" PARENT_SCOPE)
endfunction ()

#[==[.rst:

.. cmake:command:: vtk_module_graphviz

  Generate graphviz output for a module dependency graph. |module-support|

  Information about the modules built and/or available may be dumped to a
  Graphviz `.dot` file.

  .. code-block:: cmake

     vtk_module_graphviz(
       MODULES   <module>...
       OUTPUT    <path>

       [PRIVATE_DEPENDENCIES <ON|OFF>]
       [KIT_CLUSTERS <ON|OFF>])

  * ``MODULES``: (Required) The modules to output information for.
  * ``OUTPUT``: (Required) A Graphviz file describing the modules built will
    be output to this path. Relative paths are rooted to ``CMAKE_BINARY_DIR``.
  * ``PRIVATE_DEPENDENCIES``: (Default ``ON``) Whether to draw private dependency
    edges or not..
  * ``KIT_CLUSTERS``: (Default ``OFF``) Whether to draw modules as part of a kit as
    a cluster or not.
#]==]
function (vtk_module_graphviz)
  cmake_parse_arguments(PARSE_ARGV 0 _vtk_graphviz
    ""
    "PRIVATE_DEPENDENCIES;KIT_CLUSTERS;OUTPUT"
    "MODULES")

  if (_vtk_graphviz_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_graphviz: "
      "${_vtk_graphviz_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _vtk_graphviz_OUTPUT)
    message(FATAL_ERROR
      "The `OUTPUT` argument is required.")
  endif ()

  if (NOT _vtk_graphviz_MODULES)
    message(FATAL_ERROR "No modules given to output.")
  endif ()

  if (NOT DEFINED _vtk_graphviz_PRIVATE_DEPENDENCIES)
    set(_vtk_graphviz_PRIVATE_DEPENDENCIES ON)
  endif ()

  if (NOT DEFINED _vtk_graphviz_KIT_CLUSTERS)
    set(_vtk_graphviz_KIT_CLUSTERS OFF)
  endif ()

  if (NOT IS_ABSOLUTE "${_vtk_graphviz_OUTPUT}")
    string(PREPEND _vtk_graphviz_OUTPUT "${CMAKE_BINARY_DIR}/")
  endif ()

  set(_vtk_graphviz_kits)
  set(_vtk_graphviz_no_kit_modules)

  if (_vtk_graphviz_KIT_CLUSTERS)
    # Get a list of all kits.
    foreach (_vtk_graphviz_module IN LISTS _vtk_graphviz_MODULES)
      get_property(_vtk_graphviz_kit GLOBAL
        PROPERTY "_vtk_module_${_vtk_graphviz_module}_kit")
      if (_vtk_graphviz_kit)
        list(APPEND _vtk_graphviz_kits
          "${_vtk_graphviz_kit}")
      else ()
        list(APPEND _vtk_graphviz_no_kit_modules
          "${_vtk_graphviz_module}")
      endif ()
    endforeach ()
    if (_vtk_graphviz_kits)
      list(REMOVE_DUPLICATES _vtk_graphviz_kits)
    endif ()
  else ()
    set(_vtk_graphviz_no_kit_modules "${_vtk_graphviz_MODULES}")
  endif ()

  # Shapes
  set(_vtk_graphviz_first_party "rectangle")
  set(_vtk_graphviz_third_party "cds")
  set(_vtk_graphviz_internal "\"/svg/white\"")
  set(_vtk_graphviz_external "\"/svg/cyan\"")

  # Border style
  set(_vtk_graphviz_include_wrap "5")
  set(_vtk_graphviz_exclude_wrap "1")
  set(_vtk_graphviz_implementable "\"/svg/darkorchid\"")
  set(_vtk_graphviz_not_implementable "\"/svg/coral\"")

  # Dependencies
  set(_vtk_graphviz_public_depends "solid")
  set(_vtk_graphviz_private_depends "dotted")
  set(_vtk_graphviz_implements "bold")

  set(_vtk_graphviz_required_depends "normal")
  set(_vtk_graphviz_optional_depends "empty")

  set(_vtk_graphviz_contents "strict digraph modules {\nclusterrank=local;\nrankdir=TB;\n")

  # Output modules not part of a kit.
  string(APPEND _vtk_graphviz_contents
    "subgraph \"modules_without_kits\" {\n")
  foreach (_vtk_graphviz_module IN LISTS _vtk_graphviz_no_kit_modules)
    _vtk_module_graphviz_module_node(_vtk_graphviz_node "${_vtk_graphviz_module}")
    string(APPEND _vtk_graphviz_contents
      "${_vtk_graphviz_node}\n")
  endforeach ()
  string(APPEND _vtk_graphviz_contents
    "}\n")

  # Output kits as clusters.
  foreach (_vtk_graphviz_kit IN LISTS _vtk_graphviz_kits)
    string(APPEND _vtk_graphviz_contents
      "subgraph \"cluster_${_vtk_graphviz_kit}\" {\nlabel=\"${_vtk_graphviz_kit}\"\n")

    get_property(_vtk_graphviz_kit_modules GLOBAL
      PROPERTY "_vtk_kit_${_vtk_graphviz_kit}_kit_modules")
    foreach (_vtk_graphviz_kit_module IN LISTS _vtk_graphviz_kit_modules)
      if (NOT _vtk_graphviz_kit_module IN_LIST _vtk_graphviz_MODULES)
        continue ()
      endif ()

      _vtk_module_graphviz_module_node(_vtk_graphviz_node "${_vtk_graphviz_kit_module}")
      string(APPEND _vtk_graphviz_contents
        "${_vtk_graphviz_node}\n")

    endforeach ()

    string(APPEND _vtk_graphviz_contents
      "}\n")
  endforeach ()

  # Write the key cluster.
  string(APPEND _vtk_graphviz_contents "
subgraph cluster_key {
  label=Key;
  subgraph cluster_party {
    first_party [
      label=\"First party\"
      shape=${_vtk_graphviz_first_party}
      style=filled
      color=${_vtk_graphviz_not_implementable}
      fillcolor=${_vtk_graphviz_internal}
      penwidth=${_vtk_graphviz_include_wrap}
    ];
    third_party [
      label=\"Third party\"
      shape=${_vtk_graphviz_third_party}
      style=filled
      color=${_vtk_graphviz_not_implementable}
      fillcolor=${_vtk_graphviz_internal}
      penwidth=${_vtk_graphviz_include_wrap}
    ];
  }
  subgraph cluster_whence {
    internal [
      label=\"Internal module\"
      shape=${_vtk_graphviz_first_party}
      style=filled
      color=${_vtk_graphviz_not_implementable}
      fillcolor=${_vtk_graphviz_internal}
      penwidth=${_vtk_graphviz_include_wrap}
    ];
    external [
      label=\"External module\"
      shape=${_vtk_graphviz_first_party}
      style=filled
      color=${_vtk_graphviz_not_implementable}
      fillcolor=${_vtk_graphviz_external}
      penwidth=${_vtk_graphviz_include_wrap}
    ];
  }
  subgraph cluster_wrapping {
    include_wrap [
      label=\"Wrappable\"
      shape=${_vtk_graphviz_first_party}
      style=filled
      color=${_vtk_graphviz_not_implementable}
      fillcolor=${_vtk_graphviz_internal}
      penwidth=${_vtk_graphviz_include_wrap}
    ];
    exclude_wrap [
      label=\"Not wrappable\"
      shape=${_vtk_graphviz_first_party}
      style=filled
      color=${_vtk_graphviz_not_implementable}
      fillcolor=${_vtk_graphviz_internal}
      penwidth=${_vtk_graphviz_exclude_wrap}
    ];
  }
  subgraph cluster_implementable {
    implementable [
      label=\"Implementable\"
      shape=${_vtk_graphviz_first_party}
      style=filled
      color=${_vtk_graphviz_implementable}
      fillcolor=${_vtk_graphviz_internal}
      penwidth=${_vtk_graphviz_include_wrap}
    ];
    not_implementable [
      label=\"Not implementable\"
      shape=${_vtk_graphviz_first_party}
      style=filled
      color=${_vtk_graphviz_not_implementable}
      fillcolor=${_vtk_graphviz_internal}
      penwidth=${_vtk_graphviz_include_wrap}
    ];
  }
  subgraph cluster_dependencies {
    dependent [
      label=\"Dependent\"
      shape=${_vtk_graphviz_first_party}
      style=filled
      color=${_vtk_graphviz_not_implementable}
      fillcolor=${_vtk_graphviz_internal}
      penwidth=${_vtk_graphviz_include_wrap}
    ];
    private_dependee [
      label=\"Private Dependee\"
      shape=${_vtk_graphviz_first_party}
      style=filled
      color=${_vtk_graphviz_not_implementable}
      fillcolor=${_vtk_graphviz_internal}
      penwidth=${_vtk_graphviz_include_wrap}
    ];
    optional_dependee [
      label=\"Optional Dependee\"
      shape=${_vtk_graphviz_first_party}
      style=filled
      color=${_vtk_graphviz_not_implementable}
      fillcolor=${_vtk_graphviz_internal}
      penwidth=${_vtk_graphviz_include_wrap}
    ];
    public_dependee [
      label=\"Public Dependee\"
      shape=${_vtk_graphviz_first_party}
      style=filled
      color=${_vtk_graphviz_not_implementable}
      fillcolor=${_vtk_graphviz_internal}
      penwidth=${_vtk_graphviz_include_wrap}
    ];
    implemented [
      label=\"Implemented\"
      shape=${_vtk_graphviz_first_party}
      style=filled
      color=${_vtk_graphviz_implementable}
      fillcolor=${_vtk_graphviz_internal}
      penwidth=${_vtk_graphviz_include_wrap}
    ];
    dependent -> private_dependee [style=${_vtk_graphviz_private_depends}, arrowhead=${_vtk_graphviz_required_depends}];
    dependent -> optional_dependee [style=${_vtk_graphviz_private_depends}, arrowhead=${_vtk_graphviz_optional_depends}];
    dependent -> public_dependee [style=${_vtk_graphviz_public_depends}, arrowhead=${_vtk_graphviz_required_depends}];
    dependent -> implemented [style=${_vtk_graphviz_implements}, arrowhead=${_vtk_graphviz_required_depends}];
  }
}
")
  string(APPEND _vtk_graphviz_contents "}\n")

  #file(GENERATE
  #  OUTPUT  "${_vtk_graphviz_OUTPUT}"
  #  CONTENT "${_vtk_graphviz_contents}")
  file(WRITE "${_vtk_graphviz_OUTPUT}" "${_vtk_graphviz_contents}")
endfunction ()
