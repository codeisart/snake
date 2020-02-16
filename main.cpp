#include <stdio.h>
#include <vector>
#include "SDL2/SDL.h"
#include <SDL2/SDL_ttf.h>

#define WIDTH 800
#define HEIGHT 600
#define ABS(X) (x<0 ? -x : x)
#define MAX(A,B) (A>B ? A : B)
#define RGBA(R,G,B,A) (uint32_t)((A&0xff)<<24)|((R&0xff)<<16)|((G&0xff)<<8)|((B&0xff))
#define NORM(RS,RE,VAL) ((float)VAL-RS)/((float)RE-RS)
#define LERP(RS,RE,NVAL) ((float)RS+((float)(RE-RS)*NVAL))
#define CLAMP(X,Y,N) ((N)<(X) ? (X) : (N) > (Y) ? (Y) : (N))

static const uint32_t kFontSize = 16;
TTF_Font* gDebugFont = nullptr;
SDL_Renderer *gRenderer = nullptr;
int gCurrentTune = 0;

uint32_t ddraw(int x,int y, SDL_Color c, const char* fmt,  ... )
{
    char buff[1024];
    va_list args;
    va_start(args, fmt);
    vsprintf(buff, fmt, args);
    va_end(args);

    SDL_Surface *text = TTF_RenderUTF8_Blended(gDebugFont, buff, c);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(gRenderer, text);
    SDL_FreeSurface(text);
    SDL_Rect dest = { x,y };
    SDL_QueryTexture(texture, NULL, NULL, &dest.w, &dest.h);
    SDL_RenderCopy(gRenderer, texture, NULL, &dest);
    SDL_DestroyTexture(texture);
    return kFontSize;
}
void rect(uint32_t* buf, int buff_width, int buff_height, int xpos, int ypos, int width, int height, uint32_t col)
{
    uint32_t* pos = buf+(ypos*buff_width);
    for( int y=ypos; y < ypos+height; ++y, pos+=buff_width)
	for( int x=xpos; x < xpos+width; ++x )
	    pos[x] = col;
}

struct Pos2d { int x=0; int y=0; };
struct Snake
{
    Pos2d food;
    Pos2d speed;
    std::vector<Pos2d> tail;
    int scl = 15;
    int startlen=5;
    int score = 0;
    int highScore = 0;

    Snake()
    { 
	speed.x = 1;
	tail.resize(startlen); 
	int head = startlen-1;
	tail[head].x = (WIDTH/scl)/2;
	tail[head].y = (HEIGHT/scl)/2;
	for(int i = 0; i < startlen-1; ++i) {
	    tail[i].x = tail[i+1].x + -speed.x;
	    tail[i].y = tail[i+1].y + -speed.y;
	}
	// choose new food location.
	food.x = rand() % (WIDTH-1) / scl;
	food.y = rand() % (HEIGHT-1) / scl;

	if( FILE* fp = fopen("score.dat","rb"))
	{
	    fread(&highScore,sizeof(int),1,fp);
	    fclose(fp);
	}
    }
    ~Snake()
    {
	if( FILE* fp = fopen("score.dat","wb"))
	{
	    fwrite(&highScore,sizeof(int),1,fp);
	    fclose(fp);
	}
    }

    void dir(int x, int y)
    {
	if( speed.x > 0 && x < 0 ) return;
	if( speed.y > 0 && y < 0 ) return;
	speed.x = x;
	speed.y = y;
    }

    void update()
    {
	int headpos = tail.size()-1;

	// move head.
	tail[headpos].x+=speed.x;
	tail[headpos].y+=speed.y;

	// dead?
	bool bDead = false;

	if( (tail[headpos].x*scl) + scl >= WIDTH ) bDead = true;
	if( (tail[headpos].y*scl) + scl >= HEIGHT ) bDead = true;
	if( (tail[headpos].x < 0)) bDead = true;
	if( (tail[headpos].y < 0)) bDead = true;

	for( int i = 0; i < tail.size()-1; ++i)
	{
	    if( tail[i].x == tail[headpos].x && 
		tail[i].y == tail[headpos].y )
	    {
		bDead = true;
		break;
	    }
	}
   
	if( bDead )
	{
	    tail.resize(startlen); 
	    int head = startlen-1;
	    tail[head].x = (WIDTH/scl)/2;
	    tail[head].y = (HEIGHT/scl)/2;
	    for(int i = 0; i < startlen-1; ++i) {
		tail[i].x = tail[i+1].x + -speed.x;
		tail[i].y = tail[i+1].y + -speed.y;
	    }
	    // choose new food location.
	    food.x = rand() % (WIDTH-1) / scl;
	    food.y = rand() % (HEIGHT-1) / scl;

	    score = 0;
	    SDL_Delay(2000);
	}
	else if( tail[headpos].x == food.x && tail[headpos].y == food.y ) // eat food?
	{
	    // choose new food location.
	    food.x = rand() % (WIDTH-1) / scl;
	    food.y = rand() % (HEIGHT-1) / scl;

	    // grow snake.
	    tail.resize(tail.size()+1);
	    tail[headpos+1] = tail[headpos]; 
	    score++;
	    highScore = MAX(score,highScore);
	}

	// move body.
	for(int i = 0; i < tail.size()-1; ++i)
	    tail[i] = tail[i+1];

    }

