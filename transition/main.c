#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "raylib.h"
#include "raymath.h"
void ret(void); // visual mark
void data(void);
void branch(void);
void branch_static(void);
#include "start_ins_statements.h"
#include "libs/Runtime-Define-Package.h"
#include "libs/Runtime-lib-statements-raylib.h"
#include "libs/Runtime-lib-statements-stdio.h"
#include "libs/Runtime-lib-statements-stdlib.h"
#include "libs/Runtime-lib-statements-string.h"
#include "libs/Runtime-lib-statements-ctype.h"
#include "libs/Runtime-lib-statements-time.h"
#include "libs/Runtime-lib-list-raylib.h"
#include "libs/Runtime-lib-list-stdio.h"
#include "libs/Runtime-lib-list-stdlib.h"
#include "libs/Runtime-lib-list-string.h"
#include "libs/Runtime-lib-list-ctype.h"
#include "libs/Runtime-lib-list-time.h"
#include "libs/lists.h"
FILE *file;
long file_size;
void (*imp)();
void *ptr;
#define base_ins ret, data, branch, branch_static
#define start_ins to_dest_dev_base, std_start, std_end, fun_start, fun_end, if_, else_, not_else, get_else, while_, for_, loop, break_, var, var_auto, init_int, ptr_, int_, float_, char_, color_, size_t_, vector2_, camera2d_, space, space_int, space_vector2, space_ptr, data_str_int, data_str_char, sfloat, data_to_size_t_, get_str, null, flag_window_resizable, getptr, getstd, getfile, base_, up, get_file_size, get_strc, sizeof_int, sizeof_void, mul, deref_int, i_, type_char, plus_ptr, minus_ptr, minus_ptr_ptr, plus_plus_ptr, ptr_assign, at, at_int, array_access, ex_ptr_arr, char_arr, ex_char_arr, int_arr, ex_int_arr, vec2_arr, ex_vec2_arr, vector2_x, vector2_y, vector2_x_ptr, vector2_y_ptr, camera2d_offset, camera2d_target, camera2d_rotation, camera2d_zoom, camera2d_offset_ptr, camera2d_target_ptr, camera2d_rotation_ptr, camera2d_zoom_ptr, plus, minus, x_, divide, increment, decrement, not_increment, plus_float, xfloat, divide_float, equal, not_equal, not_equal_char, greater, less, greater_equal, less_equal, equal_ptr, greater_ptr, less_ptr, and_and, or_or, not_, assign, assign_int, assign_char, assign_float, assign_vec2, plus_assign, minus_assign, plus_assign_ptr, minus_assign_ptr, plus_assign_float, int_from_float, float_to_int, int_to_size_t, size_t_to_int, int_to_char, itoa_, strcb, ptr_to_ascii, ins_remove_underscores, color_black, color_white, color_red, color_green, color_blue, color_yellow, color_gold, color_purple, color_skyblue, color_darkgray, color_lightgray, init_start, break_point_std, dbg_point, Vector2Subtract_, Vector2Scale_, color_lime, assign_color, input_, color_gray, out, rerun, std_start_, std_end_, Vector2Add_, greater_equal_float, less_equal_float, var_end, hash_vector2_y, char_to_int, get_stack, get_stack_base, greater_equal_ptr, ptr_to_unsigned_char, filepathlist, filepathlist_paths, assign_filepathlist, long_, long_to_int, run_once
void (*table[])(void) = {base_ins, raylib_, stdio_, stdlib_, string_, ctype_, time_, start_ins};
void *stack = NULL;
int runonece = 0;
void *base;
void *point;
char input_str[256];
int is_fun = 0;
#define base_ins_name "ret", "data", "branch", "branch_static"
#define start_ins_name "to_dest_dev_base", "stdstart", "stdend", ">>", "<<", "if", "else", "!else", "get_else", "while", "for", "loop", "break", "var", "var_auto", "#int", "p", "int", "float", "char", "color", "size_t", "vector2", "camera2d", "space", "sp_int", "sp_vec2", "sp_ptr", "\\int", "\\char", "\\float", "\\size_t", "get_str", "null", "FLAG_WINDOW_RESIZABLE", "ptr", "std", "get_file", "base", "up", "file_size", "get_strc", "sizeof_int", "sizeof_void", "*", "*i", "i", "c", "+p", "-p", "-pp", "++p", "*=", "@", "@int", "*[", "[", "[char", "*[char", "[int", "*[int", "[vec2", "*[vec2", "vec_x", "vec_y", "vec_x_ptr", "vec_y_ptr", "cam_offset", "cam_target", "cam_rotation", "cam_zoom", "cam_offset_ptr", "cam_target_ptr", "cam_rotation_ptr", "cam_zoom_ptr", "+", "-", "x", "/", "++", "--", "+++", "+f", "xf", "/float", "==", "!=", "!=c", ">", "<", ">=", "<=", "==p", ">p", "<p", "&&", "||", "!", "=", "=i", "=c", "=f", "=vec2", "+=", "-=", "+=p", "-=p", "+=f", "i-f", "f->i", "i-size_t", "size_t->i", "i-?", "itoa", "strcb", "ptoa", "remove_underscores", "BLACK", "WHITE", "RED", "GREEN", "BLUE", "YELLOW", "GOLD", "PURPLE", "SKYBLUE", "DARKGRAY", "LIGHTGRAY", "#init", "#break", "?", "vector2_subtract", "vector2_scale", "LIME", "=color", "input", "GRAY", "out", "rerun", ">>>", "<<<", "vector2_add", ">=f", "<=f", "<<<<", "#vec_y", "c-i", "get_stack", "get_stack_base", ">=p", "p-?", "filepathlist", "filepath_paths", "=fpl", "\\long", "l-i", "onece"
char *str[] = {base_ins_name, raylib_list, stdio_list, stdlib_list, string_list, ctype_list, time_list, start_ins_name};
int strc = sizeof(str) / sizeof(str[0]);
void *funcs[4096];
int fun_max = 0;
#define new_data_size 32
#define block_size 16384
int index_num = 0;
#include "start_ins_num.h"
void *copy;
void *view;
int ins;
int is_point;
int bracket = 0;
Color drawcolor;
char *txt;
void *std_stack[1024];
int std_stack_index = 0;
void *std;
void *std_base = NULL;
void *stack_base = NULL;
int debug_step = 0;
int dbgs[256] = {0};
char *completion;
char remove_underscores_buff[512];
#define span 8
int next_is_fun_ins = 0;
Vector2 pos_back[256];
Vector2 line_pos;
Vector2 draw_pos;
Vector2 pos;
Camera2D camera;
void *views[256];
int view_index = 0;
Vector2 views_pos[256];
Vector2 mouseWorldPos;
int draggingIndex = -1;
size_t view_index_current;
#define tab_space 40
void *var_ip;
void *var_address[2048];
int var_size[2048];
void *var_buff_offset;
int var_index;
int var_index_stack[256];
void *var_buff_offset_stack[256];
int var_stack_index = 0;
void *var_buff;
Vector2 func_pos[256];
int end_y[256];
void *break_stack_stack[256];
int break_stack_index = 0;
void *fixed_point;
int switch_buff = 0;
int next_line_y;
int toggle_debug = 0;
int dbg_bracket = 0;
int last_bracket = 0;
int dbg_level[256];
void *repeat[2048];
void *repeat_out_offset[2048];
int repeat_index = 0;
void *out_put;
void *re_func(void *tmp)
{
    if (*(int *)tmp == 3)
    {
        *(int *)tmp = 2;
        return *(void **)(tmp + sizeof(int)) = tmp + *(int *)(tmp + sizeof(int));
    }
    return *(void **)(tmp + sizeof(int));
}
void debug(void)
{
    debug_step++;
    if (debug_step == 6261600)
    {
        debug_step = debug_step;
    }
    int stack_p = (stack - stack_base) / 8;
    int ins = *(int *)ptr;
    void *data_ptr = ptr + sizeof(int) * 2;
    if (ins == 1)
    {
        if (strcmp((char *)data_ptr, "get_file_buffer") == 0)
        {
            // toggle_debug = 1;
        }
        else if (strcmp((char *)data_ptr, "ismemmove") == 0)
        {
            debug_step = debug_step;
        }
    }
    if (toggle_debug)
    {
        if (ins == ins_std_start)
        {
            dbg_bracket++;
        }
        else if (ins == ins_std_end)
        {
            dbg_bracket--;
        }
        else if (ins == 2 || ins == 3)
        {
            dbg_level[stack_p] = dbg_bracket;
            last_bracket = dbg_bracket = 0;
            printf("\033[32m#\033[0m ");
        }
        else if (ins == 1)
        {
            printf("\033[32m%s\033[0m ", (char *)data_ptr);
        }
        if (ins != ins_std_start && ins != ins_std_end && ins != 1 && ins != 2 && ins != 3 && ins != 0)
        {
            if (last_bracket == 0)
            {
                printf("\n%d", debug_step);
                for (size_t i = 0; i < stack_p; i++)
                {
                    printf("[%s]", *(char **)(*(void **)(stack_base + i * 8) + 4) + 8);
                }
                printf("%d %d ", std - std_base, std_stack_index);
            }
            last_bracket = dbg_bracket;
            printf("%s ", str[ins]);
        }
        if (ins == 0)
        {
            dbg_bracket = dbg_level[stack_p - 1];
        }
    }
}
int main()
{
    file = fopen("#", "rb");
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    ptr = malloc(file_size);
    fread(ptr, file_size, 1, file);
    imp = table[*(int *)(ptr)];
    while (1)
    {
        debug();
        imp();
    }
}
#include "libs/Runtime-lib-definition-raylib.h"
#include "libs/Runtime-lib-definition-stdio.h"
#include "libs/Runtime-lib-definition-stdlib.h"
#include "libs/Runtime-lib-definition-string.h"
#include "libs/Runtime-lib-definition-ctype.h"
#include "libs/Runtime-lib-definition-time.h"
void ret(void)
{
    imp = table[*(int *)(ptr = (*(void **)(stack -= sizeof(void *)) + sizeof(int) + sizeof(void *)))];
}
void data(void)
{
    imp = table[*(int *)(ptr += *(int *)(ptr + sizeof(int)))];
}
void branch(void)
{
    *(void **)stack = ptr;
    stack += sizeof(void *);
    imp = table[*(int *)(ptr = *(void **)(ptr + sizeof(int)))];
}
void branch_static(void)
{
    *(int *)ptr = 2;
    *(void **)(ptr + sizeof(int)) = ptr + *(int *)(ptr + sizeof(int));
    branch();
}
void *check(void *view_ins_ptr)
{
    int ins = *(int *)(view_ins_ptr);
    if (ins == 1)
    {
        view_ins_ptr += *(int *)(view_ins_ptr + sizeof(int));
    }
    else if (ins == 2 || ins == 3)
    {
        view_ins_ptr += sizeof(int) + sizeof(void *);
    }
    else
    {
        view_ins_ptr += sizeof(int);
    }
    return view_ins_ptr;
}
void change_ret(int size)
{
    int ins;
    void *tmp = point;
    while (0) // (ins = *(int *)tmp)
    {
        if (ins == 2)
        {
            for (void *tmp_stack = stack - sizeof(void *); tmp_stack >= stack_base; tmp_stack -= sizeof(void *))
            {
                if (*(void **)tmp_stack == tmp)
                {
                    *(void **)tmp_stack += size;
                }
            }
        }
        tmp = check(tmp);
    }
}
void insert_ins(int ins)
{
    memmove(point + sizeof(int), point, block_size / 2);
    *(int *)point = ins;
    point += sizeof(int);
    change_ret(sizeof(int));
}
void *remove_underscores(char *str)
{
    void *dst_buff = remove_underscores_buff + switch_buff * 256;
    char *dst = dst_buff;
    switch_buff = !switch_buff;
    while (*str)
    {
        if (*str != '_')
        {
            *dst = *str;
            dst++;
        }
        str++;
    }
    *dst = '\0';
    return dst_buff;
}
void *is_repeat(void *tmp)
{
    for (size_t i = 0; i < repeat_index; i++)
    {
        if (tmp == repeat[i])
        {
            return repeat_out_offset[i];
        }
    }
    return NULL;
}
int is_repeat_check(void *tmp)
{
    for (size_t i = 0; i < repeat_index; i++)
    {
        if (tmp == repeat[i])
        {
            return 1;
        }
    }
    repeat[repeat_index++] = tmp;
    return 0;
}
int find_index(void **index_str, int index_strc, int offset)
{
    int tmp = 0;
    for (size_t i = 0; i < index_strc; i++)
    {
        void *str = index_str[i] + offset;
        void *index_buff = remove_underscores(str);
        void *input_buff = remove_underscores(input_str);
        if (strstr(index_buff, input_buff) == index_buff)
        {
            completion = str;
            index_num = i;
            tmp = -1;
        }
        if (strcmp(index_buff, input_buff) == 0)
        {
            completion = str;
            index_num = i;
            return 1;
        }
    }
    return tmp;
}
void insert_bracket(void)
{
    insert_ins(ins_std_start);
    insert_ins(ins_std_end);
}
void check_view(void)
{
    view = check(view);
}
void input(char *input_str)
{
    if (IsKeyPressed(KEY_BACKSPACE))
    {
        input_str[strlen(input_str) - 1] = '\0';
    }
    int key = GetCharPressed();
    if (key)
    {
        strncat(input_str, (char *)&key, 1);
    }
}
void set_mouse_pos_next(int offset_x, int offset_y)
{
    SetMousePosition(GetMouseX() + offset_x, GetWorldToScreen2D((Vector2){line_pos.x, offset_y}, camera).y + 2);
}
void clean_input_str(void)
{
    input_str[0] = '\0';
    GetCharPressed();
}
void key_end(void)
{
    clean_input_str();
    set_mouse_pos_next(0, line_pos.y + 20);
}
void if_input_text(void)
{
    if (is_point)
    {
        input(txt);
    }
}
void var_struct(void)
{
    if (*(int *)check(view) == 2)
    {
        txt = *(void **)(view + sizeof(int) * 2) + sizeof(int) * 2;
        if_input_text();
        check_view();
    }
}
void insert_branch(void *tmp)
{
    insert_ins(2);
    memmove(point + sizeof(void *), point, block_size / 2);
    *(void **)(point) = tmp;
    change_ret(sizeof(void *));
}
void *get_file_buffer(char *file_path)
{
    FILE *file = fopen(file_path, "rb");
    fseek(file, 0, SEEK_END);
    int file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    void *file_buff = malloc(file_size);
    fread(file_buff, file_size, 1, file);
    fclose(file);
    return file_buff;
}
int find_func(void *tmp)
{
    for (size_t i = 0; i < fun_max; i++)
    {
        if (funcs[i] == tmp || memcmp(funcs[i], tmp, block_size) == 0)
        {
            return i;
        }
    }
    return -1;
}
void add_branch_list(void *tmp)
{
    if (find_func(tmp) == -1)
    {
        funcs[fun_max++] = tmp;
    }
}
void *address_allocation(void *tmp)
{
    void *tmp2 = tmp;
    void *tmp3 = is_repeat(tmp);
    if (tmp3 == NULL)
    {
        repeat[repeat_index] = tmp;
        tmp3 = repeat_out_offset[repeat_index++] = malloc(block_size);
        int ins;
        while (ins = *(int *)tmp)
        {
            void *tmp4 = NULL;
            if (ins == 3)
            {
                *(int *)tmp = 2;
                tmp4 = tmp + *(int *)(tmp + sizeof(int));
            }
            else if (ins == 2)
            {
                tmp4 = *(void **)(tmp + sizeof(int));
            }
            if (tmp4)
            {
                add_branch_list(*(void **)(tmp + sizeof(int)) = address_allocation(tmp4));
            }
            tmp = check(tmp);
        }
        memcpy(tmp3, tmp2, block_size);
    }
    return tmp3;
}
void insert_auto(void)
{
    if (is_fun)
    {
        insert_branch(funcs[index_num]);
    }
    else
    {
        insert_ins(index_num);
    }
}
int get_block_size(void *tmp)
{
    void *tmp2 = tmp;
    while (*(int *)tmp)
    {
        tmp = check(tmp);
    }
    return tmp - tmp2 + sizeof(int);
}
void *save(void *tmp)
{
    void *tmp2 = is_repeat(tmp);
    if (tmp2 == NULL)
    {
        repeat[repeat_index] = tmp;
        int size = get_block_size(tmp);
        memcpy(out_put, tmp, size);
        repeat_out_offset[repeat_index++] = tmp2 = tmp = out_put;
        out_put += size;
        while ((ins = *(int *)tmp))
        {
            if (ins == 2)
            {
                *(int *)tmp = 3;
                *(int *)(tmp + sizeof(int)) = save(*(void **)(tmp + sizeof(int))) - tmp;
            }
            tmp = check(tmp);
        }
    }
    return tmp2;
}
void draw_view(void)
{
    while (1)
    {
        ins = *(int *)(view);
        if (mouseWorldPos.y >= pos.y && mouseWorldPos.y <= end_y[view_index_current] && mouseWorldPos.x >= pos.x)
        {
            point = view;
        }
        is_point = fixed_point == view;
        if (is_point)
        {
            line_pos = pos;
            if (IsFileDropped())
            {
                FilePathList droppedFiles = LoadDroppedFiles();
                add_branch_list(address_allocation(get_file_buffer(droppedFiles.paths[0])));
                UnloadDroppedFiles(droppedFiles);
            }
            if (IsKeyPressed(KEY_HOME))
            {
                view_index = view_index_current + 1;
            }
            if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S))
            {
                repeat_index = 0;
                void *out_put_base = out_put = malloc(1024 * 1024);
                void *tmp = views[view_index_current];
                save(tmp);
                FILE *output_file = fopen(view_index_current ? tmp + sizeof(int) * 2 : "#", "wb");
                fwrite(out_put_base, 1, out_put - out_put_base, output_file);
                fclose(output_file);
                free(out_put_base);
            }
        }
        if (next_line_y == 0 && view > fixed_point)
        {
            next_line_y = pos.y;
        }
        if (ins == ins_std_start)
        {
            pos_back[bracket] = pos;
            pos.y += 20;
            pos.x += tab_space;
            bracket++;
        }
        else if (ins == ins_std_end)
        {
            bracket--;
            pos.x = pos_back[bracket].x;
            pos.y += span;
            next_is_fun_ins = 1;
        }
        else
        {
            drawcolor = WHITE;
            void *tmp = *(void **)(view + sizeof(int));
            txt = str[ins];
            if (ins == 1)
            {
                txt = view + sizeof(int) * 2;
                if_input_text();
                drawcolor = GREEN;
            }
            else if (ins == 2)
            {
                txt = tmp + sizeof(int) * 2;
                if_input_text();
                func_pos[find_func(tmp)] = (Vector2){pos.x + MeasureText(txt, 20), pos.y + 10};
                add_branch_list(tmp);
                drawcolor = YELLOW;
            }
            else if (ins == ins_var_auto)
            {
                var_struct();
                drawcolor = LIGHTGRAY;
            }
            else if (ins == ins_var)
            {
                var_struct();
                drawcolor = GRAY;
            }
            else if (ins == ins_data_str_int)
            {
                txt = view + sizeof(int) * 3;
                if_input_text();
                check_view();
                drawcolor = SKYBLUE;
            }
            draw_pos = pos;
            if (next_is_fun_ins)
            {
                next_is_fun_ins = 0;
                draw_pos = pos_back[bracket];
            }
            else
            {
                pos.y += 20;
            }
            if (ins == 0)
            {
                end_y[view_index_current] = pos.y;
                return;
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && CheckCollisionPointRec(mouseWorldPos, (Rectangle){draw_pos.x, draw_pos.y, MeasureText(txt, 20), 20}))
            {
                draggingIndex = view_index_current;
                if (ins == 2)
                {
                    views[view_index] = tmp;
                    views_pos[view_index] = mouseWorldPos;
                    draggingIndex = view_index;
                    view_index++;
                }
            }
            DrawText(txt, draw_pos.x, draw_pos.y, 20, drawcolor);
        }
        check_view();
    }
}
Vector2 MouseDelta_zoom(void)
{
    return Vector2Scale(GetMouseDelta(), 1.0f / camera.zoom);
}
void insert_data(void)
{
    void *tmp = point + sizeof(int) * 2;
    memmove(tmp + new_data_size, point, block_size / 2);
    *(int *)(point) = 1;
    *(int *)(point + sizeof(int)) = new_data_size + sizeof(int) + sizeof(int);
    strcpy(tmp, input_str);
    change_ret(new_data_size + sizeof(int) + sizeof(int));
}
int ins_src;
int ins_dest;
void map_check_all(void *tmp)
{
    if (!is_repeat_check(tmp))
    {
        int ins;
        while (ins = *(int *)tmp)
        {
            if (ins > ins_src && ins < ins_src_range)
            {
                *(int *)tmp = ins_dest + (ins - ins_src);
            }
            if (ins == 2)
            {
                map_check_all(*(void **)(tmp + sizeof(int)));
            }
            tmp = check(tmp);
        }
    }
}
char *lib_names;
int *lib_maps;
int lib_size;
int find_lib_map(char *name)
{
    for (size_t i = 0; i < lib_size; i++)
    {
        if (strcmp(name, lib_names + i * 32) == 0)
        {
            return i;
        }
    }
    return add_lib(name);
}
int base_table_size;
int ins_src_range;
void block_lib_maps(void *block_ptr, void *names, int size)
{
    int tmp_table_size = base_table_size;
    for (size_t i = 0; i < size; i++)
    {
        int index = find_lib_map(names + i * 32);
        ins_src = tmp_table_size;
        ins_dest = lib_maps[index];
        ins_src_range = tmp_table_size += (((index + 1 == lib_size) ? get_table_size() : lib_maps[index + 1]) - ins_dest);
        repeat_index = 0;
        map_check_all(block_ptr);
    }
}
void to_dest_dev_base(void)
{
    if (!runonece)
    {
        fclose(file);
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
        funcs[fun_max++] = base = ptr;
        repeat_index = 0;
        void *mirror_buff = malloc(file_size);
        memcpy(mirror_buff, base, file_size);
        free(address_allocation(mirror_buff));
        memcpy(base, mirror_buff, file_size);
        free(mirror_buff);
        InitWindow(640, 480, "SelfEdit");
        camera.zoom = 1.0f;
        views[view_index] = base;
        views_pos[view_index++] = (Vector2){0.0f, 0.0f};
        runonece = 1;
    }
    BeginDrawing();
    ClearBackground(BLACK);
    BeginMode2D(camera);
    is_fun = 0;
    if (strlen(input_str) > 0)
    {
        if (isdigit(*(char *)input_str))
        {
            insert_ins(ins_data_str_int);
            insert_data();
            clean_input_str();
        }
        if (find_index((void **)str, strc, 0) <= 0)
        {
            is_fun = find_index(funcs, fun_max, sizeof(int) * 2);
        }
    }
    if (IsKeyPressed(KEY_SPACE))
    {
        insert_auto();
        key_end();
    }
    if (IsKeyPressed(KEY_TAB))
    {
        insert_bracket();
        insert_auto();
        set_mouse_pos_next(40, 0);
        key_end();
    }
    if (IsKeyPressed(KEY_RIGHT_ALT))
    {
        if (is_fun == 1)
        {
            insert_auto();
        }
        else
        {
            insert_ins(2);
            memmove(point + sizeof(void *), point, block_size / 2);
            void *buffer = *(void **)(point) = malloc(block_size);
            *(int *)buffer = 1;
            *(int *)(buffer + sizeof(int)) = new_data_size + sizeof(int) + sizeof(int);
            strcpy(buffer + sizeof(int) * 2, strlen(input_str) ? input_str : TextFormat("b%d", fun_max));
            *(int *)(buffer + new_data_size + sizeof(int) * 2) = 0;
            change_ret(sizeof(void *));
        }
        key_end();
    }
    if (IsKeyPressed(KEY_LEFT_ALT))
    {
        if (IsKeyDown(KEY_LEFT_SHIFT))
        {
            insert_ins(ins_var);
        }
        else
        {
            insert_ins(ins_var_auto);
        }
        insert_auto();
        key_end();
    }
    if (IsKeyPressed(KEY_GRAVE))
    {
        clean_input_str();
        insert_data();
    }
    if (IsKeyPressed(KEY_DELETE))
    {
        copy = point;
    }
    if (IsKeyReleased(KEY_DELETE))
    {
        memmove(copy, point, block_size / 2);
        change_ret(point - copy);
    }
    static void *copy2[2];
    if (IsKeyPressed(KEY_LEFT_SHIFT))
    {
        copy2[0] = point;
    }
    if (IsKeyReleased(KEY_LEFT_SHIFT))
    {
        copy2[1] = point;
    }
    if (IsKeyPressed(KEY_INSERT))
    {
        int size = copy2[1] - copy2[0];
        void *tmp = malloc(size);
        memcpy(tmp, copy2[0], size);
        memmove(point + size, point, block_size / 2);
        memcpy(point, copy2[0], size);
        free(tmp);
    }
    if (IsKeyPressed(KEY_ENTER))
    {
        set_mouse_pos_next(0, next_line_y);
    }
    if (IsKeyPressed(KEY_KP_ENTER))
    {
        strcpy(input_str, remove_underscores(completion));
    }
    if (WindowShouldClose())
    {
        exit(0);
    }
    float wheel = GetMouseWheelMove();
    if (wheel != 0)
    {
        camera.zoom += wheel * (0.1f * camera.zoom);
    }
    camera.offset = (Vector2){GetScreenWidth() / 2, GetScreenHeight() / 2};
    mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
    {
        camera.target = Vector2Subtract(camera.target, MouseDelta_zoom());
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    {
        views_pos[draggingIndex] = Vector2Add(views_pos[draggingIndex], MouseDelta_zoom());
    }
    next_line_y = 0;
    for (view_index_current = 0; view_index_current < view_index; view_index_current++)
    {
        view = views[view_index_current];
        pos = views_pos[view_index_current];
        DrawLineV(func_pos[find_func(views[view_index_current])], pos, LIME);
        draw_view();
    }
    input(input_str);
    fixed_point = point;
    DrawLine(line_pos.x, line_pos.y, mouseWorldPos.x, line_pos.y, GRAY);
    EndMode2D();
    void *tmp = (void *)TextFormat("%s %s", input_str, completion);
    DrawText(tmp, GetMouseX() + 20, GetMouseY(), 20, WHITE);
    imp = table[*(int *)(ptr = base)];
    EndDrawing();
}
void next_ins(void)
{
    imp = table[*(int *)(ptr += sizeof(int))];
}
#define next_sizeof(size) \
    std += sizeof(size);  \
    next_ins();
#define next_size(size) \
    std += size;        \
    next_ins();
int find_var_index(void *tmp)
{
    int tmp_var_index = var_index - 1;
    while (tmp_var_index >= 0)
    {
        if (!strcmp(var_ip + tmp_var_index * 32, tmp + sizeof(int) * 2))
        {
            return tmp_var_index;
        }
        tmp_var_index--;
    }
}
void *global_var(void *tmp)
{
    return var_address[find_var_index(tmp)];
}
void *local_var(void *tmp, int size)
{
    int tmp_var_index = var_index - 1;
    while (tmp_var_index >= (var_stack_index ? var_index_stack[var_stack_index - 1] : 0))
    {
        if (!strcmp(var_ip + tmp_var_index * 32, tmp + sizeof(int) * 2))
        {
            return var_address[tmp_var_index];
        }
        tmp_var_index--;
    }
    void *tmp2 = var_address[var_index] = var_buff_offset;
    var_buff_offset += size;
    var_size[var_index] = size;
    strcpy(var_ip + var_index * 32, tmp + sizeof(int) * 2);
    var_index++;
    return tmp2;
}
void run_block(void *tmp)
{
    // need not implement
}
void just_std_start(void)
{
    std_stack[std_stack_index++] = std;
}
void just_std_end(void)
{
    std = std_stack[--std_stack_index];
}
void just_fun_start(void)
{
    break_stack_stack[break_stack_index++] = stack;
    var_index_stack[var_stack_index] = var_index;
    var_buff_offset_stack[var_stack_index] = var_buff_offset;
    var_stack_index++;
    just_std_start();
}
void just_fun_end(void)
{
    break_stack_index--;
    var_stack_index--;
    var_index = var_index_stack[var_stack_index];
    var_buff_offset = var_buff_offset_stack[var_stack_index];
    just_std_end();
}
// ================================ Core control flow ================================
void std_start(void) //"stdstart"
{
    just_std_start();
    next_ins();
}
void std_end(void) //"stdend"
{
    just_std_end();
    next_ins();
}
void fun_start(void) //">>"
{
    just_fun_start();
    next_ins();
}
void fun_end(void) //"<<"
{
    just_fun_end();
    next_ins();
}
void if_(void) //"if"
{
    if (*(unsigned char *)std)
    {
        next_ins();
    }
    else
    {
        imp = table[*(int *)(ptr += sizeof(int) * 2 + sizeof(void *))];
    }
}
unsigned char is_else = 1;
void else_(void) //"else"
{
    if (is_else)
    {
        next_ins();
    }
    else
    {
        imp = table[*(int *)(ptr += sizeof(int) * 2 + sizeof(void *))];
    }
    is_else = 1;
}
void not_else(void) //"!else"
{
    is_else = 0;
    next_ins();
}
void get_else(void) //"get_else"
{
    *(unsigned char *)std = is_else;
    next_ins();
}
void while_(void) //"while"
{
    void *tmp_ptr = ptr;
    just_fun_start();
    void *tmp_stack = stack;
    void *condition = ptr += sizeof(int);
    void *body = ptr + sizeof(int) + sizeof(void *);
    while (stack >= tmp_stack)
    {
        just_std_start();
        run_block(condition);
        just_std_end();
        if (*(unsigned char *)std == 0)
        {
            break;
        }
        run_block(body);
    }
    just_fun_end();
    ptr = tmp_ptr;
    next_size(sizeof(int) + (sizeof(void *) + sizeof(int)) * 2);
}
void for_(void) //"for"
{
    void *tmp_ptr = ptr;
    just_fun_start();
    void *tmp_stack = stack;
    void *run_onece = ptr += sizeof(int);
    void *condition = ptr += sizeof(int) + sizeof(void *);
    void *body = ptr + sizeof(int) + sizeof(void *);
    run_block(run_onece);
    while (stack >= tmp_stack)
    {
        just_std_start();
        run_block(condition);
        just_std_end();
        if (*(unsigned char *)std == 0)
        {
            break;
        }
        run_block(body);
    }
    just_fun_end();
    ptr = tmp_ptr;
    next_size(sizeof(int) + (sizeof(void *) + sizeof(int)) * 3);
}
void loop(void) //"loop"
{
    void *tmp_ptr = ptr;
    just_fun_start();
    void *tmp_stack = stack;
    void *body = ptr += sizeof(int);
    run_block(body);
    while (stack >= tmp_stack)
    {
        run_block(body);
    }
    just_fun_end();
    ptr = tmp_ptr;
    next_size(sizeof(int) + (sizeof(void *) + sizeof(int)));
}
void break_(void) //"break"
{
    just_fun_end();
    imp = table[*(int *)(ptr = (*(void **)(break_stack_stack[break_stack_index] - sizeof(void *)) + sizeof(int) + sizeof(void *)))];
    stack = break_stack_stack[break_stack_index] - sizeof(void *);
}
// ================================ Variable definition and memory allocation ================================
void var(void) //"var"
{
    *(void **)std = global_var(re_func(ptr + sizeof(int)));
    next_sizeof(void *);
}
void var_auto(void) //"var_auto"
{
    int tmp = find_var_index(re_func(ptr + sizeof(int)));
    memcpy(std, var_address[tmp], var_size[tmp]);
    next_size(var_size[tmp]);
}
void init_int(void) //"#int"
{
    *(int *)std = *(int *)local_var(re_func(ptr + sizeof(int)), sizeof(int)) = *(int *)(std);
    next_sizeof(int);
}
void ptr_(void) //"p"
{
    *(void **)std = local_var(re_func(ptr + sizeof(int)), sizeof(void *));
    next_sizeof(void *);
}
void int_(void) //"int"
{
    *(void **)std = local_var(re_func(ptr + sizeof(int)), sizeof(int));
    next_sizeof(void *);
}
void float_(void) //"float"
{
    *(void **)std = local_var(re_func(ptr + sizeof(int)), sizeof(float));
    next_sizeof(void *);
}
void char_(void) //"char"
{
    *(void **)std = local_var(re_func(ptr + sizeof(int)), sizeof(char));
    next_sizeof(void *);
}
void color_(void) //"color"
{
    *(void **)std = local_var(re_func(ptr + sizeof(int)), sizeof(Color));
    next_sizeof(void *);
}
void size_t_(void) //"size_t"
{
    *(void **)std = local_var(re_func(ptr + sizeof(int)), sizeof(size_t));
    next_sizeof(void *);
}
void vector2_(void) //"vector2"
{
    *(void **)std = local_var(re_func(ptr + sizeof(int)), sizeof(Vector2));
    next_sizeof(void *);
}
void camera2d_(void) //"camera2d"
{
    *(void **)std = local_var(re_func(ptr + sizeof(int)), sizeof(Camera2D));
    next_sizeof(void *);
}
void space(void) //"space"
{
    *(void **)std = local_var(re_func(ptr + sizeof(int)), *(int *)(std));
    next_sizeof(void *);
}
void space_int(void) //"sp_int"
{
    *(void **)std = local_var(re_func(ptr + sizeof(int)), *(int *)(std) * sizeof(int));
    next_sizeof(void *);
}
void space_vector2(void) //"sp_vec2"
{
    *(void **)std = local_var(re_func(ptr + sizeof(int)), *(int *)(std) * sizeof(Vector2));
    next_sizeof(void *);
}
void space_ptr(void) //"sp_ptr"
{
    *(void **)std = local_var(re_func(ptr + sizeof(int)), *(int *)(std) * sizeof(void *));
    next_sizeof(void *);
}
// ================================ Data conversion and storage ================================
void data_str_int(void) //"\int"
{
    *(int *)std = atoi(ptr + sizeof(int) * 3);
    next_sizeof(int);
}
void data_str_char(void) //"\char"
{
    *(char *)std = *(char *)(ptr + sizeof(int) * 3);
    next_sizeof(char);
}
void sfloat(void) //"\float"
{
    *(float *)std = atof(ptr + sizeof(int) * 3);
    next_sizeof(float);
}
void data_to_size_t_(void) //"\size_t"
{
    *(size_t *)std = atoi(ptr + sizeof(int) * 3);
    next_sizeof(size_t);
}
void get_str(void) //"get_str"
{
    *(void **)std = str;
    next_sizeof(void *);
}
void null(void) //"null"
{
    *(void **)std = NULL;
    next_sizeof(void *);
}
void flag_window_resizable(void) //"FLAG_WINDOW_RESIZABLE"
{
    *(int *)(std) = FLAG_WINDOW_RESIZABLE;
    next_sizeof(int);
}
void getptr(void) //"ptr"
{
    *(void **)std = &ptr;
    next_sizeof(void *);
}
void getstd(void) //"std"
{
    *(void **)std = &std;
    next_sizeof(void *);
}
void getfile(void) //"get_file"
{
    *(void **)std = file;
    next_sizeof(void *);
}
void base_(void) //"base"
{
    *(void **)std = &base;
    next_sizeof(void *);
}
void up(void) //"up"
{
    *(void **)std = ptr + sizeof(int) * 3;
    next_sizeof(void *);
}
void get_file_size(void) //"file_size"
{
    *(size_t *)std = file_size;
    next_sizeof(int);
}
void get_strc(void) //"get_strc"
{
    *(int *)std = strc;
    next_sizeof(int);
}
void sizeof_int(void) //"sizeof_int"
{
    *(int *)std = sizeof(int);
    next_sizeof(int);
}
void sizeof_void(void) //"sizeof_void"
{
    *(int *)std = sizeof(void *);
    next_sizeof(int);
}
// ================================ Pointer dereferencing and address arithmetic ================================
void mul(void) //"*"
{
    *(void **)std = **(void ***)std;
    next_sizeof(void *);
}
void deref_int(void) //"*i"
{
    *(int *)std = ***(int ***)std;
    next_sizeof(int);
}
void i_(void) //"i"
{
    *(int *)std = **(int **)std;
    next_sizeof(int);
}
void type_char(void) //"c"
{
    *(char *)std = **(char **)std;
    next_sizeof(char);
}
void plus_ptr(void) //"+p"
{
    *(void **)std = *(void **)std + *(int *)(std + sizeof(void *));
    next_sizeof(void *);
}
void minus_ptr(void) //"-p"
{
    *(void **)std = *(void **)std - *(int *)(std + sizeof(void *));
    next_ins();
}
void minus_ptr_ptr(void) //"-pp"
{
    *(int *)std = *(void **)std - *(void **)(std + sizeof(void *));
    next_sizeof(int);
}
void plus_plus_ptr(void) //"++p"
{
    *(void **)std = (**(void ***)std)++;
    next_sizeof(void *);
}
void ptr_assign(void) //"*="
{
    *(void **)std = **(void ***)std = *(void **)(std + sizeof(void *));
    next_sizeof(void *);
}
void at(void) //"@"
{
    *(void **)local_var(re_func(ptr + sizeof(int)), sizeof(void *)) = *(void **)std;
    next_sizeof(void *);
}
void at_int(void) //"@int"
{
    *(int *)local_var(re_func(ptr + sizeof(int)), sizeof(int)) = *(int *)std;
    next_sizeof(int);
}
// ================================ Array access and indexing ================================
void array_access(void) //"*["
{
    *(void **)std = *(void **)(*(void **)std + (*(int *)(std + sizeof(void *))) * sizeof(void *));
    next_sizeof(void *);
}
void ex_ptr_arr(void) //"["
{
    *(void **)std = *(void **)std + (*(int *)(std + sizeof(void *))) * sizeof(void *);
    next_sizeof(void *);
}
void char_arr(void) //"[char"
{
    *(void **)std = *(void **)std + (*(int *)(std + sizeof(void *)));
    next_sizeof(void *);
}
void ex_char_arr(void) //"*[char"
{
    *(char *)std = *(char *)(*(void **)std + (*(int *)(std + sizeof(void *))));
    next_sizeof(char);
}
void int_arr(void) //"[int"
{
    *(void **)std = *(void **)std + (*(int *)(std + sizeof(void *))) * sizeof(int);
    next_sizeof(void *);
}
void ex_int_arr(void) //"*[int"
{
    *(int *)std = *(int *)(*(void **)std + (*(int *)(std + sizeof(void *))) * sizeof(int));
    next_sizeof(int);
}
void vec2_arr(void) //"[vec2"
{
    *(void **)std = *(void **)std + (*(int *)(std + sizeof(void *))) * sizeof(Vector2);
    next_sizeof(void *);
}
void ex_vec2_arr(void) //"*[vec2"
{
    *(Vector2 *)std = *(Vector2 *)(*(void **)std + (*(int *)(std + sizeof(void *))) * sizeof(Vector2));
    next_sizeof(Vector2);
}
// ================================ Structure member access ================================
void vector2_x(void) //"vec_x"
{
    *(float *)std = (*(Vector2 **)std)->x;
    next_sizeof(float);
}
void vector2_y(void) //"vec_y"
{
    *(float *)std = (*(Vector2 **)std)->y;
    next_sizeof(float);
}
void vector2_x_ptr(void) //"vec_x_ptr"
{
    *(void **)std = &((*(Vector2 **)std)->x);
    next_sizeof(void *);
}
void vector2_y_ptr(void) //"vec_y_ptr"
{
    *(void **)std = &((*(Vector2 **)std)->y);
    next_sizeof(void *);
}
void camera2d_offset(void) //"cam_offset"
{
    *(Vector2 *)std = (*(Camera2D **)std)->offset;
    next_sizeof(Vector2);
}
void camera2d_target(void) //"cam_target"
{
    *(Vector2 *)std = (*(Camera2D **)std)->target;
    next_sizeof(Vector2);
}
void camera2d_rotation(void) //"cam_rotation"
{
    *(float *)std = (*(Camera2D **)std)->rotation;
    next_sizeof(float);
}
void camera2d_zoom(void) //"cam_zoom"
{
    *(float *)std = (*(Camera2D **)std)->zoom;
    next_sizeof(float);
}
void camera2d_offset_ptr(void) //"cam_offset_ptr"
{
    *(void **)std = &((*(Camera2D **)std)->offset);
    next_sizeof(void *);
}
void camera2d_target_ptr(void) //"cam_target_ptr"
{
    *(void **)std = &((*(Camera2D **)std)->target);
    next_sizeof(void *);
}
void camera2d_rotation_ptr(void) //"cam_rotation_ptr"
{
    *(void **)std = &((*(Camera2D **)std)->rotation);
    next_sizeof(void *);
}
void camera2d_zoom_ptr(void) //"cam_zoom_ptr"
{
    *(void **)std = &((*(Camera2D **)std)->zoom);
    next_sizeof(void *);
}
// ================================ Arithmetic operations ================================
void plus(void) //"+"
{
    *(int *)std = *(int *)std + *(int *)(std + sizeof(int));
    next_sizeof(int);
}
void minus(void) //"-"
{
    *(int *)std = *(int *)std - *(int *)(std + sizeof(int));
    next_sizeof(int);
}
void x_(void) //"x"
{
    *(int *)std = *(int *)std * *(int *)(std + sizeof(int));
    next_sizeof(int);
}
void divide(void) //"/"
{
    *(int *)std = *(int *)std / *(int *)(std + sizeof(int));
    next_sizeof(int);
}
void increment(void) //"++"
{
    *(int *)std = (**(int **)std)++;
    next_sizeof(int);
}
void decrement(void) //"--"
{
    *(int *)std = (**(int **)std)--;
    next_sizeof(int);
}
void not_increment(void) //"+++"
{
    *(int *)std = ++(**(int **)std);
    next_sizeof(int);
}
void plus_float(void) //"+f"
{
    *(float *)std = *(float *)std + *(float *)(std + sizeof(float));
    next_sizeof(float);
}
void xfloat(void) //"xf"
{
    *(float *)std = *(float *)std * *(float *)(std + sizeof(int));
    next_sizeof(float);
}
void divide_float(void) //"/float"
{
    *(float *)std = *(float *)std / *(float *)(std + sizeof(int));
    next_sizeof(float);
}
// ================================ Comparison and logical operations ================================
void equal(void) //"=="
{
    *(unsigned char *)std = *(int *)std == *(int *)(std + sizeof(int));
    next_sizeof(char);
}
void not_equal(void) //"!="
{
    *(unsigned char *)std = *(int *)std != *(int *)(std + sizeof(int));
    next_sizeof(char);
}
void not_equal_char(void) //"!=c"
{
    *(unsigned char *)std = *(char *)std != *(char *)(std + sizeof(char));
    next_sizeof(char);
}
void greater(void) //">"
{
    *(unsigned char *)std = *(int *)std > *(int *)(std + sizeof(int));
    next_sizeof(char);
}
void less(void) //"<"
{
    *(unsigned char *)std = *(int *)std < *(int *)(std + sizeof(int));
    next_sizeof(char);
}
void greater_equal(void) //">="
{
    *(unsigned char *)std = *(int *)std >= *(int *)(std + sizeof(int));
    next_sizeof(char);
}
void less_equal(void) //"<="
{
    *(unsigned char *)std = *(int *)std <= *(int *)(std + sizeof(int));
    next_sizeof(char);
}
void equal_ptr(void) //"==p"
{
    *(unsigned char *)std = *(void **)std == *(void **)(std + sizeof(void *));
    next_sizeof(char);
}
void greater_ptr(void) //">p"
{
    *(unsigned char *)std = *(void **)std > *(void **)(std + sizeof(void *));
    next_sizeof(char);
}
void less_ptr(void) //"<p"
{
    *(unsigned char *)std = *(void **)std < *(void **)(std + sizeof(void *));
    next_sizeof(char);
}
void and_and(void) //"&&"
{
    *(unsigned char *)std = *(unsigned char *)std && *(unsigned char *)(std + sizeof(unsigned char));
    next_sizeof(char);
}
void or_or(void) //"||"
{
    *(unsigned char *)std = *(unsigned char *)std || *(unsigned char *)(std + sizeof(unsigned char));
    next_sizeof(char);
}
void not_(void) //"!"
{
    *(unsigned char *)std = !*(unsigned char *)std;
    next_sizeof(char);
}
// ================================ Assignment operation ================================
void assign(void) //"="
{
    *(void **)std = **(void ***)std = *(void **)(std + sizeof(void *));
    next_sizeof(void *);
}
void assign_int(void) //"=i"
{
    *(int *)std = **(int **)std = *(int *)(std + sizeof(void *));
    next_sizeof(int);
}
void assign_char(void) //"=c"
{
    *(char *)std = **(char **)std = *(char *)(std + sizeof(void *));
    next_sizeof(char);
}
void assign_float(void) //"=f"
{
    *(float *)std = **(float **)std = *(float *)(std + sizeof(void *));
    next_sizeof(float);
}
void assign_vec2(void) //"=vec2"
{
    *(Vector2 *)std = **(Vector2 **)std = *(Vector2 *)(std + sizeof(void *));
    next_sizeof(Vector2);
}
void plus_assign(void) //"+="
{
    *(int *)std = **(int **)std += *(int *)(std + sizeof(void *));
    next_sizeof(int);
}
void minus_assign(void) //"-="
{
    *(int *)std = **(int **)std -= *(int *)(std + sizeof(void *));
    next_sizeof(int);
}
void plus_assign_ptr(void) //"+=p"
{
    *(void **)std = **(void ***)std += *(int *)(std + sizeof(void *));
    next_sizeof(void *);
}
void minus_assign_ptr(void) //"-=p"
{
    *(void **)std = **(void ***)std -= *(int *)(std + sizeof(void *));
    next_sizeof(void *);
}
void plus_assign_float(void) //"+=f"
{
    *(float *)std = **(float **)std += *(float *)(std + sizeof(void *));
    next_sizeof(float);
}
// ================================ Type conversion and string processing ================================
void int_from_float(void) //"i-f"
{
    *(float *)std = (float)*(int *)std;
    next_sizeof(float);
}
void float_to_int(void) //"f->i"
{
    *(int *)std = (int)(*(float *)std);
    next_sizeof(int);
}
void int_to_size_t(void) //"i-size_t"
{
    *(size_t *)std = (size_t)(*(int *)std);
    next_sizeof(size_t);
}
void size_t_to_int(void) //"size_t->i"
{
    *(int *)std = (int)(*(size_t *)std);
    next_sizeof(int);
}
void int_to_char(void) //"i-?"
{
    *(unsigned char *)std = *(int *)std != 0;
    next_sizeof(char);
}
void itoa_(void) //"itoa"
{
    *(void **)std = (void *)TextFormat("%d", *(int *)std);
    next_sizeof(void *);
}
void strcb(void) //"strcb"
{
    *(void **)std = (void *)TextFormat("%s%s", *(void **)std, *(void **)(std + sizeof(void *)));
    next_sizeof(void *);
}
void ptr_to_ascii(void) //"ptoa"
{
    *(void **)std = (void *)TextFormat("%p", *(void **)std);
    next_sizeof(void *);
}
void ins_remove_underscores(void) //"remove_underscores"
{
    *(void **)std = remove_underscores(*(void **)std);
    next_sizeof(void *);
}
// ================================ Color constant ================================
void color_black(void) //"BLACK"
{
    *(Color *)std = BLACK;
    next_sizeof(Color);
}
void color_white(void) //"WHITE"
{
    *(Color *)std = WHITE;
    next_sizeof(Color);
}
void color_red(void) //"RED"
{
    *(Color *)std = RED;
    next_sizeof(Color);
}
void color_green(void) //"GREEN"
{
    *(Color *)std = GREEN;
    next_sizeof(Color);
}
void color_blue(void) //"BLUE"
{
    *(Color *)std = BLUE;
    next_sizeof(Color);
}
void color_yellow(void) //"YELLOW"
{
    *(Color *)std = YELLOW;
    next_sizeof(Color);
}
void color_gold(void) //"GOLD"
{
    *(Color *)std = GOLD;
    next_sizeof(Color);
}
void color_purple(void) //"PURPLE"
{
    *(Color *)std = PURPLE;
    next_sizeof(Color);
}
void color_skyblue(void) //"SKYBLUE"
{
    *(Color *)std = SKYBLUE;
    next_sizeof(Color);
}
void color_darkgray(void) //"DARKGRAY"
{
    *(Color *)std = DARKGRAY;
    next_sizeof(Color);
}
void color_lightgray(void) //"LIGHTGRAY"
{
    *(Color *)std = LIGHTGRAY;
    next_sizeof(Color);
}
// ================================ System and debugging ================================
void init_start(void) //"#init"
{
    base = ptr;
    std_base = std = malloc(4096);
    stack_base = stack = malloc(sizeof(void *) * 1024);
    var_buff_offset = var_buff = malloc(1048576);
    var_ip = malloc(2048 * 32);
    next_ins();
}
void break_point_std(void) //"#break"
{
    break_stack_index--;
    var_stack_index--;
    var_index = var_index_stack[var_stack_index];
    var_buff_offset = var_buff_offset_stack[var_stack_index];
    imp = table[*(int *)(ptr = (*(void **)(break_stack_stack[break_stack_index] - sizeof(void *)) + sizeof(int) + sizeof(void *)))];
    stack = break_stack_stack[break_stack_index] - sizeof(void *);
}
void dbg_point(void) //"?"
{
    // need not implement
}
void Vector2Subtract_(void) //"vector2_subtract"
{
    *(Vector2 *)std = Vector2Subtract(*(Vector2 *)std, *(Vector2 *)(std + sizeof(void *)));
    next_sizeof(Vector2);
}
void Vector2Scale_(void) //"vector2_scale"
{
    *(Vector2 *)std = Vector2Scale(*(Vector2 *)std, *(float *)(std + sizeof(void *)));
    next_sizeof(Vector2);
}
void color_lime(void) //"LIME"
{
    *(Color *)std = LIME;
    next_sizeof(Color);
}
void assign_color(void) //"=color"
{
    *(Color *)std = **(Color **)std = *(Color *)(std + sizeof(void *));
    next_sizeof(Color);
}
void input_(void) //"input"
{
    input(*(char **)std);
    next_ins();
}
void color_gray(void) //"GRAY"
{
    *(Color *)std = GRAY;
    next_sizeof(Color);
}
void out(void) //"out"
{
    if (*(unsigned char *)std)
    {
        next_ins();
    }
    else
    {
        ret();
    }
}
void rerun(void) //"rerun"
{
    imp = table[*(int *)(ptr = *(void **)(*(void **)(stack - sizeof(void *)) + sizeof(int)))];
}
void std_start_(void) //">>>"
{
    std_start();
}
void std_end_(void) //"<<<"
{
    std_end();
}
void Vector2Add_(void) //"vector2_add"
{
    *(Vector2 *)std = Vector2Add(*(Vector2 *)std, *(Vector2 *)(std + sizeof(void *)));
    next_sizeof(Vector2);
}
void greater_equal_float(void) //">=f"
{
    *(unsigned char *)std = *(float *)std >= *(float *)(std + sizeof(float));
    next_sizeof(char);
}
void less_equal_float(void) //"<=f"
{
    *(unsigned char *)std = *(float *)std <= *(float *)(std + sizeof(float));
    next_sizeof(char);
}
void var_end(void) //"<<<<"
{
    break_stack_index--;
    var_stack_index--;
    var_index = var_index_stack[var_stack_index];
    var_buff_offset = var_buff_offset_stack[var_stack_index];
    next_ins();
}
void hash_vector2_y(void) //"#vec_y"
{
    *(float *)std = (*(Vector2 *)std).y;
    next_sizeof(float);
}
void char_to_int(void) //"c-i"
{
    *(int *)std = (int)*(char *)std;
    next_sizeof(int);
}
void get_stack(void) //"get_stack"
{
    *(void **)std = &stack;
    next_sizeof(void *);
}
void get_stack_base(void) //"get_stack_base"
{
    *(void **)std = &stack_base;
    next_sizeof(void *);
}
void greater_equal_ptr(void) //">=p"
{
    *(unsigned char *)std = *(void **)std >= *(void **)(std + sizeof(void *));
    next_sizeof(char);
}
void ptr_to_unsigned_char(void) //"p-?"
{
    *(unsigned char *)std = *(void **)std ? 1 : 0;
    next_sizeof(unsigned char);
}
void filepathlist(void) //"filepathlist"
{
    *(void **)std = local_var(re_func(ptr + sizeof(int)), sizeof(FilePathList));
    next_sizeof(void *);
}
void filepathlist_paths(void) //"filepath_paths"
{
    *(void **)std = &((*(FilePathList **)std)->paths);
    next_sizeof(void *);
}
void assign_filepathlist(void) //"=fpl"
{
    *(FilePathList *)std = **(FilePathList **)std = *(FilePathList *)(std + sizeof(void *));
    next_sizeof(FilePathList);
}
void long_(void) //"\long"
{
    *(long *)std = atoi(ptr + sizeof(int) * 3);
    next_sizeof(long);
}
void long_to_int(void) //"l-i"
{
    *(int *)std = (int)*(long *)std;
    next_sizeof(int);
}
void run_once(void) //"onece"
{
    if (*(unsigned char *)std = *(char *)(ptr + sizeof(int) * 3) == '1')
    {
        *(char *)(ptr + sizeof(int) * 3) = '0';
    }
    next_sizeof(unsigned char);
}