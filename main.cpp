// decodeOneStep method is taken from this link with some minor adjustment: 
// https://raw.githubusercontent.com/lvandeve/lodepng/master/examples/example_decode.cpp

#include <GL/glew.h>
#include <GL/freeglut.h>

#include <cyMatrix.h>
#include <cyCore.h>
#include <cyVector.h>
#include <cyGL.h>
#include <cyTriMesh.h>

#include <iostream>
#include <fstream>
#include <string>

#include <lodepng.h>

#include <LTC.h>

using namespace std;

/////////////////////////////////////////////////////////////////////////////////////////////////

/// <summary>
/// The window width and height.
/// </summary>
unsigned int windowWidth = 800;
unsigned int windowHeight = 500;

/// <summary>
/// The mesh of the object.
/// </summary>
cy::TriMesh mesh;

/// <summary>
/// The array containing all of the vertices of the object.
/// </summary>
cyVec3f* vertices;

/// <summary>
/// The array containing all of the texture coords.
/// </summary>
cyVec2f* textures;

/// <summary>
/// The VAO for the water quad plane.
/// </summary>
GLuint waterVAO;

/// <summary>
/// The number of vertices from the object.
/// </summary>
int totalNumVert;

/// <summary>
/// Just wave things.
/// </summary>
float timePassed = 0.0f;
int tessLevel = 1;
float innerRadius = 1.0f;
float outerRadius = 20.0f;
int numOfWaves = 32;
float* waveAmplitude = new float[numOfWaves];
float* waveFrequency = new float[numOfWaves];
float* waveSpeed = new float[numOfWaves];
cyVec2f* waveDirection = new cyVec2f[numOfWaves];

/// <summary>
/// Program object used for managing shaders.
/// </summary>
cy::GLSLProgram prog;
cy::GLSLProgram triangleLineProg;
cy::GLSLProgram altProg;

/// <summary>
/// The condition to show triangulation.
/// </summary>
bool showTriangulation = false;

//// the following are global variables for camera movement

/// <summary>
/// The x and y coordinate of the mouse button.
/// </summary>
int lastMouseX, lastMouseY;

/// <summary>
/// Left mouse button state (pressed or not).
/// </summary>
bool isLeftButton = false;

/// <summary>
/// right mouse button state.
/// </summary>
bool isRightButton = false;

/// <summary>
/// Horizontal rotation angle in degrees.
/// Default: 90 Degrees
/// </summary>
float theta = 90.0f;

/// <summary>
/// Vertical rotation angle in degrees.
/// Default: 0 degrees
/// </summary>
float phi = 0.0f;

/// <summary>
/// Camera distance.
/// Default: 5 
/// </summary>
float cameraDistance = 5.0f;

/// <summary>
/// Camera vectors things.
/// </summary>
cy::Vec3f frontVector;
cy::Vec3f rightVector;
cy::Vec3f upVector = cy::Vec3f(0.0f, 1.0f, 0.0f);
cy::Vec3f camPosition = cy::Vec3f(0.0f, 4.0f, -10.0f);

/// <summary>
/// Key boolean values.
/// </summary>
bool wPressed = false;
bool aPressed = false;
bool sPressed = false;
bool dPressed = false;
bool shiftPressed = false;
bool spacePressed = false;

/// <summary>
/// the paths for environement skybox.
/// </summary>
string sideCubes[6] = {
	"skybox_night1/posx.png",
	"skybox_night1/negx.png",
	"skybox_night1/posy.png",
	"skybox_night1/negy.png",
	"skybox_night1/posz.png",
	"skybox_night1/negz.png"
};

/// <summary>
/// Program object used for managing shaders: skybox edition.
/// </summary>
cy::GLSLProgram cubeProg;

/// <summary>
/// The cube map textures for the skybox.
/// Contain all of the sides of the skybox.
/// </summary>
cy::GLTextureCubeMap envMap;

/// <summary>
/// The vertices of the skybox.
/// </summary>
cy::Vec3f* cubeVertices;

/// <summary>
/// The number of vertices of the skybox.
/// </summary>
int cubeNumVert;

/// <summary>
/// Vertex Array Object of the skybox.
/// </summary>
GLuint cubeVAO;

/// <summary>
/// Just area light things.
/// </summary>
cy::GLSLProgram areaLightProg;
cyGLTexture2D AL_Tex;
cy::TriMesh areaLightMesh;
cy::Vec3f* areaLightVertices;
cy::Vec2f* areaLightTextures;
cyVec2f* areaLightTexCorners;
cyVec3f* areaLight4Corners;
GLuint areaLightVAO;
int areaLightNumVert;
cy::Vec3f* areaLightUniqueVert;

/// <summary>
/// condition to use for end result.
/// </summary>
bool isDirectionalLight = true;
bool isTexturedLight = false;



/// <summary>
/// ltc1 and ltc2 are the two textures used for Minv of the area light.
/// </summary>
cy::GLTexture2D ltc1;
cy::GLTexture2D ltc2;

/////////////////////////////////////////////////////////////////////////////////////////////////

