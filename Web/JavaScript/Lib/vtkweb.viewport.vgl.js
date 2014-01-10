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
  // 3D object handler
  // ----------------------------------------------------------------------

  function create3DObjectHandler() {
    var objectIndex = {}, displayList = {}, sceneJSON;

    // ------------------------------------------------------------------

    function getKey(object) {
      return object.id + '_' + object.md5;
    }

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
    gl = m_canvas3D.getContext("experimental-webgl") || m_canvas3D.getContext("webgl"),
    m_rendererAttrs = $(m_divContainer).addClass(FACTORY_KEY).css(RENDERER_CSS).append($(m_canvas2D).css(RENDERER_CSS).css(RENDERER_CSS_2D)).append($(m_canvas3D).css(RENDERER_CSS).css(RENDERER_CSS_3D)),
    m_sceneJSON = null,
    m_objectHandler = create3DObjectHandler(),
    m_vglVtkReader = ogs.vgl.vtkReader(),
    m_viewer = null,
    m_interactorStyle,
    originalMouseDown =  document.onmousedown,
    originalMouseUp = document.onmouseup,
    originalMouseMove = document.onmousemove,
    originalContextMenu = document.oncontextmenu,

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
            if (newObject.layer === 0) {
              m_vglVtkReader.addVtkObjectData(newObject);
            }
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
      var layer;

      try {
        if (m_sceneJSON === null || typeof m_sceneJSON === 'undefined') {
          return;
        }

        m_viewer = m_vglVtkReader.updateViewer(m_canvas3D);

        if (m_viewer === null) {
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
            fp_ = cam.getFocalPoint(),
            up_ = cam.getViewUp(),
            pos_ = cam.getPosition(),
            fp = [fp_[0], fp_[1], fp_[2]],
            up = [up_[0], up_[1], up_[2]],
            pos = [pos_[0], pos_[1], pos_[2]];
        session.call("vtk:updateCamera", Number(m_options.view), fp, up, pos);
      }
    }

    // ------------------------------------------------------------------

    function updateScene() {
      try{
        if(m_sceneJSON === null || typeof(m_sceneJSON) === "undefined") {
          return;
        }

        // Fetch the object that we are missing
        m_objectHandler.fetchMissingObjects(fetchObject, m_sceneJSON);

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
        m_vglVtkReader = ogs.vgl.vtkReader();
        m_canvas3D.width = m_rendererAttrs.width();
        m_canvas3D.height = m_rendererAttrs.height();
        fetchScene();
      }
      else {
        originalMouseDown =  document.onmousedown;
        originalMouseUp = document.onmouseup;
        originalMouseMove = document.onmousemove;
        originalContextMenu = document.oncontextmenu;
      }
    }).bind('render', function(){
      if(m_rendererAttrs.hasClass('active')){
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
