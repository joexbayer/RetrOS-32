/**
 * @file edit.cpp
 * @author Joe Bayer (joexbayer)
 * @brief A text editor app
 * @version 0.1
 * @date 2024-01-10
 */

#include "edit.hpp"

#include <utils/cppUtils.hpp>
#include <utils/MsgBox.hpp>
#include <utils/Thread.hpp>
#include <utils/Widgets.hpp>
#include <syscall_helper.h>

extern "C" int invoke_syscall(int i, int arg1, int arg2, int arg3);

/* Helper functions */
static int isAlpha(unsigned char c)
{
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_');
}

static int minInt(int a, int b)
{
    return (a < b) ? a : b;
}

static int clampInt(int value, int lo, int hi)
{
    if (value < lo) {
        return lo;
    }
    if (value > hi) {
        return hi;
    }
    return value;
}

static int digitCount(int value)
{
    int v = value;
    int digits = 1;

    if (v < 0) {
        v = -v;
    }

    while (v >= 10) {
        v /= 10;
        digits++;
    }

    return digits;
}

static int open_editor_file(const char* path)
{
    if (path == nullptr || path[0] == 0) {
        return -1;
    }

    int flags_rw = FS_FILE_FLAG_READ | FS_FILE_FLAG_WRITE;
    int flags_create = FS_FILE_FLAG_CREATE | flags_rw;

    int fd = open(path, flags_rw);
    if (fd >= 0) {
        return fd;
    }

    fd = open(path, flags_create);
    if (fd >= 0) {
        return fd;
    }

    if (path[0] == '/' && path[1] != 0) {
        fd = open(path + 1, flags_create);
    }

    return fd;
}

enum {
    EDITOR_NEW_FILE_POPUP_ACTION_NONE = 0,
    EDITOR_NEW_FILE_POPUP_ACTION_CREATE = 1,
    EDITOR_NEW_FILE_POPUP_ACTION_CANCEL = 2
};

enum {
    EDITOR_NEW_FILE_POPUP_RESULT_CANCEL = 0,
    EDITOR_NEW_FILE_POPUP_RESULT_CREATE = 1
};

struct EditorNewFilePopupShared {
    volatile int running;
    volatile int result;
    char path[256];
};

static int trim_copy_path(const char* src, char* dst, int dst_size)
{
    if (src == nullptr || dst == nullptr || dst_size <= 1) {
        return 0;
    }

    int begin = 0;
    while (src[begin] == ' ' || src[begin] == '\t') {
        begin++;
    }

    int end = strlen(src);
    while (end > begin && (src[end - 1] == ' ' || src[end - 1] == '\t')) {
        end--;
    }

    int len = end - begin;
    if (len <= 0) {
        dst[0] = 0;
        return 0;
    }

    if (len >= dst_size) {
        len = dst_size - 1;
    }

    memcpy(dst, &src[begin], len);
    dst[len] = 0;
    return len;
}

static void __editor_new_file_popup_thread(void* arg)
{
    EditorNewFilePopupShared* shared = (EditorNewFilePopupShared*) arg;
    printf("[editor] popup thread start arg=%x\n", (uint32_t)shared);
    if (shared == nullptr) {
        printf("[editor] popup thread: null shared\n");
        return;
    }

    shared->result = EDITOR_NEW_FILE_POPUP_RESULT_CANCEL;
    shared->path[0] = 0;

    Window popup(268, 112, "New File", 1);

    volatile int action = EDITOR_NEW_FILE_POPUP_ACTION_NONE;
    WidgetManager* widgets = nullptr;
    Input* path_input = nullptr;

    char status[96];
    memset(status, 0, sizeof(status));

    char input_placeholder[] = "/path/to/file.c";
    char input_tag[] = "new_path";
    char create_label[] = "Create";
    char cancel_label[] = "Cancel";

    auto freeWidgets = [&]() {
        if (widgets != nullptr) {
            delete widgets;
            widgets = nullptr;
            path_input = nullptr;
        }
    };

    auto buildWidgets = [&]() -> int {
        freeWidgets();
        action = EDITOR_NEW_FILE_POPUP_ACTION_NONE;

        widgets = new WidgetManager();
        if (widgets == nullptr) {
            return -1;
        }

        Layout* input_row = new Layout(10, 32, 248, 18, HORIZONTAL, LAYOUT_FLAG_NONE);
        if (input_row == nullptr) {
            freeWidgets();
            return -1;
        }

        path_input = new Input(244, 14, input_placeholder, input_tag);
        if (path_input == nullptr) {
            delete input_row;
            freeWidgets();
            return -1;
        }

        if (input_row->addWidget(path_input, LEFT) < 0) {
            delete input_row;
            freeWidgets();
            return -1;
        }

        if (widgets->addLayout(input_row) < 0) {
            delete input_row;
            freeWidgets();
            return -1;
        }

        Layout* buttons = new Layout(48, 76, 172, 18, HORIZONTAL, LAYOUT_FLAG_NONE);
        if (buttons == nullptr) {
            freeWidgets();
            return -1;
        }

        Button* create_button = new Button(80, 14, create_label, Function<void()>([&action]() {
            action = EDITOR_NEW_FILE_POPUP_ACTION_CREATE;
        }));
        Button* cancel_button = new Button(80, 14, cancel_label, Function<void()>([&action]() {
            action = EDITOR_NEW_FILE_POPUP_ACTION_CANCEL;
        }));
        if (create_button == nullptr || cancel_button == nullptr) {
            if (create_button != nullptr) {
                delete create_button;
            }
            if (cancel_button != nullptr) {
                delete cancel_button;
            }
            delete buttons;
            freeWidgets();
            return -1;
        }

        if (buttons->addWidget(create_button, LEFT) < 0 ||
            buttons->addWidget(cancel_button, LEFT) < 0) {
            delete buttons;
            freeWidgets();
            return -1;
        }

        if (widgets->addLayout(buttons) < 0) {
            delete buttons;
            freeWidgets();
            return -1;
        }

        return 0;
    };

    if (buildWidgets() < 0) {
        printf("[editor] popup thread: widget build failed\n");
        shared->running = 0;
        return;
    }

    while (1) {
        popup.drawRect(0, 0, 268, 112, COLOR_BG);
        popup.drawContouredRect(0, 0, 268, 112);
        popup.drawText(10, 8, "Create New File", COLOR_BLACK);
        popup.drawText(10, 20, "Path:", COLOR_BLACK);
        popup.drawText(10, 56, "Enter=Create  Esc/F4=Cancel", COLOR_VGA_MEDIUM_DARK_GRAY);

        if (status[0] != 0) {
            popup.drawText(10, 66, status, COLOR_VGA_RED);
        }

        if (widgets != nullptr) {
            widgets->draw(&popup);
        }

        struct gfx_event event;
        int ret = gfx_get_event(&event, GFX_EVENT_BLOCKING);
        if (ret < 0) {
            continue;
        }

        switch (event.event) {
        case GFX_EVENT_KEYBOARD:
            if (event.data == '\n' || event.data == '\r') {
                action = EDITOR_NEW_FILE_POPUP_ACTION_CREATE;
                break;
            }

            if (event.data == 27 || event.data == KEY_F4) {
                action = EDITOR_NEW_FILE_POPUP_ACTION_CANCEL;
                break;
            }

            if (path_input != nullptr) {
                unsigned char key = (unsigned char) event.data;
                if (key == 127) {
                    key = '\b';
                }
                path_input->Keyboard(key);
            }
            break;

        case GFX_EVENT_MOUSE:
            if (widgets != nullptr) {
                widgets->Mouse(event.data, event.data2);
            }
            break;

        case GFX_EVENT_EXIT:
            action = EDITOR_NEW_FILE_POPUP_ACTION_CANCEL;
            break;

        case GFX_EVENT_RESOLUTION:
            break;

        default:
            break;
        }

        if (action == EDITOR_NEW_FILE_POPUP_ACTION_CREATE) {
            action = EDITOR_NEW_FILE_POPUP_ACTION_NONE;
            memset(status, 0, sizeof(status));

            char trimmed[256];
            memset(trimmed, 0, sizeof(trimmed));
            if (path_input == nullptr || trim_copy_path(path_input->getData(), trimmed, sizeof(trimmed)) <= 0) {
                strncpy(status, "Path is empty.", (uint32_t)(sizeof(status) - 1));
                printf("[editor] popup create: empty path\n");
                continue;
            }

            strncpy(shared->path, trimmed, (uint32_t)(sizeof(shared->path) - 1));
            shared->path[sizeof(shared->path) - 1] = 0;
            shared->result = EDITOR_NEW_FILE_POPUP_RESULT_CREATE;
            shared->running = 0;
            printf("[editor] popup create: path=%s\n", shared->path);
            freeWidgets();
            return;
        }

        if (action == EDITOR_NEW_FILE_POPUP_ACTION_CANCEL) {
            shared->result = EDITOR_NEW_FILE_POPUP_RESULT_CANCEL;
            shared->running = 0;
            printf("[editor] popup canceled\n");
            freeWidgets();
            return;
        }
    }
}

