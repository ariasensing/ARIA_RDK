////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2019-2025 The Octave Project Developers
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

#if defined (HAVE_CONFIG_H)
#  include "config.h"
#endif

#include <algorithm>
#include <array>
#include <string>

#include <cmath>

#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QComboBox>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QIcon>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QNetworkProxy>
#include <QSettings>
#include <QShortcut>
#include <QString>
#include <QStringList>
#include <QTranslator>

#if defined (HAVE_QSCINTILLA)
#  include <Qsci/qscilexer.h>
#endif

#include "gui-preferences-cs.h"
#include "gui-preferences-ed.h"
#include "gui-preferences-sc.h"
#include "gui-preferences-global.h"
#include "gui-settings.h"

#include "localcharset-wrapper.h"
#include "oct-env.h"
#include "oct-string.h"

#include "defaults.h"

OCTAVE_BEGIN_NAMESPACE(octave)

QString gui_settings::file_name () const
{
  return fileName ();
}

QString
gui_settings::directory_name () const
{
  QFileInfo sfile (fileName ());

  return sfile.absolutePath ();
}

bool
gui_settings::bool_value (const gui_pref& pref) const
{
  return value (pref).toBool ();
}

QByteArray
gui_settings::byte_array_value (const gui_pref& pref) const
{
  return value (pref).toByteArray ();
}

QColor
gui_settings::color_value (const gui_pref& pref) const
{
  return value (pref).value<QColor> ();
}

QDateTime
gui_settings::date_time_value (const gui_pref& pref) const
{
  return value (pref).toDateTime ();
}

int
gui_settings::int_value (const gui_pref& pref) const
{
  return value (pref).toInt ();
}

QString
gui_settings::string_value (const gui_pref& pref) const
{
  return value (pref).toString ();
}

QStringList
gui_settings::string_list_value (const gui_pref& pref) const
{
  return value (pref).toStringList ();
}

uint
gui_settings::uint_value (const gui_pref& pref) const
{
  return value (pref).toUInt ();
}

QColor
gui_settings::get_color_value (const QVariant& def, int mode) const
{
  QColor default_color;

  // Determine whether the default value in pref is given as
  // QPalette::ColorRole or as QColor
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  if (def.canConvert (QMetaType (QMetaType::QColor)))
#else
  if (def.canConvert (QMetaType::QColor))
#endif
    default_color = def.value<QColor> ();
  else
    {
      // The default colors are given as color roles for
      // the application's palette
      default_color = QApplication::palette ().color
                      (static_cast<QPalette::ColorRole> (def.toInt ()));
      // FIXME: use value<QPalette::ColorRole> instead of static cast after
      //        dropping support of Qt 5.4
    }

  if ((mode == 1) && (default_color != settings_color_no_change))
    {
      // In second mode, determine the default color from the first mode
#if HAVE_QCOLOR_FLOAT_TYPE
      float h, s, l, a;
#else
      qreal h, s, l, a;
#endif
      default_color.getHslF (&h, &s, &l, &a);
      qreal l_new = 1.0-l*0.85;
      if (l < 0.3)
        l_new = 1.0-l*0.7;  // convert darker into lighter colors
      default_color.setHslF (h, s, l_new, a);
    }

  return default_color;
}

QColor
gui_settings::color_value (const gui_pref& pref, int mode) const
{
  QColor default_color = get_color_value (pref.def (), mode);

  return value (pref.settings_key () + settings_color_modes_ext[mode],
                QVariant (default_color)).value<QColor> ();
}

void
gui_settings::set_color_value (const gui_pref& pref,
                               const QColor& color, int mode)
{
  int m = mode;
  if (m > 1)
    m = 1;

  setValue (pref.settings_key () + settings_color_modes_ext[m],
            QVariant (color));
}

