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

#define APP_STATES(M)                                                          \
  M(APP_STATE_IDLE)                                                            \
  M(APP_STATE_ERROR)

#define EXPAND_STATE_IDS(_name) _name,
typedef enum { APP_STATES(EXPAND_STATE_IDS) } app_state_t;

typedef struct {
  app_state_t state;
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

static app_ctx_t s_app_ctx;

// *****************************************************************************
// Public code

void APP_Initialize(void) {
  s_app_ctx.state = APP_STATE_IDLE;
  SYS_CONSOLE_PRINT(
    "\n####################"
    "\n# winc-imager v%s (https://github.com/rdpoor/winc-imager)"
    "\n####################\n",
    WINC_IMAGER_VERSION
  );
}

void APP_Tasks(void) {
  switch (s_app_ctx.state) {
  case APP_STATE_IDLE: {
    // here on idle state.
    set_state(APP_STATE_IDLE);
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
    SYS_DEBUG_PRINT(SYS_ERROR_INFO, "%s => %s", state_name(s_app_ctx.state), state_name(state));
    s_app_ctx.state = state;
  }
}

static const char *state_name(app_state_t state) {
  SYS_ASSERT(state < N_STATES, "app_state_t out of bounds");
  return s_state_names[state];
}

// *****************************************************************************
// End of file
