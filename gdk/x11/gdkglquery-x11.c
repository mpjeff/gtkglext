/* GdkGLExt - OpenGL Extension to GDK
 * Copyright (C) 2002-2003  Naofumi Yasufuku
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA.
 */

#include <string.h>

#include <gmodule.h>

#include "gdkglx.h"
#include "gdkglprivate-x11.h"
#include "gdkglconfig-x11.h"
#include "gdkglquery.h"

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
#include <gdk/gdkdisplay.h>
#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */

/**
 * gdk_gl_query_extension:
 *
 * Indicates whether the window system supports the OpenGL extension
 * (GLX, WGL, etc.).
 *
 * Return value: TRUE if OpenGL is supported, FALSE otherwise.
 **/
gboolean
gdk_gl_query_extension (void)
{
#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
  return glXQueryExtension (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
                            NULL, NULL);
#else  /* GDKGLEXT_MULTIHEAD_SUPPORT */
  return glXQueryExtension (gdk_x11_get_default_xdisplay (),
                            NULL, NULL);
#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */
}

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT

gboolean
gdk_gl_query_extension_for_display (GdkDisplay *display)
{
  g_return_val_if_fail (GDK_IS_DISPLAY (display), FALSE);

  return glXQueryExtension (GDK_DISPLAY_XDISPLAY (display),
                            NULL, NULL);
}

#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */

/**
 * gdk_gl_query_version:
 * @major: returns the major version number of the OpenGL extension.
 * @minor: returns the minor version number of the OpenGL extension.
 *
 * Returns the version numbers of the OpenGL extension to the window system.
 *
 * In the X Window System, it returns the GLX version.
 *
 * In the Microsoft Windows, it returns the Windows version.
 *
 * Return value: FALSE if it fails, TRUE otherwise.
 **/
gboolean
gdk_gl_query_version (int *major,
                      int *minor)
{
#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
  return glXQueryVersion (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
                          major, minor);
#else  /* GDKGLEXT_MULTIHEAD_SUPPORT */
  return glXQueryVersion (gdk_x11_get_default_xdisplay (),
                          major, minor);
#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */
}

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT

gboolean
gdk_gl_query_version_for_display (GdkDisplay *display,
                                  int        *major,
                                  int        *minor)
{
  g_return_val_if_fail (GDK_IS_DISPLAY (display), FALSE);

  return glXQueryVersion (GDK_DISPLAY_XDISPLAY (display),
                          major, minor);
}

#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */

/*
 * This code is based on __glutIsSupportedByGLX().
 */

/**
 * gdk_x11_gl_query_glx_extension:
 * @glconfig: a #GdkGLConfig.
 * @extension: name of GLX extension.
 *
 * Determines whether a given GLX extension is supported.
 *
 * Return value: TRUE if the GLX extension is supported, FALSE if not supported.
 **/
gboolean
gdk_x11_gl_query_glx_extension (GdkGLConfig *glconfig,
                                const char  *extension)
{
  static const char *extensions = NULL;
  const char *start;
  char *where, *terminator;
  int major, minor;

  g_return_val_if_fail (GDK_IS_GL_CONFIG (glconfig), FALSE);

  /* Extension names should not have spaces. */
  where = strchr (extension, ' ');
  if (where || *extension == '\0')
    return FALSE;

  if (!extensions)
    {
      /* Be careful not to call glXQueryExtensionsString if it
         looks like the server doesn't support GLX 1.1.
         Unfortunately, the original GLX 1.0 didn't have the notion
         of GLX extensions. */

      glXQueryVersion (GDK_GL_CONFIG_XDISPLAY (glconfig),
                       &major, &minor);

      if ((major == 1 && minor < 1) || (major < 1))
        return FALSE;

      extensions = glXQueryExtensionsString (GDK_GL_CONFIG_XDISPLAY (glconfig),
                                             GDK_GL_CONFIG_SCREEN_XNUMBER (glconfig));
    }

  /* It takes a bit of care to be fool-proof about parsing
     the GLX extensions string.  Don't be fooled by
     sub-strings,  etc. */
  start = extensions;
  for (;;)
    {
      where = strstr (start, extension);
      if (!where)
        break;

      terminator = where + strlen(extension);

      if (where == start || *(where - 1) == ' ')
        if (*terminator == ' ' || *terminator == '\0')
          {
            GDK_GL_NOTE (MISC, g_message (" - %s - supported", extension));
            return TRUE;
          }

      start = terminator;
    }

  GDK_GL_NOTE (MISC, g_message (" - %s - not supported", extension));

  return FALSE;
}

static const gchar *
gdk_gl_get_libdir (void)
{
  const gchar *libdir;

  libdir = g_getenv ("GDKGLEXT_LIBDIR");
  if (libdir)
    return libdir;

  return GDKGLEXT_LIBDIR;
}

/**
 * gdk_gl_get_proc_address:
 * @proc_name: extension function name.
 *
 * Returns the address of the OpenGL extension functions.
 *
 * Return value: the address of the extension function named by @proc_name.
 **/
GdkGLProc
gdk_gl_get_proc_address (const char *proc_name)
{
  typedef GdkGLProc (*__glXGetProcAddressProc) (const GLubyte *);
  static __glXGetProcAddressProc glx_get_proc_address = NULL;
  static gboolean initialized = FALSE;
  static GModule *module = NULL;
  gchar *file_name;
  GdkGLProc proc_address = NULL;

  GDK_GL_NOTE (FUNC, g_message (" - gdk_gl_get_proc_address ()"));

  if (!initialized)
    {
      /*
       * Look up glXGetProcAddress () function.
       */

      file_name = g_module_build_path (gdk_gl_get_libdir (), GDKGLEXT_LIBNAME);

      GDK_GL_NOTE (MISC, g_message (" - Open %s", file_name));

      module = g_module_open (file_name, G_MODULE_BIND_LAZY);
      if (module == NULL)
        {
          g_warning ("Cannot open %s", file_name);
          g_free (file_name);
          return NULL;
        }
      g_free (file_name);

      g_module_symbol (module, "glXGetProcAddress",
                       (gpointer) &glx_get_proc_address);
      if (glx_get_proc_address == NULL)
        g_module_symbol (module, "glXGetProcAddressARB",
                         (gpointer) &glx_get_proc_address);
      if (glx_get_proc_address == NULL)
        g_module_symbol (module, "glXGetProcAddressEXT",
                         (gpointer) &glx_get_proc_address);

      GDK_GL_NOTE (MISC, g_message (" - glXGetProcAddress () - %s",
                                    glx_get_proc_address ? "supported" : "not supported"));

      initialized = TRUE;
    }

  /* Try glXGetProcAddress () */

  if (glx_get_proc_address != NULL)
    {
      proc_address = glx_get_proc_address (proc_name);

      GDK_GL_NOTE (IMPL, g_message (" * glXGetProcAddress () - %s",
                                    proc_address ? "succeeded" : "failed"));
    }

  /* Try g_module_symbol () */

  if (proc_address == NULL)
    {
      g_module_symbol (module, proc_name, (gpointer) &proc_address);

      GDK_GL_NOTE (MISC, g_message (" - g_module_symbol () - %s",
                                    proc_address ? "succeeded" : "failed"));
    }

  return proc_address;
}
