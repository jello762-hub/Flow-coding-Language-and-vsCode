/*
 * Flow Programming Language v2 - Simplified Syntax
 * A human-readable language that's as simple as pseudocode
 * but powerful enough for real programs
 *
 * NEW SIMPLE SYNTAX:
 *   let x = 10                    # variables
 *   say "hello"                   # output
 *   func add(a, b)               # functions
 *       return a + b
 *   end
 *   if x > 5                     # conditionals
 *       say "big"
 *   end
 *   repeat 5 times               # loops
 *       say "hi"
 *   end
 *   while x < 10                 # while loops
 *       let x = x + 1
 *   end
 *
 * ALSO SUPPORTS VERBOSE SYNTAX (backwards compatible):
 *   set x to 10
 *   display "hello"
 *   function add taking a, b
 *   increase x by 1
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#ifdef _WIN32
#define strcasecmp _stricmp
#endif

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#ifdef _WIN32
#include <windows.h>
static HWND g_hwnd = NULL;
static HDC g_hdc = NULL;
static int g_win_width = 640;
static int g_win_height = 480;
static char g_win_title[256] = "Flow Window";
static int g_keys[256] = {0};
static int g_mouse_x = 0, g_mouse_y = 0, g_mouse_down = 0;
static COLORREF g_bg_color = RGB(30, 30, 30);
static COLORREF g_pen_color = RGB(255, 255, 255);
static int g_pen_size = 2;
static HBRUSH g_brush = NULL;
static HPEN g_pen = NULL;

LRESULT CALLBACK FlowWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_KEYDOWN:
            if (wParam < 256) g_keys[wParam] = 1;
            return 0;
        case WM_KEYUP:
            if (wParam < 256) g_keys[wParam] = 0;
            return 0;
        case WM_LBUTTONDOWN:
            g_mouse_down = 1;
            g_mouse_x = LOWORD(lParam);
            g_mouse_y = HIWORD(lParam);
            return 0;
        case WM_LBUTTONUP:
            g_mouse_down = 0;
            return 0;
        case WM_MOUSEMOVE:
            g_mouse_x = LOWORD(lParam);
            g_mouse_y = HIWORD(lParam);
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            g_hdc = BeginPaint(hwnd, &ps);
            if (g_brush) FillRect(g_hdc, &ps.rcPaint, g_brush);
            EndPaint(hwnd, &ps);
            g_hdc = GetDC(hwnd);
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void ensure_window() {
    if (g_hwnd) return;
    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASS wc = {0};
    wc.lpfnWndProc = FlowWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "FlowWindow";
    wc.hbrBackground = CreateSolidBrush(g_bg_color);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    RECT rc = {0, 0, g_win_width, g_win_height};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    g_hwnd = CreateWindow("FlowWindow", g_win_title,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, hInstance, NULL);

    g_hdc = GetDC(g_hwnd);
    g_brush = CreateSolidBrush(g_bg_color);
    g_pen = CreatePen(PS_SOLID, g_pen_size, g_pen_color);
    SelectObject(g_hdc, g_brush);
    SelectObject(g_hdc, g_pen);
}

static void pump_messages() {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
#endif

#define MAX_VARS 256
#define MAX_FUNCS 64
#define MAX_STACK 128
#define MAX_ARGS 16
#define MAX_INPUT 8192

/* ===== TYPES ===== */
typedef enum { VAL_NUMBER, VAL_STRING, VAL_BOOL, VAL_NULL, VAL_FUNC } ValueType;

typedef struct Value Value;
typedef struct Func Func;

struct Value {
    ValueType type;
    union {
        double number;
        char* string;
        int boolean;
        Func* func;
    } as;
};

struct Func {
    char* name;
    char* params[MAX_ARGS];
    int param_count;
    char** body;
    int body_lines;
};

/* ===== GLOBALS ===== */
typedef struct {
    char* name;
    Value value;
} Var;

typedef struct {
    char* tokens[MAX_INPUT];
    int token_count;
    int pos;
    int returning;
    int skipping;
} Lexer;

typedef struct {
    Var vars[MAX_VARS];
    int var_count;
    Func funcs[MAX_FUNCS];
    int func_count;
    Value stack[MAX_STACK];
    int stack_top;
} VM;

static Lexer lexer;
static VM vm;
static int g_debug_mode = 0;
static char* g_breakpoints[64];
static int g_bp_count = 0;
static int g_debug_continue = 0; /* when true, continue until breakpoint */

/* ===== FORWARD DECLARATIONS ===== */
static Value parse_expression();
static Value parse_comparison();
static Value parse_addition();
static Value parse_multiplication();
static Value parse_unary();
static Value parse_primary();
static void execute_statement();
static Value call_function(Func* func, Value* args, int argc);
static const char* peek();
static const char* advance();
static int match(const char* expected);

/* ===== MEMORY ===== */
static char* strdup_safe(const char* s) {
    if (!s) return NULL;
    char* copy = malloc(strlen(s) + 1);
    if (copy) strcpy(copy, s);
    return copy;
}

static void free_value(Value* v) {
    if (v->type == VAL_STRING && v->as.string) { free(v->as.string); v->as.string = NULL; }
}

/* ===== VALUE CREATION ===== */
static Value make_number(double n) { Value v = {VAL_NUMBER, .as.number = n}; return v; }
static Value make_string(const char* s) { Value v = {VAL_STRING, .as.string = strdup_safe(s)}; return v; }
static Value make_bool(int b) { Value v = {VAL_BOOL, .as.boolean = b}; return v; }
static Value make_null() { Value v = {0}; v.type = VAL_NULL; return v; }

/* ===== STACK ===== */
static void push(Value v) { vm.stack[vm.stack_top++] = v; }
static Value pop() { return vm.stack[--vm.stack_top]; }

/* ===== VALUE UTILITIES ===== */
static double value_to_number(Value v) {
    switch (v.type) {
        case VAL_NUMBER: return v.as.number;
        case VAL_STRING: return atof(v.as.string ? v.as.string : "0");
        case VAL_BOOL: return v.as.boolean ? 1.0 : 0.0;
        default: return 0.0;
    }
}

static int value_to_bool(Value v) {
    switch (v.type) {
        case VAL_BOOL: return v.as.boolean;
        case VAL_NUMBER: return v.as.number != 0.0;
        case VAL_STRING: return v.as.string && v.as.string[0] != '\0';
        default: return 0;
    }
}

static const char* token_or_eof(int pos) {
    const char* t = (pos >= 0 && pos < lexer.token_count) ? lexer.tokens[pos] : NULL;
    return t ? t : "<EOF>";
}

static void error_with_context(const char* msg) {
    int pos = lexer.pos;
    fprintf(stderr, "Error: %s near token %d ('%s')\n", msg, pos, token_or_eof(pos));
    fprintf(stderr, "  context: '%s' '%s' '%s'\n", token_or_eof(pos-1), token_or_eof(pos), token_or_eof(pos+1));
    exit(1);
}

static int is_current_breakpoint() {
    const char* t = peek();
    if (!t) return 0;
    for (int i = 0; i < g_bp_count; i++) if (g_breakpoints[i] && strcmp(g_breakpoints[i], t) == 0) return 1;
    return 0;
}

