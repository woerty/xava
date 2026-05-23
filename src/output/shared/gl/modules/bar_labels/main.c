#include <stdio.h>
#include <string.h>
#include "shared.h"
#include "output/shared/gl/main.h"
#include "shared/log.h"

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
    xavaLog("bar_labels: init");
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
    xavaLog("bar_labels: cleanup");
}

EXP_FUNC xava_version xava_gl_module_version(void) {
    return xava_version_get();
}
