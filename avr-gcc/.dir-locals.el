((c-mode (eval . (progn (setq
                         flycheck-gcc-include-path (map 'list 'expand-file-name (list "~/iproject/avr-gcc/inc/" "/usr/lib/avr/include"))
                         flycheck-gcc-args (list "-mmcu=atmega328p" "-Os" "-Wall")
                         flycheck-c/c++-gcc-executable '"/usr/bin/avr-gcc"
                         flycheck-disabled-checkers (list 'c/c++-clang 'irony)
                         company-clang-arguments (list "-I/home/luctins/iproject/avr-gcc/inc/"))))))
