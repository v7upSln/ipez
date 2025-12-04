#include <stdio.h>
#include <stdlib.h>
#include <sys/sysctl.h>
#include <sys/statfs.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/utsname.h>

#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>

#define FG(x) "\x1b[38;5;" #x "m"
#define RESET "\x1b[0m"

void get_sysctl_string(const char *name, char *buf, size_t size) {
    if (sysctlbyname(name, buf, &size, NULL, 0) != 0) {
        snprintf(buf, size, "Unknown");
    }
}

int64_t get_sysctl_int(const char *name) {
    int64_t val = 0;
    size_t size = sizeof(val);
    if (sysctlbyname(name, &val, &size, NULL, 0) != 0) return -1;
    return val;
}

int get_battery_percent() {
    io_registry_entry_t entry = IOServiceGetMatchingService(kIOMasterPortDefault,
                                IOServiceMatching("IOPMPowerSource"));
    if (!entry) return -1;

    CFMutableDictionaryRef props = NULL;
    if (IORegistryEntryCreateCFProperties(entry, &props, kCFAllocatorDefault, 0) != KERN_SUCCESS) {
        IOObjectRelease(entry);
        return -1;
    }

    CFNumberRef cap = CFDictionaryGetValue(props, CFSTR("CurrentCapacity"));
    CFNumberRef max = CFDictionaryGetValue(props, CFSTR("MaxCapacity"));

    int cur = 0, maxCap = 0;
    CFNumberGetValue(cap, kCFNumberIntType, &cur);
    CFNumberGetValue(max, kCFNumberIntType, &maxCap);

    CFRelease(props);
    IOObjectRelease(entry);

    if (maxCap == 0) return -1;
    return (cur * 100) / maxCap;
}

long get_uptime() {
    struct timeval boottime;
    size_t size = sizeof(boottime);
    int mib[2] = { CTL_KERN, KERN_BOOTTIME };

    if (sysctl(mib, 2, &boottime, &size, NULL, 0) != 0) return -1;

    return time(NULL) - boottime.tv_sec;
}

void format_uptime(long seconds, char *buf, size_t size) {
    int d = seconds / 86400; seconds %= 86400;
    int h = seconds / 3600;  seconds %= 3600;
    int m = seconds / 60;
    int s = seconds % 60;

    snprintf(buf, size, "%dd %dh %dm %ds", d, h, m, s);
}

int main() {

    printf(
FG(196) "                     ..'\n"
FG(40)  "                 ,xNMM.\n"
FG(226) "               .OMMMMo\n"
FG(33)  "               lMM\"\n"
FG(171) "     .;loddo:.  .olloddol;.\n"
FG(81)  "   cKMMMMMMMMMMNWMMMMMMMMMM0:\n"
FG(255) " .KMMMMMMMMMMMMMMMMMMMMMMMWd.\n"
FG(245) " XMMMMMMMMMMMMMMMMMMMMMMMX.\n"
FG(205) ";MMMMMMMMMMMMMMMMMMMMMMMM:\n"
FG(41)  ":MMMMMMMMMMMMMMMMMMMMMMMM:\n"
FG(226) " .MMMMMMMMMMMMMMMMMMMMMMMX.\n"
FG(27)  " kMMMMMMMMMMMMMMMMMMMMMMMMWd.\n"
FG(177) " 'XMMMMMMMMMMMMMMMMMMMMMMMMMMk\n"
FG(117) "  'XMMMMMMMMMMMMMMMMMMMMMMMMK.\n"
FG(231) "    kMMMMMMMMMMMMMMMMMMMMMMd\n"
FG(255) "     ;KMMMMMMMWXXWMMMMMMMk.\n"
FG(196) "       \"cooc*\"    \"*coo'\"\n"
RESET "\n"
    );

    char model[256];
    get_sysctl_string("hw.model", model, sizeof(model));

    struct utsname uts;
    uname(&uts);

    char ios_version[256];
    get_sysctl_string("kern.osproductversion", ios_version, sizeof(ios_version));

    char cpu_brand[256];
    get_sysctl_string("machdep.cpu.brand_string", cpu_brand, sizeof(cpu_brand));

    const char *cpu_arch = uts.machine;

    char gpu_name[256];
    get_sysctl_string("hw.gpu.name", gpu_name, sizeof(gpu_name));

    int battery = get_battery_percent();  // NEW

    long uptime_raw = get_uptime();
    char uptime_fmt[128];
    format_uptime(uptime_raw, uptime_fmt, sizeof(uptime_fmt));

    uint64_t memsize = get_sysctl_int("hw.memsize");
    double mem_gb = (double)memsize / (1024.0 * 1024.0 * 1024.0);

    struct statfs stats;
    statfs("/", &stats);
    double total_gb = (double)stats.f_blocks * stats.f_bsize / (1024*1024*1024);
    double free_gb  = (double)stats.f_bavail * stats.f_bsize / (1024*1024*1024);\




    printf("Device:      %s\n", model);
    printf("iOS Version: %s\n", ios_version);
    printf("Kernel:      %s\n", uts.release);
    printf("CPU:         %s\n", cpu_brand);
    printf("Arch:        %s\n", cpu_arch);
    printf("GPU:         %s\n", gpu_name);

    if (battery >= 0)
        printf("Battery:     %d%%\n", battery);
    else
        printf("Battery:     Unknown\n");

    printf("Echo:        v1.0.0\n");
    printf("Uptime:      %s\n", uptime_fmt);
    printf("RAM:         %.2f GB\n", mem_gb);
    printf("Storage:     %.2f GB total, %.2f GB free\n", total_gb, free_gb);

    printf("\n");
    return 0;
}
