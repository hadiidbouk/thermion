#ifndef THERMION_EGL_TEXTURE_H
#define THERMION_EGL_TEXTURE_H

#include <epoxy/gl.h>
#include <epoxy/egl.h>

#include <gtk/gtk.h>
#include <glib-object.h>
#include <flutter_linux/flutter_linux.h>
#include <flutter_linux/fl_texture_gl.h>
#include <flutter_linux/fl_texture.h>
#include <flutter_linux/fl_texture_registrar.h>

#include "SurfaceExportInfo.h"

G_BEGIN_DECLS

#define THERMION_TEXTURE_GL(obj) \
   (G_TYPE_CHECK_INSTANCE_CAST((obj), thermion_texture_gl_get_type(), \
                               ThermionTextureGL))

typedef enum {
    // The producer owns a GBM-backed surface. Flutter lazily imports its
    // DMA-BUF into a texture and EGLImage.
    THERMION_TEXTURE_TRANSPORT_DMA_BUF,
    // The plugin owns the source texture and EGLImage. Flutter owns the
    // imported texture name.
    THERMION_TEXTURE_TRANSPORT_PLUGIN_EGL_IMAGE,
    // LinuxOpenGLContext owns the source texture and EGLImage. Flutter owns
    // the imported texture name.
    THERMION_TEXTURE_TRANSPORT_CONTEXT_EGL_IMAGE,
    // A temporary texture created directly on Flutter's raster context to
    // capture the context before Filament initializes.
    THERMION_TEXTURE_TRANSPORT_CONTEXT_BOOTSTRAP,
} ThermionTextureTransport;

struct _ThermionTextureGL {
    FlTextureGL parent_instance;
    GLuint gl_texture_id;
    uint32_t width;
    uint32_t height;
    FlTextureRegistrar* registrar;
    // dmabuf info for lazy EGL import (DMA-BUF path only)
    int dmabuf_fd;
    uint32_t stride;
    uint32_t offset;
    uint32_t drm_format;
    uint64_t drm_modifier;
    EGLImage egl_image;
    gboolean initialized;
    int64_t surface_id;  // for Blit() and destruction
    ThermionTextureTransport transport;
    // Flutter-side GL texture (created on Flutter's context, backed by egl_image)
    GLuint flutter_gl_texture_id;
    // Deferred awaitTextureReady responses stored until populate creates the
    // bootstrap GL texture. Each entry owns a FlMethodCall reference.
    GPtrArray* pending_ready_calls;
    // Set before native resources are released. Queued idle responses use this
    // to avoid publishing a texture ID after cancellation.
    gboolean destroyed;
};

typedef struct _ThermionTextureGL ThermionTextureGL;
typedef struct {
  FlTextureGLClass parent_instance;
  gboolean (*populate)(FlTextureGL* texture,
                       uint32_t* target,
                       uint32_t* name,
                       uint32_t* width,
                       uint32_t* height,
                       GError** error);
} ThermionTextureGLClass;

G_END_DECLS

ThermionTextureGL* thermion_texture_gl_create(
    const SurfaceExportInfo& info,
    int64_t surface_id,
    FlTextureRegistrar* registrar);

// EGLImage bridge path: wraps a GL texture + EGLImage.  On first populate,
// the EGLImage is imported into a new texture on Flutter's own GL context.
ThermionTextureGL* thermion_texture_gl_create_shared(
    uint32_t width, uint32_t height,
    GLuint gl_texture_id,
    EGLImage egl_image,
    int64_t surface_id,
    FlTextureRegistrar* registrar);

// Pre-engine initialization path. The GL texture is created lazily from
// populate(), while Flutter's raster EGL context is current. This helper is
// private to the plugin shared object and is not part of its exported ABI.
ThermionTextureGL*
thermion_texture_gl_create_context_bootstrap(
    uint32_t width, uint32_t height,
    FlTextureRegistrar* registrar);

// Flutter's render context, captured during the first deferred populate().
// Used by ensure_opengl_context() to select the compatible direct or DMA-BUF
// pathway on Flutter's actual EGLDisplay.
extern EGLContext thermion_flutter_render_context;
extern EGLDisplay thermion_flutter_render_display;

#endif
