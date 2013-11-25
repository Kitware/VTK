/**
 * vtkWeb JavaScript Library.
 *
 * This module allow the Web client to start a remote vtkWeb session and
 * retreive all the connection information needed to properly connect to that
 * newly created session.
 *
 * @class vtkWeb.launcher
 *
 * {@img paraview/ParaViewWeb-multiuser.png alt Focus on the communication between the client and the front-end that manage the vtkWeb processes}
 */
(function (GLOBAL, $) {

    // Internal field used to store all connection objects
    var Connections = [], module = {}, console = GLOBAL.console;

    function generateSecretKey() {
        var text = "";
        var possible = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

        for( var i=0; i < 10; i++ )
            text += possible.charAt(Math.floor(Math.random() * possible.length));

        return text;
    }

    /**
     * @class vtkWeb.Connection
     * This class provides all the informations needed to connect to the session
     * manager web service.
     */
    /**
     * @member vtkWeb.Connection
     * @property {String} sessionManagerURL
     * The service URL that will respond to the REST request to start or stop
     * a visualization session.
     *
     * MANDATORY
     */
    /**
     * @member vtkWeb.Connection
     * @property {String} name
     * The name given for the visualization.
     *
     * RECOMMENDED/OPTIONAL
     */
    /**
     * @member vtkWeb.Connection
     * @property {String} application
     * The name of the application that should be started on the server side.
     *
     * MANDATORY
     */
    /**
     * @member vtkWeb.Connection
     * @property {String|Number} __Any_Name__
     * Any property that we want to provide to the session that will be created.
     * Such property is not necessary used by the session manager but will be
     * returned if a connection information is requested from a session.
     *
     * OPTIONAL
     */
    /**
     * @member vtkWeb.Connection
     * @property {String} secret
     * Password that should be used to protect remote session access.
     *
     * This property is used by the launcher to secure the process that it start
     * but it is also used by the client to authenticate itself against
     * the remote process.
     *
     * This can be provided by the client or by the server depending who
     * generate it. In both case, the client will use it for its authentication.
     * If missing, then the client will use the default secret key.
     *
     * OPTIONAL
     */
    /**
     * @member vtkWeb.Connection
     * @property {Number} generate-secret
     * Property used to specify where the generation of the secret key should be
     * made.
     * 0: We use the default secret key. (No dynamic one)
     * 1: The JavaScript client generate the key and its the responsability of
     *    the server to provide the generated key to the vtkWeb process.
     * 2: The launcher process generate that key when it start the vtkWeb
     *    process. That given secret key must be returned to the client within
     *    the connection object.
     *
     * OPTIONAL
     */

    //=========================================================================

    /**
     * @member vtkWeb.Connection
     * @property {String} sessionURL
     * The websocket URL that should be used to connect to the running
     * visualization session.
     * This field is provided within the response.
     */
    /**
     * @member vtkWeb.Connection
     * @property {String} id
     * The session identifier.
     * This field is provided within the response.
     */
    /**
     * @member vtkWeb.Connection
     * @property {vtkWeb.Session} session
     * The session object will be automatically added to the connection once the
     * connection is properly established by calling:
     *
     *     vtkWeb.connect(connection, success, error);
     *
     * This field is provided within the response.
     */
    //=========================================================================

    /**
     * Start a new vtkWeb process on the server side.
     * This method will make a JSON POST request to config.sessionManagerURL URL.
     *
     * @member vtkWeb.launcher
     *
     * @param {vtkWeb.ConnectionConfig} config
     * Session creation parameters. (sessionManagerURL, name, application).
     *
     * @param {Function} successCallback
     * The function will be called once the connection is successfully performed.
     * The argument of the callback will be a {@link vtkWeb.Connection}.
     *
     * @param {Function} errorCallback
     * The function will be called if anything bad happened and an explanation
     * message will be provided as argument.
     */
    function start(config, successFunction, errorFunction) {
        if(!config.hasOwnProperty("secret") && config.hasOwnProperty("generate-secret") && config["generate-secret"] === 1) {
            config.secret = generateSecretKey();
        }
        var okCallback = successFunction,
        koCallback = errorFunction,
        arg = {
            url: config.sessionManagerURL,
            type: "POST",
            dataType: "json",
            data: (JSON.stringify(config)),
            success: function (reply) {
                Connections.push(reply);
                if (okCallback) {
                    okCallback(reply);
                }
            },
            error: function (errMsg) {
                if (koCallback) {
                    koCallback(errMsg);
                }
            }
        };
        return $.ajax(arg);
    }


    /**
     * Query the Session Manager in order to retreive connection informations
     * based on a session id.
     *
     * @member vtkWeb.launcher
     *
     * @param {String} sessionManagerURL
     * Same as ConnectionConfig.sessionManagerURL value.
     *
     * @param {String} sessionId
     * The unique identifier of a session.
     *
     * @return {vtkWeb.Connection} if the session is found.
     */
    function fetchConnection(sessionManagerURL, sessionId) {
        var config = {
            url: sessionManagerURL + '/' + sessionId,
            dataType: "json"
        };
        return $.ajax(config);
    }

    /**
     * Stop a remote running visualization session.
     *
     * @member vtkWeb.launcher
     *
     * @param {vtkWeb.ConnectionConfig} connection
     */
    function stop(connection) {
        var config = {
            url: connection.sessionManagerURL + "/" + connection.id,
            type: "DELETE",
            dataType: "json",
            success: function (reply) {
                console.log(reply);
            },
            error: function (errMsg) {
                console.log("Error while trying to close service");
            }
        };
        return $.ajax(config);
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
    module.start = function (config, successFunction, errorFunction) {
        return start(config, successFunction, errorFunction);
    };
    module.stop = function (connection) {
        return stop(connection);
    };
    module.fetchConnection = function (serviceUrl, sessionId) {
        return fetchConnection(serviceUrl, sessionId);
    };
    /**
     * Return all the session connections created in that JavaScript context.
     * @member vtkWeb.launcher
     * @return {vtkWeb.Connection[]}
     */
    module.getConnections = function () {
        return Connections;
    };

    // ----------------------------------------------------------------------
    // Local module registration
    // ----------------------------------------------------------------------
    try {
      // Tests for presence of jQuery, then registers this module
      if ($ !== undefined) {
        module.registerModule('vtkweb-launcher');
      }
    } catch(err) {
      console.error('jQuery is missing: ' + err.message);
    }

}(window, jQuery));
