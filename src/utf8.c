#include <string.h>
#include <utf8proc.h>

int unicode_width(unsigned long c, int tab_size) {
  if (c == '\t')
    return tab_size;

  return utf8proc_charwidth((utf8proc_int32_t) c);
}

static const unsigned char utf8_bytes[256] = {
  1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 4,4,4,4,4,4,4,4, 5,5,5,5,6,6,1,1,
};

unsigned char utf8_char_length(const char *str) {
  return utf8_bytes[*(unsigned char *)str];
}

unsigned long utf8_to_unicode(const char *str, size_t length) {
  unsigned long unicode;

  switch (length) {
  case 1:
    unicode  =   str[0];
    break;
  case 2:
    unicode  =  (str[0] & 0x1f) << 6;
    unicode +=  (str[1] & 0x3f);
    break;
  case 3:
    unicode  =  (str[0] & 0x0f) << 12;
    unicode += ((str[1] & 0x3f) << 6);
    unicode +=  (str[2] & 0x3f);
    break;
  case 4:
    unicode  =  (str[0] & 0x0f) << 18;
    unicode += ((str[1] & 0x3f) << 12);
    unicode += ((str[2] & 0x3f) << 6);
    unicode +=  (str[3] & 0x3f);
    break;
  case 5:
    unicode  =  (str[0] & 0x0f) << 24;
    unicode += ((str[1] & 0x3f) << 18);
    unicode += ((str[2] & 0x3f) << 12);
    unicode += ((str[3] & 0x3f) << 6);
    unicode +=  (str[4] & 0x3f);
    break;
  case 6:
    unicode  =  (str[0] & 0x01) << 30;
    unicode += ((str[1] & 0x3f) << 24);
    unicode += ((str[2] & 0x3f) << 18);
    unicode += ((str[3] & 0x3f) << 12);
    unicode += ((str[4] & 0x3f) << 6);
    unicode +=  (str[5] & 0x3f);
    break;
  default:
    return 0;
  }

  /* Invalid characters could return the special 0xfffd value but NUL
   * should be just as good. */
  return unicode > 0x10FFFF ? 0 : unicode;
}

size_t utf8_length(const char *start, size_t max_width, int tab_size) {
  const char *str = start;
  const char *end = strchr(str, '\0');
  unsigned char last_bytes = 0;

  int width = 0;

  while (str < end) {
    unsigned char bytes = utf8_char_length(str);
    size_t ucwidth;
    unsigned long unicode;

    if (str + bytes > end)
      break;

    unicode = utf8_to_unicode(str, bytes);

    /* Something was encoded incorrectly */
    if (!unicode)
      break;

    ucwidth = unicode_width(unicode, tab_size);
    width += ucwidth;
    if (max_width > 0 && width > max_width) {
      width -= ucwidth;
      break;
    }

    str += bytes;
    if (ucwidth) {
      last_bytes = bytes;
    } else {
      last_bytes += bytes;
    }
  }

  return str - start;
}
