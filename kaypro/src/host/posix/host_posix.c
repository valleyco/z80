#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

#include "kaypro_host.h"

#define POSIX_DISPLAY_MIN_MS 50
#define POSIX_QUIT_BYTE 0x1C /* Ctrl-\ */

static bool display_cleared;
static bool termios_saved;
static bool quit_requested;
static bool prev_was_cr;
static struct termios termios_original;
static struct timeval display_last_paint;

static void posix_restore_terminal(void) {
  if (termios_saved) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios_original);
    termios_saved = false;
  }
  fputs("\033[?25h", stdout);
  fflush(stdout);
}

/* stdin/stdout/stderr often share one open-file description on a PTY. Setting
 * O_NONBLOCK on stdin therefore makes stdout nonblocking too; full-frame ANSI
 * paints then get EAGAIN, stdio errors out, and the screen freezes while the
 * guest still runs. Use termios VMIN/VTIME (or poll) instead. */
static void posix_ensure_stdout_blocking(void) {
  int flags = fcntl(STDOUT_FILENO, F_GETFL, 0);
  if (flags >= 0 && (flags & O_NONBLOCK)) {
    fcntl(STDOUT_FILENO, F_SETFL, flags & ~O_NONBLOCK);
  }
  clearerr(stdout);
}

static void posix_setup_terminal(void) {
  if (!isatty(STDIN_FILENO)) return;
  if (tcgetattr(STDIN_FILENO, &termios_original) != 0) return;
  termios_saved = true;
  atexit(posix_restore_terminal);

  struct termios raw = termios_original;
  raw.c_lflag &= (tcflag_t) ~(ICANON | ECHO | ISIG);
  raw.c_iflag &= (tcflag_t)~IXON;
  /* Non-blocking-style read without O_NONBLOCK: return immediately if empty. */
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
  posix_ensure_stdout_blocking();
}

static void posix_console_write(void *ctx, const uint8_t *data, size_t len) {
  (void)ctx;
  if (!data || len == 0) return;
  fwrite(data, 1, len, stdout);
  fflush(stdout);
}

static int posix_console_poll(void *ctx) {
  (void)ctx;

  struct pollfd pfd = {.fd = STDIN_FILENO, .events = POLLIN, .revents = 0};
  int pr = poll(&pfd, 1, 0);
  if (pr <= 0) return -1;
  if (!(pfd.revents & (POLLIN | POLLHUP))) return -1;

  unsigned char byte = 0;
  ssize_t n = read(STDIN_FILENO, &byte, 1);
  if (n < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) return -1;
    return -1;
  }
  if (n == 0) return -1;

  if (byte == POSIX_QUIT_BYTE) {
    quit_requested = true;
    return -1;
  }

  byte = (unsigned char)(byte & 0x7F);
  /* CP/M / MBASIC terminate lines on CR. Many terminals send LF or CRLF. */
  if (byte == '\n') {
    if (prev_was_cr) {
      prev_was_cr = false;
      return -1; /* drop LF of CRLF */
    }
    prev_was_cr = false;
    return '\r';
  }
  prev_was_cr = (byte == '\r');
  return (int)byte;
}

static void posix_log(void *ctx, const char *msg) {
  (void)ctx;
  if (msg) fprintf(stderr, "%s\n", msg);
}

static long posix_elapsed_ms(const struct timeval *then, const struct timeval *now) {
  return (now->tv_sec - then->tv_sec) * 1000L +
         (now->tv_usec - then->tv_usec) / 1000L;
}

static uint8_t posix_glyph(uint8_t c) {
  if (c >= 0x20 && c < 0x7F) return c;
  return (uint8_t)' ';
}

static bool posix_display_refresh(void *ctx, const uint8_t *cells, unsigned cols,
                                  unsigned rows, unsigned cursor_col,
                                  unsigned cursor_row) {
  (void)ctx;
  if (!cells || cols == 0 || rows == 0) return true;

  struct timeval now;
  gettimeofday(&now, NULL);
  if (display_cleared &&
      posix_elapsed_ms(&display_last_paint, &now) < POSIX_DISPLAY_MIN_MS) {
    return false; /* keep dirty so the latest frame is painted later */
  }

  posix_ensure_stdout_blocking();
  clearerr(stdout);

  if (!display_cleared) {
    fputs("\033[2J", stdout);
    display_cleared = true;
  }

  /* Paint rows with absolute CUP so we do not scroll past the grid. */
  for (unsigned r = 0; r < rows; r++) {
    fprintf(stdout, "\033[%u;1H", r + 1);
    for (unsigned c = 0; c < cols; c++) {
      uint8_t ch = posix_glyph(cells[r * cols + c]);
      fputc((int)ch, stdout);
    }
  }

  if (cursor_col >= cols) cursor_col = cols - 1;
  if (cursor_row >= rows) cursor_row = rows - 1;
  fprintf(stdout, "\033[%u;%uH\033[?25h", cursor_row + 1, cursor_col + 1);
  if (fflush(stdout) != 0 || ferror(stdout)) {
    clearerr(stdout);
    return false; /* retry later; do not clear dirty */
  }
  display_last_paint = now;
  return true;
}

bool kaypro_host_posix_quit_requested(void) { return quit_requested; }

void kaypro_host_posix_install(kaypro_t *m) {
  quit_requested = false;
  prev_was_cr = false;
  display_cleared = false;
  memset(&display_last_paint, 0, sizeof(display_last_paint));
  posix_setup_terminal();
  posix_ensure_stdout_blocking();

  kaypro_host_ops_t ops = {
      .console_write = posix_console_write,
      .console_poll = posix_console_poll,
      .display_refresh = posix_display_refresh,
      .log = posix_log,
      .ctx = NULL,
  };
  kaypro_set_host(m, &ops);
}

static uint8_t *posix_read_file(const char *path, size_t *out_size) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return NULL;
  }
  long sz = ftell(f);
  if (sz <= 0) {
    fclose(f);
    return NULL;
  }
  if (fseek(f, 0, SEEK_SET) != 0) {
    fclose(f);
    return NULL;
  }
  uint8_t *buf = (uint8_t *)malloc((size_t)sz);
  if (!buf) {
    fclose(f);
    return NULL;
  }
  if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
    fclose(f);
    free(buf);
    return NULL;
  }
  fclose(f);
  *out_size = (size_t)sz;
  return buf;
}

bool kaypro_load_rom(kaypro_t *m, const char *path) {
  size_t size = 0;
  uint8_t *data = posix_read_file(path, &size);
  if (!data) return false;
  bool ok = kaypro_load_rom_bytes(m, data, size);
  free(data);
  return ok;
}

bool kaypro_attach_disk(kaypro_t *m, int drive, const char *path) {
  size_t size = 0;
  uint8_t *data = posix_read_file(path, &size);
  if (!data) return false;
  /* FDC takes ownership of data. */
  if (!kaypro_attach_disk_mem(m, drive, data, size)) {
    free(data);
    return false;
  }
  return true;
}
