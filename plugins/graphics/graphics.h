#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include "raylib.h"
#include "../include/plugin.h"

typedef enum {
    RENDERABLE_2D,
    RENDERABLE_3D,
} RenderableType;

typedef struct {
    RenderableType type;

    union {
        struct {
            Texture2D* texture;
            Vector2 position;
            float rotation;
            float scale;
            Color tint;
        } r2d;

        struct {
            Model* model;
            // maybe add quaternion
            Vector3 position;
            Vector3 rotation; 
            Vector3 scale;
            Color tint;
        } r3d;
    };
} Renderable;

/**
 * @brief Draws a Renderable object (2D or 3D).
 *
 * @param r Pointer to a Renderable to draw.
 */
void renderable_draw(const Renderable* r);

typedef void (*renderable_draw_fn_t)(const Renderable* r);

#endif /* _GRAPHICS_H */
