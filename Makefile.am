plugin_LTLIBRARIES = libgsthkeffects.la

#ORC_SOURCE=gstvideofiltersbadorc
#include $(top_srcdir)/common/orc.mak

libgsthkeffects_la_SOURCES = \
	../videofilters/gstvideofilter2.c \
	../videofilters/gstvideofilter2.h \
    hkgraphics.c \
    hkgraphics.h \
	gsttrack.c \
    gstmotrack.c \
	gsthkeffects.c
#nodist_libgsthkeffects_la_SOURCES = $(ORC_NODIST_SOURCES)
libgsthkeffects_la_CFLAGS = \
	$(GST_PLUGINS_BASE_CFLAGS) \
	$(GST_CFLAGS) \
	$(ORC_CFLAGS)
libgsthkeffects_la_LIBADD = \
	$(GST_PLUGINS_BASE_LIBS) -lgstvideo-$(GST_MAJORMINOR) \
	$(GST_BASE_LIBS) \
	$(GST_LIBS) \
	$(ORC_LIBS) \
	$(LIBM)
libgsthkeffects_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgsthkeffects_la_LIBTOOLFLAGS = --tag=disable-static

noinst_HEADERS = \
    hkgraphics.h \
    gsttrack.h \
    gstmotrack.h

Android.mk: Makefile.am $(BUILT_SOURCES)
	androgenizer \
	-:PROJECT libgstvideofilters -:SHARED libgstvideofilters \
	 -:TAGS eng debug \
         -:REL_TOP $(top_srcdir) -:ABS_TOP $(abs_top_srcdir) \
	 -:SOURCES $(libgstvideofilters_la_SOURCES) \
	 -:CFLAGS $(DEFS) $(DEFAULT_INCLUDES) $(libgstvideofilters_la_CFLAGS) \
	 -:LDFLAGS $(libgstvideofilters_la_LDFLAGS) \
	           $(libgstvideofilters_la_LIBADD) \
	           -ldl \
	 -:PASSTHROUGH LOCAL_ARM_MODE:=arm \
		       LOCAL_MODULE_PATH:='$$(TARGET_OUT)/lib/gstreamer-0.10' \
	> $@