Editor::Editor()
    : Window(EDITOR_DEFAULT_WINDOW_WIDTH, EDITOR_DEFAULT_HEIGHT, "Editor", 1)
{
    treeView = nullptr;
    m_fd = -1;
    m_textBuffer = nullptr;
    m_bufferSize = EDITOR_BUFFER_SIZE;
    m_fileSize = 0;
    m_bufferHead = 0;
    m_bufferEdit = 0;
    m_x = 0;
    m_y = 0;
    m_dirty = 0;
    m_preferredColumn = -1;
    scrollY = 0;
    m_scrollWrap = 0;
    m_textColor = COLOR_TEXT;
    c_width = EDITOR_DEFAULT_RIGHT_PANE_WIDTH;
    c_height = EDITOR_DEFAULT_HEIGHT;
    m_ropeHead = nullptr;
    m_ropeTail = nullptr;

    m_textBuffer = (unsigned char*) malloc(m_bufferSize);
    if (m_textBuffer == nullptr) {
        exit();
    }

    memset(m_textBuffer, 0, m_bufferSize);
    memset(m_currentPath, 0, sizeof(m_currentPath));

    treeView = new TreeView(0, 0, TREE_VIEW_WIDTH, c_height);

    gfx_draw_rectangle(0, 0, EDITOR_DEFAULT_WINDOW_WIDTH, c_height, COLOR_BG);
    gfx_draw_format_text(20, 20, COLOR_VGA_MEDIUM_GRAY, "Loading...");

    reDrawHeader();
    reDraw(0, 0);
}

Editor::~Editor()
{
    if (m_fd >= 0) {
        fclose(m_fd);
        m_fd = -1;
    }

    if (treeView != nullptr) {
        delete treeView;
        treeView = nullptr;
    }

    ropeClear();

    if (m_textBuffer != nullptr) {
        free(m_textBuffer);
        m_textBuffer = nullptr;
    }
}

int Editor::gutterWidth() const
{
    int digits = digitCount(countLines());
    int width = (digits * 8) + 6;

    if (width < 18) {
        width = 18;
    }
    if (width > 42) {
        width = 42;
    }

    return width;
}

int Editor::textStartX() const
{
    return TREE_VIEW_WIDTH + gutterWidth() + TEXT_GAP_WIDTH;
}

int Editor::textClipWidth() const
{
    int width = c_width - gutterWidth() - TEXT_GAP_WIDTH - TEXT_RIGHT_PADDING;
    if (width < 8) {
        width = 8;
    }
    return width;
}

int Editor::visibleLines() const
{
    int h = c_height - HEADER_HEIGHT - STATUS_HEIGHT;
    if (h < 8) {
        return 1;
    }

    return h / 8;
}

int Editor::visibleColumns() const
{
    int w = textClipWidth();
    if (w < 8) {
        return 1;
    }

    return w / 8;
}

int Editor::lineNumberAtIndex(int index) const
{
    int line = 1;
    int i = clampInt(index, 0, m_bufferHead);

    for (int j = 0; j < i; j++) {
        if (m_textBuffer[j] == '\n') {
            line++;
        }
    }

    return line;
}

int Editor::indexFromLine(int line) const
{
    if (line <= 0) {
        return 0;
    }

    int currentLine = 0;
    int i = 0;
    while (i < m_bufferHead && currentLine < line) {
        if (m_textBuffer[i] == '\n') {
            currentLine++;
        }
        i++;
    }

    return i;
}

int Editor::lineStart(int index) const
{
    int i = clampInt(index, 0, m_bufferHead);

    while (i > 0 && m_textBuffer[i - 1] != '\n') {
        i--;
    }

    return i;
}

int Editor::lineEnd(int index) const
{
    int i = clampInt(index, 0, m_bufferHead);

    while (i < m_bufferHead && m_textBuffer[i] != '\n' && m_textBuffer[i] != 0) {
        i++;
    }

    return i;
}

int Editor::lineRowsFromStart(int line_start, int cols) const
{
    int line_end = lineEnd(line_start);
    int len = line_end - line_start;

    if (len <= 0) {
        return 1;
    }

    if (cols < 1) {
        cols = 1;
    }

    return ((len - 1) / cols) + 1;
}

int Editor::viewStartIndex(int cols) const
{
    int idx = indexFromLine(scrollY);

    for (int r = 0; r < m_scrollWrap; r++) {
        int next = nextVisualRowStart(idx, cols);
        if (next <= idx) {
            break;
        }
        idx = next;
    }

    return idx;
}

int Editor::nextVisualRowStart(int row_start_index, int cols) const
{
    if (cols < 1) {
        cols = 1;
    }

    int idx = clampInt(row_start_index, 0, m_bufferHead);
    if (idx >= m_bufferHead) {
        return m_bufferHead;
    }

    int col = 0;
    while (idx < m_bufferHead) {
        unsigned char ch = m_textBuffer[idx];
        if (ch == '\n') {
            return idx + 1;
        }

        idx++;
        col++;
        if (col >= cols) {
            return idx;
        }
    }

    return m_bufferHead;
}

int Editor::prevVisualRowStart(int row_start_index, int cols) const
{
    int target = clampInt(row_start_index, 0, m_bufferHead);
    if (target <= 0) {
        return 0;
    }

    int prev = 0;
    int cur = 0;

    while (cur < target) {
        int next = nextVisualRowStart(cur, cols);
        if (next <= cur || next >= target) {
            break;
        }

        prev = cur;
        cur = next;
    }

    return prev;
}

void Editor::cursorFromViewStart(int view_start, int cols, int* out_row, int* out_col) const
{
    int row = 0;
    int col = 0;

    if (m_bufferEdit < view_start) {
        *out_row = -1;
        *out_col = 0;
        return;
    }

    int limit = clampInt(m_bufferEdit, 0, m_bufferHead);
    for (int i = view_start; i < limit; i++) {
        unsigned char ch = m_textBuffer[i];
        if (ch == '\n') {
            row++;
            col = 0;
            continue;
        }

        col++;
        if (col >= cols) {
            row++;
            col = 0;
        }
    }

    *out_row = row;
    *out_col = col;
}

void Editor::setScrollFromIndex(int index, int cols)
{
    int idx = clampInt(index, 0, m_bufferHead);

    int line = 0;
    int line_start = 0;
    for (int i = 0; i < idx; i++) {
        if (m_textBuffer[i] == '\n') {
            line++;
            line_start = i + 1;
        }
    }

    scrollY = line;
    m_scrollWrap = 0;

    int row_start = line_start;
    while (row_start < idx) {
        int next = nextVisualRowStart(row_start, cols);
        if (next <= row_start || next > idx) {
            break;
        }

        m_scrollWrap++;
        row_start = next;
    }

    normalizeScroll(cols);
}

