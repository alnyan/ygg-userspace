#pragma once

struct object;
typedef void (*object_entry_t) (void *);

struct object *object_open(const char *pathname);
int object_load(struct object *obj);
object_entry_t object_entry_get(struct object *obj);
