import vtk
import vtk.test.Testing

tree_data = "((((((((ahli:0.1308887296,allogus:0.1308887296):0.109078899,rubribarbus:0.2399676286):0.3477240729,imias:0.5876917015):0.1279779191,((((sagrei:0.2576204042,(bremeri:0.1097436524,quadriocellifer:0.1097436524):0.1478767518):0.06150599843,ophiolepis:0.3191264027):0.08721921759,mestrei:0.4063456203):0.1298140501,(((jubar:0.1188659524,homolechis:0.1188659524):0.09052271908,confusus:0.2093886715):0.04215577182,guafe:0.2515444433):0.2846152271):0.1795099503):0.1377237125,((((garmani:0.2000335809,opalinus:0.2000335809):0.01968719882,grahami:0.2197207797):0.2178099139,valencienni:0.4375306936):0.1226128606,(lineatopus:0.4713710622,reconditus:0.4713710622):0.08877249208):0.2932497789):0.06703519523,(((evermanni:0.2135202715,stratulus:0.2135202715):0.3521520586,(((krugi:0.3267560653,pulchellus:0.3267560653):0.1312930371,(gundlachi:0.3864660126,poncensis:0.3864660126):0.0715830898):0.03035078065,(cooki:0.395288192,cristatellus:0.395288192):0.09311169105):0.07727244709):0.1495575755,(((brevirostris:0.2757423466,(caudalis:0.1704974619,marron:0.1704974619):0.1052448847):0.02672749452,websteri:0.3024698411):0.09835748687,distichus:0.400827328):0.3144025776):0.2051986227):0.03488732303,(((barbouri:0.8021085018,(((alumina:0.2681076879,semilineatus:0.2681076879):0.219367178,olssoni:0.4874748658):0.2622236606,(etheridgei:0.5883072151,(fowleri:0.3770938401,insolitus:0.3770938401):0.211213375):0.1613913113):0.05240997539):0.0672038969,((((whitemani:0.3420271265,((haetianus:0.2669834072,breslini:0.2669834072):0.06962183477,((armouri:0.1483909526,cybotes:0.1483909526):0.04416718222,shrevei:0.1925581348):0.1440471072):0.005421884492):0.1066560095,(longitibialis:0.2521253346,strahmi:0.2521253346):0.1965578014):0.09143002532,marcanoi:0.5401131613):0.2505275207,((((((baleatus:0.04173045424,barahonae:0.04173045424):0.05263675531,ricordii:0.09436720956):0.2036021511,eugenegrahami:0.2979693606):0.0851110199,christophei:0.3830803805):0.09095334022,cuvieri:0.4740337207):0.1076385501,(barbatus:0.1467942669,(porcus:0.09310584235,(chamaeleonides:0.07630236186,guamuhaya:0.07630236186):0.01680348049):0.05368842459):0.4348780039):0.2089684112):0.07867171672):0.07597999248,((((((((altitudinalis:0.1748899419,oporinus:0.1748899419):0.09220318062,isolepis:0.2670931225):0.2538920892,(allisoni:0.29602293,porcatus:0.29602293):0.2249622817):0.03703491197,(((argillaceus:0.1142165228,centralis:0.1142165228):0.0249762444,pumilis:0.1391927672):0.2356256274,loysiana:0.3748183946):0.1832017291):0.08522862529,guazuma:0.6432487489):0.04644117492,((placidus:0.1869579579,sheplani:0.1869579579):0.3773659809,(alayoni:0.3793818065,(angusticeps:0.2172126961,paternus:0.2172126961):0.1621691104):0.1849421323):0.125365985):0.07887044542,((alutaceus:0.120861969,inexpectatus:0.120861969):0.4042515809,(((clivicola:0.3359598029,(cupeyalensis:0.08606303065,cyanopleurus:0.08606303065):0.2498967723):0.1189736423,(alfaroi:0.2802339379,macilentus:0.2802339379):0.1746995073):0.0092278683,vanidicus:0.4641613135):0.06095223642):0.2434468193):0.09435314761,(argenteolus:0.6564331946,lucius:0.6564331946):0.2064803223):0.08237887432):0.01002346021):0.04468414858,(((bartschi:0.5247253674,vermiculatus:0.5247253674):0.249459768,((((baracoae:0.05853977536,(noblei:0.02140617522,smallwoodi:0.02140617522):0.03713360014):0.02849164237,luteogularis:0.08703141773):0.017899207,equestris:0.1049306247):0.6297194497,(((monticola:0.6055537678,(bahorucoensis:0.3841100683,(dolichocephalus:0.1509270933,hendersoni:0.1509270933):0.2331829749):0.2214436996):0.03149201716,darlingtoni:0.637045785):0.03288736013,(((aliniger:0.1783542747,singularis:0.1783542747):0.1377057507,chlorocyanus:0.3160600254):0.2135626601,coelestinus:0.5296226856):0.1403104596):0.0647169293):0.0395350609):0.1207482386,occultus:0.8949333739):0.1050666261);"

