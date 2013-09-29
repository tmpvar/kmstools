#include <stdint.h>
#include <inttypes.h>
#include <fcntl.h>
#include <sys/types.h>
#include <libdrm/drm.h>
#include <stdio.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <unistd.h>

int main() {
  int drm_available = drmAvailable();


  printf(
    "drmAvailable: %s\n",
    drm_available ? "yes" : "no"
  );
 
  int fd = open("/dev/dri/card0", O_RDWR);
  printf(
    "drmOpen: %s\n",
     fd ? "yes" : "no"
  ); 

  drmModeRes *resources = drmModeGetResources(fd);

  printf(
    "drmModeGetResources: %s\n",
     resources ? "yes" : "no"
  ); 

  if (!resources) {
    return -1; 
  }

  printf(
    "found: %u connectors, %u encoders, %u crtcs and %u fbs\n",
    resources->count_connectors,
    resources->count_encoders,
    resources->count_crtcs,
    resources->count_fbs
  );

  int i;
  drmModeConnector *connector; 
  for (i=0; i<resources->count_connectors; i++) {
    connector = drmModeGetConnector(
      fd,
      resources->connectors[i]
    ); 
    
    printf(
      "%u: %ummx%umm\n",
      i,
      connector->mmWidth,
      connector->mmHeight
    );

    printf("    modes (%i)\n", connector->count_modes);
    int m;
    drmModeModeInfo *mode;
    for (m=0; m<connector->count_modes; m++) {
      mode = &connector->modes[m];
      printf("    o %ux%u - %s\n", mode->hdisplay, mode->vdisplay, mode->name);
    }
    printf("\n");
    drmModeFreeConnector(connector);
  }

  drmModeFreeResources(resources);

  return 0;
}
