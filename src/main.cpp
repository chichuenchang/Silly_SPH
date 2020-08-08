#include <string>
#include <GL/freeglut.h>
#include "SPH.h"
#include "trackball.h"

TrackBallC trackball;
bool mouseLeft, mouseMid, mouseRight;

//declare global variables here
SPH mySph;
GLint wWindow = 600;
GLint hWindow = 400;

std::vector<glm::vec3> gridVertArray;
std::vector<std::vector<glm::vec3>> trigStrip2DArray;

//input control
bool drawNorm = false;
bool triangle = false;

glm::vec3 offset = {0.0f, 0.0f, 0.0f};

void GLMessage(char* message, int xPos, int yPos)
{
	static int i;
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0.f, 100.f, 0.f, 100.f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glColor3ub(0, 0, 255);
	glRasterPos2i(xPos, yPos);
	for (i = 0; i < (int)strlen(message); i++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, message[i]);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
}

void DrawLine(glm::vec3 a, glm::vec3 b, glm::vec3 color) {

	glColor3fv(&color[0]);
	glBegin(GL_LINES);
	glVertex3fv(&a[0]);
	glVertex3fv(&b[0]);
	glEnd();
}

//draws point at a with color 
void DrawPoint(glm::vec3 a, glm::vec3 color, float pSize) {

	glColor3fv(&color[0]);
	glPointSize(pSize);
	glBegin(GL_POINTS);
	glVertex3fv(&a[0]);
	glEnd();
}

inline float random11() {
	return 2.f * rand() / (float)RAND_MAX - 1.f;
}

inline float randomFlt() {
	return rand() / (float)RAND_MAX;
}

//randomizes an existing curve by adding random number to each coordinate
void Randomize(std::vector <glm::vec3> *a) {
	const float intensity = 0.01f;

	std::vector<glm::vec3>::iterator iter;
	for (iter = a->begin(); iter != a->end(); iter++) {
		glm::vec3 r(random11(), random11(), random11());
		*iter += intensity * r;
	}
}

////randomizes an existing curve by adding random number to each coordinate
void Randomize2DArray(std::vector <std::vector <glm::vec3>>* a) {

	const float intensity = 0.01f;

	std::vector<std::vector<glm::vec3>>::iterator it_i;
	for (it_i = a->begin(); it_i != a->end(); it_i++) {

		std::vector<glm::vec3>::iterator it_j;
		for (it_j = it_i->begin(); it_j != it_i->end(); it_j++) {

			glm::vec3 r(random11(), random11(), random11());
			*it_j += intensity * r;
		}

	}
}

void Offset2DArray(std::vector <std::vector <glm::vec3>>* a, glm::vec3 offset) {

	const float intensity = 0.003f;

	std::vector<std::vector<glm::vec3>>::iterator it_i;
	for (it_i = a->begin(); it_i != a->end(); it_i++) {

		std::vector<glm::vec3>::iterator it_j;
		for (it_j = it_i->begin(); it_j != it_i->end(); it_j++) {

			//glm::vec3 r(random11(), random11(), random11());
			*it_j += intensity * offset;
		}

	}
}


//display coordinate system
void CoordSyst() {
	glm::vec3 a, b, c;
	glm::vec3 origin(0, 0, 0);
	glm::vec3 red(1, 0, 0), green(0, 1, 0), blue(0, 0, 1), almostBlack(0.1f, 0.1f, 0.1f), yellow(1, 1, 0);

	//draw the coordinate system 
	a = glm::vec3(1, 0, 0);
	b = glm::vec3(0, 1, 0);
	c = glm::vec3(cross(a, b)); //use cross product to find the last vector
	glLineWidth(4);
	DrawLine(origin, a, red);
	DrawLine(origin, b, green);
	DrawLine(origin, c, blue);
	glLineWidth(1);
}

