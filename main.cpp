#define TINYOBJLOADER_IMPLEMENTATION

#define HEIGHT 800
#define WIDTH 800
#define DEPTH 800

#include <iostream>
#include <cmath>
#include <vector>
#include <eigen3/Eigen/Core>
#include <SDL2/SDL.h>
#include "tiny_obj_loader.h"

using std::endl;
using std::cout;
using std::cerr;
using std::string;
using std::vector;

typedef Eigen::Vector4d V; //World-space vector
typedef Eigen::Vector4i v; //Screen-space vector
typedef Eigen::Matrix4d M; //Matrix

//Object-loading containers
tinyobj::attrib_t attrib;
vector<tinyobj::shape_t> shapes;
vector<tinyobj::material_t> materials;

//Cube vertices
V cubeVertices[8]={
	V(-1, -1, 1, 1),
	V(1, -1, 1, 1),
	V(1, 1, 1, 1),
	V(-1, 1, 1, 1),
	V(-1, -1, -1, 1),
	V(1, -1, -1, 1),
	V(1, 1, -1, 1),
	V(-1, 1, -1, 1),
};

//Cube vertex drawing order
Uint8 cubeOrder[36]={
	0,1,2, 0,2,3,
	1,5,6, 1,6,2,
	5,4,7, 5,7,6,
	4,0,3, 4,3,7,
	4,5,1, 4,1,0,
	7,6,2, 7,2,3
};

//Cube edge drawing order
Uint8 cubeOutline[24]{
	0,1, 0,3, 0,4,
	1,5, 1,2, 
	2,6, 2,3, 3,7,
	4,5, 4,7, 5,6, 6,7
};

//Transformation data
M sM, tM, rM, mM, vM, pM, mvpM, vpM;
double uS = 1, uT = 0, uR = 0; //Uniform transfrormation parameters
V sV = {uS, uS, uS, 1}; //Scalar vector
V tV= {uT, uT, uT, 1}; //Translation vector
V rV = {uR*M_PI/180, uR*M_PI/180, uR*M_PI/180, 1}; //Rotation vector

V cRV = {1, 0, 0, 0};
V cUV = {0, 1, 0, 0};
V cFV = {0, 0, 1, 0};
V cPV = {0, -200, 700, 1};

double fov = 90;
double near = 0.01;
double far = 100;

Uint32 pixels[HEIGHT][WIDTH];
double zBuf[HEIGHT][WIDTH];

//SDL data
SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* tex;
bool quit = 0;

//Color container
struct Color{
	Color(Uint8 a=255, Uint8 r=0, Uint8 g=0, Uint8 b=0)
	:a(a), r(r), g(g), b(b){}
	Uint8 a,r,g,b;
};

//Function prototypes
//SDL methods
bool init();
void eventHandler(SDL_Event& e);
void render();

//Matrix generators
void transltionMatrix(const V& v);
void scaleMatrix(const V& v);
void rotationMatrix(const V& v);
void modelMatrix(const V& t, const V& s, const V& r);
void viewMatrix(const V& r, const V& u, const V& f, const V& p); 
void projectionMatrix();
void viewportMatrix();

//Adapter functions
void vertex(const V& p, int r=1, Color col=Color());
void line(const V& a, const V& b, Color col=Color());
void triangle(const V& a, const V& b, const V& c, bool fill=0, Color col=Color());
void cube(const V& c, double r, bool fill=0, Color col=Color());
void sphere(const V& c, double r=1, Color col=Color());

//Drawing methods
void drawVertex(v p, int r, Color col);
void drawSegment(v a, v b, Color col);
void drawFillSegment(v a, v b, Color col);
void drawTriangle(v a, v b, v c, bool fill, Color col);
void drawCube(const v* cube, bool fill, Color col);
void drawSphere(const V& c, double r, Color col);
void fillTriangle(v* t, v* l, v* r, Color col);

