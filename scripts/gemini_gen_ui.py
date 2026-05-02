#!/usr/bin/env python3
"""
Gemini Win32 UI Code Generator
===============================
读取 UI 设计图 → Gemini Vision API 分析 → 生成 Win32 C 代码

用法:
  1. 把 gptimage2 生成的图片放到 docs/design/ 目录
  2. 设置 API Key:
     set GEMINI_API_KEY=AIzaSyC71MZ5w0hpLbyfl1eRMx2zSzST9zGtzE4
  3. 运行脚本:
     python scripts/gemini_gen_ui.py --screen docs/design/login.png
     python scripts/gemini_gen_ui.py --all      # 处理所有图片
     python scripts/gemini_gen_ui.py --list     # 列出支持的画面类型

支持的画面类型:
  login      登录窗口
  main       主窗口框架（菜单/导航）
  patient    患者端主界面
  doctor     医生端主界面
  admin      管理员端主界面
  form       通用数据表单（增/改）
  table      通用数据表格（列表/搜索）
"""

import argparse
import base64
import json
import os
import re
import sys
import urllib.request
import urllib.error
from pathlib import Path

# ─── 配置 ────────────────────────────────────────────────────────────────
PROJECT_ROOT = Path(__file__).resolve().parent.parent
DESIGN_DIR = PROJECT_ROOT / "docs" / "design"
GUI_DIR = PROJECT_ROOT / "src" / "gui"
API_KEY_ENV = "GEMINI_API_KEY"

# Gemini 模型（支持视觉）
MODEL = "gemini-2.5-flash"

# 支持的图片格式
SUPPORTED_EXT = {".png", ".jpg", ".jpeg", ".webp"}

# ─── 项目上下文（注入到 prompt 中，让 Gemini 理解已有代码）──────────────

PROJECT_CONTEXT = """
## 项目概况
- 项目名：电子医疗管理系统
- 语言：纯 C（C99）
- 当前界面：Windows 终端 CLI（ANSI 转义序列）
- 目标：替换为 Win32 原生 GUI 桌面应用
- 数据存储：链表 + TSV 文本文件（data/*.txt）

## 角色与功能
- 患者（patient）：挂号、查诊断、查处方、查看住院、查治疗进度、编辑个人信息
- 医生（doctor）：接诊提醒、接诊录入、开处方、病房呼叫、标记紧急、更新进度、病历模板
- 管理员（admin）：科室/医生/患者/药品/病房 CRUD、排班管理、操作日志、数据备份、报表统计

## 核心数据结构（data_storage.h）
typedef struct {
    char patient_id[20]; char username[50]; char name[100];
    char gender[10]; int age; char phone[20]; char address[200];
    char patient_type[20]; char treatment_stage[20]; bool is_emergency;
} Patient;

typedef struct {
    char doctor_id[20]; char username[50]; char name[100];
    char department_id[20]; char title[50]; int busy_level;
} Doctor;

typedef struct {
    char department_id[20]; char name[100]; char leader[100]; char phone[20];
} Department;

typedef struct {
    char drug_id[20]; char name[100]; float price; int stock_num;
    int warning_line; bool is_special; float reimbursement_ratio; char category[20];
} Drug;

typedef struct {
    char ward_id[20]; char type[50]; int total_beds; int remain_beds; int warning_line;
} Ward;

typedef struct {
    char appointment_id[20]; char patient_id[20]; char doctor_id[20];
    char department_id[20]; char appointment_date[20]; char appointment_time[20];
    char status[20]; char create_time[30];
} Appointment;

typedef struct {
    char record_id[20]; char patient_id[20]; char doctor_id[20];
    char appointment_id[20]; char diagnosis[500]; char diagnosis_date[20]; char status[20];
} MedicalRecord;

typedef struct {
    char prescription_id[20]; char record_id[20]; char patient_id[20];
    char doctor_id[20]; char drug_id[20]; int quantity; float total_price;
    char prescription_date[20];
} Prescription;

typedef struct {
    char username[50]; char password[65]; char role[20];
} User;

typedef struct {
    User current_user; bool logged_in;
} Session;

## 已有函数接口（可直接调用）
头文件: data_storage.h, common.h, login.h, public.h
- UserNode* load_users_list(void) / int save_users_list(UserNode*)
- PatientNode* load_patients_list(void) / int save_patients_list(PatientNode*)
- DoctorNode* load_doctors_list(void) / int save_doctors_list(DoctorNode*)
- DepartmentNode* load_departments_list(void) / int save_departments_list(DepartmentNode*)
- DrugNode* load_drugs_list(void) / int save_drugs_list(DrugNode*)
- WardNode* load_wards_list(void) / int save_wards_list(WardNode*)
- AppointmentNode* load_appointments_list(void) / int save_appointments_list(AppointmentNode*)
- MedicalRecordNode* load_medical_records_list(void) / int save_medical_records_list(MedicalRecordNode*)
- PrescriptionNode* load_prescriptions_list(void) / int save_prescriptions_list(PrescriptionNode*)
- int login(User *logged_user) / int register_user(User *new_user) / void logout(Session*)
- int has_permission(const User*, const char*)
- Patient* find_patient_by_username(const char*)
- Doctor* find_doctor_by_username(const char*)
- DepartmentNode* load_departments_list(void)
- Drug* find_drug_by_id(const char*)
- int backup_data(void) / int restore_data(const char*)
- float calculate_reimbursement(float, const char*)
- int append_log(const char*, const char*, const char*, const char*, const char*)
- void get_current_time(char*, int) / void generate_id(char*, int, const char*)
"""

