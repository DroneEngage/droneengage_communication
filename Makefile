
CXX=g++
CXXARM=/usr/bin/arm-linux-gnueabihf-g++
CXXARM_ZERO=g++
EXE=uavos_comm
EXE_ARM=uavos_comm_arm
BIN=bin

INCLUDE= -I ~/TDisk/Boost/boost_1_76_0/ -I /usr/include/x86_64-linux-gnu
INCLUDE_ARM =  -I /usr/include -I /usr/include/arm-linux-gnueabihf  -I ~/TDisk/Boost/boost_1_76_0
INCLUDE_ARM_ZERO =  -I /usr/include/boost/beast -I /usr/include

LIBS=  -pthread   -lcurl  -lboost_coroutine -lssl -lcrypto
LIBS_ARM_ZERO = -pthread   -lcurl  -lboost_coroutine -lssl -lcrypto

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
	   $(BUILD)/util_rpi.o \
	   $(BUILD)/andruav_unit.o
	   

SRCS = ../main.cpp \
	   ../udpCommunicator.cpp \
	   ../configFile.cpp \
	   ../comm_server/andruav_comm_session.cpp \
	   ../comm_server/andruav_comm_server.cpp \
	   ../uavos/uavos_modules_manager.cpp \
	   ../comm_server/andruav_auth.cpp \
	   ../helpers/helpers.cpp \
	   ../helpers/util_rpi.cpp \
	   ../comm_server/andruav_unit.cpp
	   


all: release





release: uavos.release
	$(CXX)  $(CXXFLAGS_RELEASE)  -o $(BIN)/$(EXE).so  $(OBJS)   $(LIBS)  ;
	@echo "building finished ..."; 
	@echo "DONE."

debug: uavos.debug
	$(CXX) $(CXXFLAGS_DEBUG) -o $(BIN)/$(EXE).so     $(OBJS)   $(LIBS) ;
	@echo "building finished ..."; 
	@echo "DONE."

arm_release: uavos.arm.release
	$(CXXARM)  $(CXXFLAGS_RELEASE) -o $(BIN)/$(EXE_ARM).so   $(OBJS)   $(LIBS) ;
	@echo "building finished ..."; 
	@echo "DONE."

arm_debug: uavos.arm.debug
	$(CXXARM)  $(CXXFLAGS_DEBUG) -o $(BIN)/$(EXE_ARM).so   $(OBJS)   $(LIBS) ;
	@echo "building finished ..."; 
	@echo "DONE."

arm_release_zero: uavos.arm.release.zero
	$(CXXARM_ZERO)  $(CXXFLAGS_RELEASE) -o $(BIN)/$(EXE_ARM).so   $(OBJS)   $(LIBS_ARM_ZERO) ;
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


uavos.arm.release: copy
	mkdir -p $(BUILD); \
	cd $(BUILD); \
	$(CXXARM)   $(CXXFLAGS_RELEASE) -c  $(SRCS)  $(INCLUDE_ARM)  ; 
	cd .. ; 
	@echo "compliling finished ..."

uavos.arm.debug: copy
	mkdir -p $(BUILD); \
	cd $(BUILD); \
	$(CXXARM)   $(CXXFLAGS_DEBUG) -c  $(SRCS)  $(INCLUDE_ARM)  ; 
	cd .. ; 
	@echo "compliling finished ..."



uavos.arm.release.zero: copy
	mkdir -p $(BUILD); \
	cd $(BUILD); \
	$(CXXARM_ZERO)   $(CXXFLAGS_RELEASE) -c  $(SRCS)  $(INCLUDE_ARM_ZERO)  ; 
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



	

