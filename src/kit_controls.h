/* Sixteen independent instruments using one worker. GPL-3.0-or-later.
 * Only serialized engine entry points change rows/selection; the worker uses
 * fixed atomic rows and the revision seqlock, never the editing pointer. */
static const char *kit_notifications[]={"kit_pad","kit_mode","kit_base","kit_output","kit_info","kit_title","kit_mapping","kit_learn","kit_learn_status"};
#define KIT_NOTIFY_COUNT (sizeof kit_notifications/sizeof *kit_notifications)
static void kit_feedback(Engine *e) {
    e->edit_time_valid=0;
    /* Raw private f*_ curve storage has no placed UI and does not need 1100
     * individual notifications on every selection. Native point/XY adapters
     * and display revisions expose the selected curve without that burst. */
    memset(e->dirty,0,sizeof e->dirty);memset(e->dirty,1,GKMP4_GP_COUNT);
    memset(e->dirty+GKMP5_GP_COUNT,1,GP_COUNT-GKMP5_GP_COUNT);
    memset(e->dirty_alias,1,sizeof e->dirty_alias);
    e->point_dirty|=POINT_ALL;e->free_dirty=(1u<<FREE_NOTIFY_COUNT)-1;
    e->kit_dirty=(1u<<KIT_NOTIFY_COUNT)-1;++e->ui_revision;
}
static void kit_select(Engine *e,int pad) {
    if(pad==atomic_load(&e->selected))return;
    atomic_fetch_add(&e->revision,1);
    e->values=e->pad_values[pad];atomic_store(&e->selected,pad);e->current=e->pad_current[pad];
    kit_feedback(e);++e->display_revision;++e->control_context;
    atomic_fetch_add(&e->revision,1);
}
static void kit_map_feedback(Engine *e) {e->kit_dirty=(1u<<KIT_NOTIFY_COUNT)-1;++e->ui_revision;++e->display_revision;}
static int kit_assigned_note(Engine *e,int pad){return e->kit_mapping?e->kit_notes[pad]:e->kit_base+pad;}
static int kit_route(Engine *e,int note) {
    if(!e->kit_mapping)return note-e->kit_base;
    for(int pad=0;pad<PADS;pad++)if(e->kit_notes[pad]==note)return pad;
    return -1;
}
static int kit_capture_note(Engine *e,int note) {
    int pad=e->kit_learn_count;
    for(int i=0;i<pad;i++)if(e->kit_stage[i]==note){if(!e->kit_duplicate){e->kit_duplicate=1;kit_map_feedback(e);}return -1;}
    e->kit_duplicate=0;e->kit_stage[pad]=note;kit_select(e,pad);
    if(++e->kit_learn_count==PADS) {
        /* Commit only a complete, unique map. Cancel/partial learning leaves
         * the previous playable/project mapping intact. No allocation or IO. */
        memcpy(e->kit_notes,e->kit_stage,sizeof e->kit_notes);
        e->kit_mapping=1;e->kit_learn=0;e->kit_learn_done=1;
    }
    kit_map_feedback(e);return pad;
}
static int kit_set(Engine *e,const char *key,const char *text) {
    if(strcmp(key,"kit_pad") && strcmp(key,"kit_mode") && strcmp(key,"kit_base") && strcmp(key,"kit_output") && strcmp(key,"kit_mapping") && strcmp(key,"kit_learn"))return !strncmp(key,"kit_",4);
    char *end;float v=strtof(text,&end);if(end==text || *end || !isfinite(v))return 1;
    if(!strcmp(key,"kit_pad")){kit_select(e,(int)roundf(clampf(v,0,PADS-1)));return 1;}
    if(!strcmp(key,"kit_output")){e->kit_output=clampf(v,0,1);return 1;}
    int n=(int)roundf(clampf(v,0,!strcmp(key,"kit_base")?112:1));
    if(!strcmp(key,"kit_learn")) {
        if(n==e->kit_learn)return 1;
        e->kit_learn=n;e->kit_learn_count=0;e->kit_duplicate=0;e->kit_learn_done=0;
        if(n){stop_all(e);e->kit_mode=1;kit_select(e,0);for(int p=0;p<PADS;p++)e->kit_stage[p]=-1;}
        kit_map_feedback(e);return 1;
    }
    int *target=!strcmp(key,"kit_mode")?&e->kit_mode:!strcmp(key,"kit_base")?&e->kit_base:&e->kit_mapping;
    if(n!=*target){stop_all(e);*target=n;e->kit_learn=0;e->kit_learn_count=0;e->kit_learn_done=0;kit_map_feedback(e);}
    return 1;
}
static const char *kit_note(int note,char *out,size_t size) {
    static const char *names[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    snprintf(out,size,"%d / %s%d",note,names[note%12],note/12-1);return out;
}
static int kit_get(Engine *e,const char *key,char *out,int len) {
    int pad=atomic_load(&e->selected);char a[32],b[32];
    if(!strcmp(key,"kit_pad"))return snprintf(out,len,"%d",pad);
    if(!strcmp(key,"kit_mapping"))return snprintf(out,len,"%d",e->kit_mapping);
    if(!strcmp(key,"kit_learn"))return snprintf(out,len,"%d",e->kit_learn);
    if(!strcmp(key,"kit_learn_status")) {
        if(e->kit_learn)return snprintf(out,len,e->kit_duplicate?"SAME NOTE; TRY PAD %02d":"PRESS PAD %02d / 16",e->kit_learn_count+1);
        return snprintf(out,len,"%s",e->kit_learn_done?"16 PADS LEARNED":e->kit_mapping?"LEARNED MAP ACTIVE":"READY TO LEARN");
    }
    if(!strcmp(key,"kit_mode"))return snprintf(out,len,"%d",e->kit_mode);
    if(!strcmp(key,"kit_base"))return snprintf(out,len,"%d",e->kit_base);
    if(!strcmp(key,"kit_output"))return snprintf(out,len,"%.9g",e->kit_output);
    if(!strcmp(key,"kit_base_display"))return snprintf(out,len,"%s",kit_note(e->kit_base,a,sizeof a));
    if(!strcmp(key,"kit_info"))return e->kit_mode?
        snprintf(out,len,"PAD %02d: %s",pad+1,kit_note(kit_assigned_note(e,pad),a,sizeof a)):
        snprintf(out,len,"ALL NOTES -> PAD %02d",pad+1);
    if(!strcmp(key,"kit_title"))return snprintf(out,len,"PAD %02d",pad+1);
    if(!strcmp(key,"kit_range"))return snprintf(out,len,"%s - %s",kit_note(e->kit_base,a,sizeof a),kit_note(e->kit_base+15,b,sizeof b));
    if(!strcmp(key,"kit_info_on") || !strcmp(key,"kit_title_on") || !strcmp(key,"kit_learn_status_on"))return snprintf(out,len,"0");
    if(!strcmp(key,"kit_render_pending")) {
        adopt(e);int count=0;
        for(int p=0;p<PADS;p++)if(e->slots[e->pad_current[p]].revision!=atomic_load(&e->pad_audio[p]))count++;
        return snprintf(out,len,"%d",count);
    }
    return -1;
}
static int kit_number(const char **cursor,float *out,float lo,float hi,int integer) {
    char *end;float v=strtof(*cursor,&end);
    if(end==*cursor || !isfinite(v) || v<lo || v>hi || (integer && v!=roundf(v)))return 0;
    if(*end && *end!=' ' && *end!='\n' && *end!='\r' && *end!='\t')return 0;
    *cursor=end;*out=v;return 1;
}
static const char *kit_spaces(const char *s){while(*s==' ' || *s=='\n' || *s=='\r' || *s=='\t')++s;return s;}
static int kit_row(const char **cursor,float *row,int count) {
    for(int i=0;i<count;i++)if(!kit_number(cursor,row+i,gparams[i].lo,gparams[i].hi,gparams[i].discrete))return 0;
    return free_validate(row);
}
static void kit_load(Engine *e,const char *text) {
    int current=!strncmp(text,"GKMP9 ",6);
    int count=0,full=current || !strncmp(text,"GKMP8 ",6),legacy7=!strncmp(text,"GKMP7 ",6);
    if(full || legacy7 || !strncmp(text,"GKMP6 ",6))count=GP_COUNT;
    else if(!strncmp(text,"GKMP5 ",6))count=GKMP5_GP_COUNT;
    else if(!strncmp(text,"GKMP4 ",6))count=GKMP4_GP_COUNT;
    else if(!strncmp(text,"GKMP3 ",6))count=LEGACY_GP_COUNT;
    else if(!strncmp(text,"GKMP2 ",6))count=184;
    else if(!strncmp(text,"GKMP1 ",6))count=103;
    else return;
    /* Host state restore is not a MIDI/render operation. Validate every row
     * and all metadata before publishing anything; malformed kits are atomic. */
    float (*rows)[GP_COUNT]=malloc(sizeof(float)*PADS*GP_COUNT);if(!rows)return;
    for(int pad=0;pad<PADS;pad++)for(int i=0;i<GP_COUNT;i++)rows[pad][i]=gparams[i].def;
    const char *s=text+6;float first[GP_COUNT];memcpy(first,rows[0],sizeof first);
    if(!kit_row(&s,first,count))goto finish;
    float mode=0,base=36,selected=0,output=1;
    if(legacy7) {float ignored;if(!kit_number(&s,&ignored,0,1,1) || !kit_number(&s,&ignored,0,30,0) || !kit_number(&s,&ignored,0,30,0))goto finish;}
    if(count<=GKMP5_GP_COUNT)filter_migrate(first);
    if(full) {
        s=kit_spaces(s);if(strncmp(s,"KIT1 ",5))goto finish;s+=5;
        if(!kit_number(&s,&mode,0,1,1) || !kit_number(&s,&base,0,112,1) ||
           !kit_number(&s,&selected,0,PADS-1,1) || !kit_number(&s,&output,0,1,0))goto finish;
        for(int pad=0;pad<PADS;pad++)if(pad!=(int)selected) {
            s=kit_spaces(s);char tag=*s++;
            if(tag=='P'){if(!kit_row(&s,rows[pad],GP_COUNT))goto finish;}
            else if(tag!='D' || (*s && *s!=' ' && *s!='\n'))goto finish;
        }
    }
    float mapping=0;int notes[PADS];
    for(int pad=0;pad<PADS;pad++)notes[pad]=(int)base+pad;
    if(current) {
        s=kit_spaces(s);if(strncmp(s,"MAP1 ",5))goto finish;s+=5;
        if(!kit_number(&s,&mapping,0,1,1))goto finish;
        for(int pad=0;pad<PADS;pad++) {
            float note;if(!kit_number(&s,&note,0,127,1))goto finish;notes[pad]=(int)note;
            for(int i=0;i<pad;i++)if(notes[i]==notes[pad])goto finish;
        }
    }
    memcpy(rows[(int)selected],first,sizeof first);
    SampleRestore *samples=sample_restore_read(s,(const float (*)[GP_COUNT])rows);if(!samples)goto finish;
    atomic_fetch_add(&e->revision,1);stop_all(e);
    for(int pad=0;pad<PADS;pad++) {
        int changed=0;
        for(int i=0;i<GP_COUNT;i++) {
            if(rows[pad][i]!=atomic_load(&e->pad_values[pad][i]))changed=1;
            atomic_store(&e->pad_values[pad][i],rows[pad][i]);
        }
        if(changed){atomic_fetch_add(&e->pad_audio[pad],1);atomic_fetch_add(&e->pad_view[pad],1);}
    }
    e->kit_mode=(int)mode;e->kit_base=(int)base;e->kit_output=output;e->kit_mapping=(int)mapping;
    memcpy(e->kit_notes,notes,sizeof notes);e->kit_learn=0;e->kit_learn_count=0;e->kit_learn_done=0;e->kit_duplicate=0;
    atomic_store(&e->selected,(int)selected);e->values=e->pad_values[(int)selected];e->current=e->pad_current[(int)selected];
    sample_restore_commit(e,samples);sample_restore_free(samples);
    kit_feedback(e);++e->display_revision;++e->control_context;
    atomic_fetch_add(&e->revision,1);
finish:
    free(rows);
}
static int kit_save(Engine *e,char *out,int len) {
    int selected=atomic_load(&e->selected),n=snprintf(out,len,"GKMP9");
#define KIT_APPEND(...) do{if(n>=len)return 0;int added=snprintf(out+n,len-n,__VA_ARGS__);if(added<0 || added>=len-n)return 0;n+=added;}while(0)
    for(int i=0;i<GP_COUNT;i++)KIT_APPEND(" %.9g",atomic_load(&e->values[i]));
    KIT_APPEND(" KIT1 %d %d %d %.9g",e->kit_mode,e->kit_base,selected,e->kit_output);
    for(int pad=0;pad<PADS;pad++)if(pad!=selected) {
        int is_default=1;
        for(int i=0;i<GP_COUNT;i++)if(atomic_load(&e->pad_values[pad][i])!=gparams[i].def){is_default=0;break;}
        KIT_APPEND(is_default?" D":" P");
        if(!is_default)for(int i=0;i<GP_COUNT;i++)KIT_APPEND(" %.9g",atomic_load(&e->pad_values[pad][i]));
    }
    KIT_APPEND(" MAP1 %d",e->kit_mapping);
    for(int pad=0;pad<PADS;pad++)KIT_APPEND(" %d",e->kit_notes[pad]);
#undef KIT_APPEND
    return sample_state_append(e,out,len,n);
}
