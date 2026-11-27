# ROADMAP.md is generated from docs/roadmap.sexp by nova-roadmap, the one roadmap
# generator (a nova-tools release binary). This repo builds with CMake, which writes
# its own top-level Makefile in an in-source build, so these targets live here:
# `make -f tools/roadmap/roadmap.mk roadmap` renders the page and
# `make -f tools/roadmap/roadmap.mk roadmap-check` exits 1 when the committed page is not what the data
# renders (CI runs it). Edit the data, never the page; `nova-roadmap help form`
# documents the form. The pinned binary is downloaded once into tools/roadmap/bin/ and checked
# against the release's SHA256SUMS; NOVA_ROADMAP=<path> runs another one.
NOVA_ROADMAP_VERSION ?= v2.0.86
NOVA_ROADMAP ?= tools/roadmap/bin/nova-roadmap-$(NOVA_ROADMAP_VERSION)

.PHONY: roadmap roadmap-check
roadmap: $(NOVA_ROADMAP)
	$(NOVA_ROADMAP) render

roadmap-check: $(NOVA_ROADMAP)
	$(NOVA_ROADMAP) check

tools/roadmap/bin/nova-roadmap-%:
	@mkdir -p tools/roadmap/bin
	@os=$$(uname -s | tr A-Z a-z); arch=$$(uname -m); \
	case $$arch in x86_64) arch=amd64;; aarch64) arch=arm64;; esac; \
	asset=nova-roadmap_$*_$${os}_$${arch}; \
	base=https://github.com/mas-bandwidth/nova-tools/releases/download/$*; \
	curl -fsSL -o $@.tmp "$$base/$$asset" && \
	sum=$$(curl -fsSL "$$base/SHA256SUMS" | awk -v a="$$asset" '$$2 == a {print $$1}') && \
	test -n "$$sum" && { echo "$$sum  $@.tmp" | sha256sum -c - >/dev/null 2>&1 || echo "$$sum  $@.tmp" | shasum -a 256 -c - >/dev/null; } && \
	chmod +x $@.tmp && mv $@.tmp $@ || { rm -f $@.tmp; echo "nova-roadmap $* for $$os/$$arch: download or checksum failed" >&2; exit 2; }
