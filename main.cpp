#define HEIGHT 800
#define WIDTH 800
#define DEPTH 800

#include <iostream>
#include <cmath>
#include <eigen3/Eigen/Dense>
#include <SDL2/SDL.h>
using std::endl;
using std::cout;
using std::cerr;
using Eigen::Matrix4d;
using Eigen::Vector4d;

double cW = 0.8;
Vector4d cube[8] = {
	Vector4d(-cW, -cW, cW, 1),
	Vector4d(cW, -cW, cW, 1),
	Vector4d(cW, cW, cW, 1),
	Vector4d(-cW, cW, cW, 1),
	Vector4d(-cW, -cW, -cW, 1),
	Vector4d(cW, -cW, -cW, 1),
	Vector4d(cW, cW, -cW, 1),
	Vector4d(-cW, cW, -cW, 1),
};

Matrix4d sM, tM, rM, rX, rY, rZ, pM, lv;
double uS = 100, uT = 0, uR = 0; //Uniform transfrormation parameters
double sV[3] = {uS, uS, uS}; //Scalar vector
double tV[3] = {uT, uT, uT}; //Translation vector
double rV[3] = {uR*M_PI/180, uR*M_PI/180, 0/*uR*M_PI/180*/}; //Rotation vector

double angleOfView = 60;
double aspectRatio = WIDTH/HEIGHT;
double n = 0.01;
double f = 100;
double scale = tan(angleOfView*0.5*M_PI/180)*n;
double r = aspectRatio*scale;
double l = -r;
double t = scale;
double b = -t;

SDL_Window* window;
SDL_Renderer* renderer;
bool quit = 0;

bool init();
void drawVertex(const Vector4d& v);
void drawCube();
inline int getMax(const Vector4d v[], int nVert, int index);
inline int getMin(const Vector4d v[], int nVert, int index);
void drawSphere(Vector4d c, double r);
void fill(const Vector4d v[], int nVert);
void eventHandler(SDL_Event& e);


int main(){
	if(!init()) return 1;
	SDL_Event e;
	while(!quit){
	sM << sV[0],0,0,0,
		  0,sV[1],0,0,
		  0,0,sV[2],0,
		  0,0,0,1;

	tM << 1,0,0,tV[0],
		  0,1,0,tV[1],
		  0,0,1,tV[2],
		  0,0,0,1;

	rX << 1,0,0,0,
		  0,cos(rV[0]),-sin(rV[0]), 0,
		  0,sin(rV[0]),cos(rV[0]), 0,
		  0,0,0,1;
	rY << cos(rV[1]),0,sin(rV[1]),0,
		  0,1,0,0,
		  -sin(rV[1]),0,cos(rV[1]),0,
		  0,0,0,1;
	rZ << cos(rV[2]),-sin(rV[2]),0,0,
		  sin(rV[2]),cos(rV[2]),0,0,
		  0,0,1,0,
		  0,0,0,1;

	rM=rX*rZ*rY;
	
	pM << (2*n/(r-l)),0,((r+l)/(r-l)),0,
		  0,(2*n/(t-b)),((t+b)/(t-b)),0,
		  0,0,(-(f+n)/(f-n)),(-2*f*n/(f-n)),
		  0,0,-1,0;
		
		lv = pM*sM*rM*tM;

		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderClear(renderer);
		eventHandler(e);
		drawCube();
		drawSphere(Vector4d(0,0,0,0), 0.8);
		SDL_RenderPresent(renderer);
	}
	return 0;
}





