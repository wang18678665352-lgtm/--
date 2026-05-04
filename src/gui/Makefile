CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 -finput-charset=UTF-8
LDFLAGS = -mwindows
LIBS = -lcomctl32 -lcomdlg32 -lgdi32 -lmsimg32

SRC_DIR = ..
GUI_DIR = .
OUTPUT = ../../test_gui.exe

GUI_SRCS = $(wildcard $(GUI_DIR)/gui_*.c)
CORE_SRCS = \
	$(SRC_DIR)/data_storage.c \
	$(SRC_DIR)/login.c \
	$(SRC_DIR)/common.c \
	$(SRC_DIR)/sha256.c \
	$(SRC_DIR)/public.c \
	$(SRC_DIR)/ui_utils.c

GUI_OBJS = $(GUI_SRCS:.c=.o) $(CORE_SRCS:.c=.o)
RES_OBJS = app.res

.PHONY: all clean

all: $(OUTPUT)

$(OUTPUT): $(GUI_OBJS) $(RES_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	@echo ""
	@echo "=== Build OK: $(OUTPUT) ==="

gui/gui_main.o: gui/gui_main.c gui/gui_main.h gui/gui_login.h gui/gui_patient.h gui/gui_doctor.h gui/gui_admin.h
	$(CC) $(CFLAGS) -c -o $@ $<

gui/gui_login.o: gui/gui_login.c gui/gui_login.h
	$(CC) $(CFLAGS) -c -o $@ $<

gui/gui_patient.o: gui/gui_patient.c gui/gui_patient.h
	$(CC) $(CFLAGS) -c -o $@ $<

gui/gui_doctor.o: gui/gui_doctor.c gui/gui_doctor.h
	$(CC) $(CFLAGS) -c -o $@ $<

gui/gui_admin.o: gui/gui_admin.c gui/gui_admin.h
	$(CC) $(CFLAGS) -c -o $@ $<


$(SRC_DIR)/data_storage.o: $(SRC_DIR)/data_storage.c $(SRC_DIR)/data_storage.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(SRC_DIR)/login.o: $(SRC_DIR)/login.c $(SRC_DIR)/login.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(SRC_DIR)/common.o: $(SRC_DIR)/common.c $(SRC_DIR)/common.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(SRC_DIR)/sha256.o: $(SRC_DIR)/sha256.c $(SRC_DIR)/sha256.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(SRC_DIR)/public.o: $(SRC_DIR)/public.c $(SRC_DIR)/public.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(SRC_DIR)/ui_utils.o: $(SRC_DIR)/ui_utils.c $(SRC_DIR)/ui_utils.h
	$(CC) $(CFLAGS) -c -o $@ $<

app.res: app.rc app.manifest
	windres -O coff -o $@ $<

clean:
	rm -f $(GUI_OBJS) $(RES_OBJS) $(OUTPUT)
