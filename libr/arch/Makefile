include ../config.mk
include p/capstone.mk

NAME=r_arch
R2DEPS+=r_util r_reg

CFLAGS+=-DR2_PLUGIN_INCORE
CFLAGS:=-I.. -I$(LTOP)/asm/arch/include -DR2_PLUGIN_INCORE -Iarch -I$(TOP)/shlr $(CFLAGS)

.PHONY: pre
pre: p v35pre
	$(MAKE) libr_arch.$(EXT_SO)
	$(MAKE) libr_arch.$(EXT_AR)

v35pre:
	$(MAKE) -C p/arm/v35arm64

include $(STATIC_ARCH_PLUGINS)

STATIC_OBJS=$(subst ..,p/..,$(subst arch_,p/arch_,$(STATIC_OBJ)))
OBJS=arch.o arch_config.o arch_switch.o arch_op.o
OBJS+=arch_value.o arch_cond.o arch_session.o
OBJS+=${STATIC_OBJS}

include ../rules.mk
