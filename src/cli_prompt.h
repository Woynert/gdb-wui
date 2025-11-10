#ifndef CLI_PROMPT_H
#define CLI_PROMPT_H

#include "stdlib.h"
#include "stdarg.h"
#include "stdio.h"
#include "unistd.h"
#include "fcntl.h"
#include "termios.h"
#include "string.h"
#include "stdbool.h"
#include "strbuf.h"
#include "strbuf_extra.h"
#include "strview.h"

static struct termios orig_tio;

#define CLI_PROMPT_SIZE 256

typedef struct CliPrompt {
    union {
       strbuf_space_t(CLI_PROMPT_SIZE) _input;
       strbuf_t input;
    };
    uint _cursor;
    bool event_submit;
} CliPrompt;

void _CliPrompt_restore_terminal(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_tio);
}

void CliPrompt_setup(CliPrompt *cli_prompt) {
    // Enables "raw mode"
    struct termios tio;
    tcgetattr(STDIN_FILENO, &orig_tio);
    atexit(_CliPrompt_restore_terminal);

    tio = orig_tio;
    tio.c_lflag &= (uint)~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &tio);

    // Make STDIN non-blocking
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

    // Initialize structure
    *cli_prompt = (CliPrompt) { 0 };
    STRBUF_STATIC_INIT2(CLI_PROMPT_SIZE, cli_prompt->_input);
}

static void _CliPrompt_clear_line(void) {
    long wrote_bytes = write(STDOUT_FILENO, "\33[2K\r", 5);
    (void)wrote_bytes;
}

void _CliPrompt_refresh_draw(CliPrompt *prompt) {
   _CliPrompt_clear_line();
   printf("> %s", prompt->input.cstr);
   fflush(stdout);

   // update cursor position
   for (uint i = 0; i < (uint)prompt->input.size - prompt->_cursor; ++i) {
      long bw = write(STDOUT_FILENO, "\x1b[D", 3); (void)bw;
   }
}

/// @returns char position in string
/// @retval -1 Not found
static int _CliPrompt_string_find_char(char *str, int str_len, char ch, int start_index) {
   for (int i = start_index; i < str_len; ++i) {
      if (str[i] == ch) return i;
   }
   return -1;
}

/// @returns char position in string
/// @retval -1 Not found
static int _CliPrompt_string_find_last_char(char *str, int str_len, char ch) {
   for (int i = str_len; i > 0; --i) {
      if (str[i] == ch) return i;
   }
   return -1;
}

void CliPrompt_handle_prompt(CliPrompt *prompt) {

   strbuf_t *input = &prompt->input;
   uint cursor = prompt->_cursor;
   bool reposition_cursor = false;

   long unused;
   unsigned char ch;
   ssize_t n = read(STDIN_FILENO, &ch, 1);

   do {
      if (n <= 0) return;

      // Enter
      if (ch == '\n') {
         //_CliPrompt_clear_line();
         //printf("Command received: [%s]\n", input->cstr);
         //strbuf_assign(&input, cstr(""));
         //cursor = 0;
         //printf("> ");
         //fflush(stdout);
         prompt->event_submit = true;
         break;
      }

      // Visible ASCII
      if (ch >= 32 && ch <= 126) {
         if (input->size < input->capacity-1) {
            strbuf_insert_at_index_cstr(&input, (int)cursor, "-");
            input->cstr[cursor] = (char)ch;
            ++cursor;
            reposition_cursor = true;
         }
         break;
      }

      // Backspace
      if (ch == 0x7F || ch == 0x08) {
         if (cursor > 0) {
            strbuf_pop_at_index(&input, (int)cursor-1, 1);
            --cursor;
            reposition_cursor = true;
         }
         break;
      }

      // Ctrl+W
      if (ch == 0x17) {
         int erase_from = _CliPrompt_string_find_last_char(
               (char*)input->cstr, (int)(cursor) -1, ' '
               );
         if (erase_from < 0) erase_from = 0;
         int erase_length = (int)cursor - erase_from;

         strbuf_pop_at_index(&input, erase_from, erase_length);
         cursor -= (uint)erase_length;
         reposition_cursor = true;
         break;
      }

      // Escape sequences
      if (ch != 0x1B) {
         break;
      }
      unsigned char seq[5];
      ssize_t r = read(STDIN_FILENO, &seq[0], 1);
      if (r <= 0 || seq[0] != '[') {
         break;
      }
      if (read(STDIN_FILENO, &seq[1], 1) <= 0) {
         break;
      }
      switch (seq[1]) {
         case 'A': break; // Up
         case 'B': break; // Down
         case 'C':
             // Right
             if ((int)cursor < input->size) {
                ++cursor;
                unused = write(STDOUT_FILENO, "\x1b[C", 3);
             }
             break;
         case 'D':
             // Left
             if (cursor > 0) {
                --cursor;
                unused = write(STDOUT_FILENO, "\x1b[D", 3);
             }
             break;
         case '1':
             if (read(STDIN_FILENO, &seq[2], 3) != 3) { break; }
             // Shift + Right
             if (strncmp((char *)seq, (char *)"[1;5C", 5) == 0) {
                int next_white_space = _CliPrompt_string_find_char(
                      (char*)input->cstr, (int)input->size, ' ', (int)cursor +1
                      );
                if (next_white_space != -1) {
                   cursor = (uint)next_white_space;
                }
                else {
                   cursor = (uint)input->size;
                }
                reposition_cursor = true;
             }
             // Shift + Left
             else if (strncmp((char *)seq, (char *)"[1;5D", 5) == 0) {
                int next_white_space = _CliPrompt_string_find_last_char(
                      (char*)input->cstr, (int)(cursor) -1, ' '
                      );
                if (next_white_space != -1) {
                   cursor = (uint)next_white_space;
                }
                else {
                   cursor = 0;
                }
                reposition_cursor = true;
             }
             break;
         case '3':
             if (read(STDIN_FILENO, &seq[2], 1) != 1) { break; }
             // Delete
             if (strncmp((char *)seq, (char *)"[3~", 3) == 0) {
                if (cursor < (uint)input->size) {
                   strbuf_pop_at_index(&input, (int)cursor, 1);
                   reposition_cursor = true;
                }
             }
             break;
         default: break;
      }
   } while (0);

   prompt->_cursor = cursor;

   if (reposition_cursor) {
      _CliPrompt_refresh_draw(prompt);
   }
   (void)unused;
}

void CliPrompt_print_line(CliPrompt *prompt, const char *restrict format, ...) {
   _CliPrompt_clear_line();

   va_list args;
   va_start(args, format);
   vprintf(format, args);
   va_end(args);

   _CliPrompt_refresh_draw(prompt);
}

void CliPrompt_clear(CliPrompt *prompt) {
   {
      strbuf_t *tmp = &prompt->input;
      strbuf_assign(&tmp, cstr(""));
   }
   prompt->_cursor = 0;
   prompt->event_submit = false;
   _CliPrompt_refresh_draw(prompt);
}

#endif // !CLI_PROMPT_H