/// <summary>
/// Thie method change degrees into radians.
/// </summary>
/// <param name="deg"> the degree to convert </param>
/// <returns> a float of radians value</returns>
float deg2rad(double deg) {
	return deg * 4.0 * atan(1.0) / 180.0;
}

/// <summary>
/// This method creates a scaled array starting from 1.0f.
/// In general:
/// For amplitudes, use scale < 1.0f.
/// For frequencies, use scale > 1.0f.
/// </summary>
/// <param name="numOfWaves"> the number of waves to be generated </param>
/// <param name="scale"> the scale used to modify the entries </param>
/// <param name="arrayToFill"> the array pointer to the array to be filled </param>
/// <returns></returns>
void createScaledArray( int numOfWaves, float scale, float* arrayToFill) {
	if (numOfWaves < 1) {
		cerr << "Error: number of waves must be at least 1." << endl;
		return;
	}
	arrayToFill[0] = 1.0f; // first wave is always 1.0f
	if (numOfWaves == 1) {
		return;
	}
	for (int i = 1; i < numOfWaves; i++) {
		arrayToFill[i] = arrayToFill[i - 1] * scale;
	}
}

/// <summary>
/// This method generates a random number between 0 and range.
/// </summary>
/// <param name="range"> maximum number </param>
/// <returns></returns>
unsigned bounded_rand(int range) {
	for (unsigned x, r;;)
		if (x = rand(), r = x % range, x - r <= -range)
			return r;
}

/// <summary>
/// This method generates a random speed array.
/// </summary>
/// <param name="numOfWaves"> number of waves </param>
/// <param name="speedArr"> the speed array to be filled </param>
void createRandomSpeeds(int numOfWaves, float* speedArr) {
	if (numOfWaves < 1) {
		cerr << "Error: number of waves must be at least 1." << endl;
		return;
	}
	for (int i = 0; i < numOfWaves; i++) {
		speedArr[i] = bounded_rand(10);
	}
}

/// <summary>
/// This method generates a random direction vector array.
/// </summary>
/// <param name="numOfWaves"> the number of waves to be generated </param>
/// <param name="directionArr"> the array pointer to the direction array </param>
void createRandomDirections( int numOfWaves, cyVec2f* directionArr) {
	if (numOfWaves < 1) {
		cerr << "Error: number of waves must be at least 1." << endl;
		return;
	}
	for (int i = 0; i < numOfWaves; i++) {
		float x = static_cast<float>(rand()) / RAND_MAX * 2.0 - 1.0;
		float y = static_cast<float>(rand()) / RAND_MAX * 2.0 - 1.0;
		directionArr[i] = cy::Vec2f(x, y).GetNormalized();
	}
}

/// <summary>
/// This method handles the uniform setter for the quad program.
/// </summary>
/// <param name="progName"> program used </param>
/// <param name="modelMatrix"> model matrix </param>
/// <param name="viewMatrix"> view matrix </param>
/// <param name="projectionMatrix"> projection matrix </param>
/// <param name="lightDirWorld"> light direction in world space </param>
/// <param name="eye"> camera position </param>
void handleQuadMVPProgUniforms(cy::GLSLProgram& progName, cyMatrix4f modelMatrix, cyMatrix4f viewMatrix, cyMatrix4f projectionMatrix, cyVec3f lightDirWorld, cyVec3f eye) {
	progName["modelMat"] = modelMatrix;
	progName["viewMat"] = viewMatrix;
	progName["projectionMat"] = projectionMatrix;
	progName["lightDir"] = lightDirWorld;
	progName["lightColor"] = cy::Vec3f(1.0f, 1.0f, 1.0f);
	progName["cameraVec"] = eye;
	progName["constShininess"] = 256.0f;
	progName["constLightIntensity"] = 1.0f;
	progName["constAmbientLight"] = 0.5f;
}

/// <summary>
/// This method handles the uniform setter for the area light program.
/// </summary>
/// <param name="progName"> program used </param>
void handleAreaLightProgUniforms(cy::GLSLProgram& progName) {
	// area light setup for main program
	progName.SetUniform("areaLightVerts", areaLightUniqueVert, 24);
	if (isTexturedLight) {
		progName.SetUniform("areaLightTexCorners", areaLightTexCorners, 4);
		progName.SetUniform("areaLight4Corners", areaLight4Corners, 4);
	}

	ltc1.Bind(1);
	progName["ltc1"] = 1;
	ltc2.Bind(2);
	progName["ltc2"] = 2;

	if (isTexturedLight) {
		AL_Tex.Bind(0);
		prog["areaLightTex"] = 0;
	}
}

/// <summary>
/// This method handles the uniform setter for the wave parameters.
/// </summary>
/// <param name="progName"> the program used </param>
void waveUniformUpdate(cy::GLSLProgram& progName) {
	progName["numOfWaves"] = numOfWaves;
	progName.SetUniform("waveDirection", waveDirection, numOfWaves);
	progName.SetUniform1("waveAmplitude", waveAmplitude, numOfWaves);
	progName.SetUniform1("waveFrequency", waveFrequency, numOfWaves);
	progName.SetUniform1("waveSpeed", waveSpeed, numOfWaves);
}

