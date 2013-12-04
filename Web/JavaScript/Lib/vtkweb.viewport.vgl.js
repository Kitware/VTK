/**
 * vtkWeb JavaScript Library.
 *
 * This module extend the vtkWeb viewport to add support for WebGL rendering.
 *
 * @class vtkWeb.viewports.vgl
 *
 *     Viewport Factory description:
 *       - Key: vgl
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
    FACTORY_KEY = 'vgl',
    FACTORY = {
        'builder': createVGLGeometryDeliveryRenderer,
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
    // Geometry Delivery renderer - factory method
    // ----------------------------------------------------------------------

    function createVGLGeometryDeliveryRenderer(domElement) {
        var m_container = $(domElement),
        m_options = $.extend({}, DEFAULT_OPTIONS, m_container.data('config')),
        m_session = m_options.session,
        m_divContainer = GLOBAL.document.createElement('div'),
        m_canvas2D = GLOBAL.document.createElement('canvas'),
        m_canvas3D = GLOBAL.document.createElement('canvas'),
        m_ctx2d = m_canvas2D.getContext('2d'),
        gl = m_canvas3D.getContext("experimental-webgl") || m_canvas3D.getContext("webgl"),
        m_rendererAttrs = $(m_divContainer).addClass(FACTORY_KEY).css(RENDERER_CSS).append($(m_canvas2D).css(RENDERER_CSS).css(RENDERER_CSS_2D)).append($(m_canvas3D).css(RENDERER_CSS).css(RENDERER_CSS_3D)),
        m_sceneJSON = null,
        m_objectHandler = create3DObjectHandler(),
        m_vglVtkReader = ogs.vgl.vtkReader(),
        m_viewer,
        m_interactorStyle,
        m_background = null;

        // Helper functions -------------------------------------------------
        function fetchScene() {
            m_container.trigger({
                type: 'stats',
                stat_id: 'webgl-fetch-scene',
                stat_value: 0
            });
            m_session.call("vtk:getSceneMetaData", Number(m_options.view)).then(function(data) {
                m_sceneJSON = JSON.parse(data);
                m_vglVtkReader.setVtkScene(m_sceneJSON);
                m_container.trigger({
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
                var viewId = Number(m_options.view),
                newObject;

                m_container.trigger({
                    type: 'stats',
                    stat_id: 'webgl-fetch-object',
                    stat_value: 0
                });
                m_session.call("vtk:getWebGLData", viewId, sceneObject.id, part).then(function(data) {
                    try {
                      //add object to the reader
                        newObject = {
                            md5: sceneObject.md5,
                            part: part,
                            vid: viewId,
                            id: sceneObject.id,
                            data: data,
                            hasTransparency: sceneObject.hasTransparency,
                            layer: sceneObject.layer
                        };

                        m_vglVtkReader.addVtkObjectData(newObject);
                        m_container.trigger({
                            type: 'stats',
                            stat_id: 'webgl-fetch-object',
                            stat_value: 1
                        });

                        // Redraw the scene
                        drawScene();
                    } catch(error) {
                        console.log(error);
                    }
                });
            } catch(error) {
                console.log(error);
            }
        }

        // ------------------------------------------------------------------

        function drawScene() {
            try {
                if (m_viewer === null){
                  //the viewer could be null here because of async
                  //timing fetching scene and objects, so try to create it.
                  m_viewer = m_vglVtkReader.createViewer(m_canvas3D);
                  if (m_viewer === null){
                    return;
                  }
                }

                var width = m_rendererAttrs.width(),
                    height = m_rendererAttrs.height(),
                    nbObjects;

                // Update frame rate
                m_container.trigger({
                    type: 'stats',
                    stat_id: 'webgl-fps',
                    stat_value: 0
                });

              // Clear 2D overlay canvas
              m_ctx2d.canvas.width = width;
              m_ctx2d.canvas.height = height;
              m_ctx2d.clearRect(0, 0, width, height);

              HTMLCanvasElement.prototype.relMouseCoords =
                m_viewer.relMouseCoords;

              document.onmousedown = m_viewer.handleMouseDown;
              document.onmouseup = m_viewer.handleMouseUp;
              document.onmousemove = m_viewer.handleMouseMove;
              document.oncontextmenu = m_viewer.handleContextMenu;

              m_interactorStyle = m_viewer.interactorStyle();
              $(m_interactorStyle).on(ogs.vgl.command.leftButtonPressEvent, m_viewer.render);
              $(m_interactorStyle).on(ogs.vgl.command.middleButtonPressEvent, m_viewer.render);
              $(m_interactorStyle).on(ogs.vgl.command.rightButtonPressEvent, m_viewer.render);

              m_viewer.render();

                // Update frame rate
                m_container.trigger({
                    type: 'stats',
                    stat_id: 'webgl-fps',
                    stat_value: 1
                });

                m_container.trigger({
                    type: 'stats',
                    stat_id: 'webgl-nb-objects',
                    stat_value: nbObjects
                });
            } catch(error) {
                console.log(error);
            }
        }

        // ------------------------------------------------------------------

        function pushCameraState() {
            if(m_viewer !== null) {
              var cam =  m_viewer.renderWindow().activeRenderer().camera(),
                  fp = cam.focalPoint(),
                  up = cam.viewUpDirection(),
                  pos = cam.position();

              m_session.call("vtk:updateCamera", Number(m_options.view), fp, up, pos);
            }
        }

        // ------------------------------------------------------------------

        function updateScene() {
            try{
                if(m_sceneJSON === null || typeof(m_sceneJSON) === "undefined") {
                    return;
                }

                // Update the list of object to render
                m_objectHandler.updateDisplayList(m_sceneJSON);

                // Fetch the object that we are missing
                m_objectHandler.fetchMissingObjects(fetchObject);

                // Draw scene
                drawScene();
            } catch(error) {
                console.log(error);
            }
        }

        // ------------------------------------------------------------------
        // Add rendererAttrs into the DOM
        m_container.append(m_rendererAttrs);

        // ------------------------------------------------------------------
        // Add viewport listener
        m_container.bind('invalidateScene', function() {
          if(m_rendererAttrs.hasClass('active')){
              fetchScene();
              m_canvas3D.width = m_rendererAttrs.width();
              m_canvas3D.height = m_rendererAttrs.height();
              m_viewer = m_vglVtkReader.createViewer(m_canvas3D);
            }
        }).bind('render', function(){
            if(m_rendererAttrs.hasClass('active')){
              m_canvas3D.width = m_rendererAttrs.width();
              m_canvas3D.height = m_rendererAttrs.height();
              m_viewer = m_vglVtkReader.createViewer(m_canvas3D);
              drawScene();
            }
        }).bind('resetViewId', function(e){
            m_options.view = -1;
        }).bind('mouse', function(event){
            if(m_rendererAttrs.hasClass('active')){
                event.preventDefault();
              pushCameraState();
            }

        }).bind('active', function(){
            if(m_rendererAttrs.hasClass('active')){

              // Ready to render data
              fetchScene();

              m_canvas3D.width = m_rendererAttrs.width();
              m_canvas3D.height = m_rendererAttrs.height();

              //try to create the viewer, but we really may not have everything
              // we need yet. TODO: Maybe we just move this call to the drawScene func
              m_viewer = m_vglVtkReader.createViewer(m_canvas3D);

              drawScene();
            }
        });
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
        module.registerModule('vtkweb-viewport-vgl');
      }
    } catch(err) {
      console.error('jQuery or glMatrix is missing or browser does not support WebGL: ' + err.message);
    }

}(window, jQuery));
