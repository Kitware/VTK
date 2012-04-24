#!/usr/bin/env perl

# This script tries to find deprecated attribute data classes and
# methods and warns the user whenever it finds them. It also suggests
# possible modification to bring code up to date.

use Getopt::Long;

if (!GetOptions("language:s" => \$language,
		"verbose" => \$verbose,
		"help" => \$help,
		"print-messages" => \$print))
{
    die;
}

if (!$language)
{
    $language = "c++";
}

if ( !$print && ($#ARGV < 0 || $help) )
{
    print "Usage: $0 [--language {c++ | tcl | python | java}] ",
          "[--verbose] [--help] [--print-messages] file1 [file2 ...]\n";
    exit;
}


%cxxmessageids = (
		  'vtkScalars\*' => 0,
		  'vtkVectors\*' => 1,
		  'vtkNormals\*' => 2,
		  'vtkTCoords\*' => 3,
		  'vtkTensors\*' => 4,
		  'vtkScalars::New[ \t]*\(' => 5,
		  'vtkVectors::New[ \t]*\(' => 6,
		  'vtkNormals::New[ \t]*\(' => 7,
		  'vtkTCoords::New[ \t]*\(' => 8,
		  'vtkTensors::New[ \t]*\(' => 9,
		  '->GetScalar[ \t]*\(' => 10,
		  '->GetVector[ \t]*\(' => 11,
		  '->GetNormal[ \t]*\(' => 12,
		  '->GetTCoord[ \t]*\(' => 13,
		  '->GetTensor[ \t]*\(' => 14,
		  '->SetScalar[ \t]*\(' => 15,
		  '->SetVector[ \t]*\(' => 16,
		  '->SetNormal[ \t]*\(' => 17,
		  '->SetTCoord[ \t]*\(' => 18,
		  '->SetTensor[ \t]*\(' => 19,
		  '->GetScalars[ \t]*\([a-zA-Z]+.*\)' => 20,
		  '->GetVectors[ \t]*\([a-zA-Z]+.*\)' => 21,
		  '->GetNormals[ \t]*\([a-zA-Z]+.*\)' => 22,
		  '->GetTCoords[ \t]*\([a-zA-Z]+.*\)' => 23,
		  '->GetTensors[ \t]*\([a-zA-Z]+.*\)' => 24,
		  '->InsertScalar[ \t]*\(' => 25,
		  '->InsertVector[ \t]*\(' => 26,
		  '->InsertNormal[ \t]*\(' => 27,
		  '->InsertTCoord[ \t]*\(' => 28,
		  '->InsertTensor[ \t]*\(' => 29,
		  '->InsertNextScalar[ \t]*\(' => 30,
		  '->InsertNextVector[ \t]*\(' => 31,
		  '->InsertNextNormal[ \t]*\(' => 32,
		  '->InsertNextTCoord[ \t]*\(' => 33,
		  '->InsertNextTensor[ \t]*\(' => 34,
		  '->GetActiveScalars' => 35,
		  '->GetActiveVectors' => 36,
		  '->GetActiveNormals' => 37,
		  '->GetActiveTCoords' => 38,
		  '->GetActiveTensors' => 39,
		  '->GetNumberOfScalars' => 40,
		  '->GetNumberOfVectors' => 41,
		  '->GetNumberOfNormals' => 42,
		  '->GetNumberOfTCoords' => 43,
		  '->GetNumberOfTensors' => 44,
		  '->SetNumberOfScalars' => 40,
		  '->SetNumberOfVectors' => 41,
		  '->SetNumberOfNormals' => 42,
		  '->SetNumberOfTCoords' => 43,
		  '->SetNumberOfTensors' => 44,
		  );

%tclmessageids = (
		  'vtkScalars ' => 5,
		  'vtkVectors ' => 6,
		  'vtkNormals ' => 7,
		  'vtkTCoords ' => 8,
		  'vtkTensors ' => 9,
		  '[ \t]+GetScalar([ \t]*$|[^a-zA-Z0-9])' => 10,
		  '[ \t]+GetVector([ \t]*$|[^a-zA-Z0-9])' => 11,
		  '[ \t]+GetNormal([ \t]*$|[^a-zA-Z0-9])' => 12,
		  '[ \t]+GetTCoord([ \t]*$|[^a-zA-Z0-9])' => 13,
		  '[ \t]+GetTensor([ \t]*$|[^a-zA-Z0-9])' => 14,
		  '[ \t]+SetScalar([ \t]*$|[^a-zA-Z0-9])' => 15,
		  '[ \t]+SetVector([ \t]*$|[^a-zA-Z0-9])' => 16,
		  '[ \t]+SetNormal([ \t]*$|[^a-zA-Z0-9])' => 17,
		  '[ \t]+SetTCoord([ \t]*$|[^a-zA-Z0-9])' => 18,
		  '[ \t]+SetTensor([ \t]*$|[^a-zA-Z0-9])' => 19,
		  '[ \t]GetScalars[ \t]*[a-zA-Z]+.*' => 20,
		  '[ \t]GetVectors[ \t]*[a-zA-Z]+.*' => 21,
		  '[ \t]GetNormals[ \t]*[a-zA-Z]+.*' => 22,
		  '[ \t]GetTCoords[ \t]*[a-zA-Z]+.*' => 23,
		  '[ \t]GetTensors[ \t]*[a-zA-Z]+.*' => 24,
		  '[ \t]InsertScalar[ \t]+' => 25,
		  '[ \t]InsertVector[ \t]+' => 26,
		  '[ \t]InsertNormal[ \t]+' => 27,
		  '[ \t]InsertTCoord[ \t]+' => 28,
		  '[ \t]InsertTensor[ \t]+' => 29,
		  '[ \t]InsertNextScalar[ \t]+' => 30,
		  '[ \t]InsertNextVector[ \t]+' => 31,
		  '[ \t]InsertNextNormal[ \t]+' => 32,
		  '[ \t]InsertNextTCoord[ \t]+' => 33,
		  '[ \t]InsertNextTensor[ \t]+' => 34,
		  '[ \t]GetActiveScalars' => 35,
		  '[ \t]GetActiveVectors' => 36,
		  '[ \t]GetActiveNormals' => 37,
		  '[ \t]GetActiveTCoords' => 38,
		  '[ \t]GetActiveTensors' => 39,
		  '[ \t]GetNumberOfScalars' => 40,
		  '[ \t]GetNumberOfVectors' => 41,
		  '[ \t]GetNumberOfNormals' => 42,
		  '[ \t]GetNumberOfTCoords' => 43,
		  '[ \t]GetNumberOfTensors' => 44,
		  '[ \t]SetNumberOfScalars' => 40,
		  '[ \t]SetNumberOfVectors' => 41,
		  '[ \t]SetNumberOfNormals' => 42,
		  '[ \t]SetNumberOfTCoords' => 43,
		  '[ \t]SetNumberOfTensors' => 44,
		  );

%pythonmessageids = (
		     'vtkScalars[ \t]*\(\)' => 5,
		     'vtkVectors[ \t]*\(\)' => 6,
		     'vtkNormals[ \t]*\(\)' => 7,
		     'vtkTCoords[ \t]*\(\)' => 8,
		     'vtkTensors[ \t]*\(\)' => 9,
		     '\.GetScalar[ \t]*\(' => 10,
		     '\.GetVector[ \t]*\(' => 11,
		     '\.GetNormal[ \t]*\(' => 12,
		     '\.GetTCoord[ \t]*\(' => 13,
		     '\.GetTensor[ \t]*\(' => 14,
		     '\.SetScalar[ \t]*\(' => 15,
		     '\.SetVector[ \t]*\(' => 16,
		     '\.SetNormal[ \t]*\(' => 17,
		     '\.SetTCoord[ \t]*\(' => 18,
		     '\.SetTensor[ \t]*\(' => 19,
		     '.GetScalars[ \t]*\([a-zA-Z]+.*\)' => 20,
		     '.GetVectors[ \t]*\([a-zA-Z]+.*\)' => 21,
		     '.GetNormals[ \t]*\([a-zA-Z]+.*\)' => 22,
		     '.GetTCoords[ \t]*\([a-zA-Z]+.*\)' => 23,
		     '.GetTensors[ \t]*\([a-zA-Z]+.*\)' => 24,
		     '.InsertScalar[ \t]*\(' => 25,
		     '.InsertVector[ \t]*\(' => 26,
		     '.InsertNormal[ \t]*\(' => 27,
		     '.InsertTCoord[ \t]*\(' => 28,
		     '.InsertTensor[ \t]*\(' => 29,
		     '.InsertNextScalar[ \t]*\(' => 30,
		     '.InsertNextVector[ \t]*\(' => 31,
		     '.InsertNextNormal[ \t]*\(' => 32,
		     '.InsertNextTCoord[ \t]*\(' => 33,
		     '.InsertNextTensor[ \t]*\(' => 34,
		     '.GetActiveScalars' => 35,
		     '.GetActiveVectors' => 36,
		     '.GetActiveNormals' => 37,
		     '.GetActiveTCoords' => 38,
		     '.GetActiveTensors' => 39,
		     '.GetNumberOfScalars' => 40,
		     '.GetNumberOfVectors' => 41,
		     '.GetNumberOfNormals' => 42,
		     '.GetNumberOfTCoords' => 43,
		     '.GetNumberOfTensors' => 44,
		     '.SetNumberOfScalars' => 40,
		     '.SetNumberOfVectors' => 41,
		     '.SetNumberOfNormals' => 42,
		     '.SetNumberOfTCoords' => 43,
		     '.SetNumberOfTensors' => 44,
		     );

%javamessageids = (
		   'new[ \t]+vtkScalars[ \t]*\(\)' => 5,
		   'new[ \t]+vtkVectors[ \t]*\(\)' => 6,
		   'new[ \t]+vtkNormals[ \t]*\(\)' => 7,
		   'new[ \t]+vtkTCoords[ \t]*\(\)' => 8,
		   'new[ \t]+vtkTensors[ \t]*\(\)' => 9,
		   '\.GetScalar[ \t]*\(' => 10,
		   '\.GetVector[ \t]*\(' => 11,
		   '\.GetNormal[ \t]*\(' => 12,
		   '\.GetTCoord[ \t]*\(' => 13,
		   '\.GetTensor[ \t]*\(' => 14,
		   '\.SetScalar[ \t]*\(' => 15,
		   '\.SetVector[ \t]*\(' => 16,
		   '\.SetNormal[ \t]*\(' => 17,
		   '\.SetTCoord[ \t]*\(' => 18,
		   '\.SetTensor[ \t]*\(' => 19,
		   '.GetScalars[ \t]*\([a-zA-Z]+.*\)' => 20,
		   '.GetVectors[ \t]*\([a-zA-Z]+.*\)' => 21,
		   '.GetNormals[ \t]*\([a-zA-Z]+.*\)' => 22,
		   '.GetTCoords[ \t]*\([a-zA-Z]+.*\)' => 23,
		   '.GetTensors[ \t]*\([a-zA-Z]+.*\)' => 24,
		   '.InsertScalar[ \t]*\(' => 25,
		   '.InsertVector[ \t]*\(' => 26,
		   '.InsertNormal[ \t]*\(' => 27,
		   '.InsertTCoord[ \t]*\(' => 28,
		   '.InsertTensor[ \t]*\(' => 29,
		   '.InsertNextScalar[ \t]*\(' => 30,
		   '.InsertNextVector[ \t]*\(' => 31,
		   '.InsertNextNormal[ \t]*\(' => 32,
		   '.InsertNextTCoord[ \t]*\(' => 33,
		   '.InsertNextTensor[ \t]*\(' => 34,
		   '.GetActiveScalars' => 35,
		   '.GetActiveVectors' => 36,
		   '.GetActiveNormals' => 37,
		   '.GetActiveTCoords' => 38,
		   '.GetActiveTensors' => 39,
		   '.GetNumberOfScalars' => 40,
		   '.GetNumberOfVectors' => 41,
		   '.GetNumberOfNormals' => 42,
		   '.GetNumberOfTCoords' => 43,
		   '.GetNumberOfTensors' => 44,
		   '.SetNumberOfScalars' => 40,
		   '.SetNumberOfVectors' => 41,
		   '.SetNumberOfNormals' => 42,
		   '.SetNumberOfTCoords' => 43,
		   '.SetNumberOfTensors' => 44,
		   );

if ($language eq "c++")
{
    %messageids = %cxxmessageids;
}
elsif($language eq "tcl")
{
    %messageids = %tclmessageids;
}
elsif($language eq "python")
{
    %messageids = %pythonmessageids;
}
elsif($language eq "java")
{
    %messageids = %javamessageids;
}
else
{
    die "Unsupported language: $language.\n";
}
@messages = (
	     "> Encountered vtkScalars* : vtkScalars has been\n".
	     "> deprecated. You should use vtkDataArray or one\n".
	     "> of it's subclasses.\n",
	     "> Encountered vtkVectors* : vtkVectors has been\n".
	     "> deprecated. You should use vtkDataArray or one\n".
	     "> of it's subclasses.\n",
	     "> Encountered vtkNormals* : vtkNormals has been\n".
	     "> deprecated. You should use vtkDataArray or one\n".
	     "> of it's subclasses.\n",
	     "> Encountered vtkTCoords* : vtkTCoords has been\n".
	     "> deprecated. You should use vtkDataArray or one\n".
	     "> of it's subclasses.\n",
	     "> Encountered vtkTensors* : vtkTensors has been\n".
	     "> deprecated. You should use vtkDataArray or one\n".
	     "> of it's subclasses.\n",
	     "> Encountered vtkScalars constructor: vtkScalars has been\n".
	     "> deprecated. You should use vtkDataArray or one\n".
	     "> of it's subclasses.\n",
	     "> Encountered vtkVectors constructor: vtkVectors has been\n".
	     "> deprecated. You should use vtkDataArray or one\n".
	     "> of it's subclasses. Note that you have to explicitly set the\n".
	     "> number of components. For example (in Tcl):\n".
	     "> vtkFloatArray vectors\n".
	     "> vectors SetNumberOfComponents 3\n",
	     "> Encountered vtkNormals constructor: vtkNormals has been\n".
	     "> deprecated. You should use vtkDataArray or one\n".
	     "> of it's subclasses.Note that you have to explicitly set the\n".
	     "> number of components. For example (in Tcl):\n".
	     "> vtkFloatArray normals\n".
	     "> normals SetNumberOfComponents 3\n",
	     "> Encountered vtkTCoords constructor: vtkTCoords has been\n".
	     "> deprecated. You should use vtkDataArray or one\n".
	     "> of it's subclasses.Note that you have to explicitly set the\n".
	     "> number of components. For example (in Tcl):\n".
	     "> vtkFloatArray tc\n".
	     "> tc SetNumberOfComponents 2\n",
	     "> Encountered vtkTensors constructor: vtkTensors has been\n".
	     "> deprecated. You should use vtkDataArray or one\n".
	     "> of it's subclasses.Note that you have to explicitly set the\n".
	     "> number of components. For example (in Tcl):\n".
	     "> vtkFloatArray tensors\n".
	     "> tensors SetNumberOfComponents 9\n",
	     "> Encountered vtkScalars::GetScalar() : vtkScalars has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use GetComponent(id, component)\n".
	     "> instead of GetScalar(id)\n" ,
	     "> Encountered vtkVectors::GetVector(): vtkVectors has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use GetTuple(id)\n".
	     "> instead of GetVector(id)\n" ,
	     "> Encountered vtkNormals::GetNormal(): vtkNormals has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use GetTuple(id)\n".
	     "> instead of GetNormal(id)\n" ,
	     "> Encountered vtkTCoords::GetTCoord(): vtkTCoords has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use GetTuple(id)\n".
	     "> instead of GetTCoord(id)\n" ,
	     "> Encountered vtkTensors::GetTensors(): vtkTensors has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use GetTuple(id)\n".
	     "> instead of GetTensor(id)\n" ,
	     "> Encountered vtkScalars::SetScalar() : vtkScalars has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use SetComponent(id, component, value)\n".
	     "> instead of GetScalar(id)\n" ,
	     "> Encountered vtkVectors::SetVector(): vtkVectors has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use SetTuple(id, v)\n".
	     "> instead of SetVector(id, v)\n" ,
	     "> Encountered vtkNormals::SetNormal(): vtkNormals has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use SetTuple(id,v)\n".
	     "> instead of SetNormal(id,v)\n" ,
	     "> Encountered vtkTCoords::SetTCoord(): vtkTCoords has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use SetTuple(id,v)\n".
	     "> instead of SetTCoord(id,v)\n" ,
	     "> Encountered vtkTensors::SetTensors(): vtkTensors has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use SetTuple(id,v)\n".
	     "> instead of GetTensor(id,v)\n" ,
	     "> Encountered vtkScalars::GetScalars() : vtkScalars has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use GetTuples()\n".
	     "> instead of GetScalars(id). Note that, unlike GetScalars(),\n".
	     "> GetTuples() requires that enough memory is allocated in the\n".
	     "> target array. See the documentation of vtkDataArray for more\n".
	     "> information.\n",
	     "> Encountered vtkVectors::GetVectors() : vtkVectors has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use GetTuples()\n".
	     "> instead of GetVectors(id). Note that, unlike GetVectors(),\n".
	     "> GetTuples() requires that enough memory is allocated in the\n".
	     "> target array. See the documentation of vtkDataArray for more\n".
	     "> information.\n",
	     "> Encountered vtkNormals::GetNormals() : vtkNormals has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use GetTuples()\n".
	     "> instead of GetNormals(id). Note that, unlike GetNormals(),\n".
	     "> GetTuples() requires that enough memory is allocated in the\n".
	     "> target array. See the documentation of vtkDataArray for more\n".
	     "> information.\n",
	     "> Encountered vtkTCoords::GetTCoords() : vtkTCoords has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use GetTuples()\n".
	     "> instead of GetTCoords(id). Note that, unlike GetTCoords(),\n".
	     "> GetTuples() requires that enough memory is allocated in the\n".
	     "> target array. See the documentation of vtkDataArray for more\n".
	     "> information.\n",
	     "> Encountered vtkTensors::GetTensors() : vtkTensors has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use GetTuples()\n".
	     "> instead of GetTensors(id). Note that, unlike GetTensors(),\n".
	     "> GetTuples() requires that enough memory is allocated in the\n".
	     "> target array. See the documentation of vtkDataArray for more\n".
	     "> information.\n",
	     "> Encountered vtkScalars::InsertScalar() :  vtkScalars has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use InsertComponent(),\n ".
	     "> InsertValue() or InsertTuple1() instead of InsertScalar()\n",
	     "> Encountered vtkVectors::InsertVector() :  vtkVectors has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use InsertTuple(),\n ".
	     "> or InsertTuple3() instead of InsertVector()\n",
	     "> Encountered vtkNormals::InsertNormal() :  vtkNormals has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use InsertTuple(),\n ".
	     "> or InsertTuple3() instead of InsertNormal()\n",
	     "> Encountered vtkTCoords::InsertTCoord() :  vtkTCoords has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use InsertTuple(),\n ".
	     "> or InsertTuple2() instead of InsertTCoord()\n",
	     "> Encountered vtkTensors::InsertTensor() :  vtkTensors has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use InsertTuple(),\n ".
	     "> or InsertTuple9() instead of InsertTensor()\n",
	     "> Encountered vtkScalars::InsertNextScalar() :  vtkScalars has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use InsertNextComponent(),\n ".
	     "> InsertNextValue() or InsertNextTuple1() instead of InsertNextScalar()\n",
	     "> Encountered vtkVectors::InsertNextVector() :  vtkVectors has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use InsertNextTuple(),\n ".
	     "> or InsertNextTuple3() instead of InsertNextVector()\n",
	     "> Encountered vtkNormals::InsertNextNormal() :  vtkNormals has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use InsertNextTuple(),\n ".
	     "> or InsertNextTuple3() instead of InsertNextNormal()\n",
	     "> Encountered vtkTCoords::InsertNextTCoord() :  vtkTCoords has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use InsertNextTuple(),\n ".
	     "> or InsertNextTuple2() instead of InsertNextTCoord()\n",
	     "> Encountered vtkTensors::InsertNextTensor() :  vtkTensors has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use InsertNextTuple(),\n ".
	     "> or InsertNextTuple9() instead of InsertNextTensor()\n",
	     "> Replace vtkDataSetAttributes::GetActiveScalars() with \n".
	     "> vtkDataSetAttributes::GetScalars()\n",
	     "> Replace vtkDataSetAttributes::GetActiveVectors() with \n".
	     "> vtkDataSetAttributes::GetVectors()\n",
	     "> Replace vtkDataSetAttributes::GetActiveNormals() with \n".
	     "> vtkDataSetAttributes::GetNormals()\n",
	     "> Replace vtkDataSetAttributes::GetActiveTCoords() with \n".
	     "> vtkDataSetAttributes::GetTCoords()\n",
	     "> Replace vtkDataSetAttributes::GetActiveTensors() with \n".
	     "> vtkDataSetAttributes::GetTensors()\n",
	     "> Encountered vtkScalars::Set/GetNumberOfScalars() : vtkScalars has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use Set/GetNumberOfTuples()\n".
	     "> instead of Set/GetNumberOfScalars().\n",
	     "> Encountered vtkVectors::Set/GetNumberOfVectors() : vtkVectors has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use Set/GetNumberOfTuples()\n".
	     "> instead of Set/GetNumberOfVectors().\n",
	     "> Encountered vtkNormals::Set/GetNumberOfNormals() : vtkNormals has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use Set/GetNumberOfTuples()\n".
	     "> instead of Set/GetNumberOfNormals().\n",
	     "> Encountered vtkTCoords::Set/GetNumberOfTCoords() : vtkTCoords has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use Set/GetNumberOfTuples()\n".
	     "> instead of Set/GetNumberOfTCoords().\n",
	     "> Encountered vtkTensors::Set/GetNumberOfTensors() : vtkTensors has been\n".
	     "> deprecated. You should replace this object with a\n".
	     "> vtkDataArray or one of it's subclasses and use Set/GetNumberOfTuples()\n".
	     "> instead of Set/GetNumberOfTensors().\n",
	     );


if ( $print )
{
    $i = 0;
    foreach $key (@messages)
    {
	print "Message id $i:\n";
	print $key, "\n";
	$i++;
    }
    exit 0;
}

foreach $filename (@ARGV)
{
    open(FPTR, "<$filename") or die "Could not open file $filename";
    if ($verbose)
    {
	print "Processing file: $filename\n";
    }
    $i = 1;
    while (<FPTR>)
    {
	$line = $_;
	foreach $key (keys %messageids)
	{
	    if ( $line =~ m($key) )
	    {
		chomp $line;
		if ($verbose)
		{
		    print ">> File $filename line $i: ",
		          "\n$messages[$messageids{$key}]\n";
		}
		else
		{
		    print ">> File $filename line $i: ",
	              	  "Message $messageids{$key}\n";
		}
		last;
	    }
	}
	$i++;
    }
}