/// <summary>
/// This method handles the uniform setter for tessellation and radius.
/// </summary>
void updateTessAndRadiusUniforms() {
	if (isTexturedLight) {
		prog["tessLevel"] = tessLevel;
		prog["innerRadius"] = innerRadius;
		prog["outerRadius"] = outerRadius;
	}
	else {
		altProg["tessLevel"] = tessLevel;
		altProg["innerRadius"] = innerRadius;
		altProg["outerRadius"] = outerRadius;
	}
	triangleLineProg["tessLevel"] = tessLevel;
	triangleLineProg["innerRadius"] = innerRadius;
	triangleLineProg["outerRadius"] = outerRadius;
}

/// <summary>
/// This method handles the uniform setter for area light MVPs.
/// </summary>
/// <param name="model"> model matrix </param>
/// <param name="view"> view matrix </param>
/// <param name="projection"> projection matrix </param>
void areaLightMatrix(cy::Matrix4f model, cy::Matrix4f view, cy::Matrix4f projection) {
	areaLightProg["modelMat"] = model;
	areaLightProg["viewMat"] = view;
	areaLightProg["projectionMat"] = projection;
}

/// <summary>
/// This method handles the uniform setter for cubemap MVPs.
/// </summary>
/// <param name="view"> view matrix </param>
/// <param name="projection"> projection matrix </param>
void cubemapMatrix(cy::Matrix4f view, cy::Matrix4f projection) {

	cy::Matrix4f viewMatrix = cy::Matrix4f(view.GetSubMatrix3()); // remove the transltion of the matrix.

	cubeProg["viewMat"] = viewMatrix;
	cubeProg["projMat"] = projection;
}

/// <summary>
/// This method handles the uniform setter for triangleLineProg MVPs.
/// </summary>
/// <param name="model"> the model matrix to use </param>
/// <param name="view"> the view matrix to use </param>
/// <param name="projection"> the projection matrix to use </param>
void lineMVP(cy::Matrix4f model, cy::Matrix4f view, cy::Matrix4f projection) {
	triangleLineProg["modelMat"] = model;
	triangleLineProg["viewMat"] = view;
	triangleLineProg["projectionMat"] = projection;
	triangleLineProg["lineOffset"] = cy::Vec4f(0.0f, 0.0f, -0.1f, 0.0f);
}

/// <summary>
/// This method handles the MVP for the quad.
/// </summary>
void quadMVP() {
	// Model transformation
	cy::Matrix4f modelMatrix;
	modelMatrix.SetIdentity();
	
	cy:cyVec3f eye = camPosition;
	cy::Vec3f center = camPosition + frontVector;
	cy::Vec3f up = rightVector.Cross(frontVector).GetNormalized();
	cy::Matrix4f viewMatrix = cy::Matrix4f::View(eye, center, up);

	// Projection transformation
	float fov = 40.0f;										// Field of view in degrees
	float aspectRatio = ((float)windowWidth) / windowHeight;		// Aspect ratio (width/height)
	float nearClip = 0.1f;									// Near clipping plane
	float farClip = 1000.0f;								// Far clipping plane
	cy::Matrix4f projectionMatrix = cy::Matrix4f::Perspective(deg2rad(fov), aspectRatio, nearClip, farClip);

	cy::Vec3f lightDirWorld = cy::Vec3f(0.0f, 0.0f, -1.0f).GetNormalized();

	if (isTexturedLight) {
		handleQuadMVPProgUniforms(prog, modelMatrix, viewMatrix, projectionMatrix, lightDirWorld, eye);
	}
	else {
		handleQuadMVPProgUniforms(altProg, modelMatrix, viewMatrix, projectionMatrix, lightDirWorld, eye);
	}

	lineMVP(modelMatrix, viewMatrix, projectionMatrix);
	triangleLineProg["cameraVec"] = eye;

	cubemapMatrix(viewMatrix, projectionMatrix);

	areaLightMatrix(modelMatrix, viewMatrix, projectionMatrix);
}

/// <summary>
/// Helper method to draw the cubemap.
/// </summary>
void drawCubemap() {
	// draw cubemap
	glDepthMask(GL_FALSE);
	cubeProg.Bind();
	envMap.Bind(0);
	cubeProg["env"] = 0;

	// use glDrawArrays here to draw the environment
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, cubeNumVert);
	glDepthMask(GL_TRUE);
}

/// <summary>
/// Helper method to draw the triangulation.
/// </summary>
void drawTriangulation() {
	triangleLineProg.Bind();
	glDrawArrays(GL_PATCHES, 0, totalNumVert);
}

/// <summary>
/// Helper method to draw the quad when tessellation = true.
/// </summary>
void drawWaterQuad() {

	glPatchParameteri(GL_PATCH_VERTICES, 3);
	glBindVertexArray(waterVAO);
	if (isTexturedLight) {
		prog.Bind();
		prog["env"] = 0;
		
	}
	else {
		altProg.Bind();
		altProg["env"] = 0;
	}
	glDrawArrays(GL_PATCHES, 0, totalNumVert);
}

