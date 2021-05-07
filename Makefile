
CXX=g++
CXXARM=/opt/cross-pi-gcc/bin/arm-linux-gnueabihf-g++
CXXARM3=arm-linux-gnueabihf-g++
CXXARM2=~/TDisk/raspberry_pi/zero/tools_cross_compiler/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf//bin/arm-linux-gnueabihf-g++
CXXARM1=~/TDisk/raspberry_pi/zero/tools_cross_compiler/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/arm-linux-gnueabihf-g++
EXE=uavos_comm
BIN=bin
INCLUDE= -I ~/TDisk/Boost/boost_1_76_0/ -I /usr/include/x86_64-linux-gnu/curl
LIBS=  -pthread   -lcurl -L ~/TDisk/Boost/boost_1_76_s0/output_x86 -lboost_coroutine -lssl -lcrypto
#LIBS=  -lcurl -pthread -lboost_coroutine -lssl -lcrypto
CXXFLAGS =  -std=c++11

CXXFLAGS_RELEASE= $(CXXFLAGS) -DRELEASE -s   -Werror=unused-variable -Werror=unused-result
CXXFLAGS_DEBUG= $(CXXFLAGS)  -DDEBUG -g   
SRC = src
BUILD = build

OBJS = $(BUILD)/main.o \
	   $(BUILD)/udpCommunicator.o \
	   $(BUILD)/configFile.o \
	   $(BUILD)/andruav_comm_session.o \
	   $(BUILD)/andruav_comm_server.o \
	   $(BUILD)/uavos_modules_manager.o \
	   $(BUILD)/andruav_auth.o \
	   $(BUILD)/helpers.o \
	   $(BUILD)/andruav_unit.o
	   

SRCS = ../main.cpp \
	   ../udpCommunicator.cpp \
	   ../configFile.cpp \
	   ../andruav_comm_session.cpp \
	   ../andruav_comm_server.cpp \
	   ../uavos_modules_manager.cpp \
	   ../andruav_auth.cpp \
	   ../helpers/helpers.cpp \
	   ../andruav_unit.cpp
	   


all: release





release: uavos.release
	$(CXX)  $(CXXFLAGS_RELEASE)  -o $(BIN)/$(EXE).so  $(OBJS)   $(LIBS)  ;
	@echo "building finished ..."; 
	@echo "DONE."

debug: uavos.debug
	$(CXX) $(CXXFLAGS_DEBUG) -o $(BIN)/$(EXE).so     $(OBJS)   $(LIBS) ;
	@echo "building finished ..."; 
	@echo "DONE."

arm_debug: uavos.arm.debug
	$(CXXARM)  $(CXXFLAGS_DEBUG) -o $(BIN)/$(EXE).so   $(OBJS)   $(LIBS) ;
	@echo "building finished ..."; 
	@echo "DONE."

uavos.release: copy
	mkdir -p $(BUILD); \
	cd $(BUILD); \
	$(CXX)   $(CXXFLAGS_RELEASE)  -c   $(SRCS)  $(INCLUDE)  ; 
	cd .. ; 
	@echo "compliling finished ..."

uavos.debug: copy
	mkdir -p $(BUILD); \
	cd $(BUILD); \
	$(CXX)   $(CXXFLAGS_DEBUG)  -c  $(SRCS)  $(INCLUDE);
	cd .. ; 
	@echo "compliling finished ..."


uavos.arm.debug: copy
	mkdir -p $(BUILD); \
	cd $(BUILD); \
	$(CXXARM)   $(CXXFLAGS_DEBUG) -g -DDEBUG -c   $(SRCS)  $(INCLUDE)  ; 
	cd .. ; 
	@echo "compliling finished ..."



git_submodule:
	git submodule update --init --recursive


copy: clean
	mkdir -p $(BIN); \
	touch config.null.json; \
	cp config.*.json $(BIN); 
	@echo "copying finished"

clean:
	rm -rf $(BIN); 
	rm -rf $(BUILD);
	@echo "cleaning finished"



	

