/*
 * VLC-PS5 on the console. No jailbreak and no whitelist: everything lives in
 * the title's own folder, /app0, which a title can always write (as QUICK3 and
 * PS5_vkQuake do). Pieces from XPSemu (crash report, import report), QUICK3
 * (display mode, DualSense samples, audio port) and PS5SX2 (context layout).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "platform.h"

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#ifndef PLEX_BUILD_COMMIT
#define PLEX_BUILD_COMMIT "unknown-build"
#endif
#define APP_DIR "/app0"
#define LOG_PATH APP_DIR "/plexium.log"
/* Where the eboot is loaded, and a bound on its code for backtraces. */
#include "crash_termination.h"
extern unsigned plex_home_last_checkpoint(void);

int sceKernelSendNotificationRequest(int device, void *request, size_t size, int blocking);
int sceSystemServiceHideSplashScreen(void);
int sceSystemServiceLoadExec(const char *path, const char *const *argv);
int sceSystemServiceParamGetInt(int id, int *value);
int scePadInit(void);
int scePadOpen(int32_t user_id, int32_t port_type, int32_t index, const void *params);
int scePadRead(int32_t handle, void *samples, int32_t capacity);
int scePadClose(int32_t handle);
int scePadGetHandle(int32_t user_id, int32_t port_type, int32_t index);

int sceUserServiceInitialize(const void *params);
int sceUserServiceGetInitialUser(int32_t *user_id);
int sceUserServiceGetForegroundUser(int32_t *user_id);
int sceUserServiceGetLoginUserIdList(int32_t *users);
int sceAudioOutInit(void);
int sceAudioOutOpen(int32_t user_id, int32_t type, int32_t index, uint32_t len, uint32_t freq,
                    uint32_t param);
int sceAudioOutOutput(int32_t handle, const void *buf);
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_icdGetInstanceProcAddr(VkInstance instance,
                                                                  const char *name);

/* ---- start-up ---------------------------------------------------------- */

/* Launcher-provided title folder. Never change sandbox identity at startup. */
static char app_dir[256] = APP_DIR;
static bool full_access;

void plat_notify(const char *message)
{
    /* The kernel's 0xc30-byte request with the text at 0x2d (PS5SX2). */
    static uint8_t request[0xc30];
    memset(request, 0, sizeof(request));
    snprintf((char *)request + 0x2d, 1024, "%s", message);
    sceKernelSendNotificationRequest(0, request, sizeof(request), 0);
}

static int crash_fd = -1, crash_file_fd = -1;

static void write_str(const char *s)
{
    /* Its own descriptors, opened at start-up: the first console crash's report
     * never reached the log through descriptor 2. */
    if (crash_fd >= 0)
        (void)!write(crash_fd, s, strlen(s));
    if (crash_file_fd >= 0)
        (void)!write(crash_file_fd, s, strlen(s));
}

/* Fatal reporting must not acquire stdio locks, open files, inspect guessed
 * register offsets or scan a possibly corrupt stack. Descriptors are prepared
 * before handlers are installed. */
static void crash_number(const char *label, uint64_t value)
{
    char digits[17];
    static const char hex[] = "0123456789abcdef";
    for (int i=15; i>=0; --i) { digits[i]=hex[value&15]; value>>=4; }
    digits[16]=0;
    write_str(label);write_str(digits);write_str("\n");
}
static void exit_returned(void){write_str("*** fatal exit bridge returned; terminal wait, not application recovery\n");}
static void crash_handler(int sig, siginfo_t *info, void *context)
{
    static volatile sig_atomic_t in_handler;
    (void)context; // Native context layout has not been validated on this firmware.
    if (in_handler) crash_terminate(128+sig,_exit,exit_returned);
    in_handler=1;
    write_str("*** Plexium fatal signal; build " PLEX_BUILD_COMMIT "\n");
    crash_number("signal (hex): ",(uint64_t)sig);
    if(info) {
        crash_number("signal code (hex): ",(uint64_t)(int64_t)info->si_code);
        crash_number("fault address: ",(uintptr_t)info->si_addr);
    }
    crash_number("last Home checkpoint: ",plex_home_last_checkpoint());
    crash_terminate(128+sig,_exit,exit_returned);
}

#include "native_imports.h"
#include "startup_log.h"
/* Only the linked GOT decides whether an optional native import is resolved. */
/* settings.txt's "key=value" at start (read before full access: /app0 may
 * be gone after it). */
