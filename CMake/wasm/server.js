// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause\
//
// Usage: node server.js -d,--directory /path/to/Testing/Temporary -p,--port N -o,--operation {start,run,stop}
//
// # Operations (-o/--operation)
//
// In the "start" operation, we fork a new 'detached' node process that launches
// this script with "-o run" argument. We wait for the child process to
// send a "listening" message informing us that the server is now listening for connections,
// and a "vtkhttp.lock" file has been created. We exit upon receiving that information.
//
// In the "run" operation, we create a HTTP server and
// write details of the HTTP Server and our PID into a "vtkhttp.lock" file.
// We send a message "listening" via IPC to the parent process so that it can
// safely exit.
//
// The "stop" operation kills the server process by reading the pid from the "vtkhttp.lock" file.
// It subsequently removes the lock file so that future server starts succeed.

import http from 'node:http';
import { URL } from 'node:url';
import fs from 'node:fs';
import { parseArgs } from 'node:util';
import process from 'node:process';
import path from 'node:path';
import { fork } from 'node:child_process';

const OPERATIONS =
{
  START: 'start',
  RUN: 'run',
  STOP: 'stop',
};

const {
  values: { directory, binaryDirectory, port, operation },
} = parseArgs({
  options: {
    directory: {
      type: 'string',
      short: 'd',
      default: process.cwd()
    },
    binaryDirectory: {
      type: 'string',
      short: 'b',
      default: process.cwd()
    },
    port: {
      type: 'string',
      short: 'p',
      default: '8000'
    },
    operation: {
      type: 'string',
      short: 'o',
    },
  },
});

const HOST = '127.0.0.1';
const PORT = Number.parseInt(port, 10);
const OPERATION = operation;
const LOCK = path.join(directory, 'vtkhttp.lock');
const HTML_REGEX = new RegExp('.html$');
const JS_REGEX = new RegExp('.js$');

if (OPERATION == OPERATIONS.START) {
  console.log('starting server process..');
  if (fs.existsSync(LOCK)) {
    console.error(`lock file ${LOCK} exists. Run with -o stop to kill existing HTTP server and try again.`)
    process.exit(1);
  }
  fork(process.argv[1],
    ['-d', directory, '-b', binaryDirectory, '-p', port, '-o', OPERATIONS.RUN],
    {
      detached: true,
      stdio: [
        null,
        fs.openSync(path.join(directory, 'server.out'), 'w'),
        fs.openSync(path.join(directory, 'server.err'), 'w'),
        'ipc',
      ]
    })
    .on('message', (msg) => {
      console.log('received message ', msg);
      if (msg == 'listening') {
        process.exit(0);
      }
    })
    .on('spawn', () => {
      console.log('server process spawned.');
    });
} else if (OPERATION == OPERATIONS.STOP) {
  console.log('stopping server process..');
  const vtkhttp = JSON.parse(fs.readFileSync(LOCK));
  try
  {
    process.kill(vtkhttp.pid);
    console.log(`killed ${vtkhttp.pid}`);
  } catch (e)
  {
    console.warn(`failed to kill process ${vtkhttp.pid}: ${e.message}`);
  }
  fs.rmSync(LOCK, { force: true, maxRetries: 10 });
} else if (OPERATION == OPERATIONS.RUN) {
  // Create a local server to receive data from
  const server = http.createServer((incomingMesssage, response) => {
    const headers = {
      'Access-Control-Allow-Origin': '*', /* @dev First, read about security */
      'Access-Control-Allow-Methods': 'OPTIONS, POST, GET',
      'Access-Control-Max-Age': 2592000, // 30 days
      'Cross-Origin-Opener-Policy': 'same-origin',
      'Cross-Origin-Embedder-Policy': 'require-corp',
      /** add other headers as per requirement */
    };
    const url = new URL(incomingMesssage.url, `http://${incomingMesssage.headers.host}/`);
    console.debug(`${incomingMesssage.method} ${url.toString()}`);
    if (
      url.pathname === '/dump' &&
      incomingMesssage.method === 'POST'
    ) {
      const query = new URLSearchParams(url.search);
      if (query.has('file')) {
        incomingMesssage.pipe(fs.createWriteStream(query.get('file'))
          .on('finish', () => {
            const body = Buffer.from('OK');
            response
              .writeHead(200, {
                ...headers,
                'Content-Length': Buffer.byteLength(body),
                'Content-Type': 'text/plain'
              })
              .end(body);
          })
          .on('error', (err) => {
            const body = Buffer.from(`Internal server error ${err.name}, ${err.message}`);
            response
              .writeHead(500, {
                ...headers,
                'Content-Length': Buffer.byteLength(body),
                'Content-Type': 'text/plain'
              })
              .end(body);
          }));
      } else {
        const body = Buffer.from('Invalid query for /dump, expects /dump?file="/path/to/filename.ext"');
        response
          .writeHead(400, {
            ...headers,
            'Content-Length': Buffer.byteLength(body),
            'Content-Type': 'text/plain',
          })
          .end(body);
      }
    } else if (
      incomingMesssage.method === 'GET'
    ) {
      if (url.pathname.includes('/preload')) {
        const query = new URLSearchParams(url.search);
        if (query.has('file')) {
          const filePath = path.join(query.get('file'));
          console.debug(`preloading ${filePath}`);
          if (fs.existsSync(filePath)) {
            const fileStream = fs.createReadStream(filePath);
            response.writeHead(200, {
              ...headers,
              'Content-Type': 'application/octet-stream',
            });
            fileStream.pipe(response);
          }
          else {
            console.error(`File not found: ${filePath}`);
            response.writeHead(404, 'File not found').end();
          }
        } else {
          response.writeHead(400, 'Bad Request').end();
        }
      }
      else if (HTML_REGEX.test(url.pathname)) {
        const filePath = path.join(directory, url.pathname);
        console.debug(`serving ${filePath}`);
        if (fs.existsSync(filePath)) {
          const fileStream = fs.createReadStream(filePath);
          response.writeHead(200, {
            ...headers,
            'Content-Type': 'text/html',
          });
          fileStream.pipe(response);
        } else {
          console.error(`File not found: ${filePath}`);
          response.writeHead(404, 'File not found').end();
        }
      }
      else if (JS_REGEX.test(url.pathname)) {
        const fileName = path.basename(url.pathname);
        const filePath = path.join(binaryDirectory, fileName);
        console.debug(`serving ${filePath}`);
        if (fs.existsSync(filePath)) {
          const fileStream = fs.createReadStream(filePath);
          const contentType = 'application/javascript';
          response.writeHead(200, {
            ...headers,
            'Content-Type': contentType,
          });
          fileStream.pipe(response);
        } else {
          response.writeHead(404, 'File not found').end();
        }
      }
      else if (url.pathname === '/favicon.ico') {
        response.writeHead(200, {...headers, 'Content-Type': 'image/png'}).end("");
      }
    }
    if (!response.writableEnded) {
      // response.writeHead(403, 'Forbidden').end();
    }
  });

  server.listen(PORT, HOST)
    .on('connection', () => {
      console.debug('someone connected');
    })
    .on('error', (err) => {
      if (err.code === 'EADDRINUSE') {
        console.error('address in use, retrying...');
        setTimeout(() => {
          server.close();
          server.listen(PORT, HOST);
        }, 1000);
      }
    })
    .on('listening', () => {
      console.debug('socket listening');
      fs.writeFileSync(
        LOCK,
        JSON.stringify({
          ...server.address(),
          pid: process.pid,
        }));
      process.send('listening');
    });
} else {
  console.error(`unknown value for -o/--operation '${OPERATION}'`)
}
