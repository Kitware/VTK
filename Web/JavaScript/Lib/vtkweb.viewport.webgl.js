/**
 * vtkWeb JavaScript Library.
 *
 * This module extend the vtkWeb viewport to add support for WebGL rendering.
 *
 * @class vtkWeb.viewports.webgl
 *
 *     Viewport Factory description:
 *       - Key: webgl
 *       - Stats:
 *         - webgl-fps
 *         - webgl-nb-objects
 *         - webgl-fetch-scene
 *         - webgl-fetch-object
 */
(function (GLOBAL, $) {
    var module = {},
    RENDERER_CSS = {
        "position": "absolute",
        "top"     : "0px",
        "left"    : "0px",
        "right"   : "0px",
        "bottom"  : "0px",
        "z-index" : "0"
    },
    RENDERER_CSS_2D = {
        "z-index" : "1"
    },
    RENDERER_CSS_3D = {
        "z-index" : "0"
    },
    DEFAULT_OPTIONS = {
        keepServerInSynch: false
    },
    FACTORY_KEY = 'webgl',
    FACTORY = {
        'builder': createGeometryDeliveryRenderer,
        'options': DEFAULT_OPTIONS,
        'stats': {
            'webgl-fps': {
                label: 'Framerate:',
                type: 'time',
                convert: function(value) {
                    if(value === 0) {
                        return 0;
                    }
                    return Math.floor(1000 / value);
                }
            },
            'webgl-nb-objects': {
                label: 'Number&nbsp;of&nbsp;3D&nbsp;objects:',
                type: 'value',
                convert: NoOp
            },
            'webgl-fetch-scene': {
                label: 'Fetch&nbsp;scene&nbsp;(ms):',
                type: 'time',
                convert: NoOp
            },
            'webgl-fetch-object': {
                label: 'Fetch&nbsp;object&nbsp;(ms):',
                type: 'time',
                convert: NoOp
            }
        }
    },
    DEFAULT_SHADERS = {},
    mvMatrixStack = [];


    // ----------------------------------------------------------------------

    function NoOp(a) {
        return a;
    }

    // ----------------------------------------------------------------------
    // Initialize the Shaders
    // ----------------------------------------------------------------------

    DEFAULT_SHADERS["shader-fs"] = {
        type: "x-shader/x-fragment",
        code: "\
            #ifdef GL_ES\n\
            precision highp float;\n\
            #endif\n\
            uniform bool uIsLine;\
            varying vec4 vColor;\
            varying vec4 vTransformedNormal;\
            varying vec4 vPosition;\
            void main(void) {\
                float directionalLightWeighting1 = max(dot(normalize(vTransformedNormal.xyz), vec3(0.0, 0.0, 1.0)), 0.0); \
                float directionalLightWeighting2 = max(dot(normalize(vTransformedNormal.xyz), vec3(0.0, 0.0, -1.0)), 0.0);\
                vec3 lightWeighting = max(vec3(1.0, 1.0, 1.0) * directionalLightWeighting1, vec3(1.0, 1.0, 1.0) * directionalLightWeighting2);\
                if (uIsLine == false){\
                  gl_FragColor = vec4(vColor.rgb * lightWeighting, vColor.a);\
                } else {\
                  gl_FragColor = vColor*vec4(1.0, 1.0, 1.0, 1.0);\
                }\
            }"
    };

    // ----------------------------------------------------------------------

    DEFAULT_SHADERS["shader-vs"] = {
        type: "x-shader/x-vertex",
        code: "\
            attribute vec3 aVertexPosition;\
            attribute vec4 aVertexColor;\
            attribute vec3 aVertexNormal;\
            uniform mat4 uMVMatrix;\
            uniform mat4 uPMatrix;\
            uniform mat4 uNMatrix;\
            varying vec4 vColor;\
            varying vec4 vPosition;\
            varying vec4 vTransformedNormal;\
            void main(void) {\
                vPosition = uMVMatrix * vec4(aVertexPosition, 1.0);\
                gl_Position = uPMatrix * vPosition;\
                vTransformedNormal = uNMatrix * vec4(aVertexNormal, 1.0);\
                vColor = aVertexColor;\
            }"
    };

    // ----------------------------------------------------------------------

    DEFAULT_SHADERS["shader-fs-Point"] = {
        type: "x-shader/x-fragment",
        code: "\
            #ifdef GL_ES\n\
            precision highp float;\n\
            #endif\n\
            varying vec4 vColor;\
            void main(void) {\
                gl_FragColor = vColor;\
            }"
    };

    // ----------------------------------------------------------------------

    DEFAULT_SHADERS["shader-vs-Point"] = {
        type: "x-shader/x-vertex",
        code: "\
            attribute vec3 aVertexPosition;\
            attribute vec4 aVertexColor;\
            uniform mat4 uMVMatrix;\
            uniform mat4 uPMatrix;\
            uniform mat4 uNMatrix;\
            uniform float uPointSize;\
            varying vec4 vColor;\
            void main(void) {\
                vec4 pos = uMVMatrix * vec4(aVertexPosition, 1.0);\
                gl_Position = uPMatrix * pos;\
                vColor = aVertexColor*vec4(1.0, 1.0, 1.0, 1.0);\
                gl_PointSize = uPointSize;\
            }"
    };

    // ----------------------------------------------------------------------

    function getShader(gl, id) {
        try {
            var jsonShader = DEFAULT_SHADERS[id], shader = null;

            // Allocate shader
            if(jsonShader.type === "x-shader/x-fragment") {
                shader = gl.createShader(gl.FRAGMENT_SHADER);
            } else if(jsonShader.type === "x-shader/x-vertex") {
                shader = gl.createShader(gl.VERTEX_SHADER);
            } else {
                return null;
            }

            // Set code and compile
            gl.shaderSource(shader, jsonShader.code);
            gl.compileShader(shader);

            // Check compilation
            if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
                alert(gl.getShaderInfoLog(shader));
                return null;
            }

            return shader;
        } catch(error) {
            console.log(error);
        }
    }

    // ----------------------------------------------------------------------

    function initializeShader(gl, shaderProgram, pointShaderProgram) {
        try {

            var vertexShader = getShader(gl, 'shader-vs'),
            fragmentShader   = getShader(gl, 'shader-fs'),
            pointFragShader  = getShader(gl, 'shader-fs-Point'),
            pointVertShader  = getShader(gl, 'shader-vs-Point');

            // Initialize program
            gl.attachShader(shaderProgram, vertexShader);
            gl.attachShader(shaderProgram, fragmentShader);
            gl.linkProgram(shaderProgram);
            if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
                alert("Could not initialise shaders");
            }

            gl.attachShader(pointShaderProgram, pointVertShader);
            gl.attachShader(pointShaderProgram, pointFragShader);
            gl.linkProgram(pointShaderProgram);
            if (!gl.getProgramParameter(pointShaderProgram, gl.LINK_STATUS)) {
                alert("Could not initialise the point shaders");
            }

            gl.useProgram(pointShaderProgram);
            pointShaderProgram.vertexPositionAttribute = gl.getAttribLocation(pointShaderProgram, "aVertexPosition");
            gl.enableVertexAttribArray(pointShaderProgram.vertexPositionAttribute);
            pointShaderProgram.vertexColorAttribute = gl.getAttribLocation(pointShaderProgram, "aVertexColor");
            gl.enableVertexAttribArray(pointShaderProgram.vertexColorAttribute);
            pointShaderProgram.pMatrixUniform = gl.getUniformLocation(pointShaderProgram, "uPMatrix");
            pointShaderProgram.mvMatrixUniform = gl.getUniformLocation(pointShaderProgram, "uMVMatrix");
            pointShaderProgram.nMatrixUniform = gl.getUniformLocation(pointShaderProgram, "uNMatrix");
            pointShaderProgram.uPointSize = gl.getUniformLocation(pointShaderProgram, "uPointSize");

            gl.useProgram(shaderProgram);
            shaderProgram.vertexPositionAttribute = gl.getAttribLocation(shaderProgram, "aVertexPosition");
            gl.enableVertexAttribArray(shaderProgram.vertexPositionAttribute);
            shaderProgram.vertexColorAttribute = gl.getAttribLocation(shaderProgram, "aVertexColor");
            gl.enableVertexAttribArray(shaderProgram.vertexColorAttribute);
            shaderProgram.vertexNormalAttribute = gl.getAttribLocation(shaderProgram, "aVertexNormal");
            gl.enableVertexAttribArray(shaderProgram.vertexNormalAttribute);
            shaderProgram.pMatrixUniform = gl.getUniformLocation(shaderProgram, "uPMatrix");
            shaderProgram.mvMatrixUniform = gl.getUniformLocation(shaderProgram, "uMVMatrix");
            shaderProgram.nMatrixUniform = gl.getUniformLocation(shaderProgram, "uNMatrix");
            shaderProgram.uIsLine = gl.getUniformLocation(shaderProgram, "uIsLine");
        } catch(error) {
            console.log(error);
        }
    }

    // ----------------------------------------------------------------------
    // GL rendering metods
    // ----------------------------------------------------------------------

    function setMatrixUniforms(gl, shaderProgram, projMatrix, mvMatrix) {
        var mvMatrixInv = mat4.create(), normal = mat4.create();

        mat4.invert(mvMatrixInv, mvMatrix);
        mat4.transpose(normal, mvMatrixInv);

        gl.uniformMatrix4fv(shaderProgram.pMatrixUniform, false, projMatrix);
        gl.uniformMatrix4fv(shaderProgram.mvMatrixUniform, false, mvMatrix);
        if(shaderProgram.nMatrixUniform != null) gl.uniformMatrix4fv(shaderProgram.nMatrixUniform, false, normal);
    }

    // ----------------------------------------------------------------------

    function renderMesh(renderingContext, camera) {
        try {
            var obj = this,
            mvMatrix = mat4.clone(camera.getCameraMatrices()[1]),
            projMatrix = mat4.clone(camera.getCameraMatrices()[0]),
            objMatrix = mat4.transpose(mat4.create(), obj.matrix),
            gl = renderingContext.gl,
            shaderProgram = renderingContext.shaderProgram;

            gl.useProgram(shaderProgram);
            gl.uniform1i(shaderProgram.uIsLine, false);

            mvMatrix = mat4.multiply(mvMatrix, mvMatrix, objMatrix);

            gl.bindBuffer(gl.ARRAY_BUFFER, obj.vbuff);
            gl.vertexAttribPointer(shaderProgram.vertexPositionAttribute, obj.vbuff.itemSize, gl.FLOAT, false, 0, 0);
            gl.bindBuffer(gl.ARRAY_BUFFER, obj.nbuff);
            gl.vertexAttribPointer(shaderProgram.vertexNormalAttribute, obj.nbuff.itemSize, gl.FLOAT, false, 0, 0);
            gl.bindBuffer(gl.ARRAY_BUFFER, obj.cbuff);
            gl.vertexAttribPointer(shaderProgram.vertexColorAttribute, obj.cbuff.itemSize, gl.FLOAT, false, 0, 0);
            gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, obj.ibuff);

            setMatrixUniforms(gl, shaderProgram, projMatrix, mvMatrix);

            gl.drawElements(gl.TRIANGLES, obj.numberOfIndex, gl.UNSIGNED_SHORT, 0);
        } catch(error) {
            console.log(error);
        }
    }

    // ----------------------------------------------------------------------

    function renderLine(renderingContext, camera) {
        try {
            var obj = this,
            mvMatrix = mat4.clone(camera.getCameraMatrices()[1]),
            projMatrix = mat4.clone(camera.getCameraMatrices()[0]),
            objMatrix = mat4.transpose(mat4.create(), obj.matrix),
            gl = renderingContext.gl,
            shaderProgram = renderingContext.shaderProgram;

            gl.useProgram(shaderProgram);

            gl.enable(gl.POLYGON_OFFSET_FILL);  //Avoid zfighting
            gl.polygonOffset(-1.0, -1.0);

            gl.uniform1i(shaderProgram.uIsLine, true);

            mvMatrix = mat4.multiply(mvMatrix, mvMatrix, objMatrix);

            gl.bindBuffer(gl.ARRAY_BUFFER, obj.lbuff);
            gl.vertexAttribPointer(shaderProgram.vertexPositionAttribute, obj.lbuff.itemSize, gl.FLOAT, false, 0, 0);
            gl.bindBuffer(gl.ARRAY_BUFFER, obj.nbuff);
            gl.vertexAttribPointer(shaderProgram.vertexNormalAttribute, obj.nbuff.itemSize, gl.FLOAT, false, 0, 0);
            gl.bindBuffer(gl.ARRAY_BUFFER, obj.cbuff);
            gl.vertexAttribPointer(shaderProgram.vertexColorAttribute, obj.cbuff.itemSize, gl.FLOAT, false, 0, 0);
            gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, obj.ibuff);

            setMatrixUniforms(gl, shaderProgram, projMatrix, mvMatrix);

            gl.drawElements(gl.LINES, obj.numberOfIndex, gl.UNSIGNED_SHORT, 0);

            gl.disable(gl.POLYGON_OFFSET_FILL);
        } catch(error) {
            console.log(error);
        }
    }

    // ----------------------------------------------------------------------

    function renderPoints(renderingContext, camera) {
        try {
            var obj = this,
            mvMatrix = mat4.clone(camera.getCameraMatrices()[1]),
            projMatrix = mat4.clone(camera.getCameraMatrices()[0]),
            objMatrix = mat4.transpose(mat4.create(), obj.matrix),
            gl = renderingContext.gl,
            pointShaderProgram = renderingContext.pointShaderProgram;

            gl.useProgram(pointShaderProgram);

            gl.enable(gl.POLYGON_OFFSET_FILL);  //Avoid zfighting
            gl.polygonOffset(-1.0, -1.0);

            gl.uniform1f(pointShaderProgram.uPointSize, 2.0);

            mvMatrix = mat4.multiply(mvMatrix, mvMatrix, objMatrix);

            gl.bindBuffer(gl.ARRAY_BUFFER, obj.pbuff);
            gl.vertexAttribPointer(pointShaderProgram.vertexPositionAttribute, obj.pbuff.itemSize, gl.FLOAT, false, 0, 0);
            gl.bindBuffer(gl.ARRAY_BUFFER, obj.cbuff);
            gl.vertexAttribPointer(pointShaderProgram.vertexColorAttribute, obj.cbuff.itemSize, gl.FLOAT, false, 0, 0);

            setMatrixUniforms(gl, pointShaderProgram, projMatrix, mvMatrix);

            gl.drawArrays(gl.POINTS, 0, obj.numberOfPoints);

            gl.disable(gl.POLYGON_OFFSET_FILL);
        } catch(error) {
            console.log(error);
        }
    }

    // ----------------------------------------------------------------------

    function renderColorMap(renderingContext, camera) {
        try {
            var obj = this, ctx = renderingContext.ctx2d, range, txt, color, c, v,
            size, pos, dx, dy, realSize, textSizeX, textSizeY, grad,
            width = renderingContext.container.width(),
            height = renderingContext.container.height();

            range = [obj.colors[0][0], obj.colors[obj.colors.length-1][0]];
            size = [obj.size[0]*width, obj.size[1]*height];
            pos = [obj.position[0]*width, (1-obj.position[1])*height];
            pos[1] = pos[1]-size[1];
            dx = size[0]/size[1];
            dy = size[1]/size[0];
            realSize = size;

            textSizeX = Math.round(height/35);
            textSizeY = Math.round(height/23);
            if (obj.orientation == 1){
                size[0] = size[0]*dy/25;
                size[1] = size[1]-(2*textSizeY);
            } else {
                size[0] = size[0];
                size[1] = size[1]*dx/25;
            }

            // Draw Gradient
            if(obj.orientation == 1){
                pos[1] += 2*textSizeY;
                grad = ctx.createLinearGradient(pos[0], pos[1], pos[0], pos[1]+size[1]);
            } else {
                pos[1] += 2*textSizeY;
                grad = ctx.createLinearGradient(pos[0], pos[1], pos[0]+size[0], pos[1]);
            }
            if ((range[1]-range[0]) == 0){
                color = 'rgba(' + obj.colors[0][1] + ',' + obj.colors[0][2] + ',' + obj.colors[0][3] + ',1)';
                grad.addColorStop(0, color);
                grad.addColorStop(1, color);
            } else {
                for(c=0; c<obj.colors.length; c++){
                    v = ((obj.colors[c][0]-range[0])/(range[1]-range[0]));
                    if (obj.orientation == 1) v=1-v;
                    color = 'rgba(' + obj.colors[c][1] + ',' + obj.colors[c][2] + ',' + obj.colors[c][3] + ',1)';
                    grad.addColorStop(v, color);
                }
            }
            ctx.fillStyle = grad;
            ctx.fillRect(pos[0], pos[1], size[0], size[1]);
            // Draw Range Labels
            range[0] = Math.round(range[0]*1000)/1000;
            range[1] = Math.round(range[1]*1000)/1000;
            ctx.fillStyle = 'white';
            ctx.font = textSizeY + 'px sans-serif';
            ctx.txtBaseline = 'ideographic';
            if (obj.orientation == 1){
                ctx.fillText(range[1], pos[0], pos[1]-5);
                ctx.fillText(range[0], pos[0], pos[1]+size[1]+textSizeY);
            } else {
                ctx.fillText(range[0], pos[0], pos[1]+size[1]+textSizeY);
                txt = range[1].toString();
                ctx.fillText(range[1], pos[0]+size[0]-((txt.length-1)*textSizeX), pos[1]+size[1]+textSizeY);
            }
            // Draw Title
            ctx.fillStyle = 'white';
            ctx.font = textSizeY + 'px sans-serif';
            ctx.txtBaseline = 'ideographic';
            if (obj.orientation == 1) ctx.fillText(obj.title, pos[0]+(obj.size[0]*width)/2-(obj.title.length*textSizeX/2), pos[1]-textSizeY-5);
            else ctx.fillText(obj.title, pos[0]+size[0]/2-(obj.title.length*textSizeX/2), pos[1]-textSizeY-5);
        } catch(error) {
            console.log(error);
        }
    }

    // ----------------------------------------------------------------------

    function renderBackground(renderingContext, camera) {
        try {
            var background = this, gl = renderingContext.gl, shaderProgram = renderingContext.shaderProgram;

            gl.useProgram(renderingContext.shaderProgram);
            gl.uniform1i(renderingContext.shaderProgram.uIsLine, false);

            var projMatrix = mat4.create();
            var mvMatrix = mat4.create();
            var normalMatrix = mat4.create();

            // @note Not sure if this is required
            mat4.translate(mvMatrix, mvMatrix, [0.0, 0.0, -1.0]);

            gl.bindBuffer(gl.ARRAY_BUFFER, background.vbuff);
            gl.vertexAttribPointer(shaderProgram.vertexPositionAttribute, background.vbuff.itemSize, gl.FLOAT, false, 0, 0);
            gl.bindBuffer(gl.ARRAY_BUFFER, background.nbuff);
            gl.vertexAttribPointer(shaderProgram.vertexNormalAttribute, background.nbuff.itemSize, gl.FLOAT, false, 0, 0);
            gl.bindBuffer(gl.ARRAY_BUFFER, background.cbuff);
            gl.vertexAttribPointer(shaderProgram.vertexColorAttribute, background.cbuff.itemSize, gl.FLOAT, false, 0, 0);
            gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, background.ibuff);

            renderingContext.gl.uniformMatrix4fv(shaderProgram.pMatrixUniform, false, projMatrix);
            renderingContext.gl.uniformMatrix4fv(shaderProgram.mvMatrixUniform, false, mvMatrix);
            if(shaderProgram.nMatrixUniform != null) renderingContext.gl.uniformMatrix4fv(shaderProgram.nMatrixUniform, false, normalMatrix);

            gl.drawElements(gl.TRIANGLES, background.numberOfIndex, gl.UNSIGNED_SHORT, 0);
        } catch(error) {
            console.log(error);
        }
    }

    // ----------------------------------------------------------------------
    // 3D object handler
    // ----------------------------------------------------------------------

    function create3DObjectHandler() {
        var objectIndex = {}, displayList = {}, sceneJSON;

        // ------------------------------------------------------------------

        function getKey(object) {
            return object.id + '_' + object.md5;
        }

        // ------------------------------------------------------------------

        function getLayerDisplayList(layer) {
            var key = String(layer);
            if(!displayList.hasOwnProperty(key)) {
                displayList[key] = {
                    transparent: [],
                    solid: []
                };
            }
            return displayList[key];
        }

        // ------------------------------------------------------------------
        function render(displayList, renderingContext, camera) {
            var i, k, key, array;
            for(i in displayList) {
                key = displayList[i];
                if(objectIndex.hasOwnProperty(key)) {
                    array = objectIndex[key];
                    for(k in array) {
                        array[k].render(renderingContext, camera);
                    }
                }
            }
            return displayList.length;
        }

        // ------------------------------------------------------------------

        return {
            registerObject: function(object) {
                var key = getKey(object), idx;
                if(!objectIndex.hasOwnProperty(key)) {
                    objectIndex[key] = [];
                } else {
                    // Make sure is not already in
                    for(idx in objectIndex[key]) {
                        if(objectIndex[key][idx].part === object.part) {
                            return;
                        }
                    }
                }

                // Add it
                objectIndex[key].push(object);
            },

            // --------------------------------------------------------------

            updateDisplayList: function(scene) {
                // Reset displayList
                displayList = {}, sceneJSON = scene;

                // Create display lists
                for(var idx in sceneJSON.Objects) {
                    var currentObject = sceneJSON.Objects[idx],
                    displayListKey = currentObject.hasTransparency ? 'transparent' : 'solid',
                    key = getKey(currentObject);

                    getLayerDisplayList(currentObject.layer)[displayListKey].push(key);
                }
            },

            // --------------------------------------------------------------

            renderTransparent: function(layer, renderingContext, camera) {
                var displayList = getLayerDisplayList(layer).transparent;
                return render(displayList, renderingContext, camera);
            },

            // --------------------------------------------------------------

            renderSolid: function(layer, renderingContext, camera) {
                var displayList = getLayerDisplayList(layer).solid;
                return render(displayList, renderingContext, camera);
            },

            // --------------------------------------------------------------

            fetchMissingObjects: function(fetchMethod) {
                var fetch = fetchMethod, idx, part;
                for(idx in sceneJSON.Objects) {
                    var currentObject = sceneJSON.Objects[idx],
                    key = getKey(currentObject);
                    if(!objectIndex.hasOwnProperty(key)) {
                        // Request all the pieces
                        for(part = 1; part <= currentObject.parts; part++) {
                            fetch(currentObject, part);
                        }
                    }
                }
            },

            // --------------------------------------------------------------

            garbageCollect: function() {
                var refCount = {}, key, layer, array, idx;
                for(key in objectIndex) {
                    refCount[key] = 0;
                }

                // Start registering display list
                for(layer in displayList) {
                    array = displayList[layer].solid.concat(displayList[layer].transparent);
                    for(idx in array) {
                        if(refCount.hasOwnProperty(array[idx])) {
                            refCount[array[idx]]++;
                        }
                    }
                }

                // Remove entry with no reference
                for(key in refCount) {
                    if(refCount[key] === 0) {
                        delete objectIndex[key];
                    }
                }
            }

        }
    }

    // ----------------------------------------------------------------------
    // GL object creation
    // ----------------------------------------------------------------------

    function get4ByteNumber(binaryArray, cursor) {
        return (binaryArray[cursor++]) + (binaryArray[cursor++] << 8) + (binaryArray[cursor++] << 16) + (binaryArray[cursor++] << 24);
    }

    // ----------------------------------------------------------------------

    function buildBackground(gl, color1, color2) {
        try {
            if (typeof(gl) == "undefined") return;

            var background = {
                vertices: new Float32Array([-1.0, -1.0, 0.0, 1.0, -1.0, 0.0, 1.0, 1.0, 0.0, -1.0, 1.0, 0.0]),
                colors: new Float32Array([
                    color1[0], color1[1], color1[2], 1.0,
                    color1[0], color1[1], color1[2], 1.0,
                    color2[0], color2[1], color2[2], 1.0,
                    color2[0], color2[1], color2[2], 1.0]),
                index: new Uint16Array([0, 1, 2, 0, 2, 3]),
                normals: new Float32Array([0.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 0.0, -1.0]),
                numberOfIndex: 6
            };

            //Create Buffers
            background.vbuff = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, background.vbuff);
            gl.bufferData(gl.ARRAY_BUFFER, background.vertices, gl.STATIC_DRAW);
            background.vbuff.itemSize = 3;
            background.nbuff = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, background.nbuff);
            gl.bufferData(gl.ARRAY_BUFFER, background.normals, gl.STATIC_DRAW);
            background.nbuff.itemSize = 3;
            background.cbuff = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, background.cbuff);
            gl.bufferData(gl.ARRAY_BUFFER, background.colors, gl.STATIC_DRAW);
            background.cbuff.itemSize = 4;
            background.ibuff = gl.createBuffer();
            gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, background.ibuff);
            gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, background.index, gl.STREAM_DRAW);

            // bind render method
            background.render = renderBackground;

            return background;
        } catch(error) {
            console.log(error);
        }
    }

    // ----------------------------------------------------------------------

    function processWireframe(gl, obj, binaryArray, cursor) {
        try {
            var tmpArray, size, i;

            // Extract points
            obj.numberOfPoints = get4ByteNumber(binaryArray, cursor);
            cursor += 4;

            // Getting Points
            size = obj.numberOfPoints * 4 * 3;
            tmpArray = new Int8Array(size);
            for(i=0; i < size; i++) {
                tmpArray[i] = binaryArray[cursor++];
            }
            obj.points = new Float32Array(tmpArray.buffer);

            // Generating Normals
            size = obj.numberOfPoints * 3;
            tmpArray = new Array(size);
            for(i=0; i < size; i++) {
                tmpArray[i] = 0.0;
            }
            obj.normals = new Float32Array(tmpArray);

            // Getting Colors
            size = obj.numberOfPoints * 4;
            tmpArray = new Array(size);
            for(i=0; i < size; i++) {
                tmpArray[i] = binaryArray[cursor++]/255.0;;
            }
            obj.colors = new Float32Array(tmpArray);

            // Extract the number of index
            obj.numberOfIndex = get4ByteNumber(binaryArray, cursor);
            cursor += 4;

            // Getting Index
            size = obj.numberOfIndex * 2;
            tmpArray = new Int8Array(size);
            for(i=0; i < size; i++) {
                tmpArray[i] = binaryArray[cursor++];
            }
            obj.index = new Uint16Array(tmpArray.buffer);

            // Getting Matrix
            size = 16 * 4;
            tmpArray = new Int8Array(size);
            for(i=0; i < size; i++) {
                tmpArray[i] = binaryArray[cursor++];
            }
            obj.matrix = new Float32Array(tmpArray.buffer);

            // Creating Buffers
            obj.lbuff = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, obj.lbuff);
            gl.bufferData(gl.ARRAY_BUFFER, obj.points, gl.STATIC_DRAW);
            obj.lbuff.itemSize = 3;

            obj.nbuff = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, obj.nbuff);
            gl.bufferData(gl.ARRAY_BUFFER, obj.normals, gl.STATIC_DRAW);
            obj.nbuff.itemSize = 3;

            obj.cbuff = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, obj.cbuff);
            gl.bufferData(gl.ARRAY_BUFFER, obj.colors, gl.STATIC_DRAW);
            obj.cbuff.itemSize = 4;

            obj.ibuff = gl.createBuffer();
            gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, obj.ibuff);
            gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, obj.index, gl.STREAM_DRAW);

            // Bind render method
            obj.render = renderLine;
        } catch(error) {
            console.log(error);
        }
    }

    // ----------------------------------------------------------------------

    function processSurfaceMesh(gl, obj, binaryArray, cursor) {
        try {
            var tmpArray, size, i;

            // Extract number of vertices
            obj.numberOfVertices = get4ByteNumber(binaryArray, cursor);
            cursor += 4;

            // Getting Vertices
            size = obj.numberOfVertices * 4 * 3;
            tmpArray = new Int8Array(size);
            for(i=0; i < size; i++) {
                tmpArray[i] = binaryArray[cursor++];
            }
            obj.vertices = new Float32Array(tmpArray.buffer);

            // Getting Normals
            size = obj.numberOfVertices * 4 * 3;
            tmpArray = new Int8Array(size);
            for(i=0; i < size; i++) {
                tmpArray[i] = binaryArray[cursor++];
            }
            obj.normals = new Float32Array(tmpArray.buffer);

            // Getting Colors
            tmpArray = [];
            size = obj.numberOfVertices * 4;
            for(i=0; i < size; i++) {
                tmpArray[i] =  binaryArray[cursor++] / 255.0;
            }
            obj.colors = new Float32Array(tmpArray);

            // Get number of index
            obj.numberOfIndex = get4ByteNumber(binaryArray, cursor);
            cursor += 4;

            // Getting Index
            size = obj.numberOfIndex * 2;
            tmpArray  = new Int8Array(size);
            for(i=0; i < size; i++) {
                tmpArray[i] = binaryArray[cursor++];
            }
            obj.index = new Uint16Array(tmpArray.buffer);

            // Getting Matrix
            size = 16 * 4;
            tmpArray = new Int8Array(size);
            for(i=0; i < size; i++) {
                tmpArray[i] = binaryArray[cursor++];
            }
            obj.matrix = new Float32Array(tmpArray.buffer);

            // Getting TCoord
            obj.tcoord = null;

            // Create Buffers
            obj.vbuff = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, obj.vbuff);
            gl.bufferData(gl.ARRAY_BUFFER, obj.vertices, gl.STATIC_DRAW);
            obj.vbuff.itemSize = 3;

            obj.nbuff = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, obj.nbuff);
            gl.bufferData(gl.ARRAY_BUFFER, obj.normals, gl.STATIC_DRAW);
            obj.nbuff.itemSize = 3;

            obj.cbuff = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, obj.cbuff);
            gl.bufferData(gl.ARRAY_BUFFER, obj.colors, gl.STATIC_DRAW);
            obj.cbuff.itemSize = 4;

            obj.ibuff = gl.createBuffer();
            gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, obj.ibuff);
            gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, obj.index, gl.STREAM_DRAW);

            // Bind render method
            obj.render = renderMesh;
        } catch(error) {
            console.log(error);
        }
    }

    // ----------------------------------------------------------------------

    function processColorMap(gl, obj, nbColor, binaryArray, cursor) {
        try {
            var tmpArray, size, xrgb, i, c;

            // Set number of colors
            obj.numOfColors = nbColor;

            // Getting Position
            size = 2 * 4;
            tmpArray = new Int8Array(size);
            for(i=0; i < size; i++) {
                tmpArray[i] = binaryArray[cursor++];
            }
            obj.position = new Float32Array(tmpArray.buffer);

            // Getting Size
            size = 2 * 4;
            tmpArray = new Int8Array(2*4);
            for(i=0; i < size; i++) {
                tmpArray[i] = binaryArray[cursor++];
            }
            obj.size = new Float32Array(tmpArray.buffer);

            //Getting Colors
            obj.colors = [];
            for(c=0; c < obj.numOfColors; c++){
                tmpArray = new Int8Array(4);
                for(i=0; i < 4; i++) {
                    tmpArray[i] = binaryArray[cursor++];
                }
                xrgb = [
                new Float32Array(tmpArray.buffer)[0],
                binaryArray[cursor++],
                binaryArray[cursor++],
                binaryArray[cursor++]
                ];
                obj.colors[c] = xrgb;
            }

            obj.orientation = binaryArray[cursor++];
            obj.numOfLabels = binaryArray[cursor++];
            obj.title = "";
            while(cursor < binaryArray.length) {
                obj.title += String.fromCharCode(binaryArray[cursor++]);
            }

            // Bind render method
            obj.render = renderColorMap;
        } catch(error) {
            console.log(error);
        }
    }

    // ----------------------------------------------------------------------

    function processPointSet(gl, obj, binaryArray, cursor) {
        try {
            var tmpArray, size, i;

            // Get number of points
            obj.numberOfPoints = get4ByteNumber(binaryArray, cursor);
            cursor += 4;

            // Getting Points
            size = obj.numberOfPoints * 4 * 3;
            tmpArray = new Int8Array(size);
            for(i=0; i < size; i++) {
                tmpArray[i] = binaryArray[cursor++];
            }
            obj.points = new Float32Array(tmpArray.buffer);

            // Getting Colors
            size = obj.numberOfPoints * 4;
            tmpArray = [];
            for(i=0; i < size; i++) {
                tmpArray[i] = binaryArray[cursor++]/255.0;
            }
            obj.colors = new Float32Array(tmpArray);

            // Getting Matrix
            size = 16 * 4;
            tmpArray = new Int8Array(size);
            for(i=0; i < size; i++) {
                tmpArray[i] = binaryArray[cursor++]/255.0;
            }
            obj.matrix = new Float32Array(tmpArray.buffer);

            // Creating Buffers
            obj.pbuff = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, obj.pbuff);
            gl.bufferData(gl.ARRAY_BUFFER, obj.points, gl.STATIC_DRAW);
            obj.pbuff.itemSize = 3;

            obj.cbuff = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, obj.cbuff);
            gl.bufferData(gl.ARRAY_BUFFER, obj.colors, gl.STATIC_DRAW);
            obj.cbuff.itemSize = 4;

            // Bind render method
            obj.render = renderPoints;
        } catch(error) {
            console.log(error);
        }
    }

    // ----------------------------------------------------------------------

    function initializeObject(gl, obj) {
        try {
            var binaryArray = [], cursor = 0, size, type;

            // Convert char to byte
            for(var i in obj.data) {
                binaryArray.push(obj.data.charCodeAt(i) & 0xff);
            }

            // Extract size (4 bytes)
            size = get4ByteNumber(binaryArray, cursor);
            cursor += 4;

            // Extract object type
            type = String.fromCharCode(binaryArray[cursor++]);
            obj.type = type;

            // Extract raw data
            if (type == 'L'){
                processWireframe(gl, obj, binaryArray, cursor);
            } else if (type == 'M'){
                processSurfaceMesh(gl, obj, binaryArray, cursor);
            } else if (type == 'C'){
                processColorMap(gl, obj, size, binaryArray, cursor);
            } else if (type == 'P'){
                processPointSet(gl, obj, binaryArray, cursor);
            }
        } catch(error) {
            console.log(error);
        }
    }

    // ----------------------------------------------------------------------
    // Geometry Delivery renderer - factory method
    // ----------------------------------------------------------------------

    function createGeometryDeliveryRenderer(domElement) {
        var container = $(domElement),
        options = $.extend({}, DEFAULT_OPTIONS, container.data('config')),
        session = options.session,
        divContainer = GLOBAL.document.createElement('div'),
        canvas2D = GLOBAL.document.createElement('canvas'),
        canvas3D = GLOBAL.document.createElement('canvas'),
        ctx2d = canvas2D.getContext('2d'),
        screenImage = null,
        gl = canvas3D.getContext("experimental-webgl") || canvas3D.getContext("webgl"),
        shaderProgram = gl.createProgram(),
        pointShaderProgram = gl.createProgram(),
        renderer = $(divContainer).addClass(FACTORY_KEY).css(RENDERER_CSS).append($(canvas2D).css(RENDERER_CSS).css(RENDERER_CSS_2D)).append($(canvas3D).css(RENDERER_CSS).css(RENDERER_CSS_3D)),
        sceneJSON = null,
        objectHandler = create3DObjectHandler(),
        cameraLayerZero = null,
        otherCamera = [],
        mouseHandling = {
            button: null,
            lastX: 0,
            lastY: 0
        },
        renderingContext = {
            container: container,
            gl: gl,
            ctx2d: ctx2d,
            shaderProgram: shaderProgram,
            pointShaderProgram: pointShaderProgram
        },
        background = null;

        // Helper functions -------------------------------------------------

        function fetchScene() {
            container.trigger({
                type: 'stats',
                stat_id: 'webgl-fetch-scene',
                stat_value: 0
            });
            session.call("viewport.webgl.metadata", [Number(options.view)]).then(function(data) {
                sceneJSON = JSON.parse(data);
                container.trigger({
                    type: 'stats',
                    stat_id: 'webgl-fetch-scene',
                    stat_value: 1
                });
                updateScene();
            });
        }

        // ------------------------------------------------------------------

        function fetchObject(sceneObject, part) {
            try {
                var viewId = Number(options.view),
                newObject;

                container.trigger({
                    type: 'stats',
                    stat_id: 'webgl-fetch-object',
                    stat_value: 0
                });
                session.call("viewport.webgl.data", [viewId, sceneObject.id, part]).then(function(data) {
                    try {
                        // decode base64
                        data = atob(data);
                        container.trigger({
                            type: 'stats',
                            stat_id: 'webgl-fetch-object',
                            stat_value: 1
                        });

                        newObject = {
                            md5: sceneObject.md5,
                            part: part,
                            vid: viewId,
                            id: sceneObject.id,
                            data: data,
                            hasTransparency: sceneObject.hasTransparency,
                            layer: sceneObject.layer,
                            render: function(){}
                        };

                        // Process object
                        initializeObject(gl, newObject);

                        // Register it for rendering
                        objectHandler.registerObject(newObject);

                        // Redraw the scene
                        drawScene(false);
                    } catch(error) {
                        console.log(error);
                    }
                });
            } catch(error) {
                console.log(error);
            }
        }

        // ------------------------------------------------------------------

        function drawScene(saveScreenOnRender) {
            try {
                if (sceneJSON === null || cameraLayerZero === null){
                    return;
                }
                var localRenderer, localWidth, localHeight, localX, localY,
                width = renderer.width(),
                height = renderer.height(),
                nbObjects = 0, layer, localCamera;

                // Update frame rate
                container.trigger({
                    type: 'stats',
                    stat_id: 'webgl-fps',
                    stat_value: 0
                });

                // Update viewport size
                ctx2d.canvas.width = width;
                ctx2d.canvas.height = height;
                gl.canvas.width = width;
                gl.canvas.height = height;
                gl.viewportWidth = width;
                gl.viewportHeight = height;

                // Clear 3D context
                gl.viewport(0, 0, gl.viewportWidth, gl.viewportHeight);
                gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

                // Draw background
                gl.disable(gl.DEPTH_TEST);
                if(background != null) {
                    cameraLayerZero.enableOrtho();
                    background.render(renderingContext, cameraLayerZero);
                    cameraLayerZero.enablePerspective();
                }
                gl.enable(gl.DEPTH_TEST);

                // Clear 2D overlay canvas
                ctx2d.clearRect(0, 0, width, height);

                // Render each layer on top of each other (Starting with the background one)
                cameraLayerZero.setViewSize(width, height);
                for(layer = sceneJSON.Renderers.length - 1; layer >= 0; layer--) {
                    localRenderer = sceneJSON.Renderers[layer];
                    localWidth = localRenderer.size[0] - localRenderer.origin[0];
                    localHeight = localRenderer.size[1] - localRenderer.origin[1];
                    localCamera = localRenderer.camera;

                    // Convert % to pixel based
                    localWidth *= width;
                    localHeight *= height;
                    localX = localRenderer.origin[0] * width;
                    localY = localRenderer.origin[1] * height;
                    localX = (localX < 0) ? 0 : localX;
                    localY = (localY < 0) ? 0 : localY;

                    // Update renderer camera aspect ratio
                    localCamera.setViewSize(localWidth, localHeight); // FIXME maybe use the local width/height

                    // Setup viewport
                    gl.viewport(localX, localY, localWidth, localHeight);

                    // Render non-transparent objects for the current layer
                    nbObjects += objectHandler.renderSolid(layer, renderingContext, localCamera);

                    // Now render transparent objects
                    gl.enable(gl.BLEND);                //Enable transparency
                    gl.enable(gl.POLYGON_OFFSET_FILL);  //Avoid zfighting
                    gl.polygonOffset(-1.0, -1.0);

                    nbObjects += objectHandler.renderTransparent(layer, renderingContext, localCamera);

                    gl.disable(gl.POLYGON_OFFSET_FILL);
                    gl.disable(gl.BLEND);
                }

                if (saveScreenOnRender === true) {
                    screenImage = renderingContext.gl.canvas.toDataURL();
                }

                // Update frame rate
                container.trigger({
                    type: 'stats',
                    stat_id: 'webgl-fps',
                    stat_value: 1
                });

                container.trigger({
                    type: 'stats',
                    stat_id: 'webgl-nb-objects',
                    stat_value: nbObjects
                });
            } catch(error) {
                console.log(error);
            }
            container.trigger('done');
        }

        // ------------------------------------------------------------------

        function pushCameraState() {
            if(cameraLayerZero != null) {
                var fp_ = cameraLayerZero.getFocalPoint(),
                up_ = cameraLayerZero.getViewUp(),
                pos_ = cameraLayerZero.getPosition(),
                fp = [fp_[0], fp_[1], fp_[2]],
                up = [up_[0], up_[1], up_[2]],
                pos = [pos_[0], pos_[1], pos_[2]];
                session.call("viewport.camera.update", [Number(options.view), fp, up, pos]);
            }
        }

        // ------------------------------------------------------------------

        function updateScene() {
            try{
                if(sceneJSON === null || typeof(sceneJSON) === "undefined") {
                    return;
                }

                // Local variables
                var bgColor1 = [0,0,0], bgColor2 = [0,0,0], renderer;

                // Create camera for each renderer + handle Background (Layer 0)
                otherCamera = [];
                for(var idx = 0; idx < sceneJSON.Renderers.length; idx++) {
                    renderer = sceneJSON.Renderers[idx];
                    renderer.camera = createCamera();
                    renderer.camera.setCenterOfRotation(sceneJSON.Center);
                    renderer.camera.setCameraParameters( renderer.LookAt[0],
                        [renderer.LookAt[7], renderer.LookAt[8], renderer.LookAt[9]],
                        [renderer.LookAt[1], renderer.LookAt[2], renderer.LookAt[3]],
                        [renderer.LookAt[4], renderer.LookAt[5], renderer.LookAt[6]]);

                    // Custom handling of layer 0
                    if(renderer.layer === 0) {
                        cameraLayerZero = renderer.camera;
                        bgColor1 = bgColor2 = renderer.Background1;
                        if(typeof(renderer.Background2) != "undefined") {
                            bgColor2 = renderer.Background2;
                        }
                    } else {
                        otherCamera.push(renderer.camera);
                    }
                }
                background = buildBackground(gl, bgColor1, bgColor2);

                // Update the list of object to render
                objectHandler.updateDisplayList(sceneJSON);

                // Fetch the object that we are missing
                objectHandler.fetchMissingObjects(fetchObject);

                // Draw scene
                drawScene(false);
            } catch(error) {
                console.log(error);
            }
        }

        // ------------------------------------------------------------------
        // Add renderer into the DOM
        container.append(renderer);

        // ------------------------------------------------------------------
        // Add viewport listener
        container.bind('invalidateScene', function() {
            if(renderer.hasClass('active')){
                fetchScene();
            }
        }).bind('render', function(){
            if(renderer.hasClass('active')){
                drawScene(false);
            }
        }).bind('resetViewId', function(e){
            options.view = -1;
        }).bind('captureRenderedImage', function(e){
            if (renderer.hasClass('active')) {
                drawScene(true);
                $(container).parent().trigger({
                    type: 'captured-screenshot-ready',
                    imageData: screenImage
                });
            }
        }).bind('mouse', function(event){
            if(renderer.hasClass('active')){
                event.preventDefault();

                if(event.action === 'down') {
                    mouseHandling.button = event.current_button;
                    mouseHandling.lastX = event.pageX;
                    mouseHandling.lastY = event.pageY;
                } else if (event.action === 'up') {
                    mouseHandling.button = null;
                } else if (event.action === 'move' && mouseHandling.button != null && cameraLayerZero != null) {
                    var newMouseX= event.pageX, newMouseY = event.pageY,
                        mouseDX = mouseHandling.lastX - newMouseX,
                        mouseDY = mouseHandling.lastY - newMouseY,
                        lastMouseX = mouseHandling.lastX,
                        lastMouseY = mouseHandling.lastY,
                        panD, zTrans,
                        focalPoint, focusWorldPt, focusDisplayPt,
                        displayPt1, displayPt2,
                        worldPt1, worldPt2,
                        width = renderer.width(),
                        height = renderer.height();

                    mouseHandling.lastX = newMouseX;
                    mouseHandling.lastY = newMouseY;

                    if (mouseHandling.button === 1) {
                        cameraLayerZero.rotate(mouseDX, mouseDY);
                        for(var i in otherCamera) {
                            otherCamera[i].rotate(mouseDX, mouseDY);
                        }
                    } else if (mouseHandling.button === 2) {
                        panD = cameraLayerZero.calculatePanDeltas(
                            width, height, newMouseX, newMouseY,
                            lastMouseX, lastMouseY);
                        cameraLayerZero.pan(-panD[0], -panD[1], -panD[2] );
                    } else if (mouseHandling.button === 3) {
                        zTrans = (newMouseY - lastMouseY) / height;

                        // Calculate zoom scale here
                        if (zTrans > 0) {
                            cameraLayerZero.zoom(1 - Math.abs(zTrans));
                        } else {
                            cameraLayerZero.zoom(1 + Math.abs(zTrans));
                        }
                    }

                    drawScene(false);
                    pushCameraState();
                }
            }
        }).bind('active', function(){
            if(renderer.hasClass('active')){
                // Setup GL context
                gl.viewportWidth = renderer.width();
                gl.viewportHeight = renderer.height();

                gl.clearColor(0.0, 0.0, 0.0, 1.0);
                gl.clearDepth(1.0);
                gl.enable(gl.DEPTH_TEST);
                gl.depthFunc(gl.LEQUAL);

                gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);

                initializeShader(gl, shaderProgram, pointShaderProgram);

                // Ready to render data
                fetchScene();
                drawScene(false);
            }
        });
    }

    // ----------------------------------------------------------------------
    // Camera object
    // ----------------------------------------------------------------------

    function createCamera() {
        var viewAngle = 30.0,
        centerOfRotation = vec3.set(vec3.create(), 0.0,0.0,-1.0),
        aspect = 1.0,
        left = -1.0,
        right = 1.0,
        bottom = -1.0,
        top = 1.0,
        near = 0.01,
        far = 10000.0,
        position = vec4.set(vec4.create(), 0.0, 0.0, 0.0, 1.0),
        focalPoint = vec4.set(vec4.create(), 0.0, 0.0, -1.0, 1.0),
        viewUp = vec4.set(vec4.create(), 0.0, 1.0, 0.0, 0.0),
        rightDir = vec4.set(vec4.create(), 1.0, 0.0, 0.0, 0.0),
        projectionMatrix = mat4.create(),
        modelViewMatrix = mat4.create(),
        perspective = true,
        width = 100,
        height = 100,
        modified = true;

        directionOfProjection = vec4.fromValues(0.0, 0.0, -1.0, 0.0),

        // Initialize to identity (just to be safe)
        mat4.identity(modelViewMatrix);
        mat4.identity(projectionMatrix);

        function computeOrthogonalAxes() {
            computeDirectionOfProjection();
            vec3.cross(rightDir, directionOfProjection, viewUp);
            vec3.normalize(rightDir, rightDir);
            modified = true;
        };
        function computeDirectionOfProjection() {
            vec3.subtract(directionOfProjection, focalPoint, position);
            vec3.normalize(directionOfProjection, directionOfProjection);
            modified = true;
        };

        function worldToDisplay(worldPt, width, height) {
            var viewProjectionMatrix = mat4.create();
            mat4.multiply(viewProjectionMatrix, projectionMatrix, modelViewMatrix),
            result = vec4.create();

            // Transform world to clipping coordinates
            var clipPt = vec4.create();
            vec4.transformMat4(clipPt, worldPt, viewProjectionMatrix);

            if (clipPt[3] !== 0.0) {
                clipPt[0] = clipPt[0] / clipPt[3];
                clipPt[1] = clipPt[1] / clipPt[3];
                clipPt[2] = clipPt[2] / clipPt[3];
                clipPt[3] = 1.0;
            }

            var winX = Math.round((((clipPt[0]) + 1) / 2.0) * width);
            // / We calculate -point3D.getY() because the screen Y axis is
            // / oriented top->down
            var winY = Math.round(((1 - clipPt[1]) / 2.0) * height);
            var winZ = clipPt[2];
            var winW = clipPt[3];

            vec4.set(result, winX, winY, winZ, winW);
            return result;
        };

        function displayToWorld(displayPt, width, height) {
            var x = (2.0 * displayPt[0] / width) - 1;
            var y = -(2.0 * displayPt[1] / height) + 1;
            var z = displayPt[2];

            var viewProjectionInverse = mat4.create();
            mat4.multiply(viewProjectionInverse, projectionMatrix, modelViewMatrix);
            mat4.invert(viewProjectionInverse, viewProjectionInverse);

            var worldPt = vec4.create();
            vec4.set(worldPt, x, y, z, 1);
            vec4.transformMat4(worldPt, worldPt, viewProjectionInverse);

            if (worldPt[3] !== 0.0) {
                worldPt[0] = worldPt[0] / worldPt[3];
                worldPt[1] = worldPt[1] / worldPt[3];
                worldPt[2] = worldPt[2] / worldPt[3];
                worldPt[3] = 1.0;
            }

            return worldPt;
        };

        return {
            calculatePanDeltas: function(
                width, height, newMouseX, newMouseY, lastMouseX, lastMouseY) {

                var dx,dy,dz,
                    focusDisplayPt, displayPt1, displayPt2,
                    worldPt1, worldPt2,
                    focusWorldPt = vec4.fromValues(
                        focalPoint[0], focalPoint[1], focalPoint[2], 1);

                focusDisplayPt =
                    worldToDisplay(
                        focusWorldPt, width, height);

                displayPt1 = vec4.fromValues(
                    newMouseX, newMouseY, focusDisplayPt[2], 1.0);

                displayPt2 = vec4.fromValues(
                    lastMouseX, lastMouseY, focusDisplayPt[2], 1.0);

                worldPt1 = displayToWorld(
                    displayPt1, width, height);

                worldPt2 = displayToWorld(
                    displayPt2, width, height);

                dx = worldPt1[0] - worldPt2[0];
                dy = worldPt1[1] - worldPt2[1];
                dz = worldPt1[2] - worldPt2[2];

                return [dx,dy,dz];
            },
            getFocalPoint: function() {
                return focalPoint;
            },
            getPosition: function() {
                return position;
            },
            getViewUp: function() {
                return viewUp;
            },
            setCenterOfRotation: function(center) {
                //console.log('[CAMERA] centerOfRotation ' + center);
                vec3.set(centerOfRotation, center[0], center[1], center[2]);
            },
            setCameraParameters : function(angle, pos, focal, up) {
                //console.log("[CAMERA] angle: " + angle + " position: " + pos + " focal: " + focal + " up: " + up );
                viewAngle = angle * Math.PI / 180;
                vec4.set(position, pos[0], pos[1], pos[2], 1.0);
                vec4.set(focalPoint, focal[0], focal[1], focal[2], 1.0);
                vec4.set(viewUp, up[0], up[1], up[2], 0.0);
                modified = true;
            },
            setViewSize : function(w, h) {
                //console.log('[CAMERA] width: ' + w + ' height: ' + h);
                aspect = w/h;
                width = w;
                height = h;
                modified = true;
            },
            enableOrtho : function() {
                perspective = false;
                modified = true;
            },
            enablePerspective : function() {
                perspective = true;
                modified = true;
            },
            zoom : function(d) {
                if (d === 0) {
                    return;
                }

                d = d * vec3.distance(focalPoint, position);
                position[0] = focalPoint[0] - d * directionOfProjection[0];
                position[1] = focalPoint[1] - d * directionOfProjection[1];
                position[2] = focalPoint[2] - d * directionOfProjection[2];

                modified = true;
                this.getCameraMatrices();
            },
            pan : function(dx, dy, dz) {
                position[0] += dx;
                position[1] += dy;
                position[2] += dz;

                focalPoint[0] += dx;
                focalPoint[1] += dy;
                focalPoint[2] += dz;
                computeOrthogonalAxes();

                modified = true;
            },
            rotate : function(dx, dy) {
                dx = 0.5 * dx * (Math.PI / 180.0);
                dy = 0.5 * dy * (Math.PI / 180.0);

                var mat = mat4.create(),
                inverseCenterOfRotation = new vec3.create();

                mat4.identity(mat);

                inverseCenterOfRotation[0] = -centerOfRotation[0];
                inverseCenterOfRotation[1] = -centerOfRotation[1];
                inverseCenterOfRotation[2] = -centerOfRotation[2];

                mat4.translate(mat, mat, centerOfRotation);
                mat4.rotate(mat, mat, dx, viewUp);
                mat4.rotate(mat, mat, dy, rightDir);
                mat4.translate(mat, mat, inverseCenterOfRotation);

                vec3.transformMat4(position, position, mat);
                vec3.transformMat4(focalPoint, focalPoint, mat);

                // Update viewup vector
                vec4.transformMat4(viewUp, viewUp, mat);
                vec4.normalize(viewUp, viewUp);

                computeOrthogonalAxes();
                modified = true;
                this.getCameraMatrices();
            },
            getCameraMatrices : function() {
                if (modified) {
                    // Compute project matrix
                    if (perspective) {
                        mat4.perspective(projectionMatrix, viewAngle, aspect, near, far);
                    } else {
                        mat4.ortho(projectionMatrix, left, right, bottom, top, near, far);
                    }

                    // Compute modelview matrix
                    computeOrthogonalAxes();
                    mat4.lookAt(modelViewMatrix, position, focalPoint, viewUp);
                    modified = false;
                };

                return [projectionMatrix, modelViewMatrix];
            }

        };
    }

    // ----------------------------------------------------------------------
    // Init vtkWeb module if needed
    // ----------------------------------------------------------------------
    if (GLOBAL.hasOwnProperty("vtkWeb")) {
        module = GLOBAL.vtkWeb || {};
    } else {
        GLOBAL.vtkWeb = module;
    }

    // ----------------------------------------------------------------------
    // Extend the viewport factory - ONLY IF WEBGL IS SUPPORTED
    // ----------------------------------------------------------------------
    try {
        if (GLOBAL.WebGLRenderingContext && typeof(vec3) != "undefined" && typeof(mat4) != "undefined") {
            var canvas = GLOBAL.document.createElement('canvas'),
            gl = canvas.getContext("webgl") || canvas.getContext("experimental-webgl");
            if(gl) {
                // WebGL is supported
                if(!module.hasOwnProperty('ViewportFactory')) {
                    module['ViewportFactory'] = {};
                }
                module.ViewportFactory[FACTORY_KEY] = FACTORY;
            }
        }
    } catch(exception) {
        // nothing to do
    }

    // ----------------------------------------------------------------------
    // Local module registration
    // ----------------------------------------------------------------------
    try {
      // Tests for presence of jQuery and glMatrix, then registers this module
      if ($ !== undefined && module.ViewportFactory[FACTORY_KEY] !== undefined) {
        module.registerModule('vtkweb-viewport-webgl');
      }
    } catch(err) {
      console.error('jQuery or glMatrix is missing or browser does not support WebGL: ' + err.message);
    }

}(window, jQuery));
