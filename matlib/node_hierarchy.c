#include "node_hierarchy.h"
#include "cglm/struct.h" // IWYU pragma: keep

Node init_node(){
    Node node;
    node.world_transform = glms_mat4_identity();
    node.dirty = true;
    node.parent = NULL;
    node.child_count = 0;
    node.local_transform.translation = glms_vec3_zero();
    node.local_transform.rotation    = glms_quat_identity();
    node.local_transform.scale       = glms_vec3_one();
    node.model = (Model){0};
    for (size_t i = 0; i < MAX_CHILDREN; i++) {
        node.children[i] = NULL;
    }
    return node;
}

bool set_child(Node* parent, Node* child){
    if(!parent || !child) return false;

    for (size_t c = 0; c < MAX_CHILDREN; c++) {
        if(parent->children[c] == NULL){
            parent->children[c] = child;
            child->parent = parent;
            parent->child_count++;
            child->dirty = true;
            return true;
        }
    }
    return false;
}

static mat4s get_node_world_transform(Node* node){
    if(!node) return glms_mat4_identity();

    mat4s t = glms_translate_make(node->local_transform.translation);
    mat4s r = glms_quat_mat4(node->local_transform.rotation);
    mat4s s = glms_scale_make(node->local_transform.scale);

    mat4s local = glms_mat4_mul(t, glms_mat4_mul(r, s));

    if(node->parent){
        node->world_transform = glms_mat4_mul(node->parent->world_transform, local);
    }
    else{
        node->world_transform = local;
    }

    return node->world_transform;
}

void update_node_transform_hierarchy(Node* node){
    if(!node) return;

    if(node->parent && node->parent->dirty)
        node->dirty = true;

    if(node->dirty)
        node->world_transform = get_node_world_transform(node);

    for (short i = 0; i < node->child_count; i++) {
        if(node->children[i])
            update_node_transform_hierarchy(node->children[i]);
    }

    node->dirty = false;    
}

void draw_node(Node* node, Camera* cam, Color color){
    if(!node || !cam)return;

    if(node->parent){
        node->model.transform= node->world_transform;
        draw_model(&node->model, cam, color);
    }
    for (short i = 0; i < node->child_count; i++) {
        if(node->children[i])
            draw_node(node->children[i], cam, color);
    }
}
