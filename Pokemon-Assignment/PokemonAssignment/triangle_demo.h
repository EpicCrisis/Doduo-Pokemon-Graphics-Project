#ifndef _TRIANGLE_DEMO_H
#define _TRIANGLE_DEMO_H

#include <cmath>
#include <vector>
#include <iostream>
#include "demo_base.h"
#include "lodepng.h"
#include <fmod.hpp>
#include <fmod_errors.h>

#define TEXTURE_COUNT 6

//must be power of 2 number
#define SPECTRUM_SIZE 128

const int RECT_VERTEX_ARRAY_SIZE = 0;

void ERRCHECK(FMOD_RESULT result)
{
	if (result != FMOD_OK)
	{
		printf("FMOD ERROR! (%d) %s\n", result, FMOD_ErrorString(result));
	}
}

class Plane
{
private:
	float delta = 0.0f;

	float getY(float j)
	{
		return amplitude * sinf(delta + (j * frequency));
	}

public:

	float speed;
	float amplitude;
	float frequency;
	float size;
	int iteration;

	Plane(float speed, float amplitude, float frequency, float size, int iterationAmount = 10)
	{
		this->speed = speed;
		this->amplitude = amplitude;
		this->frequency = frequency;
		this->size = size;
		this->iteration = iterationAmount;
	}

	void drawPlane()
	{
		float halfSize = size / 2.0f;
		float increment = 1.0f / iteration;

		for (float i = -halfSize; i < halfSize - increment; i += increment)
		{
			for (float j = -halfSize; j < halfSize - increment; j += increment)
			{
				glBegin(GL_TRIANGLES);

				glVertex3f(j, getY(j), i);
				glVertex3f(j, getY(j), i + increment);
				glVertex3f(j + increment, getY(j + increment), i);

				glVertex3f(j + increment, getY(j + increment), i + increment);
				glVertex3f(j + increment, getY(j + increment), i);
				glVertex3f(j, getY(j), i + increment);

				glEnd();
			}
		}
	}

	void animatePlaneWave()
	{
		delta += speed;
		//std::cout << "Delta = " << delta << std::endl;
		drawPlane();
	}
};

class Vertex
{
public:
	float x;
	float y;
	float z;

	Vertex()
	{
		x = 0;
		y = 0;
		z = 0;
	}

	Vertex(float a, float b, float c)
	{
		x = a;
		y = b;
		z = c;
	}
};

Vertex SphereFunction(float u, float v, float radius)
{
	u = u * 3.142f / 180.0f;
	v = v * 3.142f / 180.0f;
	Vertex answer;
	answer.x = cos(u) * sin(v) * radius;
	answer.y = cos(v) * radius;
	answer.z = sin(u) * sin(v) * radius;
	return answer;
}

void drawSphere(float startU, float startV, float endU, float endV, float radius, float UResolution, float VResolution, float xFactor, float yFactor, float red = 1.0f, float green = 1.0f, float blue = 1.0f)
{
	float stepU = (endU - startU) / UResolution;
	float stepV = (endV - startV) / VResolution;

	glBegin(GL_TRIANGLES);
	glColor3f(red, green, blue);
	for (int i = 0; i < UResolution; i++)
	{
		for (int j = 0; j < VResolution; j++)
		{
			float u = i*stepU + startU;
			float v = j*stepV + startV;
			float un = (i + 1 == UResolution) ? endU : (i + 1)*stepU + startU;
			float vn = (j + 1 == VResolution) ? endV : (j + 1)*stepV + startV;

			Vertex p0 = SphereFunction(u, v, radius);
			Vertex p1 = SphereFunction(u, vn, radius);
			Vertex p2 = SphereFunction(un, v, radius);
			Vertex p3 = SphereFunction(un, vn, radius);

			glVertex3f(p0.x*xFactor, p0.y*yFactor, p0.z);
			glVertex3f(p2.x*xFactor, p2.y*yFactor, p2.z);
			glVertex3f(p1.x*xFactor, p1.y*yFactor, p1.z);
			glVertex3f(p3.x*xFactor, p3.y*yFactor, p3.z);
			glVertex3f(p1.x*xFactor, p1.y*yFactor, p1.z);
			glVertex3f(p2.x*xFactor, p2.y*yFactor, p2.z);
		}
	}
	glEnd();
}

class TriangleDemo : public DemoBase
{
private:
	GLuint mTextureID[TEXTURE_COUNT];

	void loadPNG(const char* path, GLuint textureID)
	{
		//Load file and decode image
		std::vector<unsigned char> image;
		unsigned width, height;
		unsigned error = lodepng::decode(image, width, height, path);

		//If there's an error, display it
		if (error != 0)
		{
			std::cout << "png load error : " << error << ": " << lodepng_error_text(error) << std::endl;
		}

		glBindTexture(GL_TEXTURE_2D, textureID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); //Apply texture wrapping along horizontal part
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); //Apply texture wrapping along vertical part

		//Bilinear filtering
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); //Near filtering for texture scaling
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); //Far filtering for texture scaling

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &image[0]);
	}

	FMOD::System* m_fmodSystem;
	FMOD::Sound* m_music;
	FMOD::Channel* m_musicChannel;

	float m_spectrumLeft[SPECTRUM_SIZE];
	float m_spectrumRight[SPECTRUM_SIZE];

	float spectrumAverage;

