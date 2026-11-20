/* Native fixed-point editor; GPL-3.0-or-later. Included after Engine helpers. */
#define POINT_EPS 0.0001f
#define POINT_ALL 63u
static const char *point_notifications[]={"point_level_fine","point_level_drag","point_time_fine","point_time_drag","ui_point_target","ui_point_mode"};
typedef struct { int osc,kind,number;char field; } PointKey;
static void point_key(char *key,int osc,int kind,const char *suffix){snprintf(key,48,"o%d_%c_%s",osc+1,kind?'p':'a',suffix);}
static float point_at(Engine *e,int osc,int kind,const char *suffix){char key[48];point_key(key,osc,kind,suffix);return at(e,key);}
static float point_value(const float *p,int osc,int kind,const char *suffix){char key[48];point_key(key,osc,kind,suffix);return value(p,key);}
static void point_store_key(Engine *e,const char *key,float v){int i=index_of(key);if(i>=0 && atomic_load(&e->values[i])!=v){atomic_store(&e->values[i],v);e->dirty[i]=1;}}
static void point_store(Engine *e,int osc,int kind,const char *suffix,float v){char key[48];point_key(key,osc,kind,suffix);point_store_key(e,key,v);}
static int point_decode(const char *key,PointKey *out){int o;char kind,field[16];if(sscanf(key,"o%d_%c_%15s",&o,&kind,field)!=3 || o<1 || o>9 || (kind!='a'&&kind!='p'))return 0;
 out->osc=o-1;out->kind=kind=='p';out->number=-1;
 if(!strcmp(field,"mode"))out->field='m';else if(!strcmp(field,"seen"))out->field='s';
 else if(strlen(field)==2 && field[0]=='l' && field[1]>='0'&&field[1]<='4'){out->field='l';out->number=field[1]-'0';}
 else if(strlen(field)==2 && field[0]=='t' && field[1]>='1'&&field[1]<='4'){out->field='t';out->number=field[1]-'0';}else return 0;
 return 1;}
