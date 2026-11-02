# Called from the top-level Makefile (outer make only).
# These targets never link OpenCE into boot.dol.

.PHONY: opence opence-status

opence-status:
	@$(MAKE) --no-print-directory -C port/wii status

opence:
	@$(MAKE) --no-print-directory -C port/wii opence
