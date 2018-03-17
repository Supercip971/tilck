
/*
 * This is a DEMO/DEBUG version of the tty driver.
 * Until the concept of character devices is implemented in exOS, that's
 * good enough for basic experiments.
 *
 * Useful info:
 * http://www.linusakesson.net/programming/tty/index.php
 */

#include <term.h>
#include <string_util.h>
#include <paging.h>
#include <arch/generic_x86/x86_utils.h>
#include <serial.h>

void video_scroll_up(u32 lines);
void video_scroll_down(u32 lines);
bool video_is_at_bottom(void);
void video_scroll_to_bottom(void);
void video_add_row_and_scroll(void);
void video_clear_row(int row_num);
void video_set_char_at(char c, u8 color, int row, int col);
void video_move_cursor(int row, int col);
void video_enable_cursor(void);
void video_disable_cursor(void);

u8 term_width = 80;
u8 term_height = 25;

u8 terminal_row;
u8 terminal_column;
u8 terminal_color;

void term_scroll_up(u32 lines)
{
   video_scroll_up(lines);

   if (!video_is_at_bottom()) {
      video_disable_cursor();
   } else {
      video_enable_cursor();
      video_move_cursor(terminal_row, terminal_column);
   }
}

void term_scroll_down(u32 lines)
{
   video_scroll_down(lines);

   if (video_is_at_bottom()) {
      video_enable_cursor();
      video_move_cursor(terminal_row, terminal_column);
   }
}

void term_setcolor(u8 color) {
   terminal_color = color;
}


static void term_incr_row()
{
   if (terminal_row < term_height - 1) {
      ++terminal_row;
      return;
   }

   video_add_row_and_scroll();
}

void term_write_char_unsafe(char c)
{
   write_serial(c);
   video_scroll_to_bottom();
   video_enable_cursor();

   if (c == '\n') {
      terminal_column = 0;
      term_incr_row();
      video_move_cursor(terminal_row, terminal_column);
      return;
   }

   if (c == '\r') {
      terminal_column = 0;
      video_move_cursor(terminal_row, terminal_column);
      return;
   }

   if (c == '\t') {
      return;
   }

   if (c == '\b') {
      if (terminal_column > 0) {
         terminal_column--;
      }

      video_set_char_at(' ', terminal_color, terminal_row, terminal_column);
      video_move_cursor(terminal_row, terminal_column);
      return;
   }

   video_set_char_at(c, terminal_color, terminal_row, terminal_column);
   ++terminal_column;

   if (terminal_column == term_width) {
      terminal_column = 0;
      term_incr_row();
   }

   video_move_cursor(terminal_row, terminal_column);
}

void term_write_char(char c)
{
   disable_interrupts();
   term_write_char_unsafe(c);
   enable_interrupts();
}

void term_write_string(const char *str)
{
   disable_interrupts();

   while (*str) {
      term_write_char_unsafe(*str++);
   }

   enable_interrupts();
}

void term_move_ch(int row, int col)
{
   terminal_row = row;
   terminal_column = col;
   video_move_cursor(row, col);
}

void term_init()
{
   term_move_ch(0, 0);
   term_setcolor(make_color(COLOR_WHITE, COLOR_BLACK));

   for (int i = 0; i < term_height; i++)
      video_clear_row(i);

   init_serial_port();
}
