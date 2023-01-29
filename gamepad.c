#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/uinput.h>

const __u16 key_bindings[KEY_CNT] = {
    [KEY_H] = BTN_TL,
    [KEY_L] = BTN_TR,

    [KEY_U] = BTN_TL2,
    [KEY_P] = BTN_TR2,

    [KEY_K] = BTN_A,
    [KEY_O] = BTN_B,
    [KEY_J] = BTN_X,
    [KEY_I] = BTN_Y,

    [KEY_F] = BTN_SELECT,
    [KEY_G] = BTN_START,
};

const __u16 joystick_keybinds[KEY_CNT] = {
    [KEY_A] = ABS_X,
    [KEY_D] = ABS_X,
    [KEY_S] = ABS_Y,
    [KEY_W] = ABS_Y};

const __u16 joystick_values[KEY_CNT] = {
    [KEY_A] = -256,
    [KEY_S] = 256,
    [KEY_D] = 256,
    [KEY_W] = -256};

int translate_keyboard_to_gamepad(int gamepad_fd, struct input_event *kbevent)
{
  struct input_event ev;
  memset(&ev, 0, sizeof(struct input_event)); // setting the memory for event
  if (key_bindings[kbevent->code])
  {
    ev.type = kbevent->type;
    ev.code = key_bindings[kbevent->code];
    ev.value = kbevent->value;
  }
  else if (joystick_keybinds[kbevent->code] || kbevent->code == KEY_A || kbevent->code == KEY_D)
  {
    ev.type = EV_ABS;
    ev.code = joystick_keybinds[kbevent->code];
    ev.value = kbevent->value == 1 || kbevent->value == 2 ? joystick_values[kbevent->code] : 0;
  }

  if (write(gamepad_fd, &ev, sizeof(struct input_event)) < 0) // writing the key change
  {
    printf("error: key-write");
    return 1;
  }

  return 0;
}

int main(void)
{
  int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK); // opening of uinput
  int fdin = open("/dev/input/event4", O_RDONLY);

  if (fd < 0 || fdin < 0)
  {
    printf("Opening of uinput failed!\n");
    return 1;
  }

  ioctl(fd, UI_SET_EVBIT, EV_KEY); // setting Gamepad keys
  ioctl(fd, UI_SET_KEYBIT, BTN_A);
  ioctl(fd, UI_SET_KEYBIT, BTN_B);
  ioctl(fd, UI_SET_KEYBIT, BTN_X);
  ioctl(fd, UI_SET_KEYBIT, BTN_Y);
  ioctl(fd, UI_SET_KEYBIT, BTN_TL);
  ioctl(fd, UI_SET_KEYBIT, BTN_TR);
  ioctl(fd, UI_SET_KEYBIT, BTN_TL2);
  ioctl(fd, UI_SET_KEYBIT, BTN_TR2);
  ioctl(fd, UI_SET_KEYBIT, BTN_START);
  ioctl(fd, UI_SET_KEYBIT, BTN_SELECT);
  ioctl(fd, UI_SET_KEYBIT, BTN_THUMBL);
  ioctl(fd, UI_SET_KEYBIT, BTN_THUMBR);
  ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_UP);
  ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_DOWN);
  ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_LEFT);
  ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_RIGHT);

  ioctl(fd, UI_SET_EVBIT, EV_ABS); // setting Gamepad thumbsticks
  ioctl(fd, UI_SET_ABSBIT, ABS_X);
  ioctl(fd, UI_SET_ABSBIT, ABS_Y);
  ioctl(fd, UI_SET_ABSBIT, ABS_RX);
  ioctl(fd, UI_SET_ABSBIT, ABS_RY);

  struct uinput_user_dev uidev; // s etting the default settings of Gamepad
  memset(&uidev, 0, sizeof(uidev));
  snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Poor Man's Gamepad"); // Name of Gamepad
  uidev.id.bustype = BUS_USB;
  uidev.id.vendor = 0x3;
  uidev.id.product = 0x3;
  uidev.id.version = 2;

  uidev.absmax[ABS_X] = 512; // Parameters of thumbsticks
  uidev.absmin[ABS_X] = -512;
  uidev.absfuzz[ABS_X] = 0;
  uidev.absflat[ABS_X] = 15;

  uidev.absmax[ABS_Y] = 512;
  uidev.absmin[ABS_Y] = -512;
  uidev.absfuzz[ABS_Y] = 0;
  uidev.absflat[ABS_Y] = 15;

  uidev.absmax[ABS_RX] = 512;
  uidev.absmin[ABS_RX] = -512;
  uidev.absfuzz[ABS_RX] = 0;
  uidev.absflat[ABS_RX] = 16;

  uidev.absmax[ABS_RY] = 512;
  uidev.absmin[ABS_RY] = -512;
  uidev.absfuzz[ABS_RY] = 0;
  uidev.absflat[ABS_RY] = 16;

  if (write(fd, &uidev, sizeof(uidev)) < 0) // writing settings
  {
    printf("error: write");
    return 1;
  }

  if (ioctl(fd, UI_DEV_CREATE) < 0) // writing ui dev create
  {
    printf("error: ui_dev_create");
    return 1;
  }

  struct input_event kbev;
  ssize_t bytes_read;
  unsigned char toggle = 0;
  while (1)
  {
    bytes_read = read(fdin, &kbev, sizeof(struct input_event));
    if (bytes_read == sizeof(struct input_event))
    {
      int result = translate_keyboard_to_gamepad(fd, &kbev);
      if (result == 1)
      {
        return 1;
      }
    }
  }

  if (ioctl(fd, UI_DEV_DESTROY) < 0)
  {
    printf("error: ioctl");
    return 1;
  }

  close(fd);
  close(fdin);
  return 1;
}
