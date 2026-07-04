#ifndef EDITORCORE_H
#define EDITORCORE_H
#ifdef __cplusplus
extern "C" {
#endif
#ifdef EDITORCORE_BUILD
#define EDITORCORE_API __declspec(dllexport)
#else
#define EDITORCORE_API __declspec(dllimport)
#endif
typedef unsigned char ec_u8;
typedef unsigned int ec_u32;
EDITORCORE_API void ec_init(void);
EDITORCORE_API void ec_frame(void);
EDITORCORE_API void ec_render(void);
EDITORCORE_API void ec_input(void);
EDITORCORE_API void ec_flush(void);
EDITORCORE_API int ec_should_halt(void);
EDITORCORE_API int ec_registry_count(void);
EDITORCORE_API int ec_registry_item(int idx, char *name, ec_u32 name_cap, ec_u8 token[32]);
#ifdef __cplusplus
}
#endif
#endif
