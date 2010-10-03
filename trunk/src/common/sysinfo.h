#ifndef _COMMON_ACPI_H__
#define _COMMON_ACPI_H__

#include <stdlib.h>
#include <stdio.h>
#include <sys/statvfs.h>

#include <lcmtypes/navlcm_system_info_t.h>

void sysinfo_read_acpi (navlcm_system_info_t *sysinfo);
int sysinfo_read_sys_cpu_mem (navlcm_system_info_t *s);
void sysinfo_read_disk_stats (navlcm_system_info_t *s);

#endif

