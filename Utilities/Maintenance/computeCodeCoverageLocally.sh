#!/bin/bash
#==========================================================================
#
#   Copyright Insight Software Consortium
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#          http://www.apache.org/licenses/LICENSE-2.0.txt
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#==========================================================================*/
if [[ $1 == "--help" ]]
then
echo "      "
echo "  How to use this script      "
echo "      "
echo "   0)  Use Linux or Mac, "
echo "        - install lcov"
echo "        - use the gcc compiler"
echo "      "
echo "   1)  Add the CMake flags:      "
echo "      "
echo "       CMAKE_CXX_FLAGS:STRING=-g -O0  -fprofile-arcs -ftest-coverage     "
echo "       CMAKE_C_FLAGS:STRING= -g -O0  -fprofile-arcs -ftest-coverage      "
echo "      "
echo "   2)  Compile VTK for Debug      "
echo "      "
echo "                     CMAKE_BUILD_TYPE  Debug      "
echo "      "
echo "   3)  From the TOP of the binary directory type the path to this script "
echo "       in the VTK source tree.  This will run all tests in VTK and generate "
echo "       code coverage for the entire toolkit.  The code coverage report will "
echo "       be generated in HTML and can be opened with your favorite browser.     "
echo "    "
echo "       For example, "
echo "    "
echo "          In Linux, you can do        firefox  ./index.html     "
echo "          In Mac,   you can do        open     ./index.html     "
echo "    "

else

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

#==========================================================================
echo -n "Please wait while lcov deletes all .da files in . and subdirectories..."
lcov --quiet --directory . --zerocounters
ctest
echo "Done"
echo -n "Please wait while lcov captures the coverage data..."
#
# Some compilers (e.g. clang) place the .gcda files in the wrong directory
#
${SCRIPT_DIR}/fixcoverage.py
cd $destinationdir
lcov --quiet --directory . --capture --output-file app.info
echo "Done"
echo -n "Please wait while lcov removes coverage for some files..."
lcov --quiet --remove app.info '*Instantiator.*' 'vtkType*Array.*'  '*Tcl.cxx' '*TCLInit.cxx' '*Python.cxx' '*Wrapping*' '*Examples*' '*Testing*'  '*Utilities*' '*_s.cxx' '*_vs*.cxx' '*_fs*.cxx' '*GS.cxx' '*VS.cxx' '*FS.cxx' '*FP*.cxx' '*VP*.cxx' 'vtkgl.cxx' '/usr/*' --output-file  app.info2
echo "Done"
echo -n "Please wait while genhtml generates an HTML view from lcov coverage data files..."
genhtml --quiet app.info2 >/dev/null 2>&1
echo "Done"
echo "To view results on Linux, type firefox ./index.html"
echo "To view results on Mac, type open ./index.html"

fi
