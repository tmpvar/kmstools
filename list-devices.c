#include <libudev.h>
#include <stdio.h>

int main() {
  struct udev *udev = udev_new();
  struct udev_enumerate *enumerate;
  struct udev_list_entry *devices, *current_device;
  struct udev_device *dev;

  if (!udev) {
    printf("could not initialize\n");
    return 1;
  }

  enumerate = udev_enumerate_new(udev);
  udev_enumerate_add_match_subsystem(enumerate, "video*");
  udev_enumerate_scan_devices(enumerate);
  devices = udev_enumerate_get_list_entry(enumerate);
  
  udev_list_entry_foreach(current_device, devices) {
    const char *path = udev_list_entry_get_name(current_device);
    dev = udev_device_new_from_syspath(udev, path);
    printf("- %s\n", udev_device_get_devnode(dev));
    udev_device_unref(dev);
  }

  udev_enumerate_unref(enumerate);
  udev_unref(udev); 
  return 0;
}