    void draw(uint32_t* p, int w, int h)
    {
	rect(p,w,h,food.x*scl,food.y*scl,scl,scl,RGBA(255,192,203,255));

	for( Pos2d i : tail)
	    rect(p,w,h,i.x*scl,i.y*scl,scl,scl,RGBA(255,255,255,255));
    }
}gSnake;

int main()
{
    srand (time(NULL));
    if(SDL_Init(SDL_INIT_VIDEO) != 0) {
	fprintf(stderr, "Could not init SDL: %s\n", SDL_GetError());
	return 1;
    }
    SDL_Window *screen = SDL_CreateWindow("My application",
	    SDL_WINDOWPOS_CENTERED,
	    SDL_WINDOWPOS_CENTERED,
	    WIDTH, HEIGHT,
	    0//SDL_WINDOW_FULLSCREEN
	    );
    if(!screen) {
	fprintf(stderr, "Could not create window\n");
	return 1;
    }
    gRenderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_SOFTWARE);
    if(!gRenderer) {
	fprintf(stderr, "Could not create renderer\n");
	return 1;
    }

    if(TTF_Init()==-1) {
	printf("TTF_Init: %s\n", TTF_GetError());
	exit(2);
    }
    // load font.ttf at size 16 into font
    gDebugFont=TTF_OpenFont("Hack-Regular.ttf", kFontSize);
    if(!gDebugFont) {
	printf("TTF_OpenFont: %s\n", TTF_GetError());
    }

    uint32_t* pixels = new uint32_t[WIDTH*HEIGHT];
    SDL_Texture * texture = SDL_CreateTexture(gRenderer,
	    SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, WIDTH, HEIGHT);

    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);

    static const SDL_Color white = {255, 255, 255, 255};
    bool quit = false;

    int32_t prevTime = SDL_GetTicks();;
    while (!quit)
    {
	int32_t currentTime = SDL_GetTicks();
	int32_t delta = currentTime-prevTime;

	uint32_t *p = pixels;
	memset(p,0,WIDTH*HEIGHT*4);
	gSnake.update();
	gSnake.draw(p,WIDTH,HEIGHT);

	SDL_UpdateTexture(texture, NULL, pixels, WIDTH * sizeof(Uint32));
	SDL_RenderClear(gRenderer);
	SDL_RenderCopy(gRenderer, texture, NULL, NULL);

	ddraw(0,0,  white, "Score: %d", gSnake.score);
	ddraw(WIDTH-(12*kFontSize), 0, white, "High Score: %d", gSnake.highScore);

	// flip.
	SDL_RenderPresent(gRenderer);

	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
	    if (e.type == SDL_QUIT)
	    {
		quit = true;
	    }
	    if (e.type == SDL_KEYDOWN)
	    {
		SDL_Keymod m = SDL_GetModState();
		if(     e.key.keysym.scancode == SDL_SCANCODE_W) gSnake.dir( 0,-1);
		else if(e.key.keysym.scancode == SDL_SCANCODE_S) gSnake.dir( 0, 1);
		else if(e.key.keysym.scancode == SDL_SCANCODE_A) gSnake.dir(-1, 0);
		else if(e.key.keysym.scancode == SDL_SCANCODE_D) gSnake.dir(1,  0);
	    }
	    if (e.type == SDL_MOUSEBUTTONDOWN)
	    {
		quit = true;
	    }
	    prevTime = currentTime;
	}
	SDL_Delay( 60 );
    }

    delete[] pixels;
    SDL_DestroyTexture(texture);
    SDL_DestroyWindow(screen);
    TTF_CloseFont(gDebugFont);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
