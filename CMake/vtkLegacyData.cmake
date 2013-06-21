# Make testing data from the legacy VTKData repository available.
# DO NOT ADD NEW DATA HERE!!
# TODO: Reference testing data from each module only as needed.
set(data "DATA{${VTK_TEST_INPUT_DIR}/,REGEX:.*}")
foreach(d
    Infovis
    Infovis/SQLite
    Infovis/XML
    Infovis/Images
    Infovis/DimacsGraphs
    Tango
    SemiDisk
    GIS
    many_blocks
    many_blocks/many_blocks
    Quadratic
    Dave_Karelitz_Small
    MetaIO
    libtiff
    AMR
    AMR/HierarchicalBoxDataset.v1.0
    AMR/HierarchicalBoxDataset.v1.1
    AMR/Enzo
    AMR/Enzo/DD0010
    UCD2D
    ex-blow_5
    chombo3d
    foot
    chi_field
    headsq
    Viewpoint
    EnSight
    )
  list(APPEND data "DATA{${VTK_TEST_INPUT_DIR}/${d}/,REGEX:.*}")
endforeach()
ExternalData_Expand_Arguments(VTKData _ ${data})
