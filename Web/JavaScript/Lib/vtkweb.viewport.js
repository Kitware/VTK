/**
 * vtkWeb JavaScript Library.
 *
 * This module allow the Web client to create viewport to vtkWeb views.
 * Those viewport are interactive windows that are used to render 2D/3D content
 * and response to user mouse interactions.
 *
 * This module registers itself as: 'vtkweb-viewport'
 *
 * @class vtkWeb.Viewport
 */
(function (GLOBAL, $) {

    // ----------------------------------------------------------------------
    // Viewport constants
    // ----------------------------------------------------------------------

    var DEFAULT_RENDERERS_CONTAINER_HTML = "<div class='renderers'></div>",
    DEFAULT_RENDERERS_CONTAINER_CSS = {
        "position": "absolute",
        "top": "0px",
        "left": "0px",
        "right": "0px",
        "bottom": "0px",
        "z-index" : "0",
        "overflow": "hidden"
    },

    DEFAULT_MOUSE_LISTENER_HTML = "<div class='mouse-listener'></div>",
    DEFAULT_MOUSE_LISTENER_CSS = {
        "position": "absolute",
        "top": "0px",
        "left": "0px",
        "right": "0px",
        "bottom": "0px",
        "z-index" : "3"
    },

    DEFAULT_STATISTIC_HTML = "<div class='statistics'></div>",
    DEFAULT_STATISTIC_CSS = {
        "position": "absolute",
        "top": "0px",
        "left": "0px",
        "right": "0px",
        "bottom": "0px",
        "z-index" : "2",
        "display" : "none"
    },

    DEFAULT_OVERLAY_HTML = "<canvas class='overlay'></canvas>",
    DEFAULT_OVERLAY_CSS = {
        "position": "absolute",
        "top": "0px",
        "left": "0px",
        "right": "0px",
        "bottom": "0px",
        "z-index" : "1"
    },

    module = {},

    DEFAULT_VIEWPORT_OPTIONS = {
        session: null,
        view: -1,
        enableInteractions: true,
        renderer: 'image'
    };

    // ----------------------------------------------------------------------
    // Mouse interaction helper methods for viewport
    // ----------------------------------------------------------------------

    function preventDefault(event) {
        event.preventDefault();
    }

    // ----------------------------------------------------------------------

    function attachMouseListener(mouseListenerContainer, renderersContainer, overlayContainer, viewport) {
        var current_button = null,
        area = [0,0,0,0],
        vp = viewport,
        overlayCtx2D = overlayContainer[0].getContext('2d');

        // Draw rectangle in overlay
        function clearOverlay() {
            if(overlayCtx2D !== null) {
                overlayCtx2D.canvas.width = mouseListenerContainer.width();
                overlayCtx2D.canvas.height = mouseListenerContainer.height();
                overlayCtx2D.clearRect(0,0,overlayCtx2D.canvas.width,overlayCtx2D.canvas.height);
            }
        }

        function redrawSelection() {
            clearOverlay();
            if(overlayCtx2D !== null) {
                overlayCtx2D.strokeStyle="#FFFFFF";
                var x1 = Math.min(area[0],area[2]);
                var y1 = Math.min(area[1],area[3]);
                var x2 = Math.max(area[0],area[2]);
                var y2 = Math.max(area[1],area[3]);
                var width = Math.abs(x2 - x1);
                var height = Math.abs(y2 - y1);
                overlayCtx2D.rect(x1, overlayContainer.height()-y2, width, height);
                overlayCtx2D.stroke();
            }
        }

        function extractCoordinates(event, start, reorder) {
            var elem_position = $(event.delegateTarget).offset(),
            height = $(event.delegateTarget).height(),
            x = (event.pageX - elem_position.left),
            y = (event.pageY - elem_position.top),
            offset = start ? 0 : 2;
            area[0 + offset] = x;
            area[1 + offset] = height - y;

            if(reorder) {
                // Re-order area
                var newArea = [
                    Math.min(area[0], area[2]),
                    Math.min(area[1], area[3]),
                    Math.max(area[0], area[2]),
                    Math.max(area[1], area[3])
                ];
                area[0] = newArea[0];
                area[1] = newArea[1];
                area[2] = newArea[2];
                area[3] = newArea[3];
            }

        }

        // Internal method used to pre-process the interaction to standardise it
        // for a vtkWeb usage.
        function mouseInteraction(event) {
            if(event.hasOwnProperty("type")) {
                if(event.type === 'mouseup') {
                    current_button = null;
                    if (vp.getSelectionMode() === true) {
                        clearOverlay();
                    }
                    renderersContainer.trigger($.extend(event, {
                        type: 'mouse',
                        action: 'up',
                        current_button: current_button
                    }));
                    extractCoordinates(event, false, true);
                    renderersContainer.trigger({
                        type: 'endInteraction',
                        area: area
                    });
                } else if(event.type === 'mousedown') {
                    current_button = event.which;
                    // Override button if modifier is used
                    // Middle: Alt - Right: Shift
                    if(event.shiftKey) {
                        current_button = 3;
                        event.shiftKey = false;
                    } else if(event.altKey) {
                        current_button = 2;
                        event.altKey = false;
                    }
                    extractCoordinates(event, true, false);
                    renderersContainer.trigger('startInteraction');
                    renderersContainer.trigger($.extend(event, {
                        type: 'mouse',
                        action: 'down',
                        current_button: current_button
                    }));

                } else if(event.type === 'mousemove' && current_button != null) {
                    if (vp.getSelectionMode() === true) {
                        extractCoordinates(event, false, false);
                        redrawSelection();
                    }
                    renderersContainer.trigger($.extend(event, {
                        type: 'mouse',
                        action: 'move',
                        current_button: current_button
                    }));
                }
            }
        }

        // Bind listener to UI container
        mouseListenerContainer.bind("contextmenu mouseover click", preventDefault);
        mouseListenerContainer.bind('mousedown mouseup mousemove', mouseInteraction);
        mouseListenerContainer.dblclick(function(event){
            renderersContainer.trigger($.extend(event, {
                type: 'mouse',
                action: 'dblclick',
                current_button: event.which
            }));
        });
        mouseListenerContainer.bind("DOMMouseScroll mousewheel",function(event){
            var scrollValue = (event.originalEvent.wheelDeltaY || -event.originalEvent.detail);
            renderersContainer.trigger($.extend(event, {
                type: 'mouse',
                action: 'scroll',
                current_button: current_button,
                scroll: scrollValue
            }));
        });
    }

    // ----------------------------------------------------------------------

    function attachTouchListener(mouseListenerContainer, renderersContainer, viewport) {
        var current_button = null, posX, posY, defaultDragButton = 1,
        isZooming = false, isDragging = false, mouseAction = 'up', target;

        function mobileTouchInteraction(evt) {
            evt.gesture.preventDefault();
            switch(evt.type) {
                case 'drag':
                    if(isZooming) {
                        return;
                    }
                    current_button = defaultDragButton;
                    if(mouseAction === 'up') {
                        mouseAction = "down";

                        target = evt.gesture.target;
                        isDragging = true;
                    } else {
                        mouseAction = "move";
                    }

                    posX = evt.gesture.touches[0].pageX;
                    posY = evt.gesture.touches[0].pageY;
                    break;
                case 'hold':
                    if(defaultDragButton === 1) {
                        defaultDragButton = 2;
                        mouseListenerContainer.html("Pan mode").css('color','#FFFFFF');
                    } else {
                        defaultDragButton = 1;
                        mouseListenerContainer.html("Rotation mode").css('color','#FFFFFF');
                    }

                    break;
                case 'release':
                    mouseListenerContainer.html('');
                    current_button = 0;
                    mouseAction = "up";
                    isZooming = false;
                    isDragging = false;
                    break;
                case 'doubletap':
                    viewport.resetCamera();
                    return;
                case 'pinch':
                    if(isDragging) {
                        return;
                    }
                    current_button = 3;
                    if(mouseAction === 'up') {
                        mouseAction = 'down';
                        posX = 0;
                        posY = mouseListenerContainer.height();
                        target = evt.gesture.target;
                        isZooming = true;
                    } else {
                        mouseAction = 'move';
                        posY = mouseListenerContainer.height() * (1+(evt.gesture.scale-1)/2);
                    }
                    break;
            }

            //mouseListenerContainer.html(mouseAction + ' (' + posX + ', ' + posY + ') b:' + current_button + ' z: ' + isZooming ).css('color','#FFFFFF');

            // Trigger event
            renderersContainer.trigger({
                type: 'mouse',
                action: mouseAction,
                current_button: current_button,
                charCode: '',
                altKey: false,
                ctrlKey: false,
                shiftKey: false,
                metaKey: false,
                delegateTarget: target,
                pageX: posX,
                pageY: posY
            });
        }

        // Bind listener to UI container
        mouseListenerContainer.hammer({
            prevent_default : true,
            prevent_mouseevents : true,
            transform : true,
            transform_always_block : true,
            transform_min_scale : 0.03,
            transform_min_rotation : 2,
            drag : true,
            drag_max_touches : 1,
            drag_min_distance : 10,
            swipe : false,
            hold : true // To switch from rotation to pan
        }).on("doubletap pinch drag release hold", mobileTouchInteraction);
    }

    // ----------------------------------------------------------------------
    // Viewport statistic manager
    // ----------------------------------------------------------------------

    function createStatisticManager() {
        var statistics = {}, formatters = {};

        // Fill stat formatters
        for(var factoryKey in vtkWeb.ViewportFactory) {
            var factory = vtkWeb.ViewportFactory[factoryKey];
            if(factory.hasOwnProperty('stats')) {
                for(var key in factory.stats) {
                    formatters[key] =  factory.stats[key];
                }
            }
        }

        function handleEvent(event) {
            var id = event.stat_id,
            value = event.stat_value,
            statObject = null;

            if(!statistics.hasOwnProperty(id) && formatters.hasOwnProperty(id)) {
                if(formatters[id].type === 'time') {
                    statObject = statistics[id] = createTimeValueRecord();
                } else if (formatters[id].type === 'value') {
                    statObject = statistics[id] = createValueRecord();
                }
            } else {
                statObject = statistics[id];
            }

            if(statObject != null) {
                statObject.record(value);
            }
        }

        // ------------------------------------------------------------------

        function toHTML() {
            var buffer = createBuffer(), hasContent = false, key, formater, stat,
            min, max;

            // Extract stat data
            buffer.append("<table class='viewport-stat'>");
            buffer.append("<tr class='stat-header'><td class='label'></td><td class='value'>Current</td><td class='min'>Min</td><td class='max'>Max</td><td class='avg'>Average</td></tr>");
            for(key in statistics) {
                if(formatters.hasOwnProperty(key) && statistics[key].valid) {
                    formater = formatters[key];
                    stat = statistics[key];
                    hasContent = true;

                    // The functiion may swap the order
                    min = formater.convert(stat.min);
                    max = formater.convert(stat.max);

                    buffer.append("<tr><td class='label'>");
                    buffer.append(formater.label);
                    buffer.append("</td><td class='value'>");
                    buffer.append(formater.convert(stat.value));
                    buffer.append("</td><td class='min'>");
                    buffer.append((min < max) ? min : max);
                    buffer.append("</td><td class='max'>");
                    buffer.append((min > max) ? min : max);
                    buffer.append("</td><td class='avg'>");
                    buffer.append(formater.convert(stat.getAverageValue()));
                    buffer.append("</td></tr>");
                }
            }
            buffer.append("</table>");

            return hasContent ? buffer.toString() : "";
        }

        // ------------------------------------------------------------------

        return {
            eventHandler: handleEvent,
            toHTML: toHTML,
            reset: function() {
                statistics = {};
            }
        }
    }

    // ----------------------------------------------------------------------

    function createBuffer() {
        var idx = -1, buffer = [];
        return {
            clear: function(){
                idx = -1;
                buffer = [];
            },
            append: function(str) {
                buffer[++idx] = str;
                return this;
            },
            toString: function() {
                return buffer.join('');
            }
        };
    }

    // ----------------------------------------------------------------------

    function createTimeValueRecord() {
        var lastTime, sum, count;

        // Default values
        lastTime = 0;
        sum = 0;
        count = 0;

        return {
            value: 0.0,
            valid: false,
            min: +1000000000.0,
            max: -1000000000.0,

            record: function(v) {
                if(v === 0) {
                    this.start();
                } else if (v === 1) {
                    this.stop();
                }
            },

            start: function() {
                lastTime = new Date().getTime();
            },

            stop: function() {
                if(lastTime != 0) {
                    this.valid = true;
                    var time = new Date().getTime();
                    this.value = time - lastTime;
                    this.min = (this.min < this.value) ? this.min : this.value;
                    this.max = (this.max > this.value) ? this.max : this.value;
                    //
                    sum += this.value;
                    count++;
                }
            },

            reset: function() {
                count = 0;
                sum = 0;
                lastTime = 0;
                this.value = 0;
                this.min = +1000000000.0;
                this.max = -1000000000.0;
                this.valid = false;
            },

            getAverageValue: function() {
                if(count == 0) {
                    return 0;
                }
                return Math.floor(sum / count);
            }
        }
    }

    // ----------------------------------------------------------------------

    function createValueRecord() {
        var sum = 0, count = 0;

        return {
            value: 0.0,
            valid: false,
            min: +1000000000.0,
            max: -1000000000.0,

            record: function(v) {
                this.valid = true;
                this.value = v;
                this.min = (this.min < this.value) ? this.min : this.value;
                this.max = (this.max > this.value) ? this.max : this.value;
                //
                sum += this.value;
                count++;
            },

            reset: function() {
                count = 0;
                sum = 0;
                this.value = 0;
                this.min = +1000000000.0;
                this.max = -1000000000.0;
                this.valid = false;
            },

            getAverageValue: function() {
                if(count === 0) {
                    return 0;
                }
                return Math.floor(sum / count);
            }
        }
    }

    // ----------------------------------------------------------------------
    // Viewport container definition
    // ----------------------------------------------------------------------

    /**
     * Create a new viewport for a vtkWeb View.
     * The options are explained below.
     *
     * @member vtkWeb.Viewport
     * @param {Object} options
     * Configure the viewport to create the way we want.
     *
     *     options = {
     *        session: sessionObject,     // Object used to communicate with the remote server.
     *        view: -1,                  // -1 for active view or use the proper viewId
     *        enableInteractions: true, // True if mouse interaction should be forwarded to the server
     *        renderer: 'image'        // Type of renderer to be used. Can only be 'image' 'webgl' or 'vgl'.
     *      // --- image renderer options
     *        interactiveQuality: 30,   // StillRender quality when interacting
     *        stillQuality: 100,        // StillRender quality when not interacting
     *      // --- webgl/vgl renderer options
     *        keepServerInSynch: false. // Push camera information to server if true
     *     }
     *
     * @return {vtkWeb.Viewport}
     */
    function createViewport(options) {
        // Make sure we have a valid autobahn session
        if (options.session === null) {
            throw "'session' must be provided within the option.";
        }

        // Create viewport
        var config = $.extend({}, DEFAULT_VIEWPORT_OPTIONS, options),
        session = options.session,
        rendererContainer = $(DEFAULT_RENDERERS_CONTAINER_HTML).css(DEFAULT_RENDERERS_CONTAINER_CSS),
        mouseListener = $(DEFAULT_MOUSE_LISTENER_HTML).css(DEFAULT_MOUSE_LISTENER_CSS),
        statContainer = $(DEFAULT_STATISTIC_HTML).css(DEFAULT_STATISTIC_CSS),
        overlayContainer = $(DEFAULT_OVERLAY_HTML).css(DEFAULT_OVERLAY_CSS),
        onDoneQueue = [],
        statisticManager = createStatisticManager(),
        inSelectionMode = false,
        viewport = {
            /**
             * Update the active renderer to be something else.
             * This allow the user to switch from Image Delivery to Geometry delivery
             * or even any other available renderer type available.
             *
             * The available renderers are indexed inside the following object vtkWeb.ViewportFactory.
             *
             * @member vtkWeb.Viewport
             * @param {String} rendererName
             * Key used to ID the renderer type.
             */
            setActiveRenderer: function(rendererName) {
                $('.' + rendererName, rendererContainer).addClass('active').show().siblings().removeClass('active').hide();
                rendererContainer.trigger('active');
                statContainer[0].innerHTML = '';
                statisticManager.reset();
            },

            /**
             * Method that should be called each time something in the scene as changed
             * and we want to update the viewport to reflect the latest state of the scene.
             *
             * @member vtkWeb.Viewport
             * @param {Function} ondone Function to call after rendering is complete.
             */
            invalidateScene: function(onDone) {
                onDoneQueue.push(onDone);
                rendererContainer.trigger('invalidateScene');
            },

            /**
             * Method that should be called when nothing has changed in the scene
             * but for some reason the viewport has been dirty.
             * (i.e. Toggeling the statistic information within the viewport)
             *
             * @member vtkWeb.Viewport
             * @param {Function} ondone Function to call after rendering is complete.
             */
            render: function(onDone, args) {
                onDoneQueue.push(onDone);
                rendererContainer.trigger({type: 'render', options: args});
            },

            /**
             * Reset the camera of the scene to make it fit in the screen as well
             * as invalidating the scene automatically.
             *
             * @member vtkWeb.Viewport
             * @param {Function} ondone Function to call after rendering is complete.
             */
            resetCamera: function(onDone) {
                onDoneQueue.push(onDone);
                return session.call("vtk:resetCamera", Number(config.view)).then(function () {
                    rendererContainer.trigger('invalidateScene');
                });
            },

            /**
             * Update Orientation Axes Visibility for the given view
             *
             * @member vtkWeb.Viewport
             * @param {Boolean} show
             * Show: true / Hide: false
             * @param {Function} ondone Function to call after rendering is complete.
             */
            updateOrientationAxesVisibility: function (show, onDone) {
                return session.call("vtk:updateOrientationAxesVisibility", Number(config.view), show).then(function () {
                    onDoneQueue.push(onDone);
                    rendererContainer.trigger('invalidateScene');
                });
            },

            /**
             * Update the Center Axes Visibility for the given view
             *
             * @member vtkWeb.Viewport
             * @param {Boolean} show
             * Show: true / Hide: false
             * @param {Function} ondone Function to call after rendering is complete.
             */
            updateCenterAxesVisibility: function (show, onDone) {
                return session.call("vtk:updateCenterAxesVisibility", Number(config.view), show).then(function () {
                    onDoneQueue.push(onDone);
                    rendererContainer.trigger('invalidateScene');
                });
            },

            /**
             * Reset view id.
             * This allow to invalidate the viewport and use the new active view
             *
             * @member vtkWeb.Viewport
             */
            resetViewId: function () {
                rendererContainer.trigger('resetViewId');
            },

            /**
             * Attach viewport to a DOM element
             *
             * @member vtkWeb.Viewport
             * @param {String} selector
             * The will be used internally to get the jQuery associated element
             *
             *     <div class="renderer"></div>
             *     viewport.bind(".renderer");
             *
             *     <div id="renderer"></div>
             *     viewport.bind("#renderer");
             *
             *     <html>
             *       <body>
             *         <!-- renderer -->
             *         <div></div>
             *       </body>
             *     </html>
             *     viewport.bind("body > div");
             */
            bind: function (selector) {
                var container = $(selector);
                if (container.attr("__vtkWeb_viewport__") !== "true") {
                    container.attr("__vtkWeb_viewport__", "true");
                    container.append(rendererContainer).append(mouseListener).append(statContainer).append(overlayContainer);
                    rendererContainer.trigger('invalidateScene');
                }
            },

            /**
             * Remove viewport from DOM element
             *
             * @member vtkWeb.Viewport
             */
            unbind: function () {
                var parentElement = rendererContainer.parent();
                if (parentElement) {
                    parentElement.attr("__vtkWeb_viewport__", "false");
                    rendererContainer.remove();
                    mouseListener.remove();
                    statContainer.remove();
                    overlayContainer.remove();
                }
            },

            /**
             * Update statistic visibility
             *
             * @member vtkWeb.Viewport
             * @param {Boolean} visible
             */
            showStatistics: function(isVisible) {
                if(isVisible) {
                    statContainer.show();
                } else {
                    statContainer.hide();
                }
            },

            /**
             * Clear current statistic values
             *
             * @member vtkWeb.Viewport
             */
            resetStatistics: function() {
                statisticManager.reset();
                statContainer.empty();
            },

            /**
             * Event triggered before a mouse down.
             *
             * @member vtkWeb.Viewport
             * @event startInteraction
             */
            /**
             * Event triggered after a mouse up.
             *
             * @member vtkWeb.Viewport
             * @event endInteraction
             * @param area
             * Provide the area in pixel between the startInteraction and endInteraction.
             * This can be used for selection or zoom to box... [minX, minY, maxX, maxY]
             */

            /**
             * Toggle selection mode.
             *
             * When selection mode is active, the next interaction will
             * create a 2D rectangle that will be used to define an area to select.
             * This won't perform any selection but will provide another interaction
             * mode on the client side which will then trigger an event with
             * the given area. It is the responsibility of the user to properly
             * handle that event and eventually trigger the appropriate server
             * code to perform a selection or a zoom-to-box type of action.
             *
             * @member vtkWeb.Viewport
             * @return {Boolean} inSelectionMode state
             */
            toggleSelectionMode: function() {
                inSelectionMode = !inSelectionMode;
                return inSelectionMode;
            },

            /**
             * Return whether or not viewport is in selection mode.
             *
             * @return {Boolean} true if viewport is in selection mode, false otherwise.
             */
            getSelectionMode: function() {
                return inSelectionMode;
            }
        };

        // Attach config object to renderer parent
        rendererContainer.data('config', config);

        // Attach onDone listener
        rendererContainer.bind('done', function(){
            while(onDoneQueue.length > 0) {
                var callback = onDoneQueue.pop();
                try {
                    if(callback) {
                        callback();
                    }
                } catch(error) {
                    console.log("On Done callback error:");
                    console.log(error);
                }
            }
        });

        // Create any renderer type that is available
        for(var key in vtkWeb.ViewportFactory) {
            try {
                vtkWeb.ViewportFactory[key].builder(rendererContainer);
            } catch(error) {
                console.log("Error while trying to load renderer: " + key);
                console.log(error);
            }
        }

        // Set default renderer
        viewport.setActiveRenderer(config.renderer);

        // Attach mouse listener if requested
        if (config.enableInteractions) {
            attachMouseListener(mouseListener, rendererContainer, overlayContainer, viewport);
            try {
                attachTouchListener(mouseListener, rendererContainer, viewport);
            } catch(error) {
                console.log('Hammer is not properly initialized');
                console.log(error);
            }
        }

        // Attach stat listener
        rendererContainer.bind('stats', function(event){
            statisticManager.eventHandler(event);
            statContainer[0].innerHTML = statisticManager.toHTML();
        });

        return viewport;
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
    // Export internal methods to the vtkWeb module
    // ----------------------------------------------------------------------
    module.createViewport = function (option) {
        return createViewport(option);
    };

    // ----------------------------------------------------------------------
    // Local module registration
    // ----------------------------------------------------------------------
    try {
      // Tests for presence of jQuery, then registers this module
      if ($ !== undefined) {
        module.registerModule('vtkweb-viewport');
      } else {
        console.error('Module failed to register, jQuery is missing');
      }
    } catch(err) {
      console.error('Caught exception while registering module: ' + err.message);
    }

}(window, jQuery));
