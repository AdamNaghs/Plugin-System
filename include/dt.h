/**
 * @file dt.h
 * @brief Provides delta time (frame time) tracking functions for the engine.
 *
 * This module offers a simple mechanism to update and retrieve frame delta time.
 * It can either use a platform-specific timer or integrate with a game framework like Raylib.
 */

 #ifndef _COMMON_H
 #define _COMMON_H
 
 /**
  * @brief Updates the internal delta time value.
  *
  * Should be called once per frame before any time-dependent logic (e.g., in the main loop).
  * The implementation may differ based on whether the platform uses a game framework like Raylib.
  */
 void dt_update(void);
 
 /**
  * @brief Retrieves the time in seconds since the last frame.
  *
  * @return A float representing the elapsed time in seconds.
  */
 float get_dt(void);
 
 #endif /* _COMMON_H */
 