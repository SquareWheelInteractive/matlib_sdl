#include "skeleton.h"
#include <string.h>
#include <math.h>
#include "glad/glad.h"

static int node_to_bone_index(cgltf_skin* skin, cgltf_node* node) {
    for (size_t i = 0; i < skin->joints_count; i++)
        if (skin->joints[i] == node) return (int)i;
    return -1;
}

/* -----------------------------------------------------------------------
   Fills sk->bones[], sk->clips[] from cgltf_data.
   skin_index  – which skin in data->skins[] to use (usually 0)
   ----------------------------------------------------------------------- */
void skeleton_load(Skeleton* sk, cgltf_data* data, unsigned short skin_index) {
    memset(sk, 0, sizeof(Skeleton));
    sk->anim_state.clip_index = -1;

    cgltf_skin* skin = &data->skins[skin_index];
    sk->bone_count   = (uint32_t)skin->joints_count;

    /* ---- 1. Bones: inverse bind matrices + parent index --------------- */
    for (uint32_t i = 0; i < sk->bone_count; i++) {
        cgltf_node* joint = skin->joints[i];

        /* inverse bind matrix */
        float ibm[16];
        cgltf_accessor_read_float(skin->inverse_bind_matrices, i, ibm, 16);
        memcpy(sk->bones[i].inverse_bind.raw, ibm, sizeof(float) * 16);

        /* parent: find the joint whose cgltf_node is this node's parent */
        sk->bones[i].parent_index = -1;
        if (joint->parent) {
            int p = node_to_bone_index(skin, joint->parent);
            sk->bones[i].parent_index = (int16_t)p;
        }
    }

    /* ---- 2. Animation clips ------------------------------------------ */
    sk->clip_count = 0;
    for (size_t a = 0; a < data->animations_count && sk->clip_count < MAX_CLIPS; a++) {
        cgltf_animation* anim = &data->animations[a];
        AnimClip* clip = &sk->clips[sk->clip_count++];

        /* name */
        if (anim->name)
            snprintf(clip->name, MAX_CLIP_NAME, "%s", anim->name);
        else
            snprintf(clip->name, MAX_CLIP_NAME, "clip_%zu", a);

        /* duration = max time across all samplers */
        clip->duration = 0.0f;
        for (size_t s = 0; s < anim->samplers_count; s++) {
            cgltf_accessor* input = anim->samplers[s].input;
            float t;
            cgltf_accessor_read_float(input, input->count - 1, &t, 1);
            if (t > clip->duration) clip->duration = t;
        }

        /* channels */
        clip->channel_count = (uint32_t)anim->channels_count;
        clip->channels = calloc(clip->channel_count, sizeof(AnimChannel));

        for (uint32_t c = 0; c < clip->channel_count; c++) {
            cgltf_animation_channel* src = &anim->channels[c];
            AnimChannel* dst = &clip->channels[c];

            /* bone index */
            int bi = node_to_bone_index(skin, src->target_node);
            if (bi < 0) { dst->keyframe_count = 0; continue; } /* non-joint node */
            dst->bone_index = (uint8_t)bi;

            /* channel type */
            if(src->target_path == cgltf_animation_path_type_translation)
                dst->type = CHANNEL_TRANSLATION;
            else if(src->target_path == cgltf_animation_path_type_rotation)
                dst->type = CHANNEL_ROTATION;
            else if(src->target_path == cgltf_animation_path_type_scale)
                dst->type = CHANNEL_SCALE;
            else{ dst->keyframe_count = 0; continue; }

            /* keyframe data */
            cgltf_accessor* input  = src->sampler->input;
            cgltf_accessor* output = src->sampler->output;
            int comp = (dst->type == CHANNEL_ROTATION) ? 4 : 3;

            dst->keyframe_count = (uint32_t)input->count;
            dst->times  = malloc(sizeof(float) * dst->keyframe_count);
            dst->values = malloc(sizeof(float) * dst->keyframe_count * comp);

            for (uint32_t k = 0; k < dst->keyframe_count; k++) {
                cgltf_accessor_read_float(input,  k, &dst->times[k], 1);
                cgltf_accessor_read_float(output, k, &dst->values[k * comp], comp);
            }
        }
    }

    /* auto-play first clip looping if any exist */
    if (sk->clip_count > 0) {
        sk->anim_state.clip_index = 0;
        sk->anim_state.looping    = 1;
        sk->anim_state.time       = 0.0f;
    }
}
/* -----------------------------------------------------------------------
   Internal: sample one channel at time t → writes into out[].
   ----------------------------------------------------------------------- */
static void sample_channel(const AnimChannel* ch, float t, float* out) {
    int comp  = (ch->type == CHANNEL_ROTATION) ? 4 : 3;
    int count = (int)ch->keyframe_count;

    if(count <= 0 || ch->times == NULL || ch->values == NULL){
        memset(out, 0, sizeof(float)*comp);
        return;
    }

    if (t <= ch->times[0]) { memcpy(out, ch->values, sizeof(float) * comp); return; }
    if (t >= ch->times[count - 1]) { memcpy(out, ch->values + (count-1)*comp, sizeof(float)*comp); return; }

    for (int i = 0; i < count - 1; i++) {
        // check between which 2 keyframes t is
        if (t >= ch->times[i] && t < ch->times[i + 1]) {
            float range = ch->times[i + 1] - ch->times[i];
            float f     = (range > 0.0f) ? (t - ch->times[i]) / range : 0.0f;
            const float* a = ch->values + i * comp;
            const float* b = ch->values + (i + 1) * comp;

            if (ch->type == CHANNEL_ROTATION) {
                // slerp for rotations
                versors qa = {{a[0], a[1], a[2], a[3]}};
                versors qb = {{b[0], b[1], b[2], b[3]}};
                versors result = glms_quat_slerp(qa, qb, f);
                out[0] = result.x;
                out[1] = result.y;
                out[2] = result.z;
                out[3] = result.w;
            } else {
                // lerp translation and scale ( both are vec3 )
                vec3s va = (vec3s){a[0], a[1], a[2]};
                vec3s vb = (vec3s){b[0], b[1], b[2]};
                vec3s result  = glms_vec3_lerp(va, vb, t);
                out[0] = result.x;
                out[1] = result.y;
                out[2] = result.z;
            }
            return;
        }
    }
}