static void handle_debug_prompt() {
    printf("[debug] pos=%d token='%s'\n", lexer.pos, token_or_eof(lexer.pos));
    /* if stdin is not a TTY (non-interactive run), log and auto-continue */
#ifdef _WIN32
    int stdin_tty = _isatty(_fileno(stdin));
#else
    int stdin_tty = isatty(fileno(stdin));
#endif
    if (!stdin_tty) {
        printf("[debug] non-interactive run - hit token '%s' at pos %d\n", token_or_eof(lexer.pos), lexer.pos);
        g_debug_continue = 1; /* auto-continue in non-interactive scenarios */
        return;
    }

    printf("debug> "); fflush(stdout);
    char dbuf[256]; if (!fgets(dbuf, sizeof(dbuf), stdin)) return;
    /* trim newline */
    size_t L = strlen(dbuf); if (L && dbuf[L-1] == '\n') dbuf[L-1] = '\0';
    if (strcmp(dbuf, "c") == 0 || strcmp(dbuf, "continue") == 0) { g_debug_continue = 1; return; }
    if (strcmp(dbuf, "s") == 0 || strcmp(dbuf, "step") == 0) { g_debug_continue = 0; return; }
    if (strcmp(dbuf, "q") == 0 || strcmp(dbuf, "quit") == 0) { exit(0); }
    if (strcmp(dbuf, "p") == 0 || strcmp(dbuf, "print") == 0) {
        for (int i = 0; i < vm.var_count; i++) {
            Value *vv = &vm.vars[i].value;
            if (vv->type == VAL_STRING) printf("%s = \"%s\"\n", vm.vars[i].name, vv->as.string);
            else if (vv->type == VAL_NUMBER) printf("%s = %g\n", vm.vars[i].name, vv->as.number);
            else if (vv->type == VAL_BOOL) printf("%s = %s\n", vm.vars[i].name, vv->as.boolean ? "true" : "false");
            else printf("%s = null\n", vm.vars[i].name);
        }
        return;
    }
    if (strncmp(dbuf, "b ", 2) == 0) {
        const char* tk = dbuf + 2;
        if (g_bp_count < 64) g_breakpoints[g_bp_count++] = strdup_safe(tk);
        printf("Breakpoint set on '%s'\n", tk);
        return;
    }
    if (strcmp(dbuf, "bl") == 0 || strcmp(dbuf, "breakpoints") == 0) {
        for (int i = 0; i < g_bp_count; i++) printf("%d: %s\n", i, g_breakpoints[i]);
        return;
    }
    if (strncmp(dbuf, "db ", 3) == 0) {
        int id = atoi(dbuf + 3);
        if (id >= 0 && id < g_bp_count) { free(g_breakpoints[id]); for (int j = id; j < g_bp_count-1; j++) g_breakpoints[j] = g_breakpoints[j+1]; g_bp_count--; }
        return;
    }
    if (strcmp(dbuf, "h") == 0 || strcmp(dbuf, "help") == 0) {
        printf("debug commands: s/step, c/continue, p/print, b <token>, bl, db <id>, q/quit\n");
        return;
    }
    /* default: step */
    g_debug_continue = 0;
}

/* ===== VARIABLES ===== */
static int find_var(const char* name) {
    for (int i = vm.var_count - 1; i >= 0; i--)
        if (strcmp(vm.vars[i].name, name) == 0) return i;
    return -1;
}

static Value get_var(const char* name) {
    int idx = find_var(name);
    if (idx >= 0) return vm.vars[idx].value;
    return make_null();
}

static void set_var(const char* name, Value v) {
    int idx = find_var(name);
    if (idx >= 0) { free_value(&vm.vars[idx].value); vm.vars[idx].value = v; return; }
    vm.vars[vm.var_count].name = strdup_safe(name);
    vm.vars[vm.var_count].value = v;
    vm.var_count++;
}

/* ===== FUNCTIONS ===== */
static int find_func(const char* name) {
    for (int i = 0; i < vm.func_count; i++)
        if (strcmp(vm.funcs[i].name, name) == 0) return i;
    return -1;
}

static Func* get_func(const char* name) {
    int idx = find_func(name);
    if (idx >= 0) return &vm.funcs[idx];
    return NULL;
}

/* ===== NATIVE FUNCTIONS ===== */
static Value native_say(Value* args, int argc) {
    if (lexer.skipping) return make_null();
    for (int i = 0; i < argc; i++) {
        if (i > 0) printf(" ");
        switch (args[i].type) {
            case VAL_NUMBER:
                if (args[i].as.number == (int)args[i].as.number)
                    printf("%d", (int)args[i].as.number);
                else
                    printf("%g", args[i].as.number);
                break;
            case VAL_STRING: printf("%s", args[i].as.string); break;
            case VAL_BOOL: printf(args[i].as.boolean ? "true" : "false"); break;
            default: printf("null"); break;
        }
    }
    printf("\n");
    return make_null();
}

static Value native_input(Value* args, int argc) {
    char buffer[MAX_INPUT];
    if (argc > 0 && args[0].type == VAL_STRING) printf("%s", args[0].as.string);
    if (fgets(buffer, sizeof(buffer), stdin)) { buffer[strcspn(buffer, "\n")] = 0; return make_string(buffer); }
    return make_null();
}

static Value native_length(Value* args, int argc) {
    if (argc < 1) return make_number(0);
    if (args[0].type == VAL_STRING) return make_number(strlen(args[0].as.string));
    return make_number(0);
}

static Value native_to_number(Value* args, int argc) {
    if (argc < 1) return make_number(0);
    if (args[0].type == VAL_NUMBER) return args[0];
    if (args[0].type == VAL_STRING) return make_number(atof(args[0].as.string));
    return make_number(0);
}

static Value native_to_string(Value* args, int argc) {
    if (argc < 1) return make_string("");
    char buffer[256];
    if (args[0].type == VAL_NUMBER) {
        if (args[0].as.number == (int)args[0].as.number) sprintf(buffer, "%d", (int)args[0].as.number);
        else sprintf(buffer, "%g", args[0].as.number);
        return make_string(buffer);
    }
    if (args[0].type == VAL_BOOL) return make_string(args[0].as.boolean ? "true" : "false");
    return make_string("null");
}

static Value native_random(Value* args, int argc) {
    if (argc == 0) return make_number((double)rand() / RAND_MAX);
    if (argc == 2 && args[0].type == VAL_NUMBER && args[1].type == VAL_NUMBER) {
        return make_number(rand() % ((int)args[1].as.number - (int)args[0].as.number + 1) + (int)args[0].as.number);
    }
    return make_number((double)rand() / RAND_MAX);
}

static Value native_abs(Value* args, int argc) {
    if (argc < 1 || args[0].type != VAL_NUMBER) return make_number(0);
    return make_number(fabs(args[0].as.number));
}

static Value native_floor(Value* args, int argc) {
    if (argc < 1 || args[0].type != VAL_NUMBER) return make_number(0);
    return make_number(floor(args[0].as.number));
}

static Value native_ceil(Value* args, int argc) {
    if (argc < 1 || args[0].type != VAL_NUMBER) return make_number(0);
    return make_number(ceil(args[0].as.number));
}

static Value native_type(Value* args, int argc) {
    if (argc < 1) return make_string("null");
    switch (args[0].type) {
        case VAL_NUMBER: return make_string("number");
        case VAL_STRING: return make_string("string");
        case VAL_BOOL: return make_string("boolean");
        default: return make_string("null");
    }
}

/* ===== WINDOW FUNCTIONS ===== */
#ifdef _WIN32
static Value native_window(Value* args, int argc) {
    if (argc >= 1 && args[0].type == VAL_STRING) {
        strncpy(g_win_title, args[0].as.string, 255);
        g_win_title[255] = 0;
    }
    if (argc >= 2 && args[1].type == VAL_NUMBER) g_win_width = (int)args[1].as.number;
    if (argc >= 3 && args[2].type == VAL_NUMBER) g_win_height = (int)args[2].as.number;
    ensure_window();
    return make_null();
}

