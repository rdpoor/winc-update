/**
 * @file app.c
 *
 * MIT License
 *
 * Copyright (c) 2022 R. D. Poor (https://github.com/rdpoor)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

// *****************************************************************************
// Includes

#include "app.h"

#include "definitions.h"
#include <stdbool.h>

// *****************************************************************************
// Private types and definitions

#define SD_DEVICE_NAME "/dev/mmcblka1"
#define SD_MOUNT_NAME "/mnt/mydrive"

#define APP_STATES(M)                                                          \
  M(APP_STATE_IDLE)                                                            \
  M(APP_STATE_AWAIT_FILESYSTEM)                                                \
  M(APP_STATE_OPENING_DIRECTORY)                                               \
  M(APP_STATE_READING_DIRECTORY)                                               \
  M(APP_STATE_CLOSING_DIRECTORY)                                               \
  M(APP_STATE_COMPLETE)                                                        \
  M(APP_STATE_ERROR)

#define EXPAND_STATE_IDS(_name) _name,
typedef enum { APP_STATES(EXPAND_STATE_IDS) } app_state_t;

typedef struct {
  app_state_t state;
  uint32_t mount_retries;
  SYS_FS_HANDLE dir_handle;
} app_ctx_t;

// *****************************************************************************
// Private (static, forward) declarations

/**
 * @brief Set the internal state.
 */
static void set_state(app_state_t state);

/**
 * @brief Return the name of the given state.
 */
static const char *state_name(app_state_t state);

// *****************************************************************************
// Private (static) storage

#define EXPAND_STATE_NAMES(_name) #_name,
static const char *s_state_names[] = {APP_STATES(EXPAND_STATE_NAMES)};

#define N_STATES (sizeof(s_state_names) / sizeof(s_state_names[0]))

char s_filename[100];    // to hold long file names

static app_ctx_t s_app_ctx;

// *****************************************************************************
// Public code

void APP_Initialize(void) {
  s_app_ctx.state = APP_STATE_IDLE;
  s_app_ctx.mount_retries = 0;
  s_app_ctx.dir_handle = SYS_FS_HANDLE_INVALID;
  SYS_CONSOLE_PRINT(
      "\n####################"
      "\n# winc-imager v%s (https://github.com/rdpoor/winc-imager)"
      "\n####################\n",
      WINC_IMAGER_VERSION);
}

void APP_Tasks(void) {
  switch (s_app_ctx.state) {
  case APP_STATE_IDLE: {
    // here on idle state.
    set_state(APP_STATE_AWAIT_FILESYSTEM);
  } break;

  case APP_STATE_AWAIT_FILESYSTEM: {
    // Waiting for file system to complete mounting.
    s_app_ctx.mount_retries += 1;
    if (SYS_FS_Mount(SD_DEVICE_NAME, SD_MOUNT_NAME, FAT, 0, NULL) ==
        SYS_FS_RES_SUCCESS) {
      // file system mounted.
      SYS_DEBUG_PRINT(SYS_ERROR_DEBUG,
                      "\nSD card mounted after %ld attempts",
                      s_app_ctx.mount_retries);
      // Set current drive so that we do not have to use absolute path.
      if (SYS_FS_CurrentDriveSet(SD_MOUNT_NAME) == SYS_FS_RES_FAILURE) {
        SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                        "\nUnable to select drive, error %d",
                        SYS_FS_Error());
        set_state(APP_STATE_ERROR);
      } else {
        set_state(APP_STATE_OPENING_DIRECTORY);
      }

    } else if (s_app_ctx.mount_retries % 100000 == 0) {
      // still waiting for file system to mount...
      SYS_DEBUG_PRINT(SYS_ERROR_INFO,
                      "\nSD card not ready after %ld attempts",
                      s_app_ctx.mount_retries);
    }
  } break;

  case APP_STATE_OPENING_DIRECTORY: {
    s_app_ctx.dir_handle = SYS_FS_DirOpen(SD_MOUNT_NAME "/");
    if (s_app_ctx.dir_handle != SYS_FS_HANDLE_INVALID) {
      SYS_CONSOLE_MESSAGE("\nsize (bytes) filename");
      set_state(APP_STATE_READING_DIRECTORY);
    } else {
      SYS_DEBUG_PRINT(
          SYS_ERROR_ERROR, "\nUnable to open directory %s", SD_MOUNT_NAME "/");
      set_state(APP_STATE_ERROR);
    }
  } break;

  case APP_STATE_READING_DIRECTORY: {
    // remain in this state until all directory entries are read and printed
    SYS_FS_FSTAT stat;

    stat.lfname = s_filename;
    stat.lfsize = sizeof(s_filename);
    if (SYS_FS_DirRead(s_app_ctx.dir_handle, &stat) == SYS_FS_RES_FAILURE) {
      SYS_DEBUG_PRINT(
          SYS_ERROR_ERROR, "\nUnable to read directory %s", SD_MOUNT_NAME "/");
      set_state(APP_STATE_ERROR);

    } else if ((stat.lfname[0] == '\0') && (stat.fname[0] == '\0')) {
      SYS_CONSOLE_MESSAGE("\nDirectory listing complete");
      set_state(APP_STATE_CLOSING_DIRECTORY);
    } else {
      // read succeeded.  Print entry and read next.
      SYS_CONSOLE_PRINT("\n%12ld %s", stat.fsize, stat.fname);
    }
  } break;

  case APP_STATE_CLOSING_DIRECTORY: {
    if (SYS_FS_DirClose(s_app_ctx.dir_handle) != SYS_FS_RES_SUCCESS) {
      SYS_DEBUG_PRINT(
          SYS_ERROR_ERROR, "\nClosing directory %s failed", SD_MOUNT_NAME "/");
    }
    s_app_ctx.dir_handle = SYS_FS_HANDLE_INVALID;
    set_state(APP_STATE_COMPLETE);
  } break;

  case APP_STATE_COMPLETE: {
    // here upon successful completion.
  } break;

  case APP_STATE_ERROR: {
    // here on error state
  } break;

  } // switch
}

// *****************************************************************************
// Private (static) code

static void set_state(app_state_t state) {
  if (s_app_ctx.state != state) {
    SYS_DEBUG_PRINT(SYS_ERROR_DEBUG,
                    "%s => %s",
                    state_name(s_app_ctx.state),
                    state_name(state));
    s_app_ctx.state = state;
  }
}

static const char *state_name(app_state_t state) {
  SYS_ASSERT(state < N_STATES, "app_state_t out of bounds");
  return s_state_names[state];
}

// *****************************************************************************
// End of file
