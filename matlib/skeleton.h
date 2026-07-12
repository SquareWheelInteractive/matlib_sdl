#pragma once
#include <stdint.h>
#include <stddef.h>
#include "cglm/struct.h" // IWYU pragma: keep
#include "external/cgltf.h"

#define MAX_BONES      128
#define MAX_CLIPS      32
#define MAX_CLIP_NAME  32

/* -----------------------------------------------------------------------
   One keyframe track for a single bone channel.
   Each channel drives exactly one of: translation, rotation, or scale.
   ----------------------------------------------------------------------- */
typedef enum {
    CHANNEL_TRANSLATION,
    CHANNEL_ROTATION,
    CHANNEL_SCALE,
} ChannelType;

typedef struct {
    ChannelType  type;
    uint8_t      bone_index;   /* index into Skeleton.bones[] */

    float*       times;        /* [keyframe_count] */
    float*       values;       /* [keyframe_count * components]
                                  translation/scale: 3 floats
                                  rotation:          4 floats (xyzw) */
    uint32_t     keyframe_count;
} AnimChannel;

typedef struct {
    char         name[MAX_CLIP_NAME];
    float        duration;  /* seconds */

    AnimChannel* channels;
    uint32_t     channel_count;
} AnimClip;

typedef struct {
    mat4s    inverse_bind;
    int16_t  parent_index;  /* -1 means root. */
} Bone;

typedef struct {
    int32_t  clip_index;   /* which clip is playing, -1 = none */
    float    time;         /* current playback time in seconds  */
    int      looping;
} AnimState;

typedef struct {
    Bone       bones[MAX_BONES];
    uint32_t   bone_count;

    AnimClip   clips[MAX_CLIPS];
    uint32_t   clip_count;

    AnimState anim_state;

    mat4s      final_matrices[MAX_BONES];
} Skeleton;

void skeleton_free(Skeleton* sk);
bool skeleton_play(Skeleton* sk, const char* clip_name, int loop);
float skeleton_advance(Skeleton* sk, float dt);
void skeleton_update(Skeleton* sk, float time);
void skeleton_free(Skeleton* sk);
void skeleton_load(Skeleton* sk, cgltf_data* data, unsigned short skin_index);
bool skeleton_play_index(Skeleton* sk, int index, int loop);
