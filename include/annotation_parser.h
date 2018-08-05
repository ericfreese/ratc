#ifndef ANNOTATION_PARSER_H
#define ANNOTATION_PARSER_H

#include <stdlib.h>

#include "annotation.h"

typedef struct annotation_parser AnnotationParser;
AnnotationParser *new_annotation_parser();
void free_annotation_parser(AnnotationParser *ap);
void annotation_parser_write(AnnotationParser *ap, const char *buf, size_t len);
Annotation *annotation_parser_read(AnnotationParser *ap);

#endif
