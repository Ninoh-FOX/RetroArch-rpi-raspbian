/*  RetroArch - A frontend for libretro.
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#if defined(__linux__)
#include <unistd.h>
#include <sys/syscall.h>
#endif

#include <file/file_path.h>
#include <file/config_file.h>
#include <streams/file_stream.h>
#include <string/stdstring.h>
#include <rthreads/rthreads.h>
#include <retro_dirent.h>

#include "pocket_menu.h"

#include "../configuration.h"
#include "../command.h"
#include "../file_path_special.h"
#include "../frontend/frontend_driver.h"
#include "../msg_hash.h"
#include "../msg_hash_lbl_str.h"
#include "../paths.h"
#include "../retroarch.h"
#include "../runloop.h"
#include "../verbosity.h"
#include "../gfx/gfx_thumbnail.h"
#include "menu_entries.h"
#include "menu_driver.h"
#ifdef HAVE_GFX_WIDGETS
#include "../gfx/gfx_widgets.h"
#endif
#ifdef HAVE_CHEEVOS
#include "../cheevos/cheevos_menu.h"
#endif

#define POCKET_CFG_FILE      "pocket_menu.cfg"
#define POCKET_PERF_EXT      ".pocket"

static struct
{
   bool     loaded;
   bool     enable;
   unsigned sense;
   unsigned perf;            /* profile in effect for the running content */
   bool     perf_applied;    /* a non-default profile was applied */
   int      perf_pending;    /* profile waiting for its frame, -1 none */
   unsigned perf_wait;       /* frames left before it is applied */
   /* The menu asks for entry values every frame it draws: answer from
    * memory. -2 = not read yet, -1 = nothing saved. */
   int      saved[2];        /* [0] core, [1] game */
   uint16_t slot_known;      /* bit per slot whose value is cached */
   char     slot_val[POCKET_STATE_SLOT_COUNT][24];
} pocket_st = { false, true, POCKET_SENSE_DEFAULT, POCKET_PERF_BALANCED, false,
   -1, 0, { -2, -2 }, 0 };

static void pocket_cache_reset(void)
{
   pocket_st.saved[0]   = -2;
   pocket_st.saved[1]   = -2;
   pocket_st.slot_known = 0;
}

/* ---------------------------------------------------------------- */
/* Preferences (pocket_menu.cfg in the config directory)             */
/* ---------------------------------------------------------------- */

static void pocket_cfg_path(char *s, size_t len)
{
   char dir[PATH_MAX_LENGTH];
   fill_pathname_application_special(dir, sizeof(dir),
         APPLICATION_SPECIAL_DIRECTORY_CONFIG);
   fill_pathname_join_special(s, dir, POCKET_CFG_FILE, len);
}

static void pocket_cfg_load(void)
{
   char path[PATH_MAX_LENGTH];
   config_file_t *conf;

   if (pocket_st.loaded)
      return;
   pocket_st.loaded = true;

   pocket_cfg_path(path, sizeof(path));
   if (!(conf = config_file_new_from_path_to_string(path)))
      return;

   {
      bool     b = true;
      unsigned u = POCKET_SENSE_DEFAULT;
      if (config_get_bool(conf, "pocket_menu_enable", &b))
         pocket_st.enable = b;
      if (config_get_uint(conf, "pocket_joypad_sense", &u) && u <= POCKET_SENSE_MAX)
         pocket_st.sense = u;
   }
   config_file_free(conf);
}

static void pocket_cfg_save(void)
{
   char path[PATH_MAX_LENGTH];
   config_file_t *conf;

   pocket_cfg_path(path, sizeof(path));
   if (!(conf = config_file_new_from_path_to_string(path)))
      if (!(conf = config_file_new_alloc()))
         return;

   config_set_string(conf, "pocket_menu_enable",
         pocket_st.enable ? "true" : "false");
   config_set_uint(conf, "pocket_joypad_sense", pocket_st.sense);
   if (!config_file_write(conf, path, true))
      RARCH_WARN("[Pocket] Cannot write \"%s\".\n", path);
   config_file_free(conf);
}

