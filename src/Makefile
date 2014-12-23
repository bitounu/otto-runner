include Makefile-common.inc

SRCS=main.c \
	application/application.c \
	application/state/state.c \
	core/core.c
OBJS=$(patsubst %.c,build/%.o,$(SRCS))
BIN=build/main
BIN2=build/main
APIS=build/plugins/test/test.so \
	 build/plugins/ipc/ipc.so

API_EXTRA_SRCS= \
	daemons/input/input.c
API_EXTRA_OBJS=$(patsubst %.c, build/%.o, $(API_EXTRA_SRCS))


APISRCS=$(patsubst %.so,%.c,$(APIS))
APIOBJS=$(patsubst %.so,%.o,$(APIS))

.SECONDARY:  $(OBJECTS) $(APIOBJS)

all: header $(BIN) $(LIB) $(APIS)
	@echo $(COMPLETE)

header:
	@echo $(HEADER)

build/%.o: %.c
	@echo $(BUILDING)
	@mkdir -p `dirname $@`
	@$(CC) -fPIC $(CFLAGS) $(INCLUDES) -c $< -o $@ -Wno-deprecated-declarations || (echo $(FAILURE) && false)

build/%.so: build/%.o $(API_EXTRA_OBJS)
	@echo $(BUILDING_DL)
	@$(CC) -shared -o build/${notdir $@} $< $(API_EXTRA_OBJS)


$(BIN): $(OBJS)
	@echo $(BUILDING_DL)
	@$(CC) -o $@ -Wl,--whole-archive $(OBJS) $(LDFLAGS) -pg -Wl,--no-whole-archive -rdynamic

%.a: $(OBJS)
	@$(AR) r $@ $^

run:
	@./$(BIN)
clean:
	@for i in $(OBJS); do (if test -e "$$i"; then ( rm $$i ); fi ); done
	@rm -f $(BIN) $(LIB)



