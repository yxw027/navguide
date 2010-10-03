#ifndef __lc_util_h__
#define __lc_util_h__

#include <glib.h>

#include <lc/lc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * lc_singleton_get:
 *
 * Retrieves a singleton instance of lc_t.  The instance will already be
 * initialized to defaults and attached to the glib event loop.  If the
 * instance could not be created or initialized, then NULL is returned.
 */
lc_t * lcu_singleton_get ();

/**
 * lc_singleton_release:
 *
 * Releases the singleton LC and decrements its reference count.  If there are
 * no more references to the singleton LC (lc_singleton_get increments the
 * reference count), then it is destroyed.
 */
void lcu_singleton_release (lc_t *lc);

/**
 * lcu_mainloop_attach_lc (lc_t *lc)
 * attaches/detaches LC to/from the glib mainloop
 * When attached, lc_handle() is invoked "automatically" when a message is 
 * received over LC.
 *
 * only one instance of lc_t can be attached per process
 *
 * returns 0 on success, -1 on failure
 */
int lcu_glib_mainloop_attach_lc (lc_t *lc);

int lcu_glib_mainloop_detach_lc (lc_t *lc);

#ifdef __cplusplus
}
#endif

#endif
