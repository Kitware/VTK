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
 *         - vgl-fps
 *         - vgl-nb-objects
 *         - vgl-fetch-scene
 *         - vgl-fetch-object
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
      'vgl-fps': {
        label: 'Framerate',
        type: 'time',
        convert: function(value) {
          if(value === 0) {
            return 0;
          }
          return (1000 / value).toFixed(2);
        }
      },
      'vgl-nb-objects': {
        label: 'Number&nbsp;of&nbsp;3D&nbsp;objects',
        type: 'value',
        convert: NoOp
      },
      'vgl-fetch-scene': {
        label: 'Fetch&nbsp;scene&nbsp;(ms)',
        type: 'time',
        convert: NoOp
      },
      'vgl-fetch-object': {
        label: 'Fetch&nbsp;object&nbsp;(ms)',
        type: 'time',
        convert: NoOp
      }
    }
  },
  DEFAULT_SHADERS = {},
  mvMatrixStack = [],
  PROGRESS_BAR_TEMPLATE =
  '<div class="download-progressbar-container">' +
  '    <span class="progressbar-title">Download Progress</span>' +
  '    <div class="progressbar-content-container">' +
  '        <span class="progress-message-span">MESSAGE</span>' +
  '        <div class="progress progress-meter-container">' +
  '            <div class="progress-bar progress-bar-striped active progress-meter" role="progressbar" aria-valuenow="100" aria-valuemin="0" aria-valuemax="100" style="width: 100%;">' +
  '            </div>' +
  '        </div>' +
  '    </div>' +
  '</div>';


  // ----------------------------------------------------------------------

  function NoOp(a) {
    return a;
  }

  function getKey(object) {
      return object.id + '_' + object.md5 + '_' + object.part;
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
    m_vglVtkReader = vgl.vtkReader(),
    m_viewer = null,
    m_interactorStyle,
    originalMouseDown =  document.onmousedown,
    originalMouseUp = document.onmouseup,
    originalMouseMove = document.onmousemove,
    originalContextMenu = document.oncontextmenu,
    m_background = null;
    screenImage = null,
    m_vglActors = {},
    m_objectIndex = {},
    m_numberOfPartsDownloaded = 0,
    m_numberOfPartsToDownload = 0;

    // Helper functions -------------------------------------------------

    function fetchMissingObjects(fetchMethod, sceneJSON) {
      for(var idx in sceneJSON.Objects) {
        var currentObject = sceneJSON.Objects[idx];
        for(var part = 1; part <= currentObject.parts; part++) {
          var key = getKey({
            'id': currentObject.id,
            'md5': currentObject.md5,
            'part': part
          });
          if(!m_objectIndex.hasOwnProperty(key)) {
            fetchMethod(currentObject, part);
          } else {
            handleCompleteSceneObject(m_objectIndex[key]);
          }
        }
      }
    }

    // ------------------------------------------------------------------

    function fetchScene() {
      m_container.trigger({
        type: 'stats',
        stat_id: 'vgl-fetch-scene',
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
          stat_id: 'vgl-fetch-scene',
          stat_value: 1
        });
        updateScene();
      });
    }

    // ------------------------------------------------------------------

    function handleDownloadProgressEvent(event) {
      var status = event.status;

      function updateProgressBar(element, percentProgress) {
        var progressString = percentProgress + '%';
        element.attr('aria-valuenow', percentProgress)
               .css('width', progressString);
               //.html(progressString);
      }

      if (status === 'create') {
        var initialMessage = "Downloading metadata for all timesteps",
            html = PROGRESS_BAR_TEMPLATE.replace(/MESSAGE/, initialMessage),
            progressElt = $(html);
        m_container.append(progressElt);
      } else if (status === 'update') {
        var progressType = event.progressType,
            pbElt = $('.progress-meter');
        if (progressType === 'retrieved-metadata') {
          $('.progress-message-span').text('Metadata retrieved, downloading objects');
          pbElt.removeClass('progress-bar-striped active');
          updateProgressBar(pbElt, 0);
        } else {
          var numPartsThisSha = event.numParts;
          m_numberOfPartsDownloaded += numPartsThisSha;
          var numberRemaining = m_numberOfPartsToDownload - m_numberOfPartsDownloaded;
          var percent = ((m_numberOfPartsDownloaded / m_numberOfPartsToDownload) * 100).toFixed(0);
          if (numberRemaining <= 0) {
            $('.download-progressbar-container').remove();
          } else {
            $('.progress-message-span').text('Downloading objects');
            updateProgressBar(pbElt, percent);
          }
        }
      }
    }

    // ------------------------------------------------------------------

    function downloadAllTimesteps() {
      m_container.trigger({
        type: 'downloadProgress',
        status: 'create'
      });

      m_session.call('viewport.webgl.metadata.alltimesteps', []).then(function(result){
        if (result.hasOwnProperty('success') && result.success === true) {
          var metaDataList = result.metaDataList;

          m_container.trigger({
            type: 'downloadProgress',
            status: 'update',
            progressType: 'retrieved-metadata'
          });

          // For progress events, I want to first know how many items to retrieve
          m_numberOfPartsToDownload = 0;
          for (var sha in metaDataList) {
            if (metaDataList.hasOwnProperty(sha)) {
              m_numberOfPartsToDownload += metaDataList[sha].numParts;
            }
          }

          m_numberOfPartsDownloaded = 0;

          setTimeout(function() {

            // Now go through and download the heavy data for anythin we don't already have
            for (var sha in metaDataList) {
              if (metaDataList.hasOwnProperty(sha)) {
                var numParts = metaDataList[sha].numParts,
                    objId = metaDataList[sha].id,
                    alreadyCached = true;
                // Before I go and fetch all the parts for this object, make sure
                // I don't already have them cached
                for (var i = 0; i < numParts; i+=1) {
                  var key = getKey({
                    'id': objId,
                    'md5': sha,
                    'part': i + 1
                  });
                  if(!m_objectIndex.hasOwnProperty(key)) {
                    alreadyCached = false;
                    break;
                  }
                }
                if (alreadyCached === false) {
                  fetchCachedObject(sha);
                } else {
                  m_container.trigger({
                    type: 'downloadProgress',
                    status: 'update',
                    numParts: numParts
                  });
                }
              }
            }

          }, 500);
        }
      }, function(metaDataError) {
        console.log("Error retrieving metadata for all timesteps");
        console.log(metaDataError);
      });
    }

    // ------------------------------------------------------------------

    function fetchCachedObject(sha) {
      var viewId = Number(m_options.view);

      m_session.call('viewport.webgl.cached.data', [sha]).then(function(result) {
        if (result.success === false) {
          console.log("Fetching cached data for " + sha + " failed, reason:");
          consolelog(result.reason);
          return;
        }
        var dataObject = result.data;
        if (dataObject.hasOwnProperty('partsList')) {
          for (var dIdx = 0; dIdx < dataObject.partsList.length; dIdx += 1) {
            // Create a complete scene part object and cache it
            var newObject = {
              md5: dataObject.md5,
              part: dIdx + 1,
              vid: viewId,
              id: dataObject.id,
              data: dataObject.partsList[dIdx],
              hasTransparency: dataObject.transparency,
              layer: dataObject.layer
            };

            var key = getKey(newObject);
            m_objectIndex[key] = newObject;

            var actors = m_vglVtkReader.parseObject(newObject);
            m_vglActors[key] = actors;
          }

          m_container.trigger({
            type: 'downloadProgress',
            status: 'update',
            numParts: dataObject.partsList.length
          });
        }
      }, function(err) {
        console.log('viewport.webgl.cached.data rpc method failed');
        console.log(err);
      });
    }

    // ------------------------------------------------------------------

    function handleCompleteSceneObject(sceneObject) {
        var renderer = m_vglVtkReader.getRenderer(sceneObject.layer),
            key = getKey(sceneObject),
            actors = {};

        // Parse the new object if its not parsed already
        // if parsed already then check if exists in  current renderer
        if (key in m_vglActors) {
            actors = m_vglActors[key];
            // if exists in current renderer do nothing
            for (i = 0; i < actors.length; i++) {
                var actor = actors[i];
                if (!renderer.hasActor(actor)) {
                    renderer.addActor(actor);
                }
            }
        } else {
            // Object was not parsed so parse it, create actors, and add them to the renderer.
            actors = m_vglVtkReader.parseObject(sceneObject);
            m_vglActors[key] = actors;

            for (i = 0; i < actors.length; i++) {
                renderer.addActor(actors[i]);
            }
        }

        // Mark the actor as valid
        actors.invalid = false;
    }

    // ------------------------------------------------------------------

    function fetchObject(sceneObject, part) {
      try {
        var viewId = Number(m_options.view),
        newObject, renderer, actor, actors, key, i;

        m_container.trigger({
          type: 'stats',
          stat_id: 'vgl-fetch-object',
          stat_value: 0
        });
        m_session.call("viewport.webgl.data", [viewId, sceneObject.id, part]).then(function(data) {
          try {
            m_container.trigger({
              type: 'stats',
              stat_id: 'vgl-fetch-object',
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

            // First, actually cache the object, because that was not happening
            var key = getKey(newObject);
            m_objectIndex[key] = newObject;

            // Now add the object to the reader, etc...
            handleCompleteSceneObject(newObject);

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
          stat_id: 'vgl-fps',
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
          stat_id: 'vgl-fps',
          stat_value: 1
        });

        m_container.trigger({
          type: 'stats',
          stat_id: 'vgl-nb-objects',
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
          m_vglActors[key].invalid = true;
        }

        // Fetch the object that we are missing
        fetchMissingObjects(fetchObject, m_sceneJSON);

        // Draw scene
        drawScene(false);
      } catch(error) {
        console.log(error);
      }
    }

    // ------------------------------------------------------------------

    function clearCache() {
      m_objectIndex = {};
      m_vglActors = {};
      m_container.trigger('invalidateScene');
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
    }).bind('downloadAllTimesteps', function(event){
      if(m_rendererAttrs.hasClass('active')){
        downloadAllTimesteps();
      }
    }).bind('clearCache', function(event){
      if(m_rendererAttrs.hasClass('active')){
        clearCache();
      }
    }).bind('downloadProgress', function(event){
      if(m_rendererAttrs.hasClass('active')){
        handleDownloadProgressEvent(event);
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