/// <summary>
/// This method draws the area light.
/// </summary>
void drawAreaLight() {
	glBindVertexArray(areaLightVAO);
	areaLightProg.Bind();
	glDrawArrays(GL_TRIANGLES, 0, areaLightNumVert);
}

/// <summary>
/// This method calculates the camera vector.
/// </summary>
void cameraVectors() {
	float thetaRad = deg2rad(theta);
	float phiRad = deg2rad(phi);
	frontVector = cy::Vec3f(
		cos(thetaRad) * cos(phiRad),
		sin(phiRad),
		sin(thetaRad) * cos(phiRad)
	).GetNormalized();
	rightVector = frontVector.Cross(cy::Vec3f(0, 1, 0)).GetNormalized();
}

/// <summary>
/// This method handles the camera movement.
/// </summary>
/// <param name="deltaTime"> time passed </param>
void cameraMovement(float deltaTime) {
	float moveSpeed = 10.0f * deltaTime; // Speed of camera movement

	// Handle camera movement
	if (wPressed) {
		camPosition += frontVector * moveSpeed;
	}
	if (sPressed) {
		camPosition -= frontVector * moveSpeed;
	}
	if (aPressed) {
		camPosition -= rightVector * moveSpeed;
	}
	if (dPressed) {
		camPosition += rightVector * moveSpeed;
	}
	if (spacePressed) {
		camPosition += upVector * moveSpeed;
	}
	if (shiftPressed) {
		camPosition -= upVector * moveSpeed;
	}
}

/// <summary>
/// This method handles the time calculations.
/// </summary>
/// <returns> time passed </returns>
float timeCalculations() {
	static int prevTime = glutGet(GLUT_ELAPSED_TIME);
	int currentTime = glutGet(GLUT_ELAPSED_TIME);
	float deltaTime = (currentTime - prevTime) / 1000.0f;
	prevTime = currentTime;
	timePassed += deltaTime;  // Accumulate time

	cameraMovement(deltaTime);

	return timePassed;
}

/// <summary>
/// Handles the display callback for rendering.
/// </summary>
void handleDisplay() {
	quadMVP();

	float time = timeCalculations();
	if (isTexturedLight) {
		prog["time"] = time;
	}
	else {
		altProg["time"] = time;
	}
	triangleLineProg["time"] = time;

	// Clear the viewport
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.4, 0.7, 0.8, 1);	// background color

	AL_Tex.Bind(0);
	if (isTexturedLight){
		prog["areaLightTex"] = 0;
	}
	areaLightProg["areaLightTex"] = 0;

	if (isTexturedLight) {
		areaLightProg["useTexture"] = 1;
	}
	else {
		areaLightProg["useTexture"] = 0;
	}

	drawWaterQuad();
	// show triangulation
	if (showTriangulation) {
		drawTriangulation();
	}
	drawAreaLight();
	drawCubemap();

	// Swap buffers
	glutSwapBuffers();
	glutPostRedisplay();
}

/// <summary>
/// Handles the keyboard input events.
/// </summary>
/// <param name="key"> the key pressed </param>
/// <param name="x"> x coordinate of mouse cursor when key is pressed </param>
/// <param name="y"> x coordinate of mouse cursor when key is pressed </param>
void keyboardInput(unsigned char key, int x, int y) {
	// Handle keyboard input
	switch (key) {
	case 27: // esc to exit
		glutLeaveMainLoop();
		break;
	case 't': case 'T': // t or T
		// show triangulation
		showTriangulation = !showTriangulation;
		glutPostRedisplay();
		break;
	case 'w': case 'W':
		// move front
		wPressed = true;
		break;
	case 'a': case 'A':
		// move left
		aPressed = true;
		break;
	case 's': case 'S':
		// move back
		sPressed = true;
		break;
	case 'd': case 'D':
		// move right
		dPressed = true;
		break;
	case 32: // space
		// move up
		spacePressed = true;
		break;
	case 'q': case 'Q':
		// textured area lights
		isTexturedLight = !isTexturedLight;
		if (isTexturedLight) {
			prog.Bind();
			handleAreaLightProgUniforms(prog);
			waveUniformUpdate(prog);
		}
		else {
			altProg.Bind();
			handleAreaLightProgUniforms(altProg);
			waveUniformUpdate(altProg);
		}
		quadMVP();
		updateTessAndRadiusUniforms();
		glutPostRedisplay();
		break;
	case 'e': case 'E':
		// directional light
		isDirectionalLight = !isDirectionalLight;
		if (isDirectionalLight) {
			prog["isDirectionalLight"] = 1;
			altProg["isDirectionalLight"] = 1;
			cubeProg["isDirectionalLight"] = 1;
		}
		else {
			prog["isDirectionalLight"] = 0;
			altProg["isDirectionalLight"] = 0;
			cubeProg["isDirectionalLight"] = 0;
		}
		quadMVP();
		glutPostRedisplay();
		break;
	}
}