//OBJ model methods
void loadFromFile(string path);
void drawObject();

//Screen pixel methods
inline v getPixel(const V& v);
void setPixel(const v& v, Color col);



//Main
int main(){
	if(!init()) return 1;
	SDL_Event e;
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	viewportMatrix();
	loadFromFile("cat.obj");
	while(!quit){
		modelMatrix(tV, sV, rV);
		viewMatrix(cRV, cUV, cFV, cPV);
		projectionMatrix();
		mvpM = pM*vM*mM;
		memset(pixels, 255, WIDTH*HEIGHT*sizeof(Uint32));
		eventHandler(e);

		V a(-1,0,0,1), b(0,0,0,1), c(0,1,0,1), zero(0,0,0,1);
		//vertex(zero);
		//line(a,b);
		//triangle(a,b,c,1);
		//cube(zero,0.2,0);
		//sphere(zero,1);
		drawObject();

		//for(int i=-10; i<10; i++) for(int j=-10; j<10; j++)
		//cube(V(2*j, 4, 2*i, 0), 1);
		
		render();
	}
	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(renderer);
	return 0;
}




//SDL initializer
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
			} else {
				tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, WIDTH, HEIGHT);
				if(tex==NULL){
					cerr << "Texture error: " << SDL_GetError() << endl;
					success = 0;
				}
			}
		}
	}
	return success;
}

//Event handler
double oldX = 0, oldY = 0;
void eventHandler(SDL_Event& e){
	while(SDL_PollEvent(&e)!=0){
		if(e.type==SDL_QUIT || e.key.keysym.sym==SDLK_ESCAPE) quit = 1;
		if(e.type==SDL_KEYDOWN){
			double f = 0.1;
			switch(e.key.keysym.sym){
				case SDLK_UP:
					cPV[2]-=f;
					break;
				case SDLK_DOWN:
					cPV[2]+=f;
					break;
				case SDLK_RIGHT:
					cPV[0]+=f;
					break;
				case SDLK_LEFT:
					cPV[0]-=f;
					break;
				case SDLK_k:
					for(int i=0; i<3; i++) sV[i]+=10;
					break;
				case SDLK_l:
					for(int i=0; i<3; i++) sV[i]-=10;
					break;
				case SDLK_i:
					fov++;
					break;
				case SDLK_o:
					fov--;
					break;
				case SDLK_w:
					cPV[1]-=f;
					break;
				case SDLK_s:
					cPV[1]+=f;
					break;
				case SDLK_a:
					rV[1]+=f;
					break;
				case SDLK_d:
					rV[1]-=f;
					break;
			}
		}
		if(e.type==SDL_MOUSEMOTION){
			int x, y;
			SDL_GetMouseState(&x, &y);
			if(x>=0 && x<=WIDTH && y>=0 && y<=HEIGHT){
				int speed = 4;
				double scaledX=(double)x*speed/WIDTH, scaledY=(double)y*speed/HEIGHT;
				rV[1]+=(scaledX-oldX);
				rV[0]-=(scaledY-oldY);
				oldX=scaledX;
				oldY=scaledY;
			}
		}	
	}
}


void transltionMatrix(const V& v){
	tM << 1,0,0,v[0],
		  0,1,0,v[1],
		  0,0,1,v[2],
		  0,0,0,1;
}

void scaleMatrix(const V& v){
	sM << v[0],0,0,0,
		  0,v[1],0,0,
		  0,0,v[2],0,
		  0,0,0,1;
}

void rotationMatrix(const V& v){
	M rX, rY, rZ;
	rX << 1,0,0,0,
		  0,cos(v[0]),-sin(v[0]), 0,
		  0,sin(v[0]),cos(v[0]), 0,
		  0,0,0,1;
	rY << cos(v[1]),0,sin(v[1]),0,
		  0,1,0,0,
		  -sin(v[1]),0,cos(v[1]),0,
		  0,0,0,1;	
	rZ << cos(v[2]),-sin(v[2]),0,0,
		  sin(v[2]),cos(v[2]),0,0,
		  0,0,1,0,
		  0,0,0,1;

	rM=rX*rZ*rY;
}