void Editor::normalizeScroll(int cols)
{
    if (cols < 1) {
        cols = 1;
    }

    int totalLines = countLines();
    if (totalLines < 1) {
        totalLines = 1;
    }

    scrollY = clampInt(scrollY, 0, totalLines - 1);

    int line_start = indexFromLine(scrollY);
    int line_rows = lineRowsFromStart(line_start, cols);
    m_scrollWrap = clampInt(m_scrollWrap, 0, line_rows - 1);
}

void Editor::ensureCursorVisible(int cols, int rows)
{
    if (cols < 1) {
        cols = 1;
    }
    if (rows < 1) {
        rows = 1;
    }

    normalizeScroll(cols);

    int guard_max = m_bufferHead + rows + 16;
    if (guard_max < 64) {
        guard_max = 64;
    }

    for (int guard = 0; guard < guard_max; guard++) {
        int view_start = viewStartIndex(cols);
        int cursor_row = 0;
        int cursor_col = 0;
        cursorFromViewStart(view_start, cols, &cursor_row, &cursor_col);

        if (cursor_row < 0) {
            int prev = prevVisualRowStart(view_start, cols);
            if (prev == view_start) {
                break;
            }
            setScrollFromIndex(prev, cols);
            continue;
        }

        if (cursor_row >= rows) {
            int next = nextVisualRowStart(view_start, cols);
            if (next == view_start) {
                break;
            }
            setScrollFromIndex(next, cols);
            continue;
        }

        break;
    }

    normalizeScroll(cols);
}

void Editor::drawLineNumbers(const int* row_lines, int row_count)
{
    int gutter_x = TREE_VIEW_WIDTH;
    int gutter_w = gutterWidth();

    int text_h = c_height - HEADER_HEIGHT - STATUS_HEIGHT;
    if (text_h < 8) {
        text_h = 8;
    }

    gfx_draw_rectangle(gutter_x, HEADER_HEIGHT, gutter_w, text_h, COLOR_BG);
    gfx_draw_line(gutter_x + gutter_w - 1, 0, gutter_x + gutter_w - 1, c_height, COLOR_VGA_MEDIUM_GRAY);

    int rows = visibleLines();
    int draw_rows = minInt(rows, row_count);
    for (int row = 0; row < draw_rows; row++) {
        int line_no = row_lines[row];
        if (line_no <= 0) {
            continue;
        }

        if (row > 0 && row_lines[row - 1] == line_no) {
            continue;
        }

        int digits = digitCount(line_no);
        int x = gutter_x + gutter_w - (digits * 8) - 2;
        if (x < gutter_x + 1) {
            x = gutter_x + 1;
        }

        gfx_draw_format_text(x, HEADER_HEIGHT + (row * 8), COLOR_VGA_MEDIUM_GRAY, "%d", line_no);
    }
}

void Editor::drawStatusLine(color_t color, const char* fmt, ...)
{
    char msg[256];
    memset(msg, 0, sizeof(msg));

    va_list args;
    va_start(args, fmt);
    csprintf(msg, fmt, args);
    va_end(args);

    int x = textStartX();
    int y = c_height - STATUS_HEIGHT;
    int w = textClipWidth();

    if (w < 8) {
        w = 8;
    }

    gfx_draw_rectangle(x, y, w, STATUS_HEIGHT, COLOR_BG);

    int max_chars = w / 8;
    if (max_chars < 1) {
        return;
    }

    int len = strlen(msg);
    if (len > max_chars) {
        msg[max_chars] = 0;
    }

    gfx_draw_format_text(x, y, color, "%s", msg);
}

void Editor::reDrawHeader()
{
    gfx_draw_rectangle(TREE_VIEW_WIDTH, 0, c_width, HEADER_HEIGHT, COLOR_BG);
    drawHeaderTable(TREE_VIEW_WIDTH, c_width);

    const char* left_label = "F1 Save  F4 Open";
    const char* right_label = "F9 Help";

    int content_left = TREE_VIEW_WIDTH + 4;
    int content_right = TREE_VIEW_WIDTH + c_width - 4;

    int left_w = strlen(left_label) * 8;
    int right_w = strlen(right_label) * 8;

    int right_x = content_right - right_w;
    if (right_x < content_left + left_w + 8) {
        left_label = "F1 Save";
        left_w = strlen(left_label) * 8;
    }

    right_w = strlen(right_label) * 8;
    right_x = content_right - right_w;
    if (right_x < content_left + left_w + 8) {
        right_label = "F9";
        right_w = strlen(right_label) * 8;
        right_x = content_right - right_w;
    }

    if (right_x < content_left + left_w + 8) {
        left_label = "";
        left_w = 0;
    }

    if (right_x < content_left) {
        right_x = content_left;
    }

    if (left_w > 0) {
        gfx_draw_format_text(content_left, 2, COLOR_BLACK, "%s", left_label);
    }
    gfx_draw_format_text(right_x, 2, COLOR_BLACK, "%s", right_label);
}

void Editor::reDraw(int from, int to)
{
    (void)from;
    (void)to;

    int cols = visibleColumns();
    int rows = visibleLines();

    if (cols < 1) {
        cols = 1;
    }
    if (rows < 1) {
        rows = 1;
    }

    normalizeScroll(cols);

    int text_x = textStartX();
    int text_w = textClipWidth();
    int text_h = c_height - HEADER_HEIGHT - STATUS_HEIGHT;
    if (text_h < 8) {
        text_h = 8;
    }

    gfx_draw_rectangle(text_x, HEADER_HEIGHT, text_w, text_h, COLOR_BG);

    int draw_rows = rows;
    if (draw_rows > EDITOR_MAX_VIEW_ROWS) {
        draw_rows = EDITOR_MAX_VIEW_ROWS;
    }

    int row_lines[EDITOR_MAX_VIEW_ROWS];
    for (int i = 0; i < draw_rows; i++) {
        row_lines[i] = 0;
    }

    int view_start = viewStartIndex(cols);
    int line_no = lineNumberAtIndex(view_start);

    int row = 0;
    int col = 0;
    if (draw_rows > 0) {
        row_lines[0] = line_no;
    }

    setColor(COLOR_TEXT);

    for (int i = view_start; i < m_bufferHead && row < draw_rows; i++) {
        unsigned char ch = m_textBuffer[i];

        if (ch == '\n') {
            row++;
            col = 0;
            line_no++;
            if (row < draw_rows) {
                row_lines[row] = line_no;
            }
            continue;
        }

        if (col >= cols) {
            row++;
            col = 0;
            if (row >= draw_rows) {
                break;
            }
            row_lines[row] = line_no;
        }

        if (isAlpha(ch)) {
            if (i == 0 || !isAlpha(m_textBuffer[i - 1])) {
                highlightSyntax(&m_textBuffer[i]);
            }
        } else {
            setColor(COLOR_TEXT);
        }

        if (ch < 32 || ch > 126) {
            ch = '?';
        }

        gfx_draw_char(text_x + (col * 8), HEADER_HEIGHT + (row * 8), ch, m_textColor);
        col++;
    }

    drawLineNumbers(row_lines, draw_rows);

    int cursor_row = 0;
    int cursor_col = 0;
    cursorFromViewStart(view_start, cols, &cursor_row, &cursor_col);

    if (cursor_row >= 0 && cursor_row < draw_rows) {
        int cx = text_x + (cursor_col * 8);
        int cy = HEADER_HEIGHT + (cursor_row * 8);

        if (cx <= text_x + (cols * 8) - 8) {
            gfx_draw_rectangle(cx, cy, 8, 8, COLOR_VGA_LIGHT_GRAY);

            if (m_bufferEdit < m_bufferHead && m_textBuffer[m_bufferEdit] != '\n') {
                unsigned char ch = m_textBuffer[m_bufferEdit];
                if (ch >= 32 && ch <= 126) {
                    if (isAlpha(ch)) {
                        if (m_bufferEdit == 0 || !isAlpha(m_textBuffer[m_bufferEdit - 1])) {
                            highlightSyntax(&m_textBuffer[m_bufferEdit]);
                        }
                    } else {
                        setColor(COLOR_TEXT);
                    }
                    gfx_draw_char(cx, cy, ch, m_textColor);
                }
            }
        }
    }

    int line = 1;
    int colnum = 1;
    getCursorLineCol(line, colnum);

    const char* file_label = m_currentPath[0] ? m_currentPath : "(untitled)";
    const char* dirty_tag = hasUnsavedChanges() ? "*" : "";

    drawStatusLine(COLOR_VGA_MEDIUM_DARK_GRAY, "%s%s  Ln:%d Col:%d", file_label, dirty_tag, line, colnum);
}