# ─── Prompt 模板 ────────────────────────────────────────────────────────

def build_system_prompt(screen_type):
    """构建系统级 prompt，描述 Win32 编码规范"""
    base = f"""你是一个 Win32 API 专家。根据用户提供的 UI 设计图，生成对应的 Win32 C 语言代码。

## 编码规范（必须遵守）
1. 使用纯 Win32 API（windows.h），不能使用 MFC、.NET、Qt 等框架
2. C 语言（C99 标准），不能使用 C++
3. 不能使用任何第三方库
4. 使用 UTF-8 编码
5. 每个文件都有 #ifndef/#define/#endif 头文件保护
6. Windows XP 及以上兼容（至少支持 Windows 7）

## 通用设计要求
1. 使用 Windows 通用控件（WC_LISTVIEW、WC_TREEVIEW、WC_TABCONTROL 等）
2. 在 InitCommonControls() 中初始化通用控件
3. ListView 使用 LVS_REPORT 模式
4. Tab 控件管理多页面
5. 窗口大小建议 1024x768，支持最小化到 800x600
6. 配色使用 Windows 系统颜色（COLOR_WINDOW、COLOR_HIGHLIGHT 等），或用柔和的浅蓝/白色系
7. 中文字体使用 "Microsoft YaHei UI" 或 "Tahoma"，字号 9-10
8. 对话框使用 DialogBox/DialogBoxParam，资源用 CreateWindow 创建（不使用 .rc 文件）

## 已有的 C 后端的集成
- 所有数据操作通过 #include "data_storage.h" 调用现有函数
- 在 .c 文件中 #include "../data_storage.h" 和 "../common.h"
- 不要重新实现数据存储逻辑，调用已有函数

## 代码输出格式
- 输出完整的 .c 和 .h 文件内容
- .h 文件包含所有导出函数声明和窗口过程声明
- .c 文件包含完整的实现（WndProc、对话框过程、控件初始化）

{PROJECT_CONTEXT}
"""
    return base


