CC  = gcc
CXX = g++
LD  = g++

SRC = source/
BIN = binary/
LIB = library/
INC = include/
MOD = modules/

INCFLAGS  = -I$(INC) -I$(SRC)include/
LIBFLAGS  = -L$(LIB)
CXXFLAGS  = $(INCFLAGS)
LDFLAGS   = $(LIBFLAGS)

CXXFLAGS += -std=c++11
CXXFLAGS += -Wno-deprecated-declarations

SRCcpp    = $(wildcard $(SRC)*.cpp)
OBJECTS   = $(SRCcpp:%.cpp=$(BIN)%.o)
TARGET    = $(BIN)raymarching

### Modules ###
STBMOD    = $(MOD)stb/
MATHMOD   = $(MOD)LiteMath/
ALLMODS   = $(STBMOD) $(MATHMOD)

IMAGE2Dcpp  = $(MATHMOD)Image2d.cpp
IMAGE2Dobj  = $(IMAGE2Dcpp:%.cpp=$(BIN)%.o)
OBJECTS    += $(IMAGE2Dobj)

.PHONY: libs
libs: stb LiteMath

.PHONY: stb
stb:
	cp $(STBMOD)stb_image.h $(INC)
	cp $(STBMOD)stb_image_write.h $(INC)

.PHONY: LiteMath
LiteMath:
	cp $(MATHMOD)LiteMath.h $(INC)
	cp $(MATHMOD)Image2d.h $(INC)


.PHONY: syncdirs
syncdirs:
	@for mod in $(ALLMODS); do mkdir -p $(BIN)$$mod; done
	@rsync -a --include '*/' --exclude '*' $(SRC) $(BIN)$(SRC)

# TODO: custom output messages
.PHONY: all
all: syncdirs $(TARGET)

$(TARGET): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

# TODO: make DEPS instead
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