static Value native_bg(Value* args, int argc) {
    if (argc < 1) return make_null();
    int r = 30, g = 30, b = 30;
    if (args[0].type == VAL_STRING) {
        const char* s = args[0].as.string;
        if (s[0] == '#' && strlen(s) >= 7) {
            r = (int)strtol(s+1, NULL, 16) >> 16 & 0xFF;
            g = (int)strtol(s+1, NULL, 16) >> 8 & 0xFF;
            b = (int)strtol(s+1, NULL, 16) & 0xFF;
        }
    } else if (argc >= 3) {
        r = (int)args[0].as.number;
        g = (int)args[1].as.number;
        b = (int)args[2].as.number;
    }
    g_bg_color = RGB(r, g, b);
    if (g_brush) { HBRUSH old = g_brush; g_brush = CreateSolidBrush(g_bg_color); DeleteObject(old); }
    if (g_hwnd) { InvalidateRect(g_hwnd, NULL, 1); }
    return make_null();
}

static Value native_color(Value* args, int argc) {
    if (argc < 1) return make_null();
    int r = 255, g = 255, b = 255;
    if (args[0].type == VAL_STRING) {
        const char* s = args[0].as.string;
        if (s[0] == '#' && strlen(s) >= 7) {
            r = (int)strtol(s+1, NULL, 16) >> 16 & 0xFF;
            g = (int)strtol(s+1, NULL, 16) >> 8 & 0xFF;
            b = (int)strtol(s+1, NULL, 16) & 0xFF;
        }
    } else if (argc >= 3) {
        r = (int)args[0].as.number;
        g = (int)args[1].as.number;
        b = (int)args[2].as.number;
    }
    g_pen_color = RGB(r, g, b);
    if (g_pen) { HPEN old = g_pen; g_pen = CreatePen(PS_SOLID, g_pen_size, g_pen_color); SelectObject(g_hdc, g_pen); DeleteObject(old); }
    return make_null();
}

static Value native_thickness(Value* args, int argc) {
    if (argc >= 1 && args[0].type == VAL_NUMBER) {
        g_pen_size = (int)args[0].as.number;
        if (g_pen) { HPEN old = g_pen; g_pen = CreatePen(PS_SOLID, g_pen_size, g_pen_color); SelectObject(g_hdc, g_pen); DeleteObject(old); }
    }
    return make_null();
}

static Value native_draw_text(Value* args, int argc) {
    if (argc < 3) return make_null();
    ensure_window();
    pump_messages();
    const char* text = (args[0].type == VAL_STRING) ? args[0].as.string : "";
    int x = (int)args[1].as.number;
    int y = (int)args[2].as.number;
    SetBkMode(g_hdc, TRANSPARENT);
    SetTextColor(g_hdc, g_pen_color);
    TextOut(g_hdc, x, y, text, (int)strlen(text));
    return make_null();
}

static Value native_draw_rect(Value* args, int argc) {
    if (argc < 5) return make_null();
    ensure_window();
    pump_messages();
    int x = (int)args[0].as.number, y = (int)args[1].as.number;
    int w = (int)args[2].as.number, h = (int)args[3].as.number;
    HBRUSH old_br = (HBRUSH)SelectObject(g_hdc, GetStockObject(NULL_BRUSH));
    Rectangle(g_hdc, x, y, x + w, y + h);
    SelectObject(g_hdc, old_br);
    return make_null();
}

static Value native_fill_rect(Value* args, int argc) {
    if (argc < 5) return make_null();
    ensure_window();
    pump_messages();
    int x = (int)args[0].as.number, y = (int)args[1].as.number;
    int w = (int)args[2].as.number, h = (int)args[3].as.number;
    int r = (argc > 4 && args[4].type == VAL_NUMBER) ? (int)args[4].as.number : 255;
    int g = (argc > 5 && args[5].type == VAL_NUMBER) ? (int)args[5].as.number : 255;
    int b = (argc > 6 && args[6].type == VAL_NUMBER) ? (int)args[6].as.number : 255;
    HBRUSH fill = CreateSolidBrush(RGB(r, g, b));
    RECT rc = {x, y, x + w, y + h};
    FillRect(g_hdc, &rc, fill);
    DeleteObject(fill);
    return make_null();
}

static Value native_draw_circle(Value* args, int argc) {
    if (argc < 4) return make_null();
    ensure_window();
    pump_messages();
    int cx = (int)args[0].as.number, cy = (int)args[1].as.number;
    int rx = (int)args[2].as.number, ry = (int)args[3].as.number;
    HBRUSH old_br = (HBRUSH)SelectObject(g_hdc, GetStockObject(NULL_BRUSH));
    Ellipse(g_hdc, cx - rx, cy - ry, cx + rx, cy + ry);
    SelectObject(g_hdc, old_br);
    return make_null();
}

static Value native_fill_circle(Value* args, int argc) {
    if (argc < 4) return make_null();
    ensure_window();
    pump_messages();
    int cx = (int)args[0].as.number, cy = (int)args[1].as.number;
    int rx = (int)args[2].as.number, ry = (int)args[3].as.number;
    HBRUSH fill = CreateSolidBrush(g_pen_color);
    HBRUSH old_br = (HBRUSH)SelectObject(g_hdc, fill);
    Ellipse(g_hdc, cx - rx, cy - ry, cx + rx, cy + ry);
    SelectObject(g_hdc, old_br);
    DeleteObject(fill);
    return make_null();
}

static Value native_draw_line(Value* args, int argc) {
    if (argc < 4) return make_null();
    ensure_window();
    pump_messages();
    MoveToEx(g_hdc, (int)args[0].as.number, (int)args[1].as.number, NULL);
    LineTo(g_hdc, (int)args[2].as.number, (int)args[3].as.number);
    return make_null();
}

static Value native_refresh(Value* args, int argc) {
    (void)args; (void)argc;
    if (g_hwnd) {
        pump_messages();
        ReleaseDC(g_hwnd, g_hdc);
        InvalidateRect(g_hwnd, NULL, 1);
        UpdateWindow(g_hwnd);
        g_hdc = GetDC(g_hwnd);
        SelectObject(g_hdc, g_brush);
        SelectObject(g_hdc, g_pen);
    }
    return make_null();
}

static Value native_key(Value* args, int argc) {
    if (argc < 1) return make_bool(0);
    pump_messages();
    int vk = 0;
    if (args[0].type == VAL_STRING) {
        const char* s = args[0].as.string;
        if (strlen(s) == 1) {
            char c = s[0];
            if (c >= 'a' && c <= 'z') vk = 'A' + (c - 'a');
            else if (c >= '0' && c <= '9') vk = '0' + (c - '0');
            else if (c == ' ') vk = VK_SPACE;
            else if (c == '\n') vk = VK_RETURN;
        } else if (strcmp(s, "space") == 0) vk = VK_SPACE;
        else if (strcmp(s, "enter") == 0) vk = VK_RETURN;
        else if (strcmp(s, "left") == 0) vk = VK_LEFT;
        else if (strcmp(s, "right") == 0) vk = VK_RIGHT;
        else if (strcmp(s, "up") == 0) vk = VK_UP;
        else if (strcmp(s, "down") == 0) vk = VK_DOWN;
        else if (strcmp(s, "escape") == 0) vk = VK_ESCAPE;
    } else if (args[0].type == VAL_NUMBER) {
        vk = (int)args[0].as.number;
    }
    return make_bool(vk > 0 && vk < 256 && g_keys[vk]);
}