static int boot_setting(const char *key, int fallback)
{
    FILE *s = fopen(APP_DIR "/settings.txt", "r");
    if (!s)
        return fallback;
    char line[128];
    size_t n = strlen(key);
    int value = fallback;
    while (fgets(line, sizeof(line), s))
        if (!strncmp(line, key, n) && line[n] == '=')
            value = atoi(line + n + 1);
    fclose(s);
    return value;
}

/* The console's video decoder (ps5/modules/ps5vdec.c). Its library (sysmodule
 * 207) and compute queue are made before the Helper's full access: after it
 * the library is refused (0x80020063, console run 11) and the queue too
 * (0x811D0111, run 12). Run 10's "no jailbreak daemon answered" with the
 * library loaded first was the Helper not running then (run 12: it answered
 * in 60 ms). settings.txt vdec_load=0 loads it after full access instead,
 * hw_decode=0 not at all. */
int sceSysmoduleLoadModule(uint16_t id);
void vlc_ps5_vdec_prepare(void) __attribute__((weak));   /* ps5/modules/ps5vdec.c */
void vlc_ps5_vdec_probe(const char *when) __attribute__((weak));
#define SYSMODULE_VIDEODEC2 207
static bool vdec_ready;

static void load_video_decoder(void)
{
    /* (the decoder's own functions only get their addresses with the
     * library: they're checked after loading it, below) */
    if (!PLEX_NATIVE_IMPORT_READY(sceSysmoduleLoadModule)) {
        fprintf(stderr, "VLC-PS5: hardware decoding off: sceSysmoduleLoadModule unresolved\n");
        return;
    }
    int rc = sceSysmoduleLoadModule(SYSMODULE_VIDEODEC2);
    vdec_ready = rc == 0;
    if (vdec_ready) {
#define REQUIRE_DECODER_IMPORT(name) vdec_ready = vdec_ready && PLEX_NATIVE_IMPORT_READY(name)
        REQUIRE_DECODER_IMPORT(sceVideodec2QueryComputeMemoryInfo);
        REQUIRE_DECODER_IMPORT(sceVideodec2AllocateComputeQueue);
        REQUIRE_DECODER_IMPORT(sceVideodec2QueryDecoderMemoryInfo);
        REQUIRE_DECODER_IMPORT(sceVideodec2CreateDecoder);
        REQUIRE_DECODER_IMPORT(sceVideodec2DeleteDecoder);
        REQUIRE_DECODER_IMPORT(sceVideodec2Decode);
        REQUIRE_DECODER_IMPORT(sceVideodec2Flush);
        REQUIRE_DECODER_IMPORT(sceVideodec2Reset);
        REQUIRE_DECODER_IMPORT(sceVideodec2GetPictureInfo);
#undef REQUIRE_DECODER_IMPORT
    }
    fprintf(stderr, "Plexium: video decoder load=%#x imports=%s\n",
            (unsigned)rc, vdec_ready ? "ready" : "unavailable");
    /* its compute queue now too, while still in the sandbox */
    if (vdec_ready && vlc_ps5_vdec_prepare)
        vlc_ps5_vdec_prepare();
    if (vdec_ready && vlc_ps5_vdec_probe)
        vlc_ps5_vdec_probe(plat_full_access() ? "after full access" : "before full access");
}

int vlc_ps5_vdec_available(void)
{
    return vdec_ready;
}

/* Plexium does not request sandbox changes, edit helper allowlists or scan
 * other titles' installation folders. The launcher supplies /app0. */
uid_t __real_geteuid(void);
gid_t __real_getegid(void);

/* VLC's home, cache and temporary files, and the graphics driver's shader
 * caches: the app's folder (a title has no HOME, and /tmp isn't ours to
 * write). Set once our folder is known: with full access /app0 is gone, and
 * Mesa's cache then failed on it ("Failed to create /app0", run 19). The
 * shader caches make later starts skip compiling; they need no full access. */
