TARGET		 	= transcode 
CXX 			= g++
CC				= gcc
LINKER			= g++
CXXFLAGS 		= -Wall 
CFLAGS			= -Wall 
LFLAGS 			= -pthread -L/usr/local/lib -lavdevice -lavfilter -lavformat -lavcodec -lmp3lame -liconv -lz -lswresample -lswscale -lavutil -lrt -lm
INCLUDEPATH 	=  	

OBJS	 		=	Transcode.o	\
					main.o	\
					AudioConvert.o

APPEND_LIB	=

OUTPUT_PATH = ./

ifeq ($(debug),yes)
	CXXFLAGS 	+= -g3
	CFLAGS		+= -g3
else
	CXXFLAGS 	+= -O0
	CFLAGS		+= -O0
endif

all : $(OUTPUT_PATH)$(TARGET)
$(OUTPUT_PATH)$(TARGET) : $(OBJS)
	$(LINKER) $(CXXFLAGS) -o $(OUTPUT_PATH)$(TARGET) $(OBJS) $(APPEND_LIB) $(LFLAGS)
AudioConvert.o : ./AudioConvert.cpp
	$(CXX) $(CXXFLAGS) -o $(@) -c $(^) $(INCLUDEPATH)
Transcode.o : ./Transcode.cpp
	$(CXX) $(CXXFLAGS) -o $(@) -c $(^) $(INCLUDEPATH)
main.o : ./main.cpp
	$(CXX) $(CXXFLAGS) -o $(@) -c $(^) $(INCLUDEPATH)
Transcoder.o : ./Transcoder.cpp
	$(CXX) $(CXXFLAGS) -o $(@) -c $(^) $(INCLUDEPATH)	
clean :
	rm -rf *.o
	rm -f $(OUTPUT_PATH)$(TARGET) 
