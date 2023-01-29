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

    [KEY_Z] = BTN_DPAD_LEFT,
    [KEY_C] = BTN_DPAD_RIGHT,
    [KEY_S] = BTN_DPAD_UP,
    [KEY_X] = BTN_DPAD_DOWN,
};

const __u16 joystick_keybinds[KEY_CNT] = {
    [KEY_Q] = ABS_X, // Left
    [KEY_E] = ABS_X, // Right
    [KEY_2] = ABS_Y, // Up
    [KEY_W] = ABS_Y, // Down

    [KEY_SEMICOLON] = ABS_RX,
    [KEY_BACKSLASH] = ABS_RX,
    [KEY_LEFTBRACE] = ABS_RY,
    [KEY_APOSTROPHE] = ABS_RY,

    [KEY_LEFT] = ABS_RX,
    [KEY_RIGHT] = ABS_RX,
    [KEY_UP] = ABS_RY,
    [KEY_DOWN] = ABS_RY,
};

const __u16 MAX_JOYSTICK_VALUE = 512;

const __u16 joystick_values[KEY_CNT] = {
    [KEY_Q] = -MAX_JOYSTICK_VALUE,
    [KEY_E] = MAX_JOYSTICK_VALUE,
    [KEY_2] = -MAX_JOYSTICK_VALUE,
    [KEY_W] = MAX_JOYSTICK_VALUE,

    [KEY_SEMICOLON] = -MAX_JOYSTICK_VALUE,
    [KEY_BACKSLASH] = MAX_JOYSTICK_VALUE,
    [KEY_LEFTBRACE] = -MAX_JOYSTICK_VALUE,
    [KEY_APOSTROPHE] = MAX_JOYSTICK_VALUE,

    [KEY_LEFT] = -MAX_JOYSTICK_VALUE,
    [KEY_RIGHT] = MAX_JOYSTICK_VALUE,
    [KEY_UP] = -MAX_JOYSTICK_VALUE,
    [KEY_DOWN] = MAX_JOYSTICK_VALUE,
};

int write_to_gamepad(int gamepad_fd, struct input_event ev)
{
  if (write(gamepad_fd, &ev, sizeof(struct input_event)) < 0) // writing the key change
  {
    printf("error: key-write");
    return 1;
  }
}

int translate_keyboard_to_gamepad(int gamepad_fd, struct input_event *kbevent)
{
  struct input_event ev;

  if (key_bindings[kbevent->code])
  {
    ev.type = kbevent->type;
    ev.code = key_bindings[kbevent->code];
    ev.value = kbevent->value;
    write_to_gamepad(gamepad_fd, ev);
  }
  else if (joystick_keybinds[kbevent->code] || kbevent->code == KEY_Q || kbevent->code == KEY_E)
  {
    ev.type = EV_ABS;
    ev.code = joystick_keybinds[kbevent->code];
    ev.value = kbevent->value == 1 || kbevent->value == 2 ? joystick_values[kbevent->code] : 0;
    write_to_gamepad(gamepad_fd, ev);
  }
  else if (kbevent->type == EV_KEY && kbevent->value == 1)
  {
    // printf("Unknown key: %i\n", kbevent->code);
  }

  memset(&ev, 0, sizeof(struct input_event));
  ev.type = EV_SYN;
  ev.code = SYN_REPORT;
  ev.value = 0;
  if (write(gamepad_fd, &ev, sizeof(struct input_event)) < 0) // writing the sync report
  {
    printf("error: sync-report");
    return 1;
  }
  return 0;
}

int main(void)
{
  int gamepad_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK); // opening of uinput
  int keyboard_fd = open("/dev/input/event4", O_RDONLY);

  if (gamepad_fd < 0 || keyboard_fd < 0)
  {
    printf("Opening of input failed!\n");
    return 1;
  }

  ioctl(gamepad_fd, UI_SET_EVBIT, EV_KEY); // setting Gamepad keys
  ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_A);
  ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_B);
  ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_X);
  ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_Y);
  ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_TL);
  ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_TR);
  ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_TL2);
  ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_TR2);
  ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_START);
  ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_SELECT);
  ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_THUMBL);
  ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_THUMBR);
  ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_DPAD_UP);
  ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_DPAD_DOWN);
  ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_DPAD_LEFT);
  ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_DPAD_RIGHT);

  ioctl(gamepad_fd, UI_SET_EVBIT, EV_ABS); // setting Gamepad thumbsticks
  ioctl(gamepad_fd, UI_SET_ABSBIT, ABS_X);
  ioctl(gamepad_fd, UI_SET_ABSBIT, ABS_Y);
  ioctl(gamepad_fd, UI_SET_ABSBIT, ABS_RX);
  ioctl(gamepad_fd, UI_SET_ABSBIT, ABS_RY);

  struct uinput_user_dev uidev; // setting the default settings of Gamepad
  memset(&uidev, 0, sizeof(uidev));
  snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Poor Man's Gamepad"); // Name of Gamepad
  uidev.id.bustype = BUS_USB;
  uidev.id.vendor = 0x3;
  uidev.id.product = 0x3;
  uidev.id.version = 2;

  uidev.absmax[ABS_X] = MAX_JOYSTICK_VALUE; // Parameters of thumbsticks
  uidev.absmin[ABS_X] = -MAX_JOYSTICK_VALUE;
  uidev.absfuzz[ABS_X] = 0;
  uidev.absflat[ABS_X] = 15;

  uidev.absmax[ABS_Y] = MAX_JOYSTICK_VALUE;
  uidev.absmin[ABS_Y] = -MAX_JOYSTICK_VALUE;
  uidev.absfuzz[ABS_Y] = 0;
  uidev.absflat[ABS_Y] = 15;

  uidev.absmax[ABS_RX] = MAX_JOYSTICK_VALUE;
  uidev.absmin[ABS_RX] = -MAX_JOYSTICK_VALUE;
  uidev.absfuzz[ABS_RX] = 0;
  uidev.absflat[ABS_RX] = 16;

  uidev.absmax[ABS_RY] = MAX_JOYSTICK_VALUE;
  uidev.absmin[ABS_RY] = -MAX_JOYSTICK_VALUE;
  uidev.absfuzz[ABS_RY] = 0;
  uidev.absflat[ABS_RY] = 16;

  if (write(gamepad_fd, &uidev, sizeof(uidev)) < 0) // writing settings
  {
    printf("error: write");
    return 1;
  }

  if (ioctl(gamepad_fd, UI_DEV_CREATE) < 0) // writing ui dev create
  {
    printf("error: ui_dev_create");
    return 1;
  }

  struct input_event kbev;
  ssize_t bytes_read;
  while (1)
  {
    bytes_read = read(keyboard_fd, &kbev, sizeof(struct input_event));
    if (bytes_read == sizeof(struct input_event))
    {
      int result = translate_keyboard_to_gamepad(gamepad_fd, &kbev);
      if (result == 1)
      {
        return 1;
      }
    }
  }

  if (ioctl(gamepad_fd, UI_DEV_DESTROY) < 0)
  {
    printf("error: ioctl");
    return 1;
  }

  close(gamepad_fd);
  close(keyboard_fd);
  return 1;
}
