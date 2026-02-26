#######################################################

# toolchain
CC = sdcc
OBJCOPY = objcopy
PACK_HEX = packihx
WCHISP = isp55e0 -f

#######################################################

XRAM_SIZE = 0x0400
XRAM_LOC = 0x0000
CODE_SIZE = 0x3800

TARGET = dongle
FREQ_SYS = 12000000

CFLAGS := --Werror -mmcs51 --model-large --std-c11 \
	--xram-size $(XRAM_SIZE) --xram-loc $(XRAM_LOC) \
	--code-size $(CODE_SIZE) \
	-I$(ROOT_DIR)../include -DFREQ_SYS=$(FREQ_SYS) \
	$(EXTRA_FLAGS)

LFLAGS := $(CFLAGS)

BUILDDIR = build
SOURCEDIR = .

SOURCES = $(wildcard $(SOURCEDIR)/*.c)
HEADERS = $(wildcard $(SOURCEDIR)/*.h)

RELS = $(SOURCES:$(SOURCEDIR)/%.c=$(BUILDDIR)/%.rel)

all: $(BUILDDIR) $(BUILDDIR)/$(TARGET).bin

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/$(TARGET).ihx: $(RELS)
	$(CC) $(RELS) $(LFLAGS) -o $(BUILDDIR)/$(TARGET).ihx

$(BUILDDIR)/%.rel: $(SOURCEDIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/$(TARGET).bin: $(BUILDDIR)/$(TARGET).ihx
	$(OBJCOPY) -I ihex -O binary $(BUILDDIR)/$(TARGET).ihx $(BUILDDIR)/$(TARGET).bin	

flash: $(BUILDDIR) $(BUILDDIR)/$(TARGET).bin
	$(WCHISP) $(BUILDDIR)/$(TARGET).bin

clean:
	rm -f *.rel
	rm -f *.asm
	rm -f *.sym
	rm -f *.o
	rm -f *.lst
	rm -rf $(BUILDDIR)

.PHONY: tests
tests:
	python3 tests/test.py

stop:
	python3 control.py -b

run:
	../../driver/whb04b

CPPCHECK_ARGS += --enable=all
CPPCHECK_ARGS += --check-level=exhaustive
CPPCHECK_ARGS += -I /usr/share/sdcc/include/
CPPCHECK_ARGS += -I /usr/share/sdcc/include/mcs51/
CPPCHECK_ARGS += -DFREQ_SYS=$(FREQ_SYS)
CPPCHECK_ARGS += --suppress=staticFunction
CPPCHECK_ARGS += --suppress=unusedFunction
CPPCHECK_ARGS += --inline-suppr
CPPCHECK_ARGS += --inconclusive

check:
	cppcheck $(CPPCHECK_ARGS) *.c