static Value native_mouse_x(Value* args, int argc) { (void)args; (void)argc; pump_messages(); return make_number(g_mouse_x); }
static Value native_mouse_y(Value* args, int argc) { (void)args; (void)argc; pump_messages(); return make_number(g_mouse_y); }
static Value native_mouse_down(Value* args, int argc) { (void)args; (void)argc; pump_messages(); return make_bool(g_mouse_down); }

static Value native_sleep_ms(Value* args, int argc) {
    if (argc >= 1 && args[0].type == VAL_NUMBER) Sleep((DWORD)args[0].as.number);
    return make_null();
}

static Value native_width(Value* args, int argc) { (void)args; (void)argc; return make_number(g_win_width); }
static Value native_height(Value* args, int argc) { (void)args; (void)argc; return make_number(g_win_height); }
#else
static Value native_window(Value* a, int n) { return make_null(); }
static Value native_bg(Value* a, int n) { return make_null(); }
static Value native_color(Value* a, int n) { return make_null(); }
static Value native_thickness(Value* a, int n) { return make_null(); }
static Value native_draw_text(Value* a, int n) { return make_null(); }
static Value native_draw_rect(Value* a, int n) { return make_null(); }
static Value native_fill_rect(Value* a, int n) { return make_null(); }
static Value native_draw_circle(Value* a, int n) { return make_null(); }
static Value native_fill_circle(Value* a, int n) { return make_null(); }
static Value native_draw_line(Value* a, int n) { return make_null(); }
static Value native_refresh(Value* a, int n) { return make_null(); }
static Value native_key(Value* a, int n) { return make_bool(0); }
static Value native_mouse_x(Value* a, int n) { return make_number(0); }
static Value native_mouse_y(Value* a, int n) { return make_number(0); }
static Value native_mouse_down(Value* a, int n) { return make_bool(0); }
static Value native_sleep_ms(Value* a, int n) { return make_null(); }
static Value native_width(Value* a, int n) { return make_number(0); }
static Value native_height(Value* a, int n) { return make_number(0); }
#endif

typedef Value (*NativeFn)(Value* args, int argc);
typedef struct { const char* name; NativeFn func; int arity; } NativeFunc;

static NativeFunc native_funcs[] = {
    {"say", native_say, -1}, {"print", native_say, -1},
    {"input", native_input, -1},
    {"length", native_length, 1}, {"len", native_length, 1},
    {"to_number", native_to_number, 1}, {"to_string", native_to_string, 1},
    {"random", native_random, -1}, {"abs", native_abs, 1},
    {"floor", native_floor, 1}, {"ceil", native_ceil, 1},
    {"type", native_type, 1},
    {"window", native_window, -1}, {"bg", native_bg, -1},
    {"color", native_color, -1}, {"thickness", native_thickness, 1},
    {"text", native_draw_text, 3}, {"draw_text", native_draw_text, 3},
    {"rect", native_draw_rect, 5}, {"draw_rect", native_draw_rect, 5},
    {"fill_rect", native_fill_rect, -1}, {"fillrect", native_fill_rect, -1},
    {"circle", native_draw_circle, 4}, {"draw_circle", native_draw_circle, 4},
    {"fill_circle", native_fill_circle, 4}, {"fillcircle", native_fill_circle, 4},
    {"line", native_draw_line, 4}, {"draw_line", native_draw_line, 4},
    {"refresh", native_refresh, 0}, {"update", native_refresh, 0},
    {"key", native_key, 1}, {"is_key", native_key, 1},
    {"mouse_x", native_mouse_x, 0}, {"mouse_y", native_mouse_y, 0},
    {"mouse_down", native_mouse_down, 0},
    {"sleep", native_sleep_ms, 1}, {"sleep_ms", native_sleep_ms, 1},
    {"width", native_width, 0}, {"height", native_height, 0}
};

#define NATIVE_COUNT (sizeof(native_funcs) / sizeof(native_funcs[0]))

/* ===== LEXER ===== */
static void init_lexer(const char* source) {
    lexer.token_count = 0; lexer.pos = 0; lexer.returning = 0; lexer.skipping = 0;
    const char* p = source;

    while (*p && lexer.token_count < MAX_INPUT) {
        while (*p == ' ' || *p == '\t' || *p == '\r') p++;
        if (*p == '\n') { p++; continue; }
        if (*p == '#') { while (*p && *p != '\n') p++; continue; }
        if (*p == '\0') break;

        /* Multi-line strings with triple quotes */
        if (*p == '"' && *(p+1) == '"' && *(p+2) == '"') {
            p += 3;
            char buffer[MAX_INPUT] = {0};
            int len = 0;
            while (*p && !(*p == '"' && *(p+1) == '"' && *(p+2) == '"') && len < MAX_INPUT - 1) {
                buffer[len++] = *p++;
            }
            if (*p) p += 3;
            char token[MAX_INPUT + 4];
            snprintf(token, sizeof(token), "\"%s\"", buffer);
            lexer.tokens[lexer.token_count++] = strdup_safe(token);
        }
        /* Regular strings */
        else if (*p == '"' || *p == '\'') {
            char quote = *p++;
            char buffer[MAX_INPUT] = {0};
            int len = 0;
            while (*p && *p != quote && len < MAX_INPUT - 1) {
                if (*p == '\\') { p++; switch(*p) {
                    case 'n': buffer[len++] = '\n'; break;
                    case 't': buffer[len++] = '\t'; break;
                    default: buffer[len++] = *p; break;
                }} else { buffer[len++] = *p; }
                p++;
            }
            if (*p == quote) p++;
            char token[MAX_INPUT + 3];
            snprintf(token, sizeof(token), "\"%s\"", buffer);
            lexer.tokens[lexer.token_count++] = strdup_safe(token);
        }
        /* Multi-word operators (check longest first) */
        else if (strncmp(p, "is greater than or equal to", 27) == 0) { lexer.tokens[lexer.token_count++] = strdup_safe(">="); p += 27; }
        else if (strncmp(p, "is less than or equal to", 24) == 0) { lexer.tokens[lexer.token_count++] = strdup_safe("<="); p += 24; }
        else if (strncmp(p, "is not equal to", 15) == 0) { lexer.tokens[lexer.token_count++] = strdup_safe("!="); p += 15; }
        else if (strncmp(p, "is greater than", 15) == 0) { lexer.tokens[lexer.token_count++] = strdup_safe(">"); p += 15; }
        else if (strncmp(p, "is less than", 12) == 0) { lexer.tokens[lexer.token_count++] = strdup_safe("<"); p += 12; }
        else if (strncmp(p, "is at least", 11) == 0) { lexer.tokens[lexer.token_count++] = strdup_safe(">="); p += 11; }
        else if (strncmp(p, "is at most", 10) == 0) { lexer.tokens[lexer.token_count++] = strdup_safe("<="); p += 10; }
        else if (strncmp(p, "is equal to", 11) == 0) { lexer.tokens[lexer.token_count++] = strdup_safe("=="); p += 11; }
        else if (strncmp(p, "equal to", 8) == 0) { lexer.tokens[lexer.token_count++] = strdup_safe("=="); p += 8; }
        else if (strncmp(p, "equals", 6) == 0 && !isalnum(*(p+6)) && *(p+6) != '_') { lexer.tokens[lexer.token_count++] = strdup_safe("=="); p += 6; }
        else if (strncmp(p, "is not", 6) == 0) { lexer.tokens[lexer.token_count++] = strdup_safe("!="); p += 6; }
        else if (strncmp(p, "multiplied by", 13) == 0) { lexer.tokens[lexer.token_count++] = strdup_safe("*"); p += 13; }
        else if (strncmp(p, "divided by", 10) == 0) { lexer.tokens[lexer.token_count++] = strdup_safe("/"); p += 10; }
        else if (strncmp(p, "plus", 4) == 0 && !isalnum(*(p+4)) && *(p+4) != '_') { lexer.tokens[lexer.token_count++] = strdup_safe("+"); p += 4; }
        else if (strncmp(p, "minus", 5) == 0 && !isalnum(*(p+5)) && *(p+5) != '_') { lexer.tokens[lexer.token_count++] = strdup_safe("-"); p += 5; }
        else if (strncmp(p, "is", 2) == 0 && !isalnum(*(p+2)) && *(p+2) != '_') { lexer.tokens[lexer.token_count++] = strdup_safe("=="); p += 2; }
        /* Compound assignment operators */
        else if (strncmp(p, "+=", 2) == 0) { lexer.tokens[lexer.token_count++] = strdup_safe("+="); p += 2; }
        else if (strncmp(p, "-=", 2) == 0) { lexer.tokens[lexer.token_count++] = strdup_safe("-="); p += 2; }
        else if (strncmp(p, "*=", 2) == 0) { lexer.tokens[lexer.token_count++] = strdup_safe("*="); p += 2; }
        else if (strncmp(p, "/=", 2) == 0) { lexer.tokens[lexer.token_count++] = strdup_safe("/="); p += 2; }
        else if (strncmp(p, "!=", 2) == 0) { lexer.tokens[lexer.token_count++] = strdup_safe("!="); p += 2; }
        else if (strncmp(p, "==", 2) == 0) { lexer.tokens[lexer.token_count++] = strdup_safe("=="); p += 2; }
        else if (strncmp(p, ">=", 2) == 0) { lexer.tokens[lexer.token_count++] = strdup_safe(">="); p += 2; }
        else if (strncmp(p, "<=", 2) == 0) { lexer.tokens[lexer.token_count++] = strdup_safe("<="); p += 2; }
        else if (strncmp(p, "and", 3) == 0 && !isalnum(*(p+3)) && *(p+3) != '_') { lexer.tokens[lexer.token_count++] = strdup_safe("and"); p += 3; }
        else if (strncmp(p, "or", 2) == 0 && !isalnum(*(p+2)) && *(p+2) != '_') { lexer.tokens[lexer.token_count++] = strdup_safe("or"); p += 2; }
        else if (strncmp(p, "not", 3) == 0 && !isalnum(*(p+3)) && *(p+3) != '_') { lexer.tokens[lexer.token_count++] = strdup_safe("not"); p += 3; }
        /* Single character operators */
        else if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '%' ||
                 *p == '(' || *p == ')' || *p == ',' || *p == '[' || *p == ']' ||
                 *p == '=' || *p == '<' || *p == '>' || *p == '!') {
            char buf[3] = {0};
            buf[0] = *p;
            if ((*p == '=' || *p == '!' || *p == '<' || *p == '>') && *(p+1) == '=') {
                buf[1] = '='; p += 2;
            } else { p++; }
            lexer.tokens[lexer.token_count++] = strdup_safe(buf);
        }
        /* Numbers */
        else if (isdigit(*p) || (*p == '-' && isdigit(*(p+1)))) {
            char buffer[64] = {0};
            int len = 0;
            if (*p == '-') buffer[len++] = *p++;
            while (isdigit(*p) || *p == '.') buffer[len++] = *p++;
            lexer.tokens[lexer.token_count++] = strdup_safe(buffer);
        }
        /* Identifiers and keywords */
        else if (isalpha(*p) || *p == '_') {
            char buffer[256] = {0};
            int len = 0;
            while (isalnum(*p) || *p == '_') buffer[len++] = *p++;
            lexer.tokens[lexer.token_count++] = strdup_safe(buffer);
        }
        else { p++; } /* skip unknown chars */
    }
}