bool pocket_menu_active(void)
{
   pocket_cfg_load();
   return pocket_st.enable
       && !retroarch_ctl(RARCH_CTL_IS_DUMMY_CORE, NULL);
}

/* ---------------------------------------------------------------- */
/* Joypad Sensitivity                                                */
/* ---------------------------------------------------------------- */

unsigned pocket_menu_joypad_sense(void)
{
   pocket_cfg_load();
   return pocket_st.sense;
}

void pocket_menu_joypad_sense_set(unsigned frames)
{
   pocket_cfg_load();
   if (frames > POCKET_SENSE_MAX || frames == pocket_st.sense)
      return;
   pocket_st.sense = frames;
   pocket_cfg_save();
}

const char *pocket_menu_joypad_sense_label(unsigned frames)
{
   switch (frames)
   {
      case 0:  return msg_hash_to_str(MENU_ENUM_LABEL_VALUE_POCKET_SENSE_OFF);
      case 2:  return msg_hash_to_str(MENU_ENUM_LABEL_VALUE_POCKET_SENSE_SLOW);
      default: break;
   }
   return msg_hash_to_str(MENU_ENUM_LABEL_VALUE_POCKET_SENSE_FAST);
}

/* ---------------------------------------------------------------- */
/* Performance profile                                               */
/* ---------------------------------------------------------------- */

/* CPUs of the slowest cluster, read once from cpufreq (world-readable,
 * no root). 0 when the part is homogeneous or the files are missing,
 * in which case Power Saving only drops sustained performance. */
static uint64_t pocket_slow_cores_mask(void)
{
   static bool     probed = false;
   static uint64_t mask   = 0;
#if defined(__linux__)
   unsigned long freq[64];
   unsigned long lo = 0, hi = 0;
   long n;
   int  i;

   if (probed)
      return mask;
   probed = true;

   n = sysconf(_SC_NPROCESSORS_CONF);
   if (n <= 1)
      return 0;
   if (n > 64)
      n = 64;

   for (i = 0; i < n; i++)
   {
      char path[96];
      char buf[32] = {0};
      RFILE *f;

      freq[i] = 0;
      snprintf(path, sizeof(path),
            "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", i);
      if ((f = filestream_open(path, RETRO_VFS_FILE_ACCESS_READ,
                  RETRO_VFS_FILE_ACCESS_HINT_NONE)))
      {
         if (filestream_gets(f, buf, sizeof(buf)))
            freq[i] = strtoul(buf, NULL, 10);
         filestream_close(f);
      }
      if (!freq[i])
         return 0;
      if (!lo || freq[i] < lo)
         lo = freq[i];
      if (freq[i] > hi)
         hi = freq[i];
   }

   if (lo == hi)
      return 0;
   for (i = 0; i < n; i++)
      if (freq[i] == lo)
         mask |= (uint64_t)1 << i;
#else
   probed = true;
#endif
   return mask;
}

#if defined(__linux__)
/* The prototype every Linux libc uses, as rthreads.c spells it out */
extern long syscall(long number, ...);

/* Power Saving confines every thread of the process, the ones a core
 * starts for itself included: a multithreaded core (PS2, N64, PSP)
 * does its work outside the frame loop's thread. What each thread had
 * is kept, so leaving the profile gives it back. */
#define POCKET_TID_SLOTS 128

static struct
{
   long     tid;
   uint64_t mask;
} pocket_tids[POCKET_TID_SLOTS];
static unsigned pocket_tid_count;
static bool     pocket_confined;