QString
gui_settings::sc_value (const sc_pref& scpref) const
{
  QString full_settings_key = sc_group + "/" + scpref.settings_key ();

  if (contains (full_settings_key))
    {
      QKeySequence key_seq = sc_def_value (scpref);

      // Get the value from the settings where the key sequences are stored
      // as strings
      return value (full_settings_key, key_seq.toString (QKeySequence::NativeText)).toString ();
    }
  else
    return scpref.def_text ();
}

QKeySequence
gui_settings::sc_def_value (const sc_pref& scpref) const
{
  return scpref.def_value ();
}

void
gui_settings::set_shortcut (QAction *action, const sc_pref& scpref, bool enable)
{
  if (! enable)
    {
      // Disable => remove existing shortcut from the action
      action->setShortcut (QKeySequence ());
      return;
    }

    QString shortcut = sc_value (scpref);
    action->setShortcut (QKeySequence (shortcut));
}

void
gui_settings::shortcut (QShortcut *sc, const sc_pref& scpref)
{
  QString shortcut = sc_value (scpref);
  sc->setKey (QKeySequence (shortcut));
}

void
gui_settings::config_icon_theme ()
{
  int theme_index;

  if (contains (global_icon_theme_index.settings_key ()))
    theme_index = int_value (global_icon_theme_index);
  else
    {
      // New pref does not exist.  Use old if required.  Add new and
      // remove deprecated key.

      if (bool_value (global_icon_theme))
        theme_index = ICON_THEME_SYSTEM;
      else
        theme_index = ICON_THEME_OCTAVE;

      setValue (global_icon_theme_index.settings_key (), theme_index);
      remove (global_icon_theme.settings_key ());
    }

  QIcon::setThemeName (global_all_icon_themes.at (theme_index));

  QStringList icon_fallbacks;

  // Set the required fallback search paths.

  switch (theme_index)
    {
    case ICON_THEME_SYSTEM:
      icon_fallbacks << global_icon_paths.at (ICON_THEME_OCTAVE);
      icon_fallbacks << global_icon_paths.at (ICON_THEME_TANGO);
      break;
    case ICON_THEME_TANGO:
      icon_fallbacks << global_icon_paths.at (ICON_THEME_OCTAVE);
      break;
    case ICON_THEME_OCTAVE:
      icon_fallbacks << global_icon_paths.at (ICON_THEME_TANGO);
      break;
    }

  icon_fallbacks << global_icon_paths.at (ICON_THEME_CURSORS);

  setValue (global_icon_fallbacks.settings_key (), icon_fallbacks);
}

QIcon
gui_settings::icon (const QString& icon_name, bool octave_only,
                    const QString& icon_alt_name)
{
  if (octave_only)
    return QIcon (global_icon_paths.at (ICON_THEME_OCTAVE) + icon_name + ".png");

  if (QIcon::hasThemeIcon (icon_name))
    return QIcon (QIcon::fromTheme (icon_name));
  else if ((! icon_alt_name.isEmpty ()) && QIcon::hasThemeIcon (icon_alt_name))
    return QIcon (QIcon::fromTheme (icon_alt_name));

  QStringList icon_fallbacks
    = value (global_icon_fallbacks.settings_key ()).toStringList ();

  for (int i = 0; i < icon_fallbacks.length (); i++ )
    {
      QString icon_file (icon_fallbacks.at (i) + icon_name + ".png");
      if (QFile (icon_file).exists ())
        return QIcon (icon_file);
    }

  //QIcon::setThemeName (current_theme);
  return QIcon ();
}

