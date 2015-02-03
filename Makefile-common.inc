CFLAGS+=-Wall -fPIC -I./src -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS \
		-DTARGET_POSIX -D_LINUX -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
		-D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -DHAVE_LIBOPENMAX=2 -DOMX \
		-DOMX_SKIP64BIT -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST \
		-DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM
ifdef PREFIX
	CFLAGS+=--sysroot=$(PREFIX) -I$(PREFIX)/include
endif
LDFLAGS+=-L/opt/vc/lib/ -L/opt/cross/lib/ -L./lib \
		 -lpthread \
		 -lc++ \
		 -ldl \
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
		 -ljpeg \
		 -lrt \
		 -lm \
		 -lbcm2835 \
		 -lmmal \
		 -lmmal_core \
		 -lmmal_util
INCLUDES+=-isystem/opt/cross/include/c++/v1/ \
		 -isystem/opt/rpi-tools/arm-bcm2708/arm-bcm2708-linux-gnueabi/arm-bcm2708-linux-gnueabi/sysroot/usr/include \
		 -isystem$(pwd) \
		 -isystem$(pwd)/src \
		 -I/opt/cross/include \
		 -I/opt/vc/include \
		 -I/opt/vc/include/interface/vcos/pthreads \
		 -I/opt/vc/include/interface/vmcs_host/linux/ \
		 -I./lib \
		 -I./src \
		 -I./assets
		 #-I./lib/gifsicle/include \
		 #-I./lib/gifsicle/include \


HEADER="\33[35m----------[\33[36;1m Stak: \33[0;33m$(BIN) \33[35m]----------\33[39m"
FAILURE="\33[36;1mStak \33[37m➜ \33[31mBuild Failed!\33[0;39m"
BUILDING="\33[36;1mStak \33[0;37m➜ \33[32mBuilding \33[34m$<...\33[39m"
COMPLETE="\33[36mStak \33[37m➜ \33[32mBuild Complete!\33[39m"
BUILDING_DL="\33[36;1mStak \33[0;37m➜ \33[32mBuilding \33[34m[ \33[35m${notdir $@}\33[34m ]\33[39m"

CC    := $(CROSS_COMPILE)clang
CXX   := $(CROSS_COMPILE)clang++
LD    := $(CROSS_COMPILE)clang++
STRIP := $(CROSS_COMPILE)strip


MAIN_BIN=		build/main
MAIN_SRCS=		src/main.cpp \
				src/application/application.cpp \
				lib/graphics/canvas/canvas.c \
				lib/graphics/seps114a/seps114a.c \
				lib/libshapes/libshapes.c

API_SRCS=		$(MODES)
MAIN_OBJS=		$(patsubst %.c,	build/%.o,	$(MAIN_SRCS))
API_OBJS=		$(patsubst %.cpp,	build/%.o,	$(API_SRCS))
API_OUTPUT=		$(patsubst %.cpp,	build/%.so,	$(API_SRCS))
ALL_OBJS=		$(MAIN_OBJS) $(API_OBJS)

.SECONDARY:  $(ALL_OBJS)

all: header $(MAIN_BIN) $(API_OUTPUT)
	@echo $(COMPLETE)

header:
	@echo $(HEADER)

$(MAIN_BIN): $(MAIN_OBJS)
	@echo $(BUILDING_DL)
	@$(CC) -o $@ -Wl,--whole-archive $(MAIN_OBJS) $(INCLUDES) $(LDFLAGS) -g -Wl,--no-whole-archive -rdynamic -std=c++14 -stdlib=libc++ -target ${TARGET}

build/%.o: %.c
	@echo $(BUILDING)
	@mkdir -p `dirname $@`
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@ -p -g -target ${TARGET}


build/%.o: %.cpp
	@echo $(BUILDING)
	@mkdir -p `dirname $@`
	@$(CXX) $(CFLAGS) $(INCLUDES) -c $< -o $@ -g -std=c++14 -stdlib=libc++  -target ${TARGET}

build/%.so: build/%.o
	@echo $(BUILDING_DL)
	@$(CXX) $(CFLAGS) -shared -o build/$(notdir $@) -g $< $(API_EXTRA_OBJS) -lbcm2835 -lpthread -lc++abi

clean:
	rm $(MAIN_OBJS) $(API_OBJS)