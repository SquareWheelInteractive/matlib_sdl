#pragma once
#include "cglm/struct.h" // IWYU pragma: keep

typedef struct {
    char name[64];

    int parent;          // parent bone index (-1 if root)

    mat4s inverse_bind;
} Bone;

typedef struct {
    Bone* bones;
    int bone_count;
} Skeleton;

typedef struct {
    float time;
    vec3s value;
} PositionKey;

typedef struct {
    float time;
    versors value;
} RotationKey;

typedef struct {
    float time;
    vec3s value;
} ScaleKey;

typedef struct {
    PositionKey* positions;
    int position_count;

    RotationKey* rotations;
    int rotation_count;

    ScaleKey* scales;
    int scale_count;
} BoneAnimation;

typedef struct {
    char name[64];

    float duration;

    BoneAnimation* bones;
} Animation;

struct Transform;
typedef struct {
    Animation* animation;

    float time;

    struct Transform* local_pose;    // one Transform per bone

    mat4s* global_pose;

    mat4s* final_matrices;
} Animator;
