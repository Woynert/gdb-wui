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

static struct termios orig_tio;

typedef struct {
    unsigned char input[256];
    uint input_len;
    uint cursor;
} CliPrompt;

void _CliPrompt_restore_terminal(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_tio);
}

void CliPrompt_setup(void) {
    // Enables "raw mode"
    struct termios tio;
    tcgetattr(STDIN_FILENO, &orig_tio);
    atexit(_CliPrompt_restore_terminal);

    tio = orig_tio;
    tio.c_lflag &= (uint)~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &tio);

    // Make STDIN non-blocking
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
}

static void CliPrompt_clear_line(void) {
    long wrote_bytes = write(STDOUT_FILENO, "\33[2K\r", 5);
    (void)wrote_bytes;
}

void _CliPrompt_refresh_draw(CliPrompt *prompt) {
   CliPrompt_clear_line();
   printf("> %.*s", prompt->input_len, prompt->input);
   fflush(stdout);

   // update cursor position
   for (uint i = 0; i < prompt->input_len - prompt->cursor; ++i) {
      long bw = write(STDOUT_FILENO, "\x1b[D", 3); (void)bw;
   }
}

/// @returns char position in string
/// @retval -1 Not found
static int string_find_char(char *str, int str_len, char ch, int start_index) {
   for (int i = start_index; i < str_len; ++i) {
      if (str[i] == ch) return i;
   }
   return -1;
}

/// @returns char position in string
/// @retval -1 Not found
static int string_find_char_backwards(char *str, int str_len, char ch) {
   for (int i = str_len; i > 0; --i) {
      if (str[i] == ch) return i;
   }
   return -1;
}

void CliPrompt_handle_prompt(CliPrompt *prompt) {

   unsigned char *input = prompt->input;
   uint input_len = prompt->input_len;
   uint cursor = prompt->cursor;
   uint max_size = sizeof(prompt->input);
   bool reposition_cursor = false;

   long unused;
   unsigned char ch;
   ssize_t n = read(STDIN_FILENO, &ch, 1);

   do {
      if (n <= 0) return;

      // Enter
      if (ch == '\n') {
         CliPrompt_clear_line();
         input[input_len] = '\0';
         printf("Command received: %s\n", input);
         input_len = 0;
         cursor = 0;
         printf("> ");
         fflush(stdout);
         break;
      }

      // Visible ASCII
      if (ch >= 32 && ch <= 126) {
         if (input_len < max_size-1) {
            memmove(&input[cursor+1], &input[cursor], input_len - cursor);

            input[cursor] = ch;
            ++(input_len);
            ++(cursor);
            reposition_cursor = true;
         }
         break;
      }

      // Backspace
      if (ch == 0x7F || ch == 0x08) {
         if (cursor > 0) {
            memmove(&input[cursor-1], &input[cursor], input_len - cursor);
            cursor--;
            input_len--;
            reposition_cursor = true;
         }
         break;
      }

      // Ctrl+W
      if (ch == 0x17) {
         int erase_from = string_find_char_backwards(
               (char*)input, (int)(cursor) -1, ' '
               );
         if (erase_from == -1) erase_from = 0;
         int erase_length = (int)cursor - erase_from;

         if (erase_length > 0) {
            memmove(&input[cursor -(uint)erase_length], &input[cursor], input_len - cursor);
            cursor -= (uint)erase_length;
            input_len -= (uint)erase_length;

            reposition_cursor = true;
         }
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
         case 'A': break;
         case 'B': break;
         case 'C':

             // Right
             if (cursor < input_len) {
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
             if (read(STDIN_FILENO, &seq[2], 3) != 3) {
                break;
             }

             // Shift + Right
             if (strncmp((char *)seq, (char *)"[1;5C", 5) == 0) {
                int next_white_space = string_find_char(
                      (char*)input, (int)input_len, ' ', (int)cursor +1
                      );
                if (next_white_space != -1) {
                   cursor = (uint)next_white_space;
                }
                else {
                   cursor = input_len;
                }
                reposition_cursor = true;
             }

             // Shift + Left
             else if (strncmp((char *)seq, (char *)"[1;5D", 5) == 0) {
                int next_white_space = string_find_char_backwards(
                      (char*)input, (int)(cursor) -1, ' '
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
             if (read(STDIN_FILENO, &seq[2], 1) != 1) {
                break;
             }

             // Delete
             if (strncmp((char *)seq, (char *)"[3~", 3) == 0) {
                if (cursor < input_len) {
                   memmove(&input[cursor], &input[cursor+1], input_len - cursor);
                   input_len--;
                   reposition_cursor = true;
                }
             }
             break;

         default:
             /*printf("Unknown escape: ESC [%c\n", seq[1]);*/
             break;
      }

   } while (0);

   prompt->input_len = input_len;
   prompt->cursor = cursor;

   if (reposition_cursor) {
      _CliPrompt_refresh_draw(prompt);
   }
   (void)unused;
}

void CliPrompt_print_line(CliPrompt *prompt, const char *restrict format, ...) {
   CliPrompt_clear_line();

   va_list args;
   va_start(args, format);
   vprintf(format, args);
   va_end(args);

   _CliPrompt_refresh_draw(prompt);
}

#endif // !CLI_PROMPT_H
