/* Native read-only library browser and immutable sample ownership. GPL-3.0-or-later. */
#include <dirent.h>
#include <errno.h>
#include "sample_data.h"
#define SAMPLE_NOTIFY_OSC(n) "o"#n"_sample_on","o"#n"_sample_name","o"#n"_source_view"
static const char *sample_notifications[]={SAMPLE_NOTIFY_OSC(1),SAMPLE_NOTIFY_OSC(2),SAMPLE_NOTIFY_OSC(3),SAMPLE_NOTIFY_OSC(4),SAMPLE_NOTIFY_OSC(5),SAMPLE_NOTIFY_OSC(6),SAMPLE_NOTIFY_OSC(7),SAMPLE_NOTIFY_OSC(8),SAMPLE_NOTIFY_OSC(9),
 "sample_source","sample_name","sample_folder","sample_status","sample_page","sample_row0","sample_row1","sample_row2","sample_row3","sample_row4","sample_row5","sample_row6","sample_row7"};
#define SAMPLE_NOTIFY_COUNT (sizeof sample_notifications/sizeof *sample_notifications)
static void sample_notify(Engine *e){e->samples.dirty=(UINT64_C(1)<<SAMPLE_NOTIFY_COUNT)-1;++e->display_revision;++e->ui_revision;}
static void sample_retire(Engine *e,SampleAsset *a) {
    if(!a){return;}
    SampleAsset *head=atomic_load(&e->samples.retired);
    do{a->next=head;}while(!atomic_compare_exchange_weak(&e->samples.retired,&head,a));
}
static void sample_collect(Engine *e) {
    /* Detach first, then test readers: a new reader can only reach live assets,
     * not this retired list. Old readers keep the entire detached list alive. */
    SampleAsset *list=atomic_exchange(&e->samples.retired,NULL);if(!list)return;
    if(!atomic_load(&e->samples.readers)){while(list){SampleAsset *next=list->next;sample_free(list);list=next;}return;}
    while(list){SampleAsset *next=list->next;sample_retire(e,list);list=next;}
}
static void sample_hold(Engine *e,int pad){atomic_fetch_add(&e->samples.readers,1);for(int i=0;i<9;i++)e->worker_samples[i]=atomic_load(&e->samples.assets[pad][i]);}
static void sample_release(Engine *e){atomic_fetch_sub(&e->samples.readers,1);}
static void sample_init(Engine *e,const char *dir) {
    snprintf(e->samples.default_folder,sizeof e->samples.default_folder,"/nvme");
    if(dir && dir[0]=='/' && snprintf(e->samples.preference_file,sizeof e->samples.preference_file,"%s/.sample-folder",dir)<PATH_MAX) {
        FILE *f=fopen(e->samples.preference_file,"rb");
        if(f){char path[PATH_MAX];size_t n=fread(path,1,sizeof path-1,f);int complete=feof(f);fclose(f);path[n]=0;
            if(complete && n>0 && path[0]=='/' && !memchr(path,0,n) && !strchr(path,'\n') && !strchr(path,'\r'))snprintf(e->samples.default_folder,sizeof e->samples.default_folder,"%s",path);}
    }else e->samples.preference_file[0]=0;

    atomic_init(&e->samples.busy,0);atomic_init(&e->samples.readers,0);atomic_init(&e->samples.retired,NULL);atomic_init(&e->samples.reply,NULL);atomic_init(&e->samples.pending,0);atomic_init(&e->samples.write,0);atomic_init(&e->samples.read,0);
    snprintf(e->samples.status,sizeof e->samples.status,"OPEN STORAGE TO BROWSE");
    for(int p=0;p<PADS;p++){for(int l=0;l<3;l++)atomic_init(&e->sound_focus[p][l],l*3);
        for(int o=0;o<9;o++){atomic_init(&e->samples.assets[p][o],NULL);atomic_init(&e->samples.source[p][o],0);atomic_init(&e->samples.generation[p][o],1);}}
}
static void sample_reset_pad(Engine *e,int pad) {
    for(int o=0;o<9;o++){SampleAsset *a=atomic_exchange(&e->samples.assets[pad][o],NULL);if(a)e->samples.bytes-=(size_t)a->count*4;sample_retire(e,a);atomic_store(&e->samples.source[pad][o],0);atomic_fetch_add(&e->samples.generation[pad][o],1);}
    for(int l=0;l<3;l++)atomic_store(&e->sound_focus[pad][l],l*3);
    sample_notify(e);
}
static void sample_free_reply(SampleReply *r){if(r){free(r->view);sample_free(r->asset);free(r);}}
static void sample_destroy(Engine *e) {
    sample_free_reply(atomic_exchange(&e->samples.reply,NULL));free(e->samples.view);
    for(int p=0;p<PADS;p++)for(int o=0;o<9;o++)sample_free(atomic_load(&e->samples.assets[p][o]));
    sample_collect(e);
}
static int sample_join(char *out,size_t size,const char *folder,const char *name){int n=snprintf(out,size,"%s%s%s",folder,!strcmp(folder,"/")?"":"/",name);return n>0 && (size_t)n<size;}
static int sample_entry_compare(const void *a,const void *b){const SampleEntry *x=a,*y=b;if(x->directory!=y->directory)return y->directory-x->directory;int n=strcasecmp(x->name,y->name);return n?n:strcmp(x->name,y->name);}
static SampleView *sample_scan(const char *path,int page,char *status,size_t size) {
    char resolved[PATH_MAX];if(!realpath(path,resolved)){if(!strcmp(path,"/nvme"))snprintf(resolved,sizeof resolved,"/");else {snprintf(status,size,"FOLDER NOT AVAILABLE");return NULL;}}
    DIR *dir=opendir(resolved);if(!dir){snprintf(status,size,"CANNOT OPEN FOLDER");return NULL;}
    SampleView *view=calloc(1,sizeof *view);if(!view){closedir(dir);snprintf(status,size,"NOT ENOUGH MEMORY");return NULL;}
    snprintf(view->path,sizeof view->path,"%s",resolved);struct dirent *entry;int error=0;
    while((entry=readdir(dir))) {
        if(entry->d_name[0]=='.'){continue;}
        char full[PATH_MAX];struct stat st;if(!sample_join(full,sizeof full,resolved,entry->d_name) || stat(full,&st))continue;
        size_t len=strlen(entry->d_name);int isdir=S_ISDIR(st.st_mode),wav=S_ISREG(st.st_mode)&&len>4&&!strcasecmp(entry->d_name+len-4,".wav");
        if(!isdir&&!wav){continue;}
        if(view->count==SAMPLE_ENTRIES){error=1;break;}
        SampleEntry *item=view->entries+view->count++;snprintf(item->name,sizeof item->name,"%s",entry->d_name);item->directory=isdir;
    }closedir(dir);
    if(error){free(view);snprintf(status,size,"FOLDER OVER 4096 ITEMS");return NULL;}
    qsort(view->entries,view->count,sizeof view->entries[0],sample_entry_compare);int last=view->count?(view->count-1)/SAMPLE_ROWS:0;view->page=page<0?0:page>last?last:page;
    snprintf(status,size,view->count?"CHOOSE WAV OR FOLDER":"NO WAV FILES HERE");return view;
}
static int sample_enqueue(Engine *e,int action,const char *path,int page) {
    unsigned w=atomic_load(&e->samples.write),r=atomic_load(&e->samples.read);
    if(w-r>=SAMPLE_REQUESTS){snprintf(e->samples.status,sizeof e->samples.status,"BROWSER BUSY - RETRY");sample_notify(e);return 0;}
    SampleRequest *q=e->samples.requests+w%SAMPLE_REQUESTS;memset(q,0,sizeof *q);q->action=action;q->page=page;q->pad=atomic_load(&e->selected);q->osc=(int)at(e,"plot_osc");
    if(strlen(path)>=sizeof q->path){snprintf(e->samples.status,sizeof e->samples.status,"PATH TOO LONG");sample_notify(e);return 0;}snprintf(q->path,sizeof q->path,"%s",path);
    if(action==SAMPLE_LOAD)q->stamp=atomic_fetch_add(&e->samples.generation[q->pad][q->osc],1)+1;
    atomic_fetch_add(&e->samples.pending,1);atomic_store(&e->samples.write,w+1);snprintf(e->samples.status,sizeof e->samples.status,action==SAMPLE_LOAD?"LOADING SAMPLE...":"READING FOLDER...");sample_notify(e);return 1;
}
static int sample_save_default(Engine *e,const char *path,char *status,size_t size) {
    if(!e->samples.preference_file[0]){snprintf(status,size,"NO SETTINGS DIRECTORY");return 0;}
    if(strchr(path,'\n') || strchr(path,'\r')){snprintf(status,size,"FOLDER NAME UNSUPPORTED");return 0;}
    char temporary[PATH_MAX];if(snprintf(temporary,sizeof temporary,"%s.XXXXXX",e->samples.preference_file)>=PATH_MAX){snprintf(status,size,"PATH TOO LONG");return 0;}
    int fd=mkstemp(temporary);if(fd<0){snprintf(status,size,"CANNOT SAVE DEFAULT");return 0;}
    FILE *f=fdopen(fd,"wb");int ok=0;
    if(f){size_t n=strlen(path);ok=fwrite(path,1,n,f)==n;if(fclose(f))ok=0;}else close(fd);
    if(ok)ok=rename(temporary,e->samples.preference_file)==0;
    if(!ok)unlink(temporary);
    snprintf(status,size,ok?"DEFAULT FOLDER SAVED":"CANNOT SAVE DEFAULT");return ok;
}
static void sample_worker(Engine *e) {
    sample_collect(e);if(atomic_load(&e->samples.reply))return;
    unsigned r=atomic_load(&e->samples.read);if(r==atomic_load(&e->samples.write))return;
    SampleRequest q=e->samples.requests[r%SAMPLE_REQUESTS];atomic_store(&e->samples.busy,1);atomic_store(&e->samples.read,r+1);
    SampleReply *reply=calloc(1,sizeof *reply);if(!reply){atomic_fetch_sub(&e->samples.pending,1);atomic_store(&e->samples.busy,0);return;}reply->pad=q.pad;reply->osc=q.osc;reply->stamp=q.stamp;
    if(q.action==SAMPLE_SCAN)reply->view=sample_scan(q.path,q.page,reply->status,sizeof reply->status);
    else if(q.action==SAMPLE_DEFAULT){char resolved[PATH_MAX];struct stat st;
        if(!realpath(q.path,resolved)||stat(resolved,&st)||!S_ISDIR(st.st_mode))snprintf(reply->status,sizeof reply->status,"FOLDER NOT AVAILABLE");
        else if(sample_save_default(e,resolved,reply->status,sizeof reply->status))snprintf(reply->default_folder,sizeof reply->default_folder,"%s",resolved);
    }
    else reply->asset=sample_read_wave(q.path,reply->status,sizeof reply->status);
    atomic_store(&e->samples.reply,reply);atomic_store(&e->samples.busy,0);
}
static void sample_put(Engine *e,int pad,const char *key,float v){int i=index_of(key);if(i>=0){atomic_store(&e->pad_values[pad][i],v);if(pad==atomic_load(&e->selected))e->dirty[i]=1;}}
static void sample_poll(Engine *e) {
    sample_collect(e); /* normal-priority display path also cleans during slow IO */
    SampleReply *r=atomic_exchange(&e->samples.reply,NULL);if(!r)return;
    snprintf(e->samples.status,sizeof e->samples.status,"%s",r->status);
    if(r->default_folder[0])snprintf(e->samples.default_folder,sizeof e->samples.default_folder,"%s",r->default_folder);
    if(r->view){free(e->samples.view);e->samples.view=r->view;r->view=NULL;}
    if(r->asset) {
        int pad=r->pad,osc=r->osc;SampleAsset *old=atomic_load(&e->samples.assets[pad][osc]);size_t oldbytes=old?(size_t)old->count*4:0;
        if(r->stamp!=atomic_load(&e->samples.generation[pad][osc]))snprintf(e->samples.status,sizeof e->samples.status,"STALE IMPORT CANCELLED");
        else if(e->samples.bytes-oldbytes+(size_t)r->asset->count*4>SAMPLE_BUDGET)snprintf(e->samples.status,sizeof e->samples.status,"KIT SAMPLE LIMIT 10 MIB");
        else {
            atomic_fetch_add(&e->revision,1);e->samples.bytes=e->samples.bytes-oldbytes+(size_t)r->asset->count*4;
            SampleAsset *a=r->asset;r->asset=NULL;sample_retire(e,atomic_exchange(&e->samples.assets[pad][osc],a));atomic_store(&e->samples.source[pad][osc],1);
            char k[48];
#define SAMPLE_PUT(suffix,v) do{snprintf(k,sizeof k,"o%d_%s",osc+1,suffix);sample_put(e,pad,k,v);}while(0)
            SAMPLE_PUT("on",1);SAMPLE_PUT("freq",55);SAMPLE_PUT("sweep",0);SAMPLE_PUT("phase",0);SAMPLE_PUT("level",1);SAMPLE_PUT("attack",0);SAMPLE_PUT("decay",fmaxf(.005f,a->count/(float)RATE));
            SAMPLE_PUT("a_mode",1);SAMPLE_PUT("a_seen",1);SAMPLE_PUT("p_mode",0);SAMPLE_PUT("p_seen",0);
#undef SAMPLE_PUT
            snprintf(k,sizeof k,"f%d_a_n",osc+1);sample_put(e,pad,k,2);
            const char *fields[]={"x0","x1","y0","y1","b0","c0"};float vals[]={0,1,1,1,0,0};
            for(int i=0;i<6;i++){snprintf(k,sizeof k,"f%d_a_%s",osc+1,fields[i]);sample_put(e,pad,k,vals[i]);}
            snprintf(k,sizeof k,"f%d_p_n",osc+1);sample_put(e,pad,k,0);sample_put(e,pad,"free_select",0);
            float duration=a->count/(float)RATE;if(duration>pad_at(e,pad,"length"))sample_put(e,pad,"length",duration);
            atomic_fetch_add(&e->pad_audio[pad],1);atomic_fetch_add(&e->pad_view[pad],1);
            if(pad==atomic_load(&e->selected)){kit_feedback(e);++e->control_context;}
            atomic_fetch_add(&e->revision,1);
            snprintf(e->samples.status,sizeof e->samples.status,strstr(r->status,"FIRST 4")?"P%02d/O%d: FIRST 4S":"LOADED P%02d / OSC%d",pad+1,osc+1);
            snprintf(k,sizeof k,"layer%d",osc/3+1);if(pad_at(e,pad,k)==0)snprintf(e->samples.status,sizeof e->samples.status,"P%02d/O%d LAYER IS OFF",pad+1,osc+1);
        }
    }
    sample_notify(e);sample_free_reply(r);atomic_fetch_sub(&e->samples.pending,1);
}
static int sample_osc_key(const char *key,const char *suffix){int o;char field[48],tail;if(sscanf(key,"o%d_%47s%c",&o,field,&tail)==2 && o>=1&&o<=9 && !strcmp(field,suffix))return o-1;return -1;}
static int sample_set(Engine *e,const char *key,const char *text) {
    int pad=atomic_load(&e->selected),osc=(int)at(e,"plot_osc"),target=sample_osc_key(key,"sample_on");
    if(target>=0 || !strcmp(key,"sample_source")) {
        if(target>=0){osc=target;}
        char *end;float v=strtof(text,&end);if(end==text||*end||!isfinite(v))return 1;int n=v>=.5f;
        if(n!=atomic_load(&e->samples.source[pad][osc])){atomic_fetch_add(&e->revision,1);atomic_store(&e->samples.source[pad][osc],n);
            atomic_store(&e->sound_focus[pad][osc/3],osc);put(e,"plot_osc",osc);e->dirty[index_of("plot_osc")]=1;
            e->point_dirty|=POINT_ALL;e->free_dirty=(1u<<FREE_NOTIFY_COUNT)-1;e->edit_time_valid=0;
            audio_changed(e);atomic_fetch_add(&e->revision,1);sample_notify(e);}return 1;
    }
    if(!strcmp(key,"sample_import_path")){if(text[0]=='/')sample_enqueue(e,SAMPLE_LOAD,text,0);return 1;}
    if(!strcmp(key,"sample_directory")){if(text[0]=='/')sample_enqueue(e,SAMPLE_SCAN,text,0);return 1;}
    if(strncmp(key,"sample_",7))return 0;
    char *end;float pressed=strtof(text,&end);if(end==text||*end||!isfinite(pressed)||pressed<.5f)return 1;
    SampleView *v=e->samples.view;const char *folder=v?v->path:"/nvme";
    if(!strcmp(key,"sample_open"))sample_enqueue(e,SAMPLE_SCAN,e->samples.default_folder,0);
    else if(!strcmp(key,"sample_set_default")){if(v)sample_enqueue(e,SAMPLE_DEFAULT,v->path,0);else{snprintf(e->samples.status,sizeof e->samples.status,"OPEN A FOLDER FIRST");sample_notify(e);}}
    else if(!strcmp(key,"sample_root"))sample_enqueue(e,SAMPLE_SCAN,"/",0);
    else if(!strcmp(key,"sample_up")){char path[PATH_MAX];snprintf(path,sizeof path,"%s",folder);char *slash=strrchr(path,'/');if(slash && slash!=path)*slash=0;else snprintf(path,sizeof path,"/");sample_enqueue(e,SAMPLE_SCAN,path,0);}
    else if(!strcmp(key,"sample_prev") || !strcmp(key,"sample_next")){if(v)sample_enqueue(e,SAMPLE_SCAN,v->path,v->page+(!strcmp(key,"sample_next")?1:-1));}
    else if(!strcmp(key,"sample_load")){if(v && v->selected[0])sample_enqueue(e,SAMPLE_LOAD,v->selected,0);else{snprintf(e->samples.status,sizeof e->samples.status,"SELECT A WAV FIRST");sample_notify(e);}}
    else if(!strcmp(key,"sample_clear")) {
        atomic_fetch_add(&e->revision,1);SampleAsset *a=atomic_exchange(&e->samples.assets[pad][osc],NULL);if(a)e->samples.bytes-=(size_t)a->count*4;sample_retire(e,a);atomic_store(&e->samples.source[pad][osc],0);atomic_fetch_add(&e->samples.generation[pad][osc],1);
        char k[32];snprintf(k,sizeof k,"o%d_on",osc+1);sample_put(e,pad,k,0);audio_changed(e);atomic_fetch_add(&e->revision,1);snprintf(e->samples.status,sizeof e->samples.status,"SAMPLE CLEARED / OSC OFF");sample_notify(e);
    }else {
        int row;char extra;if(v && sscanf(key,"sample_pick%d%c",&row,&extra)==1 && row>=0 && row<SAMPLE_ROWS) {
            int i=v->page*SAMPLE_ROWS+row;if(i<v->count){char path[PATH_MAX];if(sample_join(path,sizeof path,v->path,v->entries[i].name)){
                if(v->entries[i].directory)sample_enqueue(e,SAMPLE_SCAN,path,0);
                else{snprintf(v->selected,sizeof v->selected,"%s",path);snprintf(e->samples.status,sizeof e->samples.status,"SELECTED - PRESS LOAD");sample_notify(e);}
            }}
        }
    }
    return 1;
}
static void sample_short_name(char *out,size_t size,const char *name,size_t limit) {
    size_t n=strlen(name);if(n<=limit){snprintf(out,size,"%s",name);return;}
    size_t tail=8,head=limit-tail-1;snprintf(out,size,"%.*s~%s",(int)head,name,name+n-tail);
}
static int sample_get(Engine *e,const char *key,char *out,int len) {
    int pad=atomic_load(&e->selected),osc=(int)at(e,"plot_osc"),target=sample_osc_key(key,"sample_on");
    if(target>=0 || !strcmp(key,"sample_source"))return snprintf(out,len,"%d",atomic_load(&e->samples.source[pad][target>=0?target:osc]));
    target=sample_osc_key(key,"source_view");if(target>=0){char k[32];snprintf(k,sizeof k,"o%d_wave",target+1);return snprintf(out,len,"%d",atomic_load(&e->samples.source[pad][target])?2:at(e,k)>=4?1:0);}
    target=sample_osc_key(key,"sample_name");
    if(target>=0 || !strcmp(key,"sample_name")){SampleAsset *a=atomic_load(&e->samples.assets[pad][target>=0?target:osc]);char name[64];sample_short_name(name,sizeof name,a?a->name:"NO SAMPLE LOADED",23);return snprintf(out,len,"%s",name);}
    if(!strcmp(key,"sample_status"))return snprintf(out,len,"%.23s",e->samples.status);
    SampleView *v=e->samples.view;
    if(!strcmp(key,"sample_folder")){const char *s=v?strrchr(v->path,'/'):NULL;return snprintf(out,len,"%.23s",v?(s&&s[1]?s+1:v->path):"OPEN STORAGE");}
    if(!strcmp(key,"sample_page"))return snprintf(out,len,"%d / %d",v?v->page+1:0,v?(v->count?1+(v->count-1)/SAMPLE_ROWS:1):0);
    int row;char extra;if(sscanf(key,"sample_row%d%c",&row,&extra)==1 && row>=0&&row<SAMPLE_ROWS) {
        int i=v?v->page*SAMPLE_ROWS+row:0;if(!v || i>=v->count)return snprintf(out,len,"%s",row==0&&!v?"PRESS OPEN STORAGE":"");
        char full[PATH_MAX];int picked=sample_join(full,sizeof full,v->path,v->entries[i].name)&&!strcmp(v->selected,full);
        char name[64];sample_short_name(name,sizeof name,v->entries[i].name,21);return snprintf(out,len,"%c %s",v->entries[i].directory?'/':picked?'>':' ',name);
    }
    if(!strcmp(key,"sample_pending"))return snprintf(out,len,"%d",atomic_load(&e->samples.pending)>0);
    return -1;
}
static int sample_token(const char **s,const char **token,size_t *len) {
    while(isspace((unsigned char)**s)){++*s;}
    *token=*s;while(**s&&!isspace((unsigned char)**s))++*s;*len=(size_t)(*s-*token);return *len>0;
}
static int sample_uint(const char **s,unsigned *out,unsigned max) {
    const char *p;size_t n;if(!sample_token(s,&p,&n))return 0;uint64_t v=0;
    for(size_t i=0;i<n;i++){if(p[i]<'0'||p[i]>'9')return 0;v=v*10+(unsigned)(p[i]-'0');if(v>max)return 0;}*out=(unsigned)v;return 1;
}
static void sample_restore_free(SampleRestore *r){if(r){for(int p=0;p<PADS;p++)for(int o=0;o<9;o++)sample_free(r->assets[p][o]);free(r);}}
static SampleRestore *sample_restore_read(const char *s,const float rows[PADS][GP_COUNT]) {
    SampleRestore *r=calloc(1,sizeof *r);if(!r)return NULL;
    r->program=-1;
    for(int p=0;p<PADS;p++)for(int l=0;l<3;l++){int osc=(int)rows[p][index_of("plot_osc")];r->focus[p][l]=osc/3==l?osc:l*3;}
    while(isspace((unsigned char)*s))s++;
    if(!strncmp(s,"VIEW1 ",6)) {
        s+=6;for(int p=0;p<PADS;p++)for(int l=0;l<3;l++){unsigned v;if(!sample_uint(&s,&v,8)||v/3!=(unsigned)l)goto fail;r->focus[p][l]=(int)v;}
        while(isspace((unsigned char)*s))s++;
    }
    if(!strncmp(s,"SMP1 ",5)) {
        s+=5;unsigned records;if(!sample_uint(&s,&records,PADS*9))goto fail;
        unsigned char used[PADS][9]={{0}};size_t bytes=0;
        for(unsigned i=0;i<records;i++) {
            unsigned p,o,on,count;if(!sample_uint(&s,&p,PADS-1)||!sample_uint(&s,&o,8)||!sample_uint(&s,&on,1)||!sample_uint(&s,&count,MAX_SAMPLES)||used[p][o])goto fail;used[p][o]=1;
            const char *name,*encoded;size_t namelen,enclen;if(!sample_token(&s,&name,&namelen)||!sample_token(&s,&encoded,&enclen))goto fail;
            r->source[p][o]=(int)on;
            if(!count){if(namelen!=1||*name!='-'||enclen!=1||*encoded!='-')goto fail;continue;}
            bytes+=(size_t)count*4;if(bytes>SAMPLE_BUDGET)goto fail;
            r->assets[p][o]=sample_from_text(count,name,namelen,encoded,enclen);if(!r->assets[p][o])goto fail;
        }
    }
    while(isspace((unsigned char)*s)){s++;}
    if(!strncmp(s,"FCT1 ",5)) {
        s+=5;unsigned count;unsigned char used[PADS]={0};
        if(!sample_uint(&s,&count,PADS))goto fail;
        for(unsigned j=0;j<count;j++) {
            unsigned pad,id;const char *digest;size_t size;
            if(!sample_uint(&s,&pad,PADS-1)||used[pad]||!sample_uint(&s,&id,FACTORY_COUNT)||!id||!sample_token(&s,&digest,&size))goto fail;
            if(size!=64||memcmp(digest,factory_sounds[id-1].digest,64))goto fail;
            used[pad]=1;r->factory_id[pad]=(int)id;
        }
        while(isspace((unsigned char)*s)){s++;}
    }
    if(!strncmp(s,"PRG1 ",5)){
        s+=5;unsigned program;if(!sample_uint(&s,&program,BANK_PROGRAM_COUNT-1))goto fail;r->program=(int)program;
        while(isspace((unsigned char)*s))s++;
    }else{
        /*0.0.29 snapshots predate menu metadata. Recover their program from the
         * complete, immutable source identity vector, never a guessed knob. */
        for(int p=9;p<BANK_PROGRAM_COUNT;p++){
            int equal=1;for(int pad=0;pad<PADS;pad++)if(r->factory_id[pad]!=bank_programs[p].ids[pad]){equal=0;break;}
            if(equal){r->program=p;break;}
        }
    }
    if(*s){goto fail;}
    return r;
fail:sample_restore_free(r);return NULL;
}
static void sample_restore_commit(Engine *e,SampleRestore *r) {
    e->bank_program=r->program;
    e->samples.bytes=0;
    for(int p=0;p<PADS;p++) {
        int audio=0,view=0;
        if(r->factory_id[p]!=atomic_load(&e->factory_id[p]))audio=1;
        atomic_store(&e->factory_id[p],r->factory_id[p]);
        for(int o=0;o<9;o++) {
            SampleAsset *a=r->assets[p][o],*old=atomic_exchange(&e->samples.assets[p][o],a);r->assets[p][o]=NULL;
            if(a){e->samples.bytes+=(size_t)a->count*4;}
            if(a||old || r->source[p][o]!=atomic_load(&e->samples.source[p][o]))audio=1;
            sample_retire(e,old);atomic_store(&e->samples.source[p][o],r->source[p][o]);atomic_fetch_add(&e->samples.generation[p][o],1);
        }
        for(int l=0;l<3;l++){if(r->focus[p][l]!=atomic_load(&e->sound_focus[p][l]))view=1;atomic_store(&e->sound_focus[p][l],r->focus[p][l]);}
        if(audio){atomic_fetch_add(&e->pad_audio[p],1);}
        if(audio||view)atomic_fetch_add(&e->pad_view[p],1);
    }
    sample_notify(e);
}
static int sample_state_append(Engine *e,char *out,int len,int n) {
#define SAMPLE_APPEND(...) do{if(n>=len)return 0;int add=snprintf(out+n,len-n,__VA_ARGS__);if(add<0||add>=len-n)return 0;n+=add;}while(0)
    int needview=0,records=0;
    for(int p=0;p<PADS;p++)for(int l=0;l<3;l++){int osc=(int)pad_at(e,p,"plot_osc"),expected=osc/3==l?osc:l*3;if(atomic_load(&e->sound_focus[p][l])!=expected)needview=1;}
    if(needview){SAMPLE_APPEND(" VIEW1");for(int p=0;p<PADS;p++)for(int l=0;l<3;l++)SAMPLE_APPEND(" %d",atomic_load(&e->sound_focus[p][l]));}
    for(int p=0;p<PADS;p++)for(int o=0;o<9;o++)if(atomic_load(&e->samples.assets[p][o]) || atomic_load(&e->samples.source[p][o]))records++;
    if(records){SAMPLE_APPEND(" SMP1 %d",records);atomic_fetch_add(&e->samples.readers,1);
        for(int p=0;p<PADS;p++)for(int o=0;o<9;o++) {
            SampleAsset *a=atomic_load(&e->samples.assets[p][o]);int on=atomic_load(&e->samples.source[p][o]);if(!a&&!on)continue;
            int add=snprintf(out+n,len-n," %d %d %d %u %s ",p,o,on,a?a->count:0,a?a->hexname:"-");
            if(add<0||add>=len-n){sample_release(e);return 0;}n+=add;
            size_t bytes=a?a->encoded_len:1;if(bytes>=(size_t)(len-n)){sample_release(e);return 0;}
            memcpy(out+n,a?a->encoded:"-",bytes);n+=(int)bytes;out[n]=0;
        }sample_release(e);
    }
    int imported=0;for(int p=0;p<PADS;p++)if(atomic_load(&e->factory_id[p]))imported++;
    if(imported){SAMPLE_APPEND(" FCT1 %d",imported);for(int p=0;p<PADS;p++){int id=atomic_load(&e->factory_id[p]);if(id)SAMPLE_APPEND(" %d %d %s",p,id,factory_sounds[id-1].digest);}}
    if(e->bank_program>=8)SAMPLE_APPEND(" PRG1 %d",e->bank_program);
#undef SAMPLE_APPEND
    return n;
}
