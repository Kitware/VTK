perl doc_header2doxygen.pl --to ../../../VTK-doxygen
perl doc_version.pl --logo "vtk-logo.gif" --to ../../../VTK-doxygen
perl doc_class2example.pl --link http://public.kitware.com/cgi-bin/cvsweb.cgi/~checkout~/VTK/Utilities/Doxygen --to ../../../VTK-doxygen
perl doc_class2example.pl --dirmatch "^Testing$" --label "Tests" --title "Class To Tests" --store "doc_class2tests.dox" --unique "t" --link http://public.kitware.com/cgi-bin/cvsweb.cgi/~checkout~/VTK/Utilities/Doxygen --to ../../../VTK-doxygen
perl doc_index.pl --to ../../../VTK-doxygen
doxygen
perl doc_rmpath.pl --to ../../../VTK-doxygen --html ../../../doc/html