/// <summary>
/// This method handles the keyboard up events.
/// </summary>
/// <param name="key"> the key let go </param>
/// <param name="x"> the x pos of cursor </param>
/// <param name="y"> the y pos of cursor </param>
void keyboardUp(unsigned char key, int x, int y) {
	switch (key) {
	case 'w': case 'W': 
		wPressed = false; 
		break;
	case 'a': case 'A': 
		aPressed = false; 
		break;
	case 's': case 'S': 
		sPressed = false;
		break;
	case 'd': case 'D': 
		dPressed = false; 
		break;
	case 32: 
		spacePressed = false; 
		break;
	}
}

/// <summary>
/// Handles special key input.
/// </summary>
/// <param name="key"> the key pressed </param>
/// <param name="x"> x coordinate of mouse cursor when key is pressed </param>
/// <param name="y"> x coordinate of mouse cursor when key is pressed </param>
void specialKeyInput(int key, int x, int y) {
	if (key == GLUT_KEY_LEFT) {
		tessLevel--;
		if (tessLevel < 1)
			tessLevel = 1;
	}
	else if (key == GLUT_KEY_RIGHT) {
		tessLevel++;
	}

	if (key == GLUT_KEY_UP) { // increase inner and outer radius
		// increase inner radius
		innerRadius += 1.0f;
		outerRadius += 1.0f;
	}
	else if (key == GLUT_KEY_DOWN) { // decrease inner and outer radius
		// decrease inner radius
		if (innerRadius > 1.0f) {
			innerRadius -= 1.0f;
			outerRadius -= 1.0f;
		}
		if (innerRadius < 1.0f) {
			innerRadius = 1.0f;
			outerRadius = 20.0f;
		}
	}

	if (key == GLUT_KEY_SHIFT_L || key == GLUT_KEY_SHIFT_R) {
		shiftPressed = true;
	}

	if (key == GLUT_KEY_F1) {
		// toggle fullscreen
		glutFullScreenToggle();
	}

	updateTessAndRadiusUniforms();
	glutPostRedisplay();
}

/// <summary>
/// This method handles the special key up events.
/// </summary>
/// <param name="key"> the key pressed </param>
/// <param name="x"> the x position of cursor </param>
/// <param name="y"> the y position of cursor </param>
void specialKeyUp(int key, int x, int y) {
	if (key == GLUT_KEY_SHIFT_L || key == GLUT_KEY_SHIFT_R) {
		shiftPressed = false;
	}
}

/// <summary>
/// Handles mouse button events.
/// </summary>
/// <param name="button"> the mouse button pressed </param>
/// <param name="state"> the state of the button (down if pressed, up otherwise) </param>
/// <param name="x"> the x coordinate of the mouse when the button is pressed </param>
/// <param name="y"> the y coordinate of the mouse when the button is pressed </param>
void mouseButton(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		isLeftButton = true;
	}
	else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
		isLeftButton = false;
	}

	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
		isRightButton = true;
	}
	else if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP) {
		isRightButton = false;
	}

	// first click
	lastMouseX = x;
	lastMouseY = y;
}

/// <summary>
/// Handles mouse motion events when mouse button is clicked, hold, and dragged.
/// </summary>
/// <param name="x"> the current x coordinate of the mouse </param>
/// <param name="y"> the current y coordinate of the mouse </param>
void mouseMotion(int x, int y) {
	int dx = x - lastMouseX;
	int dy = y - lastMouseY;
	lastMouseX = x;
	lastMouseY = y;

	if (isLeftButton) {
		theta += dx * 0.5f;
		phi -= dy * 0.5f;

		// Bound phi to prevent discontinuity (cost: does not have full spherical angle)
		if (phi > 89.0f)
			phi = 89.0f;
		else if (phi < -89.0f)
			phi = -89.0f;

		// adjusting front vector
		cameraVectors();
	}
	else if (isRightButton) {
		camPosition += frontVector * dy * 0.1f;
	}

	// modify MVP on every mouse motion
	quadMVP();

	glutPostRedisplay();
}

/// <summary>
/// This function registers any callbacks for rendering and input.
/// </summary>
void registerCallbacks() {
	glutDisplayFunc(handleDisplay);
	glutKeyboardFunc(keyboardInput);
	glutKeyboardUpFunc(keyboardUp);
	glutMouseFunc(mouseButton);
	glutMotionFunc(mouseMotion);
	glutSpecialFunc(specialKeyInput);
	glutSpecialUpFunc(specialKeyUp);
}

/// <summary>
/// source: https://raw.githubusercontent.com/lvandeve/lodepng/master/examples/example_decode.cpp
/// Decode from disk to raw pixels with a single function call.
/// </summary>
/// <param name="filename"> the file to decode </param>
/// <param name="imageArray"> the destination array </param>
/// <param name="width"> the destination width </param>
/// <param name="height"> the destination height </param>
void decodeOneStep(const char* filename, vector<unsigned char>& imageArray, unsigned& width, unsigned& height) {

	//decode
	unsigned error = lodepng::decode(imageArray, width, height, filename);

	//if there's an error, display it
	if (error) std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;

	//the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
}

