# kmstools

some ramblings / hackery around drm+kms on linux (on a macbook)

## Install
 
 * a supported video card (I'm using intel HD 3000)
 * libdrm
 * mesa (egl, glesv2)

For now, you'll have to figure the rest out.

## Plan

### History

I've been trying to run wayland for (3+) years and only recently decided toi
take the plunge and put arch linux on my macbook.  Seeing that my particular
video card model was able to support wayland I became very excited!

So, now I'm running wayland and the next obvious step is to start hacking on
it.  I guess we should back up a bit.  Wayland by itself simply provides some
machinery for building a client and server. In order to get things onto the
screen, you need to use weston which sets up KMS and manages surfaces.

All rendering is done on the cpu via pixman/cairo/skia/etc or on the GPU via
egl/opengl. OpenGL textures are shared on the gpu via the `EGL_KHR_image_base`
extension.  Clients connect to Weston via the protocol specified in wayland
and are granted surfaces to render into by weston.

Everything make sense? It's actually pretty cool imho.  Except for a few things.

 * Weston's api surface is pretty large, and fairly verbose.
 * the size of compositor code in weston is negligable assuming you don't want to play in that space.
 * Weston loads modules (.so) which provide functionality, like a desktop shell, toolbars, etc.
 * The default `desktop_shell` module is ~5000 lines of c

What I'd really like is a compositor that manages video devices and the drawing of
client surfaces.  This libary could be linked against in a node addon, and assuming
the library provides enough api, much of the compositing logic could be handled in
javascript land.

A large benefit of this is that we can play around with the protocol that it speaks,
and how window management is performed.  If any of this becomes a bottleneck, the
option of moving it into C still exists.  Speed of development is my primary concern
right now.

### Now

First, I need to be able to test each one of the compositing components independently.  This is important from both an efficiency perspective and reference perspective.  In short, I'm still getting my bearings.

What exists right now:

 * eglkms.c - a basic demo of a triangle rendered in opengl directly
 * eglkms-mouse.c - adding onto the triangle example, this example pulls in mouse events from `/dev/input/event12` so you can move the triangle around the screen.  Much like a mouse pointer.
 * mouse.js - a test to prove that opening and reading events from the input stream is viable (assuming we know the length of the `timeval`)
 * list-modes - shows how you collect available modes from connected displays

### Next

#### composited

This is the main compositing component of the stack.  The idea here is that it has the ability to
run the `drmSetMaster` on a VT and perform kernel mode setting (kms) operations. This will likely
mean that it will need to be run as root.

At the base we have an addon that binds to a library that does the least possible amount of work to
create shared surfaces and perform double buffered vsync'd page flips to the display(s).

major requirements:

 * surface management/sharing
 * display resolution/bpp management
 * vsync render cycle
 * double / triple buffering

In javascript the intent right now is to have a scene graph written in js where each node is a wrapper for a surface handle in c.  It will also provide a way to specify geometric operations such as:

 * move surface (x,y,z)
 * move camera (x,y,z)
 * add/remove/delete surfaces
 * manage stacking order of surfaces
 * pin surfaces to the top

#### human events

Human input events will be handled by a swappable daemon that provides a way to do keybinds/gestures/etc.  This daemon connects to `composited` over tcp on 127.0.0.1:9999. Its entire purpose is to turn HID events into commands that are dispatched to `composited` to perform operations on the scene.

It is likely that this daemon will be written completely in node except for a binding to xkbcommon.

### Later

There are quite a few definite benefits being able to speak wayland's protocol, so there will be an implementation that uses the expected unix socket.

## Summary

I want a system that allows me to swap out components and rapidly test usability of experiments. 

# license