SCREEN_PROMPTS = {
    "login": """根据设计图生成"登录窗口"的 Win32 实现。

## 功能要求
1. 模态对话框，标题"电子医疗管理系统 - 登录"
2. 三个单选按钮：管理员、医生、患者（默认选中一个）
3. 用户名输入框 + 密码输入框（密码显示为 ***）
4. "登录"按钮 + "注册"按钮 + "退出"按钮
5. 登录按钮点击后调用 login() 函数
6. 注册按钮弹出注册对话框
7. 登录成功后关闭对话框，通过输出参数返回 User 信息
8. 显示错误信息（用户名/密码错误等）

## 函数接口
// 显示登录对话框，返回 SUCCESS/ERROR，user 参数返回登录的用户信息
int ShowLoginDialog(HINSTANCE hInstance, HWND hParent, User *user);

// 登录对话框过程
INT_PTR CALLBACK LoginDlgProc(HWND hDlg, UINT msg, WPARAM w, LPARAM l);

// 注册对话框过程
INT_PTR CALLBACK RegisterDlgProc(HWND hDlg, UINT msg, WPARAM w, LPARAM l);

## 文件
- gui_login.h
- gui_login.c
""",

    "main": """根据设计图生成"主窗口框架"的 Win32 实现。

## 功能要求
1. 主窗口：标题"电子医疗管理系统"，大小 1024x768，居中显示
2. 顶部菜单栏（非 .rc 资源，用 CreateWindow 创建）：
   - 文件：退出
   - 视图：患者端/医生端/管理员端（根据登录角色启用）
   - 帮助：关于
3. 左侧导航面板（TreeView 或 ListBox，根据角色显示不同菜单项）
4. 右侧客户区（Tab 控件或 Panel，用于显示子页面）
5. 底部状态栏（显示当前用户、角色、时间）
6. 窗口最小化到 800x600

## 导航结构
患者导航：
  ├ 挂号
  ├ 预约查询
  ├ 诊断结果
  ├ 住院信息
  ├ 治疗进度
  └ 个人信息

医生导航：
  ├ 待接诊
  ├ 接诊
  ├ 开处方
  ├ 病房呼叫
  ├ 紧急标记
  ├ 进度更新
  └ 病历模板

管理员导航：
  ├ 科室管理
  ├ 医生管理
  ├ 患者管理
  ├ 药品管理
  ├ 病房管理
  ├ 排班管理
  ├ 操作日志
  ├ 数据管理
  └ 报表统计

## 函数接口
// 创建主窗口并进入消息循环
int RunMainWindow(HINSTANCE hInstance, const User *current_user);

// 主窗口过程
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM w, LPARAM l);

// 切换右侧视图
void SwitchView(HWND hWnd, int viewId);

## 文件
- gui_main.h
- gui_main.c
""",

    "patient": """根据设计图生成"患者端"各页面的 Win32 实现。

## 功能要求
1. 挂号页面：
   - 步骤 1：选择科室（ComboBox 或 ListView）
   - 步骤 2：选择医生（ListView 显示医生信息，包括繁忙度推荐）
   - 步骤 3：选择日期和时段（上午/下午）
   - 步骤 4：确认挂号（显示信息 + 确认按钮）
   - 调用 create_appointment_node + save_appointments_list

2. 预约查询页面：
   - ListView 显示本人所有预约记录
   - 列：预约ID、医生、科室、日期、时间、状态
   - 支持取消预约（按钮）

3. 诊断结果页面：
   - ListView 显示病历列表
   - 选中后下方显示详细诊断信息

4. 处方查询页面：
   - ListView 显示处方列表
   - 选中后显示处方详情（药品、数量、金额）

5. 住院信息页面：
   - 显示病房信息（科室、病房类型、床位号）
   - 显示病房呼叫记录

6. 治疗进度页面：
   - 进度条或步骤指示器显示治疗阶段
   - 文字描述当前阶段和下一阶段

7. 个人信息页面：
   - 可编辑的表单字段（姓名、性别、年龄、电话、地址）
   - 保存按钮调用 save_patients_list

## 文件
- gui_patient.h
- gui_patient.c
""",

    "doctor": """根据设计图生成"医生端"各页面的 Win32 实现。

## 功能要求
1. 待接诊页面：
   - ListView 显示待接诊的挂号列表（预约 + 现场）
   - 点击"接诊"按钮进入接诊页面

2. 接诊页面：
   - 上方显示患者基本信息
   - 诊断录入（多行文本框 Edit）
   - 治疗建议（多行文本框）
   - 检查项目（多行文本框）
   - "保存诊断"按钮 → 调用 create_medical_record_node
   - "开处方"按钮跳转到处方页面

3. 开处方页面：
   - 药品搜索选择（ComboBox 自动补全调 smart_drug_input 逻辑）
   - 数量输入
   - 已选药品列表（ListView）
   - 总计金额
   - 报销金额
   - "添加药品"、"删除选中"、"保存处方"按钮

4. 病房呼叫页面：
   - ListView 显示来自病房的呼叫
   - "响应"按钮更新呼叫状态

5. 紧急标记页面：
   - ComboBox 选择患者
   - CheckBox 标记紧急
   - 保存更新

6. 进度更新页面：
   - 选择患者
   - ComboBox 选择下一阶段
   - "更新"按钮

7. 病历模板页面：
   - ListView 显示模板列表
   - 分类筛选
   - 选择后模板内容插入到诊断输入框

## 文件
- gui_doctor.h
- gui_doctor.c
""",

    "admin": """根据设计图生成"管理员端"各页面的 Win32 实现。

## 功能要求
1. 科室管理：ListView 显示科室列表 + 增/改/删按钮 + 表单对话框
2. 医生管理：ListView + 增/改/删/查看工作量
3. 患者管理：ListView + 增/改/删 + 搜索过滤
4. 药品管理：ListView + 增/改/删 + 补货 + 库存预警高亮
5. 病房管理：ListView + 增/改/删 + 床位预警高亮
6. 排班管理：日期选择器 + 医生选择 + 时段 + 批量生成
7. 操作日志：ListView 只读 + 按操作人/对象/时间筛选
8. 数据管理：备份/恢复按钮 + 备份列表
9. 报表统计：Tab 页切换不同报表（概览、趋势、医生负载、患者分析、财务）

## 通用要求
- 所有 ListView 使用 LVS_REPORT 模式，支持列排序
- 增/改弹出模态对话框
- 删除前弹出确认对话框
- 预警数据用红色或黄色高亮
- 搜索框 + 搜索按钮在 ListView 上方

## 文件
- gui_admin.h
- gui_admin.c
""",

    "form": """根据设计图生成"通用表单对话框"的 Win32 实现，用于数据录入和编辑。

## 功能要求
1. 模态对话框，标题根据操作类型动态设置
2. 支持以下字段类型的自动生成：
   - 文本输入（Edit Control）
   - 下拉选择（ComboBox）
   - 数字输入（Edit + 数字校验）
   - 日期输入（Edit + 格式校验）
   - 金额输入（Edit + 小数校验）
   - 多行文本（Edit ES_MULTILINE）
   - 复选框（CheckBox）
   - 单选按钮组（RadioButton）
3. "确定"和"取消"按钮
4. 输入校验：空值检查、数字范围、日期格式

## 函数接口
// 通用表单：根据 FieldDefs 数组动态生成表单
typedef struct {
    const char *label;     // 字段标签
    int controlType;       // 0=文本, 1=下拉, 2=数字, 3=日期, 4=金额, 5=多行, 6=复选框, 7=单选
    char buffer[256];      // 输入/输出缓冲区
    int bufferSize;
    int minVal;            // 数字/金额最小值
    int maxVal;            // 数字/金额最大值
    const char **options;  // 下拉选项（NULL-terminated）
    bool required;         // 是否必填
} FieldDef;

int ShowFormDialog(HWND hParent, const char *title, FieldDef *fields, int fieldCount);

## 文件
- gui_controls.h
- gui_controls.c
""",

    "table": """根据设计图生成"通用数据表格页面"的 Win32 实现。

## 功能要求
1. ListView 控件（LVS_REPORT），支持：
   - 多列显示
   - 列头可点击排序
   - 行选中高亮
   - 交替行背景色
2. 上方工具栏：
   - 搜索框 + 搜索按钮（实时过滤）
   - "新增"按钮
   - "编辑"按钮
   - "删除"按钮
   - "刷新"按钮
3. 右键上下文菜单（新增、编辑、删除、刷新）
4. 分页控件（上一页/下一页/页码跳转）
5. ListView 虚拟模式（LVN_GETDISPINFO）用于大数据集

## 函数接口
// 初始化通用列表页
// parent: 父窗口句柄
// id: 控件ID
// columns: 列定义数组
// colCount: 列数
// rect: 位置和大小
HWND CreateListViewPage(HWND parent, int id, const LVCOLUMN *columns, int colCount, RECT *rect);

// 通用表格页面窗口过程
LRESULT CALLBACK TablePageWndProc(HWND hWnd, UINT msg, WPARAM w, LPARAM l);

## 文件
- gui_controls.h (同上一个文件)
""",
}


