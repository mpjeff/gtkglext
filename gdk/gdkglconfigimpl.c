/* GdkGLExt - OpenGL Extension to GDK
 * Copyright (C) 2012  Thomas Zimmermann
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gdk/gdk.h>            /* for gdk_error_trap_(push|pop) () */

#include "gdkgldebug.h"
#include "gdkglconfigimpl.h"

G_DEFINE_TYPE (GdkGLConfigImpl, gdk_gl_config_impl, G_TYPE_OBJECT);

static void
gdk_gl_config_impl_init (GdkGLConfigImpl *self)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();
}

static void
gdk_gl_config_impl_finalize (GObject *object)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();

  G_OBJECT_CLASS (gdk_gl_config_impl_parent_class)->finalize (object);
}

static void
gdk_gl_config_impl_class_init (GdkGLConfigImplClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  klass->create_gl_window = NULL;
  klass->get_screen = NULL;
  klass->get_attrib = NULL;
  klass->get_visual = NULL;
  klass->get_depth = NULL;

  object_class->finalize = gdk_gl_config_impl_finalize;
}
