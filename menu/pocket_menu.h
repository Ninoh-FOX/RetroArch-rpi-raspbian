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

/* Pocket Menu: a compact in-game menu for handhelds, opened in place of
 * the Quick Menu while content runs. Its lists live on the ordinary menu
 * stack, so every menu driver draws them and Back pops them; the full
 * Quick Menu stays one entry away. Adapted from the Miyoo Menu of the
 * Miyoo Mini port, without its device-specific parts. */

#ifndef __POCKET_MENU_H
#define __POCKET_MENU_H

#include <stddef.h>
#include <stdint.h>
#include <boolean.h>
#include <retro_common_api.h>

RETRO_BEGIN_DECLS

#define POCKET_STATE_SLOT_COUNT 10

enum pocket_perf_profile
{
   POCKET_PERF_SAVER = 0,
   POCKET_PERF_BALANCED,
   POCKET_PERF_MAX,
   POCKET_PERF_COUNT
};

#define POCKET_SENSE_MAX     2
#define POCKET_SENSE_DEFAULT 1

/* True when the Pocket Menu should replace the Quick Menu: the feature
 * is on and content (not the dummy core) is running. */
bool pocket_menu_active(void);

/* Joypad Sensitivity: frames an action-button change waits so buttons
 * pressed together land on the same frame. 0 = off. Cached; persisted
 * to pocket_menu.cfg only when it changes. */
unsigned pocket_menu_joypad_sense(void);
void pocket_menu_joypad_sense_set(unsigned frames);
const char *pocket_menu_joypad_sense_label(unsigned frames);

/* Performance profile of the running content. */
unsigned pocket_menu_perf_get(void);
void pocket_menu_perf_set(unsigned profile);
const char *pocket_menu_perf_label(unsigned profile);
bool pocket_menu_perf_saved(bool rom, unsigned *profile);
bool pocket_menu_perf_save(bool rom);
/* Back to nothing saved: Start on the save entry */
bool pocket_menu_perf_clear(bool rom);

/* Content lifecycle: apply the saved profile on load, undo it on close. */
void pocket_menu_content_loaded(void);
void pocket_menu_content_unloaded(void);
/* Once per core frame: applies a profile deferred from content load. */
void pocket_menu_frame(void);

/* Save states. */
void pocket_menu_slots_invalidate(void);
void pocket_menu_slot_value(int slot, char *s, size_t len);
int  pocket_menu_slot_run(bool save, int slot);

/* For the menu drivers' savestate thumbnail code: the slot an entry of
 * a Pocket slot list stands for, or -1 when it is not one. */
int  pocket_menu_entry_state_slot(const char *entry_label, size_t i);
bool pocket_menu_is_slot_list(const char *list_label);

/* Pocket saves always carry a screenshot, so its slot lists show them
 * whatever Savestate Thumbnails is set to. */
bool pocket_menu_slot_list_open(void);
/* True while the menu shows the Pocket Menu or a list opened from it
 * (its stack holds the Pocket root): drivers draw those as an overlay
 * on the game, as the Miyoo Menu did. */
bool pocket_menu_in_context(void);
/* The slot list on top saves (true) or loads (false). */
bool pocket_menu_slot_list_is_save(void);
/* The achievement list opened from the Pocket Menu: drivers show the
 * selected badge large over the bottom right corner. */
bool pocket_menu_badge_list_open(void);
#define POCKET_SAVESTATE_THUMBNAILS(enabled) \
   ((enabled) || pocket_menu_slot_list_open())

/* That badge for the GPU drivers (RGUI draws its own): update loads it
 * from render(), draw puts it in the corner whose bottom right is
 * (right, bottom) from frame(), after the list text; reset drops the
 * texture, on context destroy and free. */
void pocket_menu_badge_preview_update(void);
void pocket_menu_badge_preview_draw(void *userdata, unsigned video_dims,
      float right, float bottom);
void pocket_menu_badge_preview_reset(void);

RETRO_END_DECLS

#endif
