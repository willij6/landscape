#include <stdio.h>
// #define GLUT_DISABLE_ATEXIT_HACK
// #include <windows.h>
#include <math.h>
#include <GL/glut.h>
#include <stdlib.h>

int w, h, oldx, oldy;
int tracking = 0;


#define N 129

float heights[N][N];
float drainage_totals[N][N];
int max_height;


float viewbox = 1.42;


float varphi = 45;


void rescale(GLfloat factor)
{
  GLfloat m[16];
  glGetFloatv(GL_MODELVIEW_MATRIX,m);
  glLoadIdentity();
  glTranslatef(0,0,-2*viewbox);
  glScalef(factor,factor,factor);
  glTranslatef(0,0,2*viewbox);
  glMultMatrixf(m);
}

void translate(GLfloat forward, GLfloat right, GLfloat up)
{
  GLfloat m[16];
  glGetFloatv(GL_MODELVIEW_MATRIX,m);
  glLoadIdentity();
  glTranslatef(right,up,forward);
  glMultMatrixf(m);
}

void rotate(GLfloat up, GLfloat right) {
  GLfloat m[16];
  glGetFloatv(GL_MODELVIEW_MATRIX,m);
  glLoadIdentity();
  float newvarphi = varphi + up;
  glRotatef(-newvarphi,1,0,0);
  glRotatef(-right,0,0,1);
  glRotatef(varphi,1,0,0);
  glMultMatrixf(m);
  varphi = newvarphi;
}

void levelOut() {
  GLfloat m[16];
  glGetFloatv(GL_MODELVIEW_MATRIX,m);
  glLoadIdentity();
  glRotatef(-90,1,0,0);
  glRotatef(varphi,1,0,0);
  glMultMatrixf(m);
  varphi = 90;
}

void set_max_height()
{
  int i, j;
  max_height = 0.0;
  for(i = 0; i < N; i++) {
    for(j = 0; j < N; j++) {
      if(heights[i][j] > max_height)
  max_height = heights[i][j];
    }
  }
}

float outside_lands(int i, int j)
{
  if(i < 0 || i >= N || j < 0 || j >= N) {
    return 0;
  }
  return heights[i][j];
}

void mollify()
{
  float alts[N][N];
  int i,j, ii, jj;
  float t;
  for(i = 0; i < N; i++) {
    for(j = 0; j < N; j++) {
      t = 0;
      for(ii = -1; ii < 2; ii++) {
	for(jj = -1; jj < 2; jj++) {
	  t += outside_lands(i+ii,j+jj);
	}
      }
      t /= 9;
      alts[i][j] = t;
    }
  }
  for(i = 0; i < N; i++) {
    for(j = 0; j < N; j++) {
      heights[i][j] = alts[i][j];
    }
  }
  set_max_height();
}

void set_heights(char *fname)
{
  FILE *pFile;
  int i, j;
  float t;
  pFile = fopen(fname,"r");
  for(i = 0; i < N; i++) {
    for(j = 0; j < N; j++) {
      fscanf(pFile, "%f", &t);
      heights[i][j] = t;
      /* printf("I just read %d\n",t); */
    }
  }
  mollify();
  fclose(pFile);
}




void mytrivert(GLfloat* pt);
void mycolor(GLfloat* col);

void whatever(int i, int j)
{
  GLfloat verts[3];
  GLfloat color[3];
  verts[0] = 2.0*i/N-1;
  verts[1] = 2.0*j/N-1;
  verts[2] = heights[i][j]/max_height/1.5-0.5;
  color[1] = 1.0-drainage_totals[i][j];
  color[2] = drainage_totals[i][j];
  color[0] = 0;
  // TODO: this doesn't make sense
  /* if(selecting) */
  /*   glLoadName(i*N+j); */
  mycolor(color);
  mytrivert(verts);
}

void drawObjects()
{
  int i, j;
  glBegin(GL_TRIANGLES);
  for(j = 0; j < N-1; j++) {
    for(i = 0; i < N-1; i++) {
      whatever(i,j);
      whatever(i+1,j);
      whatever(i,j+1);
      whatever(i+1,j+1);
      whatever(i+1,j);
      whatever(i,j+1);
    }
  }
  glEnd();
  glutSwapBuffers();
}