void Editor::setColor(color_t color)
{
    m_textColor = color;
}

void Editor::drawChar(unsigned char c, color_t bg)
{
    int cols = visibleColumns();
    int rows = visibleLines();

    if (cols < 1 || rows < 1) {
        return;
    }

    if (c == '\n') {
        m_x = 0;
        m_y++;
        return;
    }

    if (m_x >= cols) {
        m_x = 0;
        m_y++;
    }

    if (m_y < 0 || m_y >= rows) {
        return;
    }

    int drawX = textStartX() + (m_x * 8);
    int drawY = HEADER_HEIGHT + (m_y * 8);

    gfx_draw_rectangle(drawX, drawY, 8, 8, bg);

    if (c < 32 || c > 126) {
        c = '?';
    }

    gfx_draw_char(drawX, drawY, c, m_textColor);
    m_x++;
}

void Editor::highlightSyntax(unsigned char* start)
{
    if (start == nullptr) {
        setColor(COLOR_TEXT);
        return;
    }

    int tokenLen = 0;
    while (isAlpha(start[tokenLen])) {
        tokenLen++;
    }

    if (tokenLen == 0) {
        setColor(COLOR_TEXT);
        return;
    }

    for (int i = 0; i < 20; i++) {
        struct keyword* key = &keyWords[i];
        if (key->color == 0) {
            break;
        }

        int keyLen = strlen(key->word);
        if (tokenLen == keyLen && memcmp(start, key->word, keyLen) == 0) {
            setColor(key->color);
            return;
        }
    }

    setColor(COLOR_TEXT);
}

int Editor::countLines() const
{
    int lines = 1;
    for (int i = 0; i < m_bufferHead; i++) {
        if (m_textBuffer[i] == '\n') {
            lines++;
        }
    }
    return lines;
}

bool Editor::hasUnsavedChanges() const
{
    return m_dirty != 0;
}

void Editor::getCursorLineCol(int& line, int& col) const
{
    line = 1;
    col = 1;

    int limit = clampInt(m_bufferEdit, 0, m_bufferHead);
    for (int i = 0; i < limit; i++) {
        if (m_textBuffer[i] == '\n') {
            line++;
            col = 1;
        } else {
            col++;
        }
    }
}

void Editor::Lex()
{
}

bool Editor::ropeLocate(int index, struct rope_node** out_node, int* out_offset) const
{
    if (index < 0 || index > m_bufferHead) {
        return false;
    }

    int pos = 0;
    struct rope_node* node = m_ropeHead;
    while (node != nullptr) {
        if (index <= pos + node->len) {
            *out_node = node;
            *out_offset = index - pos;
            return true;
        }

        pos += node->len;
        node = node->next;
    }

    if (index == pos) {
        *out_node = nullptr;
        *out_offset = 0;
        return true;
    }

    return false;
}

Editor::rope_node* Editor::ropeCreateNode()
{
    struct rope_node* node = (struct rope_node*) malloc(sizeof(struct rope_node));
    if (node == nullptr) {
        return nullptr;
    }

    memset(node, 0, sizeof(struct rope_node));
    node->prev = nullptr;
    node->next = nullptr;
    node->len = 0;
    return node;
}

void Editor::ropeDetachNode(struct rope_node* node)
{
    if (node == nullptr) {
        return;
    }

    if (node->prev != nullptr) {
        node->prev->next = node->next;
    } else {
        m_ropeHead = node->next;
    }

    if (node->next != nullptr) {
        node->next->prev = node->prev;
    } else {
        m_ropeTail = node->prev;
    }

    free(node);
}

void Editor::ropeTryMerge(struct rope_node* node)
{
    if (node == nullptr) {
        return;
    }

    if (node->next != nullptr) {
        struct rope_node* next = node->next;
        if (node->len + next->len <= ROPE_CHUNK_SIZE) {
            memcpy(&node->data[node->len], next->data, next->len);
            node->len += next->len;
            ropeDetachNode(next);
        }
    }

    if (node->prev != nullptr) {
        struct rope_node* prev = node->prev;
        if (prev->len + node->len <= ROPE_CHUNK_SIZE) {
            memcpy(&prev->data[prev->len], node->data, node->len);
            prev->len += node->len;
            ropeDetachNode(node);
        }
    }
}

void Editor::ropeClear()
{
    struct rope_node* node = m_ropeHead;
    while (node != nullptr) {
        struct rope_node* next = node->next;
        free(node);
        node = next;
    }

    m_ropeHead = nullptr;
    m_ropeTail = nullptr;
}

bool Editor::ropeLoadFromBuffer(const unsigned char* data, int len)
{
    ropeClear();

    if (data == nullptr || len <= 0) {
        m_bufferHead = 0;
        return true;
    }

    int pos = 0;
    while (pos < len) {
        int chunk = len - pos;
        if (chunk > ROPE_CHUNK_SIZE) {
            chunk = ROPE_CHUNK_SIZE;
        }

        struct rope_node* node = ropeCreateNode();
        if (node == nullptr) {
            ropeClear();
            return false;
        }

        memcpy(node->data, &data[pos], chunk);
        node->len = chunk;

        if (m_ropeTail == nullptr) {
            m_ropeHead = node;
            m_ropeTail = node;
        } else {
            m_ropeTail->next = node;
            node->prev = m_ropeTail;
            m_ropeTail = node;
        }

        pos += chunk;
    }

    m_bufferHead = len;
    return true;
}

bool Editor::ropeSyncToBuffer()
{
    int pos = 0;
    struct rope_node* node = m_ropeHead;
    while (node != nullptr) {
        int to_copy = node->len;
        if (to_copy < 0) {
            to_copy = 0;
        }

        if (pos + to_copy > m_bufferSize - 1) {
            to_copy = (m_bufferSize - 1) - pos;
            if (to_copy < 0) {
                to_copy = 0;
            }
        }

        if (to_copy > 0) {
            memcpy(&m_textBuffer[pos], node->data, to_copy);
            pos += to_copy;
        }

        if (pos >= m_bufferSize - 1) {
            break;
        }

        node = node->next;
    }

    m_bufferHead = pos;
    m_textBuffer[m_bufferHead] = 0;

    if (m_bufferEdit > m_bufferHead) {
        m_bufferEdit = m_bufferHead;
    }

    return true;
}