public:

	void InitFMOD()
	{
		FMOD_RESULT result;
		unsigned int version;

		result = FMOD::System_Create(&m_fmodSystem);

		result = m_fmodSystem->getVersion(&version);
		ERRCHECK(result);

		if (version < FMOD_VERSION)
		{
			printf("FMOD Error! You are using an old version of FMOD.", version, FMOD_VERSION);
		}

		//initialize fmod system
		result = m_fmodSystem->init(32, FMOD_INIT_NORMAL, 0);
		ERRCHECK(result);

		//load and set up music
		result = m_fmodSystem->createStream("../media/", FMOD_SOFTWARE, 0, &m_music);
		ERRCHECK(result);

		//play the loaded mp3 music
		result = m_fmodSystem->playSound(FMOD_CHANNEL_FREE, m_music, false, &m_musicChannel);
		ERRCHECK(result);
	}

	float PI = 3.142;

	void init()
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA);
		glEnable(GL_ALPHA_TEST);

		glGenTextures(TEXTURE_COUNT, mTextureID);
		loadPNG("../media/unity_logo.png", mTextureID[0]);
		loadPNG("../media/angry_birds.png", mTextureID[1]);
	}

	void deinit()
	{

	}

	void drawAxis(const Matrix& viewMatrix)
	{
		glLoadMatrixf((GLfloat*)viewMatrix.mVal);
		glBegin(GL_LINES);

		glColor3f(1.0f, 0.3f, 0.3f);
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(1.0f, 0.0f, 0.0f);

		glColor3f(0.3f, 1.0f, 0.3f);
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(0.0f, 1.0f, 0.0f);

		glColor3f(0.3f, 0.3f, 1.0f);
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(0.0f, 0.0f, 1.0f);

		glEnd();
	}

	void drawTriangle(const Matrix& viewMatrix)
	{
		glLoadMatrixf((GLfloat*)viewMatrix.mVal);
		glBegin(GL_TRIANGLES);							// Drawing Using Triangles

		glColor3f(1.0f, 0.0f, 0.0f);
		glVertex3f(1.0f, 1.0f, 0.0f);					// Top Left
		glVertex3f(1.0f, -1.0f, 0.0f);					// Bottom Left
		glVertex3f(2.0f, -1.0f, 0.0f);					// Bottom Right

		glEnd();										// Finished Drawing The Triangles
	}

	void drawCube(float sizeX = 1.0f, float sizeY = 1.0f, float sizeZ = 1.0f, float red = 1.0f, float green = 1.0f, float blue = 1.0f)
	{
		sizeX /= 2.0f;
		sizeY /= 2.0f;
		sizeZ /= 2.0f;
		glBegin(GL_TRIANGLES);

		// Custom colour
		glColor3f(red, green, blue);

		// Negative Square
		glVertex3f(-sizeX, sizeY, -sizeZ);
		glVertex3f(-sizeX, -sizeY, -sizeZ);
		glVertex3f(sizeX, -sizeY, -sizeZ);

		glVertex3f(-sizeX, sizeY, -sizeZ);
		glVertex3f(sizeX, sizeY, -sizeZ);
		glVertex3f(sizeX, -sizeY, -sizeZ);

		// Positive Square
		glVertex3f(-sizeX, sizeY, sizeZ);
		glVertex3f(-sizeX, -sizeY, sizeZ);
		glVertex3f(sizeX, -sizeY, sizeZ);

		glVertex3f(-sizeX, sizeY, sizeZ);
		glVertex3f(sizeX, sizeY, sizeZ);
		glVertex3f(sizeX, -sizeY, sizeZ);

		// Green
		//glColor3f(0.25f, 1.0f, 0.25f);

		// Negative Square
		glVertex3f(sizeX, -sizeY, -sizeZ);
		glVertex3f(-sizeX, -sizeY, -sizeZ);
		glVertex3f(-sizeX, -sizeY, sizeZ);

		glVertex3f(sizeX, -sizeY, -sizeZ);
		glVertex3f(sizeX, -sizeY, sizeZ);
		glVertex3f(-sizeX, -sizeY, sizeZ);

		// Positive Square
		glVertex3f(sizeX, sizeY, -sizeZ);
		glVertex3f(-sizeX, sizeY, -sizeZ);
		glVertex3f(-sizeX, sizeY, sizeZ);

		glVertex3f(sizeX, sizeY, -sizeZ);
		glVertex3f(sizeX, sizeY, sizeZ);
		glVertex3f(-sizeX, sizeY, sizeZ);

		// Red
		//glColor3f(1.0f, 0.25f, 0.25f);

		// Negative Square
		glVertex3f(-sizeX, -sizeY, sizeZ);
		glVertex3f(-sizeX, -sizeY, -sizeZ);
		glVertex3f(-sizeX, sizeY, -sizeZ);

		glVertex3f(-sizeX, -sizeY, sizeZ);
		glVertex3f(-sizeX, sizeY, sizeZ);
		glVertex3f(-sizeX, sizeY, -sizeZ);

		// Positive Square
		glVertex3f(sizeX, -sizeY, sizeZ);
		glVertex3f(sizeX, -sizeY, -sizeZ);
		glVertex3f(sizeX, sizeY, -sizeZ);

		glVertex3f(sizeX, -sizeY, sizeZ);
		glVertex3f(sizeX, sizeY, sizeZ);
		glVertex3f(sizeX, sizeY, -sizeZ);

		// Finished Drawing The Triangles
		glEnd();
	}

	void drawTextureCube(float size = 1.0f)
	{
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, mTextureID[0]);

		glBegin(GL_TRIANGLES);

		// Negative Square (back)
		glTexCoord2f(1.0f, 0.0f), glVertex3f(-size, size, -size);
		glTexCoord2f(1.0f, 1.0f), glVertex3f(-size, -size, -size);
		glTexCoord2f(0.0f, 1.0f), glVertex3f(size, -size, -size);

		glTexCoord2f(1.0f, 0.0f), glVertex3f(-size, size, -size);
		glTexCoord2f(0.0f, 0.0f), glVertex3f(size, size, -size);
		glTexCoord2f(0.0f, 1.0f), glVertex3f(size, -size, -size);

		// Positive Square (front)
		glTexCoord2f(0.0f, 1.0f), glVertex3f(-size, size, size);
		glTexCoord2f(0.0f, 0.0f), glVertex3f(-size, -size, size);
		glTexCoord2f(1.0f, 0.0f), glVertex3f(size, -size, size);

		glTexCoord2f(0.0f, 1.0f), glVertex3f(-size, size, size);
		glTexCoord2f(1.0f, 1.0f), glVertex3f(size, size, size);
		glTexCoord2f(1.0f, 0.0f), glVertex3f(size, -size, size);

		// Negative Square (down)
		glTexCoord2f(1.0f, 0.0f), glVertex3f(size, -size, -size);
		glTexCoord2f(0.0f, 0.0f), glVertex3f(-size, -size, -size);
		glTexCoord2f(0.0f, 1.0f), glVertex3f(-size, -size, size);

		glTexCoord2f(1.0f, 0.0f), glVertex3f(size, -size, -size);
		glTexCoord2f(1.0f, 1.0f), glVertex3f(size, -size, size);
		glTexCoord2f(0.0f, 1.0f), glVertex3f(-size, -size, size);

		// Positive Square (up)
		glTexCoord2f(1.0f, 0.0f), glVertex3f(size, size, -size);
		glTexCoord2f(0.0f, 0.0f), glVertex3f(-size, size, -size);
		glTexCoord2f(0.0f, 1.0f), glVertex3f(-size, size, size);

		glTexCoord2f(1.0f, 0.0f), glVertex3f(size, size, -size);
		glTexCoord2f(1.0f, 1.0f), glVertex3f(size, size, size);
		glTexCoord2f(0.0f, 1.0f), glVertex3f(-size, size, size);

		// Negative Square (left)
		glTexCoord2f(1.0f, 0.0f), glVertex3f(-size, -size, size);
		glTexCoord2f(0.0f, 0.0f), glVertex3f(-size, -size, -size);
		glTexCoord2f(0.0f, 1.0f), glVertex3f(-size, size, -size);

		glTexCoord2f(1.0f, 0.0f), glVertex3f(-size, -size, size);
		glTexCoord2f(1.0f, 1.0f), glVertex3f(-size, size, size);
		glTexCoord2f(0.0f, 1.0f), glVertex3f(-size, size, -size);

		// Positive Square (right)
		glTexCoord2f(0.0f, 1.0f), glVertex3f(size, -size, size);
		glTexCoord2f(1.0f, 1.0f), glVertex3f(size, -size, -size);
		glTexCoord2f(1.0f, 0.0f), glVertex3f(size, size, -size);

		glTexCoord2f(0.0f, 1.0f), glVertex3f(size, -size, size);
		glTexCoord2f(0.0f, 0.0f), glVertex3f(size, size, size);
		glTexCoord2f(1.0f, 0.0f), glVertex3f(size, size, -size);

		glEnd();
	}

	void DrawWall(float size = 1.0f)
	{
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, mTextureID[0]);

		glBegin(GL_TRIANGLES);

		// Negative Square (back)
		glTexCoord2f(1.0f, 0.0f), glVertex3f(-size, size, -size);
		glTexCoord2f(1.0f, 1.0f), glVertex3f(-size, -size, -size);
		glTexCoord2f(0.0f, 1.0f), glVertex3f(size, -size, -size);

		glTexCoord2f(1.0f, 0.0f), glVertex3f(-size, size, -size);
		glTexCoord2f(0.0f, 0.0f), glVertex3f(size, size, -size);
		glTexCoord2f(0.0f, 1.0f), glVertex3f(size, -size, -size);

		glEnd();
	}

	void drawPyramid(float size = 1.0f)
	{
		size /= 2.0f;
		glBegin(GL_TRIANGLES);							// Drawing Using Triangles

														// Base
		glColor3f(0.25f, 1.0f, 0.25f);
		glVertex3f(size, -size, size);					// Top
		glVertex3f(size, -size, -size);					// Bottom Left
		glVertex3f(-size, -size, size);					// Bottom Right

		glVertex3f(-size, -size, -size);				// Top
		glVertex3f(size, -size, -size);					// Bottom Left
		glVertex3f(-size, -size, size);					// Bottom Right

														// Red Triangles
		glColor3f(1.0f, 0.25f, 0.25f);
		glVertex3f(0.0f, size, 0.0f);					// Top Vertex
		glVertex3f(size, -size, -size);					// Bottom Left
		glVertex3f(size, -size, size);					// Bottom Right

		glVertex3f(0.0f, size, 0.0f);					// Top Vertex
		glVertex3f(-size, -size, -size);				// Bottom Left
		glVertex3f(-size, -size, size);					// Bottom Right

														// Blue Triangles
		glColor3f(0.25f, 0.25f, 1.0f);
		glVertex3f(0.0f, size, 0.0f);					// Top Vertex
		glVertex3f(-size, -size, size);					// Bottom Left
		glVertex3f(size, -size, size);					// Bottom Right

		glVertex3f(0.0f, size, 0.0f);					// Top Vertex
		glVertex3f(-size, -size, -size);				// Bottom Left
		glVertex3f(size, -size, -size);					// Bottom Right

		glEnd();										// Finished Drawing The Triangles
	}

	void drawCylinder(float height, float radius, int iteration = 100, float red = 1.0f, float green = 1.0f, float blue = 1.0f) //Change iteration to steps
	{
		//Offset
		float offsetX = 0.0f;
		float offsetY = 0.0f;
		float offsetZ = 0.0f;
		float halfHeight = height / 2.0f;

		glColor3f(red, green, blue);
		// Drawing Using Triangles
		glBegin(GL_TRIANGLES);

		float t = 360.0f / iteration;

		for (int i = 0; i < iteration; i++)
		{
			float x1 = radius * cos((i * t) * PI / 180.0);
			float z1 = radius * sin((i * t) * PI / 180.0);

			float x2 = radius * cos(((i + 1) * t) * PI / 180.0);
			float z2 = radius * sin(((i + 1) * t) * PI / 180.0);

			// ---- Upper Circle ----
			//glColor3f(1.0f, (i % 2 == 0 ? 0.5f : 1.0f), 0.5f);
			//Center of the circle
			glVertex3f(offsetX, offsetY + halfHeight, offsetZ);

			//First point
			glVertex3f(x1, offsetY + halfHeight, z1);

			//Second point
			glVertex3f(x2, offsetY + halfHeight, z2);

			// ---- Lower Circle ----
			//glColor3f(1.0f, (i % 2 == 0 ? 0.5f : 1.0f), 0.5f);
			//Center of the circle
			glVertex3f(offsetX, offsetY - halfHeight, offsetZ);

			//First point
			glVertex3f(x1, offsetY - halfHeight, z1);

			//Second point
			glVertex3f(x2, offsetY - halfHeight, z2);

			// ---- Upper Triangle ----
			//glColor3f(0.5f, 1.0f, 0.5f);
			//First point
			glVertex3f(x1, offsetY + halfHeight, z1);

			//Second point
			glVertex3f(x2, offsetY + halfHeight, z2);

			//Third point
			glVertex3f(x1, offsetY - halfHeight, z1);

			// ---- Lower Triangle ----
			//glColor3f(0.5f, 0.5f, 1.0f);
			//First point
			glVertex3f(x1, offsetY - halfHeight, z1);

			//Second point
			glVertex3f(x2, offsetY - halfHeight, z2);

			//Third point
			glVertex3f(x2, offsetY + halfHeight, z2);
		}
		// Finished Drawing The Triangles
		glEnd();
	}

	void drawCone(float height, float radius, int iteration = 100, float red = 1.0f, float green = 1.0f, float blue = 1.0f)
	{
		//Offset
		float offsetX = 0.0f;
		float offsetY = 0.0f;
		float offsetZ = 0.0f;
		float halfHeight = height / 2.0f;

		glColor3f(red, green, blue);
		// Drawing Using Triangles
		glBegin(GL_TRIANGLES);

		float t = 360.0f / iteration;

		for (int i = 0; i < iteration; i++)
		{
			float x1 = radius * cos((i * t) * PI / 180.0);
			float z1 = radius * sin((i * t) * PI / 180.0);

			float x2 = radius * cos(((i + 1) * t) * PI / 180.0);
			float z2 = radius * sin(((i + 1) * t) * PI / 180.0);

			// ---- Circle ----
			//glColor3f(1.0f, (i % 2 == 0 ? 0.5f : 1.0f), 0.5f);
			//Center of the circle
			glVertex3f(offsetX, offsetY - halfHeight, offsetZ);

			//First point
			glVertex3f(x1, offsetY - halfHeight, z1);

			//Second point
			glVertex3f(x2, offsetY - halfHeight, z2);

			// ---- Triangle ----
			//glColor3f(0.5f, (i % 2 == 0 ? 0.5f : 1.0f), (i % 2 == 0 ? 1.0f : 0.5f));
			//Upper vertex
			glVertex3f(offsetX, offsetY + halfHeight, offsetZ);

			//First point
			glVertex3f(x1, offsetY - halfHeight, z1);

			//Second point
			glVertex3f(x2, offsetY - halfHeight, z2);
		}
		// Finished Drawing The Triangles
		glEnd();
	}

	void drawTrianglePyramid(float height, float width, float breadth, float red = 1.0f, float green = 1.0f, float blue = 1.0f)
	{
		glBegin(GL_TRIANGLES);

		// Bottom of the triangle pyramid. ---//
		glColor3f(red, green, blue);
		glVertex3f(-width, 0.0f, 0.0f);
		glVertex3f(width, 0.0f, 0.0f);
		glVertex3f(0.0f, 0.0f, breadth);

		// Left side. ---//
		//glColor3f(0.0f, 0.0f, 1.0f);
		glVertex3f(-width, 0.0f, 0.0f);
		glVertex3f(0.0f, 0.0f, breadth);
		glVertex3f(0.0f, height, 0.0f);

		// Right side. ---//
		//glColor3f(1.0f, 0.0f, 0.0f);
		glVertex3f(width, 0.0f, 0.0f);
		glVertex3f(0.0f, 0.0f, breadth);
		glVertex3f(0.0f, height, 0.0f);

		// Back side. ---//
		//glColor3f(0.0f, 1.0f, 0.0f);
		glVertex3f(width, 0.0f, 0.0f);
		glVertex3f(-width, 0.0f, 0.0f);
		glVertex3f(0.0f, height, 0.0f);

		glEnd();
	}

	void drawVertexCube(GLuint textureID, float size = 1.0f)
	{
		GLfloat vertices[] =
		{
			// Front //
			-size, -size, size,
			size, -size, size,
			size, size, size,

			-size, -size, size,
			-size, size, size,
			size, size, size,

			// Back //
			-size, -size, -size,
			-size, size, -size,
			size, size, -size,

			-size, -size, -size,
			size, -size, -size,
			size, size, -size,

			// Left //
			-size, size, size,
			-size, -size, size,
			-size, -size, -size,

			-size, size, size,
			-size, size, -size,
			-size, -size, -size,

			// Right //
			size, size, size,
			size, -size, size,
			size, -size, -size,

			size, size, size,
			size, size, -size,
			size, -size, -size,

			// Up //
			size, size, size,
			-size, size, size,
			-size, size, -size,

			size, size, size,
			size, size, -size,
			-size, size, -size,

			// Down //
			size, -size, size,
			-size, -size, size,
			-size, -size, -size,

			size, -size, size,
			size, -size, -size,
			-size, -size, -size,
		};

		GLubyte colors[] =
		{
			// Front //
			255, 0, 255,
			255, 0, 0,
			0, 255, 0,

			255, 0, 255,
			255, 0, 0,
			0, 255, 0,

			// Back //
			255, 0, 255,
			255, 0, 0,
			0, 255, 0,

			255, 0, 255,
			255, 0, 0,
			0, 255, 0,

			// Left //
			255, 0, 255,
			255, 0, 0,
			0, 255, 0,

			255, 0, 255,
			255, 0, 0,
			0, 255, 0,

			// Right //
			255, 0, 255,
			255, 0, 0,
			0, 255, 0,

			255, 0, 255,
			255, 0, 0,
			0, 255, 0,

			// Up //
			255, 0, 255,
			255, 0, 0,
			0, 255, 0,

			255, 0, 255,
			255, 0, 0,
			0, 255, 0,

			// Down //
			255, 0, 255,
			255, 0, 0,
			0, 255, 0,

			255, 0, 255,
			255, 0, 0,
			0, 255, 0,
		};

		GLfloat texCoords[] =
		{
			// Front //
			0.0f, 1.0f,
			1.0f, 1.0f,
			1.0f, 0.0f,

			0.0f, 1.0f,
			0.0f, 0.0f,
			1.0f, 0.0f,

			// Back //
			1.0f, 0.0f,
			1.0f, 1.0f,
			0.0f, 1.0f,

			1.0f, 0.0f,
			0.0f, 0.0f,
			0.0f, 1.0f,

			// Left //
			1.0f, 0.0f,
			1.0f, 1.0f,
			0.0f, 1.0f,

			1.0f, 0.0f,
			0.0f, 0.0f,
			0.0f, 1.0f,

			// Right //
			0.0f, 1.0f,
			0.0f, 0.0f,
			1.0f, 0.0f,

			0.0f, 1.0f,
			1.0f, 1.0f,
			1.0f, 0.0f,

			// Up //
			1.0f, 0.0f,
			0.0f, 0.0f,
			0.0f, 1.0f,

			1.0f, 0.0f,
			1.0f, 1.0f,
			0.0f, 1.0f,

			// Down //
			1.0f, 0.0f,
			0.0f, 0.0f,
			0.0f, 1.0f,

			1.0f, 0.0f,
			1.0f, 1.0f,
			0.0f, 1.0f,
		};

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, textureID);

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		glVertexPointer(3, GL_FLOAT, 0, vertices);
		glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
		glColorPointer(3, GL_UNSIGNED_BYTE, 0, colors);

		glDrawArrays(GL_TRIANGLES, 0, 36);

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	void updateFmod()
	{
		m_fmodSystem->update();

		//set spectrum for left and right stereo channel
		m_musicChannel->getSpectrum(m_spectrumLeft, SPECTRUM_SIZE, 0, FMOD_DSP_FFT_WINDOW_RECT);

		m_musicChannel->getSpectrum(m_spectrumRight, SPECTRUM_SIZE, 0, FMOD_DSP_FFT_WINDOW_RECT);

		//point the first audio spectrum for both left and right channels
		std::cout << m_spectrumLeft[0] << ", " << m_spectrumRight[0] << std::endl;
	}

	float rot1 = 0.0f;

	void draw(const Matrix& viewMatrix)
	{
		drawAxis(viewMatrix);

		glLoadMatrixf((GLfloat*)viewMatrix.mVal);

		// Show Wireframes. //
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); 

		// ----------------------------------------------------------------------------------------------------
		// Drawing the body. ---//
		
		Matrix theBodyModelMatrix;
		
		Matrix bodyTranslation1 = Matrix::makeTranslationMatrix(Vector(1.5f, 1.85f, 0.5f));
		Matrix bodyMove1 = Matrix::makeTranslationMatrix(Vector(0.0f, 0.15 * sin(rot1 * 2), 0.0f));
		
		theBodyModelMatrix = bodyMove1 * bodyTranslation1;
		
		Matrix bodyMatrix1 = viewMatrix * theBodyModelMatrix;
		glLoadMatrixf((GLfloat*)bodyMatrix1.mVal);
		//drawCube(3.25f, 3.5f, 3.25f, 0.804f, 0.522f, 0.247f);
		drawSphere(0.0f, 360.0f, 360.0f, 180.0f, 2.5f, 20.0f, 20.0f, 1.0f, 1.0f, 0.804f, 0.522f, 0.247f);
		
		// Drawing 2 necks. ---//
		
		for (int x = 0; x < 2; x++)
		{
			Matrix neckPivotTranslation1;
			Matrix neckPivotRotation1;
		
			// Drawing pivot for first neck. ---//
			if (x == 0)
			{
				neckPivotTranslation1 = Matrix::makeTranslationMatrix(Vector(1.0f, 1.75f, -1.0f));
				neckPivotRotation1 = Matrix::makeRotateMatrix(30.0f, Vector(1.0f, 0.0f, 1.0f));
			}
			else if (x == 1)
			{
				neckPivotTranslation1 = Matrix::makeTranslationMatrix(Vector(-1.0f, 1.75f, -1.0f));
				neckPivotRotation1 = Matrix::makeRotateMatrix(30.0f, Vector(1.0f, 0.0f, -1.0f));
			}
		
			Matrix neckMove1 = Matrix::makeTranslationMatrix(Vector(0.0f, 0.1 * sin(rot1 * 2), 0.0f));
		
			Matrix theNeckPivotModelMatrix1 = theBodyModelMatrix * neckMove1 * neckPivotTranslation1 * neckPivotRotation1;
		
			Matrix neckPivotMatrix1 = viewMatrix * theNeckPivotModelMatrix1;
			glLoadMatrixf((GLfloat*)neckPivotMatrix1.mVal);
			drawCube(0.5f, 0.5f, 0.5f, 0.804f, 0.522f, 0.247f);
		
			// First neck. ---//
			Matrix neckTranslation1 = Matrix::makeTranslationMatrix(Vector(0.0f, 2.15f, 0.0f));
		
			Matrix theNeckModelMatrix1 = theNeckPivotModelMatrix1 * neckTranslation1;
		
			Matrix neckMatrix1 = viewMatrix * theNeckModelMatrix1;
			glLoadMatrixf((GLfloat*)neckMatrix1.mVal);
			drawCylinder(4.5f, 0.2f, 100, 0.212f, 0.212f, 0.212f);
		
			// First head. ---//
			Matrix headTranslation1;
			Matrix headRotation1;
			Matrix headMove1;
		
			if (x == 0)
			{
				headTranslation1 = Matrix::makeTranslationMatrix(Vector(-0.25f, 3.0f, 0.0f));
				headRotation1 = Matrix::makeRotateMatrix(-30.0f, Vector(1.0f, 0.0f, 1.0f));
				headMove1 = Matrix::makeRotateMatrix(2.0f * cos(rot1), Vector(0.0f, 0.0f, 1.0f));
			}
			else if (x == 1)
			{
				headTranslation1 = Matrix::makeTranslationMatrix(Vector(0.25f, 3.0f, 0.0f));
				headRotation1 = Matrix::makeRotateMatrix(-30.0f, Vector(1.0f, 0.0f, -1.0f));
				headMove1 = Matrix::makeRotateMatrix(-(2.0f * cos(rot1)), Vector(0.0f, 0.0f, 1.0f));
			}
		
			Matrix theHeadModelMatrix1 = theNeckModelMatrix1 * headMove1 * headTranslation1 * headRotation1;
		
			Matrix headMatrix1 = viewMatrix * theHeadModelMatrix1;
			glLoadMatrixf((GLfloat*)headMatrix1.mVal);
			//drawCube(1.85f, 1.85f, 1.85f, 0.804f, 0.522f, 0.247f);
			drawSphere(0.0f, 360.0f, 360.0f, 180.0f, 1.25f, 20.0f, 20.0f, 1.0f, 1.0f, 0.804f, 0.522f, 0.247f);
		
			// First top beak of the pokemon Doduo. ---//
			Matrix beakTranslation1;
			Matrix beakRotation1;
			Matrix beakRotation2;
			Matrix beakRotation3;
		
			beakTranslation1 = Matrix::makeTranslationMatrix(Vector(0.0f, -0.5f, -1.05f));
			beakRotation1 = Matrix::makeRotateMatrix(90.0f, Vector(1.0, 0.0, 0.0f));
			beakRotation2 = Matrix::makeRotateMatrix(0.0f, Vector(0.0, 0.0, 1.0f));
		
			Matrix theBeakModelMatrix1 = theHeadModelMatrix1 * beakTranslation1 * beakRotation2 * beakRotation1;
		
			Matrix beakMatrix1 = viewMatrix * theBeakModelMatrix1;
			glLoadMatrixf((GLfloat*)beakMatrix1.mVal);
			drawTrianglePyramid(3.0f, 0.45f, 0.35f, 1.000f, 0.871f, 0.678f);
		
			// First bottom beak. ---//
			beakTranslation1 = Matrix::makeTranslationMatrix(Vector(0.0f, -0.5f, -0.95f));
			beakRotation1 = Matrix::makeRotateMatrix(90.0f, Vector(1.0, 0.0, 0.0f));
			beakRotation2 = Matrix::makeRotateMatrix(180.0f, Vector(0.0, 0.0, 1.0f));
		
			Matrix theBeakModelMatrix2 = theHeadModelMatrix1 * beakTranslation1 * beakRotation2 * beakRotation1;
		
			Matrix beakMatrix2 = viewMatrix * theBeakModelMatrix2;
			glLoadMatrixf((GLfloat*)beakMatrix2.mVal);
			drawTrianglePyramid(2.5f, 0.35f, 0.25f, 1.000f, 0.871f, 0.678f);
		
			// Drawing the eyes of the pokemon Doduo. ---//
			for (int e = 0; e < 2; e++)
			{
				Matrix eyeTranslate1;
				Matrix eyeRotate1;
		
				if (e == 0)
				{
					eyeTranslate1 = Matrix::makeTranslationMatrix(Vector(0.5f, 0.15f, -0.95f));
					eyeRotate1 = Matrix::makeRotateMatrix(90.0f, Vector(1.0, 0.0, 0.0f));
				}
				else if (e == 1)
				{
					eyeTranslate1 = Matrix::makeTranslationMatrix(Vector(-0.5f, 0.15f, -0.95f));
					eyeRotate1 = Matrix::makeRotateMatrix(90.0f, Vector(1.0, 0.0, 0.0f));
				}
		
				Matrix theEyeModelMatrix1 = theHeadModelMatrix1 * eyeTranslate1 * eyeRotate1;
		
				Matrix eyeMatrix1 = viewMatrix * theEyeModelMatrix1;
				glLoadMatrixf((GLfloat*)eyeMatrix1.mVal);
				//drawCylinder(0.05f, 0.35f, 100, 0.1f, 0.1f, 0.1f);
				drawSphere(0.0f, 360.0f, 360.0f, 180.0f, 0.4f, 20.0f, 20.0f, 1.0f, 1.0f, 0.1f, 0.1f, 0.1f);
		
				// Drawing the shiny effect. ---//
				Matrix shinyEyeTranslate1 = Matrix::makeTranslationMatrix(Vector(0.05f, 0.2f, 0.1f));
		
				Matrix theShinyModelMatrix1 = theEyeModelMatrix1 * shinyEyeTranslate1;
		
				Matrix shinyEyeMatrix1 = viewMatrix * theShinyModelMatrix1;
				glLoadMatrixf((GLfloat*)shinyEyeMatrix1.mVal);
				//drawCylinder(0.05f, 0.15f, 100, 0.9f, 0.9f, 0.9f);
				drawSphere(0.0f, 360.0f, 360.0f, 180.0f, 0.2f, 20.0f, 20.0f, 1.0f, 1.0f, 0.9f, 0.9f, 0.9f);
		
				// Second shiny effect. ---//
				Matrix shinyEyeTranslate2 = Matrix::makeTranslationMatrix(Vector(-0.15f, 0.275f, -0.1f));
		
				theShinyModelMatrix1 = theEyeModelMatrix1 * shinyEyeTranslate2;
		
				Matrix shinyEyeMatrix2 = viewMatrix * theShinyModelMatrix1;
				glLoadMatrixf((GLfloat*)shinyEyeMatrix2.mVal);
				//drawCylinder(0.05f, 0.10f, 100, 0.9f, 0.9f, 0.9f);
				drawSphere(0.0f, 360.0f, 360.0f, 180.0f, 0.1f, 20.0f, 20.0f, 1.0f, 1.0f, 0.9f, 0.9f, 0.9f);
			}
		}

		// Leg part. ---//
		Matrix theLegModelMatrix;
		
		rot1 += 0.15;
		//rot1 += 0.01f;
			
		// Drawing both legs. ---//
		for (int j = 0; j < 2; j++)
		{
			// Leg pivot for rotation. ---//
			Matrix legMove1;
		
			if (j == 0)
			{
				legMove1 = Matrix::makeRotateMatrix(-(30.0f * sin(rot1)), Vector(1.0f, 0.0f, 0.0f));
			}
			else if (j == 1)
			{
				legMove1 = Matrix::makeRotateMatrix((30.0f * sin(rot1)), Vector(1.0f, 0.0f, 0.0f));
			}
		
			Matrix legMove2 = Matrix::makeTranslationMatrix(Vector(0.0f, 0.25 * sin(rot1 * 2), 0.0f));
			Matrix legTranslateAmount1 = Matrix::makeTranslationMatrix(Vector(0.0f + j * 3.0f, 0.0f, 0.0f));
			Matrix legRotateAmount1 = Matrix::makeRotateMatrix(15.0f, Vector(1.0f, 0.0f, 0.0f));
		
			theLegModelMatrix = legMove2 * legMove1 * legRotateAmount1 * legTranslateAmount1;
		
			Matrix pivotLegMatrix1 = viewMatrix * theLegModelMatrix;
			glLoadMatrixf((GLfloat*)pivotLegMatrix1.mVal);
			drawCube(0.5f, 0.5f, 0.5f, 0.804f, 0.522f, 0.247f);
		
			// Upper leg. ---//
			Matrix leftUpperLegTranslate1 = Matrix::makeTranslationMatrix(Vector(0.0f, -1.5f, 0.75f));
			Matrix leftUpperLegRotate1 = Matrix::makeRotateMatrix(25.0f, Vector(1.0f, 0.0f, 0.0f));
		
			theLegModelMatrix = theLegModelMatrix * leftUpperLegTranslate1 * leftUpperLegRotate1;
		
			Matrix leftUpperLegMatrix1 = viewMatrix * theLegModelMatrix;
			glLoadMatrixf((GLfloat*)leftUpperLegMatrix1.mVal);
			drawCylinder(3.0f, 0.2f, 100, 1.000f, 0.871f, 0.678f);
		
			// Lower leg. ---//
			Matrix leftLowerLegTranslate1 = Matrix::makeTranslationMatrix(Vector(0.0f, -3.65f, -1.0f));
			Matrix leftLowerLegRotate1 = Matrix::makeRotateMatrix(-25.0f, Vector(1.0f, 0.0f, 0.0f));
		
			theLegModelMatrix = theLegModelMatrix * leftLowerLegTranslate1 * leftLowerLegRotate1;
		
			Matrix leftLowerLegMatrix1 = viewMatrix * theLegModelMatrix;
			glLoadMatrixf((GLfloat*)leftLowerLegMatrix1.mVal);
			drawCylinder(5.0f, 0.225f, 100, 1.000f, 0.871f, 0.678f);
		
			// Ball of foot or ankle. ---//
			Matrix ankleModelMatrix1;
			Matrix moveFeet1;
		
			if (j == 0)
			{
				moveFeet1 = Matrix::makeRotateMatrix((-25.0f * sin(rot1)), Vector(1.0f, 0.0f, 0.0f));
			}
			else if (j == 1)
			{
				moveFeet1 = Matrix::makeRotateMatrix(-(-25.0f * sin(rot1)), Vector(1.0f, 0.0f, 0.0f));
			}
		
			Matrix ankleTranslate1 = Matrix::makeTranslationMatrix(Vector(0.0f, -2.5f, 0.0f));
			Matrix ankleRotate1 = Matrix::makeRotateMatrix(90.0f, Vector(1.0f, 0.0f, 0.0f));
		
			ankleModelMatrix1 = theLegModelMatrix * ankleTranslate1 * ankleRotate1 * moveFeet1;
		
			Matrix ankleLegMatrix1 = viewMatrix * ankleModelMatrix1;
			glLoadMatrixf((GLfloat*)ankleLegMatrix1.mVal);
			drawCube(0.5f, 0.5f, 0.5f, 1.000f, 0.871f, 0.678f);
		
			// 3 main toes for the Duduo pokemon. //
			for (int f = 0; f < 3; f++)
			{
				Matrix feetTranslate1;
				Matrix feetRotate1;
		
				// First feet position. ---//
				if (f == 0)
				{
					feetTranslate1 = Matrix::makeTranslationMatrix(Vector(-0.5f, 1.0f, 0.0f));
					feetRotate1 = Matrix::makeRotateMatrix(-30.0f, Vector(0.0f, 0.0f, 1.0f));
				}
				else if (f == 1)
				{
					feetTranslate1 = Matrix::makeTranslationMatrix(Vector(0.0f, 1.0f, 0.0f));
					feetRotate1 = Matrix::makeRotateMatrix(0.0f, Vector(0.0f, 0.0f, 1.0f));
				}
				else if (f == 2)
				{
					feetTranslate1 = Matrix::makeTranslationMatrix(Vector(0.5f, 1.0f, 0.0f));
					feetRotate1 = Matrix::makeRotateMatrix(30.0f, Vector(0.0f, 0.0f, 1.0f));
				}
		
				Matrix theToeModelMatrix1 = ankleModelMatrix1 * feetTranslate1 * feetRotate1;
		
				Matrix feetMatrix1 = viewMatrix * theToeModelMatrix1;
				glLoadMatrixf((GLfloat*)feetMatrix1.mVal);
				drawCylinder(2.0f, 0.185f, 100, 1.000f, 0.871f, 0.678f);
		
				// First toe position. ---//
				Matrix toeTranslate1 = Matrix::makeTranslationMatrix(Vector(0.0f, 1.5f, 0.0f));
		
				theToeModelMatrix1 = theToeModelMatrix1 * toeTranslate1;
		
				Matrix feetMatrix2 = viewMatrix * theToeModelMatrix1;
				glLoadMatrixf((GLfloat*)feetMatrix2.mVal);
				drawCone(1.0f, 0.2f, 100, 0.863f, 0.863f, 0.863f);
			}
			// ----------------------------------------------------------------------------------------------------
		}
			
		// Old code for drawing the feet //

		//// Second feet position. ---//
		//Matrix feetTranslate2 = Matrix::makeTranslationMatrix(Vector(0.0f, 0.0f, -1.0f));
		//Matrix feetRotate3 = Matrix::makeRotateMatrix(0.0f, Vector(0.0f, 1.0f, 0.0f));
		//
		//Matrix theToeModelMatrix2 = theLegModelMatrix * feetTranslate2 * feetRotate3 * feetRotate1;
		//
		//Matrix feetMatrix3 = viewMatrix * theToeModelMatrix2;
		//glLoadMatrixf((GLfloat*)feetMatrix3.mVal);
		//drawCylinder(2.0f, 0.185f);
		//
		//// Second toe position. ---//
		//theToeModelMatrix2 = theToeModelMatrix2 * toeTranslate1;
		//
		//Matrix feetMatrix4 = viewMatrix * theToeModelMatrix2;
		//glLoadMatrixf((GLfloat*)feetMatrix4.mVal);
		//drawCone(1.0f, 0.2f);
		//
		//// Third feet position. ---//
		//Matrix feetTranslate3 = Matrix::makeTranslationMatrix(Vector(0.5f, 0.0f, -1.0f));
		//Matrix feetRotate4 = Matrix::makeRotateMatrix(30.0f, Vector(0.0f, 1.0f, 0.0f));
		//
		//Matrix theToeModelMatrix3 = theLegModelMatrix * feetTranslate3 * feetRotate4 * feetRotate1;
		//
		//Matrix feetMatrix5 = viewMatrix * theToeModelMatrix3;
		//glLoadMatrixf((GLfloat*)feetMatrix5.mVal);
		//drawCylinder(2.0f, 0.185f);
		//
		//// Third toe position. ---//
		//theToeModelMatrix3 = theToeModelMatrix3 * toeTranslate1;
		//
		//Matrix feetMatrix6 = viewMatrix * theToeModelMatrix3;
		//glLoadMatrixf((GLfloat*)feetMatrix6.mVal);
		//drawCone(1.0f, 0.2f);
		//
		//Matrix coneTranslation1 = Matrix::makeTranslationMatrix(Vector(i, 0.0f, 0.0f));
		//Matrix coneRotation1 = Matrix::makeRotateMatrix(90.0f, Vector(1.0f, 0.0f, 0.0f));
		//Matrix coneMatrix1 = viewMatrix * coneTranslation1 * coneRotation1;
		//glLoadMatrixf((GLfloat*)coneMatrix1.mVal);
		//drawCone(1.0f, 0.2f);
		//
		//// Old code to draw a sphere //
		//
		//GLUquadric *quadric;
		//quadric = gluNewQuadric();
		//gluSphere(quadric, 0.5, 30, 30);
		//} 
		//// ----------------------------------------------------------------------------------------------------
		//
		//// 3 main toes on right side. ---//
		//for (int i = 0; i < 3; i++)
		//{
		//Matrix feetTranslate1 = Matrix::makeTranslationMatrix(Vector(i * 0.5f + 5.0f, 0.0f, 0.0f));
		//Matrix toeTranslate1 = Matrix::makeTranslationMatrix(Vector(0.0f, 1.5f, 0.0f));
		//Matrix feetRotate1 = Matrix::makeRotateMatrix(90.0f, Vector(1.0f, 0.0f, 0.0f));
		//Matrix feetRotate2 = Matrix::makeRotateMatrix((30.0f * i) - 30.0f, Vector(0.0f, 1.0f, 0.0f));
		//
		//Matrix modelMatrix = feetTranslate1 * feetRotate2 * feetRotate1;
		//Matrix feetMatrix1 = viewMatrix * modelMatrix;
		//glLoadMatrixf((GLfloat*)feetMatrix1.mVal);
		//drawCylinder(2.0f, 0.18f);
		//
		//modelMatrix = modelMatrix * toeTranslate1;
		//Matrix feetMatrix2 = viewMatrix * modelMatrix;
		//glLoadMatrixf((GLfloat*)feetMatrix2.mVal);
		//drawCone(1.0f, 0.2f);
		//}
		//

		// Old code for drawing the neck. //

		// Drawing pivot for second neck. ---//
		//Matrix neckPivotTranslation2 = Matrix::makeTranslationMatrix(Vector(-1.0f, 1.75f, -1.0f));
		//Matrix neckPivotRotation2 = Matrix::makeRotateMatrix(30.0f, Vector(1.0f, 0.0f, -1.0f));
		//
		//Matrix theNeckPivotModelMatrix2 = theBodyModelMatrix * neckPivotTranslation2 * neckPivotRotation2;
		//
		//Matrix neckPivotMatrix2 = viewMatrix * theNeckPivotModelMatrix2;
		//glLoadMatrixf((GLfloat*)neckPivotMatrix2.mVal);
		//drawCube(0.5f, 0.5f, 0.5f);
		//
		// Second neck. ---//
		//Matrix neckTranslation2 = Matrix::makeTranslationMatrix(Vector(0.0f, 2.65f, 0.0f));
		//
		//Matrix theNeckModelMatrix2 = theNeckPivotModelMatrix2 * neckTranslation2;
		//
		//Matrix neckMatrix2 = viewMatrix * theNeckModelMatrix2;
		//glLoadMatrixf((GLfloat*)neckMatrix2.mVal);
		//drawCylinder(5.0f, 0.2f);

	}

	// Draw Functions //
		
	//drawVertexCube(1, 3.0f);
	//drawCube();
	//drawPyramid();
	//drawCylinder(2.0f, 1.0f);
	//drawCone(2.0f, 1.0f);

	// Oval Orbiter //

	//rot1 += 0.1f;
	//rot2 += 0.05f;
	//
	//glLoadMatrixf((GLfloat*)ovalOrbiter(viewMatrix, rot2, rot1, 2.0f, 5.0f, 2.5f).mVal);
	//drawCube();
		
	// "DNA" Animation //

	//float radius = 2.0f;
	//float offsetY = 0.2f;
	//float offsetAngle = 30.0f;
	//int cubeAmount = 50;
	//
	//for (int i = 0; i < cubeAmount; i++)
	//{
	//	//Method 1 - Translate + Rotation
	//	Matrix translationCube = Matrix::makeTranslationMatrix(Vector(radius, offsetY * i, 0));
	//	Matrix rotationCube = Matrix::makeRotateMatrix(cubeBaseAngle - (i * offsetAngle), Vector(0, 1, 0));
	//	Matrix cubeMatrix = viewMatrix * rotationCube * translationCube;
	//	//glLoadMatrixf((GLfloat*)cubeMatrix.mVal);
	//
	//	//Method 2 - Use circle formula
	//	Matrix magicMatrix = Matrix::makeTranslationMatrix(Vector(cosf((cubeBaseAngle - (i * offsetAngle)) * PI / 180.0f) * radius, offsetY * i, sinf((cubeBaseAngle - (i * offsetAngle)) * PI / 180.0f) * radius));
	//	glLoadMatrixf((GLfloat*)(viewMatrix * magicMatrix).mVal);
	//
	//	drawCube(0.5f);
	//}
	//cubeBaseAngle += 0.05f;

	// Cone Transformation //

	/*Matrix coneTranslation = Matrix::makeTranslationMatrix(Vector(0.0f, 4.0f, 4.0f));
	Matrix coneRotation = Matrix::makeRotateMatrix(90.0f, Vector(1.0f, 1.0f, 1.0f));
	Matrix coneMatrix = viewMatrix * coneRotation;
	glLoadMatrixf((GLfloat*)coneMatrix.mVal);
	drawCone(1.0f, 0.2f);*/

	// Example for sequences of transforms //

	//Matrix modelMatrix1;
	//Matrix viewSpaceMatrix = viewMatrix * modelMatrix1;
	//glLoadMatrixf((GLfloat*)viewSpaceMatrix.mVal);
	//drawCube(); //This cube is affected by the last applied matrix
	//
	//Matrix modelMatrix2;
	//Matrix viewSpaceMatrix = viewMatrix * modelMatrix2;
	//glLoadMatrixf((GLfloat*)viewSpaceMatrix.mVal);
	//drawCube(); //This cube is affected by the last applied matrix
	
	// Oval Orbit Pattern //

	//Matrix ovalOrbiter(const Matrix& viewMatrix, float rot1, float rot2, float amplitude, float l1, float l2)
	//{
	//	// Matrix Transformation //
	//	Matrix translate1 = Matrix::makeTranslationMatrix(Vector(0.0f, amplitude * sin(rot1 * PI / 180.0f), 0.0f));
	//	Matrix rotate1 = Matrix::makeRotateMatrix(rot2, Vector(0.0f, 1.0f, 0.0f));
	//	Matrix translate2 = Matrix::makeTranslationMatrix(Vector(l1 * cos(rot2 * PI / 180.0f), 0.0f, l2 * sin(rot2 * PI / 180.0f)));
	//	Matrix scale1 = Matrix::makeScaleMatrix(Vector(2.0f, 1.0f, 3.0f));
	//
	//	// NOTE on OpenGL Matrix model //
	//	// Screen = Proj * View * Model //
	//	// Model = TransformC(3rd) * TransformB(2nd) * TransformA(1st) (Transform could be Rotate, Scale, Translate, etc) //
	//
	//	// Perform Model Transformation //
	//
	//	Matrix modelMatrix = translate2;
	//
	//	Matrix viewSpaceMatrix = viewMatrix * modelMatrix;
	//
	//	return viewSpaceMatrix;
	//}
	//
	//float rot1 = 0.0f;
	//float rot2 = 0.0f;
	//
	//float cubeBaseAngle = 0.0f;

	// DepthMask //

	//void drawDepthMask(const Matrix& viewMatrix)
	//{
	//	// Show Wireframes. //
	//	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); 
	//
	//	drawAxis(viewMatrix);
	//
	//	glLoadMatrixf((GLfloat*)viewMatrix.mVal);
	//	
	//	glDepthMask(true);
	//
	//	Matrix WallTranslate1 = Matrix::makeTranslationMatrix(Vector(0.0f, 0.0f, 0.0f));
	//	Matrix WallTranslate2 = Matrix::makeTranslationMatrix(Vector(0.0f, 0.0f, -10.0f));
	//
	//	Matrix theWallMatrix1 = WallTranslate1;
	//	Matrix modelMatrix1 = viewMatrix * theWallMatrix1;
	//	glLoadMatrixf((GLfloat*)modelMatrix1.mVal);
	//	DrawWall(5.0f);
	//
	//	Matrix theWallMatrix2 = WallTranslate2;
	//	Matrix modelMatrix2 = viewMatrix * theWallMatrix2;
	//	glLoadMatrixf((GLfloat*)modelMatrix2.mVal);
	//	DrawWall(5.0f);
	//
	//	// Alpha Blend //
	//	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//
	//	// Additive Blend //
	//	glBlendFunc(GL_ONE, GL_ONE);
	//
	//	glDepthMask(false);
	//
	//	// Multiply Blend (Also called modulate) ---//
	//	//glBlendFunc(GL_ZERO, GL_SRC_COLOR);
	//	//glBlendFunc(GL_DST_COLOR, GL_ZERO);
	//
	//	drawTextureCube(3.0f);
	//
	//	glDepthMask(true);
	//}

};

#endif