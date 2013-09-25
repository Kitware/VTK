/**
 * vtkWeb JavaScript Library.
 *
 * This module extend the vtkWeb viewport to add support for Image delivery
 * mechanism for rendering.
 *
 * @class vtkWeb.viewport.image
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
        /**
         * @member vtkWeb.ViewPortConfig
         * @property {Number} interactiveQuality
         * Compression quality that should be used to encode the image on the
         * server side while interacting.
         *
         * Default: 30
         */
        interactiveQuality: 30,
        /**
         * @member vtkWeb.ViewPortConfig
         * @property {Number} stillQuality
         * Compression quality that should be used to encode the image on the
         * server side when we stoped interacting.
         *
         * Default: 100
         */
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
                /**
                 * @class request.Render
                 * Container Object that provide all the input needed to request
                 * a rendering from the server side.
                 *
                 *      {
                 *        size: [width, height], // Size of the image to generate
                 *        view: 234523,          // View proxy globalId
                 *        mtime: 23423456,       // Last Modified time image received
                 *        quality: 100,          // Image quality expected
                 *        localtime: 3563456     // Local time at sending for round trip computation statistic
                 *      }
                 */
                var renderCfg = {
                    /**
                     * @member request.Render
                     * @property {Array} size
                     * Size of the Viewport for which the image should be render
                     * for. [width, height] in pixel.
                     */
                    size: [ container.innerWidth(), container.innerHeight() ],
                    /**
                     * @member request.Render
                     * @property {Number} view
                     * GlobalID of the view Proxy.
                     */
                    view: Number(options.view),
                    /**
                     * @member request.Render
                     * @property {Number} MTime
                     * Last received image MTime.
                     */
                    mtime: fetch ? 0 : lastMTime,
                    /**
                     * @member request.Render
                     * @property {Number} quality
                     * Image compression quality.
                     * -   0: Looks Bad but small in size.
                     * - 100: Looks good bug big in size.
                     */
                    quality: quality,
                    /**
                     * @property {Number} localTime
                     * Local client time used to compute the round trip time cost.
                     * Equals to new Date().getTime().
                     */
                    localTime : new Date().getTime()
                };

                container.trigger({
                    type: 'stats',
                    stat_id: 'image-fps',
                    stat_value: 0 // start
                });

                session.call("vtk:stillRender", renderCfg).then(function (res) {
                    /**
                     * @class reply.Render
                     * Object returned from the server as a response to a
                     * stillRender request. It includes information about the
                     * rendered image along with the rendered image itself.
                     *
                     *    {
                     *       image     : "sdfgsdfg/==",      // Image encoding in a String
                     *       size      : [width, height],    // Image size
                     *       format    : "jpeg;base64",      // Image type + encoding
                     *       global_id : 234652436,          // View Proxy ID
                     *       stale     : false,              // Image is stale
                     *       mtime     : 23456345,           // Image MTime
                     *       localTime : 3563456,            // Value provided at request
                     *       workTime  : 10                  // Number of ms that were needed for the processing
                     *    }
                     */
                    /**
                     * @member reply.Render
                     * @property {String} image
                     * Rendered image content encoded as a String.
                     */
                    /**
                     * @member reply.Render
                     * @property {Array} size
                     * Size of the rendered image (width, height).
                     */
                    /**
                     * @member reply.Render
                     * @property {String} format
                     * String indicating the format and encoding for the image
                     * e.g. "jpeg;base64" or "png;base64".
                     */
                    /**
                     * @member reply.Render
                     * @property {Number} global_id
                     * GlobalID of the view proxy from which the image is
                     * obtained.
                     */
                    /**
                     * @member reply.Render
                     * @property {Boolean} stale
                     * For better frame-rates when interacting, vtkWeb may
                     * return a stale rendered image, while the newly rendered
                     * image is being processed. This flag indicates that a new
                     * rendering for this view is currently being processed on
                     * the server.
                     */
                    /**
                     * @member reply.Render
                     * @property {Number} mtime
                     * Timestamp of the generated image. This is used to prevent
                     * a redelivery of the same image.
                     */
                    /**
                     * @member reply.Render
                     * @property {Number} localTime
                     * Unchanged value that was in the request. This will help
                     * to compute round trip cost.
                     */
                    /**
                     * @member reply.Render
                     * @property {Number} workTime
                     * Delta time that was needed on the server side to handle
                     * the request. This does not include the json parsing.
                     * Just the high level opeartion achieved by vtkWeb.
                     */
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

                /**
                 * @class request.InteractionEvent
                 * Container Object used to encapsulate MouseEvent status
                 * formated in an handy manner for vtkWeb.
                 *
                 *     {
                 *       view         : 23452345, // View proxy globalId
                 *       action       : "down",   // Enum["down", "up", "move"]
                 *       charCode     : "",       // In key press will hold the char value
                 *       altKey       : false,    // Is alt Key down ?
                 *       ctrlKey      : false,    // Is ctrl Key down ?
                 *       shiftKey     : false,    // Is shift Key down ?
                 *       metaKey      : false,    // Is meta Key down ?
                 *       buttonLeft   : false,    // Is button Left down ?
                 *       buttonMiddle : false,    // Is button Middle down ?
                 *       buttonRight  : false,    // Is button Right down ?
                 *     }
                 */
                var vtkWeb_event = {
                    /**
                     * @member request.InteractionEvent
                     * @property {Number}  view Proxy global ID
                     */
                    view: Number(options.view),
                    /**
                     * @member request.InteractionEvent
                     * @property {String}  action
                     * Type of mouse action and can only be one of:
                     *
                     * - down
                     * - up
                     * - move
                     * - dblclick
                     * - scroll
                     */
                    action: evt.action,
                    /**
                     * @member request.InteractionEvent
                     * @property {String}  charCode
                     */
                    charCode: evt.charCode,
                    /**
                     * @member request.InteractionEvent
                     * @property {Boolean} altKey
                     */
                    altKey: evt.altKey,
                    /**
                     * @member request.InteractionEvent
                     * @property {Boolean} ctrlKey
                     */
                    ctrlKey: evt.ctrlKey,
                    /**
                     * @member request.InteractionEvent
                     * @property {Boolean} shiftKey
                     */
                    shiftKey: evt.shiftKey,
                    /**
                     * @member request.InteractionEvent
                     * @property {Boolean} metaKey
                     */
                    metaKey: evt.metaKey,
                    /**
                     * @member request.InteractionEvent
                     * @property {Boolean} buttonLeft
                     */
                    buttonLeft: (evt.current_button === 1 ? true : false),
                    /**
                     * @member request.InteractionEvent
                     * @property {Boolean} buttonMiddle
                     */
                    buttonMiddle: (evt.current_button === 2 ? true : false),
                    /**
                     * @member request.InteractionEvent
                     * @property {Boolean} buttonRight
                     */
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
                session.call("vtk:mouseInteraction", vtkWeb_event).then(function (res) {
                    if (res) {
                        action_pending = false;
                        render();
                    }
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

}(window, jQuery));
