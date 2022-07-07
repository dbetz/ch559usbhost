BUILD_DIR=build
MKDIR_P=mkdir -p

project_name=CH559USB
xram_size=0x0800
xram_loc=0x0600
code_size=0xEFFF
dfreq_sys=48000000

TARGET=$(project_name).bin

CFLAGS=-V -mmcs51 --model-large --xram-size $(xram_size) --xram-loc $(xram_loc) --code-size $(code_size) -I/ -DFREQ_SYS=$(dfreq_sys)

OBJS=\
$(BUILD_DIR)/main.rel \
$(BUILD_DIR)/util.rel \
$(BUILD_DIR)/USBHost.rel \
$(BUILD_DIR)/USBHid.rel \
$(BUILD_DIR)/USBHub.rel \
$(BUILD_DIR)/USBFtdi.rel \
$(BUILD_DIR)/uart.rel

.PHONY: all

all: directories $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR):
	$(MKDIR_P) $(BUILD_DIR)

$(BUILD_DIR)/%.rel: %.c $(HDRS)
	sdcc -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(project_name).ihx: $(OBJS)
	sdcc $(CFLAGS) $^ -o $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.ihx
	packihx $(BUILD_DIR)/$(project_name).ihx > $(BUILD_DIR)/$(project_name).hex

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.hex
	sdobjcopy -I ihex -O binary $< $@

flash:	$(BUILD_DIR)/$(TARGET)
	chflasher.py $(BUILD_DIR)/$(TARGET)

directories: $(BUILD_DIR)

clean:
	$(RM) -rf $(BUILD_DIR)

