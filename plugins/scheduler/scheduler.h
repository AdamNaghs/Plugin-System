/**
 * @file scheduler.h
 * @brief A simple plugin-based task scheduler for periodic function execution.
 *
 * This scheduler allows plugins to register recurring tasks that run at fixed intervals.
 * Each task is associated with a callback function and optional user data.
 */

 #ifndef _SCHEDULER
 #define _SCHEDULER
 
 #include "../../include/plugin_api.h"

 #define CC_SCHEDULER_REGISTER "scheduler::register"
 
 /**
  * @brief Function pointer type for scheduled tasks.
  *
  * @param ctx        Pointer to the shared CoreContext.
  * @param user_data  Optional user data provided at registration.
  */
 typedef void (*ScheduledFn)(CoreContext* ctx, void* user_data);
 
 /**
  * @brief Represents a single scheduled task.
  */
 typedef struct {
     const char* name;        /**< The name of the task (optional, for debugging/logging). */
     float interval;          /**< Time interval in seconds between calls. */
     float elapsed;           /**< Internal tracker for time since last call. */
     ScheduledFn fn;          /**< The function to call at the specified interval. */
     void* user_data;         /**< Optional data passed to the callback. */
 } ScheduledTask;
 
 /**
  * @brief A resizable list of scheduled tasks.
  */
 typedef struct {
     ScheduledTask* tasks;    /**< Array of scheduled tasks. */
     size_t count;            /**< Number of active tasks. */
     size_t capacity;         /**< Allocated capacity of the task array. */
 } Scheduler;
 
 /**
  * @brief Registers a new task to run periodically.
  *
  * @param name       Optional name for the task (used for debugging).
  * @param interval   Time interval in seconds between executions.
  * @param fn         Function pointer to be called on each interval.
  * @param user_data  Optional pointer to user-defined data.
  */
 void scheduler_register(const char* name, float interval, ScheduledFn fn, void* user_data);
 
 #endif /* _SCHEDULER */
 