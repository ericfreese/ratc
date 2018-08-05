#ifndef ANNOTATOR_H
#define ANNOTATOR_H

typedef struct annotator Annotator;
Annotator *new_annotator(char *cmd, char *annotation_type);
void annotator_ref_inc(Annotator *ar);
void annotator_ref_dec(Annotator *ar);
const char *annotator_type(Annotator *ar);
const char *annotator_command(Annotator *ar);

#endif