void DrawTriangle() {
	std::vector <std::vector<Triangle*>> trigArray2D = mySph.Get_TriangleArray2D();
	std::vector <std::vector<Triangle*>>::iterator it_Array;
	for (it_Array = trigArray2D.begin(); it_Array != trigArray2D.end(); it_Array++) {
		std::vector <Triangle*>::iterator it_trig;
		for (it_trig = it_Array->begin(); it_trig != it_Array->end(); it_trig++) {
			DrawLine((*it_trig)->getV1(), (*it_trig)->getV2(), glm::vec3(1, 0, 0));
			DrawLine((*it_trig)->getV2(), (*it_trig)->getV3(), glm::vec3(1, 0, 0));
			DrawLine((*it_trig)->getV3(), (*it_trig)->getV1(), glm::vec3(1, 0, 0));
			
			if (drawNorm) {
				glm::vec3 c = 1.0f / 3.0f * ((*it_trig)->getV1() + (*it_trig)->getV2() + (*it_trig)->getV3());
				DrawLine(c, c+ 0.1f*(*it_trig)->getNorm(), glm::vec3(0.1f, 0.2f, 0.3f));
			}
		}
	}
}

void DrawParticles() {
	std::vector <Particle*> pArray = mySph.Get_ParticleArray();
	std::vector <Particle*>::iterator iter;
	for (iter = pArray.begin(); iter != pArray.end(); iter++) {
		DrawPoint((*iter)->pos, glm::vec3(0.25f, 0.3f, 0.8f), 12.0f);
	}
}

void createVertData(std::vector<std::vector<glm::vec3>>& out_trigVert2DArray, std::vector<glm::vec3>&out_gridVertArray) {

	//out_trigVert2DArray.clear();

	glm::vec3 c = { 0.64f, 0.4f, 0.24f };
	std::vector<glm::vec3> temp;

	//bowl
	float s = 1.0f;
	temp.push_back(offset + c + s * (glm::vec3(0.4f, 0.0f, 0.0f) + glm::vec3(0.0f, 0.1f, 0.0f)));	
	temp.push_back(offset + c + s * glm::vec3(0.1f, 0.0f, 0.0f));
	temp.push_back(offset + c + s * (glm::vec3(0.0f, 0.0f, -0.2f) + glm::vec3(0.0f, 0.1f, 0.0f)));
	temp.push_back(offset + c + s * glm::vec3(0.0f, 0.0f, -0.1f));
	temp.push_back(offset + c + s * (glm::vec3(-0.2f, 0.0f, 0.0f) + glm::vec3(0.0f, 0.1f, 0.0f)));
	temp.push_back(offset + c + s * (glm::vec3(-0.1f, 0.0f, 0.0f) + glm::vec3(0.0f, 0.0f, 0.0f)));
	temp.push_back(offset + c + s * (glm::vec3(0.0f, 0.0f, 0.2f) + glm::vec3(0.0f, 0.1f, 0.0f)));
	temp.push_back(offset + c + s * (glm::vec3(0.0f, 0.0f, 0.1f) + glm::vec3(0.0f, 0.0f, 0.0f)));
	temp.push_back(offset + c + s * (glm::vec3(0.4f, 0.0f, 0.0f) + glm::vec3(0.0f, 0.1f, 0.0f)));	
	temp.push_back(offset + c + s * glm::vec3(0.1f, 0.0f, 0.0f));
	out_trigVert2DArray.push_back(temp);
	
	temp.clear();
	temp.push_back(offset + c + s * (glm::vec3(0.11f, 0.0f, 0.0f) + glm::vec3(0.0f, 0.001f, 0.0f)));
	temp.push_back(offset + c + s * (glm::vec3(0.0f, 0.0f, 0.11f) + glm::vec3(0.0f, 0.001f, 0.0f)));
	temp.push_back(offset + c + s * (glm::vec3(0.0f, 0.0f, -0.11f)+glm::vec3(0.0f, 0.001f, 0.0f)));
	temp.push_back(offset + c + s * (glm::vec3(-0.11f, 0.0f, 0.0f) + glm::vec3(0.0f, 0.001f, 0.0f)));
	out_trigVert2DArray.push_back(temp);


	/*float d = 0.05f;
	out_gridVertArray.clear();
	for (int j = 0; j < 5; j++) {

		for (int i = 0; i < 5; i++) {
			if (i == 2) d = 0.08f;
			else if (i == 1 || i == 3) d = 0.06f;
			else d = 0.04f;
			out_gridVertArray.push_back(glm::vec3(-0.2f+ 0.24f *j, 0.6f - d*j, -0.1 + 0.22f * i));

		}
	}*/


}