# ─── 辅助函数 ───────────────────────────────────────────────────────────

def get_api_key():
    """获取 API Key"""
    key = os.environ.get(API_KEY_ENV)
    if not key:
        # 尝试从 .env 文件读取
        env_file = PROJECT_ROOT / ".env"
        if env_file.exists():
            for line in env_file.read_text(encoding="utf-8").splitlines():
                line = line.strip()
                if line.startswith(API_KEY_ENV):
                    key = line.split("=", 1)[1].strip().strip("\"'")
                    break
    if not key:
        print(f"错误: 未找到 API Key。请设置环境变量 {API_KEY_ENV}")
        print(f"  set {API_KEY_ENV}=your_key")
        sys.exit(1)
    return key


def encode_image(image_path):
    """将图片编码为 base64"""
    with open(image_path, "rb") as f:
        return base64.b64encode(f.read()).decode("utf-8")


def get_mime_type(ext):
    """根据文件扩展名获取 MIME 类型"""
    mime_map = {
        ".png": "image/png",
        ".jpg": "image/jpeg",
        ".jpeg": "image/jpeg",
        ".webp": "image/webp",
    }
    return mime_map.get(ext.lower(), "image/png")


def call_gemini_api(api_key, image_base64, mime_type, screen_type):
    """调用 Gemini Vision API"""
    url = f"https://generativelanguage.googleapis.com/v1beta/models/{MODEL}:generateContent?key={api_key}"

    system_prompt = build_system_prompt(screen_type)
    user_prompt = SCREEN_PROMPTS.get(screen_type, "")
    if not user_prompt:
        print(f"错误: 未知的画面类型 '{screen_type}'")
        sys.exit(1)

    # 构建请求体
    payload = {
        "contents": [{
            "parts": [
                {"text": system_prompt + "\n\n---\n\n用户需求：\n" + user_prompt},
                {
                    "inline_data": {
                        "mime_type": mime_type,
                        "data": image_base64
                    }
                }
            ]
        }],
        "generationConfig": {
            "temperature": 0.3,   # 较低温度，更确定性的输出
            "maxOutputTokens": 8192,
            "topP": 0.95,
        }
    }

    data = json.dumps(payload).encode("utf-8")
    req = urllib.request.Request(
        url,
        data=data,
        headers={"Content-Type": "application/json"},
        method="POST"
    )

    print(f"  正在调用 Gemini API ({MODEL})...")
    try:
        with urllib.request.urlopen(req, timeout=120) as resp:
            result = json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        print(f"  HTTP 错误: {e.code}")
        print(f"  响应: {e.read().decode('utf-8')}")
        return None
    except urllib.error.URLError as e:
        print(f"  网络错误: {e.reason}")
        return None
    except json.JSONDecodeError:
        print("  响应不是有效的 JSON")
        return None

    # 提取生成的文本
    try:
        candidates = result.get("candidates", [])
        if not candidates:
            print(f"  错误: 没有候选响应")
            print(f"  完整响应: {json.dumps(result, indent=2, ensure_ascii=False)}")
            return None
        text = candidates[0].get("content", {}).get("parts", [{}])[0].get("text", "")
        if not text:
            print(f"  警告: 响应中没有文本内容")
            return None
        return text
    except (KeyError, IndexError, TypeError) as e:
        print(f"  解析响应失败: {e}")
        print(f"  原始响应 (前500字): {json.dumps(result, indent=2, ensure_ascii=False)[:500]}")
        return None