static float point_upper(float base){return fminf(20000,base*16);}
static float point_base(Engine *e,int osc){char key[32];snprintf(key,sizeof key,"o%d_freq",osc+1);return at(e,key);}
static void point_focus(Engine *e,int osc,int kind,int selected){
 if((int)at(e,"plot_osc")!=osc || (int)at(e,"plot_env")!=kind)e->point_dirty|=POINT_ALL;
 point_store_key(e,"plot_osc",osc);point_store_key(e,"plot_env",kind);
 if(selected>=0 && (int)at(e,"point_select")!=selected){point_store_key(e,"point_select",selected);e->point_dirty|=15;}
}
static void point_seed(Engine *e,int osc,int kind){
 e->edit_time_valid=0;
 char free_count[48];snprintf(free_count,sizeof free_count,"f%d_%c_n",osc+1,kind?'p':'a');put(e,free_count,0);
 struct gkick_envelope_point_info pts[5];int n=0;float length=at(e,"length");char key[40];
 if(!kind){
  snprintf(key,sizeof key,"o%d_attack",osc+1);float attack=at(e,key);snprintf(key,sizeof key,"o%d_decay",osc+1);float decay=at(e,key);
  pts[n++]=(struct gkick_envelope_point_info){0,attack>0?0:1,false};
  if(attack>0 && attack<length)pts[n++]=(struct gkick_envelope_point_info){attack/length,1,false};
  if(attack+decay<length)pts[n++]=(struct gkick_envelope_point_info){(attack+decay)/length,0,false};
  float last=attack>=length?length/attack:clampf(1-(length-attack)/decay,0,1);pts[n++]=(struct gkick_envelope_point_info){1,last,false};
 }else{
  float base=point_base(e,osc),upper=point_upper(base);snprintf(key,sizeof key,"o%d_sweep",osc+1);float start=fminf(20000,base*exp2f(at(e,key)/12));
  snprintf(key,sizeof key,"o%d_sweep_time",osc+1);float time=at(e,key);float end=start+(base-start)*fminf(1,length/time);
  pts[n++]=(struct gkick_envelope_point_info){0,(start-20)/(upper-20),false};
  if(time<length)pts[n++]=(struct gkick_envelope_point_info){time/length,(base-20)/(upper-20),false};
  pts[n++]=(struct gkick_envelope_point_info){1,(end-20)/(upper-20),false};
 }
 while(n<5){int best=0;for(int j=1;j<n-1;j++)if(pts[j+1].x-pts[j].x>pts[best+1].x-pts[best].x)best=j;
  for(int j=n;j>best+1;j--)pts[j]=pts[j-1];
  pts[best+1]=(struct gkick_envelope_point_info){(pts[best].x+pts[best+2].x)*0.5f,(pts[best].y+pts[best+2].y)*0.5f,false};n++;
 }
 for(int j=0;j<5;j++){char suffix[32];snprintf(suffix,sizeof suffix,"l%d",j);point_store(e,osc,kind,suffix,clampf(pts[j].y,0,1));
  if(j){snprintf(suffix,sizeof suffix,"t%d",j);point_store(e,osc,kind,suffix,fmaxf(POINT_EPS,pts[j].x-pts[j-1].x));}}
 point_store(e,osc,kind,"seen",1);e->point_dirty|=POINT_ALL;e->free_dirty=0x7fffu;
}
static void point_prepare(Engine *e,const PointKey *key,float v){if((key->field=='l'||key->field=='t'||(key->field=='m'&&v>0))&&!point_at(e,key->osc,key->kind,"seen"))point_seed(e,key->osc,key->kind);}
static void point_changed(Engine *e,const PointKey *key){
 if(key->field=='l'||key->field=='t'){
  point_store(e,key->osc,key->kind,"mode",1);point_store(e,key->osc,key->kind,"seen",1);point_focus(e,key->osc,key->kind,key->number);
  unsigned mask=key->field=='l'?(e->point_origin==1?2:3):(e->point_origin==1?8:12);e->point_dirty|=mask|32;
 }else if(key->field=='m'){point_focus(e,key->osc,key->kind,-1);e->point_dirty|=POINT_ALL;}
}
static void point_apply(const float *p,int osc,int kind,struct gkick_envelope *env,float upper){
 struct gkick_envelope_point_info pts[5];float weights[4],total=0,position=0;
 for(int j=0;j<4;j++){char suffix[32];snprintf(suffix,sizeof suffix,"t%d",j+1);weights[j]=fmaxf(POINT_EPS,point_value(p,osc,kind,suffix));total+=weights[j];}
 for(int j=0;j<5;j++){char suffix[32];snprintf(suffix,sizeof suffix,"l%d",j);float y=clampf(point_value(p,osc,kind,suffix),0,1);
  if(kind)y=(20+(upper-20)*y)/upper;
  if(j)position+=weights[j-1]/total;
  pts[j]=(struct gkick_envelope_point_info){j==4?1:position,y,false};
 }
 gkick_envelope_set_points(env,pts,5);gkick_envelope_set_apply_type(env,GEONKICK_ENVELOPE_APPLY_LINEAR);
}
static int point_selected(Engine *e,int *osc,int *kind,int *point){*osc=(int)at(e,"plot_osc");*kind=(int)at(e,"plot_env");*point=(int)at(e,"point_select");return *osc*2+*kind;}
static int point_virtual_set(Engine *e,const char *key,const char *text){
 int level=!strcmp(key,"point_level_fine")||!strcmp(key,"point_level_drag"),time=!strcmp(key,"point_time_fine")||!strcmp(key,"point_time_drag"),copy=!strcmp(key,"point_copy");
 if(!level&&!time&&!copy)return 0;
 char *end;float v=strtof(text,&end);if(end==text||*end||!isfinite(v))return 1;
 int osc,kind,selected;point_selected(e,&osc,&kind,&selected);
 if(copy){if(v>0){atomic_fetch_add(&e->revision,1);point_seed(e,osc,kind);point_store(e,osc,kind,"mode",1);audio_changed(e);atomic_fetch_add(&e->revision,1);}return 1;}
 if(time && selected==0)return 1;
 char target[48],suffix[8],value_text[48];int fine=strstr(key,"_fine")!=NULL;
 unsigned origin=e->point_origin;e->point_origin=fine?1:2;
 if(!point_at(e,osc,kind,"mode")){point_key(target,osc,kind,"mode");set_param(e,target,"1");}
 snprintf(suffix,sizeof suffix,"%c%d",level?'l':'t',selected);point_key(target,osc,kind,suffix);
 snprintf(value_text,sizeof value_text,"%.9g",clampf(fine?v/10000.f:v,time?POINT_EPS:0,1));set_param(e,target,value_text);e->point_origin=origin;return 1;
}
static int point_virtual_get(Engine *e,const char *key,char *buf,int len){
 if(strncmp(key,"point_",6) && strncmp(key,"ui_point_",9))return 0;
 int osc,kind,selected;int target=point_selected(e,&osc,&kind,&selected);
 if(!strcmp(key,"ui_point_target"))return snprintf(buf,len,"%d",target);
 if(!strcmp(key,"ui_point_mode"))return snprintf(buf,len,"%.0f",point_at(e,osc,kind,"mode"));
 if(!strcmp(key,"point_copy"))return snprintf(buf,len,"0");
 int level=!strncmp(key,"point_level_",12),time=!strncmp(key,"point_time_",11);if(!level&&!time)return 0;
 char suffix[32];snprintf(suffix,sizeof suffix,"%c%d",level?'l':'t',selected);float v=(time&&selected==0)?0:point_at(e,osc,kind,suffix);
 if(strstr(key,"_display")){
  if(level){if(!kind)return snprintf(buf,len,"%.2f %%",v*100);float upper=point_upper(point_base(e,osc));return snprintf(buf,len,"%.2f Hz",20+(upper-20)*v);}
  if(!selected)return snprintf(buf,len,"START / 0 ms");
  float total=0;for(int j=1;j<=4;j++){snprintf(suffix,sizeof suffix,"t%d",j);total+=point_at(e,osc,kind,suffix);}return snprintf(buf,len,"%.2f ms",1000*at(e,"length")*v/total);
 }
 return snprintf(buf,len,"%.9g",strstr(key,"_fine")?roundf(v*10000):v);
}
