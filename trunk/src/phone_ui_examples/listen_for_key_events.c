#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lcm/lcm.h>
#include <lcmtypes/navlcm_s60_key_event_t.h>

#include <common/s60_key_codes.h>

static int
on_key_event (const char *channel, const navlcm_s60_key_event_t *msg, 
        void *user)
{
    const char *typestr = NULL;
    if (msg->type == S60_KEY_PRESS) typestr = "press";
    else if (msg->type == S60_KEY_UP) typestr = "key up";
    else if (msg->type == S60_KEY_DOWN) typestr = "key down";
    else typestr = "???";

    printf ("keycode: 0x%04X scancode: 0x%04X type: %9s modifiers: 0x%08X\n",
            msg->keycode, msg->scancode, typestr, msg->modifiers);
    return 0;
}

int main (int argc, char **argv)
{
    lcm_t *lcm = lcm_create ("udpm://");

    navlcm_s60_key_event_t_subscribe (lcm, "KEY", on_key_event, NULL);

    while (1) {
        lcm_handle (lcm);
    }

    lcm_destroy (lcm);

    return 0;
}
