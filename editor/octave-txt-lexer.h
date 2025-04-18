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

#if ! defined (octave_octave_txt_lexer_h)
#define octave_octave_txt_lexer_h 1

#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexer.h>

OCTAVE_BEGIN_NAMESPACE(octave)

class octave_txt_lexer : public QsciLexer
{
  Q_OBJECT

public:

  virtual const char * language () const;

  virtual const char * lexer () const;

  virtual QString description (int style) const;
};

OCTAVE_END_NAMESPACE(octave)

#endif
