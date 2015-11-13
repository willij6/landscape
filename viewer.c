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


/*
  The main concerns:
  * Loading and adjusting the data
  * Drawing the map
  * Navigating, changing the view
  * Responding to event callbacks
  * Setting up the projection matrix correctly
  * Initializing everything
  The code is divided up accordingly.
*/


#define N 129
#define SHOW_MAZE 0

float heights[N][N];
float drainage_totals[N][N];
int mazeData[N][N];


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
      if(t)
      	mazeData[i][j] = 1;
      else
      	mazeData[i][j] = 0;
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
  normalize(heights); // redundant, if we're going to smooth
  smooth(heights); // also normalizes
  readFromFile(drainage_totals,"drainage.txt");
  normalize(drainage_totals); // redundant, if we're going to smooth
  smooth(drainage_totals); // also normalizes
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
  // convert integer values in range 0...N
  // to floats between -1 and 1
  verts[0] = 2.0*i/N-1;
  verts[1] = 2.0*j/N-1;
  verts[2] = heights[i][j]/2.5;
  color[1] = (1.0-drainage_totals[i][j])/1.2;
  color[2] = drainage_totals[i][j];
  if(SHOW_MAZE)
    color[0] = (mazeData[i][j])?0:1;
  else
    color[0] = 0;
  triangleVertex(verts,color);
}



