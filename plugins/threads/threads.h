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


 #pragma once
 /**
  * @file threads_plugin.h
  * @brief Defines keys exported by the Threads plugin for CoreContext binding.
  */
 
 /** 
  * @brief Spawns a background thread.
  * 
  * @signature void (*)(int (*fn)(void*), void* user_data)
  */
 #define CC_THREAD_SPAWN           "thread::spawn"
 
 // ---------------- Raw Thread Functions ----------------
 
 /**
  * @brief Creates a raw thread.
  * 
  * @signature int (*)(thrd_t* thr, thrd_start_t func, void* arg)
  */
 #define CC_THREAD_CREATE          "thread::raw::create"
 
 /**
  * @brief Joins a thread.
  * 
  * @signature int (*)(thrd_t thr, int* result)
  */
 #define CC_THREAD_JOIN            "thread::raw::join"
 
 /**
  * @brief Detaches a thread.
  * 
  * @signature int (*)(thrd_t thr)
  */
 #define CC_THREAD_DETACH          "thread::raw::detach"
 
 /**
  * @brief Sleeps the thread for the specified duration.
  * 
  * @signature int (*)(const struct timespec* duration, struct timespec* remaining)
  */
 #define CC_THREAD_SLEEP           "thread::raw::sleep"
 
 /**
  * @brief Exits the current thread with a result code.
  * 
  * @signature void (*)(int result)
  */
 #define CC_THREAD_EXIT            "thread::raw::exit"
 
 /**
  * @brief Yields the current thread.
  * 
  * @signature void (*)(void)
  */
 #define CC_THREAD_YIELD           "thread::raw::yield"
 
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
 