#pragma once

typedef enum {
  ESP_UNKNOWN = 0,
  ESP_ESC,
  ESP_FE,
  ESP_PRIV_PARAM,
  ESP_PARAM_NUM,
  ESP_PARAM_SEP,
  ESP_SEP,
  ESP_INTER,
  ESP_FINAL
} EscSeqPartType;

typedef struct esc_seq_part EscSeqPart;
struct esc_seq_part {
  EscSeqPart *next;
  EscSeqPartType type;
  const char *value;
};

typedef struct esc_seq EscSeq;
struct esc_seq {
  EscSeqPart *first;
  EscSeqPart *last;
};

EscSeqPart *new_esc_seq_part(EscSeqPartType type, const char *value);
void free_esc_seq_part(EscSeqPart *esp);

EscSeq *new_esc_seq();
void free_esc_seq(EscSeq *es);
void esc_seq_add_part(EscSeq *es, EscSeqPart *esp);
