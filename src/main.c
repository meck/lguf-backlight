#include <libusb-1.0/libusb.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define MIN_STEP 1
#define MAX_STEP 10

static const int HID_GET_REPORT = 0x01;
static const int HID_SET_REPORT = 0x09;
static const int HID_REPORT_TYPE_FEATURE = 0x03;

static const uint16_t vendor_id = 0x43e;
static const uint16_t product_id = 0x9a40;

uint16_t steps[] = {
    0x0000, 0x0190, 0x01af, 0x01d2, 0x01f7, 0x021f, 0x024a, 0x0279, 0x02ac,
    0x02e2, 0x031d, 0x035c, 0x03a1, 0x03eb, 0x043b, 0x0491, 0x04ee, 0x0553,
    0x05c0, 0x0635, 0x06b3, 0x073c, 0x07d0, 0x086f, 0x091b, 0x09d5, 0x0a9d,
    0x0b76, 0x0c60, 0x0d5c, 0x0e6c, 0x0f93, 0x10d0, 0x1227, 0x1399, 0x1529,
    0x16d9, 0x18aa, 0x1aa2, 0x1cc1, 0x1f0b, 0x2184, 0x2430, 0x2712, 0x2a2e,
    0x2d8b, 0x312b, 0x3516, 0x3951, 0x3de2, 0x42cf, 0x4822, 0x4de1, 0x5415,
    0x5ac8, 0x6203, 0x69d2, 0x7240, 0x7b5a, 0x852d, 0x8fc9, 0x9b3d, 0xa79b,
    0xb4f5, 0xc35f, 0xd2f0};

const size_t n_steps = sizeof(steps) / sizeof(steps[0]);

static libusb_device *get_lg_ultrafine_usb_device(libusb_device **devs) {
  libusb_device *dev, *lgdev = NULL;
  int i = 0;

  while ((dev = devs[i++]) != NULL) {
    struct libusb_device_descriptor desc;
    int r = libusb_get_device_descriptor(dev, &desc);
    if (r < 0) {
      printf("failed to get device descriptor");
      return NULL;
    }

    if (desc.idVendor == vendor_id && desc.idProduct == product_id) {
      lgdev = dev;
    }
  }

  return lgdev;
}

size_t get_brightness(libusb_device_handle *handle) {
  unsigned char data[8] = {0x00};

  int res = libusb_control_transfer(
      handle,
      LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS |
          LIBUSB_RECIPIENT_INTERFACE,
      HID_GET_REPORT, (HID_REPORT_TYPE_FEATURE << 8) | 0, 1, data, sizeof(data),
      0);

  if (res < 0) {
    printf("Unable to get brightness.\n");
    printf("libusb_control_transfer error: %s (%d)\n", libusb_error_name(res),
           res);
    exit(EXIT_FAILURE);
  }

  uint16_t val = data[0] + (data[1] << 8);

  size_t idx = 0;
  while (idx++ < n_steps) {
    if (val <= steps[idx]) {
      return idx;
    }
  }

  printf("Bad brightness index.\n");
  exit(EXIT_FAILURE);
}

void set_brightness(libusb_device_handle *handle, size_t idx) {

  if (idx >= n_steps) {
    printf("Bad brightness index.\n");
    exit(EXIT_FAILURE);
  }

  uint16_t val = steps[idx];

  unsigned char data[6] = {(unsigned char)(val & 0x00ff),
                           (unsigned char)((val >> 8) & 0x00ff),
                           0x00,
                           0x00,
                           0x00,
                           0x00};
  int res = libusb_control_transfer(
      handle,
      LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS |
          LIBUSB_RECIPIENT_INTERFACE,
      HID_SET_REPORT, (HID_REPORT_TYPE_FEATURE << 8) | 0, 1, data, sizeof(data),
      0);

  if (res < 0) {
    printf("Unable to set brightness.\n");
    printf("libusb_control_transfer error: %s\n", libusb_error_name(res));
    exit(EXIT_FAILURE);
  }
}

void increment(libusb_device_handle *handle, size_t step) {
  size_t bri = get_brightness(handle);
  bri = bri + step < n_steps ? bri + step : n_steps - 1;
  set_brightness(handle, bri);
}

void decrement(libusb_device_handle *handle, size_t step) {
  size_t bri = get_brightness(handle);
  bri = step < bri ? bri - step : 0;
  set_brightness(handle, bri);
}

int main(int argc, char *argv[]) {

  size_t step_size = 1;
  bool do_increment;

  libusb_device **devs, *lgdev;
  int res, openCode, iface = 1;
  ssize_t cnt;
  libusb_device_handle *handle;

  if (argc < 2 || argc > 3 ||
      !(strcmp(argv[1], "inc") == 0 || strcmp(argv[1], "dec") == 0)) {

    printf("usage: %s <command> [<stepsize>]\n", argv[0]);
    printf("  command:  inc or dec\n");
    printf("  stepsize: %d to %d\n", MIN_STEP, MAX_STEP);

    exit(EXIT_FAILURE);
  }

  do_increment = strcmp(argv[1], "inc") == 0;

  if (argc == 3) {
    step_size = (size_t)atoi(argv[2]);
  };

  if (step_size < MIN_STEP || step_size > MAX_STEP) {
    printf("Invalid step size, should be %d to %d\n", MIN_STEP, MAX_STEP);
    exit(EXIT_FAILURE);
  }

  res = libusb_init(NULL);

#if LIBUSB_API_VERSION >= 0x01000106
  libusb_set_option(NULL, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING);
#else
  libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_WARNING); // LIBUSB_LOG_LEVEL_DEBUG
#endif

  if (res < 0) {
    printf("Unable to initialize libusb.\n");
    return res;
  }

  cnt = libusb_get_device_list(NULL, &devs);
  if (cnt < 0) {
    printf("Unable to get USB device list (%ld).\n", cnt);
    return (int)cnt;
  }

  lgdev = get_lg_ultrafine_usb_device(devs);

  if (lgdev == NULL) {
    printf("Failed to get LG screen device.\n");
    return -1;
  }

  openCode = libusb_open(lgdev, &handle);

  if (openCode == 0) {
    libusb_set_auto_detach_kernel_driver(handle, 1);
    res = libusb_claim_interface(handle, iface);
    if (res == LIBUSB_SUCCESS) {

      do_increment ? increment(handle, step_size)
                   : decrement(handle, step_size);

      libusb_release_interface(handle, iface);
      libusb_attach_kernel_driver(handle, iface);
    } else {
      printf("Failed to claim interface %d. Error: %d\n", iface, res);
      printf("Error: %s\n", libusb_error_name(res));
    }
    libusb_close(handle);
  } else {
    printf("libusb_open failed and returned %d\n", openCode);
  }
  libusb_free_device_list(devs, 1);

  libusb_exit(NULL);

  return EXIT_SUCCESS;
}
