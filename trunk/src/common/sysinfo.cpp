#include <common/sysinfo.h>

#define delimiters " \t\n\r"

/* update system info from /proc/acpi
 */
void sysinfo_read_acpi (navlcm_system_info_t *sysinfo)
{
    FILE *fp;
    char *line = (char*)malloc(50);
    char *l, *m;

    // critical temperature
    fp = fopen ("/proc/acpi/thermal_zone/THM/trip_points", "r");
    if (fp) {
        if (fgets (line, 49, fp)) {
            l = line;
            while ((m = strsep (&l, delimiters)) != NULL) {
                if (sscanf (m, "%d", &sysinfo->temp_critical) == 1)
                    break;
            }
        }
    fclose (fp);
    }

    // CPU temperature
    fp = fopen ("/proc/acpi/thermal_zone/THM/temperature", "r");
    if (fp) {
        if (fgets (line, 49, fp)) {
            l = line;
            while ((m = strsep (&l, delimiters)) != NULL) {
                if (sscanf (m, "%d", &sysinfo->temp_cpu) == 1)
                    break;
            }
        }
    fclose (fp);
    }

    // battery discharge rate
    fp = fopen ("/proc/acpi/battery/BAT0/state", "r");
    if (fp) {
        while (fgets (line, 49, fp)) {
            if (strstr (line, "present rate")) {
                l = line;
                while ((m = strsep (&l, delimiters)) != NULL) {
                    if (sscanf (m, "%d", &sysinfo->battery_discharge_rate) == 1)
                        break;
                }
            }
        }
    fclose (fp);
    }

    // battery remaining mA
    fp = fopen ("/proc/acpi/battery/BAT0/state", "r");
    if (fp) {
        while (fgets (line, 49, fp)) {
            if (strstr (line, "remaining capacity")) {
                l = line;
                while ((m = strsep (&l, delimiters)) != NULL) {
                    if (sscanf (m, "%d", &sysinfo->battery_remaining_mA) == 1)
                        break;
                }
            }
        }
    fclose (fp);
    }

    // battery design capacity
    fp = fopen ("/proc/acpi/battery/BAT0/info", "r");
    if (fp) {
        while (fgets (line, 49, fp)) {
            if (strstr (line, "design capacity:")) {
                l = line;
                while ((m = strsep (&l, delimiters)) != NULL) {
                    if (sscanf (m, "%d", &sysinfo->battery_design_capacity_mA) == 1)
                        break;
                }
            }
        }
    fclose (fp);
    }
}

void sysinfo_read_disk_stats (navlcm_system_info_t *s)
{
        struct statvfs vfs;
        char filename[256];
        sprintf (filename, "%s/.bashrc", getenv ("HOME"));
        if (statvfs (filename, &vfs) == 0) {
            s->disk_space_remaining_gb = 1.0 * vfs.f_bsize * vfs.f_bfree/1048576000;
        }
}

int sysinfo_read_sys_cpu_mem (navlcm_system_info_t *s)
{
    FILE *fp = fopen ("/proc/stat", "r");
    if (! fp) { return -1; }

    char buf[4096];
    char tmp[80];

    while (! feof (fp)) {
        if(!fgets (buf, sizeof (buf), fp)) {
            if(feof(fp))
                break;
            else
                return -1;
        }

        if (! strncmp (buf, "cpu ", 4)) {
            int cpu_user, cpu_user_low, cpu_system, cpu_idle;
            sscanf (buf, "%s %u %u %u %u", 
                    tmp,
                    &cpu_user,
                    &cpu_user_low,
                    &cpu_system,
                    &cpu_idle);
            int elapsed_jiffies = cpu_user - s->cpu_user + cpu_user_low - s->cpu_user_low + cpu_system - s->cpu_system + cpu_idle - s->cpu_idle;
            int loaded_jiffies = cpu_user - s->cpu_user + cpu_user_low - s->cpu_user_low + cpu_system - s->cpu_system;
            if (!elapsed_jiffies)
                s->cpu_usage = .0;
            else
                s->cpu_usage = 1.0 * loaded_jiffies / elapsed_jiffies;
            s->cpu_user = cpu_user;
            s->cpu_user_low = cpu_user_low;
            s->cpu_system = cpu_system;
            s->cpu_idle = cpu_idle;
            break;
        }
    }
    fclose (fp);

    fp = fopen ("/proc/meminfo", "r");
    if (! fp) { return -1; }
    while (! feof (fp)) {
        char units[10];
        memset (units,0,sizeof(units));
        if(!fgets (buf, sizeof (buf), fp)) {
            if(feof(fp))
                break;
            else
                return -1;
        }

        int64_t buffers, cached;

        if (! strncmp ("MemTotal:", buf, strlen ("MemTotal:"))) {
            sscanf (buf, "MemTotal: %ld %9s", &s->memtotal, units);
            s->memtotal *= 1024;
        } else if (! strncmp ("MemFree:", buf, strlen ("MemFree:"))) {
            sscanf (buf, "MemFree: %ld %9s", &s->memfree, units);
            s->memfree *= 1024;
        } else if (! strncmp ("Buffers:", buf, strlen ("Buffers:"))) {
            sscanf (buf, "Buffers: %ld %9s", &buffers, units);
            buffers *= 1024;
        } else if (! strncmp ("Cached:", buf, strlen ("Cached"))) {
            sscanf (buf, "Cached: %ld %9s", &cached, units);
            cached *= 1024;
        } else if (! strncmp ("SwapTotal:", buf, strlen("SwapTotal:"))) {
            sscanf (buf, "SwapTotal: %ld %9s", &s->swaptotal, units);
            s->swaptotal *= 1024;
        } else if (! strncmp ("SwapFree:", buf, strlen("SwapFree:"))) {
            sscanf (buf, "SwapFree: %ld %9s", &s->swapfree, units);
            s->swapfree *= 1024;
        } else {
            continue;
        }

        if (0 != strcmp (units, "kB")) {
            fprintf (stderr, "unknown units [%s] while reading "
                    "/proc/meminfo!!!\n", units);
        }

        if (!s->swaptotal)
            s->swap_usage = .0;
        else
            s->swap_usage = 1.0 - 1.0 *  s->swapfree / s->swaptotal;
        if (!s->memtotal)
            s->mem_usage = .0;
        else
            s->mem_usage = 1.0 - 1.0 *  ( s->memfree + buffers + cached) / s->memtotal;
    }

    fclose (fp);

    return 0;
}

