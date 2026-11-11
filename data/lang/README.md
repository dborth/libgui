# Languages

English is the text in the program, so there is no English file. Every other language is one `xx.lang` file here, in
the gettext PO format (a plain text subset of it):

```
msgid "Save As"
msgstr "Speichern unter"
```

* `msgid` is the English text exactly as it appears in the code. `msgstr` is the translation, in UTF-8. An empty
  `msgstr` means "not translated yet": the English text is shown.
* A `%d`, `%s`... in a `msgid` must appear in the same order in the `msgstr` if the code builds the message with
  `TrFormat()`; a translation that does not match is not used (the English text is shown instead).
* Lines starting with `#` are comments.

The build compiles each `xx.lang` with `msgfmt` (GNU gettext) into `xx.mo`, links it into the program, and
`xx_mo` / `xx_mo_size` are declared by `xx_mo.h`. `msgfmt` stops the build on a malformed file or a duplicate `msgid`.
The files carry no PO header: the build puts one (`charset=UTF-8`, which newer `msgfmt` requires for non-ASCII text) in front of each file as it compiles it.
The program uses a catalog in place, without copying it: `textTranslator->loadLanguage(xx_mo, xx_mo_size)`.