bool Editor::ropeInsertChar(int index, unsigned char c)
{
    if (m_bufferHead >= m_bufferSize - 1) {
        return false;
    }

    struct rope_node* node = nullptr;
    int offset = 0;
    if (!ropeLocate(index, &node, &offset)) {
        return false;
    }

    if (m_ropeHead == nullptr) {
        struct rope_node* fresh = ropeCreateNode();
        if (fresh == nullptr) {
            return false;
        }

        fresh->data[0] = c;
        fresh->len = 1;
        m_ropeHead = fresh;
        m_ropeTail = fresh;
        m_bufferHead++;
        return true;
    }

    if (node == nullptr) {
        if (m_ropeTail->len < ROPE_CHUNK_SIZE) {
            m_ropeTail->data[m_ropeTail->len++] = c;
        } else {
            struct rope_node* fresh = ropeCreateNode();
            if (fresh == nullptr) {
                return false;
            }

            fresh->data[0] = c;
            fresh->len = 1;
            fresh->prev = m_ropeTail;
            m_ropeTail->next = fresh;
            m_ropeTail = fresh;
        }

        m_bufferHead++;
        return true;
    }

    if (offset == node->len && node->len < ROPE_CHUNK_SIZE) {
        node->data[node->len++] = c;
        m_bufferHead++;
        return true;
    }

    if (offset == 0 && node->len < ROPE_CHUNK_SIZE) {
        memmove(&node->data[1], &node->data[0], node->len);
        node->data[0] = c;
        node->len++;
        m_bufferHead++;
        return true;
    }

    if (offset > 0 && offset < node->len) {
        int right_len = node->len - offset;

        struct rope_node* right = ropeCreateNode();
        if (right == nullptr) {
            return false;
        }

        memcpy(right->data, &node->data[offset], right_len);
        right->len = right_len;

        node->len = offset;

        right->next = node->next;
        right->prev = node;
        if (node->next != nullptr) {
            node->next->prev = right;
        } else {
            m_ropeTail = right;
        }
        node->next = right;

        if (node->len < ROPE_CHUNK_SIZE) {
            node->data[node->len++] = c;
        } else {
            struct rope_node* mid = ropeCreateNode();
            if (mid == nullptr) {
                return false;
            }

            mid->data[0] = c;
            mid->len = 1;

            mid->prev = node;
            mid->next = right;
            node->next = mid;
            right->prev = mid;
        }

        m_bufferHead++;
        return true;
    }

    struct rope_node* fresh = ropeCreateNode();
    if (fresh == nullptr) {
        return false;
    }

    fresh->data[0] = c;
    fresh->len = 1;

    if (offset <= 0) {
        fresh->prev = node->prev;
        fresh->next = node;
        if (node->prev != nullptr) {
            node->prev->next = fresh;
        } else {
            m_ropeHead = fresh;
        }
        node->prev = fresh;
    } else {
        fresh->next = node->next;
        fresh->prev = node;
        if (node->next != nullptr) {
            node->next->prev = fresh;
        } else {
            m_ropeTail = fresh;
        }
        node->next = fresh;
    }

    m_bufferHead++;
    return true;
}

bool Editor::ropeDeleteChar(int index)
{
    if (index < 0 || index >= m_bufferHead) {
        return false;
    }

    struct rope_node* node = nullptr;
    int offset = 0;
    if (!ropeLocate(index, &node, &offset)) {
        return false;
    }

    if (node == nullptr) {
        return false;
    }

    if (offset == node->len && node->next != nullptr) {
        node = node->next;
        offset = 0;
    }

    if (offset < 0 || offset >= node->len) {
        return false;
    }

    memmove(&node->data[offset], &node->data[offset + 1], node->len - offset - 1);
    node->len--;
    m_bufferHead--;

    if (node->len <= 0) {
        ropeDetachNode(node);
    } else {
        ropeTryMerge(node);
    }

    if (m_bufferHead < 0) {
        m_bufferHead = 0;
    }

    return true;
}

void Editor::scroll(int lines)
{
    if (lines == 0) {
        return;
    }

    int cols = visibleColumns();
    normalizeScroll(cols);

    if (lines > 0) {
        while (lines > 0) {
            int start = viewStartIndex(cols);
            int next = nextVisualRowStart(start, cols);
            if (next == start) {
                break;
            }
            setScrollFromIndex(next, cols);
            lines--;
        }
    } else {
        while (lines < 0) {
            int start = viewStartIndex(cols);
            int prev = prevVisualRowStart(start, cols);
            if (prev == start) {
                break;
            }
            setScrollFromIndex(prev, cols);
            lines++;
        }
    }

    normalizeScroll(cols);
}

void Editor::Reset()
{
    ropeClear();

    memset(m_textBuffer, 0, m_bufferSize);
    m_bufferHead = 0;
    m_bufferEdit = 0;
    m_fileSize = 0;
    m_dirty = 0;
    m_preferredColumn = -1;
    scrollY = 0;
    m_scrollWrap = 0;
    m_currentPath[0] = 0;

    reDrawHeader();
    reDraw(0, 0);
}

bool Editor::SaveMsg()
{
    if (!hasUnsavedChanges()) {
        return true;
    }

    MsgBox msg("Save changes?", "Save changes to file?", MSGBOX_YES_NO_CANCEL);
    MsgBoxResult ret = msg.show();

    if (ret == MSGBOX_YES) {
        return Save();
    }

    if (ret == MSGBOX_NO) {
        return true;
    }

    return false;
}

bool Editor::Quit()
{
    if (!SaveMsg()) {
        reDraw(0, m_bufferHead);
        return false;
    }

    if (m_fd >= 0) {
        fclose(m_fd);
        m_fd = -1;
    }

    exit();
    return true;
}

bool Editor::Open(char* path)
{
    if (path == nullptr || path[0] == 0) {
        printf("[editor] Open: empty path\n");
        return false;
    }
    printf("[editor] Open: requested %s\n", path);

    int old_fd = m_fd;

    if (m_fd >= 0) {
        if (!SaveMsg()) {
            reDraw(0, m_bufferHead);
            return false;
        }
    }

    int new_fd = open_editor_file(path);
    if (new_fd < 0) {
        printf("[editor] Open: failed %s\n", path);
        drawStatusLine(COLOR_VGA_RED, "Unable to open/create %s", path);
        return false;
    }

    if (old_fd >= 0) {
        fclose(old_fd);
    }
    m_fd = new_fd;

    setTitle(path);

    strncpy(m_currentPath, path, (uint32_t)(sizeof(m_currentPath) - 1));
    m_currentPath[sizeof(m_currentPath) - 1] = 0;

    memset(m_textBuffer, 0, m_bufferSize);

    int read_bytes = read(m_fd, m_textBuffer, m_bufferSize - 1);
    if (read_bytes < 0) {
        read_bytes = 0;
    }

    if (read_bytes >= m_bufferSize) {
        read_bytes = m_bufferSize - 1;
    }

    m_textBuffer[read_bytes] = 0;
    m_bufferHead = read_bytes;

    if (!ropeLoadFromBuffer(m_textBuffer, m_bufferHead)) {
        if (m_fd >= 0) {
            fclose(m_fd);
            m_fd = -1;
        }
        m_currentPath[0] = 0;
        m_textBuffer[0] = 0;
        m_bufferHead = 0;
        m_bufferEdit = 0;
        m_fileSize = 0;
        m_dirty = 0;
        m_preferredColumn = -1;
        scrollY = 0;
        m_scrollWrap = 0;
        drawStatusLine(COLOR_VGA_RED, "Out of memory while loading file.");
        reDrawHeader();
        reDraw(0, m_bufferHead);
        return false;
    }

    m_bufferEdit = 0;
    scrollY = 0;
    m_scrollWrap = 0;
    m_fileSize = m_bufferHead;
    m_dirty = 0;
    m_preferredColumn = -1;

    reDrawHeader();
    reDraw(0, m_bufferHead);
    printf("[editor] Open: success %s (bytes=%d)\n", path, m_bufferHead);
    return true;
}

void Editor::setFd(int fd)
{
    m_fd = fd;
}

