/* Host test for the menu state machine. */
#include "menu.h"
#include "tracks.h"
/* button bits come from menu.c's own host fallback */
#include <stdio.h>
#include <string.h>

/* ui.h stubs -- menu_draw is exercised for crashes, not pixels */
void ui_init(void){}
void ui_begin(int w,int h){(void)w;(void)h;}
void ui_end(void){}
void ui_rect(float a,float b,float c,float d,float e,float f,float g,float h){
    (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;}
static char last[256];
/* Every drawn string, and how many came out EMPTY.
 *
 * `last` alone could not do the job the check below claims: it holds whichever
 * ui_text ran last, which is always the footer hint, so it was non-empty however
 * broken row_value was. A row added to the enum and forgotten in row_value draws
 * an empty value column -- that is the thing to count. */
static int n_texts, n_empty;
static char texts[64][128];
void ui_text(float x,float y,float s,float r,float g,float b,float a,const char*t){
    (void)x;(void)y;(void)s;(void)r;(void)g;(void)b;(void)a;
    snprintf(last,sizeof(last),"%s",t);
    if(!t[0]) n_empty++;
    if(n_texts<64) snprintf(texts[n_texts],sizeof(texts[0]),"%s",t);
    n_texts++;}
static int drew(const char*needle){
    int i; for(i=0;i<n_texts&&i<64;i++) if(strstr(texts[i],needle)) return 1;
    return 0;}
static void draw_fresh(menu_t*m){ n_texts=0; n_empty=0; last[0]=0; menu_draw(m,960,544); }
float ui_text_w(float s,const char*t){return (float)strlen(t)*10.f*s;}
float ui_text_h(float s){return 19.f*s;}

static int fails=0;
static void ck(int ok,const char*w){ if(!ok){printf("  FAIL: %s\n",w);fails++;} }
static unsigned prev=0;
static void press(menu_t*m,unsigned b){ menu_input(m,b,prev); prev=b; menu_input(m,0,prev); prev=0; }

int main(void){
    menu_t m; int i;
    menu_init(&m,0,0);
    ck(!m.open,"starts closed");
    ck(m.req_track==-1 && m.req_car==-1,"no pending requests at init");

    press(&m,SCE_CTRL_START);
    ck(m.open,"START opens");
    press(&m,SCE_CTRL_START);
    ck(!m.open,"START closes");
    press(&m,SCE_CTRL_START);
    press(&m,SCE_CTRL_CIRCLE);
    ck(!m.open,"CIRCLE closes");

    /* track wraps both ways and raises a request each time */
    press(&m,SCE_CTRL_START);
    m.req_track=-1;
    press(&m,SCE_CTRL_LEFT);
    ck(m.track==N_TRACKS-1,"track wraps below zero");
    ck(m.req_track==N_TRACKS-1,"track change requests a load");
    m.req_track=-1;
    press(&m,SCE_CTRL_RIGHT);
    ck(m.track==0,"track wraps back to zero");

    /* every row reachable, no out-of-range value on any of them */
    for(i=0;i<40;i++){
        press(&m,SCE_CTRL_DOWN);
        menu_draw(&m,960,544);            /* must not read out of bounds */
        ck(m.track>=0&&m.track<N_TRACKS,"track in range");
        ck(m.car>=0&&m.car<3,"car in range");
        ck(m.tires>=0&&m.tires<4,"tires in range");
        ck(m.reso>=0&&m.reso<4,"reso in range");
    }

    /* upgrades cycle 0..3 */
    menu_init(&m,0,0); prev=0;
    press(&m,SCE_CTRL_START);
    m.row=MENU_TIRES;
    for(i=0;i<4;i++) press(&m,SCE_CTRL_RIGHT);
    ck(m.tires==0,"tires wrap after four steps");
    press(&m,SCE_CTRL_LEFT);
    ck(m.tires==3,"tires wrap downward");

    /* actions */
    menu_init(&m,0,0); prev=0;
    press(&m,SCE_CTRL_START);
    m.row=MENU_RESTART;
    press(&m,SCE_CTRL_CROSS);
    ck(m.req_restart==1,"Restart raises a request");
    ck(!m.open,"Restart closes the menu");
    menu_init(&m,0,0); prev=0;
    press(&m,SCE_CTRL_START);
    m.row=MENU_QUIT;
    press(&m,SCE_CTRL_CROSS);
    ck(m.req_quit==1,"Quit raises a request");

    /* booster cycles 0..3 and is the exhaust the car wears */
    menu_init(&m,0,0); prev=0;
    press(&m,SCE_CTRL_START);
    m.row=MENU_BOOST;
    for(i=0;i<4;i++) press(&m,SCE_CTRL_RIGHT);
    ck(m.boost==0,"booster wraps after four steps");
    press(&m,SCE_CTRL_LEFT);
    ck(m.boost==3,"booster wraps downward");

    /* NO picker row may draw an empty value. This is what catches a row added to
       the enum and forgotten in row_value -- the case the previous version of
       this check could not see, because it only looked at the last string drawn
       and that is always the footer hint. */
    menu_init(&m,0,0); prev=0;
    press(&m,SCE_CTRL_START);
    for(i=0;i<MENU_ROWS;i++){
        m.row=i; draw_fresh(&m);
        ck(n_empty==0,"no row draws an empty string -- one is missing from row_value");
        ck(n_texts>=MENU_ROWS+MENU_FIRST_ACTION,"every label and value is drawn");
    }
    ck(MENU_FIRST_ACTION<MENU_ROWS,"there is at least one action row");

    /* ---- the skin, which is PER CAR --------------------------------------- */
    menu_init(&m,0,0); prev=0;
    press(&m,SCE_CTRL_START);
    ck(m.skins==1,"a car is assumed to have one skin until a scene says otherwise");
    ck(m.skin[0]==0&&m.skin[1]==0&&m.skin[2]==0,"every car starts on the skin it is modelled wearing");

    /* WRAPS OVER WHAT THE CAR HAS, not over MENU_SKINS. main.c writes `skins`
       from the loaded scene, and a car packed without the extra atlases has one:
       the row must then not move at all rather than offering three positions that
       all draw the same paint. */
    m.row=MENU_SKIN;
    for(i=0;i<3;i++) press(&m,SCE_CTRL_RIGHT);
    ck(m.skin[0]==0,"with one skin packed the row cannot move");
    m.skins=2;
    press(&m,SCE_CTRL_RIGHT);
    ck(m.skin[0]==1,"RIGHT steps to the next skin");
    press(&m,SCE_CTRL_RIGHT);
    ck(m.skin[0]==0,"and wraps at the number the CAR has, not at four");
    m.skins=MENU_SKINS;
    press(&m,SCE_CTRL_LEFT);
    ck(m.skin[0]==MENU_SKINS-1,"with all four packed it wraps downward to the last");

    /* IT IS LIVE. The alternate atlases are already uploaded, so a repaint is a
       re-pointed GLuint -- it must not ask for the reload of BOTH scenes that the
       texture rows need, nor for the car to be loaded again. A mutant that raised
       either would cost a visible stall on every press. */
    m.req_reload=0; m.req_car=-1;
    press(&m,SCE_CTRL_RIGHT);
    ck(!m.req_reload,"a skin change needs no reload");
    ck(m.req_car==-1,"and does not reload the car");

    /* PER CAR, which is the whole point of the row: skin 3 of the Overkill and
       skin 3 of the Buggy are different textures (car_askin1<n> against
       car_askin2<n>), so one shared index would mean "the same skin" named a
       different colour on each car. Switching away and back has to preserve it. */
    menu_init(&m,0,0); prev=0;
    press(&m,SCE_CTRL_START);
    m.skins=MENU_SKINS;
    m.row=MENU_SKIN;
    for(i=0;i<3;i++) press(&m,SCE_CTRL_RIGHT);
    ck(m.skin[0]==3,"the Overkill is on its fourth skin");
    m.row=MENU_CAR;
    press(&m,SCE_CTRL_RIGHT);
    ck(m.car==1,"moved to the Buggy");
    ck(m.skin[1]==0,"which keeps its OWN skin, not the Overkill's");
    m.row=MENU_SKIN;
    press(&m,SCE_CTRL_RIGHT);
    ck(m.skin[1]==1&&m.skin[0]==3,"changing one car's skin leaves the other's alone");
    m.row=MENU_CAR;
    press(&m,SCE_CTRL_LEFT);
    ck(m.car==0&&m.skin[0]==3,"and coming back restores the first car's choice");

    /* The row has to SAY which of the two situations it is in, because with one
       skin packed it looks broken -- it will not move and nothing explains why.
       Bound to the words, not to the count, so raising MENU_SKINS cannot empty
       either message. */
    m.row=MENU_SKIN; m.skins=1; draw_fresh(&m);
    ck(drew("not packed"),"with one skin the row says the rest are not packed");
    m.skins=MENU_SKINS; draw_fresh(&m);
    ck(!drew("not packed"),"and with four it does not");
    ck(drew("of 4"),"the row shows how many skins this car has");

    /* A `skins` outside 1..MENU_SKINS must not index off the end of skin[]. 0 is
       what a scene with no paint at all reports. */
    m.skins=0;   press(&m,SCE_CTRL_RIGHT); draw_fresh(&m);
    ck(m.skin[0]>=0&&m.skin[0]<MENU_SKINS,"a zero skin count stays in range");
    m.skins=999; press(&m,SCE_CTRL_RIGHT); draw_fresh(&m);
    ck(m.skin[0]>=0&&m.skin[0]<MENU_SKINS,"and so does an absurd one");

    /* ---- texture quality (the game's RenderQual / VIDEO_TexQual) ---------- */
    menu_init(&m,0,0); prev=0;
    press(&m,SCE_CTRL_START);
    ck(m.tex_quality==0,"quality starts at the sharpest level");
    ck(!m.req_reload,"and asks for no reload before it is touched");
    m.row=MENU_TEXQUAL;

    /* CLAMPS at the sharp end -- one press past sharpest must not be blurriest,
       which is what wrapping would do, and it must not ask for a pointless
       reload of both scenes either. */
    press(&m,SCE_CTRL_LEFT);
    ck(m.tex_quality==0,"quality clamps at the sharp end rather than wrapping");
    ck(!m.req_reload,"an unchanged quality raises no reload");

    for(i=0;i<MENU_TEXQUAL_LEVELS-1;i++){
        m.req_reload=0;
        press(&m,SCE_CTRL_RIGHT);
        ck(m.tex_quality==i+1,"RIGHT steps quality one level down");
        ck(m.req_reload==1,"a quality change asks for a reload");
    }
    ck(m.tex_quality==MENU_TEXQUAL_LEVELS-1,"quality reaches the lowest level");
    m.req_reload=0;
    press(&m,SCE_CTRL_RIGHT);
    ck(m.tex_quality==MENU_TEXQUAL_LEVELS-1,"quality clamps at the blurry end");
    ck(!m.req_reload,"and raises no reload once it is already there");

    /* The row has to SAY what it does. Bound to the resolutions the original's
       three texture sets actually are -- 512 / 256 / 128 -- not to whatever the
       constant happens to be, so renaming the levels cannot quietly empty it. */
    for(i=0;i<MENU_TEXQUAL_LEVELS;i++){
        char px[16];
        m.tex_quality=i; m.row=MENU_TEXQUAL; draw_fresh(&m);
        snprintf(px,sizeof(px),"%d px",512>>i);
        ck(drew(px),"the quality row shows the resolution it will load");
    }

    /* ---- texture colour order (565 red-high vs Vita3K's swapped) --------- */
    menu_init(&m,0,0); prev=0;
    press(&m,SCE_CTRL_START);
    ck(m.tex_swap_rb==0,"the channel order defaults to standard, for hardware");
    m.row=MENU_TEXORDER;
    m.req_reload=0;
    press(&m,SCE_CTRL_RIGHT);
    ck(m.tex_swap_rb==1,"RIGHT switches to the swapped order");
    ck(m.req_reload==1,"and asks for the reload that re-uploads the textures");
    m.req_reload=0;
    press(&m,SCE_CTRL_LEFT);
    ck(m.tex_swap_rb==0,"LEFT switches back -- either direction toggles");
    ck(m.req_reload==1,"and asks for a reload again");
    /* The row has to name the MACHINE each order is for. The symptom (blue sand,
       gold sea) is unmistakable but says nothing about which way to press. */
    m.tex_swap_rb=0; draw_fresh(&m);
    ck(drew("hardware"),"the standard order says it is the one for hardware");
    m.tex_swap_rb=1; draw_fresh(&m);
    ck(drew("Vita3K"),"and the swapped order says it is the one for Vita3K");

    /* Every other picker still wraps -- the clamp is deliberate and local, and a
       clamp copied onto the track row would strand the player on track 1. */
    menu_init(&m,0,0); prev=0;
    press(&m,SCE_CTRL_START);
    m.row=MENU_TRACK;
    press(&m,SCE_CTRL_LEFT);
    ck(m.track==N_TRACKS-1,"the track row still wraps, unlike quality");

    /* input is ignored while closed */
    menu_init(&m,0,0); prev=0;
    press(&m,SCE_CTRL_RIGHT);
    ck(m.track==0 && m.req_track==-1,"closed menu ignores d-pad");

    printf("%s\n", fails?"SOME CHECKS FAILED":"all menu checks passed");
    return fails?1:0;
}