QString
gui_settings::get_default_font_family ()
{
  // Get all available fixed width fonts from the Qt font database.

  QStringList fonts;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  for (QString font : QFontDatabase::families ())
    {
      if (QFontDatabase::isFixedPitch (font))
        fonts << font;
    }
#else
  QFontDatabase font_database;

  for (QString font : font_database.families ())
    {
      if (font_database.isFixedPitch (font))
        fonts << font;
    }
#endif

  QString default_family;

#if defined (Q_OS_MAC)
  // Use hard coded default on macOS, since selection of fixed width
  // default font is unreliable (see bug #59128).
  // Test for macOS default fixed width font
  if (fonts.contains (global_mono_font.def ().toString ()))
    default_family = global_mono_font.def ().toString ();
#endif

  // If default font is still empty (on all other platforms or
  // if macOS default font is not available): use QFontDatabase
  if (default_family.isEmpty ())
    {
      // Get the system's default monospaced font
      QFont fixed_font = QFontDatabase::systemFont (QFontDatabase::FixedFont);
      default_family = fixed_font.defaultFamily ();

      // Since this might be unreliable, test all available fixed width fonts
      if (! fonts.contains (default_family))
        {
          // Font returned by QFontDatabase is not in fixed fonts list.
          // Fallback: take first from this list
          default_family = fonts[0];
        }
    }

  // Test env variable which has preference
  std::string env_default_family = sys::env::getenv ("OCTAVE_DEFAULT_FONT");
  if (! env_default_family.empty ())
    default_family = QString::fromStdString (env_default_family);

  return default_family;
}

QStringList
gui_settings::get_default_font ()
{
  QString default_family = get_default_font_family ();

  // determine the fefault font size of the system
  // FIXME: QApplication::font () does not return the monospace font,
  //        but the size should be probably near to the monospace font
  QFont font = QApplication::font ();

  int font_size = font.pointSize ();
  if (font_size == -1)
    font_size = static_cast <int> (std::floor (font.pointSizeF ()));

  // check for valid font size, otherwise take default 10
  QString default_font_size = "10";
  if (font_size > 0)
    default_font_size = QString::number (font_size);

  std::string env_default_font_size
    = sys::env::getenv ("OCTAVE_DEFAULT_FONT_SIZE");

  if (! env_default_font_size.empty ())
    default_font_size = QString::fromStdString (env_default_font_size);

  QStringList result;
  result << default_family;
  result << default_font_size;
  return result;
}

QString
gui_settings::get_gui_translation_dir ()
{
  // get environment variable for the locale dir (e.g. from run-octave)
  std::string dldir = sys::env::getenv ("OCTAVE_LOCALE_DIR");
  if (dldir.empty ())
    dldir = config::oct_locale_dir ();  // env-var empty, load the default location
  return QString::fromStdString (dldir);
}

void
gui_settings::load_translator (QTranslator *tr, const QLocale& locale, const QString& filename, const QString& prefix, const QString& directory) const
{
  if (! tr->load (locale, filename, prefix, directory))
    qWarning () << "failed to load translator for locale" << locale.name () << "from file" << filename << "with prefix" << prefix << "from directory" << directory;
}

void
gui_settings::load_translator (QTranslator *tr, const QString& prefix, const QString& language, const QString& directory) const
{
  if (! tr->load (prefix + language, directory))
    if (! tr->load (prefix + language.toLower (), directory))
      qWarning () << "failed to load translator file" << (prefix + language) << "or" << (prefix + language.toLower ()) << "from directory" << directory;
}

void
gui_settings::config_translators (QTranslator *qt_tr,
                                  QTranslator *qsci_tr,
                                  QTranslator *gui_tr)
{
  QString qt_trans_dir
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    = QLibraryInfo::path (QLibraryInfo::TranslationsPath);
#else
    = QLibraryInfo::location (QLibraryInfo::TranslationsPath);
#endif

  QString language = "SYSTEM";  // take system language per default

  // FIXME: can we somehow ensure that the settings object will always
  // be initialize and valid?

  // get the locale from the settings if already available
  language = string_value (global_language);

  // load the translations depending on the settings
  if (language == "SYSTEM")
    {
      // get the system locale and pass it to the translators for loading
      // the suitable translation files
      QLocale sys_locale = QLocale::system ();

      load_translator (qt_tr, sys_locale, "qt", "_", qt_trans_dir);
      load_translator (qsci_tr, sys_locale, "qscintilla", "_", qt_trans_dir);
      load_translator (gui_tr, sys_locale, "", "", get_gui_translation_dir ());
    }
  else
    {
      // load the translation files depending on the given locale name
      load_translator (qt_tr, "qt_", language, qt_trans_dir);
      load_translator (qsci_tr, "qscintilla_", language, qt_trans_dir);
      load_translator (gui_tr, "", language, get_gui_translation_dir ());
    }
}