def treeEquals(tree1, tree2):

    tree1Iter = vtk.vtkTreeDFSIterator()
    tree1Iter.SetTree(tree1)
    tree2Iter = vtk.vtkTreeDFSIterator()
    tree2Iter.SetTree(tree2)

    while(tree1Iter.HasNext()):
        if not tree2Iter.HasNext():
            return False

        if tree1.GetNumberOfChildren(tree1Iter.Next()) != tree2.GetNumberOfChildren(tree2Iter.Next()):
            return False

    if tree2Iter.HasNext():
        return False


    return True

class TestRCalculatorFilter(vtk.test.Testing.vtkTest):

    def testTableOutput(self):
        rcal = vtk.vtkRCalculatorFilter()
        rcal.SetRscript("output = list(test=c(1,2,3,4))\n");
        rcal.GetTable('output')
        input = vtk.vtkTable()
        rcal.SetInputData(input)

        rcal.Update()

        t1 = rcal.GetOutput().GetColumnByName('test')
        value = 1

        for i in range(0, t1.GetNumberOfTuples()):
            self.assertEqual(value, t1.GetValue(i))
            value += 1


    def testTreeOutput(self):
        tree_reader = vtk.vtkNewickTreeReader()
        tree_reader.ReadFromInputStringOn()
        tree_reader.SetInputString(tree_data)


        rcal = vtk.vtkRCalculatorFilter()

        rcal.SetInputConnection(tree_reader.GetOutputPort())
        rcal.SetRscript("library(ape)\n\
                         output = read.tree(text=\"" + tree_data + "\")\n");
        rcal.GetTree('output')
        rcal.Update()

        expected_tree = tree_reader.GetOutput()

        self.assertTrue(treeEquals(expected_tree, rcal.GetOutput()))

    def testTableInputOutput(self):
        rcal = vtk.vtkRCalculatorFilter()
        rcal.SetRscript("output = input\n");
        rcal.PutTable('input')
        rcal.GetTable('output')

        value = 1
        array = vtk.vtkDoubleArray()
        array.SetNumberOfComponents(1)
        array.SetNumberOfTuples(4)
        array.SetName('test')
        for i in range(0, 4):
            array.SetValue(i, value)
            value += 1

        input = vtk.vtkTable()
        input.AddColumn(array)
        rcal.SetInputData(input)

        rcal.Update()

        t1 = rcal.GetOutput().GetColumnByName('test')
        value = 1

        for i in range(0, t1.GetNumberOfTuples()):
            self.assertEqual(value, t1.GetValue(i))
            value += 1

    def testMultiTableOutputs(self):
        outputs = vtk.vtkStringArray()
        outputs.SetNumberOfComponents(1)
        outputs.SetNumberOfTuples(3)
        outputs.SetValue(0, "output1")
        outputs.SetValue(1, "output2")
        outputs.SetValue(2, "output3")
        rcal = vtk.vtkRCalculatorFilter()
        rcal.SetRscript("output1 = list(test=c(1,2,3,4))\n\
                         output2 = list(test=c(5,6,7,8))\n\
                         output3 = list(test=c(9,10,11,12))\n");

        rcal.GetTables(outputs)

        input = vtk.vtkTable()
        rcal.SetInputData(input)

        rcal.Update()


        t1 = rcal.GetOutput().GetPieceAsDataObject(0).GetColumnByName('test')
        value = 1

        for i in range(0, t1.GetNumberOfTuples()):
            self.assertEqual(value, t1.GetValue(i))
            value += 1

        t2  = rcal.GetOutput().GetPieceAsDataObject(1).GetColumnByName('test')
        for i in range(0, t2.GetNumberOfTuples()):
            self.assertEqual(value, t2.GetValue(i))
            value += 1

        t3  = rcal.GetOutput().GetPieceAsDataObject(2).GetColumnByName('test')
        for i in range(0, t3.GetNumberOfTuples()):
            self.assertEqual(value, t3.GetValue(i))
            value += 1

    def testMultiTreeOutputs(self):
        outputs = vtk.vtkStringArray()
        outputs.SetNumberOfComponents(1)
        outputs.SetNumberOfTuples(2)
        outputs.SetValue(0, "tree1")
        outputs.SetValue(1, "tree2")
        rcal = vtk.vtkRCalculatorFilter()


        rcal.SetRscript("library(ape)\n\
                         tree1 = read.tree(text=\"" + tree_data + "\")\n\
                         tree2 = read.tree(text=\"" + tree_data + "\")\n")

        rcal.GetTrees(outputs)

        input = vtk.vtkTable()
        rcal.SetInputData(input)

        rcal.Update()

        compo = rcal.GetOutput()
        tree1 = compo.GetPieceAsDataObject(0)
        self.assertTrue(tree1.IsA('vtkTree'))
        tree2 = compo.GetPieceAsDataObject(1)
        self.assertTrue(tree2.IsA('vtkTree'))




    def testMultiTableInputs(self):
        inputs = vtk.vtkStringArray()
        inputs.SetNumberOfComponents(1)
        inputs.SetNumberOfTuples(3)
        inputs.SetValue(0, "input1")
        inputs.SetValue(1, "input2")
        inputs.SetValue(2, "input3")

        outputs = vtk.vtkStringArray()
        outputs.SetNumberOfComponents(1)
        outputs.SetNumberOfTuples(3)
        outputs.SetValue(0, "output1")
        outputs.SetValue(1, "output2")
        outputs.SetValue(2, "output3")

        rcal = vtk.vtkRCalculatorFilter()
        # Copy input to output for validation
        rcal.SetRscript("output1 = input1\n\
                         print(input1[[1]])\n\
                         output2 = input2\n\
                         output3 = input3\n");

        rcal.GetTables(outputs)
        rcal.PutTables(inputs)

        value = 1
        array1 = vtk.vtkDoubleArray()
        array1.SetNumberOfComponents(1)
        array1.SetNumberOfTuples(4)
        array1.SetName('test')
        for i in range(0, 4):
            array1.SetValue(i, value)
            value += 1

        array2 = vtk.vtkDoubleArray()
        array2.SetNumberOfComponents(1)
        array2.SetNumberOfTuples(4)
        array2.SetName('test')
        for i in range(0, 4):
            array2.SetValue(i, value)
            value += 1

        array3 = vtk.vtkDoubleArray()
        array3.SetNumberOfComponents(1)
        array3.SetNumberOfTuples(4)
        array3.SetName('test')
        for i in range(0, 4):
            array3.SetValue(i, value)
            value += 1


        input1 = vtk.vtkTable()
        input1.AddColumn(array1)
        input2 = vtk.vtkTable()
        input2.AddColumn(array2)
        input3 = vtk.vtkTable()
        input3.AddColumn(array3)

        compo = vtk.vtkMultiPieceDataSet()
        compo.SetNumberOfPieces(3)

        compo.SetPiece(0, input1)
        compo.SetPiece(1, input2)
        compo.SetPiece(2, input3)

        rcal.SetInputData(compo)

        rcal.Update()

        t1 = rcal.GetOutput().GetPieceAsDataObject(0).GetColumnByName('test')
        value = 1

        for i in range(0, t1.GetNumberOfTuples()):
            self.assertEqual(value, t1.GetValue(i))
            value += 1

        t2  = rcal.GetOutput().GetPieceAsDataObject(1).GetColumnByName('test')
        for i in range(0, t2.GetNumberOfTuples()):
            self.assertEqual(value, t2.GetValue(i))
            value += 1

        t3  = rcal.GetOutput().GetPieceAsDataObject(2).GetColumnByName('test')
        for i in range(0, t3.GetNumberOfTuples()):
            self.assertEqual(value, t3.GetValue(i))
            value += 1


if __name__ == "__main__":
    vtk.test.Testing.main([(TestRCalculatorFilter, 'testTreeOutput')])