static const char* peek() { return (lexer.pos < lexer.token_count) ? lexer.tokens[lexer.pos] : NULL; }
static const char* advance() { return (lexer.pos < lexer.token_count) ? lexer.tokens[lexer.pos++] : NULL; }

static int match(const char* expected) {
    const char* t = peek();
    if (t && strcmp(t, expected) == 0) { lexer.pos++; return 1; }
    return 0;
}

static int is_number(const char* s) {
    if (!s) return 0;
    if (*s == '-' || *s == '.') s++;
    if (!*s) return 0;
    int dot = 0;
    while (*s) { if (*s == '.') { if (dot) return 0; dot = 1; } else if (!isdigit(*s)) return 0; s++; }
    return 1;
}

static int is_identifier(const char* s) {
    if (!s || (!isalpha(*s) && *s != '_')) return 0;
    s++;
    while (*s) { if (!isalnum(*s) && *s != '_') return 0; s++; }
    return 1;
}

static void error(const char* msg) { error_with_context(msg); }

static int find_end() {
    int depth = 1;
    while (peek()) {
        if (strcmp(peek(), "if") == 0 || strcmp(peek(), "while") == 0 ||
            strcmp(peek(), "for") == 0 || strcmp(peek(), "func") == 0 ||
            strcmp(peek(), "function") == 0 || strcmp(peek(), "repeat") == 0) depth++;
        else if (strcmp(peek(), "end") == 0) { depth--; if (depth == 0) return lexer.pos; }
        advance();
    }
    error("Missing 'end'");
    return -1;
}

/* ===== EXPRESSION PARSER ===== */
static Value parse_primary() {
    const char* t = peek();
    if (!t) error("Unexpected end of input");

    if (strcmp(t, "(") == 0) { advance(); Value v = parse_expression(); if (!match(")")) error("Expected ')'"); return v; }
    if (t[0] == '"' && t[strlen(t)-1] == '"') { advance(); char b[MAX_INPUT] = {0}; strncpy(b, t+1, strlen(t)-2); return make_string(b); }
    if (t[0] == '\'' && t[strlen(t)-1] == '\'') { advance(); char b[MAX_INPUT] = {0}; strncpy(b, t+1, strlen(t)-2); return make_string(b); }
    if (is_number(t)) { advance(); return make_number(atof(t)); }
    if (strcmp(t, "true") == 0) { advance(); return make_bool(1); }
    if (strcmp(t, "false") == 0) { advance(); return make_bool(0); }
    if (strcmp(t, "null") == 0) { advance(); return make_null(); }

    if (is_identifier(t)) {
        advance();
        /* Function call */
        if (match("(")) {
            Func* func = get_func(t);
            if (func) {
                Value args[MAX_ARGS]; int argc = 0;
                if (!match(")")) { do { if (argc >= MAX_ARGS) error("Too many arguments"); args[argc++] = parse_expression(); } while (match(",")); if (!match(")")) error("Expected ')'"); }
                return call_function(func, args, argc);
            }
            for (size_t i = 0; i < NATIVE_COUNT; i++) {
                if (strcmp(native_funcs[i].name, t) == 0) {
                    Value args[MAX_ARGS]; int argc = 0;
                    if (!match(")")) { do { if (argc >= MAX_ARGS) error("Too many arguments"); args[argc++] = parse_expression(); } while (match(",")); if (!match(")")) error("Expected ')'"); }
                    return native_funcs[i].func(args, argc);
                }
            }
            error("Unknown function");
        }
        return get_var(t);
    }
    error("Unexpected token");
    return make_null();
}

static Value parse_unary() {
    if (match("!")) { Value v = parse_unary(); return make_bool(!value_to_bool(v)); }
    if (match("-")) { Value v = parse_unary(); return make_number(-value_to_number(v)); }
    return parse_primary();
}

