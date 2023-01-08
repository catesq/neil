
#include "worker.h"
#include "../PluginAdapter.h"

LV2_Worker_Status
lv2_worker_respond (
        LV2_Worker_Respond_Handle handle,
        uint32_t                  size,
        const void*               data)
{
    LV2Worker* worker = (LV2Worker*)handle;

    zix_ring_write ( worker->responses, (const char*)&size, sizeof(size));
    zix_ring_write ( worker->responses, (const char*)data, size);

    return LV2_WORKER_SUCCESS;
}


/**
 * Worker logic running in a separate thread (if
 * worker is threaded).
 */
void* worker_func (void* data)
{
    g_message("worker func init");
    LV2Worker*     worker = (LV2Worker*)data;
    lv2_adapter* plugin = worker->plugin;
    void*          buf    = NULL;

    while (true) {
        g_message("worker func waiting");
        zix_sem_wait(&worker->sem);

        g_message("worker func done waiting");
        if (worker->halting) {
            break;
        }

        uint32_t size = 0;
        zix_ring_read(worker->requests, (char*)&size, sizeof(size));
        g_message("worker func request ring read 1");

        if (!(buf = realloc(buf, size))) {
            g_warning ("error: realloc() failed");
            free (buf);
            return NULL;
        }
g_message("worker func ring read 2");
        zix_ring_read(worker->requests, (char*)buf, size);
        zix_sem_wait(&plugin->work_lock);
        g_message("worker func ring wait 2");

        worker->iface->work(plugin->lilvInstance->lv2_handle,
                            lv2_worker_respond, worker, size, buf);

        zix_sem_post(&plugin->work_lock);
    }

    free(buf);
    return NULL;
}


void lv2_worker_init (
    lv2_adapter*               plugin,
    LV2Worker*                   worker,
    const LV2_Worker_Interface * iface,
    bool                         threaded)
{
    g_return_if_fail (plugin && worker && iface);

    worker->iface = iface;
    worker->threaded = threaded;

    if (threaded) {
        zix_thread_create(&worker->thread, 4096, &worker_func, worker);
        worker->requests = zix_ring_new(4096);
        zix_ring_mlock (worker->requests);
    }

    worker->responses = zix_ring_new(4096);
    worker->response  = malloc(4096);
    zix_ring_mlock (worker->responses);
}


/**
 * Stops the worker and frees resources.
 */
void lv2_worker_finish(LV2Worker* worker) {
    worker->halting = true;
    if (worker->requests) {
        if (worker->threaded) {
            zix_sem_post(&worker->sem);
            zix_thread_join(worker->thread, NULL);
            zix_ring_free(worker->requests);
        }
        zix_ring_free(worker->responses);
        free(worker->response);
    }
}


/**
 * Called from plugins during run() to request that
 * Zrythm calls the work() method in a non-realtime
 * context with the given arguments.
 */
LV2_Worker_Status lv2_worker_schedule(
    LV2_Worker_Schedule_Handle handle,
    uint32_t                   size,
    const void*                data )
{
    LV2Worker* worker      = (LV2Worker*) handle;
    lv2_adapter* plugin  = worker->plugin;

    if (!worker->threaded) {

        /* Execute work immediately in this thread */
        zix_sem_wait (&plugin->work_lock);
        worker->iface->work (plugin->lilvInstance->lv2_handle, lv2_worker_respond, worker, size, data);
        zix_sem_post (&plugin->work_lock);

    } else {

        /* Schedule a request to be executed by the worker thread */
        zix_ring_write(worker->requests, (const char*)&size, sizeof(size));
        zix_ring_write(worker->requests, (const char*)data, size); zix_sem_post (&worker->sem);

    }

    return LV2_WORKER_SUCCESS;
}


/**
 * Called during run() to process worker replies.
 *
 * Internally calls work_response in
 * https://lv2plug.in/doc/html/group__worker.html.
 */
void
lv2_worker_emit_responses (
    LV2Worker* worker,
    LilvInstance* instance)
{
    if (worker->responses) {
        uint32_t read_space = zix_ring_read_space (worker->responses);

        while (read_space) {
            uint32_t size = 0;
            zix_ring_read(worker->responses, (char*)&size, sizeof(size));

            zix_ring_read(worker->responses, (char*)worker->response, size);

            worker->iface->work_response (instance->lv2_handle, size, worker->response);

            read_space -= sizeof(size) + size;
        }
    }
}