bool Editor::Save()
{
    if (m_fd < 0 && m_currentPath[0] == 0) {
        drawStatusLine(COLOR_VGA_RED, "No open file. Use F4 to open/create one.");
        return false;
    }

    if (!ropeSyncToBuffer()) {
        drawStatusLine(COLOR_VGA_RED, "Save failed.");
        return false;
    }

    if (m_currentPath[0] != 0) {
        if (m_fd >= 0) {
            fclose(m_fd);
            m_fd = -1;
        }

        m_fd = open_editor_file(m_currentPath);
        if (m_fd < 0) {
            drawStatusLine(COLOR_VGA_RED, "Save failed.");
            return false;
        }
    }

    int ret = write(m_fd, m_textBuffer, m_bufferHead);
    if (ret < 0 || ret != m_bufferHead) {
        drawStatusLine(COLOR_VGA_RED, "Save failed.");
        return false;
    }

    m_fileSize = m_bufferHead;
    m_dirty = 0;
    reDraw(0, m_bufferHead);
    return true;
}

void Editor::applyResolution(int window_width, int window_height)
{
    c_width = window_width - TREE_VIEW_WIDTH;
    if (c_width < EDITOR_MIN_RIGHT_PANE_WIDTH) {
        c_width = EDITOR_MIN_RIGHT_PANE_WIDTH;
    }

    c_height = window_height;
    if (c_height < EDITOR_MIN_HEIGHT) {
        c_height = EDITOR_MIN_HEIGHT;
    }

    if (treeView != nullptr) {
        treeView->resize(TREE_VIEW_WIDTH, c_height);
    }

    int cols = visibleColumns();
    int rows = visibleLines();
    ensureCursorVisible(cols, rows);
}

bool Editor::showNewFileDialog(char* out_path, int out_path_size)
{
    if (out_path == nullptr || out_path_size <= 1) {
        return false;
    }

    out_path[0] = 0;

    /* Must live on heap: user threads have separate stacks in this kernel. */
    EditorNewFilePopupShared* shared = (EditorNewFilePopupShared*) malloc(sizeof(EditorNewFilePopupShared));
    if (shared == nullptr) {
        printf("[editor] showNewFileDialog: alloc failed\n");
        drawStatusLine(COLOR_VGA_RED, "New File: out of memory.");
        return false;
    }

    memset(shared, 0, sizeof(EditorNewFilePopupShared));
    shared->running = 1;
    shared->result = EDITOR_NEW_FILE_POPUP_RESULT_CANCEL;
    shared->path[0] = 0;

    Thread popup_thread(__editor_new_file_popup_thread, 0);
    int thread_id = popup_thread.start((void*)shared);
    if (thread_id < 0) {
        printf("[editor] showNewFileDialog: thread start failed\n");
        drawStatusLine(COLOR_VGA_RED, "Failed to open New File window.");
        free(shared);
        return false;
    }
    printf("[editor] showNewFileDialog: thread id=%d\n", thread_id);

    int await_ret = invoke_syscall(SYSCALL_AWAIT_PROCESS, thread_id, 0, 0);
    if (await_ret < 0) {
        printf("[editor] showNewFileDialog: await failed ret=%d\n", await_ret);
        drawStatusLine(COLOR_VGA_RED, "Failed waiting for New File window.");
        free(shared);
        return false;
    }
    printf("[editor] showNewFileDialog: done result=%d path=%s\n", shared->result, shared->path);

    if (shared->result != EDITOR_NEW_FILE_POPUP_RESULT_CREATE || shared->path[0] == 0) {
        printf("[editor] showNewFileDialog: canceled/invalid\n");
        free(shared);
        return false;
    }

    strncpy(out_path, shared->path, (uint32_t)(out_path_size - 1));
    out_path[out_path_size - 1] = 0;
    free(shared);
    return true;
}