static void use_app_dir(void)
{
    char path[300];
    setenv("HOME", app_dir, 1);
    snprintf(path, sizeof(path), "%s/cache", app_dir);
    mkdir(path, 0755);
    setenv("TMPDIR", path, 1);
    snprintf(path, sizeof(path), "%s/cache/shaders", app_dir);
    mkdir(path, 0755);
    setenv("PS5VK_SHADER_CACHE_DIR", path, 1);
    snprintf(path, sizeof(path), "%s/cache/mesa", app_dir);
    mkdir(path, 0755);
    setenv("MESA_SHADER_CACHE_DIR", path, 1);
    /* libbluray keeps its cache in $HOME/.cache/bluray and makes only the
     * last folder ("Error creating directory" in every disc's log) */
    snprintf(path, sizeof(path), "%s/.cache", app_dir);
    mkdir(path, 0755);
    fprintf(stderr, "VLC-PS5: shader caches in %s/cache\n", app_dir);
}

int plat_language(void)
{
    int lang = -1;
    /* parameter 1: the system language (English when the console doesn't
     * give the call) */
    if (!PLEX_NATIVE_IMPORT_READY(sceSystemServiceParamGetInt) || sceSystemServiceParamGetInt(1, &lang) != 0)
        return -1;
    return lang;
}

bool plat_full_access(void)
{
    return full_access;
}

/* The log is written by its own thread: unbuffered, one log line was many
 * small writes to the app's folder, and the main thread's lines (the stats
 * every 5 s while playing) froze the picture for 40-150 ms on the console.
 * Lines now collect in stdio's buffer and go to the file every 200 ms. */
static void *log_flusher(void *arg)
{
    (void)arg;
    for (;;) {
        usleep(200000);
        fflush(stderr);
    }
    return NULL;
}

void plat_init(void)
{
    rename(LOG_PATH, APP_DIR "/plexium.old.log");
    /* Append mode, like the crash report's descriptor: with "w" the lines still
     * buffered at a crash were written over the start of the report. */
    // A failed freopen closes stderr, making subsequent diagnostics undefined.
    // Open first; preserve the original stream/descriptor if the title is read-only.
    if (plex_startup_log_open(LOG_PATH)) {
        static char log_buffer[256 * 1024];
        setvbuf(stderr, log_buffer, _IOFBF, sizeof(log_buffer));
        pthread_t flusher;
        if (pthread_create(&flusher, NULL, log_flusher, NULL) == 0)
            pthread_detach(flusher);
        else
            setvbuf(stderr, NULL, _IONBF, 0);
        setvbuf(stdout, NULL, _IOLBF, 0);
    } else {
        fprintf(stderr, "Plexium: title log unavailable; retaining existing stderr\n");
    }
    fprintf(stderr,"start: build " PLEX_BUILD_COMMIT " pid=%d title=PPSA85421 runtime=locked-PS5_Vulkan; SIGSYS/SIGABRT tracked separately\n",(int)getpid());
    fprintf(stderr, "VLC-PS5: load address marker plat_init=%p\n", (void *)plat_init);
    crash_fd = open(LOG_PATH, O_WRONLY | O_APPEND);
    crash_file_fd = open(APP_DIR "/vlc-crash.txt", O_WRONLY | O_CREAT | O_TRUNC, 0600);

    static uint8_t alt_stack[64 * 1024] __attribute__((aligned(16)));
    stack_t ss = { .ss_sp = alt_stack, .ss_size = sizeof(alt_stack) };
    sigaltstack(&ss, NULL);
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = crash_handler;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&sa.sa_mask);
    static const int sigs[] = { SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT, SIGSYS };
    for (size_t i = 0; i < sizeof(sigs) / sizeof(sigs[0]); i++)
        sigaction(sigs[i], &sa, NULL);
    /* A dropped network connection must not end the app. */
    signal(SIGPIPE, SIG_IGN);

    fprintf(stderr, "Plexium: native startup; helper requests disabled\n");
    /* Retain the original decoder initialization order, without helper or
     * sandbox changes between phases. */
    int hw_decode = boot_setting("hw_decode", 1), vdec_load = boot_setting("vdec_load", 1);
    if (!hw_decode)
        fprintf(stderr, "VLC-PS5: hardware decoding off (settings.txt): decoder library not loaded\n");
    if (hw_decode && vdec_load == 1) {
        load_video_decoder();
    }
    full_access = __real_geteuid() == 0; // Observe launcher state; never change it.
    if (hw_decode && vdec_load != 1)
        load_video_decoder();
    else if (vdec_ready && vlc_ps5_vdec_probe && plat_full_access())
        vlc_ps5_vdec_probe("after full access");
    use_app_dir();
    struct stat permissions;
    // Use the resolved installation directory after sandbox lifting, and the
    // kernel IDs: Mesa's compatibility wrappers intentionally return real IDs.
    if (stat(app_dir, &permissions) == 0)
        fprintf(stderr, "install: euid=%u egid=%u app owner=%u:%u mode=%03o\n",
                (unsigned)__real_geteuid(), (unsigned)__real_getegid(),
                (unsigned)permissions.st_uid, (unsigned)permissions.st_gid,
                (unsigned)(permissions.st_mode & 0777));
    char executable_path[300];
    snprintf(executable_path, sizeof executable_path, "%s/eboot.bin", app_dir);
    if (stat(executable_path, &permissions) == 0)
        fprintf(stderr, "install: eboot mode=%03o\n", (unsigned)(permissions.st_mode & 0777));
}