static bool pocket_tid_affinity(long tid, uint64_t *get, uint64_t set)
{
   unsigned long bits[64 / (8 * sizeof(unsigned long))];
   unsigned i;

   memset(bits, 0, sizeof(bits));
   if (get)
   {
      *get = 0;
      if (syscall(__NR_sched_getaffinity, tid, sizeof(bits), bits) <= 0)
         return false;
      for (i = 0; i < 64; i++)
         if (bits[i / (8 * sizeof(unsigned long))]
               & (1UL << (i % (8 * sizeof(unsigned long)))))
            *get |= (uint64_t)1 << i;
      return true;
   }
   if (!set)
   {
      long n = sysconf(_SC_NPROCESSORS_CONF);
      for (i = 0; (long)i < n && i < 64; i++)
         set |= (uint64_t)1 << i;
   }
   for (i = 0; i < 64; i++)
      if (set & ((uint64_t)1 << i))
         bits[i / (8 * sizeof(unsigned long))] |=
            1UL << (i % (8 * sizeof(unsigned long)));
   return syscall(__NR_sched_setaffinity, tid, sizeof(bits), bits) == 0;
}

/* @mask confines every thread; 0 gives each back what it had, and all
 * CPUs to a thread started while confined. */
static bool pocket_process_affinity(uint64_t mask)
{
   struct RDIR *dir;
   bool ok = false;

   if (!mask && !pocket_confined)
      return true;
   if (!(dir = retro_opendir("/proc/self/task")))
      return false;
   if (mask && !pocket_confined)
      pocket_tid_count = 0;

   while (retro_readdir(dir))
   {
      long     tid  = strtol(retro_dirent_get_name(dir), NULL, 10);
      uint64_t prev = 0;
      unsigned i;

      if (tid <= 0)
         continue;
      for (i = 0; i < pocket_tid_count; i++)
         if (pocket_tids[i].tid == tid)
            break;
      if (mask)
      {
         if (     i == pocket_tid_count
               && i < POCKET_TID_SLOTS
               && !pocket_confined
               && pocket_tid_affinity(tid, &prev, 0))
         {
            pocket_tids[i].tid  = tid;
            pocket_tids[i].mask = prev;
            pocket_tid_count++;
         }
         ok |= pocket_tid_affinity(tid, NULL, mask);
      }
      else
         pocket_tid_affinity(tid, NULL,
               i < pocket_tid_count ? pocket_tids[i].mask : 0);
   }
   retro_closedir(dir);

   pocket_confined = mask && ok;
   if (!mask)
      pocket_tid_count = 0;
   return mask ? ok : true;
}
#endif

/* Runs on the thread that drives the frame loop: affinity applies to
 * the calling thread, and the menu and the content loader both run
 * there. */
static void pocket_perf_apply(unsigned profile)
{
   settings_t *settings = config_get_ptr();
   bool sustained       = settings ? settings->bools.sustained_performance_mode : false;
#ifdef HAVE_THREADS
   bool prefer_fast     = settings ? settings->bools.thread_prefer_fast_cores : false;
#endif

#if defined(__linux__)
   if (profile != POCKET_PERF_SAVER)
      pocket_process_affinity(0);
#endif

   switch (profile)
   {
      case POCKET_PERF_SAVER:
         {
            uint64_t slow = pocket_slow_cores_mask();
#if defined(__linux__)
            if (!slow || !pocket_process_affinity(slow))
#else
            if (!slow || !sthread_set_current_affinity(slow))
#endif
               sthread_set_current_affinity(0);
            sustained = false;
         }
         break;
      case POCKET_PERF_MAX:
         if (!sthread_prefer_fast_cores())
            sthread_set_current_affinity(0);
         sustained = true;
         break;
      default:
         /* RetroArch's own choice, as if no profile were set */
#ifdef HAVE_THREADS
         if (!(prefer_fast && sthread_prefer_fast_cores()))
#endif
            sthread_set_current_affinity(0);
         break;
   }

   frontend_driver_set_sustained_performance_mode(sustained);
   pocket_st.perf         = profile;
   pocket_st.perf_applied = (profile != POCKET_PERF_BALANCED);
   RARCH_LOG("[Pocket] Performance profile: %u.\n", profile);
}

unsigned pocket_menu_perf_get(void)
{
   /* the menu may open before a deferred profile is applied */
   if (pocket_st.perf_pending >= 0)
      return (unsigned)pocket_st.perf_pending;
   return pocket_st.perf;
}

