#pragma once

#include "matlib.h"

#define MAX_CHILDREN 8

typedef struct Node{
    Transform local_transform;
    mat4s world_transform;
    bool dirty;
    struct Node* parent;
    struct Node* children[MAX_CHILDREN];
    unsigned char child_count;

    Model model;
}Node;

Node init_node();
void update_node_transform_hierarchy(Node* node);
bool set_child(Node* parent, Node* child);
void draw_node(Node* no, Camera* cam, Color color);
