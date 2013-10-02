/*
  The following is a rough outline of a first round api for
  controlling the composited daemon.  Node is used for the
  ability to rapidly prototype new features and experiement
  with even more hairbrained ideas.
*/

var compositor = require('composited');
var net = require('net');

var Display = function() {};
Display.prototype.modes = null; // populated on instantiation

// display modes
compositor.display.on('add', function(device) {

  device.modes[0].activate();
  
  if (compositor.display.devices.length > 1) {
    if (config.mirrored) {
      compositor.mirror(compositor.display.getPrimary(), ); 
    }
  }
});

// Window Tree
// TODO: I'm really tempted to put all of this in js.  It is not _too_
//       much work, and can be fairly minimal.  It also allows the
//       surface management section below to be possible.
// 
//       there is the potential for a full on scene graph here, which
//       should probably be avoided... maybe.
//
//       Another benefit is we don't need to cross the boundary just to
//       move a window.  That can be batched.
var Surface = function() {}

// Surface Management
// TODO: it would be interesting to be able to provide custom
//       geometry on which window surfaces could be placed.
//       similar to compiz-fusion, but with more control from JS

// Request looks like:
// { width  : 1024,
//   height : 1024,
//   depth  : 32
// }
compositor.requestSurface = function(request, fn) {

  // TODO: validation
  var surface = compositor.createSurface(request);
  if (!surface) {
    fn(new Error(compositor.getError());
  }
});

compositor.resizeSurface = function(id, width, height, fn) {
  var surface = compositor.surfaceById(id);

  if (surface) {
    surface.resize(width, height);
    fn(null); 
  } else {
    fn(new Error('surface does not exist')); 
  }
});

// Client Stuff
var stream = require('stream');
var createProtocolHandler = function(conn) {
  var ret = new stream.Transform();
  ret._transform = function(buf, encoding, done) {
    // parse out the request

    var obj = parse(buf);

    // respond using `this.push(<response>)`
    done();
  }
}

net.createServer(function(conn) {
  // handle requests and force response
  // TODO: figure out the transport. JSON or msgpack
  //       are contenders.
  conn.pipe(createProtocolHandler()).pipe(conn);

}).listen('127.0.0.1', 9999);

