#ifndef _BLUETOOTH_H__
#define _BLUETOOTH_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

int bluetooth_search_device(char *mac_addr);

#endif