void Editor::FileChooser()
{
    char selected_file[256];
    memset(selected_file, 0, sizeof(selected_file));

    enum {
        CHOOSER_ACTION_NONE = 0,
        CHOOSER_ACTION_OPEN = 1,
        CHOOSER_ACTION_NEW = 2,
        CHOOSER_ACTION_CANCEL = 3
    };

    volatile int chooser_action = CHOOSER_ACTION_NONE;
    WidgetManager* chooser_widgets = nullptr;
    Button* open_button = nullptr;

    char open_label[] = "Open";
    char new_label[] = "New File";
    char cancel_label[] = "Cancel";

    int cached_panel_x = -1;
    int cached_panel_y = -1;
    int cached_panel_w = -1;
    int cached_panel_h = -1;

    auto freeChooserWidgets = [&]() {
        if (chooser_widgets != nullptr) {
            delete chooser_widgets;
            chooser_widgets = nullptr;
            open_button = nullptr;
        }
    };

    auto rebuildChooserWidgets = [&](int panel_x, int panel_y, int panel_w, int panel_h) {
        freeChooserWidgets();
        chooser_action = CHOOSER_ACTION_NONE;

        chooser_widgets = new WidgetManager();
        if (chooser_widgets == nullptr) {
            return;
        }

        int button_h = 14;
        int button_w = 82;
        int button_gap = 6;

        if (panel_w >= 170) {
            if ((button_w * 3) + (button_gap * 2) > panel_w - 8) {
                button_w = (panel_w - 8 - (button_gap * 2)) / 3;
                if (button_w < 44) {
                    button_w = 44;
                }
            }

            int content_w = (button_w * 3) + (button_gap * 2);
            int layout_x = panel_x + (panel_w - content_w) / 2;
            if (layout_x < panel_x + 2) {
                layout_x = panel_x + 2;
            }

            int layout_y = panel_y + panel_h - button_h - 8;
            if (layout_y < panel_y + 18) {
                layout_y = panel_y + 18;
            }

            Layout* actions = new Layout(layout_x, layout_y, content_w + 4, button_h + 4, HORIZONTAL, LAYOUT_FLAG_NONE);
            if (actions == nullptr) {
                return;
            }

            open_button = new Button(button_w, button_h, open_label, Function<void()>([&chooser_action]() {
                chooser_action = CHOOSER_ACTION_OPEN;
            }));
            Button* new_button = new Button(button_w, button_h, new_label, Function<void()>([&chooser_action]() {
                chooser_action = CHOOSER_ACTION_NEW;
            }));
            Button* cancel_button = new Button(button_w, button_h, cancel_label, Function<void()>([&chooser_action]() {
                chooser_action = CHOOSER_ACTION_CANCEL;
            }));
            if (open_button == nullptr || new_button == nullptr || cancel_button == nullptr) {
                if (open_button != nullptr) {
                    delete open_button;
                }
                if (new_button != nullptr) {
                    delete new_button;
                }
                if (cancel_button != nullptr) {
                    delete cancel_button;
                }
                delete actions;
                open_button = nullptr;
                return;
            }

            actions->addWidget(open_button, LEFT);
            actions->addWidget(new_button, LEFT);
            actions->addWidget(cancel_button, LEFT);
            chooser_widgets->addLayout(actions);
        } else {
            button_w = panel_w - 6;
            if (button_w < 24) {
                button_w = 24;
            }

            int content_h = (button_h * 3) + 8;
            int layout_y = panel_y + panel_h - content_h - 6;
            if (layout_y < panel_y + 18) {
                layout_y = panel_y + 18;
            }

            Layout* actions = new Layout(panel_x + 2, layout_y, button_w + 4, content_h, VERTICAL, LAYOUT_FLAG_NONE);
            if (actions == nullptr) {
                return;
            }

            open_button = new Button(button_w, button_h, open_label, Function<void()>([&chooser_action]() {
                chooser_action = CHOOSER_ACTION_OPEN;
            }));
            Button* new_button = new Button(button_w, button_h, new_label, Function<void()>([&chooser_action]() {
                chooser_action = CHOOSER_ACTION_NEW;
            }));
            Button* cancel_button = new Button(button_w, button_h, cancel_label, Function<void()>([&chooser_action]() {
                chooser_action = CHOOSER_ACTION_CANCEL;
            }));
            if (open_button == nullptr || new_button == nullptr || cancel_button == nullptr) {
                if (open_button != nullptr) {
                    delete open_button;
                }
                if (new_button != nullptr) {
                    delete new_button;
                }
                if (cancel_button != nullptr) {
                    delete cancel_button;
                }
                delete actions;
                open_button = nullptr;
                return;
            }

            actions->addWidget(open_button, LEFT);
            actions->addWidget(new_button, LEFT);
            actions->addWidget(cancel_button, LEFT);
            chooser_widgets->addLayout(actions);
        }
    };

    auto drawChooserPanel = [&](int panel_x, int panel_y, int panel_w, int panel_h) {
        gfx_draw_rectangle(panel_x, panel_y, panel_w, panel_h, COLOR_BG);
        gfx_draw_format_text(panel_x + 4, panel_y + 4, COLOR_TEXT, "File Browser");
        gfx_draw_format_text(panel_x + 4, panel_y + 14, COLOR_TEXT, "Pick file from tree, then Open. New File creates one.");

        char selected_label[160];
        memset(selected_label, 0, sizeof(selected_label));
        if (selected_file[0] == 0) {
            strncpy(selected_label, "(none)", (uint32_t)(sizeof(selected_label) - 1));
        } else {
            int max_chars = (panel_w - 12) / 8;
            if (max_chars < 1) {
                max_chars = 1;
            }

            int name_len = strlen(selected_file);
            if (name_len <= max_chars) {
                strncpy(selected_label, selected_file, (uint32_t)(sizeof(selected_label) - 1));
            } else if (max_chars > 3) {
                int keep = max_chars - 3;
                if (keep > (int)sizeof(selected_label) - 4) {
                    keep = (int)sizeof(selected_label) - 4;
                }

                strncpy(selected_label, selected_file, (uint32_t)keep);
                selected_label[keep] = '.';
                selected_label[keep + 1] = '.';
                selected_label[keep + 2] = '.';
                selected_label[keep + 3] = 0;
            } else {
                strncpy(selected_label, "...", (uint32_t)(sizeof(selected_label) - 1));
            }
        }

        gfx_draw_format_text(panel_x + 4, panel_y + 26, COLOR_VGA_MEDIUM_DARK_GRAY, "Selected: %s", selected_label);
    };

    reDrawHeader();
    treeView->drawTree(this);
    reDraw(0, m_bufferHead);

    while (1) {
        int panel_x = textStartX();
        int panel_y = HEADER_HEIGHT;
        int panel_w = textClipWidth();
        int panel_h = c_height - HEADER_HEIGHT - STATUS_HEIGHT;
        if (panel_h < 28) {
            panel_h = 28;
        }

        if (chooser_widgets == nullptr ||
            panel_x != cached_panel_x || panel_y != cached_panel_y ||
            panel_w != cached_panel_w || panel_h != cached_panel_h) {
            rebuildChooserWidgets(panel_x, panel_y, panel_w, panel_h);
            cached_panel_x = panel_x;
            cached_panel_y = panel_y;
            cached_panel_w = panel_w;
            cached_panel_h = panel_h;
        }

        if (open_button != nullptr) {
            if (selected_file[0] != 0) {
                open_button->enable();
            } else {
                open_button->disable();
            }
        }

        drawChooserPanel(panel_x, panel_y, panel_w, panel_h);
        if (chooser_widgets != nullptr) {
            chooser_widgets->draw(this);
        }

        struct gfx_event event;
        gfx_get_event(&event, GFX_EVENT_BLOCKING);

        switch (event.event) {
        case GFX_EVENT_KEYBOARD:
            if (event.data == KEY_F4 || event.data == 27) {
                freeChooserWidgets();
                reDrawHeader();
                reDraw(0, m_bufferHead);
                return;
            }

            if (event.data == '\n' || event.data == 'o' || event.data == 'O') {
                chooser_action = CHOOSER_ACTION_OPEN;
            }

            if (event.data == 'n' || event.data == 'N') {
                chooser_action = CHOOSER_ACTION_NEW;
            }
            break;

        case GFX_EVENT_RESOLUTION:
            applyResolution(event.data, event.data2);
            reDrawHeader();
            treeView->drawTree(this);
            reDraw(0, m_bufferHead);
            cached_panel_x = -1;
            cached_panel_y = -1;
            cached_panel_w = -1;
            cached_panel_h = -1;
            break;

        case GFX_EVENT_MOUSE:
            if (event.data < TREE_VIEW_WIDTH) {
                const char* file = treeView->click(event.data, event.data2);
                treeView->drawTree(this);
                if (file != nullptr) {
                    strncpy(selected_file, file, (uint32_t)(sizeof(selected_file) - 1));
                    selected_file[sizeof(selected_file) - 1] = 0;
                    drawStatusLine(COLOR_VGA_MEDIUM_DARK_GRAY, "Selected %s", selected_file);
                }
            } else {
                if (chooser_widgets != nullptr) {
                    chooser_widgets->Mouse(event.data, event.data2);
                }
            }
            break;

        case GFX_EVENT_EXIT:
            if (Quit()) {
                freeChooserWidgets();
                return;
            }
            break;

        default:
            break;
        }

        if (chooser_action == CHOOSER_ACTION_OPEN) {
            chooser_action = CHOOSER_ACTION_NONE;
            if (selected_file[0] == 0) {
                drawStatusLine(COLOR_VGA_RED, "Select a file from tree first.");
                continue;
            }

            if (!Open(selected_file)) {
                reDrawHeader();
                treeView->drawTree(this);
                reDraw(0, m_bufferHead);
                continue;
            }

            freeChooserWidgets();
            reDrawHeader();
            reDraw(0, m_bufferHead);
            return;
        }

        if (chooser_action == CHOOSER_ACTION_NEW) {
            chooser_action = CHOOSER_ACTION_NONE;

            char new_file[256];
            memset(new_file, 0, sizeof(new_file));
            if (showNewFileDialog(new_file, sizeof(new_file)) && new_file[0] != 0) {
                printf("[editor] chooser new: trying open %s\n", new_file);
                if (!Open(new_file)) {
                    reDrawHeader();
                    treeView->drawTree(this);
                    reDraw(0, m_bufferHead);
                    drawStatusLine(COLOR_VGA_RED, "Create failed: %s", new_file);
                    printf("[editor] chooser new: open failed %s\n", new_file);
                    continue;
                }

                freeChooserWidgets();
                reDrawHeader();
                if (treeView != nullptr) {
                    treeView->refresh();
                    treeView->drawTree(this);
                }
                reDraw(0, m_bufferHead);
                return;
            }

            drawStatusLine(COLOR_VGA_MEDIUM_DARK_GRAY, "New file creation canceled.");
            continue;
        }

        if (chooser_action == CHOOSER_ACTION_CANCEL) {
            chooser_action = CHOOSER_ACTION_NONE;
            freeChooserWidgets();
            reDrawHeader();
            reDraw(0, m_bufferHead);
            return;
        }
    }
}

