/* Variable point envelopes and native five-grip viewport; GPL-3.0-or-later.
 * UI mutations run under the wrapper lock. DSP tables are built only by worker.
 * The original five-point parameters and their IDs remain supported. */
#define FREE_MAX 16
#define FREE_EPS 0.000001f
#define FREE_TABLE 4097
#define FREE_NOTIFY_COUNT 15
static const char *free_notifications[FREE_NOTIFY_COUNT]={
 "edit_l0","edit_l1","edit_l2","edit_l3","edit_l4",
 "edit_t1","edit_t2","edit_t3","edit_t4",
 "free_level","free_time","free_bend","free_status","free_select","free_bank"};
typedef struct {int n;float x[FREE_MAX],y[FREE_MAX],b[FREE_MAX-1],d[FREE_MAX-1];} FreeCurve;
static void free_key(char *key,int osc,int kind,const char *suffix){snprintf(key,48,"f%d_%c_%s",osc+1,kind?'p':'a',suffix);}
static float free_value(const float *p,int osc,int kind,const char *suffix){char key[48];free_key(key,osc,kind,suffix);return value(p,key);}
static void free_store(Engine *e,int osc,int kind,const char *suffix,float v){char key[48];free_key(key,osc,kind,suffix);int i=index_of(key);atomic_store(&e->values[i],v);}
static void free_read(const float *p,int osc,int kind,FreeCurve *c){
 memset(c,0,sizeof *c);c->n=(int)p[gfree_base[osc][kind]];char key[16];
 if(c->n>=2){
  int base=gfree_base[osc][kind];
  for(int j=0;j<c->n;j++){c->x[j]=p[base+1+j*2];c->y[j]=p[base+2+j*2];if(j<c->n-1){c->b[j]=p[base+33+j*2];c->d[j]=p[base+34+j*2];}}
 }else if(point_value(p,osc,kind,"seen")==0){
  float length=value(p,"length");char key[40];
  if(!kind){
   snprintf(key,sizeof key,"o%d_attack",osc+1);float attack=value(p,key);snprintf(key,sizeof key,"o%d_decay",osc+1);float decay=value(p,key);
   c->n=1;c->x[0]=0;c->y[0]=attack>0?0:1;
   if(attack>0 && attack<length){c->x[c->n]=attack/length;c->y[c->n++]=1;}
   if(attack+decay<length){c->x[c->n]=(attack+decay)/length;c->y[c->n++]=0;}
   c->x[c->n]=1;c->y[c->n++]=attack>=length?length/attack:clampf(1-(length-attack)/decay,0,1);
  }else{
   snprintf(key,sizeof key,"o%d_freq",osc+1);float base=value(p,key),upper=point_upper(base);
   snprintf(key,sizeof key,"o%d_sweep",osc+1);float start=fminf(20000,base*exp2f(value(p,key)/12));
   snprintf(key,sizeof key,"o%d_sweep_time",osc+1);float time=value(p,key);c->n=1;c->y[0]=(start-20)/(upper-20);
   if(time<length){c->x[c->n]=time/length;c->y[c->n++]=(base-20)/(upper-20);}
   c->x[c->n]=1;c->y[c->n++]=(start+(base-start)*fminf(1,length/time)-20)/(upper-20);
  }
 }else{
  c->n=5;float total=0;
  for(int j=1;j<=4;j++){snprintf(key,sizeof key,"t%d",j);total+=point_value(p,osc,kind,key);}
  for(int j=0;j<5;j++){snprintf(key,sizeof key,"l%d",j);c->y[j]=point_value(p,osc,kind,key);if(j){snprintf(key,sizeof key,"t%d",j);c->x[j]=c->x[j-1]+point_value(p,osc,kind,key)/total;}}c->x[4]=1;
 }
}
static void free_current(Engine *e,int osc,int kind,FreeCurve *c){
 float p[GP_COUNT];int base=gfree_base[osc][kind];
 for(int j=0;j<63;j++)p[base+j]=atomic_load(&e->values[base+j]);
 base=gpoint_base[osc][kind];for(int j=0;j<11;j++)p[base+j]=atomic_load(&e->values[base+j]);
 int i=index_of("length");p[i]=atomic_load(&e->values[i]);
 const char *fields[]={"attack","decay","freq","sweep","sweep_time"};
 for(int j=0;j<5;j++){char key[40];snprintf(key,sizeof key,"o%d_%s",osc+1,fields[j]);i=index_of(key);p[i]=atomic_load(&e->values[i]);}
 free_read(p,osc,kind,c);
}
static void free_write(Engine *e,int osc,int kind,const FreeCurve *c){int base=gfree_base[osc][kind];
 for(int j=0;j<FREE_MAX;j++){
  atomic_store(&e->values[base+1+j*2],j<c->n?c->x[j]:j/15.f);
  atomic_store(&e->values[base+2+j*2],j<c->n?c->y[j]:0);
  if(j<FREE_MAX-1){atomic_store(&e->values[base+33+j*2],j<c->n-1?c->b[j]:0);atomic_store(&e->values[base+34+j*2],j<c->n-1?c->d[j]:0);}
 }atomic_store(&e->values[base],c->n);
}
static float free_handle(float base,float bend){return base+(bend>0?bend*(1-base):bend*base);}
static float free_offset(float base,float handle){float span=handle>=base?1-base:base;return span>1e-8f?clampf((handle-base)/span,-1,1):0;}
static float free_shape(const FreeCurve *c,float x){
 int j=0;while(j<c->n-2 && x>c->x[j+1])j++;
 float t=clampf((x-c->x[j])/(c->x[j+1]-c->x[j]),0,1),a=c->y[j],b=c->y[j+1],bend=c->b[j];
 if(bend==0 && c->d[j]==0)return a+(b-a)*t;
 /* Cubic Bezier with x handles at 1/3 and 2/3: x(t)=t, so no
  * iterative inversion or pow() in synthesis. Bend moves both y handles. */
 float h1=a+(b-a)/3,h2=a+2*(b-a)/3;
 h1=free_handle(h1,bend);h2=free_handle(h2,c->d[j]);
 float u=1-t;return u*u*u*a+3*u*u*t*h1+3*u*t*t*h2+t*t*t*b;
}
static void free_apply(const float *p,int osc,int kind,struct gkick_envelope *env,float upper){
 if(free_value(p,osc,kind,"n")<2){point_apply(p,osc,kind,env,upper);return;}
 FreeCurve c;free_read(p,osc,kind,&c);float table[FREE_TABLE];
 for(int j=0;j<FREE_TABLE;j++){float y=free_shape(&c,j/(float)(FREE_TABLE-1));table[j]=kind?(20+(upper-20)*y)/upper:y;}
 gkick_envelope_set_table(env,table,FREE_TABLE);
}
static int free_validate(const float *p){
 for(int o=0;o<9;o++)for(int k=0;k<2;k++){
  float count=free_value(p,o,k,"n");if(count==0)continue;if(count<2 || count>FREE_MAX || count!=floorf(count))return 0;
  FreeCurve c;free_read(p,o,k,&c);if(c.x[0]!=0 || c.x[c.n-1]!=1)return 0;
  for(int j=1;j<c.n;j++)if(c.x[j]-c.x[j-1]<FREE_EPS*0.9f)return 0;
 }return 1;
}
static void free_context(Engine *e,int *o,int *k,int *s,FreeCurve *c){
 *o=(int)at(e,"plot_osc");*k=(int)at(e,"plot_env");free_current(e,*o,*k,c);*s=(int)at(e,"free_select");if(*s>=c->n)*s=c->n-1;
}
static void free_selection(Engine *e,int n,int selected){
 ++e->ui_revision;
 selected=selected<0?0:selected>=n?n-1:selected;put(e,"free_select",selected);
 if(selected>0 && selected<n-1)put(e,"free_bank",(selected-1)/3);
 e->free_dirty=(1u<<FREE_NOTIFY_COUNT)-1;
}
/* Always show endpoints plus up to three interior grips. Unused native
 * grips coincide with the final endpoint and ignore writes. */
