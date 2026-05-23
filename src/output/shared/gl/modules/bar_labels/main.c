#include <stdio.h>
#include <string.h>
#include "shared.h"
#include "output/shared/gl/main.h"
#include "shared/log.h"
#include "label_renderer.h"

static label_renderer *g_lr = NULL;

EXP_FUNC void xava_gl_module_ionotify_callback(xava_ionotify_event event,
        const char *filename, int id, XAVA *xava) {
    UNUSED(filename); UNUSED(id); UNUSED(xava); UNUSED(event);
}

EXP_FUNC void xava_gl_module_config_load(XAVAGLModule *module, XAVA *xava) {
    UNUSED(module); UNUSED(xava);
    xavaLog("bar_labels: config_load");
}

EXP_FUNC void xava_gl_module_init(XAVAGLModuleOptions *options) {
    UNUSED(options);
    g_lr = label_renderer_create("/usr/share/fonts/TTF/DejaVuSans.ttf", 14.0f);
    if (!g_lr) { xavaLog("bar_labels: label_renderer init failed"); }
    else { xavaLog("bar_labels: label_renderer init OK"); }
}

EXP_FUNC void xava_gl_module_apply(XAVAGLModuleOptions *options) {
    UNUSED(options);
    xavaLog("bar_labels: apply");
}

EXP_FUNC void xava_gl_module_event(XAVAGLModuleOptions *options) {
    UNUSED(options);
}

EXP_FUNC void xava_gl_module_clear(XAVAGLModuleOptions *options) {
    UNUSED(options);
}

EXP_FUNC void xava_gl_module_draw(XAVAGLModuleOptions *options) {
    UNUSED(options);
}

EXP_FUNC void xava_gl_module_cleanup(XAVAGLModuleOptions *options) {
    UNUSED(options);
    label_renderer_destroy(g_lr);
    g_lr = NULL;
}

EXP_FUNC xava_version xava_gl_module_version(void) {
    return xava_version_get();
}