bool init(){
	bool success = 1;
	if(SDL_Init(SDL_INIT_VIDEO)<0){
		cerr << "SDL error: " << SDL_GetError() << endl;
		success = 0;
	} else {
		window = SDL_CreateWindow("TEST", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
		if(window==NULL){
			cerr << "Window error: " << SDL_GetError() << endl;
			success = 0;
		} else {
			renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
			if(renderer==NULL){
				cerr << "Renderer error: " << SDL_GetError() << endl;
				success = 0;
			}
		}
	}
	return success;
}

void drawVertex(const Vector4d& v){
	SDL_RenderDrawPoint(renderer, v[0]+WIDTH/2, v[1]+HEIGHT/2);
}

void drawSphere(Vector4d c, double r){
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
	double res = 0.1;
	for(double i = 0; i < 2*M_PI; i+=res){
		Vector4d pOld = c;
		for(double j = 0; j < 2*M_PI; j+=res){
			Vector4d p(r*sin(i)*cos(j)+c[0], r*sin(i)*sin(j)+c[1], r*cos(i)+c[2], 1);
			p = lv*p;
			if(j!=0) SDL_RenderDrawLine(renderer, p[0]+WIDTH/2, p[1]+HEIGHT/2, pOld[0]+WIDTH/2, pOld[1]+HEIGHT/2);	
			pOld = p;
		}
	}
}

void drawCube(){
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	Vector4d tCube[8];
	for(int i = 0; i < 8; i++) tCube[i] = lv*cube[i];
	/*Vector4d vec[] = {tCube[0], tCube[1], tCube[2], tCube[3]};
	fill(vec, 4);
	vec[0]=tCube[0]; vec[1]=tCube[1]; vec[2]=tCube[5]; vec[3]=tCube[4];
	fill(vec, 4);
	vec[0]=tCube[1]; vec[1]=tCube[2]; vec[2]=tCube[6]; vec[3]=tCube[5];
	fill(vec, 4);
	vec[0]=tCube[0]; vec[1]=tCube[3]; vec[2]=tCube[7]; vec[3]=tCube[4];
	fill(vec, 4);
	vec[0]=tCube[4]; vec[1]=tCube[5]; vec[2]=tCube[6]; vec[3]=tCube[7];
	fill(vec, 4);
	vec[0]=tCube[3]; vec[1]=tCube[2]; vec[2]=tCube[6]; vec[3]=tCube[7];
	fill(vec, 4);*/
	for(int i = 0; i < 8; i++) for(int j = i; j < 8; j++)
	SDL_RenderDrawLine(renderer, tCube[i][0]+WIDTH/2, tCube[i][1]+HEIGHT/2, tCube[j][0]+WIDTH/2, tCube[j][1]+HEIGHT/2);
}

void fill(const Vector4d v[], int nVert){
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
	
	int xMin = getMin(v, nVert, 0);
	int xMax = getMax(v, nVert, 0);
	int yMin = getMin(v, nVert, 1);
	int yMax = getMax(v, nVert, 1);

	for(int i=yMin; i<yMax; ++i){
		for(int j=xMin; j<xMax; ++j){
			bool c = 0;
			for(int k=0, l=nVert-1; k<nVert; l=k++){
				if(((v[k][1]>i)!=(v[l][1]>i))&&(j<(v[l][0]-v[k][0])*(i-v[k][1])/(v[l][1]-v[k][1])+v[k][0]))
				   	c=!c;
			}
			if(c) SDL_RenderDrawPoint(renderer, j+WIDTH/2, i+HEIGHT/2);
		}
	}	
}

double oldX = 0, oldY = 0;
bool mouseDown = 0;
void eventHandler(SDL_Event& e){
	while(SDL_PollEvent(&e)!=0){
		if(e.type==SDL_QUIT || e.key.keysym.sym==SDLK_ESCAPE) quit = 1;
		if(e.type==SDL_MOUSEBUTTONDOWN)	mouseDown = 1;			
		if(e.type==SDL_MOUSEBUTTONUP) mouseDown=0;
		if(e.type==SDL_KEYDOWN){
			double f = 0.05;
			switch(e.key.keysym.sym){
				case SDLK_UP:
					rV[0]+=f;
					break;
				case SDLK_DOWN:
					rV[0]-=f;
					break;
				case SDLK_RIGHT:
					rV[1]+=f;
					break;
				case SDLK_LEFT:
					rV[1]-=f;
					break;
				case SDLK_k:
					for(int i=0; i<3; i++) sV[i]+=10;
					break;
				case SDLK_l:
					for(int i=0; i<3; i++) sV[i]-=10;
					break;
			}
		}
		if(e.type==SDL_MOUSEMOTION){
			int x, y;
			SDL_GetMouseState(&x, &y);
			int speed = 4;
			double scaledX=(double)x*speed/WIDTH, scaledY=(double)y*speed/HEIGHT;
			rV[1]-=(scaledX-oldX);
			rV[0]-=(scaledY-oldY);
			oldX=scaledX;
			oldY=scaledY;
		}	
	}
}

int getMax(const Vector4d v[], int nVert, int index){ 
	int max = v[0][index];
	for (int i = 1; i < nVert; i++) if(v[i][index]>max) max=v[i][index];
	return max; 
}
int getMin(const Vector4d v[], int nVert, int index){ 
	int min = v[0][index];
	for (int i = 1; i < nVert; i++) if(v[i][index]<min) min=v[i][index];
	return min; 
}