// the main function that renders everything
void display()
{
  // step 1: set up the lights
  // (we do this here, not in myinit,
  //  because the lights move with the model)
  GLfloat top_left[] = {-1,1,1,0};
          // Why top left?
          // https://en.wikipedia.org/wiki/Terrain_cartography#Shaded_relief
  GLfloat overhead[] = {0,0,1,0};
  glLightfv(GL_LIGHT0, GL_POSITION, top_left);
  glLightfv(GL_LIGHT1, GL_POSITION, overhead);

  // step 2: draw the map
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

// Goal: a system that allows the user to move in 3 dimensions,
// and rotate the view, except that up must always be up
// (one degree of freedom is missing)



// In model coordinates longitutde, latitude, height are x, y, z.
// The projection matrix roughly converts x and y to screen coordinates,
// so if the view matrix is trivial, we get an overhead view looking down


// The general view matrix will look like
// R_phi * M * T
// Where
//     T is a translation, moving the eye position to the origin
//     M is a combination of rotation by theta around z and scaling
//         (note that scaling and rotation commuting)
//     R_phi is a rotation by angle phi around the x axis



// The user can trigger the following motions:
// - rotate: change theta and phi (but maintains up as up)
// - translate: postmult by a translation
//     NB: translation is relative to how we're turned!
// - rescale the view: postmult by a rescaling
//     Actually a rescaling around a point in front of the viewer
//     Again, this is relative to how we're turned, so we get
//     Away with postmultiplication


// Claim: these transformations preserve the form R_phi*M*T,
//        and the latter two don't change phi:
// Proof:
// If Z is a combination of rescaling and translation (a homothety)
// then Z*R_phi*M*T = R_phi*M*Z'*T for some homothety Z'
// because homotheties are a normal subgroup.
//
// And then Z' = A*T' for some scaling A and some translation T',
// so Z*R_phi*M*T = R_phi*M*Z'*T = R_phi*(M*A)*(T'*T)



// Frequently, we need to postmultiply by something
// Since OpenGL only knows about premultiplication (mult on the right),
// we have to instead...
// - save the ModelView Matrix in a variable m
// - load whatever matrix we want to change the ModelView by
// - right multiply by m



// To enact the rotation one, we need to change phi and theta.
// The best way to change theta is to strip off R_phi, post multiply
// by a rotation by dtheta, and then put R_phi back.

// To do this, we need to store phi:
// R_phi is glRotatef(-phi,1,0,0)
// phi is basically the inclination, or some proxy for it
float phi;







// When the user zooms in and out,
// we should zoom around a specific point
// Ideally, this would be the point behind the
// mouse, but since selection isn't working TODO
// instead use as a proxy the point
//  (0,0,-focus)
GLfloat focus = 1.1;
// Also, to start, the center of the map should be there

// called at the start
void initialModelView()
{
  phi = 45; // start with some inclination

  glTranslatef(0,0,-focus); // multiply on left by some conjugate of
				// desired T
  glRotatef(-phi,1,0,0); // R_phi
  glRotatef(45,0,0,1); // M

}

// called when the user rescales the view
// factor is the ratio of how much stuff should be scaled by
void rescale(GLfloat factor)
{
  // postmultiply by a rescaling,
  // or rather a rescaling around a point in front
  // of the viewer, which we get by conjugating
  // the rescaling by translation.

  // Also, OpenGL only knows about premultiplication,
  GLfloat oldMV[16];
  glGetFloatv(GL_MODELVIEW_MATRIX,oldMV); // save the Model-View matrix
  // load the matrix which we want to multiply Model-View BY...
  glLoadIdentity();
  glTranslatef(0,0,-focus);
  glScalef(factor,factor,factor); // the actual rescaling
  glTranslatef(0,0,focus);
  // multiply on the RIGHT by the original model view matrix
  glMultMatrixf(oldMV);
}

// translate the view, relative to the current orientation
void translate(GLfloat forward, GLfloat right, GLfloat up)
{
  // new modelview = translation * (old modelview)
  GLfloat oldMV[16];
  // store old model view
  glGetFloatv(GL_MODELVIEW_MATRIX,oldMV);
  // load the translation
  glLoadIdentity();
  glTranslatef(right,up,forward);
  // multiply on the RIGHT by the old model view
  glMultMatrixf(oldMV);
}

// rotate the view, maintaining up as up
void rotate(GLfloat up, GLfloat right) {
  // new model view = R_(new phi) * R_(d theta) * R_(- old phi) * (old model view)
  GLfloat oldMV[16];
  // save the old model view
  glGetFloatv(GL_MODELVIEW_MATRIX,oldMV);
  // load the three rotations
  glLoadIdentity();
  float oldphi = phi;
  phi += up;
	  // OpenGL always mult's on the right,
	  // so they're in reverse order
	  glRotatef(-phi,1,0,0);
	  glRotatef(-right,0,0,1);
	  glRotatef(oldphi,1,0,0);
  // RIGHT multiply by the original model view
  glMultMatrixf(oldMV);

}

// set phi to 90, so the user is looking horizontally
void levelOut() {
  // new model view = R_(new phi) * R_(- old phi) * (old model view)
  // save the old model-view matrix
  GLfloat oldMV[16];
  glGetFloatv(GL_MODELVIEW_MATRIX,oldMV);
  // load the new ones
  float oldphi = phi;
  phi = 90;
  glLoadIdentity();
  glRotatef(-phi,1,0,0);
  glRotatef(oldphi,1,0,0);
  glMultMatrixf(oldMV);
}




// CALLBACKS FOR FLYING AROUND
// What follows is a list of functions that are directly registered
// with GLUT as callbacks to call when the user
// * Clicks and drags (mymouse and mymove)
// * Presses certain keys (keyfunc and specialkeyfunc)

// These actions are translated into calls to
// the rescale, rotate, translate, and levelOut functions above

// Also, the SPACE bar causes the heights to be rescaled.

// The current controls can be read off from the functions below.
// They're not so great, and will be changed in the future.



// called for "ordinary" keys, like letters
void keyfunc(unsigned char key, int x, int y)
{
  if(key == ' ') {
    // this changes all the heights to be a bit smoother
    smooth(heights);
  }
  else if(key == 'w') {
    translate(0.07,0,0); // move forward
  }
  else if(key == 's') {
    translate(-0.07,0,0); // move backwards
  }
  else if(key == 'a') {
    rotate(0,5); // turn left
  }
  else if(key == 'd') {
    rotate(0,-5); // turn right
  }
  glutPostRedisplay();
}

// called for special keys like arrows and home
void specialkeyfunc(int key, int x, int y)
{
  if(key == GLUT_KEY_UP) {
    translate(0,0,-0.07); // translate upwards
  }
  else if(key == GLUT_KEY_DOWN) {
    translate(0,0,0.07); // translate downwards
  }
  else if(key == GLUT_KEY_LEFT) {
    translate(0,0.07,0); // translate left
  }
  else if(key == GLUT_KEY_RIGHT) {
    translate(0,-0.07,0); // translate right
  }
  else if(key == GLUT_KEY_HOME) {
    levelOut();
  }
  glutPostRedisplay();
}


// Mouse Callbacks

// the mouse callbacks are more confusing,
// since we want to respond to mouse motions,
// not to clicks in specific locations
//
// Also, we need to remember which button is
// pressed, the left or right one...



int oldx, oldy; // most recent mouse location
int mode = 0; // 1 = left-drag, -1 = right-drag,
	      // 0 = not dragging at all




// left-dragging translates left/right/forward/backwards
// (but not up or down)
// right-dragging rotates

// Additionally, there's the mouse wheel, which triggers
// rescaling.  OpenGL things of scrolling as pushing
// buttons 3 and 4 on the mouse


// Callback called when the mouse is moved
// with at least one button pressed
void mymove(int x, int y)
{
  // compare new location to old location
  // and record the new location
  int dx = x - oldx;
  int dy = y - oldy;
  oldx = x;
  oldy = y;

  if(mode == 1)
    rotate(dy*0.3,dx*0.3);
  else if(mode == -1)
    translate(dy*0.01,dx*0.01,0);
  glutPostRedisplay();
}


 
// Callback for pressing or releasing mouse buttons,
// or mouse-scrolling
void mymouse(int button, int state, int x, int y)
{
  if(state == GLUT_UP)
    mode = 0; // end tracking
  if(state == GLUT_DOWN)
  {
    // first, check for mouse wheel!
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
    // otherwise, set up tracking
    oldx = x;
    oldy = y;
    if(button == GLUT_LEFT_BUTTON)
      mode = 1;
    else // hopefully, button == GLUT_RIGHT_BUTTON
      mode = -1;
  }
}



// THE PROJECTION MATRIX AND RESIZING




// The projection matrix is set in the following
// function, which is called when the event loop
// begins (why?) and also when the window resizes
void myreshape(int width, int height)
{
  // The projection matrix is a perspective view,
  // with the eye at the origin

  // adjust the viewport, otherwise rescaling won't work
  glViewport(0,0,width,height);


  // Calculate values for the call to glFrustum()
  
  // distance to front clipping pane
  GLdouble front = 0.03;
  // distance to rear clipping pane
  GLdouble back = 30;
  // slope of the sides of the frustum
  // (high values look unnatural)
  GLdouble slope = 0.7;
  GLdouble left = -slope*front;
  GLdouble right = slope*front;
  GLdouble bottom = -slope*front;
  GLdouble top = slope*front;
  if(width < height) {
    bottom = bottom*height/width;
    top = top*height/width;
  }
  else {
    left = left*width/height;
    right = right*width/height;
  }

  // set the projection matrix using glFrustum
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(left,right,bottom,top,front,back);
  glMatrixMode(GL_MODELVIEW);
  
  glutPostRedisplay();
}



// INITIALIZATION

  
void gl_init()
{
  // set up various flags
  glShadeModel(GL_SMOOTH); // smooth shading
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE); // two-sided lighting
  glEnable(GL_DEPTH_TEST); // need this for z-buffer depth to work
  glEnable(GL_NORMALIZE); // otherwise, the normals aren't normalized and
			  // shading acts screwy when you zoom in and out
  glEnable(GL_LIGHTING); // yes, we would like to use lighting
  
  // we'll use two lights: light0 and light1
  // light0 is the top left lighting one,
  // while light1 is directly above the model
  // See the start of display()
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHT1);
  GLfloat diffuse[] = {0.4,0.4,0.4,1};
  GLfloat specular[] = {0.1,0.1,0.1,1};
  glLightfv(GL_LIGHT0, GL_DIFFUSE,diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR,specular);
  glLightfv(GL_LIGHT1, GL_DIFFUSE,diffuse);
  glLightfv(GL_LIGHT1, GL_SPECULAR,specular);
  // the locations of the lights are set when
  // display() is called

  // set up default material
  glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 3);
  // most of the material parameters are set up
  // when display() is called, specifically within triangleVertex()

  // the background color--the color of the sky
  glClearColor(1.0, 1.0, 1.0, 0.0);

  // the Matrices  
  // initialProjection() ; // taken care of by the reshape callback
  initialModelView();

  
}

// set up all the GLUT-related stuff
void glut_init(int argc, char **argv)
{
  glutInit(&argc, argv);
  // use double buffering
  // do z-buffer depth tests
  // use RGB (rather than color index) <- this is on by default
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  // set up the window
  glutInitWindowSize(700,700);
  glutInitWindowPosition(0,0);
  glutCreateWindow("Virtual Landscape");
  // register the callbacks
  glutDisplayFunc(display);
  glutMotionFunc(mymove);
  glutReshapeFunc(myreshape); // window resized
  glutMouseFunc(mymouse);
  glutKeyboardFunc(keyfunc); // normal keyboard keys
  glutSpecialFunc(specialkeyfunc); // special keyboard keys
}


int main(int argc, char **argv)
{
  // load the elevation and color data
  loadData(argc,argv);
  // initialize GLUT
  glut_init(argc,argv);
  // initialize GL
  gl_init();
  glutPostRedisplay(); // TODO: is leaving this out a bad idea?
  glutMainLoop();
  return 0;
}