void modelMatrix(const V& t, const V& s, const V& r){
	transltionMatrix(t);
	scaleMatrix(s);
	rotationMatrix(r);
	mM = rM*tM*sM;
}

void viewMatrix(const V& r, const V& u, const V& f, const V& p){
	vM << r[0],u[0],f[0],p[0],	
		  r[1],u[1],f[1],p[1],	
		  r[2],u[2],f[2],p[2],	
		  0,0,0,1;
}

void projectionMatrix(){
	double top = near*tan((M_PI/180)*fov/2);
	double bottom = -top;
	double right = top*(WIDTH/HEIGHT);
	double left = -right;
	double height = top-bottom;
	double width = right-left;
	double depth = far-near;

	double w = -2*near/width;
	double h = -2*near/height;
	double dd = -(far+near)/depth;
	double wd = (right+left)/width;
	double hd = (top+bottom)/height;
	double qn = -2*far*near/depth;
	
	pM << w,0,wd,0,
		  0,h,hd,0,
		  0,0,dd,qn,
		  0,0,-1,0;
}

void viewportMatrix(){
	vpM << WIDTH/2,0,0,WIDTH/2,
		   0,-HEIGHT/2,0,HEIGHT/2,
		   0,0,(far-near)/2,(far+near)/2,
		   0,0,0,1;
}





//Adapter functions (from world to screen space)
void vertex(const V& p, int r, Color col){ drawVertex(getPixel(p),r,col); }
void line(const V& a, const V& b, Color col){	drawSegment(getPixel(a), getPixel(b),col); }
void triangle(const V& a, const V& b, const V& c, bool fill, Color col){ drawTriangle(getPixel(a),getPixel(b),getPixel(c),fill,col);}
void cube(const V& c, double r, bool fill, Color col){	
	v tCube[8];
	M m;
	m << r,0,0,c[0],
		 0,r,0,c[1],
		 0,0,r,c[2],
		 0,0,0,1;
	for(int i = 0; i < 8; i++) tCube[i]=getPixel(m*cubeVertices[i]);
	drawCube(tCube,fill,col); }
void sphere(const V& c, double r, Color col){ drawSphere(c,r,col); }


//Draw a vertex
void drawVertex(v p, int r, Color col){
	for(int i=0; i<=r; ++i)
		for(double j=0; j<2*M_PI; j+=0.05)
			setPixel(v(i*cos(j)+p[0],i*sin(j)+p[1],2,1),col);
}

