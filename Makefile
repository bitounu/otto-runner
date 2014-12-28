include Makefile-common.inc

MAIN_BIN=		build/main
MAIN_SRCS=		src/main.c \
				src/application/application.c \
				src/application/state/state.c \
				src/core/core.c


API_SRCS=		src/plugins/test/test.c \
	 			src/plugins/ipc/ipc.c \
	 			src/daemons/input/inputd.c

API_EXTRA_SRCS= src/apis/input/input.c \
				src/application/rpc/rpc.c

MAIN_OBJS=		$(patsubst %.c,	build/%.o,	$(MAIN_SRCS))
API_OBJS=		$(patsubst %.c,	build/%.o,	$(API_SRCS))
API_EXTRA_OBJS=	$(patsubst %.c,	build/%.o,	$(API_EXTRA_SRCS))
API_OUTPUT=		$(patsubst %.c,	build/%.so,	$(API_SRCS))


ALL_OBJS=		$(MAIN_OBJS) $(API_OBJS) $(API_EXTRA_OBJS)

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
	@$(CC) $(CFLAGS) -shared -o build/$(notdir $@) $< $(API_EXTRA_OBJS) -lbcm2835 -lpthread
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