/* -----------------------------------------------------------------------
   Internal: local TRS matrix for bone[i] at time t in the current clip.
   ----------------------------------------------------------------------- */
static mat4s bone_local_matrix(const Skeleton* sk, uint32_t bone_idx, const AnimClip* clip, float t) {
    vec3s   T = glms_vec3_zero();
    versors R = glms_quat_identity();
    vec3s   S = glms_vec3_one();

    for (uint32_t i = 0; i < clip->channel_count; i++) {
        const AnimChannel* ch = &clip->channels[i];
        if (ch->bone_index != bone_idx) continue;

        float out[4];
        sample_channel(ch, t, out);

        if (ch->type == CHANNEL_TRANSLATION) {
            T = (vec3s){{out[0], out[1], out[2]}};
        } else if (ch->type == CHANNEL_ROTATION) {
            R = (versors){{out[0], out[1], out[2], out[3]}};
        } else if (ch->type == CHANNEL_SCALE) {
            S = (vec3s){{out[0], out[1], out[2]}};
        }
    }

    mat4s m = glms_mat4_identity();
    m = glms_translate(m, T);
    m = glms_mat4_mul(m, glms_quat_mat4(R));
    m = glms_scale(m, S);
    return m;
}

/* -----------------------------------------------------------------------
   Writes skeleton->final_matrices[], which you then upload to the shader.

   time   – current playback time in seconds (you manage looping / clamping
             however you like, or use skeleton_advance() below)
   ----------------------------------------------------------------------- */
void skeleton_update(Skeleton* sk, float time) {
    if (sk->anim_state.clip_index < 0 || sk->anim_state.clip_index >= (int32_t)sk->clip_count) {
        for (size_t i = 0; i < sk->bone_count; i++)
            sk->final_matrices[i] = glms_mat4_identity();
        return;
    }

    const AnimClip* clip = &sk->clips[sk->anim_state.clip_index];

    mat4s world[MAX_BONES];

    for (size_t i = 0; i < sk->bone_count; i++) {
        mat4s local = bone_local_matrix(sk, i, clip, time);

        int16_t p = sk->bones[i].parent_index;
        if (p < 0)
            world[i] = local;
        else
            world[i] = glms_mat4_mul(world[p], local);

        sk->final_matrices[i] = glms_mat4_mul(world[i], sk->bones[i].inverse_bind);
    }
}

/* -----------------------------------------------------------------------
   Advances playback by dt seconds and loops if needed.
   Returns the new time so you can pass it straight to skeleton_update().
   ----------------------------------------------------------------------- */
float skeleton_advance(Skeleton* sk, float dt) {
    if (sk->anim_state.clip_index < 0 || sk->anim_state.clip_index >= sk->clip_count) return 0.0f;

    AnimClip* clip = &sk->clips[sk->anim_state.clip_index];
    sk->anim_state.time += dt;
    if (sk->anim_state.looping && sk->anim_state.time > clip->duration)
        sk->anim_state.time = fmodf(sk->anim_state.time, clip->duration);
    return sk->anim_state.time;
}

/* -----------------------------------------------------------------------
   Switch to a named clip by name.  Returns true on success, false if not found.
   ----------------------------------------------------------------------- */
bool skeleton_play(Skeleton* sk, const char* clip_name, int loop) {
    for (size_t i = 0; i < sk->clip_count; i++) {
        if (strcmp(sk->clips[i].name, clip_name) == 0) {
            sk->anim_state.clip_index = (int32_t)i;
            sk->anim_state.time       = 0.0f;
            sk->anim_state.looping    = loop;
            return true;
        }
    }
    return false;   /* clip not found */
}
/* -----------------------------------------------------------------------
   Switch to a named clip by index.  Returns true on success, false if not found.
   ----------------------------------------------------------------------- */
bool skeleton_play_index(Skeleton* sk, int index, int loop) {
    if (index < sk->clip_count) {
        sk->anim_state.clip_index = index;
        sk->anim_state.time       = 0.0f;
        sk->anim_state.looping    = loop;
        return true;
    }
    return false;   /* clip not found */
}

void skeleton_free(Skeleton* sk) {
    if(!sk) return;
    
    for (uint32_t c = 0; c < sk->clip_count; c++) {
        AnimClip* clip = &sk->clips[c];

        if(clip->channels == NULL) continue;

        for (size_t i = 0; i < clip->channel_count; i++) {
            if(clip->channels[i].times != NULL)
                free(clip->channels[i].times);
            if(clip->channels[i].values != NULL)
                free(clip->channels[i].values);
        }
        free(clip->channels);
    }
    memset(sk, 0, sizeof(Skeleton));
    sk->anim_state.clip_index = -1;
}