const char *pocket_menu_perf_label(unsigned profile)
{
   switch (profile)
   {
      case POCKET_PERF_SAVER: return msg_hash_to_str(MENU_ENUM_LABEL_VALUE_POCKET_PERF_SAVER);
      case POCKET_PERF_MAX:   return msg_hash_to_str(MENU_ENUM_LABEL_VALUE_POCKET_PERF_MAX);
      default:                break;
   }
   return msg_hash_to_str(MENU_ENUM_LABEL_VALUE_POCKET_PERF_BALANCED);
}

void pocket_menu_perf_set(unsigned profile)
{
   if (profile >= POCKET_PERF_COUNT)
      return;
   pocket_st.perf_pending = -1;
   pocket_perf_apply(profile);
}

/* <config>/<core>/<core>.pocket for the core, <config>/<core>/<game>.pocket
 * for the game: the layout core and game overrides use. */
static bool pocket_perf_path(bool rom, char *s, size_t len, bool create)
{
   char dir[PATH_MAX_LENGTH];
   char core_dir[PATH_MAX_LENGTH];
   rarch_system_info_t *sys = &runloop_state_get_ptr()->system;
   const char *core_name    = sys ? sys->info.library_name : NULL;
   const char *name         = core_name;

   if (string_is_empty(core_name))
      return false;
   if (rom)
   {
      const char *base = path_get(RARCH_PATH_BASENAME);
      if (string_is_empty(base))
         return false;
      name = path_basename_nocompression(base);
      if (string_is_empty(name))
         return false;
   }

   fill_pathname_application_special(dir, sizeof(dir),
         APPLICATION_SPECIAL_DIRECTORY_CONFIG);
   fill_pathname_join_special(core_dir, dir, core_name, sizeof(core_dir));
   fill_pathname_join_special_ext(s, dir, core_name, name,
         POCKET_PERF_EXT, len);
   if (create && !path_is_directory(core_dir))
      path_mkdir(core_dir);
   return true;
}

static int pocket_perf_read(bool rom)
{
   char path[PATH_MAX_LENGTH];
   void *buf   = NULL;
   int64_t len = 0;
   int ret     = -1;

   if (     pocket_perf_path(rom, path, sizeof(path), false)
         && path_is_valid(path)
         && filestream_read_file(path, &buf, &len) && len > 0)
   {
      unsigned v = (unsigned)strtoul((const char*)buf, NULL, 10);
      if (v < POCKET_PERF_COUNT)
         ret = (int)v;
   }
   free(buf);
   return ret;
}

bool pocket_menu_perf_saved(bool rom, unsigned *profile)
{
   int *slot = &pocket_st.saved[rom ? 1 : 0];
   if (*slot == -2)
      *slot = pocket_perf_read(rom);
   if (*slot < 0 || !profile)
      return false;
   *profile = (unsigned)*slot;
   return true;
}

bool pocket_menu_perf_save(bool rom)
{
   char path[PATH_MAX_LENGTH];
   char data[8];
   char msg[256];
   int  n;
   bool ok          = false;
   unsigned profile = pocket_menu_perf_get();

   if (pocket_perf_path(rom, path, sizeof(path), true))
   {
      n  = snprintf(data, sizeof(data), "%u\n", profile);
      ok = n > 0 && filestream_write_file(path, data, n);
   }

   pocket_st.saved[rom ? 1 : 0] = ok ? (int)profile : -2;

   if (ok)
   {
      rarch_system_info_t *sys = &runloop_state_get_ptr()->system;
      const char *who          = rom
            ? path_basename_nocompression(path_get(RARCH_PATH_BASENAME))
            : sys->info.library_name;
      snprintf(msg, sizeof(msg), msg_hash_to_str(MSG_POCKET_PERF_SAVED),
            pocket_menu_perf_label(profile), who ? who : "");
   }
   else
      strlcpy(msg, msg_hash_to_str(MSG_POCKET_PERF_SAVE_FAILED), sizeof(msg));

   runloop_msg_queue_push(msg, strlen(msg), 1, 120, true, NULL,
         MESSAGE_QUEUE_ICON_DEFAULT, MESSAGE_QUEUE_CATEGORY_INFO);
   return ok;
}

