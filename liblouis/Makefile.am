liblouisincludedir = $(includedir)/liblouis

# Don't include liblouis.h in dist, this will break subdir builds
# when dist is configured with a different ucs-option than the build,
# i.e. ucs2 dist and ucs4 build and vice versa.
nodist_liblouisinclude_HEADERS = \
	liblouis.h

lib_LTLIBRARIES = liblouis.la

AM_CPPFLAGS = 						\
	$(WARN_CFLAGS)					\
	-DTABLESDIR=\""$(datadir)"/liblouis/tables\"	\
	-I$(top_srcdir)/gnulib				\
	-I$(top_builddir)/gnulib

liblouis_la_LIBADD = $(top_builddir)/gnulib/libgnu.la

liblouis_la_LDFLAGS =	\
	-version-info $(LIBLOUIS_CURRENT):$(LIBLOUIS_REVISION):$(LIBLOUIS_AGE) -no-undefined \
	$(LTLIBINTL)

if HAVE_LD_OUTPUT_DEF
liblouis_la_LDFLAGS += -Wl,--output-def,liblouis-$(DLL_VERSION).def
defexecdir = $(bindir)
defexec_DATA = liblouis-$(DLL_VERSION).def
DISTCLEANFILES = $(defexec_DATA)
endif

liblouis_la_SOURCES =				\
	internal.h					\
	compileTranslationTable.c		\
	lou_translateString.c			\
	lou_backTranslateString.c		\
	commonTranslationFunctions.c		\
	metadata.c 				\
	pattern.c				\
	logging.c				\
	utils.c				\
	maketable.c

# Don't include liblouis.h in dist, this will break subdir builds
# when dist is configured with a different ucs-option than the build,
# i.e. ucs2 dist and ucs4 build and vice versa.
nodist_liblouis_la_SOURCES = \
	liblouis.h

if HAVE_SOURCE_FORMATTER
format-sources-local:
	for file in $(SOURCES); do $(SOURCE_FORMATTER) -i $$file; done
	$(SOURCE_FORMATTER) -i liblouis.h.in
endif

# ------------------------------------------

GRAAL_HOME   = graalvm-ce-19.0.2/Contents/Home
NATIVE_IMAGE = $(GRAAL_HOME)/bin/native-image
GRAAL_SDK    = mvn/org/graalvm/sdk/graal-sdk/19.0.2/graal-sdk-19.0.2.jar
LEIN         = lein
MVN          = mvn

$(GRAAL_HOME) :
	curl -L -o graalvm.tar.gz \
		"https://github.com/oracle/graal/releases/download/vm-19.0.2/graalvm-ce-darwin-amd64-19.0.2.tar.gz"
	gtar -xzvf graalvm.tar.gz
	rm graalvm.tar.gz

$(NATIVE_IMAGE) : | $(GRAAL_HOME)
	$(GRAAL_HOME)/bin/gu install native-image

$(GRAAL_SDK) : | $(GRAAL_HOME)
	mkdir mvn
	$(MVN) "install:install-file" -Dfile=$(GRAAL_HOME)/jre/lib/boot/graal-sdk.jar \
	                              -DgroupId=org.graalvm.sdk \
	                              -DartifactId=graal-sdk \
	                              -Dversion=19.0.2 \
	                              -Dpackaging=jar \
	                              -DlocalRepositoryPath=mvn \
	                              -DcreateChecksum=true

liblouis_la_SOURCES += liblouis.c liblouis-impl.h
liblouis_la_LDFLAGS += -L$(top_builddir)/liblouis -llouis-impl

CLJ = clj/utils.clj java/pre/louis/c.java java/post/louis/java.java
JAR = lein/louis-0.1.0-SNAPSHOT-standalone.jar

$(JAR) : project.clj $(CLJ) | $(GRAAL_SDK)
	$(LEIN) do clean, \
	           with-profile step1 compile, \
	           with-profile step2 javac, \
	           with-profile step3 uberjar

liblouis-impl.dylib liblouis-impl.h : $(JAR) | $(NATIVE_IMAGE)
	$(NATIVE_IMAGE) --initialize-at-build-time \
	                --report-unsupported-elements-at-runtime \
	                -H:+ReportExceptionStackTraces \
	                --shared \
	                -Dinternal.h.path=$(CURDIR)/internal.h \
	                -cp $< \
	                -H:Name=liblouis-impl \
	                louis.java

liblouis.c : liblouis-impl.h

clean : clean-graal-stuff
clean-graal-stuff :
	rm -rf liblouis-impl.dylib      \
	       liblouis-impl.h          \
	       liblouis-impl_dynamic.h  \
	       graal_isolate.h          \
	       graal_isolate_dynamic.h  \
	       lein
