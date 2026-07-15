#include "views_common.h"
#include <string.h>
extern __declspec(dllimport) void *from(u32);
extern __declspec(dllimport) int cvm_sha256(const u8 *p, u32 n, H out);
extern __declspec(dllimport) void cvm_edge(const H parent, const H child);
/* stack: none. Uses active view key if tag: typein is in var from payload args?
 * Simpler approach used by recipe: stack has new_name bytes? 
 * payload: views id; stack: name buffer[256] as C string already hashed by caller.
 *
 * Recipe: active_key -> if tag, sha256(input) is wrong for content-addressed tags.
 * Content-addressed: tag key == sha256("#text"). Renaming means new node + edge.
 * For explorer "URL" edit: treat typein as display override stored in a side map — too heavy.
 *
 * Practical: write typein into a small uset override of the tag key pointing to upload(typein).
 * That changes resolved content to the new string if server allows.
 */
extern __declspec(dllimport) void cvm_flush_key(const H key);
extern __declspec(dllimport) u8 *cvm_var_get(const u8 *id, u32 id_len, u32 *size);
extern __declspec(dllimport) u8 *cvm_cached_base(void);
extern __declspec(dllimport) u32 cvm_cached_len(void);
extern __declspec(dllimport) void cvm_cached_set_len(u32);
extern __declspec(dllimport) int cvm_resolve_payload_hash(const H k, H h);
extern __declspec(dllimport) void cvm_upload_async(const u8 *p, u32 n);

/* payload: typein var id (entire). stack: view key[32] */
__declspec(dllexport) void run(void){
    H key; memcpy(key, from(32), 32);
    u8 *vid = cvm_payload(); u32 vidn = cvm_payload_size();
    if (!vidn || zero_key(key)) { cont(); return; }
    u32 ts=0; u8 *text = cvm_var_get(vid, vidn, &ts);
    if (!text || !ts) { cont(); return; }
    /* C-string length */
    u32 n=0; while (n<ts && text[n]) n++;
    if (!n) { cont(); return; }
    /* Upload text as content and uset key -> content hash via flush path:
     * resolve key into cache, replace cache bytes with text, flush_key. */
    H h; cvm_resolve_payload_hash(key, h);
    u8 *b = cvm_cached_base();
    if (n + 1 > (1u<<20)) { cont(); return; }
    memcpy(b, text, n);
    /* no need for NUL in content */
    cvm_cached_set_len(n);
    cvm_flush_key(key);
    cont();
}
