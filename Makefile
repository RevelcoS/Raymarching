CC  = gcc
CXX = g++
LD  = g++

SRC = source/
BIN = binary/
LIB = library/
INC = include/
MOD = modules/

INCFLAGS  = -I$(INC) -I$(SRC)include/
LIBFLAGS  = -L$(LIB) -lglfw.3.4

UNAME =
ifeq ($(OS),Windows_NT)
	UNAME = Windows
else
	UNAME = $(shell uname -s)
endif

ifeq ($(UNAME),Darwin)
	INCFLAGS += -I/usr/local/opt/libomp/include
	LIBFLAGS += -L/usr/local/opt/libomp/lib
endif

CXXFLAGS  = $(INCFLAGS)
CCFLAGS   = $(INCFLAGS)
LDFLAGS   = $(LIBFLAGS)


CXXFLAGS += -std=c++11
CXXFLAGS += -Xpreprocessor
CXXFLAGS += -fopenmp
CXXFLAGS += -Wno-deprecated-declarations
CXXFLAGS += -O2

CCFLAGS  += -std=c17
CCFLAGS  += -O2

LDFLAGS += -O2
ifeq ($(UNAME),Darwin)
	LDFLAGS += -lomp
endif


SRCcpp    = $(wildcard $(SRC)*.cpp)
OBJECTS   = $(SRCcpp:%.cpp=$(BIN)%.o)
TARGET    = $(BIN)raymarching

### Modules ###
STBMOD    = $(MOD)stb/
MATHMOD   = $(MOD)LiteMath/
GLADMOD   = $(MOD)glad/
ALLMODS   = $(STBMOD) $(MATHMOD) $(GLADMOD)

# Image2D
IMAGE2Dcpp  = $(MATHMOD)Image2d.cpp
IMAGE2Dobj  = $(IMAGE2Dcpp:%.cpp=$(BIN)%.o)
OBJECTS     += $(IMAGE2Dobj)

# Glad
GLADc       = $(GLADMOD)glad.c
GLADobj     = $(GLADc:%.c=$(BIN)%.o)
OBJECTS     += $(GLADobj)

.PHONY: libs
libs: stb LiteMath OpenMP

.PHONY: stb
stb:
	cp $(STBMOD)stb_image.h $(INC)
	cp $(STBMOD)stb_image_write.h $(INC)

.PHONY: LiteMath
LiteMath:
	cp $(MATHMOD)LiteMath.h $(INC)
	cp $(MATHMOD)Image2d.h $(INC)

.PHONY: OpenMP
OpenMP: OpenMPinstall
	export OMP_NUM_THREADS=4

.PHONY: OpenMPinstall
ifeq ($(UNAME),Darwin)
OpenMPinstall:
	brew install libomp
else
OpenMPinstall: ;
endif

.PHONY: syncdirs
syncdirs:
	@for mod in $(ALLMODS); do mkdir -p $(BIN)$$mod; done
	@rsync -a --include '*/' --exclude '*' $(SRC) $(BIN)$(SRC)

# TODO: custom output messages
.PHONY: all
all: syncdirs $(TARGET)

# Target binary
$(TARGET): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

# TODO: make DEPS instead
$(GLADobj): $(GLADc)
	$(CC) -c $(CCFLAGS) -o $@ $^

$(BIN)$(MOD)%.o: $(MOD)%.cpp $(INC)%.h
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(BIN)$(SRC)%.o: $(SRC)%.cpp $(SRC)$(INC)%.h
	$(CXX) -c $(CXXFLAGS) -o $@ $<

# Define USE_STB_IMAGE for LiteImage
$(IMAGE2Dobj): $(IMAGE2Dcpp)
	$(CXX) -c -DUSE_STB_IMAGE $(CXXFLAGS) -o $@ $<

.PHONY: run
run: all
	./$(TARGET)

.PHONY: clean
clean:
	rm -f $(OBJECTS)

.PHONY: cleansrc
cleansrc:
	rm -rf $(BIN)$(SRC)