/// <summary>
/// This method initializes cube mapping.
/// </summary>
void cubeMapping() {

	envMap.Initialize();

	for (int i = 0; i < 6; i++) {
		// load image from file
		unsigned sideWidth;
		unsigned sideHeight;
		std::vector<unsigned char> sideImage;

		string image_path = sideCubes[i]; // where sideCubes is defined above

		// set file name below
		decodeOneStep(image_path.c_str(), sideImage, sideWidth, sideHeight);

		// Check if the image was successfully loaded
		if (sideImage.empty()) {
			cerr << "Error: Failed to load cube map texture: " << image_path << endl;
			return;
		}

		// set image data
		envMap.SetImageRGBA((cy::GLTextureCubeMap::Side)i, sideImage.data(), sideWidth, sideHeight);
	}

	envMap.BuildMipmaps();
	envMap.SetSeamless();
}

/// <summary>
/// This method prepares the vertices of the skybox.
/// </summary>
void cubeSetup() {
	cy::TriMesh cubeMesh;
	bool cubeSuccess = cubeMesh.LoadFromFileObj("cube.obj", true);

	if (!cubeSuccess) {
		cerr << "Error: Failed to load cube.obj file!" << endl;
		return;
	}

	int cubeFaces = cubeMesh.NF();
	cubeNumVert = cubeFaces * 3;

	// Ensure cubeVertices is allocated with the correct size
	delete[] cubeVertices;
	cubeVertices = new cy::Vec3f[cubeNumVert];

	int vertOffSet = 0;
	for (int i = 0; i < cubeFaces; i++) {
		// Faces for cubeVertices

		cy::TriMesh::TriFace faces = cubeMesh.F(i);

		int vertInd1 = faces.v[0];
		int vertInd2 = faces.v[1];
		int vertInd3 = faces.v[2];

		if (vertOffSet < cubeNumVert) {
			cubeVertices[vertOffSet] = cubeMesh.V(vertInd1);
			cubeVertices[vertOffSet + 1] = cubeMesh.V(vertInd2);
			cubeVertices[vertOffSet + 2] = cubeMesh.V(vertInd3);
		}

		vertOffSet += 3;
	}
}

/// <summary>
/// Helper method to handle loading the obj file.
/// Create arrays containing all the necessity datas.
/// </summary>
void loadObjFileSetup(cyTriMesh &mesh, cyVec3f* &vertices, cyVec2f* &textures, int &numVert) {
    int numFaces = mesh.NF();    // get the number of faces
    numVert = numFaces * 3;

    if (vertices != nullptr) 
        delete[] vertices; // Prevent memory leaks
    vertices = new cyVec3f[numVert];

    if (textures != nullptr) 
        delete[] textures; // Prevent memory leaks
    textures = new cyVec2f[numVert];

    int vertOffSet = 0;

    for (int i = 0; i < numFaces; i++) {
        // Faces for vertices
        cy::TriMesh::TriFace faces = mesh.F(i);

        int vertInd1 = faces.v[0];
        int vertInd2 = faces.v[1];
        int vertInd3 = faces.v[2];

        if (vertOffSet < numVert) {
            vertices[vertOffSet] = mesh.V(vertInd1);
            vertices[vertOffSet + 1] = mesh.V(vertInd2);
            vertices[vertOffSet + 2] = mesh.V(vertInd3);
        }

        // textures
        cy::TriMesh::TriFace texFace = mesh.FT(i);
        int texInd1 = texFace.v[0];
        int texInd2 = texFace.v[1];
        int texInd3 = texFace.v[2];

        if (vertOffSet < numVert) {
            textures[vertOffSet] = mesh.VT(texInd1).XY();
            textures[vertOffSet + 1] = mesh.VT(texInd2).XY();
            textures[vertOffSet + 2] = mesh.VT(texInd3).XY();
        }

        vertOffSet += 3;
    }
}

/// <summary>
/// This method generates the VAO and VBO for the skybox.
/// </summary>
void cubeVaoVbo() {

	// VAO for the cube
	glGenVertexArrays(1, &cubeVAO);
	glBindVertexArray(cubeVAO);

	// VBO for the cube
	GLuint cubeBuffer;
	glGenBuffers(1, &cubeBuffer);

	// Bind and upload position data
	glBindBuffer(GL_ARRAY_BUFFER, cubeBuffer);
	glBufferData(GL_ARRAY_BUFFER, cubeNumVert * sizeof(cy::Vec3f), cubeVertices, GL_STATIC_DRAW);

	// Set up position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
}

/// <summary>
/// This method prepares the Vao and Vbo for the water quad plane.
/// </summary>
void waterQuadVAOVBOfromOBJ() {
	GLuint waterBuffer, waterTextureBuffer;
	glGenVertexArrays(1, &waterVAO);
	glBindVertexArray(waterVAO);

	glGenBuffers(1, &waterBuffer);
	glGenBuffers(1, &waterTextureBuffer);

	glBindBuffer(GL_ARRAY_BUFFER, waterBuffer);
	glBufferData(GL_ARRAY_BUFFER, totalNumVert * sizeof(cy::Vec3f), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, waterTextureBuffer);
	glBufferData(GL_ARRAY_BUFFER, totalNumVert * sizeof(cy::Vec2f), textures, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0);
	glEnableVertexAttribArray(1);
}

