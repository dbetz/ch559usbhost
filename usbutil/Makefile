BUILD_DIR=build
MKDIR_P = mkdir -p

CFLAGS += -pthread
CPPFLAGS += $(CFLAGS) -std=c++14
LIBS += -lstdc++

TARGET=usbutil

OBJS=\
${BUILD_DIR}/main.o \
${BUILD_DIR}/SerialPort.o

HDRS=\
src/SerialPort.h

.PHONY: all

all: directories ${BUILD_DIR}/${TARGET}

${BUILD_DIR}:
	${MKDIR_P} ${BUILD_DIR}

${BUILD_DIR}/%.o: ./src/%.c $(HDRS)
	$(CC) -c $(CFLAGS) $< -o $@

${BUILD_DIR}/%.o: ./src/%.cpp $(HDRS)
	$(CC) -c $(CPPFLAGS) $< -o $@

${BUILD_DIR}/${TARGET}: $(OBJS)
	$(CC) $(CPPFLAGS) $^ $(LIBS) -o $@

directories: ${BUILD_DIR}

run:	${BUILD_DIR}/${TARGET}
	sudo ./build/${TARGET}

clean:
	$(RM) ${BUILD_DIR}/*.o
	$(RM) ${BUILD_DIR}/*.so
	$(RM) ${BUILD_DIR}/*.a
	$(RM) ${BUILD_DIR}/${TARGET}
