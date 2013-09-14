#if !defined(_WIN32) && !defined(unix)
#define unix
#endif

/* We want to define NOJIT for architectures other than x86 and amd64.
   Unfortunately there are lots of differences between compilers in
   this area—see http://sourceforge.net/p/predef/wiki/Architectures/
   for a list. */
#if \
  !defined(NOJIT) && \
  !defined(__i386__) && \
  !defined(__i386) && \
  !defined(_M_I86) && \
  !defined(_M_IX86) && \
  !defined(__X86__) && \
  !defined(_X86_) && \
  !defined(__THW_INTEL__) && \
  !defined(__I86__) && \
  !defined(__INTEL__) && \
  !defined(__amd64__) && \
  !defined(_M_X64)
#define NOJIT
#endif
