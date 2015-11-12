#include <stdio.h>
#include <math.h>
#include <GL/glut.h>
#include <stdlib.h>


/*
  viewer.c
  Reads in some height data from heights.txt and renders it
  using Ancient OpenGL and GLUT.  TODO: upgrade to Modern OpenGL

  Colors are determined by data from drainage.txt: higher
  numbers are blue, lesser numbers are green

  The user can fly around the rendered landscape using some
  mouse and keyboard controls
*/




#define N 129

float heights[N][N];
float drainage_totals[N][N];
// int mazeData[N][N];


// CODE FOR LOADING AND ADJUSTING THE DATA 

// This linearly rescales the contents of the array
// so that the lowest value is 0 and the highest is 1
void normalize(float array[N][N])
{
  float min, max;
  min = max = (**array);
  int i, j;
  // first pass: calculate the minimum and maximum
  for(i = 0; i < N; i++) {
    for(j = 0; j < N; j++) {
      if(array[i][j] > max)
	max = array[i][j];
      if(array[i][j] < min)
	min = array[i][j];
    }
  }
  // x maps to (x - min)/(max - min) = (x - min)/scale
  float scale = max - min;
  if(scale == 0.0)
    scale = 1;
  // second pass: adjust everything
  for(i = 0; i < N; i++) {
    for(j = 0; j < N; j++) {
      array[i][j] = (array[i][j] - min)/scale;
    }
  }
}
 
// replace every value in the array with
// the average of its immediate neighborhood
void smooth(float array[N][N])
{
  float newArray[N][N]; // temporarily store new values here
  for(int i = 0; i < N; i++) {
    for(int j = 0; j < N; j++) {
      float total = 0; // running total of
      int count = 0; // size of neighborhood
      
      // Iterate over a five-by-five neighborhood
      for(int ii = i-1; ii < i+2; ii++) {
	for(int jj = j-1; jj < j+2; jj++) {
	  if(ii < 0 || ii >= N || jj < 0 || jj >= N)
	    continue;
	  total += array[ii][jj];
	  count++;
	}
      }
      // usually count == 5 now, but this isn't the case
      // near the edges
      newArray[i][j] = total/count;
    }
  }
  // Now copy newArray back into array
  for(int i = 0; i < N; i++) {
    for(int j = 0; j < N; j++) {
      array[i][j] = newArray[i][j];
    }
  }
  normalize(array);
}

// reads from fname into array
void readFromFile(float array[N][N], char *fname)
{
  FILE *pFile;
  float t;
  pFile = fopen(fname,"r");
  for(int i = 0; i < N; i++) {
    for(int j = 0; j < N; j++) {
      // read a floatinto array[i][j]
      fscanf(pFile, "%f", &t);
      array[i][j] = t;
      /* if(t) */
      /* 	mazeData[i][j] = 1; */
      /* else */
      /* 	mazeData[i][j] = 0; */
    }
  }
  fclose(pFile);
}


// Do all the setup: load the data
// in from heights.txt (or whatever file the user specified)
// and drainage.txt
void loadData(int argc, char **argv)
{
  readFromFile(heights,(argc>1)?argv[1]:"heights.txt");
  normalize(heights);
  smooth(heights);
  readFromFile(drainage_totals,"drainage.txt");
  normalize(drainage_totals);
  /* smooth(drainage_totals); */
}


// CODE FOR DRAWING THE TRIANGLES

// First, some utility functions

// dot product of two vectors
// TODO: this is probably in a library
GLfloat dot(GLfloat* p1, GLfloat* p2)
{
  return(p1[0]*p2[0] + p1[1]*p2[1] + p1[2]*p2[2]);
}


// calculate a unit vector d normal to the triangle with
// vertices at a, b, and c
void calculateNormal(GLfloat* a, GLfloat* b, GLfloat* c, GLfloat* d)
{
  // first, set d to the cross product of b-a and c-a
  d[0] = (b[1]-a[1])*(c[2]-a[2])-(b[2]-a[2])*(c[1]-a[1]);
  d[1] = (b[2]-a[2])*(c[0]-a[0])-(b[0]-a[0])*(c[2]-a[2]);
  d[2] = (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);
  // then renormalize it
  float length = sqrt(dot(d,d));
  for(int i = 0; i < 3; i++) d[i] /= length;
}