/// <summary>
/// This method prepares the Vao and Vbo for the area light.
/// </summary>
void areaLightVAOVBOfromOBJ() {
	GLuint areaLightBuffer, areaLightTexBuffer;
	glGenVertexArrays(1, &areaLightVAO);
	glBindVertexArray(areaLightVAO);

	glGenBuffers(1, &areaLightBuffer);
	glGenBuffers(1, &areaLightTexBuffer);

	glBindBuffer(GL_ARRAY_BUFFER, areaLightBuffer);
	glBufferData(GL_ARRAY_BUFFER, areaLightNumVert * sizeof(cy::Vec3f), areaLightVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, areaLightTexBuffer);
	glBufferData(GL_ARRAY_BUFFER, areaLightNumVert * sizeof(cy::Vec2f), areaLightTextures, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0);
	glEnableVertexAttribArray(1);
}

/// <summary>
/// This method sets up the area light vertices for sending it to tessShader.
/// </summary>
void areaLightUniqueVerts() {
	areaLightUniqueVert = new cy::Vec3f[24]; // 6 area lights, 4 vertices each
	areaLightTexCorners = new cy::Vec2f[4]; // 4 corners of the area light texture
	areaLight4Corners = new cy::Vec3f[4]; // 4 corners of the area light texture
	
	int vertOffSet = 0;
	for (int i = 0; i < 24; i += 4) {
		areaLightUniqueVert[i] = areaLightVertices[vertOffSet];			// bottom left
		areaLightUniqueVert[i + 1] = areaLightVertices[vertOffSet + 1];	// bottom right
		areaLightUniqueVert[i + 2] = areaLightVertices[vertOffSet + 2]; // top right
		areaLightUniqueVert[i + 3] = areaLightVertices[vertOffSet + 5]; // top left

		// for the texture coordinates, want to find the offset (using min) and scale.
		cyVec2f uv0 = areaLightTextures[vertOffSet];
		cyVec2f uv2 = areaLightTextures[vertOffSet + 2];

		if (vertOffSet == 0) {
			areaLightTexCorners[0] = uv0; // bottom left corner
			areaLightTexCorners[1] = areaLightTextures[vertOffSet+1]; // bottom right corner
			areaLightTexCorners[2] = uv2; // top right corner
			areaLightTexCorners[3] = areaLightTextures[vertOffSet+5]; // top left corner

			areaLight4Corners[0] = areaLightUniqueVert[i]; // bottom left corner
			areaLight4Corners[1] = areaLightUniqueVert[i + 1]; // bottom right corner
			areaLight4Corners[2] = areaLightUniqueVert[i + 2]; // top right corner
			areaLight4Corners[3] = areaLightUniqueVert[i + 3]; // top left corner
		}

		else {
			cyVec2f newBotLeft = areaLightTextures[vertOffSet];
			cyVec2f currBotLeft = areaLightTexCorners[0];
			if (newBotLeft.x <= currBotLeft.x && newBotLeft.y <= currBotLeft.y) {
				areaLightTexCorners[0] = newBotLeft;
				areaLight4Corners[0] = areaLightUniqueVert[i]; // bottom left corner
			}

			cyVec2f newBotRight = areaLightTextures[vertOffSet + 1];
			cyVec2f currBotRight = areaLightTexCorners[1];
			if (newBotRight.x >= currBotRight.x && newBotRight.y <= currBotRight.y) {
				areaLightTexCorners[1] = newBotRight;
				areaLight4Corners[1] = areaLightUniqueVert[i+1];
			}

			cyVec2f newTopRight = areaLightTextures[vertOffSet + 2];
			cyVec2f currTopRight = areaLightTexCorners[2];
			if (newTopRight.x >= currTopRight.x && newTopRight.y >= currTopRight.y) {
				areaLightTexCorners[2] = newTopRight;
				areaLight4Corners[2] = areaLightUniqueVert[i+2];
			}

			cyVec2f newTopLeft = areaLightTextures[vertOffSet + 5];
			cyVec2f currTopLeft = areaLightTexCorners[3];
			if (newTopLeft.x <= currTopLeft.x && newTopLeft.y >= currTopLeft.y) {
				areaLightTexCorners[3] = newTopLeft;
				areaLight4Corners[3] = areaLightUniqueVert[i+3];
			}
		}

		vertOffSet += 6; // each area light was made from 6 vertices
	}
}

/// <summary>
/// Set up the texture for the Minv using precomputed values.
/// 
/// ltc1: inverted transformation matrices.
/// ltc2: fresnel90, horizon clipping factor, smith coefficient for geometric attenuation.
/// </summary>
cy::GLTexture2D loadMinvTexture(const float* matrixTable) {
	cy::GLTexture2D texture;
	texture.Initialize();
	texture.SetImage(matrixTable, 4, 64, 64); // 4 channels, 64x64 size
	texture.SetWrappingMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	texture.SetFilteringMode(GL_LINEAR, GL_NEAREST);
	return texture;
}