int
gui_settings::get_valid_lexer_styles (QsciLexer *lexer, int *styles)
{
#if defined (HAVE_QSCINTILLA)
  int max_style = 0;
  int actual_style = 0;
  while (actual_style < ed_max_style_number && max_style < ed_max_lexer_styles)
    {
      if ((lexer->description (actual_style)) != "")  // valid style
        styles[max_style++] = actual_style;
      actual_style++;
    }

  return max_style;
#else
  octave_unused_parameter (lexer);
  octave_unused_parameter (styles);

  return 0;
#endif
}

#if defined (HAVE_QSCINTILLA)
/*!
 * Copys the attributes bold, italic and underline from QFont
 * @p attr to the font @p base and returns the result without
 * changing @p base,
 * @param attr QFont with the desired attributes
 * @param base QFont with desired family and size
 */
static QFont
copy_font_attributes (const QFont& attr, const QFont& base)
{
  QFont dest (base);

  dest.setBold (attr.bold ());
  dest.setItalic (attr.italic ());
  dest.setUnderline (attr.underline ());

  return dest;
}
#endif

void
gui_settings::read_lexer_settings (QsciLexer *lexer, int mode, int def)
{
#if defined (HAVE_QSCINTILLA)
  // Test whether the settings for lexer is already contained in the
  // given gui settings file. If yes, load them, if not copy them from the
  // default settings file.
  // This is useful when a new language support is implemented and the
  // existing settings file is used (which is of course the common case).
  int m = mode;
  if (m > 1)
    m = 1;

  QString group ("Scintilla" + settings_color_modes_ext[m]);

  beginGroup (group);
  beginGroup (lexer->language ());

  QStringList lexer_keys = allKeys ();

  endGroup ();
  endGroup ();

  if (def == settings_reload_default_colors_flag || lexer_keys.count () == 0)
    {
      // We have to reload the default values or no Lexer keys found:
      // If mode == 0, take all settings except font from default lexer
      // If Mode == 1, take all settings except font from default lexer
      //               and convert the color by inverting the lightness

      // Get the default font
      QStringList def_font = get_default_font ();
      QFont df (def_font[0], def_font[1].toInt ());
      QFont dfa = copy_font_attributes (lexer->defaultFont (), df);
      lexer->setDefaultFont (dfa);

      QColor c, p;

      int styles[ed_max_lexer_styles];  // array for saving valid styles
      int max_style = get_valid_lexer_styles (lexer, styles);

      for (int i = 0; i < max_style; i++)
        {
          c = get_color_value (QVariant (lexer->color (styles[i])), m);
          lexer->setColor (c, styles[i]);
          p = get_color_value (QVariant (lexer->paper (styles[i])), m);
          lexer->setPaper (p, styles[i]);
          dfa = copy_font_attributes (lexer->font (styles[i]), df);
          lexer->setFont (dfa, styles[i]);
        }
      // Set defaults last for not changing the defaults of the styles
      lexer->setDefaultColor (lexer->color (styles[0]));
      lexer->setDefaultPaper (lexer->paper (styles[0]));

      // Write settings if not just reload the default values
      if (def != settings_reload_default_colors_flag)
        {
          const std::string group_str = group.toStdString ();
          lexer->writeSettings (*this, group_str.c_str ());
          sync ();
        }
    }
  else
    {
      // Found lexer keys, read the settings
      const std::string group_str = group.toStdString ();
      lexer->readSettings (*this, group_str.c_str ());
    }
#else
  octave_unused_parameter (lexer);
  octave_unused_parameter (mode);
  octave_unused_parameter (def);

  return;
#endif
}

