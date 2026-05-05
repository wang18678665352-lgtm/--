# ============================================================================
#  Makefile — GUI 程序 (test_gui.exe) 构建脚本
#  GUI executable (test_gui.exe) build script
#
#  编译器: GCC (MinGW-w64)
#  标准:   C99
#  平台:   Windows (Win32 API + Common Controls)
#
#  构建产物: ../../test_gui.exe  (相对于 gui/ 目录, 在项目根下)
#  Output: ../../test_gui.exe (relative to gui/, at project root)
# ============================================================================

CC = gcc

# 编译选项 / Compilation flags
#   -Wall -Wextra: 启用几乎所有警告
#   -O2:           优化级别 2 (平衡速度与体积)
#   -std=c99:      使用 C99 标准
#   -finput-charset=UTF-8: 源文件编码为 UTF-8
CFLAGS = -Wall -Wextra -O2 -std=c99 -finput-charset=UTF-8

# 链接选项: -mwindows 抑制控制台窗口 (GUI 程序)
# Linker flag: -mwindows suppresses console window for GUI apps
LDFLAGS = -mwindows

# 链接库 / Linked libraries
#   -lcomctl32:  通用控件 (TreeView, ListView, Tab 等)
#   -lcomdlg32:  通用对话框
#   -lgdi32:     GDI 图形绘制
#   -lmsimg32:   透明位图合成 (AlphaBlend 等)
LIBS = -lcomctl32 -lcomdlg32 -lgdi32 -lmsimg32

SRC_DIR = ..
GUI_DIR = .
OUTPUT = ../../test_gui.exe

# GUI 模块源文件 (自动收集 gui/gui_*.c)
# GUI module sources (auto-collect gui/gui_*.c)
GUI_SRCS = $(wildcard $(GUI_DIR)/gui_*.c)

# 核心模块源文件 (来自上级 src/ 目录)
# Core module sources (from parent src/ directory)
CORE_SRCS = \
	$(SRC_DIR)/data_storage.c \
	$(SRC_DIR)/login.c \
	$(SRC_DIR)/common.c \
	$(SRC_DIR)/sha256.c \
	$(SRC_DIR)/public.c \
	$(SRC_DIR)/ui_utils.c

# 所有目标文件 (含 GUI 模块 + 核心模块) + Windows 资源
# All object files (GUI + core modules) + Windows resource
GUI_OBJS = $(GUI_SRCS:.c=.o) $(CORE_SRCS:.c=.o)
RES_OBJS = app.res

.PHONY: all clean

all: $(OUTPUT)

# 最终链接: 将所有 .o + .res 链接为 test_gui.exe
# Final link: all .o + .res → test_gui.exe
$(OUTPUT): $(GUI_OBJS) $(RES_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	@echo ""
	@echo "=== Build OK: $(OUTPUT) ==="

# =======================  GUI 模块编译规则 / GUI Module Rules =======================

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

# =======================  核心模块编译规则 / Core Module Rules =======================

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

# 编译 Windows 资源: .rc + .manifest → .res (COFF 格式)
# Compile Windows resource: .rc + .manifest → .res (COFF format)
app.res: app.rc app.manifest
	windres -O coff -o $@ $<

# 清理构建产物 / Clean build artifacts
clean:
	rm -f $(GUI_OBJS) $(RES_OBJS) $(OUTPUT)
