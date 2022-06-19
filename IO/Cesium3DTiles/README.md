# vtk3DTilesWriter - Convert a multiblock dataset to the 3D Tiles format.

Currently, to create a valid 3D Tiles dataset we may need additional
conversions: from GLTF to GLB and from GLB to B3DM. We can use
JavaScript tools to do these conversions.

# Install conversion scripts
- Using node and npm installed on Ubuntu 20.04:
- `cd ~/external/3d-tiles-tools/;npm install 3d-tiles-tools`. Help at: <https://github.com/AnalyticalGraphicsInc/3d-tiles-tools/tree/master/tools>
- `cd ~/external/gltf-pipeline;npm install gltf-pipeline`. Help at: <https://github.com/CesiumGS/gltf-pipeline>
- Clone <https://github.com/CesiumGS/3d-tiles-samples>. and then `npm install.`

# Convert data to GLB or B3DM - Optional
See Testing/Cxx/Test3DTilesWriter for conversions of Jacksonville data
stored in OBJs and or Berlin data stored in CityGML.
Note that the test saves the 3D Tiles data using GLTF files.
If needed, you can use GLB or B3DM, but you'll need to do the following conversions
manually:
`cd ~/projects/VTK/build/Testing/Temporary/jacksonville-3dtiles/`
`cd ~/projects/VTK/build/Testing/Temporary/berlin-3dtiles/`

- Convert gltf to glb
```
find . -name '*.gltf' -exec bash -c 'nodejs ~/external/gltf-pipeline/bin/gltf-pipeline.js -i ${0} -o ${0%.*}.glb' {} \;
find . -name '*.gltf' -exec rm {} \;
find . -name '*.bin' -exec rm {} \;
```
- Convert glb to b3dm
```
find . -name '*.glb' -exec bash -c 'nodejs ~/external/3d-tiles-tools/tools/bin/3d-tiles-tools.js glbToB3dm ${0} ${0%.*}.b3dm' {} \;
find . -name '*.glb' -exec rm {} \;

```
# View in Cesium
1. Use 3d-tiles-samples
  - Link the tileset created for previous set:
  `cd ~/external/3d-tiles-samples/tilesets; ln -s ~/projects/VTK/build/Testing/Temporary/jacksonville-3dtiles`
  `cd ~/external/3d-tiles-samples/tilesets; ln -s ~/projects/VTK/build/Testing/Temporary/berlin-3dtiles`
  - Start web server:
  `cd ..;npm start`
2. `google-chrome jacksonville-3dtiles.html;google-chrome berlin-3dtiles.html`

# Test the tilesets using 3d-tiles-validator
```
cd ~/external/3d-tiles-validator/validator/
node ./bin/3d-tiles-validator.js -i ~/projects/VTK/build/Testing/Temporary/jacksonville-3dtiles-points/tileset.json
node ./bin/3d-tiles-validator.js -i ~/projects/VTK/build/Testing/Temporary/jacksonville-3dtiles-colorpoints/tileset.json
```
