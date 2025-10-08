;; emacs mode for the Em programming language (.em files)
;; based on the simpc-mode.el (given by Tsoding)

(require 'subr-x)

(defvar em-mode-syntax-table
  (let ((table (make-syntax-table)))
    ;; C/C++ style comments
	(modify-syntax-entry ?/ ". 124b" table)
	(modify-syntax-entry ?* ". 23" table)
	(modify-syntax-entry ?\n "> b" table)
    ;; Preprocessor stuff?
    (modify-syntax-entry ?# "." table)
    ;; Chars are the same as strings
    (modify-syntax-entry ?' "\"" table)
    ;; Treat <> as punctuation (needed to highlight C++ keywords
    ;; properly in template syntax)
    (modify-syntax-entry ?< "." table)
    (modify-syntax-entry ?> "." table)

    (modify-syntax-entry ?& "." table)
    (modify-syntax-entry ?% "." table)
    table))

(defun em-types ()
  '("void" "bool" "char" "int" "float" "string"))

(defun em-keywords ()
  '("if" "else" "for" "while" "return" "break" "continue" "true" "false"))

(defun em-font-lock-keywords ()
  (list
   ;; preprocessors
   `("# *[#a-zA-Z0-9_]+" . font-lock-preprocessor-face)
   `("#.*include \\(\\(<\\|\"\\).*\\(>\\|\"\\)\\)" . (1 font-lock-string-face))
   ;; keywords
   `(,(regexp-opt (em-keywords) 'symbols) . font-lock-keyword-face)
   ;; types
   `(,(regexp-opt (em-types) 'symbols) . font-lock-type-face)
   ;; function names
   '("\\b\\([A-Za-z_][A-Za-z0-9_]*\\)\\s-*(" 1 font-lock-function-name-face)
   ;; identifier that follows a data type
   `(,(concat "\\b\\(" (regexp-opt (em-types)) "\\)\\s-+\\([A-Za-z_][A-Za-z0-9_]*\\)\\b")
     (1 font-lock-type-face)
     (2 font-lock-variable-name-face))))

(defun em--previous-non-empty-line ()
  (save-excursion
    (forward-line -1)
    (while (and (not (bobp))
                (string-empty-p
                 (string-trim-right
                  (thing-at-point 'line t))))
      (forward-line -1))
    (thing-at-point 'line t)))

(defun em--indentation-of-previous-non-empty-line ()
  (save-excursion
    (forward-line -1)
    (while (and (not (bobp))
                (string-empty-p
                 (string-trim-right
                  (thing-at-point 'line t))))
      (forward-line -1))
    (current-indentation)))

(defun em--desired-indentation ()
  (let* ((cur-line (string-trim-right (thing-at-point 'line t)))
         (prev-line (string-trim-right (em--previous-non-empty-line)))
         (indent-len 4)
         (prev-indent (em--indentation-of-previous-non-empty-line)))
    (cond
     ((and (string-suffix-p "{" prev-line)
           (string-prefix-p "}" (string-trim-left cur-line)))
      prev-indent)
     ((string-suffix-p "{" prev-line)
      (+ prev-indent indent-len))
     ((string-prefix-p "}" (string-trim-left cur-line))
      (max (- prev-indent indent-len) 0))
     ((string-suffix-p ":" prev-line)
      (if (string-suffix-p ":" cur-line)
          prev-indent
        (+ prev-indent indent-len)))
     ((string-suffix-p ":" cur-line)
      (max (- prev-indent indent-len) 0))
     (t prev-indent))))

(defun em-indent-line ()
  (interactive)
  (when (not (bobp))
    (let* ((desired-indentation
            (em--desired-indentation))
           (n (max (- (current-column) (current-indentation)) 0)))
      (indent-line-to desired-indentation)
      (forward-char n))))

(define-derived-mode em-mode prog-mode "Em"
  "Major mode for editing Em files."
  :syntax-table em-mode-syntax-table
  (setq-local font-lock-defaults '(em-font-lock-keywords))
  (setq-local indent-line-function 'em-indent-line)
  (setq-local comment-start "// "))

(provide 'em-mode)
