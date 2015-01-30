#ifndef __TEST_CODECS_H__
#define __TEST_CODECS_H__

#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include <squash/squash.h>

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define SQUASH_ASSERT_VALUE(expr, expected)           \
  do {                                                \
    if ((expr) != (expected)) {                       \
      fprintf (stderr, "%s:%d: %s != %s\n",           \
               __FILE__, __LINE__, #expr, #expected); \
      exit (SIGTRAP);                                 \
    }                                                 \
  } while (0)

static inline void
squash__assert_status (SquashStatus value, SquashStatus status, const char* file, const int line) {
  if (value != status) {
      fprintf (stderr, "%s:%d: %s (%d), expected %s (%d)\n",
               file, line,
               squash_status_to_string (value), value,
               squash_status_to_string (status), status);
      exit (SIGTRAP);
  }
}

#define SQUASH_ASSERT_STATUS(expr, status) \
  squash__assert_status ((expr), (status), __FILE__, __LINE__)

#define SQUASH_ASSERT_OK(expr) \
  SQUASH_ASSERT_STATUS(expr, SQUASH_OK)

#define SQUASH_ASSERT_NO_ERROR(expr)                                    \
  do {                                                                  \
    if ((expr) < 0) {                                                   \
      fprintf (stderr, "%s:%d: %s (%d)\n",__FILE__, __LINE__, squash_status_to_string ((expr)), (expr)); \
      exit (SIGTRAP);                                                   \
    }                                                                   \
  } while (0)

static inline void squash__assert (bool e, const char* expr, const char* file, int line) {
  if (!e) {
    fprintf (stderr, "%s:%d: assertion (%s) failed\n", file, line, expr);
    exit (SIGTRAP);
  }
}

#define SQUASH_ASSERT(expr) \
  squash__assert ((expr), #expr, __FILE__, __LINE__)

#define LOREM_IPSUM                                                     \
  "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed vulputate " \
  "lectus nisl, vitae ultricies justo dictum nec. Vestibulum ante ipsum " \
  "primis in faucibus orci luctus et ultrices posuere cubilia Curae; "  \
  "Suspendisse suscipit quam a lectus adipiscing, sed tempor purus "    \
  "cursus. Vivamus id nulla eget elit eleifend molestie. Integer "      \
  "sollicitudin lorem enim, eu eleifend orci facilisis sed. Pellentesque " \
  "sodales luctus enim vel viverra. Cras interdum vel nisl in "         \
  "facilisis. Curabitur sollicitudin tortor vel congue "                \
  "auctor. Suspendisse egestas orci vitae neque placerat blandit.\n"    \
  "\n"                                                                  \
  "Aenean sed nisl ultricies, vulputate lorem a, suscipit nulla. Donec " \
  "egestas volutpat neque a eleifend. Nullam porta semper "             \
  "nunc. Pellentesque adipiscing molestie magna, quis pulvinar metus "  \
  "gravida sit amet. Vestibulum mollis et sapien eu posuere. Quisque "  \
  "tristique dignissim ante et aliquet. Phasellus vulputate condimentum " \
  "nulla in vulputate.\n"                                               \
  "\n"                                                                  \
  "Nullam volutpat tellus at nisi auctor, vitae mattis nibh viverra. Nunc " \
  "vitae lectus tristique, ultrices nibh quis, lobortis elit. Curabitur " \
  "at vestibulum nisi, nec facilisis ante. Nulla pharetra blandit lacus, " \
  "at sodales nulla placerat eget. Nulla congue varius tortor, sit amet " \
  "tempor est mattis nec. Praesent vitae tristique ipsum, rhoncus "     \
  "tristique lorem. Sed et erat tristique ligula accumsan fringilla eu in " \
  "urna. Donec dapibus hendrerit neque nec venenatis. In euismod sapien " \
  "ipsum, auctor consectetur mi dapibus hendrerit.\n"                   \
  "\n"                                                                  \
  "Phasellus sagittis rutrum velit, in sodales nibh imperdiet a. Integer " \
  "vitae arcu blandit nibh laoreet scelerisque eu sit amet eros. Aenean " \
  "odio felis, aliquam in eros at, ornare luctus magna. In semper "     \
  "tincidunt nunc, sollicitudin gravida nunc laoreet eu. Cras eu tempor " \
  "sapien, ut dignissim elit. Proin eleifend arcu tempus, semper erat et, " \
  "accumsan erat. Praesent vulputate diam mi, eget mollis leo "         \
  "pellentesque eget. Aliquam eu tortor posuere, posuere velit sed, "   \
  "suscipit eros. Nam eu leo vitae mauris condimentum lobortis non quis " \
  "mauris. Nulla venenatis fringilla urna nec venenatis. Nam eget velit " \
  "nulla. Proin ut malesuada felis. Suspendisse vitae nunc neque. Donec " \
  "faucibus tempor lacinia. Vivamus ac vulputate sapien, eget lacinia " \
  "nisl.\n"                                                             \
  "\n"                                                                  \
  "Curabitur eu dolor molestie, ullamcorper lorem quis, egestas "       \
  "urna. Suspendisse in arcu sed justo blandit condimentum. Ut auctor, " \
  "sem quis condimentum mattis, est purus pulvinar elit, quis viverra " \
  "nibh metus ac diam. Etiam aliquet est eu dui fermentum consequat. Cras " \
  "auctor diam eget bibendum sagittis. Aenean elementum purus sit amet " \
  "sem euismod, non varius felis dictum. Aliquam tempus pharetra ante a " \
  "sagittis. Curabitur ut urna felis. Etiam sed vulputate nisi. Praesent " \
  "at libero eleifend, sagittis quam a, varius sapien."
#define LOREM_IPSUM_LENGTH (sizeof (LOREM_IPSUM) - 1)

void check_codec (SquashCodec* codec);

#endif /* __TEST_CODECS_H__ */