//Draw a triangle
void drawTriangle(v a, v b, v c, bool fill, Color col){
	if(fill){
		v* s[3];
		if(a[1]<b[1]){
			if(a[1]<c[1]){
				s[0]=&a;
				if(b[1]<c[1]){s[1]=&b; s[2]=&c;}
			   	else {s[1]=&c; s[2]=&b;}
			} else {
				s[0]=&c;
				if(b[1]<a[1]){s[1]=&b; s[2]=&a;}
			   	else {s[1]=&a; s[2]=&b;}
			}
		} else {
			if(b[1]<c[1]){
				s[0]=&b;
				if(a[1]<c[1]){s[1]=&a; s[2]=&c;}
				else {s[1]=&c; s[2]=&a;}
			} else {
				s[0]=&c;
				if(a[1]<b[1]){s[1]=&a; s[2]=&b;}
				else {s[1]=&b; s[2]=&a;}
			}
		}


		if((*s[1])[0]>(*s[2])[0]) std::swap(s[1], s[2]);

		if((*s[1])[1]==(*s[2])[1]) fillTriangle(s[0], s[1], s[2], col);
		else if((*s[0])[1]==(*s[1])[1]) fillTriangle(s[2], s[0], s[1], col);
		else {
			v a1,a2, a3,a4;
			a1 = (*s[0]);
			if((*s[1])[1]<(*s[2])[1]){ a2=(*s[2]); a3=(*s[1]);} else {a2=(*s[1]); a3=(*s[2]);}
			a4 = v(WIDTH,a3[1],1,1);
			int denom = ((a1[0]-a2[0])*(a3[1]-a4[1])-(a1[1]-a2[1])*(a3[0]-a4[0]));
			int x=WIDTH/2,y=HEIGHT/2;
			if(denom!=0){
				x = ((a1[0]*a2[1]-a1[1]*a2[0])*(a3[0]-a4[0])-(a1[0]-a2[0])*(a3[0]*a4[1]-a3[1]*a4[0]))/denom;
				y = ((a1[0]*a2[1]-a1[1]*a2[0])*(a3[1]-a4[1])-(a1[1]-a2[1])*(a3[0]*a4[1]-a3[1]*a4[0]))/denom;
			}
			v m(x,y,a1[2],1);
			fillTriangle(&a1, &a3, &m, col);
			fillTriangle(&a2, &a3, &m, col);
		}
	} else {
		drawSegment(a,b,col);
		drawSegment(b,c,col);
		drawSegment(c,a,col);
	}
}

//Draw a cube
void drawCube(const v* cube, bool fill, Color col){
	if(fill) for(int i=0; i<12; ++i) drawTriangle(cube[cubeOrder[3*i]], cube[cubeOrder[3*i+1]], cube[cubeOrder[3*i+2]],1,col);
	 else for(int i=0; i<12; ++i) drawSegment(cube[cubeOutline[2*i]], cube[cubeOutline[2*i+1]],col);
}

//Draw a cube
void drawSphere(const V& c, double r, Color col){
	double res = 0.2;
	v pOld;
	for(double i = 0; i < 2*M_PI; i+=res){
		for(double j = 0; j < 2*M_PI; j+=res){
			v p = getPixel(V(r*sin(i)*cos(j)+c[0], r*sin(i)*sin(j)+c[1], r*cos(i)+c[2], 1));
			if(j!=0)drawSegment(p, pOld,col);
			pOld = p;
		}
	}
}

//Draw horizontal segment
void drawFillSegment(v a, v b, Color col){
	if(a[0]>b[0]) std::swap(a,b);
	for(int x=a[0]; x<b[0]; ++x) setPixel(v(x,a[1],a[2],1),col);
}

//Draw a line segment
void drawSegment(v a, v b, Color col){
	int dX = b[0]-a[0];	int dY = b[1]-a[1]; int dZ = b[2]-a[2];
	int xI = dX<0?-1:1; int yI = dY<0?-1:1; int zI = dZ<0?-1:1; 
	int l = abs(dX); int m = abs(dY); int n = abs(dZ);
	int dX2 = l<<1; int dY2 = m<<1; int dZ2 = n<<1;
	double err1, err2;
	if(l>=m && l>=n){
		err1 = dY2-l;
		err2 = dZ2-l;
		for(int i = 0; i < l; i++){
			setPixel(a,col);
			if(err1>0){a[1]+=yI; err1-=dX2;}
			if(err2>0){a[2]+=zI; err2-=dX2;}
			err1+=dY2;
			err2+=dZ2;
			a[0]+=xI;
		}
	} else if(m>=1 && m>=n){
		err1=dX2-m;
		err2=dZ2-m;
		for(int i = 0; i < m; i++){
			setPixel(a,col);
			if(err1>0){a[0]+=xI; err1-=dY2;}
			if(err2>0){a[2]+=zI; err2-=dY2;}
			err1+=dX2;
			err2+=dZ2;
			a[1]+=yI;
		}
	} else {
		err1=dY2-n;
		err2=dX2-n;
		for(int i = 0; i < n; i++){
			setPixel(a,col);
			if(err1>0){a[1]+=yI; err1-=dZ2;}
			if(err2>0){a[0]+=xI; err2-=dZ2;}
			err1+=dY2;
			err2+=dX2;
			a[2]+=zI;
		}
	}
	setPixel(a,col);
}

