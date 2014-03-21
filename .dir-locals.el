((nil . ((indent-tabs-mode . nil)
         (c-basic-offset . 2)
         (tab-width . 2))))

(setq
 ac-clang-flags
 (mapconcat
  (function (lambda (x) (format "-I%s" x)))
  (list (file-name-directory (dir-locals-find-file ".")))
  " "))
