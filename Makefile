OBJS=\
	main.o \
	graphics/seps114a/seps114a.o \
	graphics/canvas/canvas.o \
	io/bq27510/bq27510.o \
	lib/lodepng.o \
	graphics/tinypng/TinyPngOut.o \
	tiger.o
	#graphics/fbdev/fbdev.o
SRCS=$(patsubst %.o,%.c,$(OBJS))
BIN=stak-test
CFLAGS+=-DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall -g -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi
LDFLAGS+=-L$(STAGING_DIR)/usr/lib/ \
		 -L$(STAGING_DIR)/opt/vc/lib/ \
		 -lGLESv2 \
		 -lEGL \
		 -lopenmaxil \
		 -lbcm_host \
		 -lvcos \
		 -lvchiq_arm \
		 -lkhrn_client \
		 -lvchostif \
		 -lvcilcs \
		 -lvcfiled_check \
		 -lnanovg \
		 -lpthread \
		 -lc \
		 -lrt \
		 -lm \
		 -lbcm2835 \
		 -L../libs/ilclient \
		 -L../libs/vgfont \
		 -L/usr/local/lib
INCLUDES+=-I$(STAGING_DIR)/usr/include -I$(STAGING_DIR)/opt/vc/include/ -I$(STAGING_DIR)/opt/vc/include/interface/vcos/pthreads -I$(STAGING_DIR)/opt/vc/include/interface/vmcs_host/linux -I./ -I../libs/ilclient -I../libs/vgfont


HEADER="\33[35m----------[\33[36;1m Stak: \33[0;33m$(BIN) \33[35m]----------\33[39m"
FAILURE="\33[36;1mStak \33[37m➜ \33[31mBuild Failed!\33[0;39m"
BUILDING="\33[36;1mStak \33[0;37m➜ \33[32mBuilding \33[34m$<...\33[39m"
COMPLETE="\33[36mStak \33[37m➜ \33[32mBuild Complete!\33[39m"

all: $(BIN) $(LIB)
	@echo $(COMPLETE)

header:
	@echo $(HEADER)
%.o: %.c
	@echo $(BUILDING)
	#@echo $(INCLUDES)
	$(CC) $(CFLAGS) $(INCLUDES) -s -c $< -o $@ -Wno-deprecated-declarations || (echo $(FAILURE) && false)

%.o: %.cpp
	@echo $(BUILDING)
	$(CXX) $(CFLAGS) $(INCLUDES) -c $< -o $@ -Wno-deprecated-declarations || (echo $(FAILURE) && false)

$(BIN): header $(OBJS)
	$(CC) -o $@ -Wl,--whole-archive $(OBJS) $(LDFLAGS) -pg -Wl,--no-whole-archive -rdynamic

%.a: $(OBJS)
	@$(AR) r $@ $^

#docs:
#	cldoc generate $(CFLAGS) $(INCLUDES) -- --output doc $()
run:
	@./$(BIN)
clean:
	@for i in $(OBJS); do (if test -e "$$i"; then ( rm $$i ); fi ); done
	@rm -f $(BIN) $(LIB)