GLfloat dist(GLfloat* p1, GLfloat* p2)
{
  GLfloat dx, dy, dz;
  dx = p1[0] - p2[0];
  dy = p1[1] - p2[1];
  dz = p1[2] - p2[2];
  return sqrt(dx*dx + dy*dy + dz*dz);
}

GLfloat dot(GLfloat* p1, GLfloat* p2)
{
  return(p1[0]*p2[0] + p1[1]*p2[1] + p1[2]*p2[2]);
}

void cross(GLfloat* a, GLfloat* b, GLfloat* c, GLfloat* d)
{
  GLfloat la;
  //GLfloat sum[3];
  int i;
  d[0] = (b[1]-a[1])*(c[2]-a[2])-(b[2]-a[2])*(c[1]-a[1]);
  d[1] = (b[2]-a[2])*(c[0]-a[0])-(b[0]-a[0])*(c[2]-a[2]);
  d[2] = (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);
  //for(i = 0; i < 3; i++) sum[i] = a[i]+b[i]+c[i];
  la = dot(d,d);
  la = sqrt(la);
  // if(dot(d,sum) > 0) la = -la;
  for(i = 0; i < 3; i++) d[i] /= la;
  //printf("Sum was (%f,%f,%f)\n",sum[0],sum[1],sum[2]);
  //printf("Normal was (%f,%f,%f)\n",d[0],d[1],d[2]);
  
}

void mytrivert(GLfloat* pt) {
  static GLfloat vertices[3][3];
  static int stored = 0;
  int i;
  GLfloat normal[3];
  for(i = 0; i < 3; i++)
    vertices[stored][i] = pt[i];
  stored++;
  if(stored < 3)
    return;
  cross(vertices[0],vertices[1],vertices[2],normal);
  glNormal3fv(normal);
  for(i = 0; i < 3; i++)
    glVertex3fv(vertices[i]);
  stored = 0;
}



void myinit()
{
  GLfloat mat_specular[] = {0.0, 0.0, 0.0,1.0};
  GLfloat mat_diffuse[] = {0.0,0.0, 0.0, 1.0}; // though, this'll be changed later
  GLfloat mat_ambient[] = {0.0, 0.0, 0.0, 1.0};
  GLfloat mat_shininess= 100.0; // wait, this doesn't matter

  /* glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient); */
  /* glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse); */
  /* glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular); */
  // default position is vertical, which is okay, for now
  // default ambient is black
  // default diffuse and specular are white
  // good enough
  GLfloat all_ones[] = {1, 1, 1, 1};
  GLfloat halves[] = {0.5, 0.5, 0.5,1};

    
  /* glLightfv(GL_LIGHT0,GL_DIFFUSE,mat_specular); */
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
  glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);


  GLfloat light_pos[] = {0.0,0.0,1.0,0.0};
  /* glLightfv(GL_LIGHT1, GL_POSITION, light_pos); */
  glLightfv(GL_LIGHT0, GL_DIFFUSE, halves);
  glLightfv(GL_LIGHT0, GL_SPECULAR, halves);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, halves);
  
  glShadeModel(GL_SMOOTH);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHT1);
  
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

  
  glClearColor(1.0, 1.0, 1.0, 0.0);
  glColor3f(1.0,0.0,0.0);
  
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  float w = viewbox;
  glOrtho(-w,w,-w,w,-w,w);
  /* glOrtho(-1.5,1.5,-1.5,1.5,-1.5,1.5); */
  glMatrixMode(GL_MODELVIEW);
  glTranslatef(0,0,-2*viewbox);
  glRotatef(-45,1,0,0);
  glRotatef(45,0,0,1);
  glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
}

 

void mycolor(GLfloat* arg) {
  GLfloat color[4];
  int i;
  for(i = 0; i < 3; i++)
    color[i] = arg[i];
  color[3] = 1.0;
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);
}

void display()
{

  int i, j,k, ell, jay;
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  drawObjects();
  glutSwapBuffers();
}


