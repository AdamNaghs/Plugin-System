/**
 * @file signals.h
 * @brief Signal system for event-based communication between plugins.
 *
 * This plugin implements a Godot-style signal system, supporting synchronous and deferred signal emission,
 * allowing decoupled event-driven logic between plugins using `CoreContext`.
 */

 #ifndef _SIGNALS_H
 #define _SIGNALS_H
 
 #include "../../include/plugin_api.h"

 #pragma once
 // signals_plugin.h — Keys and types exported by the Signals plugin
 
 /**
  * Connects a callback function to a named signal.
  * 
  * Signature:
  *   void (*)(const char* signal_name, SignalCallback cb, void* user_data)
  */
 #define CC_SIGNAL_CONNECT "signal::connect"
 
 /**
  * Emits a signal immediately (synchronous).
  * 
  * Signature:
  *   void (*)(CoreContext* ctx, const char* signal_name, void* sender, void* args)
  */
 #define CC_SIGNAL_EMIT "signal::emit"
 
 /**
  * Emits a signal at the end of the current frame (deferred).
  * 
  * Signature:
  *   void (*)(const char* signal_name, void* sender, void* args)
  */
 #define CC_SIGNAL_DEFERRED "signal::emit_deferred"
 
 
 /**
  * @brief Function pointer type for signal callbacks.
  *
  * @param ctx        Pointer to the CoreContext of the system.
  * @param sender     The originator of the signal.
  * @param args       Optional argument payload.
  * @param user_data  Custom user data provided during connection.
  */
 typedef void (*SignalCallback)(CoreContext* ctx, void* sender, void* args, void* user_data);
 
 /**
  * @brief A connection between a signal name and a callback function.
  */
 typedef struct {
     const char* signal_name;        /**< The name of the signal to listen to. */
     SignalCallback callback;        /**< The callback function to invoke when the signal is emitted. */
     void* user_data;                /**< Optional user data to pass to the callback. */
 } SignalConnection;
 
 /**
  * @brief A dynamically resizable list of signal connections.
  */
 typedef struct {
     SignalConnection* data;         /**< Array of signal connections. */
     size_t count;                   /**< Number of active connections. */
     size_t capacity;                /**< Allocated size of the connection array. */
 } SignalConnectionArray;
 
 /**
  * @brief Represents a queued (deferred) signal.
  */
 typedef struct {
     const char* name;               /**< Signal name. */
     void* sender;                   /**< The originator of the signal. */
     void* args;                     /**< Optional argument payload. */
 } QueuedSignal;
 
 /**
  * @brief A dynamically resizable queue of deferred signals.
  */
 typedef struct {
     QueuedSignal* data;             /**< Array of queued signals. */
     size_t count;                   /**< Number of queued signals. */
     size_t capacity;                /**< Allocated size of the queue. */
 } SignalQueueArray;
 
 /**
  * @brief Connects a callback to a named signal.
  *
  * @param name       The name of the signal to listen for.
  * @param cb         The callback function to invoke.
  * @param user_data  Optional user data to be passed to the callback.
  */
 void signal_connect(const char* name, SignalCallback cb, void* user_data);
 
 typedef void (*signal_connect_fn_t)(const char* name, SignalCallback cb, void* user_data);

 /**
  * @brief Emits a signal immediately (synchronously).
  *
  * @param ctx     The CoreContext in which to emit the signal.
  * @param name    The signal name to emit.
  * @param sender  The signal emitter.
  * @param args    Optional argument payload.
  */
 void signal_emit(CoreContext* ctx, const char* name, void* sender, void* args);
 
 typedef void (*signal_emit_fn_t)(CoreContext* ctx, const char* name, void* sender, void* args);

 /**
  * @brief Queues a signal to be emitted on the next frame.
  *
  * @param name    The signal name to emit.
  * @param sender  The signal emitter.
  * @param args    Optional argument payload.
  */
 void signal_emit_deferred(const char* name, void* sender, void* args);

 typedef void (*signal_emit_deferred_fn_t)(const char* name, void* sender, void* args);
 
 /**
  * @brief Flushes all deferred signals by emitting them.
  *
  * Called automatically once per frame by the signal plugin.
  *
  * @param ctx  The current CoreContext.
  */
 void signal_flush(CoreContext* ctx);
 
 #endif /* _SIGNALS_H */
 