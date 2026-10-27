#pragma once

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL3_GL3_IMPLEMENTATION
#include "Nuklear/nuklear.h"
#include "Nuklear/nuklear_sdl3_gl3.h"

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

#include "renderer/renderer.h"

typedef struct {
    struct nk_colorf color;
    float intensity;
    float range;
    nk_bool enabled;
} UILight;
typedef struct{
    UILight lights[MAX_LIGHTS];
    struct nk_colorf ambient;
    struct nk_colorf fog_color;
    float fog_density;
} RendererUIState;

void init_ui_renderer(RendererUIState* ui){
    ui->ambient   = (struct nk_colorf){1.0f, 1.0f, 1.0f, 1.0f};
    ui->fog_color = (struct nk_colorf){0.0f, 0.0f, 0.0f, 1.0f};
    ui->fog_density = 0.08f;

    for (int i = 0; i < MAX_LIGHTS; i++) {
        ui->lights[i].color = (struct nk_colorf){1.0f, 1.0f, 1.0f, 1.0f};
        ui->lights[i].intensity = 1.0f;
        ui->lights[i].range = 5.0f;
        ui->lights[i].enabled = false;
    }
}

void draw_ui_renderer(struct nk_context* ctx, Renderer* renderer, RendererUIState* ui){
    if (nk_begin(ctx, "Demo", nk_rect(12, 12, 230, 400),
    NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
    NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
    {
        // ambient
        nk_layout_row_static(ctx, 25, 50,1);
            nk_label(ctx, "Ambient", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 80, 1);
            ui->ambient = nk_color_picker(ctx, ui->ambient, NK_RGB);

        // fog color
        nk_layout_row_static(ctx, 25, 80,1);
            nk_label(ctx, "Fog Color", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 80, 1);
            ui->fog_color = nk_color_picker(ctx, ui->fog_color, NK_RGB);

        // fog density
        nk_layout_row_static(ctx, 25, 120, 1);
            nk_labelf(ctx, NK_TEXT_LEFT, "Fog density: %.3f", ui->fog_density);
        nk_layout_row_dynamic(ctx, 10, 1);
            nk_slider_float(ctx, 0.0f, &ui->fog_density, 0.5f, 0.01f);

        // --- Lights Section ---
        nk_layout_row_static(ctx, 25, 200, 1);
        nk_label(ctx, "Lights", NK_TEXT_LEFT);

        for (int i = 0; i < renderer->light_count; i++) {
            Light curr_light = renderer->lights[i];
            if(curr_light.id == INVALID_LIGHT) continue;

            char light_title[32];
            switch (curr_light.type) {
                case LIGHT_TYPE_DIRECTIONAL:
                    snprintf(light_title, sizeof(light_title), "Directional light id: %d", curr_light.id);
                    break;
                case LIGHT_TYPE_SPOT:
                    snprintf(light_title, sizeof(light_title), "Spot light id: %d", curr_light.id);
                    break;
                case LIGHT_TYPE_POINT:
                    snprintf(light_title, sizeof(light_title), "Point light id: %d", curr_light.id);
                    break;
            }

            UILight* ui_light = &ui->lights[i];
            if (nk_tree_push_id(ctx, NK_TREE_NODE, light_title, NK_MINIMIZED, curr_light.id)) {
                nk_layout_row_dynamic(ctx, 80, 1);
                ui_light->color  = nk_color_picker(ctx, ui_light->color, NK_RGB);
                nk_layout_row_dynamic(ctx, 15, 1);
                    nk_property_float(ctx, "Intensity", 0.0f, &ui_light->intensity, 10.0f, 0.1f, 0.01f);
                nk_layout_row_dynamic(ctx, 15, 1);
                    nk_property_float(ctx, "Range", 0.0f, &ui_light->range, 25.0f, 0.1f, 0.01f);
                    ui_light->enabled = nk_check_label(ctx, "Enable", ui_light->enabled);
                nk_tree_pop(ctx);
            }
        }
    }
    nk_end(ctx);

    renderer->ambient = (Color){ui->ambient.r, ui->ambient.g, ui->ambient.b, 1.0f};
    renderer->fog.color = (Color){ui->fog_color.r, ui->fog_color.g, ui->fog_color.b, 1.0f};
    renderer->fog.density = ui->fog_density;

    for (int i = 0; i < renderer->light_count; i++) {
        Light* light = &renderer->lights[i];
        UILight ui_light = ui->lights[i];

        light->color = (Color){ui_light.color.r,
                               ui_light.color.g,
                               ui_light.color.b,
                               1.0f};
        light->intensity = ui_light.intensity;
        light->range = ui_light.range;
        light->enabled = !ui_light.enabled;
    }
}
