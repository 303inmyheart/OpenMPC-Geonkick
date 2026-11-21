/* Factory data overlays only the parameters absent from the MPC model. All
 * work here runs on the existing synthesis worker, never the audio callback. */
static void factory_envelope(struct gkick_envelope *env,const FactoryCurve *curve){
    gkick_envelope_set_points(env,curve->point,(size_t)curve->count);
    gkick_envelope_set_apply_type(env,curve->apply);
}
static void factory_flat(struct gkick_envelope *env){
    struct gkick_envelope_point_info points[]={{0,1,false},{1,1,false}};
    gkick_envelope_set_points(env,points,2);gkick_envelope_set_apply_type(env,GEONKICK_ENVELOPE_APPLY_LINEAR);
}
static void factory_reset_dsp(Engine *e){
    if(!e->factory_was_configured)return;
    struct gkick_synth *s=e->synth;factory_flat(s->envelope);
    factory_flat(s->filter->cutoff_env);factory_flat(s->filter->q_env);
    factory_flat(s->distortion->drive_env);factory_flat(s->distortion->volume_env);s->distortion->in_limiter=1;
    for(int j=0;j<9;j++){
        struct gkick_oscillator *o=s->oscillators[j];o->pitch_shift=0;o->fm_input=0;
        factory_flat(o->envelopes[GKICK_OSC_NOISE_DENSITY_ENVELOPE]);
        factory_flat(o->envelopes[GKICK_OSC_PITCH_SHIFT_ENVELOPE]);
        factory_flat(o->filter->cutoff_env);factory_flat(o->filter->q_env);
        factory_flat(o->distortion->drive_env);factory_flat(o->distortion->volume_env);o->distortion->in_limiter=1;
    }
    e->factory_was_configured=0;
}
static int factory_same(const float *p,const FactorySound *f,const char *key){int j=index_of(key);return j>=0&&p[j]==f->baseline[j];}
static float factory_scalar(const float *p,const FactorySound *f,const char *prefix,const char *key,float original){
    char full[48];snprintf(full,sizeof full,"%s%s",prefix,key);return factory_same(p,f,full)?original:value(p,full);
}
static int factory_curve_same(const float *p,const FactorySound *f,int osc,int kind){
    char key[32];snprintf(key,sizeof key,"f%d_%c_n",osc,kind?'p':'a');int start=index_of(key);
    /* Each original free curve has one count,16 x/y pairs and30 bends. */
    for(int j=start;j<start+63;j++)if(p[j]!=f->baseline[j])return 0;
    snprintf(key,sizeof key,"o%d_%c_mode",osc,kind?'p':'a');start=index_of(key);
    for(int j=start;j<start+11;j++)if(p[j]!=f->baseline[j])return 0;
    if(!kind){snprintf(key,sizeof key,"o%d_attack",osc);if(!factory_same(p,f,key))return 0;snprintf(key,sizeof key,"o%d_decay",osc);if(!factory_same(p,f,key))return 0;}
    else {snprintf(key,sizeof key,"o%d_sweep",osc);if(!factory_same(p,f,key))return 0;snprintf(key,sizeof key,"o%d_sweep_time",osc);if(!factory_same(p,f,key))return 0;}
    return 1;
}
static void factory_configure(Engine *e,const float *p){
    int id=(int)p[GP_COUNT+12];if(!id)return;
    const FactorySound *f=&factory_sounds[id-1];struct gkick_synth *s=e->synth;e->factory_was_configured=1;
    s->amplitude=f->scalar[0][FA_LEVEL];factory_envelope(s->envelope,&f->curves[0][FC_AMP]);
    for(int j=0;j<FACTORY_UNITS;j++){
        const float *raw=f->scalar[j];const FactoryCurve *curves=f->curves[j];char prefix[16]="";
        struct gkick_filter *filter=s->filter;struct gkick_distortion *drive=s->distortion;
        if(j){
            snprintf(prefix,sizeof prefix,"o%d_",j);struct gkick_oscillator *o=s->oscillators[j-1];filter=o->filter;drive=o->distortion;
            o->amplitude=factory_scalar(p,f,prefix,"level",raw[FA_LEVEL]);
            char phase_key[32];snprintf(phase_key,sizeof phase_key,"%sphase",prefix);
            o->initial_phase=factory_same(p,f,phase_key)?raw[FA_PHASE]:value(p,phase_key)*(float)M_PI/180;
            o->noise_density=factory_scalar(p,f,prefix,"density",raw[FA_DENSITY]);
            o->seed=(unsigned)raw[FA_SEED];o->fm_input=0;
            char key[24];snprintf(key,sizeof key,"o%d_fm",j);
            if((j-1)%3==0)o->is_fm=factory_same(p,f,key)?raw[FA_FM]>0:value(p,key)>0;
            o->fm_k=raw[FA_FMK];
            if((j-1)%3==1){snprintf(key,sizeof key,"o%d_fm",j-1);if(!factory_same(p,f,key))o->fm_k=value(p,key)*8;}
            int sample=p[GP_COUNT+j-1]>0;
            if(factory_curve_same(p,f,j,0))factory_envelope(o->envelopes[GKICK_OSC_AMPLITUDE_ENVELOPE],curves+FC_AMP);
            int same_pitch=factory_curve_same(p,f,j,1);
            if(same_pitch){
                o->frequency=factory_scalar(p,f,prefix,"freq",raw[FA_FREQ]);
                factory_envelope(o->envelopes[GKICK_OSC_FREQUENCY_ENVELOPE],curves+FC_FREQ);
            }
            if(sample&&same_pitch){
                o->pitch_shift=raw[FA_PITCH];
                factory_envelope(o->envelopes[GKICK_OSC_PITCH_SHIFT_ENVELOPE],curves+FC_PITCH);
                /* Source sample pitch is independent of tone frequency. Make a
                 * hardware/touch frequency edit transpose the original sample. */
                snprintf(key,sizeof key,"o%d_freq",j);
                float table[FREE_TABLE],pitch_table[FREE_TABLE],base=value(p,key),shift=12*log2f(base/f->baseline[index_of(key)]);
                o->frequency=point_upper(base);
                for(int n=0;n<FREE_TABLE;n++){
                    float y=gkick_envelope_get_value(o->envelopes[GKICK_OSC_PITCH_SHIFT_ENVELOPE],n/(float)(FREE_TABLE-1));
                    float semitones=(2*y-1)*raw[FA_PITCH]+shift;
                    pitch_table[n]=.5f+semitones/240.f;table[n]=55*exp2f(semitones/12)/o->frequency;
                }
                if(!factory_same(p,f,key)){o->pitch_shift=120;gkick_envelope_set_table(o->envelopes[GKICK_OSC_PITCH_SHIFT_ENVELOPE],pitch_table,FREE_TABLE);}
                gkick_envelope_set_table(o->envelopes[GKICK_OSC_FREQUENCY_ENVELOPE],table,FREE_TABLE);
                gkick_envelope_set_apply_type(o->envelopes[GKICK_OSC_FREQUENCY_ENVELOPE],GEONKICK_ENVELOPE_APPLY_LINEAR);
            }
            factory_envelope(o->envelopes[GKICK_OSC_NOISE_DENSITY_ENVELOPE],curves+FC_DENSITY);
        }
        char model[40];snprintf(model,sizeof model,"%sfilter_model",prefix);
        if(value(p,model)==0){
            gkick_filter_set_cutoff_freq(filter,factory_scalar(p,f,prefix,"cutoff",raw[FA_CUTOFF]));
            gkick_filter_set_factor(filter,factory_scalar(p,f,prefix,"q",raw[FA_Q]));
        }
        factory_envelope(filter->cutoff_env,curves+FC_CUTOFF);factory_envelope(filter->q_env,curves+FC_Q);
        drive->drive=factory_scalar(p,f,prefix,"drive",raw[FA_DRIVE]);drive->in_limiter=raw[FA_DRIVE_IN];
        drive->out_limiter=factory_scalar(p,f,prefix,"drive_out",raw[FA_DRIVE_OUT]);
        factory_envelope(drive->drive_env,curves+FC_DRIVE);factory_envelope(drive->volume_env,curves+FC_VOLUME);
    }
}
static float factory_pitch_plot(const float *p,int osc,struct gkick_oscillator *o,float shape){
    int id=(int)p[GP_COUNT+12];if(!id)return (shape*o->frequency-20)/(o->frequency-20);
    float hz=gkick_envelope_get_apply_type(o->envelopes[GKICK_OSC_FREQUENCY_ENVELOPE])==GEONKICK_ENVELOPE_APPLY_LOGARITHMIC?
        powf(10,(log10f(o->frequency)-GKICK_LOG20)*shape+GKICK_LOG20):shape*o->frequency;
    char key[32];snprintf(key,sizeof key,"o%d_freq",osc+1);return (hz-20)/(point_upper(value(p,key))-20);
}
static float factory_output(Engine *e,int pad){
    float current=pad_at(e,pad,"volume");int id=atomic_load(&e->factory_id[pad]);
    if(!id)return current;
    const FactorySound *f=&factory_sounds[id-1];
    return current==f->baseline[index_of("volume")]?f->limiter:current;
}