static Value parse_multiplication() {
    Value left = parse_unary();
    while (peek() && (strcmp(peek(), "*") == 0 || strcmp(peek(), "/") == 0 || strcmp(peek(), "%") == 0)) {
        const char* op = advance(); Value right = parse_unary();
        double ln = value_to_number(left), rn = value_to_number(right);
        if (strcmp(op, "*") == 0) left = make_number(ln * rn);
        else if (strcmp(op, "/") == 0) { if (rn == 0) error("Division by zero"); left = make_number(ln / rn); }
        else left = make_number(fmod(ln, rn));
    }
    return left;
}

static Value parse_addition() {
    Value left = parse_multiplication();
    while (peek() && (strcmp(peek(), "+") == 0 || strcmp(peek(), "-") == 0)) {
        const char* op = advance(); Value right = parse_multiplication();
        if (strcmp(op, "+") == 0) {
            if (left.type == VAL_STRING || right.type == VAL_STRING) {
                char b[MAX_INPUT]; snprintf(b, sizeof(b), "%s%s",
                    left.type == VAL_STRING ? left.as.string : "",
                    right.type == VAL_STRING ? right.as.string : "");
                left = make_string(b);
            } else {
                left = make_number(value_to_number(left) + value_to_number(right));
            }
        } else {
            left = make_number(value_to_number(left) - value_to_number(right));
        }
    }
    return left;
}

static Value parse_comparison() {
    Value left = parse_addition();
    while (peek() && (strcmp(peek(), "==") == 0 || strcmp(peek(), "!=") == 0 ||
                       strcmp(peek(), ">") == 0 || strcmp(peek(), "<") == 0 ||
                       strcmp(peek(), ">=") == 0 || strcmp(peek(), "<=") == 0)) {
        const char* op = advance(); Value right = parse_addition();
        /* If both are strings, compare lexically */
        if (left.type == VAL_STRING && right.type == VAL_STRING) {
            int cmp = strcmp(left.as.string ? left.as.string : "", right.as.string ? right.as.string : "");
            if (strcmp(op, "==") == 0) left = make_bool(cmp == 0);
            else if (strcmp(op, "!=") == 0) left = make_bool(cmp != 0);
            else if (strcmp(op, ">") == 0) left = make_bool(cmp > 0);
            else if (strcmp(op, "<") == 0) left = make_bool(cmp < 0);
            else if (strcmp(op, ">=") == 0) left = make_bool(cmp >= 0);
            else if (strcmp(op, "<=") == 0) left = make_bool(cmp <= 0);
        } else {
            double ln = value_to_number(left), rn = value_to_number(right);
            if (strcmp(op, "==") == 0) left = make_bool(ln == rn);
            else if (strcmp(op, "!=") == 0) left = make_bool(ln != rn);
            else if (strcmp(op, ">") == 0) left = make_bool(ln > rn);
            else if (strcmp(op, "<") == 0) left = make_bool(ln < rn);
            else if (strcmp(op, ">=") == 0) left = make_bool(ln >= rn);
            else if (strcmp(op, "<=") == 0) left = make_bool(ln <= rn);
        }
    }
    return left;
}

static Value parse_expression() {
    Value left = parse_comparison();
    while (peek() && (strcmp(peek(), "and") == 0 || strcmp(peek(), "or") == 0)) {
        const char* op = advance(); Value right = parse_comparison();
        if (strcmp(op, "and") == 0) left = make_bool(left.as.boolean && right.as.boolean);
        else left = make_bool(left.as.boolean || right.as.boolean);
    }
    return left;
}

