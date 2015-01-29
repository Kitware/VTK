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

  function getKey(object) {
      return object.id + '_' + object.md5 + '_' + object.part;
  }

  // ----------------------------------------------------------------------
  // 3D object handler
  // ----------------------------------------------------------------------

  function create3DObjectHandler() {
    var objectIndex = {}, displayList = {}, sceneJSON;

    return {
      fetchMissingObjects: function(fetchMethod, sceneJSON) {
        var fetch = fetchMethod, idx, part, i = 0;
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
      }
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
    gl = m_canvas3D.getContext("webgl") || m_canvas3D.getContext("experimental-webgl"),
    m_rendererAttrs = $(m_divContainer).addClass(FACTORY_KEY).css(RENDERER_CSS).append($(m_canvas2D).css(RENDERER_CSS).css(RENDERER_CSS_2D)).append($(m_canvas3D).css(RENDERER_CSS).css(RENDERER_CSS_3D)),
    m_sceneJSON = null,
    m_sceneData = null,
    m_objectHandler = create3DObjectHandler(),
    m_vglVtkReader = vgl.vtkReader(),
    m_viewer = null,
    m_interactorStyle,
    originalMouseDown =  document.onmousedown,
    originalMouseUp = document.onmouseup,
    originalMouseMove = document.onmousemove,
    originalContextMenu = document.oncontextmenu,
    m_background = null;
    screenImage = null,
    m_vglActors = {}

    // Helper functions -------------------------------------------------
    function fetchScene() {
      m_container.trigger({
        type: 'stats',
        stat_id: 'webgl-fetch-scene',
        stat_value: 0
      });
      m_session.call("viewport.webgl.metadata", [Number(m_options.view)]).then(function(data) {
        if (m_sceneData === data) {
          updateScene();
          return;
        }
        m_sceneData = data;
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
        newObject, renderer, actor, actors, key, i;

        m_container.trigger({
          type: 'stats',
          stat_id: 'webgl-fetch-object',
          stat_value: 0
        });
        m_session.call("viewport.webgl.data", [viewId, sceneObject.id, part]).then(function(data) {
          try {
            m_container.trigger({
              type: 'stats',
              stat_id: 'webgl-fetch-object',
              stat_value: 1
            });

            //add object to the reader
            newObject = {
              md5: sceneObject.md5,
              part: part,
              vid: viewId,
              id: sceneObject.id,
              data: data,
              hasTransparency: sceneObject.transparency,
              layer: sceneObject.layer
            };

            renderer = m_vglVtkReader.getRenderer(sceneObject.layer);
            key = getKey(newObject);

            // Parse the new object if its not parsed already
            // if parsed already then check if exists in  current renderer
            if (key in m_vglActors) {
                actors = m_vglActors[key];
                // if exists in current renderer do nothing
                for (i = 0; i < actors.length; i++) {
                    actor = actors[i];
                    if (!renderer.hasActor(actor)) {
                        renderer.addActor(actor);
                    }
                }
            }
            // if not parsed then parse it, create actor and add it to the
            // renderer.
            else {
                actors = m_vglVtkReader.parseObject(newObject);
                m_vglActors[key] = actors;

                for (i = 0; i < actors.length; i++) {
                    renderer.addActor(actors[i]);
                }
            }
            // Mark the actor as valid
            actors.invalid = false;

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

    function render(saveScreenOnRender) {
      m_canvas3D.width = m_rendererAttrs.width();
      m_canvas3D.height = m_rendererAttrs.height();
      m_viewer = m_vglVtkReader.updateCanvas(m_canvas3D);
      drawScene(saveScreenOnRender);
    }

    // ------------------------------------------------------------------

    function drawScene(saveScreenOnRender) {
      var layer;

      try {
        if (m_sceneJSON === null || typeof m_sceneJSON === 'undefined') {
          return;
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

        m_viewer.render();

        if (saveScreenOnRender === true) {
          screenImage = m_canvas3D.toDataURL();
        }

        numObjects = m_vglVtkReader.numObjects();

        // Update frame rate
        m_container.trigger({
          type: 'stats',
          stat_id: 'webgl-fps',
          stat_value: 1
        });

        m_container.trigger({
          type: 'stats',
          stat_id: 'webgl-nb-objects',
          stat_value: numObjects
        });
      } catch(error) {
        console.log(error);
      }
      m_container.trigger('done');
    }

    // ------------------------------------------------------------------

    function pushCameraState() {
      if(m_viewer !== null) {
        var cam =  m_viewer.renderWindow().activeRenderer().camera(),
            fp_ = cam.focalPoint(),
            up_ = cam.viewUpDirection(),
            pos_ = cam.position(),
            fp = [fp_[0], fp_[1], fp_[2]],
            up = [up_[0], up_[1], up_[2]],
            pos = [pos_[0], pos_[1], pos_[2]];
        m_session.call("viewport.camera.update", [Number(m_options.view), fp, up, pos]);
      }
    }

    // ------------------------------------------------------------------

    function updateScene() {
      var key;

      try{
        if(m_sceneJSON === null || typeof(m_sceneJSON) === "undefined") {
          return;
        }

        m_vglVtkReader.initScene();

        // Mark all actors as invalid
        for (key in m_vglActors) {
          if (m_vglActors[key].invalid !== undefined &&
              !m_vglActors[key].invalid) {
            delete m_vglActors[key];
          }
          else {
            m_vglActors[key].invalid = true;
          }
        }

        // Fetch the object that we are missing
        m_objectHandler.fetchMissingObjects(fetchObject, m_sceneJSON);

        // Draw scene
        drawScene(false);
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

        if (m_vglVtkReader === null) {
          m_vglVtkReader = vgl.vtkReader();
        } else {
          m_vglVtkReader.deleteViewer();
        }

        m_canvas3D.width = m_rendererAttrs.width();
        m_canvas3D.height = m_rendererAttrs.height();
        m_viewer = m_vglVtkReader.createViewer(m_canvas3D);
        m_viewer.renderWindow().activeRenderer().setResetScene(false);
        m_viewer.renderWindow().activeRenderer().setResetClippingRange(false);



        // Bind mouse event handlers
        m_container.on('mouse', function(event) {
            if (m_viewer) {
              if (event.action === 'move') {
                m_viewer.handleMouseMove(event.originalEvent);
              }
              else if (event.action === 'up') {
                m_viewer.handleMouseUp(event.originalEvent);
              }
              else if (event.action === 'down') {
                m_viewer.handleMouseDown(event.originalEvent);
              }
            }
        });

        fetchScene();
      }
    }).bind('render', function(){
      if(m_rendererAttrs.hasClass('active')){
        render(false);
      }
    }).bind('captureRenderedImage', function(e){
      if (m_rendererAttrs.hasClass('active')) {
          render(true);
          $(m_container).parent().trigger({
              type: 'captured-screenshot-ready',
              imageData: screenImage
          });
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