void Editor::Help()
{
    reDrawHeader();
    treeView->drawTree(this);

    int x = textStartX();

    gfx_draw_rectangle(x, HEADER_HEIGHT, textClipWidth(), c_height - HEADER_HEIGHT - STATUS_HEIGHT, COLOR_BG);
    gfx_draw_format_text(x, HEADER_HEIGHT, COLOR_TEXT, "Help");
    gfx_draw_format_text(x, HEADER_HEIGHT + 8, COLOR_TEXT, "F1: Save file");
    gfx_draw_format_text(x, HEADER_HEIGHT + 16, COLOR_TEXT, "F4: Open/create file");
    gfx_draw_format_text(x, HEADER_HEIGHT + 24, COLOR_TEXT, "F2/F3: Scroll up/down");
    gfx_draw_format_text(x, HEADER_HEIGHT + 32, COLOR_TEXT, "F9 or Esc: Close help");

    gfx_draw_format_text(x, HEADER_HEIGHT + 48, COLOR_TEXT, "Arrow keys: Move cursor");
    gfx_draw_format_text(x, HEADER_HEIGHT + 56, COLOR_TEXT, "Backspace: Delete character");
    gfx_draw_format_text(x, HEADER_HEIGHT + 64, COLOR_TEXT, "Enter: New line");
    gfx_draw_format_text(x, HEADER_HEIGHT + 72, COLOR_TEXT, "Tab: Insert spaces");
    gfx_draw_format_text(x, HEADER_HEIGHT + 80, COLOR_TEXT, "Mouse: Click file in tree to open");

    while (1) {
        struct gfx_event event;
        gfx_get_event(&event, GFX_EVENT_BLOCKING);

        switch (event.event) {
        case GFX_EVENT_KEYBOARD:
            if (event.data == 'q' || event.data == KEY_F9 || event.data == 27) {
                return;
            }
            break;

        case GFX_EVENT_MOUSE:
            if (event.data < TREE_VIEW_WIDTH) {
                treeView->click(event.data, event.data2);
                treeView->drawTree(this);
            }
            break;

        case GFX_EVENT_RESOLUTION:
            applyResolution(event.data, event.data2);
            reDrawHeader();
            treeView->drawTree(this);

            x = textStartX();
            gfx_draw_rectangle(x, HEADER_HEIGHT, textClipWidth(), c_height - HEADER_HEIGHT - STATUS_HEIGHT, COLOR_BG);
            gfx_draw_format_text(x, HEADER_HEIGHT, COLOR_TEXT, "Help");
            gfx_draw_format_text(x, HEADER_HEIGHT + 8, COLOR_TEXT, "F1: Save file");
            gfx_draw_format_text(x, HEADER_HEIGHT + 16, COLOR_TEXT, "F4: Open/create file");
            gfx_draw_format_text(x, HEADER_HEIGHT + 24, COLOR_TEXT, "F2/F3: Scroll up/down");
            gfx_draw_format_text(x, HEADER_HEIGHT + 32, COLOR_TEXT, "F9 or Esc: Close help");

            gfx_draw_format_text(x, HEADER_HEIGHT + 48, COLOR_TEXT, "Arrow keys: Move cursor");
            gfx_draw_format_text(x, HEADER_HEIGHT + 56, COLOR_TEXT, "Backspace: Delete character");
            gfx_draw_format_text(x, HEADER_HEIGHT + 64, COLOR_TEXT, "Enter: New line");
            gfx_draw_format_text(x, HEADER_HEIGHT + 72, COLOR_TEXT, "Tab: Insert spaces");
            gfx_draw_format_text(x, HEADER_HEIGHT + 80, COLOR_TEXT, "Mouse: Click file in tree to open");
            break;

        case GFX_EVENT_EXIT:
            if (Quit()) {
                return;
            }
            break;

        default:
            break;
        }
    }
}

void Editor::EditorLoop()
{
    reDrawHeader();
    treeView->drawTree(this);
    reDraw(0, m_bufferHead);

    while (1) {
        struct gfx_event event;
        gfx_get_event(&event, GFX_EVENT_BLOCKING);

        switch (event.event) {
        case GFX_EVENT_KEYBOARD:
            putChar(event.data);
            break;

        case GFX_EVENT_MOUSE:
            if (event.data < TREE_VIEW_WIDTH) {
                const char* file = treeView->click(event.data, event.data2);
                treeView->drawTree(this);
                if (file != nullptr) {
                    Open((char*)file);
                }
                reDrawHeader();
                reDraw(0, m_bufferHead);
            }
            break;

        case GFX_EVENT_RESOLUTION:
            applyResolution(event.data, event.data2);
            reDrawHeader();
            treeView->drawTree(this);
            reDraw(0, m_bufferHead);
            break;

        case GFX_EVENT_EXIT:
            if (Quit()) {
                return;
            }
            break;

        default:
            break;
        }
    }
}

void Editor::putChar(unsigned char c)
{
    if (c == '\r') {
        c = '\n';
    }

    bool buffer_changed = false;

    switch (c) {
    case '\b':
        if (m_bufferEdit <= 0 || m_bufferHead <= 0) {
            return;
        }

        if (ropeDeleteChar(m_bufferEdit - 1)) {
            m_bufferEdit--;
            m_dirty = 1;
            m_preferredColumn = -1;
            buffer_changed = true;
        }
        break;

    case KEY_LEFT:
        if (m_bufferEdit > 0) {
            m_bufferEdit--;
        }
        m_preferredColumn = -1;
        break;

    case KEY_RIGHT:
        if (m_bufferEdit < m_bufferHead) {
            m_bufferEdit++;
        }
        m_preferredColumn = -1;
        break;

    case KEY_DOWN: {
        int currentStart = lineStart(m_bufferEdit);
        int currentCol = m_bufferEdit - currentStart;

        if (m_preferredColumn < 0) {
            m_preferredColumn = currentCol;
        }

        int currentEnd = lineEnd(currentStart);
        if (currentEnd >= m_bufferHead || m_textBuffer[currentEnd] != '\n') {
            break;
        }

        int nextStart = currentEnd + 1;
        int nextEnd = lineEnd(nextStart);
        int nextLen = nextEnd - nextStart;

        m_bufferEdit = nextStart + minInt(m_preferredColumn, nextLen);
        break;
    }

    case KEY_UP: {
        int currentStart = lineStart(m_bufferEdit);
        int currentCol = m_bufferEdit - currentStart;

        if (m_preferredColumn < 0) {
            m_preferredColumn = currentCol;
        }

        if (currentStart == 0) {
            break;
        }

        int prevEnd = currentStart - 1;
        int prevStart = lineStart(prevEnd);
        int prevLen = prevEnd - prevStart;

        m_bufferEdit = prevStart + minInt(m_preferredColumn, prevLen);
        break;
    }

    case KEY_F3:
        scroll(1);
        reDraw(0, m_bufferHead);
        return;

    case KEY_F2:
        scroll(-1);
        reDraw(0, m_bufferHead);
        return;

    case KEY_F9:
        m_preferredColumn = -1;
        Help();
        break;

    case KEY_F4:
        m_preferredColumn = -1;
        FileChooser();
        break;

    case KEY_F1:
        m_preferredColumn = -1;
        Save();
        break;

    default:
        if (c == '\t') {
            for (int n = 0; n < 4; n++) {
                if (!ropeInsertChar(m_bufferEdit, ' ')) {
                    break;
                }
                m_bufferEdit++;
                buffer_changed = true;
            }
            if (buffer_changed) {
                m_dirty = 1;
                m_preferredColumn = -1;
            }
            break;
        }

        if (c == 0) {
            return;
        }

        if (c != '\n' && (c < 32 || c > 126)) {
            return;
        }

        if (!ropeInsertChar(m_bufferEdit, c)) {
            return;
        }

        m_bufferEdit++;
        buffer_changed = true;

        if (c == '{' && m_bufferHead < m_bufferSize - 1) {
            ropeInsertChar(m_bufferEdit, '}');
            buffer_changed = true;
        }

        m_dirty = 1;
        m_preferredColumn = -1;
        break;
    }

    if (m_bufferEdit < 0) {
        m_bufferEdit = 0;
    }

    if (buffer_changed) {
        ropeSyncToBuffer();
    }

    if (m_bufferEdit > m_bufferHead) {
        m_bufferEdit = m_bufferHead;
    }

    ensureCursorVisible(visibleColumns(), visibleLines());
    reDraw(0, m_bufferHead);
}

extern "C" int main(int argc, char* argv[])
{
    Editor* editor = new Editor();

    if (argc > 1) {
        editor->Open(argv[1]);
    } else {
        editor->FileChooser();
    }

    editor->EditorLoop();
    return 0;
}
