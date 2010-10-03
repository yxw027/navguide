#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <lcm/lcm.h>
#include <lcmtypes/navlcm_phone_print_t.h>

int main (int argc, char **argv)
{
    lcm_t *lcm = lcm_create ("udpm://?transmit_only=true");

    char buf[1024];

    navlcm_phone_print_t msg;
    msg.text = buf;

    printf ("type stuff\n");
    while (fgets (buf, sizeof (buf), stdin)) {
        if (strlen (buf) == 0) break;
        g_strchomp (buf);
        navlcm_phone_print_t_publish (lcm, "PHONE_PRINT", &msg);
    }

    lcm_destroy (lcm);

    return 0;
}
