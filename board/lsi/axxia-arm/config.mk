PLATFORM_RELFLAGS += -fno-strict-aliasing

ifdef CROSS_LIBS
PLATFORM_LDFLAGS += -L $(CROSS_LIBS)
endif