bool pocket_menu_perf_clear(bool rom)
{
   char path[PATH_MAX_LENGTH];

   if (     pocket_perf_path(rom, path, sizeof(path), false)
         && path_is_valid(path)
         && filestream_delete(path) != 0)
      return false;
   pocket_st.saved[rom ? 1 : 0] = -1;
   return true;
}

void pocket_menu_content_loaded(void)
{
   unsigned profile = POCKET_PERF_BALANCED;

   pocket_cache_reset();
   if (!pocket_menu_active())
      return;
   /* the game's profile wins over the core's */
   if (     !pocket_menu_perf_saved(true, &profile)
         && !pocket_menu_perf_saved(false, &profile))
      profile = POCKET_PERF_BALANCED;

   /* Balanced with nothing applied before is what RetroArch already did
    * at init: no syscalls, no JNI round trip.
    *
    * Applied a couple of seconds into the game, not here: a thread
    * inherits the affinity of the one that creates it, and the video
    * and audio drivers - the Mali driver lazily, over its first frames
    * of work - start their threads after the core. Pinned here, those
    * would be squeezed onto the emulation's cores (two of them, on a
    * 6+2 part, for Maximum). */
   pocket_st.perf_pending = (profile != POCKET_PERF_BALANCED || pocket_st.perf_applied)
         ? (int)profile : -1;
   pocket_st.perf_wait    = 120;
}

void pocket_menu_frame(void)
{
   if (pocket_st.perf_pending >= 0 && !(pocket_st.perf_wait && --pocket_st.perf_wait))
   {
      unsigned profile       = (unsigned)pocket_st.perf_pending;
      pocket_st.perf_pending = -1;
      pocket_perf_apply(profile);
   }
}

void pocket_menu_content_unloaded(void)
{
   pocket_cache_reset();
   pocket_st.perf_pending = -1;
   if (pocket_st.perf_applied)
      pocket_perf_apply(POCKET_PERF_BALANCED);
}

/* ---------------------------------------------------------------- */
/* Save states                                                       */
/* ---------------------------------------------------------------- */

void pocket_menu_slots_invalidate(void)
{
   pocket_st.slot_known = 0;
}

void pocket_menu_slot_value(int slot, char *s, size_t len)
{
   char path[PATH_MAX_LENGTH];
   struct stat st;
   char *val;

   if (slot < 0 || slot >= POCKET_STATE_SLOT_COUNT)
   {
      strlcpy(s, msg_hash_to_str(MENU_ENUM_LABEL_VALUE_POCKET_SLOT_EMPTY), len);
      return;
   }

   val = pocket_st.slot_val[slot];
   if (!(pocket_st.slot_known & (1u << slot)))
   {
      if (     runloop_get_savestate_path(path, sizeof(path), slot)
            && stat(path, &st) == 0)
      {
         struct tm tm_buf;
#if defined(_WIN32)
         localtime_s(&tm_buf, &st.st_mtime);
#else
         localtime_r(&st.st_mtime, &tm_buf);
#endif
         strftime(val, sizeof(pocket_st.slot_val[0]), "%Y-%m-%d %H:%M", &tm_buf);
      }
      else
         strlcpy(val, msg_hash_to_str(MENU_ENUM_LABEL_VALUE_POCKET_SLOT_EMPTY),
               sizeof(pocket_st.slot_val[0]));
      pocket_st.slot_known |= (uint16_t)(1u << slot);
   }
   strlcpy(s, val, len);
}