static void free_map(Engine *e,const FreeCurve *c,int m[5],float x[5]){
 int bank=(int)clampf(at(e,"free_bank"),0,fmaxf(0,(c->n-3)/3));int first=1+3*bank;
 m[0]=0;m[4]=c->n-1;x[0]=0;x[4]=1;
 for(int j=1;j<=3;j++){m[j]=first+j-1;if(m[j]>=c->n-1){m[j]=-1;x[j]=1-(4-j)*POINT_EPS;}else x[j]=c->x[m[j]];}
}
/* Native Pitch editors publish adjacent duration writes separately. Keep the
 * raw four-weight vector across those writes: rebasing after the first would
 * move untouched points and accumulate rounding drift during a drag. */
static void free_times(Engine *e,int o,int k,const FreeCurve *c,const float x[5]){
 int bank=(int)clampf(at(e,"free_bank"),0,fmaxf(0,(c->n-3)/3));
 if(!e->edit_time_valid || e->edit_osc!=o || e->edit_kind!=k || e->edit_bank!=bank || e->edit_count!=c->n){
  for(int j=0;j<4;j++)e->edit_weights[j]=x[j+1]-x[j];
  e->edit_osc=o;e->edit_kind=k;e->edit_bank=bank;e->edit_count=c->n;e->edit_time_valid=1;
 }
}
/* XYPad geometry: native puck centers are (width*x,height*(1-y)).
 * Encode the common 18px inset here; plot columns share this exact rectangle. */
