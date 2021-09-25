PLATFORM_RELFLAGS += -fno-strict-aliasing -Werror -Wno-address-of-packed-member

ifdef CROSS_LIBS
PLATFORM_LDFLAGS += -L $(CROSS_LIBS)
endif

ifdef AXXIA_VERSION
PLATFORM_CPPFLAGS += -DAXXIA_VERSION='"$(AXXIA_VERSION)"'
endif