int pocket_menu_slot_run(bool save, int slot)
{
   settings_t *settings = config_get_ptr();
   bool old_auto_index;
   bool ok;

   if (!settings || slot < 0 || slot >= POCKET_STATE_SLOT_COUNT)
      return -1;

   configuration_set_int(settings, settings->ints.state_slot, slot);
   /* the slot picked is the slot written, not the next auto index */
   old_auto_index                       = settings->bools.savestate_auto_index;
   settings->bools.savestate_auto_index = false;

   if (save)
   {
      runloop_state_t *runloop_st = runloop_state_get_ptr();
      bool old_thumbnail          = settings->bools.savestate_thumbnail_enable;
      char png[PATH_MAX_LENGTH];

      /* Menu drivers cache a slot's thumbnail by path: drop the old one
       * so the list shows the new capture. The state itself is left for
       * the core to overwrite, which keeps Undo Save State working. */
      gfx_savestate_thumbnail_get_path(png, sizeof(png),
            runloop_st->name.savestate, slot);
      if (path_is_valid(png))
         filestream_delete(png);

      settings->bools.savestate_thumbnail_enable = true;
      ok = command_event(CMD_EVENT_SAVE_STATE, NULL);
      settings->bools.savestate_thumbnail_enable = old_thumbnail;
   }
   else
      ok = command_event(CMD_EVENT_LOAD_STATE, NULL);

   settings->bools.savestate_auto_index = old_auto_index;
   pocket_st.slot_known                 = 0;

   if (!ok)
      return -1;

#if defined(HAVE_GFX_WIDGETS) && defined(HAVE_SCREENSHOTS)
   if (save && dispwidget_get_ptr()->active)
   {
      runloop_state_t *runloop_st = runloop_state_get_ptr();
      char png[PATH_MAX_LENGTH];
      char name[8];
      gfx_savestate_thumbnail_get_path(png, sizeof(png),
            runloop_st->name.savestate, slot);
      snprintf(name, sizeof(name), "%d", slot);
      gfx_widget_state_slot_show(dispwidget_get_ptr(), name, png);
   }
#endif

   /* One press: back to the game, as Resume does */
   command_event(CMD_EVENT_RESUME, NULL);
   return 0;
}

int pocket_menu_entry_state_slot(const char *entry_label, size_t i)
{
   if (     entry_label
         && string_is_equal(entry_label, MENU_ENUM_LABEL_POCKET_STATE_SLOT_STR)
         && i < POCKET_STATE_SLOT_COUNT)
      return (int)i;
   return -1;
}

bool pocket_menu_is_slot_list(const char *list_label)
{
   return list_label
      && (   string_is_equal(list_label, MENU_ENUM_LABEL_POCKET_SAVE_STATE_STR)
          || string_is_equal(list_label, MENU_ENUM_LABEL_POCKET_LOAD_STATE_STR));
}

bool pocket_menu_slot_list_open(void)
{
   const char *label = NULL;
   menu_entries_get_last_stack(NULL, &label, NULL, NULL, NULL);
   return pocket_menu_is_slot_list(label);
}

bool pocket_menu_slot_list_is_save(void)
{
   const char *label = NULL;
   menu_entries_get_last_stack(NULL, &label, NULL, NULL, NULL);
   return label && string_is_equal(label, MENU_ENUM_LABEL_POCKET_SAVE_STATE_STR);
}

bool pocket_menu_in_context(void)
{
   struct menu_state *menu_st = menu_state_get_ptr();
   file_list_t *stack         = menu_st && menu_st->entries.list
         ? MENU_LIST_GET(menu_st->entries.list, 0) : NULL;
   size_t i;

   if (!stack)
      return false;
   /* the stack is a handful of lists deep */
   for (i = stack->size; i-- > 0; )
      if (     stack->list[i].label
            && string_is_equal(stack->list[i].label, MENU_ENUM_LABEL_POCKET_MENU_STR))
         return true;
   return false;
}

bool pocket_menu_badge_list_open(void)
{
   const char *label = NULL;
   menu_entries_get_last_stack(NULL, &label, NULL, NULL, NULL);
   return label
       && string_is_equal(label, MENU_ENUM_LABEL_ACHIEVEMENT_LIST_STR)
       && pocket_menu_in_context();
}

