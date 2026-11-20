/* Sample assets/file browser, GPL-3.0-or-later. No audio callback owns IO. */
#include <limits.h>
#include <stdint.h>
#include <stdatomic.h>
#define SAMPLE_BUDGET (10u*1024u*1024u)
#define SAMPLE_REQUESTS 8
#define SAMPLE_ROWS 8
#define SAMPLE_ENTRIES 4096
#define SNAP_COUNT (GP_COUNT+9+3)
typedef struct SampleAsset {
    struct SampleAsset *next;
    unsigned count;float *data;char name[64],hexname[129];char *encoded;size_t encoded_len;
} SampleAsset;
typedef struct {char name[256];int directory;} SampleEntry;
typedef struct {char path[PATH_MAX],selected[PATH_MAX];int count,page;SampleEntry entries[SAMPLE_ENTRIES];} SampleView;
enum {SAMPLE_SCAN=1,SAMPLE_LOAD=2};
typedef struct {int action,pad,osc,page;unsigned stamp;char path[PATH_MAX];} SampleRequest;
typedef struct {SampleView *view;SampleAsset *asset;int pad,osc;unsigned stamp;char status[64];} SampleReply;
typedef struct {
    _Atomic(SampleAsset*) assets[16][9],retired;
    atomic_int source[16][9],readers,busy;
    atomic_uint generation[16][9],write,read,pending;
    SampleRequest requests[SAMPLE_REQUESTS];
    _Atomic(SampleReply*) reply;
    SampleView *view;
    size_t bytes;uint64_t dirty;char status[64];
} SampleManager;
typedef struct {SampleAsset *assets[16][9];int source[16][9],focus[16][3];} SampleRestore;