static float free_xy(float value,int axis){return axis?(18+134*value)/170.f:(18+1064*value)/1100.f;}
static int free_xy_get(Engine *e,const char *key,char *buf,int len){
 int axis=-1,j=-1;char a,tail;
 if(!strcmp(key,"xy_x"))axis=0;else if(!strcmp(key,"xy_y"))axis=1;
 else if(!strcmp(key,"ui_point_count"))axis=2;
 else if(sscanf(key,"point_%c_%d%c",&a,&j,&tail)==2 && (a=='x'||a=='y') && j>=0&&j<16)axis=a=='y';
 if(axis<0)return 0;
 int o,k,selected;FreeCurve c;free_context(e,&o,&k,&selected,&c);
 if(axis==2)return snprintf(buf,len,"%d",c.n);
 if(j<0)j=selected;
 if(j>=c.n)j=c.n-1;
 return snprintf(buf,len,"%.9g",free_xy(axis?c.y[j]:c.x[j],axis));
}
static int free_xy_set(Engine *e,const char *key,const char *text){
 int axis=!strcmp(key,"xy_y");if(!axis && strcmp(key,"xy_x"))return 0;
 char *end;float v=strtof(text,&end);if(end==text||*end||!isfinite(v))return 1;
 char value[48];snprintf(value,sizeof value,"%.9g",clampf(axis?(v*170-18)/134.f:(v*1100-18)/1064.f,0,1));
 set_param(e,axis?"free_level":"free_time",value);return 1;
}
static int free_virtual_get(Engine *e,const char *key,char *buf,int len){
 if(strncmp(key,"edit_",5) && strncmp(key,"free_",5))return 0;
 int o,k,s;FreeCurve c;free_context(e,&o,&k,&s,&c);int m[5];float x[5];free_map(e,&c,m,x);
 int j=-1;char tail=0;
 if(sscanf(key,"edit_l%d%c",&j,&tail)==1 && j>=0 && j<5)return snprintf(buf,len,"%.9g",c.y[m[j]<0?c.n-1:m[j]]);
 if(sscanf(key,"edit_t%d%c",&j,&tail)==1 && j>=1 && j<=4){free_times(e,o,k,&c,x);return snprintf(buf,len,"%.9g",e->edit_weights[j-1]);}
 if(!strcmp(key,"free_add") || !strcmp(key,"free_delete") || !strcmp(key,"free_prev") || !strcmp(key,"free_next"))return snprintf(buf,len,"0");
 if(!strcmp(key,"free_select"))return snprintf(buf,len,"%d",s);
 if(!strcmp(key,"free_bank"))return snprintf(buf,len,"%.0f",clampf(at(e,key),0,fmaxf(0,(c.n-3)/3)));
 if(!strcmp(key,"free_status_display"))return snprintf(buf,len,"POINT %d / %d%s",s+1,c.n,c.n==FREE_MAX?"  / MAX":"");
 if(!strcmp(key,"free_status"))return snprintf(buf,len,"%.9g",(c.n*16+s)/271.f);
 if(!strcmp(key,"free_level_display"))return k?snprintf(buf,len,"%.2f Hz",20+(point_upper(point_base(e,o))-20)*c.y[s]):snprintf(buf,len,"%.2f %%",c.y[s]*100);
 if(!strcmp(key,"free_time_display"))return snprintf(buf,len,"%.2f ms%s",1000*at(e,"length")*c.x[s],s==0 || s==c.n-1?" / END":"");
 if(!strcmp(key,"free_bend_display"))return s==c.n-1?snprintf(buf,len,"END / --"):snprintf(buf,len,"%+.0f %%",(c.b[s]+c.d[s])*50);
 if(!strcmp(key,"free_level"))return snprintf(buf,len,"%.9g",c.y[s]);
 if(!strcmp(key,"free_time"))return snprintf(buf,len,"%.9g",c.x[s]);
 if(!strcmp(key,"free_bend"))return snprintf(buf,len,"%.9g",s<c.n-1?(c.b[s]+c.d[s])*0.5f:0);
 return 0;
}
static int free_virtual_set(Engine *e,const char *key,const char *text){
 if(strncmp(key,"edit_",5) && strncmp(key,"free_",5))return 0;
 char *end;float v=strtof(text,&end);if(end==text || *end || !isfinite(v))return 1;
 int o,k,s;FreeCurve c;free_context(e,&o,&k,&s,&c);
 if(!strcmp(key,"free_select") || !strcmp(key,"free_bank")){
  if(!strcmp(key,"free_select")){int selected=(int)clampf(roundf(v),0,c.n-1);if(selected==s)return 1;free_selection(e,c.n,selected);}
  else {int bank=(int)clampf(roundf(v),0,fmaxf(0,(c.n-3)/3));if(bank==(int)at(e,"free_bank"))return 1;++e->ui_revision;put(e,"free_bank",bank);put(e,"free_select",c.n>2?1+bank*3:0);e->free_dirty=(1u<<FREE_NOTIFY_COUNT)-1;}
  return 1;
 }
 if(!strcmp(key,"free_prev") || !strcmp(key,"free_next")){if(v>0)free_selection(e,c.n,s+(!strcmp(key,"free_next")?1:-1));return 1;}
 int j=-1;char tail=0;int level=sscanf(key,"edit_l%d%c",&j,&tail)==1 && j>=0&&j<5;
 int interval=!level && sscanf(key,"edit_t%d%c",&j,&tail)==1 && j>=1&&j<=4;
 int add=!strcmp(key,"free_add"),del=!strcmp(key,"free_delete"),y=!strcmp(key,"free_level"),xedit=!strcmp(key,"free_time"),bend=!strcmp(key,"free_bend");
 if(!level&&!interval&&!add&&!del&&!y&&!xedit&&!bend)return 1;
 if((add||del) && v<=0)return 1;
 if((add && c.n==FREE_MAX) || (del && (c.n<=2 || s==0 || s==c.n-1)))return 1;
 int m[5];float x[5];free_map(e,&c,m,x);
 if(level){if(m[j]<0)return 1;s=m[j];v=clampf(v,0,1);if(v==c.y[s])return 1;c.y[s]=v;}
 if(interval){
  free_times(e,o,k,&c,x);v=clampf(v,POINT_EPS,1);if(fabsf(v-e->edit_weights[j-1])<1e-7f)return 1;
  e->edit_weights[j-1]=v;float total=0;for(int t=0;t<4;t++)total+=e->edit_weights[t];
  float pos=0;int clamped=0;
  for(int t=1;t<=3;t++){pos+=e->edit_weights[t-1]/total;if(m[t]>0){int atp=m[t];float accepted=clampf(pos,c.x[atp-1]+FREE_EPS,c.x[atp+1]-FREE_EPS);if(fabsf(accepted-pos)>1e-6f)clamped=1;c.x[atp]=accepted;}}
  if(clamped)e->edit_time_valid=0;
  /* Adjacent duration events keep the explicitly selected point selected.
   * Selection can also be set with PREV/NEXT, or by a level gesture. */
  int selected_slot=-1;for(int t=1;t<=3;t++)if(m[t]==s)selected_slot=t;
  if(selected_slot<0 || (j!=selected_slot && j!=selected_slot+1))s=m[j==4?3:j];
  if(s<0)s=c.n-1;
 }
 if(y){v=clampf(v,0,1);if(v==c.y[s])return 1;c.y[s]=v;}
 if(xedit){if(s==0||s==c.n-1)return 1;v=clampf(v,c.x[s-1]+FREE_EPS,c.x[s+1]-FREE_EPS);if(v==c.x[s])return 1;c.x[s]=v;}
 if(bend){if(s==c.n-1)return 1;v=clampf(v,-1,1);if(fabsf(v-(c.b[s]+c.d[s])*0.5f)<1e-7f)return 1;c.b[s]=c.d[s]=v;}
 if(add){
  int atp=s<c.n-1?s:c.n-2;
  /* No room beside selected point: use the largest remaining interval. */
  if(c.x[atp+1]-c.x[atp]<FREE_EPS*2.1f){for(int t=0;t<c.n-1;t++)if(c.x[t+1]-c.x[t]>c.x[atp+1]-c.x[atp])atp=t;}
  float nx=(c.x[atp]+c.x[atp+1])*0.5f;
  float a=c.y[atp],d=c.y[atp+1];
  float h1=free_handle(a+(d-a)/3,c.b[atp]),h2=free_handle(a+2*(d-a)/3,c.d[atp]);
  float ab=(a+h1)*0.5f,bc=(h1+h2)*0.5f,cd=(h2+d)*0.5f;
  float abc=(ab+bc)*0.5f,bcd=(bc+cd)*0.5f,ny=(abc+bcd)*0.5f;
  /* De Casteljau subdivision preserves the existing cubic exactly. */
  for(int t=c.n;t>atp+1;t--){c.x[t]=c.x[t-1];c.y[t]=c.y[t-1];}
  for(int t=c.n-1;t>atp;t--){c.b[t]=c.b[t-1];c.d[t]=c.d[t-1];}
  c.b[atp]=free_offset(a+(ny-a)/3,ab);c.d[atp]=free_offset(a+2*(ny-a)/3,abc);
  c.b[atp+1]=free_offset(ny+(d-ny)/3,bcd);c.d[atp+1]=free_offset(ny+2*(d-ny)/3,cd);
  c.x[atp+1]=nx;c.y[atp+1]=ny;c.n++;s=atp+1;
 }
 if(del){for(int t=s;t<c.n-1;t++){c.x[t]=c.x[t+1];c.y[t]=c.y[t+1];}for(int t=s;t<c.n-2;t++){c.b[t]=c.b[t+1];c.d[t]=c.d[t+1];}c.n--;s=s<c.n?s:c.n-1;}
 if(!interval)e->edit_time_valid=0;
 atomic_fetch_add(&e->revision,1);
 if(!point_at(e,o,k,"seen"))point_seed(e,o,k);
 point_store(e,o,k,"mode",1);point_store(e,o,k,"seen",1);free_write(e,o,k,&c);free_selection(e,c.n,s);e->point_dirty|=POINT_ALL;
 audio_changed(e);atomic_fetch_add(&e->revision,1);return 1;
}