/// <summary>
/// This method sets up the wave parameters.
/// </summary>
void waveSetup() {
	// wave parameters
	createScaledArray(numOfWaves, 0.5f, waveAmplitude);
	createScaledArray(numOfWaves, 1.3f, waveFrequency);
	createRandomDirections(numOfWaves, waveDirection);
	createRandomSpeeds(numOfWaves, waveSpeed);

	if (isTexturedLight) {
		waveUniformUpdate(prog);
	}
	else {
		waveUniformUpdate(altProg);
	}

	triangleLineProg["numOfWaves"] = numOfWaves;
	triangleLineProg.SetUniform("waveDirection", waveDirection, numOfWaves);
	triangleLineProg.SetUniform1("waveAmplitude", waveAmplitude, numOfWaves);
	triangleLineProg.SetUniform1("waveFrequency", waveFrequency, numOfWaves);
	triangleLineProg.SetUniform1("waveSpeed", waveSpeed, numOfWaves);
}

/// <summary>
/// The main function to initialize GLUT, set up the window and OpenGL settings.
/// This enters the GLUT main loop and starts rendering.
/// </summary>
/// <param name="argc"> the number of command line arguments </param>
/// <param name="argv"> an array of command line arguments </param>
/// <returns> returns 0 on success </returns>
int main(int argc, char* argv[]) {

	//// initializes GLUT and OpenGL
	glutInit(&argc, argv);
	glutInitContextFlags(GLUT_DEBUG);
	glutInitWindowSize(windowWidth, windowHeight);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutCreateWindow("CS5610 - Final Project");
	glViewport(0, 0, windowWidth, windowHeight);

	//OpenGL initialization
	GLenum err = glewInit();
	if (GLEW_OK != err) {
		fprintf(stderr, "GLEW error");
		return 1;
	}

	CY_GL_REGISTER_DEBUG_CALLBACK;		// generate interrupts when there's an error
	glEnable(GL_DEPTH_TEST);			// Enable depth testing
	glDepthFunc(GL_LEQUAL);				// Set depth function to "less than or equal"
	////

	//// obj file loading
	const char* objFilePath = argv[1];
	bool success = mesh.LoadFromFileObj(objFilePath, true);
	loadObjFileSetup(mesh, vertices, textures, totalNumVert);
	waterQuadVAOVBOfromOBJ();

	// area lights obj file load
	const char* areaLightObjFilePath = argv[2];
	bool areaLightSuccess = areaLightMesh.LoadFromFileObj(areaLightObjFilePath, true);
	loadObjFileSetup(areaLightMesh, areaLightVertices, areaLightTextures, areaLightNumVert);
	areaLightUniqueVerts(); // get the unique vertices for the area lights (for each 6 vertices, take the first three and last vertices)
	areaLightVAOVBOfromOBJ();

	// area light textures
	auto areaLightTexFileName = argv[3];
	std::vector<unsigned char> areaLightTexData;
	unsigned areaLightTexWidth, areaLightTexHeight;
	decodeOneStep(areaLightTexFileName, areaLightTexData, areaLightTexWidth, areaLightTexHeight);

	AL_Tex.Initialize();
	AL_Tex.SetImage(areaLightTexData.data(), 4, areaLightTexWidth, areaLightTexHeight);
	AL_Tex.BuildMipmaps();
	AL_Tex.SetWrappingMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

	////
	// cube (texture) mapping setup
	cubeMapping();
	// cube vertices setup
	cubeSetup();
	// cube vba and vbo setup
	cubeVaoVbo();

	// shader program setup
	prog.BuildFiles("tessShader.vert", "tessShader.frag", nullptr, "tessShader.tesc", "tessShader.tese");
	altProg.BuildFiles("tessShader.vert", "altTessShader.frag", nullptr, "tessShader.tesc", "tessShader.tese");
	triangleLineProg.BuildFiles("tessShader.vert", "triangleLine.frag", "triangleLine.geom", "tessShader.tesc", "tessShader.tese");
	cubeProg.BuildFiles("envcube.vert", "envcube.frag");
	areaLightProg.BuildFiles("areaLight.vert", "areaLight.frag");
	
	cameraVectors();
	quadMVP(); // the line, cubemap, and arealight MVP is all here.
	updateTessAndRadiusUniforms();
	waveSetup();
	ltc1 = loadMinvTexture(LTC1);
	ltc2 = loadMinvTexture(LTC2);
	prog["isDirectionalLight"] = 1;
	altProg["isDirectionalLight"] = 1;
	cubeProg["isDirectionalLight"] = 1;
	if (isTexturedLight) {
		handleAreaLightProgUniforms(prog);
	}
	else {
		handleAreaLightProgUniforms(altProg);
	}

	// Register callbacks
	registerCallbacks();

	// Enter the GLUT main loop
	glutMainLoop();
	return 0;
}