def extract_code_blocks(text, screen_type):
    """从生成的文本中提取代码块，保存为文件"""
    if not text:
        return []

    # 查找所有 ```c 和 ```h 代码块
    pattern = r"```([ch])\s*\n(.*?)```"
    matches = re.findall(pattern, text, re.DOTALL)

    if not matches:
        # 如果没有代码块标记，整个文本可能包含代码
        print("  ⚠ 未找到标记的代码块，尝试直接保存完整输出")
        return []

    saved_files = []
    for lang, code in matches:
        code = code.strip()
        if not code:
            continue

        # 从代码中推断文件名
        # 检查 #include "gui_xxx.h" 或类似模式
        filename = None
        include_match = re.search(r'#include\s+"(gui_\w+\.h)"', code)
        if include_match:
            basename = include_match.group(1).replace(".h", "")
            if lang == "h":
                filename = f"{basename}.h"
            else:
                filename = f"{basename}.c"

        if not filename:
            # 检查 #ifndef GUI_XXX_H
            ifndef_match = re.search(r'#ifndef\s+(GUI_\w+_H)', code)
            if ifndef_match:
                basename = ifndef_match.group(1).lower().replace("_h", "")
                if lang == "h":
                    filename = f"{basename}.h"
                else:
                    filename = f"{basename}.c"

        if not filename:
            # 默认命名
            filename = f"gui_{screen_type}.{lang}"

        filepath = GUI_DIR / filename
        filepath.write_text(code, encoding="utf-8")
        saved_files.append(str(filepath))

    return saved_files


def save_generated_text(text, screen_type):
    """如果代码提取失败，保存原始文本为 .md"""
    output_file = GUI_DIR / f"gemini_{screen_type}_output.md"
    output_file.write_text(text, encoding="utf-8")
    return str(output_file)