/* ===== STATEMENT EXECUTION ===== */
static void execute_statement() {
    const char* t = peek();
    if (!t) return;

    /* ---- IDENTIFIER-ASSIGNMENT: "x == expr" used as assignment at statement start (accept 'x equals 5') ---- */
    if (is_identifier(t)) {
        const char* name = advance();
        if (peek() && strcmp(peek(), "==") == 0) {
            /* treat 'x == expr' at statement start as assignment for natural english 'x equals 5' */
            advance();
            set_var(name, parse_expression());
            return;
        }
        /* revert so later handlers can process identifier-starting statements */
        lexer.pos--; 
    }

    /* ---- OUTPUT: say / display / print ---- */
    if (strcmp(t, "say") == 0 || strcmp(t, "display") == 0 || strcmp(t, "print") == 0) {
        advance();
        if (peek() && strcmp(peek(), "end") != 0 && strcmp(peek(), "say") != 0 &&
            strcmp(peek(), "display") != 0 && strcmp(peek(), "print") != 0 &&
            strcmp(peek(), "let") != 0 && strcmp(peek(), "set") != 0 &&
            strcmp(peek(), "if") != 0 && strcmp(peek(), "while") != 0 &&
            strcmp(peek(), "for") != 0 && strcmp(peek(), "func") != 0 &&
            strcmp(peek(), "function") != 0 && strcmp(peek(), "return") != 0 &&
            strcmp(peek(), "repeat") != 0 && strcmp(peek(), "increase") != 0 &&
            strcmp(peek(), "reduce") != 0 && strcmp(peek(), "make") != 0 &&
            strcmp(peek(), "store") != 0) {
            Value v = parse_expression();
            native_say(&v, 1);
        }
        return;
    }

    /* ---- VARIABLE: let x = expr ---- */
    if (strcmp(t, "let") == 0) {
        advance();
        const char* name = advance();
        if (match("=")) { set_var(name, parse_expression()); return; }
        /* Also support: let x (without assignment, defaults to null) */
        set_var(name, make_null());
        return;
    }

    /* ---- VARIABLE (verbose): set x to expr ---- */
    if (strcmp(t, "set") == 0) {
        advance();
        const char* name = advance();
        if (!match("to")) error("Expected 'to'");
        set_var(name, parse_expression());
        return;
    }

    /* ---- VARIABLE (verbose): make x be expr ---- */
    if (strcmp(t, "make") == 0) {
        advance();
        const char* name = advance();
        if (!match("be")) error("Expected 'be'");
        set_var(name, parse_expression());
        return;
    }

    /* ---- VARIABLE (verbose): store x as expr ---- */
    if (strcmp(t, "store") == 0) {
        advance();
        const char* name = advance();
        if (!match("as")) error("Expected 'as'");
        set_var(name, parse_expression());
        return;
    }

    /* ---- COMPOUND ASSIGNMENT: let x += expr ---- */
    if (strcmp(t, "+=") == 0 || strcmp(t, "-=") == 0 || strcmp(t, "*=") == 0 || strcmp(t, "/=") == 0) {
        /* This shouldn't be at statement start, but handle gracefully */
        advance();
        return;
    }

    /* ---- INCREMENT: increase x by N ---- */
    if (strcmp(t, "increase") == 0) {
        advance();
        const char* name = advance();
        if (!match("by")) error("Expected 'by'");
        Value amt = parse_expression();
        Value cur = get_var(name);
        set_var(name, make_number(cur.as.number + amt.as.number));
        return;
    }

    /* ---- ADD TO: add N to x (natural form) ---- */
    if (strcmp(t, "add") == 0) {
        advance();
        Value amt = parse_expression();
        if (!match("to")) error("Expected 'to'");
        const char* name = advance();
        Value cur = get_var(name);
        set_var(name, make_number(cur.as.number + amt.as.number));
        return;
    }

    /* ---- SUBTRACT FROM: subtract N from x ---- */
    if (strcmp(t, "subtract") == 0) {
        advance();
        Value amt = parse_expression();
        if (!match("from")) error("Expected 'from'");
        const char* name = advance();
        Value cur = get_var(name);
        set_var(name, make_number(cur.as.number - amt.as.number));
        return;
    }

    /* ---- DECREMENT: reduce x by N ---- */
    if (strcmp(t, "reduce") == 0) {
        advance();
        const char* name = advance();
        if (!match("by")) error("Expected 'by'");
        Value amt = parse_expression();
        Value cur = get_var(name);
        set_var(name, make_number(cur.as.number - amt.as.number));
        return;
    }

    /* ---- IF ---- */
    if (strcmp(t, "if") == 0) {
        advance();
        Value cond = parse_expression();

        /* Handle: if condition then single_statement */
        if (match("then")) {
            /* Check if next token is end (empty then block) */
            if (peek() && strcmp(peek(), "end") != 0) {
                if (cond.as.boolean) {
                    execute_statement();
                } else {
                    /* Skip the single statement without executing */
                    lexer.skipping = 1;
                    execute_statement();
                    lexer.skipping = 0;
                }
            }
            return;
        }

        /* Handle: if condition \n statements... end */
        int body_start = lexer.pos;
        int end = find_end();
        if (cond.as.boolean) {
            lexer.pos = body_start;
            while (lexer.pos < end && !lexer.returning) execute_statement();
            lexer.pos = end + 1;
        } else { lexer.pos = end + 1; }
        return;
    }

    /* ---- WHILE ---- */
    if (strcmp(t, "while") == 0) {
        advance();
        int cond_start = lexer.pos;
        Value cond = parse_expression();
        int body_start = lexer.pos;
        int end = find_end();
        int iters = 0;
        while (cond.as.boolean && iters < 10000 && !lexer.returning) {
            iters++;
            lexer.pos = body_start;
            while (lexer.pos < end && !lexer.returning) execute_statement();
            lexer.pos = cond_start;
            cond = parse_expression();
        }
        lexer.pos = end + 1;
        return;
    }

    /* ---- REPEAT N TIMES ---- */
    if (strcmp(t, "repeat") == 0) {
        advance();
        Value count = parse_expression();
        if (!match("times")) error("Expected 'times'");
        int body_start = lexer.pos;
        int end = find_end();
        int n = (int)count.as.number;
        for (int i = 0; i < n && !lexer.returning; i++) {
            lexer.pos = body_start;
            while (lexer.pos < end && !lexer.returning) execute_statement();
        }
        lexer.pos = end + 1;
        return;
    }

    /* ---- FOR EACH ---- */
    if (strcmp(t, "for") == 0) {
        advance();
        if (match("each")) {
            const char* item = advance();
            if (!match("in")) error("Expected 'in'");
            Value list = parse_expression();
            if (!match("do")) error("Expected 'do'");
            int body_start = lexer.pos;
            int end = find_end();
            if (list.type == VAL_STRING) {
                for (int i = 0; list.as.string[i] && !lexer.returning; i++) {
                    char c[2] = {list.as.string[i], 0};
                    set_var(item, make_string(c));
                    lexer.pos = body_start;
                    while (lexer.pos < end && !lexer.returning) execute_statement();
                }
            }
            lexer.pos = end + 1;
        }
        return;
    }

    /* ---- FUNCTION: func name(params)...end ---- */
    if (strcmp(t, "func") == 0 || strcmp(t, "function") == 0) {
        advance();
        Func func = {0};
        func.name = strdup_safe(advance());

        if (match("(")) {
            if (!match(")")) {
                do {
                    if (func.param_count >= MAX_ARGS) error("Too many parameters");
                    func.params[func.param_count++] = strdup_safe(advance());
                } while (match(","));
                if (!match(")")) error("Expected ')'");
            }
        } else if (match("taking")) {
            while (peek() && strcmp(peek(), "end") != 0 && strcmp(peek(), "let") != 0 &&
                   strcmp(peek(), "set") != 0 && strcmp(peek(), "if") != 0 &&
                   strcmp(peek(), "while") != 0 && strcmp(peek(), "for") != 0 &&
                   strcmp(peek(), "func") != 0 && strcmp(peek(), "function") != 0 &&
                   strcmp(peek(), "return") != 0 && strcmp(peek(), "repeat") != 0 &&
                   strcmp(peek(), "increase") != 0 && strcmp(peek(), "reduce") != 0 &&
                   strcmp(peek(), "say") != 0 && strcmp(peek(), "display") != 0 &&
                   strcmp(peek(), "print") != 0) {
                if (func.param_count >= MAX_ARGS) error("Too many parameters");
                func.params[func.param_count++] = strdup_safe(advance());
                match(",");
            }
        }

        int body_start = lexer.pos;
        int end = find_end();
        func.body = malloc(sizeof(char*) * MAX_INPUT);
        func.body_lines = 0;
        lexer.pos = body_start;
        while (lexer.pos < end && func.body_lines < MAX_INPUT) {
            func.body[func.body_lines++] = strdup_safe(peek());
            advance();
        }
        lexer.pos = end + 1;

        if (vm.func_count >= MAX_FUNCS) error("Too many functions");
        vm.funcs[vm.func_count++] = func;
        return;
    }

    /* ---- RETURN ---- */
    if (strcmp(t, "return") == 0) {
        advance();
        if (peek() && strcmp(peek(), "end") != 0 && strcmp(peek(), "say") != 0 &&
            strcmp(peek(), "display") != 0 && strcmp(peek(), "print") != 0 &&
            strcmp(peek(), "let") != 0 && strcmp(peek(), "set") != 0 &&
            strcmp(peek(), "if") != 0 && strcmp(peek(), "while") != 0 &&
            strcmp(peek(), "for") != 0 && strcmp(peek(), "func") != 0 &&
            strcmp(peek(), "function") != 0 && strcmp(peek(), "repeat") != 0) {
            push(parse_expression());
        } else {
            push(make_null());
        }
        lexer.returning = 1;
        return;
    }

    /* ---- PROGRAM (just skip the name) ---- */
    if (strcmp(t, "program") == 0) {
        advance();
        while (peek() && is_identifier(peek()) && strcmp(peek(), "end") != 0 &&
               strcmp(peek(), "let") != 0 && strcmp(peek(), "set") != 0 &&
               strcmp(peek(), "if") != 0 && strcmp(peek(), "while") != 0 &&
               strcmp(peek(), "for") != 0 && strcmp(peek(), "func") != 0 &&
               strcmp(peek(), "function") != 0 && strcmp(peek(), "return") != 0 &&
               strcmp(peek(), "repeat") != 0 && strcmp(peek(), "say") != 0 &&
               strcmp(peek(), "display") != 0 && strcmp(peek(), "print") != 0 &&
               strcmp(peek(), "increase") != 0 && strcmp(peek(), "reduce") != 0 &&
               strcmp(peek(), "make") != 0 && strcmp(peek(), "store") != 0) {
            advance();
        }
        return;
    }

    /* ---- END (shouldn't reach here normally) ---- */
    if (strcmp(t, "end") == 0) { advance(); return; }

    /* ---- EXPRESSION STATEMENT (function call) ---- */
    if (is_identifier(t)) {
        const char* name = advance();
        if (match("(")) {
            Func* func = get_func(name);
            if (func) {
                Value args[MAX_ARGS]; int argc = 0;
                if (!match(")")) { do { if (argc >= MAX_ARGS) error("Too many arguments"); args[argc++] = parse_expression(); } while (match(",")); if (!match(")")) error("Expected ')'"); }
                call_function(func, args, argc);
                return;
            }
            for (size_t i = 0; i < NATIVE_COUNT; i++) {
                if (strcmp(native_funcs[i].name, name) == 0) {
                    Value args[MAX_ARGS]; int argc = 0;
                    if (!match(")")) { do { if (argc >= MAX_ARGS) error("Too many arguments"); args[argc++] = parse_expression(); } while (match(",")); if (!match(")")) error("Expected ')'"); }
                    native_funcs[i].func(args, argc);
                    return;
                }
            }
        }
    }

    advance(); /* skip unknown */
}

