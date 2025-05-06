/**
 * @file threads_plugin.h
 * @brief Exposes threading functionality to other plugins via CoreContext.
 *
 * This plugin provides a cooperative thread pool and job queue system, allowing plugins
 * to offload work to background threads. It wraps `tinycthread` for platform-agnostic threading.
 */

 #ifndef _THREADS_PLUGIN_H
 #define _THREADS_PLUGIN_H
 
 #include "../../include/plugin_api.h"
 #include "../../external/tinycthread/source/tinycthread.h"
 
 /**
  * @brief Spawns a background thread to run the given function with user data.
  *
  * If there is a free slot in the internal thread pool, the job runs in a background thread.
  * If the pool is full, the job is either queued or optionally executed synchronously (based on implementation).
  *
  * @param fn         Function pointer of the form `int my_job(void* user_data)`.
  * @param user_data  Pointer to arbitrary user data passed into the job.
  */
 void thread_spawn(int (*fn)(void*), void* user_data);
 
 #endif /* _THREADS_PLUGIN_H */
 