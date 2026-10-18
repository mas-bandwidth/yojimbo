# ROADMAP.md is generated from docs/roadmap.sexp by nova-tools tools/roadmap,
# the one roadmap generator: `make roadmap` renders it, `make roadmap-check`
# exits 1 when the committed page is not what the data renders (CI runs it).
# Edit the data, never the page; `go run $(ROADMAP_TOOL) --help` documents the form.
ROADMAP_TOOL ?= github.com/mas-bandwidth/nova-tools/tools/roadmap@9d564eb81259c8a19c6515cd61c040030940671f

.PHONY: roadmap roadmap-check
roadmap:
	go run $(ROADMAP_TOOL)

roadmap-check:
	go run $(ROADMAP_TOOL) --check
