/* Wide filter controls with frozen classic automation; GPL-3.0-or-later. */
static void filter_key(char *out,const char *prefix,const char *suffix){snprintf(out,48,"%s%s",prefix,suffix);}
static float filter_at(Engine *e,const char *prefix,const char *suffix){char key[48];filter_key(key,prefix,suffix);return at(e,key);}
static float filter_value(const float *p,const char *prefix,const char *suffix){char key[48];filter_key(key,prefix,suffix);return value(p,key);}
static int filter_decode(const char *key,char *prefix,const char **suffix){
 prefix[0]=0;*suffix=key;
 if(key[0]=='o' && key[1]>='1' && key[1]<='9' && key[2]=='_'){memcpy(prefix,key,3);prefix[3]=0;*suffix=key+3;}
 return !strcmp(*suffix,"cutoff_fine") || !strcmp(*suffix,"cutoff_touch") || !strcmp(*suffix,"res_fine") || !strcmp(*suffix,"res_touch");
}
static float filter_coordinate(Engine *e,const char *prefix,int resonance){
 int wide=filter_at(e,prefix,"filter_model")>0;
 if(resonance){float q=filter_at(e,prefix,wide?"q_wide":"q");return wide?logf(q/.5f)/logf(40):logf(q/10.f)/logf(100);}
 return logf(filter_at(e,prefix,wide?"cutoff_wide":"cutoff")/20.f)/logf(1000);
}
static int filter_virtual_get(Engine *e,const char *key,char *buf,int len){
 char prefix[8],copy[64];const char *suffix;int display=0;size_t n=strlen(key);
 if(n>=8 && !strcmp(key+n-8,"_display")){if(n-8>=sizeof copy)return 0;memcpy(copy,key,n-8);copy[n-8]=0;key=copy;display=1;}
 if(!filter_decode(key,prefix,&suffix))return 0;
 int resonance=!strncmp(suffix,"res_",4),fine=strstr(suffix,"_fine")!=NULL;
 float coordinate=filter_coordinate(e,prefix,resonance);
 if(display){
  int wide=filter_at(e,prefix,"filter_model")>0;
  if(!resonance)return snprintf(buf,len,"%.2f Hz",filter_at(e,prefix,wide?"cutoff_wide":"cutoff"));
  if(wide)return snprintf(buf,len,"%.1f %% / Q %.2f",coordinate*100,filter_at(e,prefix,"q_wide"));
  return snprintf(buf,len,"%.1f %% / CLASSIC",coordinate*100);
 }
 float result=fine?(resonance?roundf(coordinate*1000):roundf(coordinate*log2f(1000)*240)):coordinate;
 return snprintf(buf,len,"%.9g",result);
}
static int filter_virtual_set(Engine *e,const char *key,const char *text){
 char prefix[8];const char *suffix;if(!filter_decode(key,prefix,&suffix))return 0;
 char *end;float v=strtof(text,&end);if(end==text||*end||!isfinite(v))return 1;
 int resonance=!strncmp(suffix,"res_",4),fine=strstr(suffix,"_fine")!=NULL,wide=filter_at(e,prefix,"filter_model")>0;
 float current=filter_coordinate(e,prefix,resonance),scale=resonance?1000:log2f(1000)*240;
 if(fine?v==roundf(current*scale):fabsf(v-current)<1e-7f)return 1;
 float coordinate=clampf(fine?v/scale:v,0,1);
 float physical=resonance?(wide?.5f*powf(40,coordinate):10*powf(100,coordinate)):20*powf(1000,coordinate);
 if(!resonance)physical=clampf(physical,wide?20:30,wide?20000:8000);
 char target[48],val[48];filter_key(target,prefix,resonance?(wide?"q_wide":"q"):(wide?"cutoff_wide":"cutoff"));
 snprintf(val,sizeof val,"%.9g",physical);set_param(e,target,val);return 1;
}
static void filter_migrate(float *p){
 for(int j=0;j<10;j++){
  char prefix[8]="",key[48];if(j)snprintf(prefix,sizeof prefix,"o%d_",j);
  filter_key(key,prefix,"filter_model");p[index_of(key)]=0;
  filter_key(key,prefix,"cutoff_wide");p[index_of(key)]=filter_value(p,prefix,"cutoff");
  filter_key(key,prefix,"q_wide");p[index_of(key)]=clampf(filter_value(p,prefix,"q")/10,.5f,20);
 }
}
static void filter_configure(struct gkick_filter *f,const float *p,const char *prefix){
 int wide=filter_value(p,prefix,"filter_model")>0;
 gkick_filter_set_wide(f,wide,filter_value(p,prefix,"q_wide"));
 gkick_filter_set_cutoff_freq(f,filter_value(p,prefix,wide?"cutoff_wide":"cutoff"));
 gkick_filter_set_factor(f,filter_value(p,prefix,"q"));
}
/* Old automation keeps its original range. In WIDE it also updates the same
 * frequency / bounded equivalent Q; old projects themselves stay CLASSIC. */
static void filter_legacy_changed(Engine *e,const char *key,float v){
 char prefix[8]="",target[48];const char *suffix=key;
 if(key[0]=='o' && key[1]>='1'&&key[1]<='9'&&key[2]=='_'){memcpy(prefix,key,3);prefix[3]=0;suffix=key+3;}
 if(strcmp(suffix,"cutoff") && strcmp(suffix,"q"))return;
 if(filter_at(e,prefix,"filter_model")<=0)return;
 filter_key(target,prefix,!strcmp(suffix,"q")?"q_wide":"cutoff_wide");
 int i=index_of(target);atomic_store(&e->values[i],!strcmp(suffix,"q")?clampf(v/10,.5f,20):v);e->dirty[i]=1;
}
