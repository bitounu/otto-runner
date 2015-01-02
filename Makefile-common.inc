CFLAGS+= -Wall -fPIC -I./src
ifdef PREFIX
	CFLAGS+=--sysroot=$(PREFIX) -I$(PREFIX)/include
endif
LDFLAGS+=-L/opt/vc/lib/ \
		 -lpthread \
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
		 -lbcm2835
INCLUDES+=-I`pwd` -I/opt/vc/include -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux/ -I./lib -I./src -I./assets


HEADER="\33[35m----------[\33[36;1m Stak: \33[0;33m$(BIN) \33[35m]----------\33[39m"
FAILURE="\33[36;1mStak \33[37m➜ \33[31mBuild Failed!\33[0;39m"
BUILDING="\33[36;1mStak \33[0;37m➜ \33[32mBuilding \33[34m$<...\33[39m"
COMPLETE="\33[36mStak \33[37m➜ \33[32mBuild Complete!\33[39m"
BUILDING_DL="\33[36;1mStak \33[0;37m➜ \33[32mBuilding \33[34m[ \33[35m${notdir $@}\33[34m ]\33[39m"

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)g++
STRIP := $(CROSS_COMPILE)strip


MAIN_BIN=		build/main
MAIN_SRCS=		src/main.c \
				src/application/application.c \
				lib/graphics/canvas/canvas.c \
				lib/graphics/seps114a/seps114a.c \
				lib/libshapes/libshapes.c

API_SRCS=		$(MODES)
MAIN_OBJS=		$(patsubst %.c,	build/%.o,	$(MAIN_SRCS))
API_OBJS=		$(patsubst %.c,	build/%.o,	$(API_SRCS))
API_OUTPUT=		$(patsubst %.c,	build/%.so,	$(API_SRCS))
ALL_OBJS=		$(MAIN_OBJS) $(API_OBJS)

.SECONDARY:  $(ALL_OBJS)

all: header $(MAIN_BIN) $(API_OUTPUT)
	@echo $(COMPLETE)

header:
	@echo $(HEADER)

build/%.o: %.c
	@echo $(BUILDING)
	@mkdir -p `dirname $@`
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@ -Wno-deprecated-declarations || (echo $(FAILURE) && false)

build/%.so: build/%.o $(API_EXTRA_OBJS)
	@echo $(BUILDING_DL)
	@$(CC) $(CFLAGS) -shared -o build/$(notdir $@) -pg $< $(API_EXTRA_OBJS) -lbcm2835 -lpthread
	@$(STRIP) build/$(notdir $@)


$(MAIN_BIN): $(MAIN_OBJS)
	@echo $(BUILDING_DL)
	@$(CC)  -o $@ -Wl,--whole-archive $(MAIN_OBJS) $(LDFLAGS) -pg -Wl,--no-whole-archive -rdynamic
	@$(STRIP) $@

%.a: $(MAIN_OBJS)
	@$(AR) r $@ $<

run: $(MAIN_BIN)
	@./$(MAIN_BIN)

clean:
	@rm -f $(MAIN_BIN) $(API_OUTPUT) $(ALL_OBJS)