bool
gui_settings::update_settings_key (const QString& old_key,
                                   const QString& new_key)
{
  if (contains (old_key))
    {
      QVariant preference = value (old_key);
      setValue (new_key, preference);
      remove (old_key);
      return true;
    }

  return false;
}

void
gui_settings::update_network_settings ()
{
  QNetworkProxy proxy;

  // Assume no proxy and empty proxy data
  QNetworkProxy::ProxyType proxy_type = QNetworkProxy::NoProxy;
  QString scheme;
  QString host;
  int port = 0;
  QString user;
  QString pass;
  QUrl proxy_url = QUrl ();

  if (bool_value (global_use_proxy))
    {
      // Use a proxy, collect all required information
      QString proxy_type_string = string_value (global_proxy_type);

      // The proxy type for the Qt proxy settings
      if (proxy_type_string == "Socks5Proxy")
        proxy_type = QNetworkProxy::Socks5Proxy;
      else if (proxy_type_string == "HttpProxy")
        proxy_type = QNetworkProxy::HttpProxy;

      // The proxy data from the settings
      if (proxy_type_string == "HttpProxy"
          || proxy_type_string == "Socks5Proxy")
        {
          host = string_value (global_proxy_host);
          port = int_value (global_proxy_port);
          user = string_value (global_proxy_user);
          pass = string_value (global_proxy_pass);
          if (proxy_type_string == "HttpProxy")
            scheme = "http";
          else if (proxy_type_string == "Socks5Proxy")
            scheme = "socks5";

          QUrl env_var_url = QUrl ();
          proxy_url.setScheme (scheme);
          proxy_url.setHost (host);
          proxy_url.setPort (port);
          if (! user.isEmpty ())
            proxy_url.setUserName (user);
          if (! pass.isEmpty ())
            proxy_url.setPassword (pass);
        }

      // The proxy data from environment variables
      if (proxy_type_string == global_proxy_all_types.at (2))
        {
          const std::array<std::string, 6> env_vars =
          {
            "ALL_PROXY", "all_proxy",
            "HTTP_PROXY", "http_proxy",
            "HTTPS_PROXY", "https_proxy"
          };

          unsigned int count = 0;
          while (! proxy_url.isValid () && count < env_vars.size ())
            {
              proxy_url = QUrl (QString::fromStdString
                                (sys::env::getenv (env_vars[count])));
              count++;
            }

          if (proxy_url.isValid ())
            {
              // Found an entry, get the data from the string
              scheme = proxy_url.scheme ();

              if (scheme.contains ("socks", Qt::CaseInsensitive))
                proxy_type = QNetworkProxy::Socks5Proxy;
              else
                proxy_type = QNetworkProxy::HttpProxy;

              host = proxy_url.host ();
              port = proxy_url.port ();
              user = proxy_url.userName ();
              pass = proxy_url.password ();
            }
        }
    }

  // Set proxy for Qt framework
  proxy.setType (proxy_type);
  proxy.setHostName (host);
  proxy.setPort (port);
  proxy.setUser (user);
  proxy.setPassword (pass);

  QNetworkProxy::setApplicationProxy (proxy);

  // Set proxy for curl library if not based on environment variables
  std::string proxy_url_str = proxy_url.toString ().toStdString ();
  sys::env::putenv ("http_proxy", proxy_url_str);
  sys::env::putenv ("HTTP_PROXY", proxy_url_str);
  sys::env::putenv ("https_proxy", proxy_url_str);
  sys::env::putenv ("HTTPS_PROXY", proxy_url_str);
}