def process_single_image(api_key, image_path):
    """处理单张图片"""
    image_path = Path(image_path)
    if not image_path.exists():
        print(f"错误: 文件不存在 {image_path}")
        return

    # 判断画面类型（从文件名推断）
    stem = image_path.stem.lower()
    screen_type = None
    for key in SCREEN_PROMPTS.keys():
        if key in stem:
            screen_type = key
            break

    if not screen_type:
        print(f"无法从文件名推断画面类型，可用的类型:")
        for k in SCREEN_PROMPTS.keys():
            print(f"  {k}")
        screen_type = input(f"请为 {image_path.name} 输入画面类型: ").strip()
        if screen_type not in SCREEN_PROMPTS:
            print(f"错误: 未知类型 '{screen_type}'")
            return

    print(f"\n处理: {image_path.name} → 类型: {screen_type}")

    # 编码图片
    mime_type = get_mime_type(image_path.suffix)
    img_b64 = encode_image(image_path)
    img_size_kb = len(img_b64) * 3 / 4 / 1024
    print(f"  图片大小: {img_size_kb:.0f} KB")

    if img_size_kb > 5000:
        print(f"  ⚠ 图片过大，建议压缩到 5MB 以下")

    # 调用 Gemini
    result = call_gemini_api(api_key, img_b64, mime_type, screen_type)
    if not result:
        return

    # 保存结果
    saved = extract_code_blocks(result, screen_type)
    if saved:
        print(f"  ✓ 已生成代码文件:")
        for f in saved:
            size = os.path.getsize(f)
            print(f"    {f} ({size} bytes)")
    else:
        md_file = save_generated_text(result, screen_type)
        print(f"  ⚠ 未提取到代码块，原始输出已保存到:")
        print(f"    {md_file}")

    # 打印 token 使用统计
    print(f"  ✓ 完成: {image_path.name}")


def process_all(api_key):
    """处理 docs/design/ 下所有图片"""
    if not DESIGN_DIR.exists():
        print(f"错误: 目录不存在 {DESIGN_DIR}")
        print(f"请先在 {DESIGN_DIR} 中放入设计图")
        return

    images = []
    for f in sorted(DESIGN_DIR.iterdir()):
        if f.suffix.lower() in SUPPORTED_EXT:
            images.append(f)

    if not images:
        print(f"在 {DESIGN_DIR} 中未找到图片文件")
        print(f"支持的格式: {', '.join(SUPPORTED_EXT)}")
        return

    print(f"找到 {len(images)} 张图片，开始处理...")
    for img in images:
        process_single_image(api_key, img)

    print(f"\n全部处理完成！代码已生成到: {GUI_DIR}")


def list_screen_types():
    """列出所有支持的画面类型"""
    print("支持的画面类型:\n")
    for key, prompt in SCREEN_PROMPTS.items():
        desc = prompt.strip().split("\n")[0] if prompt.strip() else "无描述"
        print(f"  {key:10s} - {desc}")
    print()
    print("将图片放入 docs/design/ 目录后，脚本会根据文件名自动匹配类型。")
    print("例如: login_main.png → login, doctor_patient_list.png → doctor")


# ─── 主入口 ─────────────────────────────────────────────────────────────

def main():
    global MODEL
    parser = argparse.ArgumentParser(
        description="Gemini Win32 UI Code Generator - 从设计图生成 Win32 GUI 代码"
    )
    parser.add_argument("--screen", type=str,
                        help="单张图片路径")
    parser.add_argument("--all", action="store_true",
                        help="处理 docs/design/ 下所有图片")
    parser.add_argument("--list", action="store_true",
                        help="列出支持的画面类型")
    parser.add_argument("--model", type=str, default=MODEL,
                        help=f"Gemini 模型名（默认: {MODEL}）")

    args = parser.parse_args()

    # 确保输出目录存在
    GUI_DIR.mkdir(parents=True, exist_ok=True)

    if args.list:
        list_screen_types()
        return

    if not args.screen and not args.all:
        parser.print_help()
        print("\n请指定 --screen <路径> 或 --all")
        return

    if args.model:
        MODEL = args.model

    api_key = get_api_key()

    if args.all:
        process_all(api_key)
    elif args.screen:
        process_single_image(api_key, args.screen)


if __name__ == "__main__":
    main()
