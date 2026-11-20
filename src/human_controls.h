/* Original humanizer amount adapters; fine hardware, direct Touch. GPL-3.0-or-later. */
static int human_control(const char *key,int *timing,int *fine){
 *timing=!strcmp(key,"human_timing_fine")||!strcmp(key,"human_timing_touch");
 *fine=strstr(key,"_fine")!=NULL;
 return *timing || !strcmp(key,"human_velocity_fine") || !strcmp(key,"human_velocity_touch");
}
static int human_get(Engine *e,const char *key,char *buf,int len){
 char copy[64];size_t n=strlen(key);int display=0,timing,fine;
 if(n>=8 && !strcmp(key+n-8,"_display")){if(n-8>=sizeof copy)return 0;memcpy(copy,key,n-8);copy[n-8]=0;key=copy;display=1;}
 if(!human_control(key,&timing,&fine))return 0;
 float physical=at(e,timing?"human_timing":"human_velocity");
 if(display)return snprintf(buf,len,timing?"%.2f ms":"+/- %.1f %%",physical);
 return snprintf(buf,len,"%.9g",fine?roundf(physical*(timing?4:10)):physical/30);
}
static int human_set(Engine *e,const char *key,const char *text){
 int timing,fine;if(!human_control(key,&timing,&fine))return 0;
 char *end;float v=strtof(text,&end);if(end==text||*end||!isfinite(v))return 1;
 const char *target=timing?"human_timing":"human_velocity";float current=at(e,target),scale=timing?4:10;
 if(fine?v==roundf(current*scale):fabsf(v-current/30)<1e-7f)return 1;
 char value[48];snprintf(value,sizeof value,"%.9g",clampf(fine?v/scale:v*30,0,30));set_param(e,target,value);return 1;
}
