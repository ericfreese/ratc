#pragma once

#include <stdint.h>

typedef struct annotation Annotation;
Annotation *new_annotation(uint32_t start, uint32_t end, const char *type, const char *value);
void annotation_ref_inc(Annotation *a);
void annotation_ref_dec(Annotation *a);
uint32_t annotation_start(Annotation *a);
uint32_t annotation_end(Annotation *a);
const char* annotation_type(Annotation *a);
const char* annotation_value(Annotation *a);
