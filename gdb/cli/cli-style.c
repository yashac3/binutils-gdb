/* CLI colorizing

   Copyright (C) 2018-2019 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "defs.h"
#include "cli/cli-cmds.h"
#include "cli/cli-style.h"
#include "source-cache.h"
#include "observable.h"

/* True if styling is enabled.  */

#if defined (__MSDOS__) || defined (__CYGWIN__)
bool cli_styling = false;
#else
bool cli_styling = true;
#endif

/* True if source styling is enabled.  Note that this is only
   consulted when cli_styling is true.  */

bool source_styling = true;

/* Name of colors; must correspond to ui_file_style::basic_color.  */
static const char * const cli_colors[] = {
  "none",
  "black",
  "red",
  "green",
  "yellow",
  "blue",
  "magenta",
  "cyan",
  "white",
  nullptr
};

/* Names of intensities; must correspond to
   ui_file_style::intensity.  */
static const char * const cli_intensities[] = {
  "normal",
  "bold",
  "dim",
  nullptr
};

/* See cli-style.h.  */

cli_style_option file_name_style ("filename", ui_file_style::GREEN);

/* See cli-style.h.  */

cli_style_option function_name_style ("function", ui_file_style::YELLOW);

/* See cli-style.h.  */

cli_style_option variable_name_style ("variable", ui_file_style::CYAN);

/* See cli-style.h.  */

cli_style_option address_style ("address", ui_file_style::BLUE);

/* See cli-style.h.  */

cli_style_option highlight_style ("highlight", ui_file_style::RED);

/* See cli-style.h.  */

cli_style_option title_style ("title", ui_file_style::BOLD);

/* See cli-style.h.  */

cli_style_option metadata_style ("metadata", ui_file_style::DIM);

/* See cli-style.h.  */

cli_style_option::cli_style_option (const char *name,
				    ui_file_style::basic_color fg)
  : m_name (name),
    m_foreground (cli_colors[fg - ui_file_style::NONE]),
    m_background (cli_colors[0]),
    m_intensity (cli_intensities[ui_file_style::NORMAL])
{
}

/* See cli-style.h.  */

cli_style_option::cli_style_option (const char *name,
				    ui_file_style::intensity i)
  : m_name (name),
    m_foreground (cli_colors[0]),
    m_background (cli_colors[0]),
    m_intensity (cli_intensities[i])
{
}

/* Return the color number corresponding to COLOR.  */

static int
color_number (const char *color)
{
  for (int i = 0; i < ARRAY_SIZE (cli_colors); ++i)
    {
      if (color == cli_colors[i])
	return i - 1;
    }
  gdb_assert_not_reached ("color not found");
}

/* See cli-style.h.  */

ui_file_style
cli_style_option::style () const
{
  int fg = color_number (m_foreground);
  int bg = color_number (m_background);
  ui_file_style::intensity intensity = ui_file_style::NORMAL;

  for (int i = 0; i < ARRAY_SIZE (cli_intensities); ++i)
    {
      if (m_intensity == cli_intensities[i])
	{
	  intensity = (ui_file_style::intensity) i;
	  break;
	}
    }

  return ui_file_style (fg, bg, intensity);
}

/* Implements the cli_style_option::do_show_* functions.
   WHAT and VALUE are the property and value to show.
   The style for which WHAT is shown is retrieved from CMD context.  */

static void
do_show (const char *what, struct ui_file *file,
	 struct cmd_list_element *cmd,
	 const char *value)
{
  cli_style_option *cso = (cli_style_option *) get_cmd_context (cmd);
  fputs_filtered (_("The "), file);
  fprintf_styled (file, cso->style (), _("\"%s\" style"), cso->name ());
  fprintf_filtered (file, _(" %s is: %s\n"), what, value);
}

/* See cli-style.h.  */

void
cli_style_option::do_show_foreground (struct ui_file *file, int from_tty,
				      struct cmd_list_element *cmd,
				      const char *value)
{
  do_show (_("foreground color"), file, cmd, value);
}

/* See cli-style.h.  */

void
cli_style_option::do_show_background (struct ui_file *file, int from_tty,
				      struct cmd_list_element *cmd,
				      const char *value)
{
  do_show (_("background color"), file, cmd, value);
}

/* See cli-style.h.  */

void
cli_style_option::do_show_intensity (struct ui_file *file, int from_tty,
				     struct cmd_list_element *cmd,
				     const char *value)
{
  do_show (_("display intensity"), file, cmd, value);
}

/* See cli-style.h.  */

void
cli_style_option::add_setshow_commands (enum command_class theclass,
					const char *prefix_doc,
					struct cmd_list_element **set_list,
					void (*do_set) (const char *args,
							int from_tty),
					struct cmd_list_element **show_list,
					void (*do_show) (const char *args,
							 int from_tty))
{
  m_set_prefix = std::string ("set style ") + m_name + " ";
  m_show_prefix = std::string ("show style ") + m_name + " ";

  add_prefix_cmd (m_name, no_class, do_set, prefix_doc, &m_set_list,
		  m_set_prefix.c_str (), 0, set_list);
  add_prefix_cmd (m_name, no_class, do_show, prefix_doc, &m_show_list,
		  m_show_prefix.c_str (), 0, show_list);

  add_setshow_enum_cmd ("foreground", theclass, cli_colors,
			&m_foreground,
			_("Set the foreground color for this property."),
			_("Show the foreground color for this property."),
			nullptr,
			nullptr,
			do_show_foreground,
			&m_set_list, &m_show_list, (void *) this);
  add_setshow_enum_cmd ("background", theclass, cli_colors,
			&m_background,
			_("Set the background color for this property."),
			_("Show the background color for this property."),
			nullptr,
			nullptr,
			do_show_background,
			&m_set_list, &m_show_list, (void *) this);
  add_setshow_enum_cmd ("intensity", theclass, cli_intensities,
			&m_intensity,
			_("Set the display intensity for this property."),
			_("Show the display intensity for this property."),
			nullptr,
			nullptr,
			do_show_intensity,
			&m_set_list, &m_show_list, (void *) this);
}