//Triangle filler
void fillTriangle(v* t, v* l, v*r, Color col){
	//if((*l)[0]>(*r)[0]) std::swap(l,r);
	int h = abs((*l)[1]-(*t)[1]); 
	if(h!=0){
		int lL = (*l)[0]-(*t)[0];
		int rL = (*r)[0]-(*t)[0];
		v lT = *t, rT = *t, mid=*t;
		int lOff, rOff;
		for(int i = 0; i<=h; ++i){
			if((*t)[1]<(*l)[1]){
				lOff = lL*i/h, rOff = rL*i/h;
				lT[1]=rT[1]=(i+(*t)[1]);
			} else {
				lOff = lL*(h-i)/h, rOff = rL*(h-i)/h;
				lT[1]=rT[1]=((*t)[1]-h+i);
			}
			lT[0] = (*t)[0]+lOff, rT[0] = (*t)[0]+rOff+1; 
			drawFillSegment(lT, rT, col);
		}
	}
}

//Load .OBJ file
void loadFromFile(string path){
	string err;
	tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path.c_str());
	if(!err.empty()) cerr << err << endl;
}

//Draw loaded .OBJ model
void drawObject(){
	for(size_t s=0; s<shapes.size(); ++s){
		size_t index_offset = 0;
		vector<v> shape;
		for(size_t f=0; f<shapes[s].mesh.num_face_vertices.size(); ++f){
			size_t fv = shapes[s].mesh.num_face_vertices[f];
			for(size_t v=0; v<fv;++v){
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset+v];
				double vx = attrib.vertices[3*idx.vertex_index+0];
				double vy = attrib.vertices[3*idx.vertex_index+1];
				double vz = attrib.vertices[3*idx.vertex_index+2];
				V vert(vx,vy,vz,1.0);
				Eigen::Vector4i ver = getPixel(vert);
				shape.push_back(ver);
				setPixel(ver, Color());
				setPixel(Eigen::Vector4i(ver[0]+1, ver[1], ver[2], ver[3]), Color());
				setPixel(Eigen::Vector4i(ver[0]-1, ver[1], ver[2], ver[3]), Color());
				setPixel(Eigen::Vector4i(ver[0], ver[1]+1, ver[2], ver[3]), Color());
				setPixel(Eigen::Vector4i(ver[0], ver[1]-1, ver[2], ver[3]), Color());
			}
			//for(size_t sh=0; sh<shape.size()/2; ++sh)
			//drawSegment(shape[sh], shape[sh+1], Color());
			index_offset+=fv;
		}
	}

}


//World to screen space converter
v getPixel(const V& p){
	V vec = p;
	vec = mvpM*vec; //Pass through Model-View-Projection matix
	vec/=vec[3]; //Normalize
	vec = vpM*vec; //Pass through viewport matrix
	v t(vec[0], vec[1], vec[2], vec[3]);
	return t;
}

//Set pixel
void setPixel(const v& v, Color col){
	if(v[0]<0||v[0]>WIDTH||v[1]<0||v[1]>HEIGHT||v[2]<near||v[2]>far) return;
	Uint32 c = 0; c = (((((((c<<8)|col.a)<<8)|col.r)<<8)|col.g)<<8)|col.b;
	pixels[(int)v[1]][(int)v[0]] = c;
	//zBuf[(int)v[1]][(int)v[0]] = v[2];
}


//Render image
void render(){
	SDL_UpdateTexture(tex, NULL, pixels, HEIGHT*sizeof(Uint32));
	SDL_RenderCopy(renderer, tex, NULL, NULL);
	SDL_RenderPresent(renderer);
}
