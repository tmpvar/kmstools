/*
 * Copyright © 2011 Kristian Høgsberg
 * Copyright © 2011 Benjamin Franzke
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>

#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES

#include <gbm.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <drm.h>
#include <xf86drmMode.h>
#include <xf86drm.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <linux/input.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>

EGLDisplay dpy;
EGLContext ctx;
EGLSurface surface;
EGLConfig config;
EGLint major, minor, n;
const char *ver;
uint32_t handle, stride;
struct kms kms;
int ret, fd;
struct gbm_device *gbm;
struct gbm_bo *bo;
drmModeCrtcPtr saved_crtc;
struct gbm_surface *gs;

struct kms {
   drmModeConnector *connector;
   drmModeEncoder *encoder;
   drmModeModeInfo mode;
   uint32_t fb_id;
};

static EGLBoolean
setup_kms(int fd, struct kms *kms)
{
   drmModeRes *resources;
   drmModeConnector *connector;
   drmModeEncoder *encoder;
   int i;

   resources = drmModeGetResources(fd);
   if (!resources) {
      fprintf(stderr, "drmModeGetResources failed\n");
      return EGL_FALSE;
   }

   for (i = 0; i < resources->count_connectors; i++) {
      connector = drmModeGetConnector(fd, resources->connectors[i]);
      if (connector == NULL)
	 continue;

      if (connector->connection == DRM_MODE_CONNECTED &&
	  connector->count_modes > 0)
	 break;

      drmModeFreeConnector(connector);
   }

   if (i == resources->count_connectors) {
      fprintf(stderr, "No currently active connector found.\n");
      return EGL_FALSE;
   }

   for (i = 0; i < resources->count_encoders; i++) {
      encoder = drmModeGetEncoder(fd, resources->encoders[i]);

      if (encoder == NULL)
	 continue;

      if (encoder->encoder_id == connector->encoder_id)
	 break;

      drmModeFreeEncoder(encoder);
   }

   kms->connector = connector;
   kms->encoder = encoder;
   kms->mode = connector->modes[0];

   return EGL_TRUE;
}

GLfloat view_rotx = 0.0, view_roty = 0.0, view_rotz = 45.0;
GLfloat view_tranx = 0.0, view_trany = 0.0, view_tranz = -60;

static void
setup_viewport(int width, int height) {
  GLfloat ar = (GLfloat) width / (GLfloat) height;

  glViewport(0, 0, (GLint) width, (GLint) height);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-ar, ar, -1, 1, 5.0, 60.0);

}

static void
render_stuff()
{
   static const GLfloat verts[3][2] = {
      { -1, -1 },
      {  1, -1 },
      {  0,  1 }
   };
   static const GLfloat colors[3][3] = {
      { 1, 0, 0 },
      { 0, 1, 0 },
      { 0, 0, 1 }
   };

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(view_tranx, view_trany, view_tranz);

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glClearColor(0.4, 0.4, 0.4, 0.0);
 
   glEnable(GL_BLEND);
   glEnable(GL_LINE_SMOOTH);
   glEnable(GL_POINT_SMOOTH);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
   glEnable(GL_POLYGON_SMOOTH);
   glEnable(GL_MULTISAMPLE);

   glPushMatrix();
   glRotatef(view_rotx, 1, 0, 0);
   glRotatef(view_roty, 0, 1, 0);
   glRotatef(view_rotz, 0, 0, 1);

   glVertexPointer(2, GL_FLOAT, 0, verts);
   glColorPointer(3, GL_FLOAT, 0, colors);
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_COLOR_ARRAY);

   glDrawArrays(GL_TRIANGLES, 0, 3);

   glDisableClientState(GL_VERTEX_ARRAY);
   glDisableClientState(GL_COLOR_ARRAY);

   glPopMatrix();

   glFinish();
}

static const char device_name[] = "/dev/dri/card0";

static const EGLint attribs[] = {
   EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
   EGL_RED_SIZE, 1,
   EGL_GREEN_SIZE, 1,
   EGL_BLUE_SIZE, 1,
   EGL_ALPHA_SIZE, 0,
   EGL_DEPTH_SIZE, 1,
   EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
   EGL_SAMPLE_BUFFERS, 1,
   EGL_SAMPLES, 4,
   EGL_NONE
};

int done = 0;
void sighandler(int signum) {
  done = 1;
  printf("handled %i, exiting..\n", signum);
}

int pending_flip = 0;

static void handle_flip_event(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data) {
  pending_flip = 0;
}


static void fb_prepare() {

}

static void fb_flip() {


}


int main()
{
   signal(SIGTERM, sighandler);
   signal(SIGINT, sighandler);


   fd = open(device_name, O_RDWR);
   if (fd < 0) {
      /* Probably permissions error */
      fprintf(stderr, "couldn't open %s, skipping\n", device_name);
      return -1;
   }

   gbm = gbm_create_device(fd);
   if (gbm == NULL) {
      fprintf(stderr, "couldn't create gbm device\n");
      ret = -1;
      goto close_fd;
   }

   dpy = eglGetDisplay(gbm);
   if (dpy == EGL_NO_DISPLAY) {
      fprintf(stderr, "eglGetDisplay() failed\n");
      ret = -1;
      goto destroy_gbm_device;
   }
	
   if (!eglInitialize(dpy, &major, &minor)) {
      printf("eglInitialize() failed\n");
      ret = -1;
      goto egl_terminate;
   }

   ver = eglQueryString(dpy, EGL_VERSION);
   printf("EGL_VERSION = %s\n", ver);

   if (!setup_kms(fd, &kms)) {
      ret = -1;
      goto egl_terminate;
   }

   eglBindAPI(EGL_OPENGL_API);

   if (!eglChooseConfig(dpy, attribs, &config, 1, &n) || n != 1) {
      fprintf(stderr, "failed to choose argb config\n");
      goto egl_terminate;
   }
   
   ctx = eglCreateContext(dpy, config, EGL_NO_CONTEXT, NULL);
   if (ctx == NULL) {
      fprintf(stderr, "failed to create context\n");
      ret = -1;
      goto egl_terminate;
   }

   gs = gbm_surface_create(gbm, kms.mode.hdisplay, kms.mode.vdisplay,
			   GBM_BO_FORMAT_XRGB8888,
			   GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
   surface = eglCreateWindowSurface(dpy, config, gs, NULL);

   if (!eglMakeCurrent(dpy, surface, surface, ctx)) {
      fprintf(stderr, "failed to make context current\n");
      ret = -1;
      goto destroy_context;
   }
   

   int evfd = open("/dev/input/event13", O_RDONLY|O_NDELAY);
   if (!evfd) {
     done = 1;
     printf("could not open event stream\n");
   }

   setup_viewport(kms.mode.hdisplay, kms.mode.vdisplay);

   saved_crtc = drmModeGetCrtc(fd, kms.encoder->crtc_id);
   if (saved_crtc == NULL) {
     goto rm_fb; 
   }
  
   int touch = 0, first = 1;
   unsigned int posx = 0, posy = 0;
   printf("ready to go\n");
   while (!done) {
     
     if (!pending_flip) {
       render_stuff();

       eglSwapBuffers(dpy, surface);

       bo = gbm_surface_lock_front_buffer(gs);
       handle = gbm_bo_get_handle(bo).u32;
       stride = gbm_bo_get_stride(bo);
       if (drmModeAddFB(fd,
	    	      kms.mode.hdisplay, kms.mode.vdisplay,
		        24, 32, stride, handle, &kms.fb_id))
       {
         fprintf(stderr, "failed to create fb\n");
         goto restore_crtc;
       }
     
       gbm_surface_release_buffer(gs, bo);
       ret = drmModePageFlip(fd, kms.encoder->crtc_id, kms.fb_id, DRM_MODE_PAGE_FLIP_EVENT, NULL);
       pending_flip = 1;
     } else {
       fd_set pending;
       FD_ZERO(&pending);
       FD_SET(fd, &pending);
       struct timeval t;
       memset(&t, 0, sizeof(struct timeval));
       if (select(fd+1, &pending, NULL, NULL, &t)) {
         drmEventContext ev;
         memset(&ev, 0, sizeof(drmEventContext));
         ev.version = DRM_EVENT_CONTEXT_VERSION;
         ev.page_flip_handler = handle_flip_event;

         drmHandleEvent(fd, &ev);
       }
     } 
     
     struct input_event mouse_event[64];
     int bytes = read(evfd, mouse_event, sizeof(struct input_event)*64); 

     if (bytes > 0) { 
        int e;
        for (e=0; e<bytes/sizeof(struct input_event);e++) {
          struct input_event ev = mouse_event[e];

          switch (ev.type) {
            case EV_KEY:
              if (ev.code == BTN_TOUCH) {
                touch = (ev.value) ? 1 : 0;
                posx = 0, posy = 0;
                printf("touch %s\n", touch ? "on" : "off");
              }
            break;

            case EV_ABS:
              if (touch) {
                if (ev.code == ABS_MT_POSITION_X) {
                  if (posx) {
                    int diff = ev.value - posx;
                    view_tranx += diff * .01;
                  }
                  posx = ev.value;  
                  printf(" - posx: %u; view_tranx: %f\n", posx, view_tranx);
                }

                if (ev.code == ABS_MT_POSITION_Y) {
                  if (posy) {
                    int diff = ev.value - posy;
                    view_trany -= diff * .01;
                  }
                  posy = ev.value;  
                  printf(" - posx: %u; view_trany: %f\n", posy, view_trany);
                }
                
              }
            break;       
          }

          if (ev.type == EV_KEY && ev.code == BTN_LEFT && ev.value) {
            done = 1;
          }
        }
     }
   }

   printf("done\n");

restore_crtc:
   ret = drmModeSetCrtc(fd, saved_crtc->crtc_id, saved_crtc->buffer_id,
                        saved_crtc->x, saved_crtc->y,
                        &kms.connector->connector_id, 1, &saved_crtc->mode);
   if (ret) {
     fprintf(stderr, "failed to restore crtc: %m\n");
   }
free_saved_crtc:
   drmModeFreeCrtc(saved_crtc);
rm_fb:
   drmModeRmFB(fd, kms.fb_id);
   eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
destroy_context:
   eglDestroyContext(dpy, ctx);
egl_terminate:
   eglTerminate(dpy);
destroy_gbm_device:
   gbm_device_destroy(gbm);
close_fd:
   close(fd);

   return ret;
}
