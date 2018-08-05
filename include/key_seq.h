#pragma once

typedef struct key_seq KeySeq;

KeySeq *new_key_seq();
void free_key_seq(KeySeq *ks);
void key_seq_add(KeySeq *ks, char *key);
int key_seq_ends_with(KeySeq *ks, KeySeq *end);
char *key_seq_str(KeySeq *ks);