const char *plat_data_dir(void)
{
    return app_dir;
}

const char *const *plat_media_roots(void)
{
    /* Tried in order; the ones a title can't open are skipped. /data's
     * folders open only with full access. */
    static const char *const roots[] = {
        "/mnt/usb0", "/mnt/usb1", "/mnt/usb2", "/mnt/usb3",
        "/mnt/usb4", "/mnt/usb5", "/mnt/usb6", "/mnt/usb7",
        "/mnt/ext0", "/mnt/ext1", "/data/plexium", "/data/media", NULL,
    };
    return roots;
}

void plat_hide_splash(void)
{
    fprintf(stderr, "VLC-PS5: splash hidden (%#x)\n", sceSystemServiceHideSplashScreen());
}

void plat_quit(void)
{
    fflush(NULL);
    sceSystemServiceLoadExec("exit", NULL);
    _exit(0);
}

/* A title sees the drives that were mounted when it started (a USB drive
 * plugged in later isn't in its view; one pulled out does vanish). Loading
 * our own eboot again starts a fresh process, as PS4 homebrew relaunches. */
void plat_restart(void)
{
    fprintf(stderr, "VLC-PS5: restarting to see new drives\n");
    fflush(NULL);
    int rc = sceSystemServiceLoadExec(APP_DIR "/eboot.bin", NULL);
    fprintf(stderr, "VLC-PS5: restart refused (%#x)\n", rc);
}