/* ===== FUNCTION CALL ===== */
static Value call_function(Func* func, Value* args, int argc) {
    int saved_var_count = vm.var_count;
    int saved_stack_top = vm.stack_top;

    Value saved_values[MAX_ARGS];
    for (int i = 0; i < func->param_count && i < argc; i++) {
        int idx = find_var(func->params[i]);
        if (idx >= 0) { saved_values[i] = vm.vars[idx].value; vm.vars[idx].value = args[i]; }
        else { set_var(func->params[i], args[i]); saved_values[i] = make_null(); }
    }

    int saved_pos = lexer.pos, saved_token_count = lexer.token_count;
    char** saved_tokens = malloc(sizeof(char*) * saved_token_count);
    for (int i = 0; i < saved_token_count; i++) saved_tokens[i] = lexer.tokens[i];

    lexer.pos = 0; lexer.token_count = 0; lexer.returning = 0;
    for (int i = 0; i < func->body_lines; i++)
        lexer.tokens[lexer.token_count++] = strdup_safe(func->body[i]);

    while (lexer.pos < lexer.token_count && !lexer.returning) execute_statement();

    Value result = (vm.stack_top > saved_stack_top) ? pop() : make_null();

    for (int i = 0; i < lexer.token_count; i++) free(lexer.tokens[i]);
    for (int i = 0; i < saved_token_count; i++) lexer.tokens[i] = saved_tokens[i];
    free(saved_tokens);

    for (int i = func->param_count - 1; i >= 0; i--) {
        int idx = find_var(func->params[i]);
        if (idx >= 0 && i < argc) { free_value(&vm.vars[idx].value); vm.vars[idx].value = saved_values[i]; }
    }
    for (int i = vm.var_count - 1; i >= saved_var_count; i--) { free(vm.vars[i].name); free_value(&vm.vars[i].value); }
    vm.var_count = saved_var_count;

    lexer.pos = saved_pos; lexer.token_count = saved_token_count; lexer.returning = 0;
    return result;
}

/* ===== FILE I/O ===== */
static char* read_file(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) { fprintf(stderr, "Cannot open: %s\n", filename); exit(1); }
    fseek(f, 0, SEEK_END); long size = ftell(f); fseek(f, 0, SEEK_SET);
    char* buf = malloc(size + 1);
    fread(buf, 1, size, f); buf[size] = '\0'; fclose(f);
    return buf;
}

/* ===== MAIN ===== */
int main(int argc, char* argv[]) {
    /* check for debug flags */
    for (int ai = 1; ai < argc; ai++) {
        if (strcmp(argv[ai], "--debug") == 0 || strcmp(argv[ai], "-g") == 0 || strcmp(argv[ai], "-d") == 0) {
            g_debug_mode = 1;
            /* remove flag by shifting */
            for (int j = ai; j < argc-1; j++) argv[j] = argv[j+1];
            argc--; argv[argc] = NULL;
            break;
        }
    }

    if (argc < 2) {
        /* Interactive REPL mode */
        printf("Flow Programming Language v2 - REPL\nType :quit to exit, :help for commands.\n");
        memset(&vm, 0, sizeof(VM));
        char line[MAX_INPUT];
        while (1) {
            printf("flow> ");
            if (!fgets(line, sizeof(line), stdin)) break;
            /* Trim leading whitespace */
            char* p = line; while (*p == ' ' || *p == '\t') p++;
            if (*p == '\n' || *p == '\0') continue;
            if (p[0] == ':' ) {
                if (strncmp(p, ":quit", 5) == 0) break;
                if (strncmp(p, ":help", 5) == 0) {
                    printf(":quit - exit REPL\n:vars - list variables\n:funcs - list functions\n:help - this message\n");
                    continue;
                }
                if (strncmp(p, ":vars", 5) == 0) {
                    for (int i = 0; i < vm.var_count; i++) printf("%s = %s\n", vm.vars[i].name, vm.vars[i].value.type == VAL_STRING ? vm.vars[i].value.as.string : "<non-string>");
                    continue;
                }
                if (strncmp(p, ":funcs", 6) == 0) {
                    for (int i = 0; i < vm.func_count; i++) printf("%s/%d\n", vm.funcs[i].name, vm.funcs[i].param_count);
                    continue;
                }
            }

            /* Support multi-line blocks entered in REPL (simple depth counting) */
            char source[MAX_INPUT] = {0};
            strncpy(source, p, sizeof(source)-1);
            int depth = 0;
            /* check if this line starts a block */
            {
                const char* kt = "func function if while for repeat";
                for (const char* k = kt; *k; ) {
                    char key[32] = {0}; int n = 0;
                    while (*k && *k != ' ') key[n++] = *k++;
                    if (strstr(p, key) == p) depth = 1;
                    if (*k == ' ') k++; else break;
                }
            }
            while (depth > 0) {
                char more[MAX_INPUT];
                if (!fgets(more, sizeof(more), stdin)) break;
                strncat(source, "\n", sizeof(source)-strlen(source)-1);
                strncat(source, more, sizeof(source)-strlen(source)-1);
                /* naive adjust depth */
                char* q = more;
                while (*q) {
                    if (strncmp(q, "end", 3) == 0 && (q[3] == '\n' || q[3] == '\0' || q[3] == ' ')) depth--; 
                    if (strncmp(q, "if", 2) == 0 || strncmp(q, "while", 5) == 0 || strncmp(q, "for", 3) == 0 || strncmp(q, "func", 4) == 0 || strncmp(q, "function", 8) == 0 || strncmp(q, "repeat", 6) == 0) depth++;
                    q++;
                }
            }

            init_lexer(source);
            while (lexer.pos < lexer.token_count) {
                if (g_debug_mode) {
                    if (g_debug_continue) {
                        if (is_current_breakpoint()) { handle_debug_prompt(); }
                    } else {
                        handle_debug_prompt();
                    }
                }
                execute_statement();
            }
            for (int i = 0; i < lexer.token_count; i++) free(lexer.tokens[i]);
        }
        return 0;
    }
    /* Populate breakpoints from FLOW_BREAKPOINTS env var (comma-separated) */
    char* bstr = getenv("FLOW_BREAKPOINTS");
    if (bstr) {
        char* cp = strdup_safe(bstr);
        char* tok = strtok(cp, ",");
        while (tok && g_bp_count < 64) {
            while (*tok == ' ' || *tok == '\t') tok++;
            char* end = tok + strlen(tok) - 1;
            while (end > tok && (*end == ' ' || *end == '\t')) { *end = '\0'; end--; }
            g_breakpoints[g_bp_count++] = strdup_safe(tok);
            tok = strtok(NULL, ",");
        }
        free(cp);
    }

    /* Allow enabling debug mode via environment variable for shells that mangle args */
    if (!g_debug_mode) {
        char* ed = getenv("FLOW_DEBUG");
        if (ed && (strcmp(ed, "1") == 0 || strcasecmp(ed, "true") == 0)) {
            g_debug_mode = 1;
        }
    }

    memset(&vm, 0, sizeof(VM));
    char* source = read_file(argv[1]);
    init_lexer(source);

    fprintf(stderr, "[diag] entering file-run loop, token_count=%d\n", lexer.token_count);
    while (lexer.pos < lexer.token_count) {
        if (g_debug_mode) {
            if (g_debug_continue) {
                if (is_current_breakpoint()) { handle_debug_prompt(); }
            } else {
                handle_debug_prompt();
            }
        }
        execute_statement();
    }

    free(source);
    for (int i = 0; i < lexer.token_count; i++) free(lexer.tokens[i]);
    return 0;
}