void initTrig2DArray(std::vector<std::vector<glm::vec3>>& in_trigStripV2DArray, std::vector<glm::vec3>& in_gridVertArray) {

	mySph.clearTriangleArray2D();

	int num_trigArray = in_trigStripV2DArray.size();
	//std::cout << "num_trigArray = " << num_trigArray << std::endl;
	for (int i = 0; i < num_trigArray; i++) {

		int num_triangle = in_trigStripV2DArray[i].size() - 2;

		Triangle* tempTrig;
		std::vector<Triangle*> tempTrigArray;
		tempTrigArray.clear();

		for (int j = 0; j < num_triangle; j++) {
			if (j % 2 == 0) {
				tempTrig = new Triangle(in_trigStripV2DArray[i][j], in_trigStripV2DArray[i][j + 2], in_trigStripV2DArray[i][j + 1]);
			}
			else {
				tempTrig = new Triangle(in_trigStripV2DArray[i][j], in_trigStripV2DArray[i][j + 1], in_trigStripV2DArray[i][j + 2]);
			}

			tempTrigArray.push_back(tempTrig);
		}
		mySph.PushTriangleToArray2D(tempTrigArray);
	}


	//int num_row = 4;
	//int num_col = 4;
	//for (int j = 0; j < num_col; j++) {
	//	Triangle* tempT;
	//	std::vector<Triangle*> tempTArray;
	//	tempTArray.clear();

	//	for (int i = 0; i < num_row; i++) {

	//		//if (j % 2 == 0) {
	//			tempT = new Triangle(in_gridVertArray[j * (num_col+1) + i], in_gridVertArray[j * (num_col + 1) + i + 1], in_gridVertArray[j * (num_col + 1) + i + num_row+1]);
	//		//}
	//			tempTArray.push_back(tempT);
	//			
	//			tempT = new Triangle(in_gridVertArray[j * (num_col + 1) + i + num_row + 1], in_gridVertArray[j * (num_col + 1) + i + 1], in_gridVertArray[j * (num_col + 1) + i + num_row + 1 +1]);

	//			tempTArray.push_back(tempT);
	//	}

	//	mySph.PushTriangleToArray2D(tempTArray);

	//}


}


void Kbd(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27: // on [ESC]
		exit(0); 
		break;
	case '-':
		mySph.Clear_ParticleArray();
		break;
	case '+':
		mySph.Init_Particles(100);
		break;
	case 32:
		mySph.Set_Density(500.0f);
		break;
	case 'r':
		Randomize(&gridVertArray);
		Randomize2DArray(&trigStrip2DArray);
		initTrig2DArray(trigStrip2DArray, gridVertArray);
		break;
	case'n':
		drawNorm = !drawNorm;
		break;
	case't':
		triangle = !triangle;
		if (!triangle) mySph.clearTriangleArray2D();
		else initTrig2DArray(trigStrip2DArray, gridVertArray);
		break;
	case'1':
		mySph.Init_Particles(1);
		break;

		//move triangles
	case'z':
		Offset2DArray(&trigStrip2DArray, glm::vec3(-1.0f, 0.0f, 0.0f));
		initTrig2DArray(trigStrip2DArray, gridVertArray);
		break;
	case'x':
		Offset2DArray(&trigStrip2DArray, glm::vec3(1.0f, 0.0f, 0.0f));
		initTrig2DArray(trigStrip2DArray, gridVertArray);
		break;
	case'a':
		Offset2DArray(&trigStrip2DArray, glm::vec3(0.0f, -1.0f, 0.0f));
		initTrig2DArray(trigStrip2DArray, gridVertArray);
		break;
	case's':
		Offset2DArray(&trigStrip2DArray, glm::vec3(0.0f, 1.0f, 0.0f));
		initTrig2DArray(trigStrip2DArray, gridVertArray);
		break;
	case'q':
		Offset2DArray(&trigStrip2DArray, glm::vec3(0.0f, 0.0f, -1.0f));
		initTrig2DArray(trigStrip2DArray, gridVertArray);
		break;
	case'w':
		Offset2DArray(&trigStrip2DArray, glm::vec3(0.0f, 0.0f, 1.0f));
		initTrig2DArray(trigStrip2DArray, gridVertArray);
		break;


	case'p':
		mySph.PCIToggle();
		break;
	}
}

