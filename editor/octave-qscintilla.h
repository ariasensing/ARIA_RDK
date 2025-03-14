////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2013-2025 The Octave Project Developers
//
// See the file COPYRIGHT.md in the top-level directory of this
// distribution or <https://octave.org/copyright/>.
//
// This file is part of Octave.
//
// Octave is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Octave is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Octave; see the file COPYING.  If not, see
// <https://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////
#include "editor_config.h"

#if ! defined (octave_octave_qscintilla_h)
#define octave_octave_qscintilla_h 1

#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QPointer>
#include <QTemporaryFile>
#include <Qsci/qsciscintilla.h>

#include "gui-settings.h"
#include "qt-interpreter-events.h"



class octave_qscintilla : public QsciScintilla
{
  Q_OBJECT

public:

  octave_qscintilla (QWidget *p);

  ~octave_qscintilla () = default;

  enum
  {
    ST_NONE = 0,
    ST_LINE_COMMENT,
    ST_BLOCK_COMMENT
  };

  virtual void contextMenuEvent (QContextMenuEvent *e);
  virtual void setCursorPosition (int line, int col);

  void context_help_doc (bool);
  void context_edit ();
  void context_run ();
  void get_global_textcursor_pos (QPoint *global_pos, QPoint *local_pos);
  bool get_actual_word ();
  void clear_selection_markers ();
  QString eol_string ();
  void get_current_position (int *pos, int *line, int *col);
  QStringList comment_string (bool comment = true);
  int get_style (int pos = -1);
  int is_style_comment (int pos = -1);
  void smart_indent (bool do_smart_indent, int do_auto_close,
                     int line, int ind_char_width);

  void smart_indent_line_or_selected_text (int lineFrom, int lineTo);

  void set_word_selection (const QString& word = QString ());

  void show_selection_markers (int l1, int c1, int l2, int c2);

  void set_selection_marker_color (const QColor& c);

  void replace_all (const QString& o_str, const QString& n_str,
                    bool re, bool cs, bool wo);

Q_SIGNALS:

  void update_rowcol_indicator_signal (int line, int col);
  void execute_command_in_terminal_signal (const QString&);
  void create_context_menu_signal (QMenu *);
  void context_menu_edit_signal (const QString&);
  void qsci_has_focus_signal (bool);
  void status_update (bool, bool);
  void show_doc_signal (const QString&);
  void show_symbol_tooltip_signal (const QPoint&, const QString&);
  void context_menu_break_condition_signal (int);
  void context_menu_break_once (int);
  void ctx_menu_run_finished_signal (int, QPointer<QTemporaryFile>,
                                     QPointer<QTemporaryFile>, bool, bool);
  void focus_console_after_command_signal ();

  void interpreter_event (const fcn_callback& fcn);
  void interpreter_event (const meth_callback& meth);

public Q_SLOTS:

  void handle_enter_debug_mode ();
  void handle_exit_debug_mode ();

private Q_SLOTS:

  void ctx_menu_run_finished (int, QPointer<QTemporaryFile>,
                              QPointer<QTemporaryFile>, bool, bool);

  void contextmenu_help (bool);
  void contextmenu_doc (bool);
  void contextmenu_help_doc (bool);
  void contextmenu_edit (bool);
  void contextmenu_run (bool);
  void contextmenu_run_temp_error ();

  void contextmenu_break_condition (bool);
  void contextmenu_break_once (const QPoint&);

  void text_changed ();
  void cursor_position_changed (int, int);

protected:

  void focusInEvent (QFocusEvent *focusEvent);

  void show_replace_action_tooltip ();

  bool event (QEvent *e);

  void keyPressEvent (QKeyEvent *e);

  void dragEnterEvent (QDragEnterEvent *e);

private:

  void auto_close (int auto_endif, int l,
                   const QString& line, QString& first_word);

  QPointer<QTemporaryFile> create_tmp_file (const QString& extension,
                                            const QString& contents);

  bool m_debug_mode;

  QString m_word_at_cursor;

  QString m_selection;
  QString m_selection_replacement;
  int m_selection_line;
  int m_selection_col;
  int m_indicator_id;
};

OCTAVE_END_NAMESPACE(octave)

#endif