static cmd_list_element *style_set_list;
static cmd_list_element *style_show_list;

static void
set_style (const char *arg, int from_tty)
{
  printf_unfiltered (_("\"set style\" must be followed "
		       "by an appropriate subcommand.\n"));
  help_list (style_set_list, "set style ", all_commands, gdb_stdout);
}

static void
show_style (const char *arg, int from_tty)
{
  cmd_show_list (style_show_list, from_tty, "");
}

static void
set_style_enabled  (const char *args, int from_tty, struct cmd_list_element *c)
{
  g_source_cache.clear ();
  gdb::observers::source_styling_changed.notify ();
}

static void
show_style_enabled (struct ui_file *file, int from_tty,
		    struct cmd_list_element *c, const char *value)
{
  if (cli_styling)
    fprintf_filtered (file, _("CLI output styling is enabled.\n"));
  else
    fprintf_filtered (file, _("CLI output styling is disabled.\n"));
}

static void
show_style_sources (struct ui_file *file, int from_tty,
		    struct cmd_list_element *c, const char *value)
{
  if (source_styling)
    fprintf_filtered (file, _("Source code styling is enabled.\n"));
  else
    fprintf_filtered (file, _("Source code styling is disabled.\n"));
}

/* Builds the "set style NAME " prefix.  */

static std::string
set_style_name (const char *name)
{
  std::string result ("set style ");

  result += name;
  result += " ";
  return result;
}

void
_initialize_cli_style ()
{
  add_prefix_cmd ("style", no_class, set_style, _("\
Style-specific settings.\n\
Configure various style-related variables, such as colors"),
		  &style_set_list, "set style ", 0, &setlist);
  add_prefix_cmd ("style", no_class, show_style, _("\
Style-specific settings.\n\
Configure various style-related variables, such as colors"),
		  &style_show_list, "show style ", 0, &showlist);

  add_setshow_boolean_cmd ("enabled", no_class, &cli_styling, _("\
Set whether CLI styling is enabled."), _("\
Show whether CLI is enabled."), _("\
If enabled, output to the terminal is styled."),
			   set_style_enabled, show_style_enabled,
			   &style_set_list, &style_show_list);

  add_setshow_boolean_cmd ("sources", no_class, &source_styling, _("\
Set whether source code styling is enabled."), _("\
Show whether source code styling is enabled."), _("\
If enabled, source code is styled.\n"
#ifdef HAVE_SOURCE_HIGHLIGHT
"Note that source styling only works if styling in general is enabled,\n\
see \"show style enabled\"."
#else
"Source highlighting is disabled in this installation of gdb, because\n\
it was not linked against GNU Source Highlight."
#endif
			   ), set_style_enabled, show_style_sources,
			   &style_set_list, &style_show_list);

#define STYLE_ADD_SETSHOW_COMMANDS(STYLE, PREFIX_DOC)	  \
  STYLE.add_setshow_commands (no_class, PREFIX_DOC,		\
			      &style_set_list,				\
			      [] (const char *args, int from_tty)	\
			      {						\
				help_list				\
				  (STYLE.set_list (),			\
				   set_style_name (STYLE.name ()).c_str (), \
				   all_commands,			\
				   gdb_stdout);				\
			      },					\
			      &style_show_list,				\
			      [] (const char *args, int from_tty)	\
			      {						\
				cmd_show_list				\
				  (STYLE.show_list (),			\
				   from_tty,				\
				   "");					\
			      })

  STYLE_ADD_SETSHOW_COMMANDS (file_name_style,
			      _("\
Filename display styling.\n\
Configure filename colors and display intensity."));

  STYLE_ADD_SETSHOW_COMMANDS (function_name_style,
			      _("\
Function name display styling.\n\
Configure function name colors and display intensity"));

  STYLE_ADD_SETSHOW_COMMANDS (variable_name_style,
			      _("\
Variable name display styling.\n\
Configure variable name colors and display intensity"));

  STYLE_ADD_SETSHOW_COMMANDS (address_style,
			      _("\
Address display styling.\n\
Configure address colors and display intensity"));

  STYLE_ADD_SETSHOW_COMMANDS (title_style,
			      _("\
Title display styling.\n\
Configure title colors and display intensity\n\
Some commands (such as \"apropos -v REGEXP\") use the title style to improve\n\
readability."));

  STYLE_ADD_SETSHOW_COMMANDS (highlight_style,
			      _("\
Highlight display styling.\n\
Configure highlight colors and display intensity\n\
Some commands use the highlight style to draw the attention to a part\n\
of their output."));

  STYLE_ADD_SETSHOW_COMMANDS (metadata_style,
			      _("\
Metadata display styling.\n\
Configure metadata colors and display intensity\n\
The \"metadata\" style is used when GDB displays information about\n\
your data, for example \"<unavailable>\""));
}