void Idle()
{
	mySph.Update();
	//update your variables here
	glutPostRedisplay();
}

void reshape(int w, int h)
{
	glViewport(0, 0, w, h);
	glLoadIdentity();
	gluOrtho2D(0.0, mySph.Get_World_Size().x, 0.0, mySph.Get_World_Size().y);
	glutReshapeWindow(w, h);
}

void Display(void)
{
	glClearColor(0.5f, 0.5f, 0.5f, 1); //background color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GLMessage("Jichun Zheng, CS590 Final Project", 12, 88);
	/*GLMessage("Press '+' to add particle", 12, 83);
	GLMessage("Press 'r' to randomize triangles", 12, 78);
	GLMessage("Press 'p' to toggle PCI (Caution: may hurt your computer)", 12, 10);*/
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(40, (GLfloat)wWindow / (GLfloat)hWindow, 0.01, 100); //set the camera
	glMatrixMode(GL_MODELVIEW); //set the scene
	glLoadIdentity();
	//gluLookAt(8, 9, 10, 0, 0, 0, 0, 1, 0); //set where the camera is looking at and from. 

	glMatrixMode(GL_MODELVIEW);
	trackball.Set3DViewCamera();
	
	CoordSyst();
	
	// update() and drawcall
	
	DrawTriangle();
	DrawParticles();

	glutSwapBuffers();

}


void Mouse(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		trackball.Set(true, x, y);
		mouseLeft = true;
	}
	if (button == GLUT_LEFT_BUTTON && state == GLUT_UP)
	{
		trackball.Set(false, x, y);
		mouseLeft = false;
	}
	if (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN)
	{
		trackball.Set(true, x, y);
		mouseMid = true;
	}
	if (button == GLUT_MIDDLE_BUTTON && state == GLUT_UP)
	{
		trackball.Set(true, x, y);
		mouseMid = false;
	}
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
	{
		trackball.Set(true, x, y);
		mouseRight = true;
	}
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP)
	{
		trackball.Set(true, x, y);
		mouseRight = false;
	}
}

void MouseMotion(int x, int y) {
	if (mouseLeft)  trackball.Rotate(x, y);
	if (mouseMid)   trackball.Translate(x, y);
	if (mouseRight) trackball.Zoom(x, y);
	glutPostRedisplay();
}

void initStatus() {
	//initTriangleVerts(triangleStipArray);
	//initTriangle(triangleStipArray);

	createVertData(trigStrip2DArray, gridVertArray);
	initTrig2DArray(trigStrip2DArray, gridVertArray);
	mySph.Init_Particles(100);
}

int main(int argc, char** argv)
{
	glutInitDisplayString("stencil>=2 rgb double depth samples");
	glutInit(&argc, argv);
	glutInitWindowSize(wWindow, hWindow);
	glutInitWindowPosition(100, 50);
	glutCreateWindow("SPH Test");

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
	initStatus();

	glutDisplayFunc(Display);
	glutIdleFunc(Idle);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(Kbd);
	glutSpecialUpFunc(NULL);
	glutSpecialFunc(NULL);

	glutMouseFunc(Mouse);
	glutMotionFunc(MouseMotion);

	glutMainLoop();

	return 1;
}