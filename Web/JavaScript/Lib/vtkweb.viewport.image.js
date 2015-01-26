/**
 * vtkWeb JavaScript Library.
 *
 * This module extend the vtkWeb viewport to add support for Image delivery
 * mechanism for rendering.
 *
 * @class vtkWeb.viewports.image
 *
 *     Viewport Factory description:
 *       - Key: image
 *       - Stats:
 *         - image-fps
 *         - image-round-trip
 *         - image-server-processing
 */
(function (GLOBAL, $) {
    var module = {},
    RENDERER_CSS = {
        "position": "absolute",
        "top": "0px",
        "left": "0px",
        "right": "0px",
        "bottom": "0px",
        "z-index" : "0"
    },
    DEFAULT_OPTIONS = {
        interactiveQuality: 30,
        stillQuality: 100
    },
    FACTORY_KEY = 'image',
    FACTORY = {
        'builder': createImageDeliveryRenderer,
        'options': DEFAULT_OPTIONS,
        'stats': {
            'image-fps': {
                label: 'Framerate:',
                type: 'time',
                convert: function(value) {
                    if(value === 0) {
                        return 0;
                    }
                    return Math.floor(1000 / value);
                }
            },
            'image-round-trip': {
                label: 'Round&nbsp;trip&nbsp;(ms):',
                type: 'value',
                convert: NoOp
            },
            'image-server-processing': {
                label: 'Processing&nbsp;Time&nbsp;(ms):',
                type: 'value',
                convert: NoOp
            }
        }
    };

    // ----------------------------------------------------------------------

    function NoOp(a) {
        return a;
    }

    // ----------------------------------------------------------------------
    // Image Delivery renderer - factory method
    // ----------------------------------------------------------------------

    function createImageDeliveryRenderer(domElement) {
        var container = $(domElement),
        options = $.extend({}, DEFAULT_OPTIONS, container.data('config')),
        bgImage = new Image(),
        session = options.session,
        canvas = GLOBAL.document.createElement('canvas'),
        ctx2d = canvas.getContext('2d'),
        renderer = $(canvas).addClass(FACTORY_KEY).css(RENDERER_CSS).append(bgImage),
        force_render = false,
        statistics = null,
        lastMTime = 0,
        render_onidle_timeout = null,
        action_pending = false,
        button_state = {
            left : false,
            right: false,
            middle : false
        },
        quality = 100;

        // ----
        /// Internal method that returns true if the mouse interaction event should be
        /// throttled.
        function eatMouseEvent(event) {
            var force_event = (button_state.left !== event.buttonLeft || button_state.right  !== event.buttonRight || button_state.middle !== event.buttonMiddle);
            if (!force_event && !event.buttonLeft && !event.buttonRight && !event.buttonMiddle && !event.scroll) {
                return true;
            }
            if (!force_event && action_pending) {
                return true;
            }
            button_state.left   = event.buttonLeft;
            button_state.right  = event.buttonRight;
            button_state.middle = event.buttonMiddle;
            return false;
        }

        //-----
        // Internal function that requests a render on idle. Calling this
        // mutliple times will only result in the request being set once.
        function renderOnIdle() {
            if (render_onidle_timeout === null) {
                render_onidle_timeout = GLOBAL.setTimeout(render, 250);
            }
        }

        // Setup internal API
        function render(fetch) {
            if (force_render === false) {
                if (render_onidle_timeout !== null) {
                    // clear any renderOnIdle requests that are pending since we
                    // are sending a render request.
                    GLOBAL.clearTimeout(render_onidle_timeout);
                    render_onidle_timeout = null;
                }
                force_render = true;

                var renderCfg = {
                    size: [ container.innerWidth(), container.innerHeight() ],
                    view: Number(options.view),
                    mtime: fetch ? 0 : lastMTime,
                    quality: quality,
                    localTime : new Date().getTime()
                };

                container.trigger({
                    type: 'stats',
                    stat_id: 'image-fps',
                    stat_value: 0 // start
                });

                session.call("viewport.image.render", [renderCfg]).then(function (res) {
                    options.view = Number(res.global_id);
                    lastMTime = res.mtime;
                    if(res.hasOwnProperty("image") && res.image !== null) {
                        /**
                         * @member vtkWeb.Viewport
                         * @event start-loading
                         */

                        $(container).parent().trigger("start-loading");
                        bgImage.width  = res.size[0];
                        bgImage.height = res.size[1];
                        var previousSrc = bgImage.src;
                        bgImage.src = "data:image/" + res.format  + "," + res.image;

                        container.trigger({
                            type: 'stats',
                            stat_id: 'image-fps',
                            stat_value: 1 // stop
                        });

                        container.trigger({
                            type: 'stats',
                            stat_id: 'image-round-trip',
                            stat_value: Number(new Date().getTime() - res.localTime) - res.workTime
                        });

                        container.trigger({
                            type: 'stats',
                            stat_id: 'image-server-processing',
                            stat_value: Number(res.workTime)
                        });
                    }
                    renderStatistics();
                    force_render = false;
                    container.trigger('done');

                    // the image we received is not the latest, we should
                    // request another render to try to get the latest image.
                    if (res.stale === true) {
                        renderOnIdle();
                    }
                });
            }
        }

        // internal function to render stats.
        function renderStatistics() {
            if (statistics) {
                ctx2d.font = "bold 12px sans-serif";
                //ctx2d.fillStyle = "white";
                ctx2d.fillStyle = "black";
                ctx2d.fillRect(10, 10, 240, 100);
                //ctx2d.fillStyle = "black";
                ctx2d.fillStyle = "white";
                ctx2d.fillText("Frame Rate: " + statistics.frameRate().toFixed(2), 15, 25);
                ctx2d.fillText("Average Frame Rate: " + statistics.averageFrameRate().toFixed(2),
                    15, 40);
                ctx2d.fillText("Round trip: " + statistics.roundTrip() + " ms - Max: " + statistics.maxRoundTrip() + " ms",
                    15, 55);
                ctx2d.fillText("Server work time: " + statistics.serverWorkTime() + " ms - Max: " + statistics.maxServerWorkTime() + " ms",
                    15, 70);
                ctx2d.fillText("Minimum Frame Rate: " + statistics.minFrameRate().toFixed(2),
                    15, 85);
                ctx2d.fillText("Loading time: " + statistics.trueLoadTime(),
                    15, 100);
            }
        }

        // Choose if rendering is happening in Canvas or image
        bgImage.onload = function(){
            paint();
        };

        // internal function used to draw update data on the canvas. When not
        // using canvas, this has no effect.
        function paint() {
            /**
             * @member vtkWeb.Viewport
             * @event stop-loading
             */
            $(container).parent().trigger("stop-loading");
            ctx2d.canvas.width = $(container).width();
            ctx2d.canvas.height = $(container).height();
            ctx2d.drawImage(bgImage, 0, 0, bgImage.width, bgImage.height);
            renderStatistics();
        }

        // Attach listener to container for mouse interaction and invalidateScene
        container.bind('invalidateScene', function() {
            if(renderer.hasClass('active')){
                render(true);
            }
        }).bind('resetViewId', function(e){
            options.view = -1;
        }).bind('captureRenderedImage', function(e){
            if(renderer.hasClass('active')){
                $(container).parent().trigger({
                    type: 'captured-screenshot-ready',
                    imageData: bgImage.src
                });
            }
        }).bind('render', function(e){
            if(renderer.hasClass('active')){
                var opts = e.options,
                previousQuality = quality,
                forceRender = false;

                if(opts) {
                    quality = opts.hasOwnProperty('quality') ? opts.quality : quality;
                    options.view = opts.hasOwnProperty('view') ? opts.view : options.view;
                    forceRender = opts.hasOwnProperty('forceRender');
                }

                render(forceRender);

                // Revert back to previous state
                quality = previousQuality;
            }
        }).bind('mouse', function(evt){
            if(renderer.hasClass('active')){
                // stop default event handling by the browser.
                evt.preventDefault();

                // Update quality based on the type of the event
                if(evt.action === 'up' || evt.action === 'dblclick' || evt.action === 'scroll') {
                    quality = options.stillQuality;
                } else {
                    quality = options.interactiveQuality;
                }

                var vtkWeb_event = {
                    view: Number(options.view),
                    action: evt.action,
                    charCode: evt.charCode,
                    altKey: evt.altKey,
                    ctrlKey: evt.ctrlKey,
                    shiftKey: evt.shiftKey,
                    metaKey: evt.metaKey,
                    buttonLeft: (evt.current_button === 1 ? true : false),
                    buttonMiddle: (evt.current_button === 2 ? true : false),
                    buttonRight: (evt.current_button === 3 ? true : false)
                },
                elem_position = $(evt.delegateTarget).offset(),
                pointer = {
                    x : (evt.pageX - elem_position.left),
                    y : (evt.pageY - elem_position.top)
                };

                if(evt.action === 'scroll') {
                    vtkWeb_event.scroll = evt.scroll;
                } else {
                    vtkWeb_event.x = pointer.x / renderer.width();
                    vtkWeb_event.y = 1.0 - (pointer.y / renderer.height());
                }

                if (eatMouseEvent(vtkWeb_event)) {
                    return;
                }

                action_pending = true;
                session.call("viewport.mouse.interaction", [vtkWeb_event]).then(function (res) {
                    if (res) {
                        action_pending = false;
                        render();
                    }
                }, function(error) {
                    console.log("Call to viewport.mouse.interaction failed");
                    console.log(error);
                });
            }
        }).append(renderer);
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
    // Extend the viewport factory
    // ----------------------------------------------------------------------
    if(!module.hasOwnProperty('ViewportFactory')) {
        module['ViewportFactory'] = {};
    }
    module.ViewportFactory[FACTORY_KEY] = FACTORY;

    // ----------------------------------------------------------------------
    // Local module registration
    // ----------------------------------------------------------------------
    try {
      // Tests for presence of jQuery, then registers this module
      if ($ !== undefined) {
        module.registerModule('vtkweb-viewport-image');
      }
    } catch(err) {
      console.error('jQuery is missing: ' + err.message);
    }

}(window, jQuery));