// accept a vertex at location pt with color col
// Every third vertex, draw a triangle
// (with the correct normal)
// pt and col are arrays of length 3
void triangleVertex(GLfloat* pt, GLfloat *col) {
  static GLfloat vertices[3][3];
  static GLfloat colors[3][4];
  static int count = 0; // is this the first, second, or third vertex?
  for(int i = 0; i < 3; i++) {
    vertices[count][i] = pt[i];
    colors[count][i] = col[i];
  }
  colors[count][3] = 1.0; // alpha
  // check if this is one of every third vertex
  if(++count < 3)
    return;
  count = 0;

  // draw a triangle
  // first calculate the normal
  GLfloat normal[3];
  calculateNormal(vertices[0],vertices[1],vertices[2],normal);
  glNormal3fv(normal); // set it
  // now tell OpenGL the vertices' locations and colors
  for(int i = 0; i < 3; i++) {
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colors[i]);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, colors[i]);
    glVertex3fv(vertices[i]);
  }

}

// process the point at i, j:
// calculate the correct location and color, and send
// them to triangleVertex()
//
// Like triangleVertex and glVertex3fv, this needs to
// be called in meaningful sets of three.
void process_point(int i, int j)
{
  GLfloat verts[3];
  GLfloat color[3];
  verts[0] = 2.0*i/N-1;
  verts[1] = 2.0*j/N-1;
  verts[2] = heights[i][j]/2.5-0.5;
  color[1] = 1.0-drainage_totals[i][j];
  color[2] = drainage_totals[i][j];
  color[0] = 0; // (mazeData[i][j])?0:1;
  triangleVertex(verts,color);
}



// the main function that renders everything
void display()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glBegin(GL_TRIANGLES);
  for(int j = 0; j < N-1; j++) {
    for(int i = 0; i < N-1; i++) {
      // first triangle
      process_point(i,j);
      process_point(i+1,j);
      process_point(i,j+1);
      // second triangle
      process_point(i+1,j+1);
      process_point(i+1,j);
      process_point(i,j+1);
    }
  }
  glEnd();
  glutSwapBuffers();
}





// CHANGING THE VIEW, FLYING AROUND



#define VIEWBOX 1.42



// angle of inclination, sort of
float varphi;
// TODO: what was I thinking here?


  
void initialView()
{
  varphi = 45;
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  float w = VIEWBOX;
  glOrtho(-w,w,-w,w,-w,w);
  /* glOrtho(-1.5,1.5,-1.5,1.5,-1.5,1.5); */
  glMatrixMode(GL_MODELVIEW);
  glTranslatef(0,0,-2*VIEWBOX);
  glRotatef(-45,1,0,0); // TODO: is this varphi?
  glRotatef(45,0,0,1); // TODO: is this varphi?
}


void rescale(GLfloat factor)
{
  GLfloat m[16];
  glGetFloatv(GL_MODELVIEW_MATRIX,m);
  glLoadIdentity();
  glTranslatef(0,0,-2*VIEWBOX);
  glScalef(factor,factor,factor);
  glTranslatef(0,0,2*VIEWBOX);
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







 





void myreshape(int width, int height)
{
  int w, h;
  w = width;
  h = height;
  glViewport(0,0,w,h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  if(width < height) {
    glFrustum(-0.1*VIEWBOX,0.1*VIEWBOX,-0.1*VIEWBOX*height/width,0.1*VIEWBOX*height/width, VIEWBOX/10,10*VIEWBOX);
}
  else {
    glFrustum(-0.1*VIEWBOX*width/height,0.1*VIEWBOX*width/height,-0.1*VIEWBOX,0.1*VIEWBOX,VIEWBOX/10,10*VIEWBOX);
  }


  glMatrixMode(GL_MODELVIEW);
  glutPostRedisplay();
}



int oldx, oldy;
int tracking = 0;

 
 
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
      smooth(heights);
  }
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
  /* GLfloat all_ones[] = {1, 1, 1, 1}; */
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

  initialView();
  glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
}



int main(int argc, char **argv)
{
  loadData(argc,argv);
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE |  GLUT_RGB | GLUT_DEPTH);
  glutInitWindowSize(700,700);
  glutInitWindowPosition(0,0);
  glutCreateWindow("Virtual Landscape");
  glutDisplayFunc(display);
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