// initialize a given combo box with available text encodings
void
gui_settings::combo_encoding (QComboBox *combo, const QString& current)
{
  std::vector<std::string> encoding_list {string::get_encoding_list ()};

  // prepend SYSTEM
  std::string locale_charset {octave_locale_charset_wrapper ()};
  std::transform (locale_charset.begin (), locale_charset.end (),
                  locale_charset.begin (), ::toupper);
  locale_charset = "SYSTEM (" + locale_charset + ")";
  encoding_list.insert (encoding_list.begin (), locale_charset);

  // get the value from the settings file if no current encoding is given
  QString enc {current};

  // Check for valid codec for the default.  If this fails, "SYSTEM" (i.e.
  // locale_charset) will be chosen.
  // FIXME: The default is "SYSTEM" on all platforms.  So can this fallback
  // logic be removed completely?
  bool default_exists = false;
  bool show_system = false;
  if (ed_default_enc.def ().toString ().startsWith ("SYSTEM"))
    show_system = true;
  else if (std::find (encoding_list.begin (), encoding_list.end (),
                      ed_default_enc.def ().toString ().toStdString ())
           != encoding_list.end ())
    default_exists = true;

  QString default_enc = QString::fromStdString (locale_charset);

  if (enc.isEmpty ())
    {
      enc = string_value (ed_default_enc);

      if (enc.isEmpty ())  // still empty?
        {
          if (default_exists)
            enc = ed_default_enc.def ().toString ();
          else
            enc = default_enc;
        }
    }

  // fill the combo box
  for (const auto& c : encoding_list)
    combo->addItem (QString::fromStdString (c));

  // prepend current encoding if not in list
  if (combo->findText (enc, Qt::MatchExactly) < 0)
    combo->insertItem (0, enc);

  // prepend the default item
  combo->insertSeparator (0);
  if (show_system || ! default_exists)
    combo->insertItem (0, default_enc);
  else
    combo->insertItem (0, ed_default_enc.def ().toString ());

  // select the current encoding
  combo->setCurrentIndex (combo->findText (enc, Qt::MatchExactly));

  combo->setMaxVisibleItems (12);
}

void
gui_settings::reload ()
{
  // Declare some empty options, which may be set at first startup for
  // writing them into the newly created settings file
  QString custom_editor;
  QStringList def_font;

  // Check whether the settings file does not yet exist
  if (! QFile::exists (file_name ()))
    {
      // Get the default font (for terminal)
      def_font = get_default_font ();

      // Get a custom editor defined as env variable
      std::string env_default_editor
        = sys::env::getenv ("OCTAVE_DEFAULT_EDITOR");

      if (! env_default_editor.empty ())
        custom_editor = QString::fromStdString (env_default_editor);
    }

  check ();

  // Write some settings that were dynamically determined at first startup

  // Custom editor
  if (! custom_editor.isEmpty ())
    setValue (global_custom_editor.settings_key (), custom_editor);

  // Default monospace font for the terminal
  if (def_font.count () > 1)
    {
      setValue (cs_font.settings_key (), def_font[0]);
      setValue (cs_font_size.settings_key (), def_font[1].toInt ());
    }

  // Write the default monospace font into the settings for later use by
  // console and editor as fallbacks of their font preferences.
  setValue (global_mono_font.settings_key (), get_default_font_family ());
}

void
gui_settings::check ()
{
  if (status () == QSettings::NoError)
    {
      // Test usability (force file to be really created)
      setValue ("dummy", 0);
      sync ();
    }

  if (! (QFile::exists (file_name ())
         && isWritable ()
         && status () == QSettings::NoError))
    {
      QString msg
        = QString (QT_TR_NOOP ("Error %1 creating the settings file\n%2\n"
                               "Make sure you have read and write permissions to\n%3\n\n"
                               "Octave GUI must be closed now."));

      QMessageBox::critical (nullptr,
                             QString (QT_TR_NOOP ("Octave Critical Error")),
                             msg.arg (status ())
                                .arg (file_name ())
                                .arg (directory_name ()));

      exit (1);
    }
  else
    remove ("dummy");  // Remove test entry
}

OCTAVE_END_NAMESPACE(octave)
