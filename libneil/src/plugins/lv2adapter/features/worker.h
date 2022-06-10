#pragma once
// from https://git.zrythm.org/zrythm/zrythm/src/branch/master/inc/plugins/lv2/lv2_worker.h


#include "zix/ring.h"
#include "zix/sem.h"
#include "zix/thread.h"
#include "lilv/lilv.h"
#include "lv2/worker/worker.h"

struct PluginAdapter;


struct LV2Worker {
    PluginAdapter*              plugin;     // Pointer back to the plugin
    ZixRing*                    requests  = nullptr;   // Requests to the worker
    ZixRing*                    responses = nullptr;  // Responses from the worker
    void*                       response  = nullptr;   // Worker response buffer
    ZixSem                      sem;        // Worker semaphore
    ZixThread                   thread;     // Worker thread
    const LV2_Worker_Interface* iface;      // Plugin worker interface
    bool                        threaded;   // Run work in another thread
    bool                        halting;
    bool                        enable;     //
};


void
lv2_worker_init (
  PluginAdapter*              plugin,
  LV2Worker*                  worker,
  const LV2_Worker_Interface* iface,
  bool                        threaded);


/**
 * Stops the worker and frees resources.
 */
void
lv2_worker_finish (LV2Worker* worker);


/**
 * Called from plugins during run() to request that
 * Zrythm calls the work() method in a non-realtime
 * context with the given arguments.
 */
LV2_Worker_Status
lv2_worker_schedule (
  LV2_Worker_Schedule_Handle handle,
  uint32_t                   size,
  const void*                data);


/**
 * Called during run() to process worker replies.
 *
 * Internally calls work_response in
 * https://lv2plug.in/doc/html/group__worker.html.
 */
void
lv2_worker_emit_responses (
  LV2Worker* worker, LilvInstance* instance);