int max(int x, int y)
{
  if(x > y) 
    return x;
  return y;
}

void myreshape(int width, int height)
{
  w = width;
  h = height;
  glViewport(0,0,w,h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  if(width < height) {
    glFrustum(-0.1*viewbox,0.1*viewbox,-0.1*viewbox*height/width,0.1*viewbox*height/width, viewbox/10,10*viewbox);
}
  else {
    glFrustum(-0.1*viewbox*width/height,0.1*viewbox*width/height,-0.1*viewbox,0.1*viewbox,viewbox/10,10*viewbox);
  }


  glMatrixMode(GL_MODELVIEW);
  glutPostRedisplay();
}




 
 
void mymouse(int button, int state, int x, int y)
{
  if(state == GLUT_DOWN)
  {
    if(button == 3) {
      rescale(1.1);
      glutPostRedisplay();
      return;
    }
    if(button == 4) {
      rescale(0.9);
      glutPostRedisplay();
      return;
    }
    oldx = x;
    oldy = y;
    if(button == GLUT_LEFT_BUTTON)
      tracking = 1;
    else
      tracking = -1;
    // printf("tracking");
  }
  if(state == GLUT_UP)
  {
    tracking = 0;
    // printf("not tracking");
  }
}

  

      

void mymove(int x, int y)
{
  GLfloat m[16];
  if(tracking == 1)
  {
    int dx = x - oldx;
    int dy = y - oldy;
    oldx = x;
    oldy = y;
    /* rotate(0,0); */
    rotate(dy*0.3,dx*0.3);
  }
  else if(tracking == -1)
  {
    int dx = x - oldx;
    int dy = y - oldy;
    oldx = x;
    oldy = y;
    translate(dy*0.03,dx*0.03,0);
  }
  GLfloat light_pos[] = {0.0,0.0,1.0,0.0};
  glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
  glutPostRedisplay();
      
}


void keyfunc(unsigned char key, int x, int y)
{
  if(key == ' ') {
      mollify();
  }
  /* else if(key == '4') { */
  /*     levelOut(); */
  /* } */
  else if(key == 'w') {
    translate(0.07,0,0);
  }
  else if(key == 's') {
    translate(-0.07,0,0);
  }
  else if(key == 'a') {
    rotate(0,5);
  }
  else if(key == 'd') {
    rotate(0,-5);
  }
  glutPostRedisplay();
}

void specialkeyfunc(int key, int x, int y)
{
  if(key == GLUT_KEY_UP) {
    translate(0,0,-0.07);
  }
  else if(key == GLUT_KEY_DOWN) {
    translate(0,0,0.07);
  }
  else if(key == GLUT_KEY_LEFT) {
    translate(0,0.07,0);
  }
  else if(key == GLUT_KEY_RIGHT) {
    translate(0,-0.07,0);
  }
  else if(key == GLUT_KEY_HOME) {
    levelOut();
  }
  glutPostRedisplay();
}


void load_drainage_hack()
{
  int i, j;
  set_heights("drainage.txt");
  mollify();
  
  for(i = 0; i < N; i++) {
    for(j = 0; j < N; j++) {
      drainage_totals[i][j] = heights[i][j]/max_height;
      /* drainage_totals[i][j] = sqrt(drainage_totals[i][j]); */
    }
  }

}


int main(int argc, char **argv)
{
  load_drainage_hack();
  set_heights((argc>1)?argv[1]:"heights.txt");
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE |  GLUT_RGB | GLUT_DEPTH);
  glutInitWindowSize(700,700);
  glutInitWindowPosition(0,0);
  glutCreateWindow("Virtual Landscape");
  glutDisplayFunc(display);
  /* glutIdleFunc(spinme); */
  glutMotionFunc(mymove);
  glutReshapeFunc(myreshape);
  glutMouseFunc(mymouse);
  glutKeyboardFunc(keyfunc);
  glutSpecialFunc(specialkeyfunc);
  myinit();
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_NORMALIZE);
  glutPostRedisplay();
  glutMainLoop();
  return 0;
}