/* ---------------------------------------------------------------- */
/* Selected badge preview                                            */
/* ---------------------------------------------------------------- */

/* A badge not downloaded yet is looked for again every this many
 * frames, this many times: the list fetches the set as it opens. */
#define POCKET_BADGE_RETRY_FRAMES 30
#define POCKET_BADGE_TRIES        60

static struct
{
   gfx_thumbnail_t thumb;
   size_t   selection;
   unsigned type;
   unsigned wait;
   unsigned tries;
   bool     visible;
} pocket_badge;

void pocket_menu_badge_preview_reset(void)
{
   pocket_badge.visible = false;
   pocket_badge.type    = 0;
   gfx_thumbnail_reset(&pocket_badge.thumb);
}

void pocket_menu_badge_preview_update(void)
{
#ifdef HAVE_CHEEVOS
   struct menu_state *menu_st = menu_state_get_ptr();
   file_list_t *list          = menu_st->entries.list
         ? MENU_LIST_GET_SELECTION(menu_st->entries.list, 0) : NULL;
   size_t selection           = menu_st->selection_ptr;
   unsigned type;

   if (     !list
         || selection >= list->size
         || !pocket_menu_badge_list_open())
   {
      if (pocket_badge.type)
         pocket_menu_badge_preview_reset();
      return;
   }

   type = list->list[selection].type;
   if (     type <  MENU_SETTINGS_CHEEVOS_START
         || type >= MENU_SETTINGS_NETPLAY_ROOMS_START)
   {
      if (pocket_badge.type)
         pocket_menu_badge_preview_reset();
      return;
   }

   if (type != pocket_badge.type || selection != pocket_badge.selection)
   {
      gfx_thumbnail_reset(&pocket_badge.thumb);
      pocket_badge.type      = type;
      pocket_badge.selection = selection;
      pocket_badge.wait      = 0;
      pocket_badge.tries     = 0;
   }
   pocket_badge.visible = true;

   if (     pocket_badge.thumb.status == GFX_THUMBNAIL_STATUS_MISSING
         && pocket_badge.tries < POCKET_BADGE_TRIES
         && !(pocket_badge.wait && --pocket_badge.wait))
      gfx_thumbnail_reset(&pocket_badge.thumb);

   if (pocket_badge.thumb.status == GFX_THUMBNAIL_STATUS_UNKNOWN)
   {
      char path[PATH_MAX_LENGTH];
      settings_t *settings = config_get_ptr();
      rcheevos_menu_get_badge_path(type - MENU_SETTINGS_CHEEVOS_START,
            path, sizeof(path));
      gfx_thumbnail_request_file(path, &pocket_badge.thumb,
            settings->uints.gfx_thumbnail_upscale_threshold);
      gfx_thumbnail_flush_requests();
      pocket_badge.wait = POCKET_BADGE_RETRY_FRAMES;
      pocket_badge.tries++;
   }
#endif
}

void pocket_menu_badge_preview_draw(void *userdata, unsigned video_dims,
      float right, float bottom)
{
   gfx_thumbnail_shadow_t shadow;
   unsigned w    = VIDEO_SCALE_W(video_dims);
   unsigned h    = VIDEO_SCALE_H(video_dims);
   unsigned size = ((w < h ? w : h) * 3) / 10;
   float margin  = (float)size / 12.0f;

   if (!pocket_badge.visible || !size)
      return;

   shadow.type          = GFX_THUMBNAIL_SHADOW_DROP;
   shadow.alpha         = 0.5f;
   shadow.drop.x_offset = margin / 2.0f;
   shadow.drop.y_offset = margin / 2.0f;
   shadow.outline.width = 0;

   gfx_thumbnail_draw(userdata, video_dims, &pocket_badge.thumb,
         right  - margin - (float)size,
         bottom - margin - (float)size,
         VIDEO_SCALE_PACK(size, size), GFX_THUMBNAIL_ALIGN_CENTRE,
         1.0f, 1.0f, &shadow);
}
