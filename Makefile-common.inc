red						:=	`tput setaf 1`
green					:=	`tput setaf 2`
yellow					:=	`tput setaf 3`
blue					:=	`tput setaf 4`
magenta					:=	`tput setaf 5`
cyan					:=	`tput setaf 6`
white					:=	`tput setaf 7`
reset					:=	`tput sgr0`

TARGET					:=	arm-stak-linux-gnueabihf
STAK_SDK				:=	/opt/stak-sdk
VC_SDK					:=	/opt/vc
CROSS_TOOLCHAIN_DIR		:=	$(STAK_SDK)/bin
CROSS_PREFIX			:=	$(TARGET)-

CC						:=	$(CROSS_TOOLCHAIN_DIR)/$(CROSS_PREFIX)gcc
CXX						:=	$(CROSS_TOOLCHAIN_DIR)/$(CROSS_PREFIX)g++
LD						:=	$(CROSS_TOOLCHAIN_DIR)/$(CROSS_PREFIX)ld
STRIP					:=	$(CROSS_TOOLCHAIN_DIR)/$(CROSS_PREFIX)strip


CFLAGS+=\
							-Wall \
							-fPIC \
							-DPIC \
							-DSTANDALONE \
							-D__STDC_CONSTANT_MACROS \
							-D__STDC_LIMIT_MACROS \
							-DTARGET_POSIX \
							-D_LINUX \
							-D_REENTRANT \
							-D_LARGEFILE64_SOURCE \
							-D_FILE_OFFSET_BITS=64 \
							-U_FORTIFY_SOURCE \
							-DHAVE_LIBOPENMAX=2 \
							-DOMX \
							-DOMX_SKIP64BIT \
							-DUSE_EXTERNAL_OMX \
							-DHAVE_LIBBCM_HOST \
							-DUSE_EXTERNAL_LIBBCM_HOST \
							-DUSE_VCHIQ_ARM

#	TARGETFLAGS=\
#				-target arm-linux-engeabihf \
#				-integrated-as \
#				-mfloat-abi=softfp

LDFLAGS+=\
							-L$(STAK_SDK)/lib \
							-L$(VC_SDK)/lib \
							-L./lib \
							-lpthread \
							-ldl \
							-lstdc++ \
							-lc \
							-lkhrn_client \
							-lvchostif \
							-lvcilcs \
							-lvcfiled_check \
							-lm \
							-lvcos \
							-lvchiq_arm\
							-lkhrn_client \
							-lvchostif \
							-lvcilcs \
							-lvcfiled_check \
							-lbcm_host \
							-lGLESv2 \
							-lEGL \
							-export-dynamic
INCLUDES+=\
							-isystem./ \
							-isystem./src \
							-I$(STAK_SDK)/include \
							-I$(VC_SDK)/include \
							-I$(VC_SDK)/include/interface/vcos/pthreads \
							-I$(VC_SDK)/include/interface/vmcs_host/linux/ \
							-I./lib \
							-I./src \
							-I./assets


HEADER					=	"${magenta}----------[${cyan} Stak: ${yellow}$(MAIN_BIN) ${magenta}]----------${reset}"
FAILURE					=	"${cyan}Stak${yellow}➜ ${red}Build Failed!${reset}"
BUILDING				=	"${cyan}Stak${yellow}➜ ${green}Building ${blue}$<...${reset}"
COMPLETE				=	"${cyan}Stak${yellow}➜ ${green}Build Complete!${reset}"
BUILDING_DL				=	"${cyan}Stak${yellow}➜ ${green}Building ${blue}[ ${magenta}${notdir $<}${blue} ]${reset}"

MAIN_BIN				:=	build/main
MAIN_C					:=	lib/graphics/canvas/canvas.c \
							lib/graphics/seps114a/seps114a.c \
							lib/bcm2835.c \
							src/application/application.c
MAIN_CXX				:=	src/main.cpp

API_SRCS				:=	$(MODES)
MAIN_C_OBJS				:=	$(patsubst %.c,		build/%.o,	$(MAIN_C))
MAIN_CXX_OBJS			:=	$(patsubst %.cpp,	build/%.o,	$(MAIN_CXX))
API_OBJS				:=	$(patsubst %.cpp,	build/%.o,	$(API_SRCS))
API_OUTPUT				:=	$(patsubst %.cpp,	build/%.so,	$(API_SRCS))
ALL_OBJS				:=	$(MAIN_OBJS) $(API_OBJS)

.SECONDARY:  $(ALL_OBJS)

all: header $(MAIN_BIN) $(API_OUTPUT)
	@echo $(COMPLETE)

header:
	@echo $(HEADER)

$(MAIN_BIN): $(MAIN_C_OBJS) $(MAIN_CXX_OBJS)
	@echo $(BUILDING_DL)
	@$(CC) $(LDFLAGS) -g $^ -o $@

build/%.o: %.c
	@echo $(BUILDING)
	@mkdir -p `dirname $@`
	@$(CC) $(TARGETFLAGS) $(CFLAGS) $(INCLUDES) -c $< -o $@


build/%.o: %.cpp
	@echo $(BUILDING)
	@mkdir -p `dirname $@`
	@$(CXX) $(TARGETFLAGS) $(CFLAGS) $(INCLUDES) -g -std=c++1y -c $< -o $@

build/%.so: build/%.o
	@echo $(BUILDING_DL)
	@$(CXX) $(CFLAGS) $(LDFLAGS) -shared -o build/$(notdir $@) -g $< $(API_EXTRA_OBJS)

clean:
	rm -rf build/