double plat_time(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

/* ---- DualSense ------------------------------------------------------------ */

#include "pad_ps5.h"

/* ---- the TV through VK_KHR_display (QUICK3's mode choice) ----------------- */

PFN_vkGetInstanceProcAddr plat_vk_loader(void)
{
    return (PFN_vkGetInstanceProcAddr)vk_icdGetInstanceProcAddr;
}

const char *const *plat_instance_extensions(uint32_t *count)
{
    static const char *const extensions[] = { VK_KHR_SURFACE_EXTENSION_NAME,
                                              VK_KHR_DISPLAY_EXTENSION_NAME };
    *count = 2;
    return extensions;
}

bool plat_create_surface(VkInstance instance, VkPhysicalDevice gpu, VkSurfaceKHR *surface,
                         uint32_t *width, uint32_t *height, uint32_t *refresh_mhz)
{
    uint32_t displays = 1;
    VkDisplayPropertiesKHR display;
    VkResult r = vkGetPhysicalDeviceDisplayPropertiesKHR(gpu, &displays, &display);
    if ((r != VK_SUCCESS && r != VK_INCOMPLETE) || displays == 0) {
        fprintf(stderr, "VLC-PS5: no display (%d)\n", r);
        return false;
    }
    uint32_t planes = 0;
    vkGetPhysicalDeviceDisplayPlanePropertiesKHR(gpu, &planes, NULL);
    VkDisplayPlanePropertiesKHR plane_props[16];
    planes = planes > 16 ? 16 : planes;
    vkGetPhysicalDeviceDisplayPlanePropertiesKHR(gpu, &planes, plane_props);
    uint32_t plane = UINT32_MAX;
    for (uint32_t i = 0; i < planes; i++) {
        if (plane_props[i].currentDisplay == VK_NULL_HANDLE ||
            plane_props[i].currentDisplay == display.display) {
            plane = i;
            break;
        }
    }
    if (plane == UINT32_MAX) {
        fprintf(stderr, "VLC-PS5: no plane for the display\n");
        return false;
    }
    uint32_t modes = 0;
    vkGetDisplayModePropertiesKHR(gpu, display.display, &modes, NULL);
    VkDisplayModePropertiesKHR props[32];
    modes = modes > 32 ? 32 : modes;
    vkGetDisplayModePropertiesKHR(gpu, display.display, &modes, props);
    if (modes == 0) {
        fprintf(stderr, "VLC-PS5: the display offers no mode\n");
        return false;
    }
    /* The largest mode at its 120 Hz (smooth menus; 24p films divide evenly).
     * Any mode the title sets makes the TV re-sync once at start, 60 Hz too
     * (tested), so the faster one. "refresh=60" in /app0/settings.txt picks 60. */
    uint32_t want_hz = 120;
    char settings_path[300];
    snprintf(settings_path, sizeof(settings_path), "%s/settings.txt", app_dir);
    FILE *settings = fopen(settings_path, "r");
    if (settings) {
        char line[128];
        while (fgets(line, sizeof(line), settings))
            sscanf(line, "refresh=%u", &want_hz);
        fclose(settings);
    }
    uint32_t best = 0;
    for (uint32_t i = 0; i < modes; i++) {
        const VkDisplayModeParametersKHR *p = &props[i].parameters;
        const VkDisplayModeParametersKHR *b = &props[best].parameters;
        fprintf(stderr, "VLC-PS5: mode %u: %ux%u at %.2f Hz\n", i, p->visibleRegion.width,
                p->visibleRegion.height, p->refreshRate / 1000.0);
        uint64_t ap = (uint64_t)p->visibleRegion.width * p->visibleRegion.height;
        uint64_t ab = (uint64_t)b->visibleRegion.width * b->visibleRegion.height;
        int dp = abs((int)p->refreshRate - (int)want_hz * 1000);
        int db = abs((int)b->refreshRate - (int)want_hz * 1000);
        if (ap > ab || (ap == ab && dp < db))
            best = i;
    }
    const VkDisplayModeParametersKHR *p = &props[best].parameters;
    *width = p->visibleRegion.width;
    *height = p->visibleRegion.height;
    *refresh_mhz = p->refreshRate;
    VkDisplaySurfaceCreateInfoKHR info = {
        .sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR,
        .displayMode = props[best].displayMode,
        .planeIndex = plane,
        .planeStackIndex = plane_props[plane].currentStackIndex,
        .transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .globalAlpha = 1.0f,
        .alphaMode = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR,
        .imageExtent = { *width, *height },
    };
    r = vkCreateDisplayPlaneSurfaceKHR(instance, &info, NULL, surface);
    fprintf(stderr, "VLC-PS5: display surface %ux%u at %.2f Hz, plane %u: %d\n", *width, *height,
            *refresh_mhz / 1000.0, plane, r);
    return r == VK_SUCCESS;
}

/* ---- sound ----------------------------------------------------------------- */

static int audio_port = -1;

bool plat_audio_open(void)
{
    int rc = sceAudioOutInit();
    /* System user, main port, 16-bit stereo (QUICK3, PS5_vkQuake, XPSemu). */
    audio_port = sceAudioOutOpen(0xff, 0, 0, AUDIO_GRAIN, AUDIO_RATE, 1);
    fprintf(stderr, "VLC-PS5: audio init %#x, port %#x\n", rc, audio_port);
    return audio_port >= 0;
}

void plat_audio_output(const int16_t *frames)
{
    if (audio_port >= 0)
        sceAudioOutOutput(audio_port, frames);
}

static int surround_port = -1;

bool plat_audio_open_surround(void)
{
    static bool tried;
    if (!tried) {
        tried = true;
        /* Format 2: 16-bit, 8 channels (L R C LFE Ls Rs Lb Rb). The system
         * mixes it down for a stereo TV. */
        surround_port = sceAudioOutOpen(0xff, 0, 0, AUDIO_GRAIN, AUDIO_RATE, 2);
        fprintf(stderr, "VLC-PS5: surround port %#x\n", surround_port);
    }
    return surround_port >= 0;
}

void plat_audio_output_surround(const int16_t *frames)
{
    if (surround_port >= 0)
        sceAudioOutOutput(surround_port, frames);
}

const char *plat_screenshot_path(uint64_t frame)
{
    (void)frame;
    return NULL;
}

bool plat_script_done(uint64_t frame)
{
    (void)frame;
    return false;
}
