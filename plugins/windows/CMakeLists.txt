include (SquashPlugin)

check_include_file("Ntifs.h" HAVE_NTIFS_H)
if(HAVE_NTIFS_H)
  set(SQUASH_WINDOWS_ENABLED_DEFAULT "")
else()
  set(SQUASH_WINDOWS_ENABLED_DEFAULT "DEFAULT_DISABLED")
endif(HAVE_NTIFS_H)

squash_plugin (
  NAME windows
  ${SQUASH_WINDOWS_ENABLED_DEFAULT}
  SOURCES squash-windows.c)
