#include <Xdmf.h>
#include <XdmfDiff.h>

XdmfDOM * createGrid(int * connections, double * points, double * nodeValues, double * cellValues)
{
	XdmfRoot * myRoot = new XdmfRoot();
	XdmfDomain * myDomain = new XdmfDomain();
	XdmfDOM * myDOM = new XdmfDOM();

	myRoot->SetDOM(myDOM);
	myRoot->Build();
	myRoot->Insert(myDomain);

	XdmfGrid * myGrid = new XdmfGrid();
	myGrid->SetName("test");

	// Write Topology
	XdmfTopology * myTopology = myGrid->GetTopology();
	myTopology->SetTopologyType(XDMF_HEX);
	myTopology->SetNumberOfElements(2);

	XdmfArray * myConnections = myTopology->GetConnectivity();
	myConnections->SetHeavyDataSetName("output.h5:/Connections");
	myConnections->SetNumberType(XDMF_INT32_TYPE);
	myConnections->SetNumberOfElements(16);

	myConnections->SetValues(0, connections, 16);

	// Write Geometry
	XdmfGeometry * myGeometry = myGrid->GetGeometry();
	myGeometry->SetGeometryType(XDMF_GEOMETRY_XYZ);
	myGeometry->SetNumberOfPoints(12);

	XdmfArray * myPoints = myGeometry->GetPoints();
	myPoints->SetHeavyDataSetName("output.h5:/XYZ");
	myPoints->SetNumberType(XDMF_FLOAT64_TYPE);
	myPoints->SetNumberOfElements(36);

	myPoints->SetValues(0, points, 36);

	myDomain->Insert(myGrid);

	XdmfAttribute * currAttribute = new XdmfAttribute();
	currAttribute->SetName("NodeValues");
	currAttribute->SetAttributeCenter(XDMF_ATTRIBUTE_CENTER_NODE);
	currAttribute->SetAttributeType(XDMF_ATTRIBUTE_TYPE_SCALAR);
	currAttribute->SetDeleteOnGridDelete(true);

	XdmfArray * array = currAttribute->GetValues();
	array->SetHeavyDataSetName("output.h5:/NodeValues");
	// For now only support the data we know we are writing
	array->SetNumberType(XDMF_FLOAT64_TYPE);
	array->SetNumberOfElements(12);

	array->SetValues(0,nodeValues,12);

	myGrid->Insert(currAttribute);

	XdmfAttribute * cellAttribute = new XdmfAttribute();
	cellAttribute->SetName("CellValues");
	cellAttribute->SetAttributeCenter(XDMF_ATTRIBUTE_CENTER_CELL);
	cellAttribute->SetAttributeType(XDMF_ATTRIBUTE_TYPE_SCALAR);
	cellAttribute->SetDeleteOnGridDelete(true);

	XdmfArray * cellArray = cellAttribute->GetValues();
	cellArray->SetHeavyDataSetName("output.h5:/CellValues");
	// For now only support the data we know we are writing
	cellArray->SetNumberType(XDMF_FLOAT64_TYPE);
	cellArray->SetNumberOfElements(2);

	cellArray->SetValues(0,cellValues,2);

	myGrid->Insert(cellAttribute);
	myGrid->Build();

	delete myGrid;
	delete myDomain;
	delete myRoot;

	return myDOM;
}

int main()
{
	int connections[16] = {0,1,7,6,3,4,10,9,1,2,8,7,4,5,11,10};
	double points[36] = {0,0,1,1,0,1,3,0,2,0,1,1,1,1,1,3,2,2,0,0,-1,1,0,-1,3,0,-2,0,1,-1,1,1,-1,3,2,-2};
	double nodeValues[12] = {100, 200, 300, 300, 400, 500, 300, 400, 500, 500, 600, 700};
	double cellValues[2] = {100, 200};

	XdmfDOM * myDOM1 = createGrid(connections, points, nodeValues, cellValues);
	nodeValues[0] = 110;
	XdmfDOM * myDOM2 = createGrid(connections, points, nodeValues, cellValues);

	XdmfDiff * myDiff = new XdmfDiff(myDOM1, myDOM2);

	if (myDiff->AreEquivalent())
	{
		return -1;
	}

	myDiff->SetAbsoluteError(5);
	if (myDiff->AreEquivalent())
	{
		return -1;
	}


	myDiff->SetAbsoluteError(11);
	if (!myDiff->AreEquivalent())
	{
		return -1;
	}

	myDiff->SetRelativeError(.05);
	if (myDiff->AreEquivalent())
	{
		return -1;
	}

	myDiff->SetRelativeError(.1);
	if (!myDiff->AreEquivalent())
	{
		return -1;
	}

	myDiff->SetRelativeError(0);
	myDiff->SetIgnoreAllAttributes(true);
	if (!myDiff->AreEquivalent())
	{
		return -1;
	}

	delete myDiff;
	return 0;
}


